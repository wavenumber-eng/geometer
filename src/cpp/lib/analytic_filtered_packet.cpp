#include "geometer/analytic_filtered_packet.h"

#include "analytic_filtered_packet_sequences.h"
#include "analytic_result_packet_records_internal.h"
#include "geometer/sha256.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <numeric>
#include <tuple>
#include <utility>
#include <vector>

namespace geometer
{
namespace
{

constexpr std::uint32_t kNone = std::numeric_limits<std::uint32_t>::max();
constexpr std::uint64_t kFixedPacketWork = 1024;
constexpr std::uint64_t kFixedPacketBytes = 4096;
using analytic_packet_detail::canonicalize_sequences;
using analytic_packet_detail::CanonicalSequences;
using analytic_packet_detail::checked_add;
using analytic_packet_detail::checked_multiply;
using analytic_packet_detail::SequenceRange;
using analytic_packet_detail::sort_units;
using analytic_packet_detail::WorkBudget;

auto source_key(const AnalyticSourceReference& source) noexcept
{
    return std::tuple{static_cast<std::uint16_t>(source.kind),
                      static_cast<std::uint16_t>(source.role), source.operand_id, source.primary_id,
                      source.secondary_id};
}

bool source_less(const AnalyticSourceReference& left, const AnalyticSourceReference& right) noexcept
{
    return source_key(left) < source_key(right);
}

bool source_equal(const AnalyticSourceReference& left,
                  const AnalyticSourceReference& right) noexcept
{
    return source_key(left) == source_key(right);
}

enum class SourceExpectation : std::uint8_t
{
    geometry = 0,
    subtraction = 1,
};

struct SourceUse
{
    std::uint8_t buffer = 0;
    SourceExpectation expectation = SourceExpectation::geometry;
    std::uint32_t begin = 0;
    std::uint32_t count = 0;
};

bool source_use_less(const SourceUse& left, const SourceUse& right) noexcept
{
    return std::tie(left.buffer, left.expectation, left.begin, left.count) <
           std::tie(right.buffer, right.expectation, right.begin, right.count);
}

bool source_use_equal(const SourceUse& left, const SourceUse& right) noexcept
{
    return !source_use_less(left, right) && !source_use_less(right, left);
}

struct SourceOccurrence
{
    AnalyticSourceReference source;
    std::uint32_t descriptor = 0;
    std::uint32_t offset = 0;
    std::uint32_t source_index = 0;
};

struct EventTemp
{
    AnalyticOperandEventRecord record;
    SequenceRange references;
    std::uint32_t sequence_rank = 0;
};

struct RawReference
{
    std::uint32_t event = 0;
    std::uint64_t value = 0;
};

struct SourceTables
{
    std::vector<AnalyticSourceReference> sources;
    std::vector<AnalyticPacketSourceSetRecord> sets;
    std::vector<std::uint32_t> indices;
    std::vector<std::uint32_t> handles;
    std::uint64_t logical_bytes = 0;
};

struct OperandInfo
{
    std::uint64_t operand_id = 0;
    std::uint64_t stage_id = 0;
    std::uint8_t operation = 0;
    std::uint16_t geometry_kind = 0;
    std::uint32_t geometry_index = 0;
};

struct SegmentBinding
{
    std::uint64_t segment_id = 0;
    std::uint64_t curve_id = 0;
    std::uint64_t operand_id = 0;
    std::uint8_t kind = 0;
};

struct OperandSource
{
    std::uint64_t operand_id = 0;
    AnalyticSourceReference source;
};

static_assert(sizeof(SourceUse) <= 16);
static_assert(sizeof(SourceOccurrence) <= 48);
static_assert(sizeof(EventTemp) <= 64);
static_assert(sizeof(RawReference) <= 16);
static_assert(sizeof(OperandInfo) <= 32);
static_assert(sizeof(SegmentBinding) <= 32);
static_assert(sizeof(OperandSource) <= 40);
static_assert(sizeof(AnalyticSourceReference) <= 32);
static_assert(sizeof(AnalyticNormalizedVertexNm) <= 24);
static_assert(sizeof(AnalyticNormalizedFragmentNm) <= 32);
static_assert(sizeof(AnalyticNormalizedRing) <= 32);
static_assert(sizeof(AnalyticNormalizedRegion) <= 8);
static_assert(sizeof(AnalyticFilteredBoundaryLineage) <= 32);
static_assert(sizeof(AnalyticFilteredVertexLineage) <= 16);
static_assert(sizeof(AnalyticFilteredRegionLineage) <= 16);
static_assert(sizeof(AnalyticResultVertexRecord) <= 32);
static_assert(sizeof(AnalyticDirectedFragmentRecord) <= 48);
static_assert(sizeof(AnalyticResultRingRecord) <= 32);
static_assert(sizeof(AnalyticResultRegionRecord) <= 24);
static_assert(sizeof(AnalyticOperandEventRecord) <= 48);
static_assert(sizeof(AnalyticJobResultRecord) <= 48);

std::uint64_t search_units(std::uint64_t count) noexcept
{
    std::uint64_t levels = 1;
    for (std::uint64_t value = count; value > 1; value = (value + 1) >> 1U)
        ++levels;
    return levels;
}

bool add_array_bytes(std::uint64_t count, std::uint64_t bytes_per_item,
                     std::uint64_t& total) noexcept
{
    std::uint64_t term = 0;
    return checked_multiply(count, bytes_per_item, term) && checked_add(total, term, total);
}

const OperandInfo* find_operand(const std::vector<OperandInfo>& operands,
                                std::uint64_t operand_id) noexcept
{
    const auto found = std::lower_bound(operands.begin(), operands.end(), operand_id,
                                        [](const OperandInfo& value, std::uint64_t id)
                                        { return value.operand_id < id; });
    return found != operands.end() && found->operand_id == operand_id ? &*found : nullptr;
}

bool valid_source_shape(const AnalyticSourceReference& source) noexcept
{
    if (source.operand_id == 0 || source.primary_id == 0)
        return false;
    if (source.kind == AnalyticSourceKind::authored_segment_curve)
        return source.secondary_id != 0 &&
               (source.role == AnalyticSourceRole::authored_line ||
                source.role == AnalyticSourceRole::authored_circular_arc);
    if (source.kind == AnalyticSourceKind::subtractive_operand_effect)
        return source.role == AnalyticSourceRole::none && source.secondary_id == 0;
    if (source.kind != AnalyticSourceKind::compact_feature_role)
        return false;
    switch (source.role)
    {
    case AnalyticSourceRole::primitive_outer_circle:
    case AnalyticSourceRole::primitive_inner_circle:
    case AnalyticSourceRole::capsule_left_line:
    case AnalyticSourceRole::capsule_end_cap:
    case AnalyticSourceRole::capsule_right_line:
    case AnalyticSourceRole::capsule_start_cap:
        return source.secondary_id == 0;
    default:
        return false;
    }
}

bool bind_geometry_source(const AnalyticSourceReference& source,
                          const AnalyticRequestPacketRecords& records,
                          const std::vector<OperandInfo>& operands,
                          const std::vector<SegmentBinding>& segments) noexcept
{
    const OperandInfo* operand = find_operand(operands, source.operand_id);
    if (operand == nullptr || !valid_source_shape(source) ||
        source.kind == AnalyticSourceKind::subtractive_operand_effect)
        return false;
    if (source.kind == AnalyticSourceKind::authored_segment_curve)
    {
        if (operand->geometry_kind != 1)
            return false;
        const auto found = std::lower_bound(segments.begin(), segments.end(), source.primary_id,
                                            [](const SegmentBinding& value, std::uint64_t id)
                                            { return value.segment_id < id; });
        return found != segments.end() && found->segment_id == source.primary_id &&
               found->curve_id == source.secondary_id && found->operand_id == source.operand_id &&
               ((found->kind == 1 && source.role == AnalyticSourceRole::authored_line) ||
                (found->kind == 2 && source.role == AnalyticSourceRole::authored_circular_arc));
    }
    std::uint64_t feature = 0;
    switch (operand->geometry_kind)
    {
    case 2:
        if (operand->geometry_index >= records.disks.size() ||
            source.role != AnalyticSourceRole::primitive_outer_circle)
            return false;
        feature = records.disks[operand->geometry_index].feature_id;
        break;
    case 3:
        if (operand->geometry_index >= records.annuli.size() ||
            (source.role != AnalyticSourceRole::primitive_outer_circle &&
             source.role != AnalyticSourceRole::primitive_inner_circle))
            return false;
        feature = records.annuli[operand->geometry_index].feature_id;
        break;
    case 4:
        if (operand->geometry_index >= records.capsules.size() ||
            (source.role != AnalyticSourceRole::capsule_left_line &&
             source.role != AnalyticSourceRole::capsule_end_cap &&
             source.role != AnalyticSourceRole::capsule_right_line &&
             source.role != AnalyticSourceRole::capsule_start_cap))
            return false;
        feature = records.capsules[operand->geometry_index].feature_id;
        break;
    default:
        return false;
    }
    return source.primary_id == feature;
}

bool bind_subtraction_source(const AnalyticSourceReference& source,
                             const std::vector<OperandInfo>& operands) noexcept
{
    const OperandInfo* operand = find_operand(operands, source.operand_id);
    return operand != nullptr && operand->operation == 2 &&
           source.kind == AnalyticSourceKind::subtractive_operand_effect &&
           source.role == AnalyticSourceRole::none && source.primary_id == operand->stage_id &&
           source.secondary_id == 0;
}

struct CompactInput
{
    std::vector<AnalyticNormalizedVertexNm> vertices;
    std::vector<AnalyticNormalizedFragmentNm> fragments;
    std::vector<std::uint32_t> ring_fragments;
    std::vector<AnalyticNormalizedRing> rings;
    std::vector<AnalyticNormalizedRegion> regions;
    std::vector<std::uint32_t> old_vertex_to_normalized;
    std::vector<std::uint32_t> old_boundary_to_normalized;
    std::vector<std::uint32_t> old_ring_to_normalized;
    std::vector<std::uint32_t> old_region_to_normalized;
    std::vector<std::uint32_t> old_ring_parents;
    std::vector<AnalyticFilteredBoundaryLineage> boundaries;
    std::vector<AnalyticFilteredVertexLineage> vertex_lineage;
    std::vector<AnalyticFilteredRegionLineage> region_lineage;
    std::vector<AnalyticSourceReference> lineage_sources;
    std::vector<AnalyticFilteredOperandOutcomeEvent> events;
    std::vector<AnalyticFilteredTaggedResultReference> event_references;
    std::vector<AnalyticSourceReference> event_sources;
};

std::uint64_t compact_logical_bytes(const CompactInput& input) noexcept
{
    return kFixedPacketBytes + input.vertices.size() * 24ULL + input.fragments.size() * 32ULL +
           input.ring_fragments.size() * 4ULL + input.rings.size() * 32ULL +
           input.regions.size() * 8ULL + input.old_vertex_to_normalized.size() * 4ULL +
           input.old_boundary_to_normalized.size() * 4ULL +
           input.old_ring_to_normalized.size() * 4ULL +
           input.old_region_to_normalized.size() * 4ULL + input.old_ring_parents.size() * 4ULL +
           input.boundaries.size() * 32ULL + input.vertex_lineage.size() * 16ULL +
           input.region_lineage.size() * 16ULL + input.lineage_sources.size() * 32ULL +
           input.events.size() * 32ULL + input.event_references.size() * 8ULL +
           input.event_sources.size() * 32ULL;
}

bool move_compact(AnalyticFilteredNormalizationResult& normalization, CompactInput& output)
{
    const auto& material_rings = normalization.outcomes.lineage.regions.rings;
    try
    {
        output.old_ring_parents.reserve(material_rings.size());
        for (const auto& ring : material_rings)
            output.old_ring_parents.push_back(ring.parent_ring);
    }
    catch (const std::bad_alloc&)
    {
        return false;
    }
    output.vertices = std::move(normalization.vertices);
    output.fragments = std::move(normalization.fragments);
    output.ring_fragments = std::move(normalization.ring_fragments);
    output.rings = std::move(normalization.rings);
    output.regions = std::move(normalization.regions);
    output.old_vertex_to_normalized = std::move(normalization.old_vertex_to_normalized);
    output.old_boundary_to_normalized = std::move(normalization.old_boundary_to_normalized);
    output.old_ring_to_normalized = std::move(normalization.old_ring_to_normalized);
    output.old_region_to_normalized = std::move(normalization.old_region_to_normalized);
    output.boundaries = std::move(normalization.outcomes.lineage.boundaries);
    output.vertex_lineage = std::move(normalization.outcomes.lineage.vertices);
    output.region_lineage = std::move(normalization.outcomes.lineage.region_lineage);
    output.lineage_sources = std::move(normalization.outcomes.lineage.source_references);
    output.events = std::move(normalization.outcomes.events);
    output.event_references = std::move(normalization.outcomes.result_references);
    output.event_sources = std::move(normalization.outcomes.source_references);
    normalization = {};
    return true;
}

struct IncidentKey
{
    std::uint8_t side = 0;
    std::int64_t other_x = 0;
    std::int64_t other_y = 0;
    std::uint8_t kind = 0;
    std::uint8_t direction = 0;
    bool major_arc = false;
    std::uint64_t radius_nm = 0;
};

auto incident_key(const IncidentKey& value) noexcept
{
    return std::tuple{value.side,      value.other_x,   value.other_y,  value.kind,
                      value.direction, value.major_arc, value.radius_nm};
}

struct IncidentOccurrence
{
    std::uint32_t vertex = 0;
    IncidentKey key;
    std::uint32_t label = 0;
};

static_assert(sizeof(IncidentOccurrence) <= 64);

std::size_t least_rotation(const std::vector<std::uint32_t>& values, std::size_t begin,
                           std::size_t count) noexcept
{
    std::size_t left = 0;
    std::size_t right = 1;
    std::size_t offset = 0;
    while (left < count && right < count && offset < count)
    {
        const std::uint32_t a = values[begin + (left + offset) % count];
        const std::uint32_t b = values[begin + (right + offset) % count];
        if (a == b)
        {
            ++offset;
            continue;
        }
        if (a > b)
        {
            left += offset + 1;
            if (left == right)
                ++left;
        }
        else
        {
            right += offset + 1;
            if (left == right)
                ++right;
        }
        offset = 0;
    }
    return std::min(left, right);
}

class PacketBuilder
{
  public:
    PacketBuilder(const AnalyticRequestPacketRecords& records, std::uint32_t job_index,
                  const AnalyticFilteredGeometry& geometry,
                  const std::vector<AnalyticCurvePair>& pairs, const AnalyticSolverLimits& limits)
        : records_(records), job_index_(job_index), geometry_(geometry), pairs_(pairs),
          limits_(limits)
    {
        budget_.telemetry = &result_.telemetry;
    }

    AnalyticFilteredJobPacketResult build()
    {
        if (!pre_admit())
        {
            if (result_.error == AnalyticFilteredPacketError::resource_limit_exceeded &&
                job_index_ < records_.jobs.size())
            {
                result_.error = AnalyticFilteredPacketError::none;
                build_failed_packet(65'547);
                finish_telemetry();
            }
            return std::move(result_);
        }
        if (geometry_.curves.empty())
        {
            const auto& job = records_.jobs[job_index_];
            for (std::uint32_t local = 0; local < job.stage_count; ++local)
                if (records_.stages[job.stage_begin + local].operand_count != 0)
                {
                    result_.error = AnalyticFilteredPacketError::invalid_argument;
                    finish_telemetry();
                    return std::move(result_);
                }
            build_empty_success_packet();
            finish_telemetry();
            return std::move(result_);
        }
        AnalyticSolverLimits normalization_limits = limits_;
        normalization_limits.predicate_calls -= reserved_work_ + preflight_work_;
        normalization_limits.working_memory_bytes -= reserved_memory_;
        AnalyticFilteredNormalizationResult normalization = build_analytic_filtered_normalization(
            records_, job_index_, geometry_, pairs_, normalization_limits);
        result_.normalization_error = normalization.error;
        result_.telemetry.normalization_work_units = normalization.telemetry.predicate_calls;
        result_.telemetry.normalization_peak_working_memory_bytes =
            normalization.telemetry.peak_working_memory_bytes;
        result_.telemetry.algebraic_fallback_calls =
            normalization.telemetry.algebraic_fallback_calls;
        if (normalization.error != AnalyticFilteredNormalizationError::none)
        {
            if (normalization.error == AnalyticFilteredNormalizationError::invalid_argument)
            {
                result_.error = AnalyticFilteredPacketError::invalid_argument;
                finish_telemetry();
                return std::move(result_);
            }
            const std::uint32_t code =
                normalization.error == AnalyticFilteredNormalizationError::resource_limit_exceeded
                    ? 65'547
                : normalization.error ==
                        AnalyticFilteredNormalizationError::normalization_error_exceeded
                    ? 65'543
                    : 65'544;
            build_failed_packet(code);
            finish_telemetry();
            return std::move(result_);
        }
        std::uint64_t compact_transfer_bytes =
            result_.telemetry.normalization_peak_working_memory_bytes;
        if (!add_array_bytes(normalization.outcomes.lineage.regions.rings.size(), 4,
                             compact_transfer_bytes) ||
            compact_transfer_bytes > limits_.working_memory_bytes)
            return resource_failure();
        result_.telemetry.peak_working_memory_bytes =
            std::max(result_.telemetry.peak_working_memory_bytes, compact_transfer_bytes);
        if (!move_compact(normalization, input_))
            return resource_failure();
        compact_bytes_ = std::max(compact_logical_bytes(input_), compact_transfer_bytes);
        budget_.limit =
            limits_.predicate_calls - result_.telemetry.normalization_work_units - preflight_work_;
        if (reserved_work_ > budget_.limit)
            return resource_failure();
        try
        {
            if (!prepare_bindings() || !prepare_source_uses() || !build_source_tables() ||
                !publish_vertices_and_fragments() || !publish_rings_and_regions() ||
                !publish_events() || !finalize_success())
            {
                if (result_.error == AnalyticFilteredPacketError::none)
                    result_.error = AnalyticFilteredPacketError::invalid_argument;
                clear_publication();
                finish_telemetry();
                return std::move(result_);
            }
        }
        catch (const std::bad_alloc&)
        {
            return resource_failure();
        }
        finish_telemetry();
        return std::move(result_);
    }

  private:
    std::uint64_t source_tables_persistent_bytes() const noexcept
    {
        return source_tables_.logical_bytes;
    }

    bool persistent_bytes(std::uint64_t& bytes) const noexcept
    {
        bytes = compact_bytes_;
        return checked_add(bytes, binding_bytes_, bytes) &&
               checked_add(bytes, source_use_bytes_, bytes) &&
               checked_add(bytes, source_tables_persistent_bytes(), bytes) &&
               checked_add(bytes, publication_bytes_, bytes);
    }

    bool check_phase(std::uint64_t scratch_bytes)
    {
        std::uint64_t bytes = 0;
        if (!persistent_bytes(bytes) || !checked_add(bytes, scratch_bytes, bytes) ||
            bytes > limits_.working_memory_bytes)
            return fail_resource();
        result_.telemetry.peak_working_memory_bytes =
            std::max(result_.telemetry.peak_working_memory_bytes, bytes);
        return true;
    }

    bool initialize_publication_bytes()
    {
        publication_bytes_ = 0;
        for (const auto [count, item_bytes] :
             {std::pair<std::uint64_t, std::uint64_t>{input_.vertices.size(), 32},
              {input_.fragments.size(), 48},
              {input_.rings.size(), 32},
              {input_.ring_fragments.size(), 4},
              {input_.regions.size(), 24},
              {input_.event_references.size(), 8},
              {input_.events.size(), 48},
              {input_.vertices.size(), 4},
              {input_.fragments.size(), 4},
              {input_.rings.size(), 4},
              {input_.regions.size(), 4},
              {input_.old_vertex_to_normalized.size(), 4},
              {input_.old_boundary_to_normalized.size(), 4},
              {input_.old_ring_to_normalized.size(), 4},
              {input_.old_region_to_normalized.size(), 4},
              {1, 48}})
            if (!add_array_bytes(count, item_bytes, publication_bytes_))
                return fail_resource();
        return check_phase(0);
    }

    bool pre_admit()
    {
        if (!analytic_solver_limits_within_hard_ceilings(limits_) ||
            job_index_ >= records_.jobs.size() ||
            geometry_.curves.size() != geometry_.bounds.size() ||
            geometry_.curves.size() != geometry_.occurrences.size())
        {
            result_.error = AnalyticFilteredPacketError::invalid_argument;
            return false;
        }
        const auto& job = records_.jobs[job_index_];
        if (job.stage_begin > records_.stages.size() ||
            job.stage_count > records_.stages.size() - job.stage_begin)
        {
            result_.error = AnalyticFilteredPacketError::invalid_argument;
            return false;
        }
        preflight_work_ = static_cast<std::uint64_t>(job.stage_count) + 1;
        if (preflight_work_ > limits_.predicate_calls)
            return fail_resource();
        std::uint64_t operands = 0;
        for (std::uint32_t local = 0; local < job.stage_count; ++local)
        {
            const auto& stage = records_.stages[job.stage_begin + local];
            if (stage.stage_id == 0 || (stage.operation != 1 && stage.operation != 2) ||
                stage.operand_begin > records_.operands.size() ||
                stage.operand_count > records_.operands.size() - stage.operand_begin)
            {
                result_.error = AnalyticFilteredPacketError::invalid_argument;
                return false;
            }
            std::uint64_t next = 0;
            if (!checked_add(operands, stage.operand_count, next))
                return fail_resource();
            operands = next;
        }
        std::uint64_t fragments = 0;
        std::uint64_t pair_term = 0;
        if (!checked_multiply(geometry_.curves.size(), 4, fragments) ||
            !checked_multiply(pairs_.size(), 4, pair_term) ||
            !checked_add(fragments, pair_term, fragments))
            return fail_resource();
        std::uint64_t vertices = 0;
        std::uint64_t events = 0;
        std::uint64_t source_uses = 0;
        std::uint64_t twice_fragments = 0;
        if (!checked_multiply(fragments, 2, vertices) || !checked_multiply(operands, 3, events) ||
            !checked_multiply(fragments, 2, twice_fragments) ||
            !checked_add(vertices, twice_fragments, source_uses) ||
            !checked_add(source_uses, twice_fragments, source_uses) ||
            !checked_add(source_uses, events, source_uses))
            return fail_resource();
        std::uint64_t work = kFixedPacketWork;
        std::uint64_t term = 0;
        std::uint64_t twice_vertices = 0;
        if (!checked_multiply(vertices, 2, twice_vertices))
            return fail_resource();
        for (const std::uint64_t count :
             {twice_vertices, vertices, fragments, fragments, twice_fragments, events})
        {
            if (!checked_add(work, sort_units(count), work))
                return fail_resource();
        }
        if (!checked_add(work, source_uses, work))
            return fail_resource();
        std::uint64_t memory = kFixedPacketBytes;
        if (!checked_multiply(source_uses, 24, term) || !checked_add(memory, term, memory) ||
            !checked_multiply(vertices, 40, term) || !checked_add(memory, term, memory) ||
            !checked_multiply(fragments, 96, term) || !checked_add(memory, term, memory) ||
            !checked_multiply(events, 64, term) || !checked_add(memory, term, memory))
            return fail_resource();
        reserved_work_ = work;
        reserved_memory_ = memory;
        result_.telemetry.reserved_packet_work_units = work;
        result_.telemetry.reserved_packet_memory_bytes = memory;
        if (preflight_work_ > limits_.predicate_calls ||
            work > limits_.predicate_calls - preflight_work_ ||
            memory > limits_.working_memory_bytes)
            return fail_resource();
        return true;
    }

    bool fail_resource()
    {
        result_.error = AnalyticFilteredPacketError::resource_limit_exceeded;
        return false;
    }

    AnalyticFilteredJobPacketResult resource_failure()
    {
        clear_publication();
        result_.error = AnalyticFilteredPacketError::none;
        build_failed_packet(65'547);
        finish_telemetry();
        return std::move(result_);
    }

    bool prepare_bindings()
    {
        const auto& job = records_.jobs[job_index_];
        std::uint64_t visits = job.stage_count;
        for (std::uint32_t local = 0; local < job.stage_count; ++local)
            visits += records_.stages[job.stage_begin + local].operand_count;
        if (!budget_.charge(visits))
            return fail_resource();
        const std::uint64_t operand_count = visits - job.stage_count;
        binding_bytes_ = 0;
        if (!add_array_bytes(operand_count, 32, binding_bytes_) ||
            !add_array_bytes(geometry_.occurrences.size(), 32, binding_bytes_) ||
            !add_array_bytes(geometry_.occurrences.size(), 40, binding_bytes_) || !check_phase(0))
            return false;
        operands_.reserve(static_cast<std::size_t>(visits - job.stage_count));
        for (std::uint32_t local = 0; local < job.stage_count; ++local)
        {
            const auto& stage = records_.stages[job.stage_begin + local];
            if (stage.stage_id == 0 || (stage.operation != 1 && stage.operation != 2) ||
                stage.operand_begin > records_.operands.size() ||
                stage.operand_count > records_.operands.size() - stage.operand_begin)
                return false;
            for (std::uint32_t offset = 0; offset < stage.operand_count; ++offset)
            {
                const auto& operand = records_.operands[stage.operand_begin + offset];
                operands_.push_back({operand.operand_id, stage.stage_id, stage.operation,
                                     operand.geometry_kind, operand.geometry_index});
            }
        }
        if (!budget_.charge_sort(operands_.size()))
            return fail_resource();
        std::sort(operands_.begin(), operands_.end(),
                  [](const OperandInfo& left, const OperandInfo& right)
                  { return left.operand_id < right.operand_id; });
        if ((!operands_.empty() && operands_.front().operand_id == 0) ||
            std::adjacent_find(operands_.begin(), operands_.end(),
                               [](const OperandInfo& left, const OperandInfo& right)
                               { return left.operand_id == right.operand_id; }) != operands_.end())
            return false;
        std::uint64_t segment_count = 0;
        std::uint64_t ring_visits = 0;
        for (const OperandInfo& operand : operands_)
        {
            if (operand.geometry_kind != 1)
                continue;
            if (operand.geometry_index >= records_.planar_regions.size())
                return false;
            const auto& region = records_.planar_regions[operand.geometry_index];
            if (region.outer_ring >= records_.rings.size() ||
                region.hole_reference_begin > records_.ring_references.size() ||
                region.hole_reference_count >
                    records_.ring_references.size() - region.hole_reference_begin)
                return false;
            const auto count_ring = [&](std::uint32_t ring_index)
            {
                if (ring_index >= records_.rings.size())
                    return false;
                ++ring_visits;
                return checked_add(segment_count, records_.rings[ring_index].segment_count,
                                   segment_count);
            };
            if (!count_ring(region.outer_ring))
                return fail_resource();
            for (std::uint32_t hole = 0; hole < region.hole_reference_count; ++hole)
                if (!count_ring(records_.ring_references[region.hole_reference_begin + hole]))
                    return fail_resource();
        }
        std::uint64_t segment_work = 0;
        if (!checked_multiply(ring_visits, 2, segment_work) ||
            !checked_add(segment_work, segment_count, segment_work) ||
            !budget_.charge(segment_work))
            return fail_resource();
        if (!add_array_bytes(segment_count, 32, binding_bytes_) || !check_phase(0))
            return false;
        segments_.reserve(static_cast<std::size_t>(segment_count));
        for (const OperandInfo& operand : operands_)
        {
            if (operand.geometry_kind != 1)
                continue;
            if (operand.geometry_index >= records_.planar_regions.size())
                return false;
            const auto& region = records_.planar_regions[operand.geometry_index];
            const auto append_ring = [&](std::uint32_t ring_index)
            {
                if (ring_index >= records_.rings.size())
                    return false;
                const auto& ring = records_.rings[ring_index];
                if (ring.segment_begin > records_.segments.size() ||
                    ring.segment_count > records_.segments.size() - ring.segment_begin)
                    return false;
                for (std::uint32_t offset = 0; offset < ring.segment_count; ++offset)
                {
                    const auto& segment = records_.segments[ring.segment_begin + offset];
                    segments_.push_back(
                        {segment.id, segment.curve_id, operand.operand_id, segment.kind});
                }
                return true;
            };
            if (!append_ring(region.outer_ring) ||
                region.hole_reference_begin > records_.ring_references.size() ||
                region.hole_reference_count >
                    records_.ring_references.size() - region.hole_reference_begin)
                return false;
            for (std::uint32_t hole = 0; hole < region.hole_reference_count; ++hole)
                if (!append_ring(records_.ring_references[region.hole_reference_begin + hole]))
                    return false;
        }
        if (!budget_.charge_sort(segments_.size()))
            return fail_resource();
        std::sort(segments_.begin(), segments_.end(),
                  [](const SegmentBinding& left, const SegmentBinding& right)
                  { return left.segment_id < right.segment_id; });
        if (std::adjacent_find(segments_.begin(), segments_.end(),
                               [](const SegmentBinding& left, const SegmentBinding& right)
                               { return left.segment_id == right.segment_id; }) != segments_.end())
            return false;

        std::uint64_t occurrence_work = 0;
        std::uint64_t lookup_work = 0;
        if (!checked_add(search_units(operands_.size()), search_units(segments_.size()),
                         lookup_work) ||
            !checked_add(lookup_work, 1, lookup_work) ||
            !checked_multiply(geometry_.occurrences.size(), lookup_work, occurrence_work) ||
            !budget_.charge(occurrence_work))
            return fail_resource();
        allowed_geometry_sources_.reserve(geometry_.occurrences.size());
        for (const auto& occurrence : geometry_.occurrences)
        {
            if (!bind_geometry_source(occurrence.source, records_, operands_, segments_))
                return false;
            allowed_geometry_sources_.push_back(occurrence.source);
        }
        if (!budget_.charge_sort(allowed_geometry_sources_.size()))
            return fail_resource();
        std::sort(allowed_geometry_sources_.begin(), allowed_geometry_sources_.end(), source_less);
        allowed_geometry_sources_.erase(std::unique(allowed_geometry_sources_.begin(),
                                                    allowed_geometry_sources_.end(), source_equal),
                                        allowed_geometry_sources_.end());
        expected_event_sources_.reserve(allowed_geometry_sources_.size());
        if (!budget_.charge(allowed_geometry_sources_.size()) ||
            !budget_.charge_sort(allowed_geometry_sources_.size()))
            return fail_resource();
        for (const auto& source : allowed_geometry_sources_)
            expected_event_sources_.push_back({source.operand_id, source});
        std::sort(expected_event_sources_.begin(), expected_event_sources_.end(),
                  [](const OperandSource& left, const OperandSource& right)
                  {
                      if (left.operand_id != right.operand_id)
                          return left.operand_id < right.operand_id;
                      return source_less(left.source, right.source);
                  });
        return true;
    }

    bool add_use(std::uint8_t buffer, AnalyticFilteredSourceRange range,
                 SourceExpectation expectation, bool require_nonempty)
    {
        const auto& sources = buffer == 0 ? input_.lineage_sources : input_.event_sources;
        if (range.begin > sources.size() || range.count > sources.size() - range.begin ||
            (require_nonempty && range.count == 0))
            return false;
        source_uses_.push_back({buffer, expectation, range.begin, range.count});
        ++result_.telemetry.source_range_visits;
        return true;
    }

    bool prepare_source_uses()
    {
        std::uint64_t use_count = input_.vertices.size();
        std::uint64_t term = 0;
        if (!checked_multiply(input_.fragments.size(), 2, term) ||
            !checked_add(use_count, term, use_count) ||
            !checked_add(use_count, input_.regions.size(), use_count) ||
            !checked_add(use_count, input_.events.size(), use_count))
            return fail_resource();
        std::uint64_t work = 0;
        if (!checked_add(use_count, input_.vertex_lineage.size(), work) ||
            !checked_add(work, input_.region_lineage.size(), work) || !budget_.charge(work))
            return fail_resource();
        source_use_bytes_ = 0;
        std::uint64_t map_bytes = 0;
        if (!add_array_bytes(use_count, 16, source_use_bytes_) ||
            !add_array_bytes(input_.old_vertex_to_normalized.size(), 4, map_bytes) ||
            !add_array_bytes(input_.old_region_to_normalized.size(), 4, map_bytes) ||
            !initialize_publication_bytes() || !check_phase(map_bytes))
            return false;
        source_uses_.reserve(static_cast<std::size_t>(use_count));
        vertex_use_begin_ = static_cast<std::uint32_t>(source_uses_.size());
        std::vector<std::uint32_t> lineage_by_vertex(input_.old_vertex_to_normalized.size(), kNone);
        for (std::uint32_t index = 0; index < input_.vertex_lineage.size(); ++index)
        {
            const auto& lineage = input_.vertex_lineage[index];
            if (lineage.arrangement_vertex >= lineage_by_vertex.size() ||
                lineage_by_vertex[lineage.arrangement_vertex] != kNone)
                return false;
            lineage_by_vertex[lineage.arrangement_vertex] = index;
        }
        for (const auto& vertex : input_.vertices)
        {
            if (vertex.arrangement_vertex >= lineage_by_vertex.size() ||
                lineage_by_vertex[vertex.arrangement_vertex] == kNone)
                return false;
            if (!add_use(0,
                         input_.vertex_lineage[lineage_by_vertex[vertex.arrangement_vertex]]
                             .intersection,
                         SourceExpectation::geometry, false))
                return false;
        }
        fragment_use_begin_ = static_cast<std::uint32_t>(source_uses_.size());
        for (const auto& fragment : input_.fragments)
        {
            if (fragment.old_boundary >= input_.boundaries.size() ||
                !add_use(0, input_.boundaries[fragment.old_boundary].positive,
                         SourceExpectation::geometry, false) ||
                !add_use(0, input_.boundaries[fragment.old_boundary].subtraction,
                         SourceExpectation::subtraction, false))
                return false;
        }
        region_use_begin_ = static_cast<std::uint32_t>(source_uses_.size());
        std::vector<std::uint32_t> lineage_by_region(input_.old_region_to_normalized.size(), kNone);
        for (std::uint32_t index = 0; index < input_.region_lineage.size(); ++index)
        {
            const auto& lineage = input_.region_lineage[index];
            if (lineage.region >= lineage_by_region.size() ||
                lineage_by_region[lineage.region] != kNone)
                return false;
            lineage_by_region[lineage.region] = index;
        }
        for (const auto& region : input_.regions)
        {
            if (region.old_region >= lineage_by_region.size() ||
                lineage_by_region[region.old_region] == kNone ||
                !add_use(0,
                         input_.region_lineage[lineage_by_region[region.old_region]]
                             .positive_contributors,
                         SourceExpectation::geometry, true))
                return false;
        }
        event_use_begin_ = static_cast<std::uint32_t>(source_uses_.size());
        for (const auto& event : input_.events)
            if (!add_use(1, event.sources, SourceExpectation::geometry, true))
                return false;
        return source_uses_.size() == use_count;
    }

    bool build_source_tables()
    {
        if (!budget_.charge(source_uses_.size()))
            return fail_resource();
        std::uint64_t descriptor_scratch = 0;
        if (!add_array_bytes(source_uses_.size(), 4, descriptor_scratch) ||
            !add_array_bytes(source_uses_.size(), 16, descriptor_scratch) ||
            !add_array_bytes(source_uses_.size(), 4, descriptor_scratch) ||
            !check_phase(descriptor_scratch))
            return false;
        std::vector<std::uint32_t> descriptor_order(source_uses_.size());
        std::iota(descriptor_order.begin(), descriptor_order.end(), 0);
        if (!budget_.charge_sort(descriptor_order.size()))
            return fail_resource();
        std::sort(descriptor_order.begin(), descriptor_order.end(),
                  [&](std::uint32_t left, std::uint32_t right)
                  { return source_use_less(source_uses_[left], source_uses_[right]); });
        std::vector<SourceUse> descriptors;
        descriptors.reserve(source_uses_.size());
        std::vector<std::uint32_t> descriptor_by_use(source_uses_.size());
        for (std::uint32_t use : descriptor_order)
        {
            if (descriptors.empty() || !source_use_equal(descriptors.back(), source_uses_[use]))
                descriptors.push_back(source_uses_[use]);
            descriptor_by_use[use] = static_cast<std::uint32_t>(descriptors.size() - 1);
        }
        std::uint64_t memberships = 0;
        if (!budget_.charge(descriptors.size()))
            return fail_resource();
        for (const SourceUse& descriptor : descriptors)
        {
            if (!checked_add(memberships, descriptor.count, memberships))
                return fail_resource();
        }
        if (memberships > limits_.source_reference_memberships)
            return fail_resource();
        result_.telemetry.source_memberships = memberships;
        std::uint64_t source_scratch = descriptor_scratch;
        if (!add_array_bytes(memberships, 48, source_scratch) ||
            !add_array_bytes(memberships, 32, source_scratch) ||
            !add_array_bytes(memberships, 4, source_scratch) ||
            !add_array_bytes(descriptors.size(), 8, source_scratch) || !check_phase(source_scratch))
            return false;

        std::vector<SourceOccurrence> occurrences;
        std::uint64_t membership_work = 0;
        std::uint64_t per_membership_work = 0;
        if (!checked_add(search_units(allowed_geometry_sources_.size()),
                         search_units(operands_.size()), per_membership_work) ||
            !checked_add(per_membership_work, 3, per_membership_work) ||
            !checked_multiply(memberships, per_membership_work, membership_work) ||
            !budget_.charge(membership_work))
            return fail_resource();
        occurrences.reserve(static_cast<std::size_t>(memberships));
        source_tables_.sources.reserve(static_cast<std::size_t>(memberships));
        for (std::uint32_t descriptor_index = 0; descriptor_index < descriptors.size();
             ++descriptor_index)
        {
            const SourceUse descriptor = descriptors[descriptor_index];
            const auto& buffer =
                descriptor.buffer == 0 ? input_.lineage_sources : input_.event_sources;
            for (std::uint32_t offset = 0; offset < descriptor.count; ++offset)
            {
                const AnalyticSourceReference& source = buffer[descriptor.begin + offset];
                if (offset != 0 && !source_less(buffer[descriptor.begin + offset - 1], source))
                    return false;
                if (descriptor.expectation == SourceExpectation::geometry)
                {
                    if (!std::binary_search(allowed_geometry_sources_.begin(),
                                            allowed_geometry_sources_.end(), source, source_less))
                        return false;
                }
                else if (!bind_subtraction_source(source, operands_))
                    return false;
                occurrences.push_back({source, descriptor_index, offset, 0});
            }
        }
        if (!budget_.charge_sort(occurrences.size()))
            return fail_resource();
        std::sort(occurrences.begin(), occurrences.end(),
                  [](const SourceOccurrence& left, const SourceOccurrence& right)
                  { return source_less(left.source, right.source); });
        for (auto& occurrence : occurrences)
        {
            if (source_tables_.sources.empty() ||
                !source_equal(source_tables_.sources.back(), occurrence.source))
                source_tables_.sources.push_back(occurrence.source);
            occurrence.source_index = static_cast<std::uint32_t>(source_tables_.sources.size() - 1);
        }
        if (source_tables_.sources.size() > limits_.provenance_references)
            return fail_resource();
        result_.telemetry.unique_sources = source_tables_.sources.size();
        if (!budget_.charge_sort(occurrences.size()))
            return fail_resource();
        std::sort(occurrences.begin(), occurrences.end(),
                  [](const SourceOccurrence& left, const SourceOccurrence& right)
                  {
                      return std::tie(left.descriptor, left.offset) <
                             std::tie(right.descriptor, right.offset);
                  });
        std::vector<std::uint32_t> labels;
        labels.reserve(occurrences.size());
        std::vector<SequenceRange> ranges(descriptors.size());
        std::uint32_t cursor = 0;
        for (std::uint32_t descriptor = 0; descriptor < descriptors.size(); ++descriptor)
        {
            ranges[descriptor] = {cursor, descriptors[descriptor].count};
            while (cursor < occurrences.size() && occurrences[cursor].descriptor == descriptor)
            {
                labels.push_back(occurrences[cursor].source_index);
                ++cursor;
            }
        }
        if (cursor != occurrences.size())
            return false;
        CanonicalSequences sequences;
        std::uint64_t source_base = 0;
        if (!persistent_bytes(source_base) ||
            !checked_add(source_base, source_scratch, source_base) ||
            !canonicalize_sequences(labels, ranges, true, source_base, limits_.working_memory_bytes,
                                    budget_, sequences))
            return fail_resource();
        source_tables_.sets = std::move(sequences.records);
        source_tables_.indices = std::move(sequences.indices);
        source_tables_.handles.resize(source_uses_.size());
        for (std::uint32_t use = 0; use < source_uses_.size(); ++use)
            source_tables_.handles[use] = sequences.handles[descriptor_by_use[use]];
        source_tables_.logical_bytes = sequences.logical_bytes;
        if (!add_array_bytes(memberships, 32, source_tables_.logical_bytes) ||
            !add_array_bytes(source_uses_.size(), 4, source_tables_.logical_bytes))
            return false;
        std::uint64_t retained_source_scratch = descriptor_scratch;
        if (!add_array_bytes(memberships, 48, retained_source_scratch) ||
            !add_array_bytes(memberships, 4, retained_source_scratch) ||
            !add_array_bytes(descriptors.size(), 8, retained_source_scratch) ||
            !add_array_bytes(descriptors.size(), 4, retained_source_scratch) ||
            !check_phase(retained_source_scratch))
            return false;
        result_.telemetry.unique_source_sets = source_tables_.sets.size();
        return source_tables_.indices.size() <= limits_.source_reference_memberships;
    }

    bool publish_vertices_and_fragments()
    {
        std::uint64_t incident_count = 0;
        if (!checked_multiply(input_.fragments.size(), 2, incident_count) ||
            !budget_.charge(input_.fragments.size()) || !budget_.charge(incident_count * 3))
            return fail_resource();
        std::uint64_t incident_scratch = 0;
        if (!add_array_bytes(incident_count, 64, incident_scratch) ||
            !add_array_bytes(incident_count, 4, incident_scratch) ||
            !add_array_bytes(incident_count, 4, incident_scratch) ||
            !add_array_bytes(input_.vertices.size(), 8, incident_scratch) ||
            !check_phase(incident_scratch))
            return false;
        std::vector<IncidentOccurrence> incidents;
        incidents.reserve(static_cast<std::size_t>(incident_count));
        for (const auto& fragment : input_.fragments)
        {
            if (fragment.start_vertex >= input_.vertices.size() ||
                fragment.end_vertex >= input_.vertices.size() ||
                fragment.start_vertex == fragment.end_vertex)
                return false;
            const auto& start = input_.vertices[fragment.start_vertex];
            const auto& end = input_.vertices[fragment.end_vertex];
            const std::uint8_t kind = fragment.kind == AnalyticAtomicCurveKind::line ? 1 : 2;
            const std::uint8_t direction = kind == 1 ? 0 : fragment.counterclockwise ? 1 : 2;
            incidents.push_back(
                {fragment.start_vertex,
                 {0, end.x_nm, end.y_nm, kind, direction, fragment.major_arc, fragment.radius_nm},
                 0});
            incidents.push_back({fragment.end_vertex,
                                 {1, start.x_nm, start.y_nm, kind, direction, fragment.major_arc,
                                  fragment.radius_nm},
                                 0});
        }
        if (!budget_.charge_sort(incidents.size()))
            return fail_resource();
        std::vector<std::uint32_t> by_key(incidents.size());
        std::iota(by_key.begin(), by_key.end(), 0);
        std::sort(
            by_key.begin(), by_key.end(), [&](std::uint32_t left, std::uint32_t right)
            { return incident_key(incidents[left].key) < incident_key(incidents[right].key); });
        std::uint32_t next_label = 0;
        for (std::uint32_t at = 0; at < by_key.size(); ++at)
        {
            if (at != 0 && incident_key(incidents[by_key[at - 1]].key) <
                               incident_key(incidents[by_key[at]].key))
                ++next_label;
            incidents[by_key[at]].label = next_label;
        }
        if (!budget_.charge_sort(incidents.size()))
            return fail_resource();
        std::sort(
            incidents.begin(), incidents.end(),
            [](const IncidentOccurrence& left, const IncidentOccurrence& right)
            { return std::tie(left.vertex, left.label) < std::tie(right.vertex, right.label); });
        std::vector<std::uint32_t> incident_labels;
        incident_labels.reserve(incidents.size());
        std::vector<SequenceRange> incident_ranges(input_.vertices.size());
        std::uint32_t cursor = 0;
        for (std::uint32_t vertex = 0; vertex < input_.vertices.size(); ++vertex)
        {
            const std::uint32_t begin = cursor;
            while (cursor < incidents.size() && incidents[cursor].vertex == vertex)
            {
                incident_labels.push_back(incidents[cursor].label);
                ++cursor;
            }
            if (cursor == begin)
                return false;
            incident_ranges[vertex] = {begin, cursor - begin};
        }
        CanonicalSequences incident_sequences;
        std::uint64_t incident_base = 0;
        if (!persistent_bytes(incident_base) ||
            !checked_add(incident_base, incident_scratch, incident_base))
            return fail_resource();
        if (!canonicalize_sequences(incident_labels, incident_ranges, false, incident_base,
                                    limits_.working_memory_bytes, budget_, incident_sequences))
            return fail_resource();

        std::uint64_t publication_scratch = incident_scratch;
        if (!checked_add(publication_scratch, incident_sequences.logical_bytes,
                         publication_scratch) ||
            !add_array_bytes(input_.vertices.size(), 4, publication_scratch) ||
            !add_array_bytes(input_.fragments.size(), 64, publication_scratch) ||
            !check_phase(publication_scratch))
            return false;

        if (!budget_.charge_sort(input_.vertices.size()))
            return fail_resource();
        std::vector<std::uint32_t> vertex_order(input_.vertices.size());
        std::iota(vertex_order.begin(), vertex_order.end(), 0);
        std::sort(vertex_order.begin(), vertex_order.end(),
                  [&](std::uint32_t left, std::uint32_t right)
                  {
                      const auto& a = input_.vertices[left];
                      const auto& b = input_.vertices[right];
                      return std::tuple{a.x_nm, a.y_nm, incident_sequences.handles[left],
                                        source_tables_.handles[vertex_use_begin_ + left]} <
                             std::tuple{b.x_nm, b.y_nm, incident_sequences.handles[right],
                                        source_tables_.handles[vertex_use_begin_ + right]};
                  });
        if (!budget_.charge(input_.vertices.size()))
            return fail_resource();
        normalized_vertex_to_packet_.resize(input_.vertices.size());
        result_.maps.arrangement_vertex_to_packet_vertex.assign(
            input_.old_vertex_to_normalized.size(), kNoAnalyticFilteredPacketIndex);
        records_out_.vertices.reserve(input_.vertices.size());
        for (std::uint32_t index = 0; index < vertex_order.size(); ++index)
        {
            if (index != 0)
            {
                const std::uint32_t left = vertex_order[index - 1];
                const std::uint32_t right = vertex_order[index];
                const auto& a = input_.vertices[left];
                const auto& b = input_.vertices[right];
                if (std::tuple{a.x_nm, a.y_nm, incident_sequences.handles[left],
                               source_tables_.handles[vertex_use_begin_ + left]} ==
                    std::tuple{b.x_nm, b.y_nm, incident_sequences.handles[right],
                               source_tables_.handles[vertex_use_begin_ + right]})
                    return false;
            }
            const std::uint32_t old = vertex_order[index];
            normalized_vertex_to_packet_[old] = index;
            const auto& vertex = input_.vertices[old];
            if (vertex.arrangement_vertex >= input_.old_vertex_to_normalized.size() ||
                input_.old_vertex_to_normalized[vertex.arrangement_vertex] != old)
                return false;
            const std::uint32_t handle = source_tables_.handles[vertex_use_begin_ + old];
            records_out_.vertices.push_back({static_cast<std::uint64_t>(index) + 1, vertex.x_nm,
                                             vertex.y_nm, handle, handle == 0 ? 0U : 1U});
            if (vertex.arrangement_vertex >=
                    result_.maps.arrangement_vertex_to_packet_vertex.size() ||
                result_.maps.arrangement_vertex_to_packet_vertex[vertex.arrangement_vertex] !=
                    kNoAnalyticFilteredPacketIndex)
                return false;
            result_.maps.arrangement_vertex_to_packet_vertex[vertex.arrangement_vertex] = index;
        }

        struct FragmentTemp
        {
            AnalyticDirectedFragmentRecord record;
            std::uint32_t normalized = 0;
            std::uint32_t old_boundary = 0;
        };
        if (!budget_.charge(input_.fragments.size()) ||
            !budget_.charge_sort(input_.fragments.size()))
            return fail_resource();
        std::vector<FragmentTemp> fragments;
        fragments.reserve(input_.fragments.size());
        for (std::uint32_t index = 0; index < input_.fragments.size(); ++index)
        {
            const auto& fragment = input_.fragments[index];
            const bool line = fragment.kind == AnalyticAtomicCurveKind::line;
            fragments.push_back({{0, normalized_vertex_to_packet_[fragment.start_vertex],
                                  normalized_vertex_to_packet_[fragment.end_vertex],
                                  static_cast<std::uint8_t>(line ? 1 : 2),
                                  static_cast<std::uint8_t>(line                        ? 0
                                                            : fragment.counterclockwise ? 1
                                                                                        : 2),
                                  fragment.major_arc, fragment.radius_nm,
                                  source_tables_.handles[fragment_use_begin_ + index * 2],
                                  source_tables_.handles[fragment_use_begin_ + index * 2 + 1]},
                                 index,
                                 fragment.old_boundary});
        }
        std::sort(
            fragments.begin(), fragments.end(),
            [](const FragmentTemp& left, const FragmentTemp& right)
            {
                const auto& a = left.record;
                const auto& b = right.record;
                return std::tie(a.start_vertex, a.end_vertex, a.kind, a.direction, a.major_arc,
                                a.radius_nm, a.positive_source_set, a.subtraction_source_set) <
                       std::tie(b.start_vertex, b.end_vertex, b.kind, b.direction, b.major_arc,
                                b.radius_nm, b.positive_source_set, b.subtraction_source_set);
            });
        normalized_fragment_to_packet_.resize(fragments.size());
        result_.maps.boundary_to_packet_fragment.assign(input_.old_boundary_to_normalized.size(),
                                                        kNoAnalyticFilteredPacketIndex);
        records_out_.fragments.reserve(fragments.size());
        for (std::uint32_t index = 0; index < fragments.size(); ++index)
        {
            if (index != 0)
            {
                const auto& a = fragments[index - 1].record;
                const auto& b = fragments[index].record;
                if (std::tie(a.start_vertex, a.end_vertex, a.kind, a.direction, a.major_arc,
                             a.radius_nm, a.positive_source_set, a.subtraction_source_set) ==
                    std::tie(b.start_vertex, b.end_vertex, b.kind, b.direction, b.major_arc,
                             b.radius_nm, b.positive_source_set, b.subtraction_source_set))
                    return false;
            }
            auto value = fragments[index].record;
            value.id = static_cast<std::uint64_t>(index) + 1;
            records_out_.fragments.push_back(value);
            normalized_fragment_to_packet_[fragments[index].normalized] = index;
            if (fragments[index].old_boundary >= result_.maps.boundary_to_packet_fragment.size() ||
                input_.old_boundary_to_normalized[fragments[index].old_boundary] !=
                    fragments[index].normalized ||
                result_.maps.boundary_to_packet_fragment[fragments[index].old_boundary] !=
                    kNoAnalyticFilteredPacketIndex)
                return false;
            result_.maps.boundary_to_packet_fragment[fragments[index].old_boundary] = index;
        }
        return true;
    }

    bool publish_rings_and_regions()
    {
        std::uint64_t ring_work = 0;
        if (!checked_multiply(input_.ring_fragments.size(), 3, ring_work) ||
            !checked_add(ring_work, input_.rings.size(), ring_work) || !budget_.charge(ring_work))
            return fail_resource();
        std::uint64_t ring_scratch = 0;
        if (!add_array_bytes(input_.ring_fragments.size(), 4, ring_scratch) ||
            !add_array_bytes(input_.rings.size(), 8, ring_scratch) || !check_phase(ring_scratch))
            return false;
        std::vector<std::uint32_t> ring_labels;
        std::vector<SequenceRange> ring_ranges(input_.rings.size());
        ring_labels.reserve(input_.ring_fragments.size());
        for (std::uint32_t ring = 0; ring < input_.rings.size(); ++ring)
        {
            const auto& value = input_.rings[ring];
            if (value.fragment_begin > input_.ring_fragments.size() || value.fragment_count == 0 ||
                value.fragment_count > input_.ring_fragments.size() - value.fragment_begin)
                return false;
            const std::uint32_t begin = static_cast<std::uint32_t>(ring_labels.size());
            for (std::uint32_t offset = 0; offset < value.fragment_count; ++offset)
            {
                const std::uint32_t fragment = input_.ring_fragments[value.fragment_begin + offset];
                if (fragment >= normalized_fragment_to_packet_.size())
                    return false;
                ring_labels.push_back(normalized_fragment_to_packet_[fragment]);
            }
            const std::size_t rotation = least_rotation(ring_labels, begin, value.fragment_count);
            std::rotate(ring_labels.begin() + begin, ring_labels.begin() + begin + rotation,
                        ring_labels.begin() + begin + value.fragment_count);
            ring_ranges[ring] = {begin, value.fragment_count};
        }
        CanonicalSequences ring_sequences;
        std::uint64_t ring_base = 0;
        if (!persistent_bytes(ring_base) || !checked_add(ring_base, ring_scratch, ring_base))
            return fail_resource();
        if (!canonicalize_sequences(ring_labels, ring_ranges, false, ring_base,
                                    limits_.working_memory_bytes, budget_, ring_sequences))
            return fail_resource();
        std::uint64_t ring_publication_scratch = ring_scratch;
        if (!checked_add(ring_publication_scratch, ring_sequences.logical_bytes,
                         ring_publication_scratch) ||
            !add_array_bytes(input_.rings.size(), 4, ring_publication_scratch) ||
            !add_array_bytes(input_.regions.size(), 40, ring_publication_scratch) ||
            !check_phase(ring_publication_scratch))
            return false;
        if (!budget_.charge_sort(input_.rings.size()))
            return fail_resource();
        std::vector<std::uint32_t> ring_order(input_.rings.size());
        std::iota(ring_order.begin(), ring_order.end(), 0);
        std::sort(ring_order.begin(), ring_order.end(), [&](std::uint32_t left, std::uint32_t right)
                  { return input_.rings[left].depth < input_.rings[right].depth; });
        normalized_ring_to_packet_.assign(input_.rings.size(), kNone);
        std::size_t depth_begin = 0;
        while (depth_begin < ring_order.size())
        {
            std::size_t depth_end = depth_begin + 1;
            const std::uint32_t depth = input_.rings[ring_order[depth_begin]].depth;
            while (depth_end < ring_order.size() &&
                   input_.rings[ring_order[depth_end]].depth == depth)
                ++depth_end;
            if (!budget_.charge_sort(depth_end - depth_begin))
                return fail_resource();
            std::sort(ring_order.begin() + depth_begin, ring_order.begin() + depth_end,
                      [&](std::uint32_t left, std::uint32_t right)
                      {
                          const std::uint32_t left_parent =
                              input_.rings[left].parent_ring == kNoAnalyticNormalizedIndex
                                  ? kNone
                                  : normalized_ring_to_packet_[input_.rings[left].parent_ring];
                          const std::uint32_t right_parent =
                              input_.rings[right].parent_ring == kNoAnalyticNormalizedIndex
                                  ? kNone
                                  : normalized_ring_to_packet_[input_.rings[right].parent_ring];
                          return std::tuple{ring_sequences.handles[left], left_parent} <
                                 std::tuple{ring_sequences.handles[right], right_parent};
                      });
            for (std::size_t at = depth_begin; at < depth_end; ++at)
                normalized_ring_to_packet_[ring_order[at]] = static_cast<std::uint32_t>(at);
            depth_begin = depth_end;
        }
        records_out_.rings.reserve(input_.rings.size());
        records_out_.fragment_references.reserve(input_.ring_fragments.size());
        if (!budget_.charge(input_.rings.size() + input_.ring_fragments.size()))
            return fail_resource();
        result_.maps.ring_to_packet_ring.assign(input_.old_ring_to_normalized.size(),
                                                kNoAnalyticFilteredPacketIndex);
        for (std::uint32_t packet_ring = 0; packet_ring < ring_order.size(); ++packet_ring)
        {
            const std::uint32_t normalized = ring_order[packet_ring];
            const auto& value = input_.rings[normalized];
            const SequenceRange range = ring_ranges[normalized];
            const std::uint32_t begin =
                static_cast<std::uint32_t>(records_out_.fragment_references.size());
            for (std::uint32_t offset = 0; offset < range.count; ++offset)
                records_out_.fragment_references.push_back(ring_labels[range.begin + offset]);
            const std::uint32_t parent = value.parent_ring == kNoAnalyticNormalizedIndex
                                             ? kNone
                                             : normalized_ring_to_packet_[value.parent_ring];
            if (value.old_ring >= input_.old_ring_parents.size())
                return false;
            const std::uint32_t old_parent = input_.old_ring_parents[value.old_ring];
            const std::uint32_t expected_parent =
                old_parent == kNoAnalyticFilteredRing ? kNoAnalyticNormalizedIndex
                : old_parent < input_.old_ring_to_normalized.size()
                    ? input_.old_ring_to_normalized[old_parent]
                    : kNoAnalyticNormalizedIndex;
            if (value.parent_ring != expected_parent ||
                value.old_ring >= input_.old_ring_to_normalized.size() ||
                input_.old_ring_to_normalized[value.old_ring] != normalized)
                return false;
            records_out_.rings.push_back({static_cast<std::uint64_t>(packet_ring) + 1, begin,
                                          range.count, parent, value.depth, value.depth & 1U});
            if (value.old_ring >= result_.maps.ring_to_packet_ring.size() ||
                result_.maps.ring_to_packet_ring[value.old_ring] != kNoAnalyticFilteredPacketIndex)
                return false;
            result_.maps.ring_to_packet_ring[value.old_ring] = packet_ring;
        }

        struct RegionTemp
        {
            AnalyticResultRegionRecord record;
            std::uint32_t normalized = 0;
            std::uint32_t old_region = 0;
        };
        if (!budget_.charge(input_.regions.size()) || !budget_.charge_sort(input_.regions.size()))
            return fail_resource();
        std::vector<RegionTemp> regions;
        regions.reserve(input_.regions.size());
        for (std::uint32_t index = 0; index < input_.regions.size(); ++index)
        {
            const auto& region = input_.regions[index];
            if (region.outer_ring >= normalized_ring_to_packet_.size())
                return false;
            regions.push_back({{0, normalized_ring_to_packet_[region.outer_ring],
                                source_tables_.handles[region_use_begin_ + index]},
                               index,
                               region.old_region});
        }
        std::sort(regions.begin(), regions.end(),
                  [](const RegionTemp& left, const RegionTemp& right)
                  {
                      return std::tie(left.record.outer_ring, left.record.positive_source_set) <
                             std::tie(right.record.outer_ring, right.record.positive_source_set);
                  });
        normalized_region_to_packet_.resize(regions.size());
        result_.maps.region_to_packet_region.assign(input_.old_region_to_normalized.size(),
                                                    kNoAnalyticFilteredPacketIndex);
        records_out_.regions.reserve(regions.size());
        for (std::uint32_t index = 0; index < regions.size(); ++index)
        {
            if (index != 0 && std::tie(regions[index - 1].record.outer_ring,
                                       regions[index - 1].record.positive_source_set) ==
                                  std::tie(regions[index].record.outer_ring,
                                           regions[index].record.positive_source_set))
                return false;
            auto record = regions[index].record;
            record.id = static_cast<std::uint64_t>(index) + 1;
            records_out_.regions.push_back(record);
            normalized_region_to_packet_[regions[index].normalized] = index;
            if (regions[index].old_region >= result_.maps.region_to_packet_region.size() ||
                input_.old_region_to_normalized[regions[index].old_region] !=
                    regions[index].normalized ||
                result_.maps.region_to_packet_region[regions[index].old_region] !=
                    kNoAnalyticFilteredPacketIndex)
                return false;
            result_.maps.region_to_packet_region[regions[index].old_region] = index;
        }
        return true;
    }

    bool collect_events(std::vector<EventTemp>& events, std::vector<RawReference>& raw_references)
    {
        std::uint64_t event_source_visits = 0;
        for (const auto& event : input_.events)
            if (!checked_add(event_source_visits, event.sources.count, event_source_visits))
                return fail_resource();
        std::uint64_t event_work = 0;
        std::uint64_t event_lookup_work = 0;
        std::uint64_t expected_lookup_work = 0;
        if (!checked_multiply(search_units(expected_event_sources_.size()), 2,
                              expected_lookup_work) ||
            !checked_add(search_units(operands_.size()), expected_lookup_work, event_lookup_work) ||
            !checked_multiply(input_.events.size(), event_lookup_work, event_lookup_work))
            return fail_resource();
        if (!checked_add(input_.events.size(), input_.event_references.size(), event_work) ||
            !checked_add(event_work, event_source_visits, event_work) ||
            !checked_add(event_work, event_lookup_work, event_work) || !budget_.charge(event_work))
            return fail_resource();
        std::uint64_t event_scratch = 0;
        if (!add_array_bytes(input_.events.size(), 64, event_scratch) ||
            !add_array_bytes(input_.event_references.size(), 16, event_scratch) ||
            !add_array_bytes(operands_.size(), 1, event_scratch) || !check_phase(event_scratch))
            return false;
        events.reserve(input_.events.size());
        raw_references.reserve(input_.event_references.size());
        std::vector<std::uint8_t> operand_seen(operands_.size());
        for (std::uint32_t index = 0; index < input_.events.size(); ++index)
        {
            const auto& event = input_.events[index];
            const OperandInfo* operand = find_operand(operands_, event.operand_id);
            if (operand == nullptr ||
                event.result_references.begin > input_.event_references.size() ||
                event.result_references.count >
                    input_.event_references.size() - event.result_references.begin)
                return false;
            operand_seen[static_cast<std::size_t>(operand - operands_.data())] = 1;
            EventTemp temp;
            temp.record.operand_id = event.operand_id;
            temp.record.kind = event.kind;
            temp.record.source_set = source_tables_.handles[event_use_begin_ + index];
            if (temp.record.source_set == 0)
                return false;
            switch (event.kind)
            {
            case AnalyticOperandOutcomeKind::contributes_final_material:
            case AnalyticOperandOutcomeKind::redundant_or_absorbed_coverage:
            case AnalyticOperandOutcomeKind::partially_removed_later:
            case AnalyticOperandOutcomeKind::completely_removed_later:
                if (operand->operation != 1)
                    return false;
                break;
            case AnalyticOperandOutcomeKind::subtraction_effect_survives:
            case AnalyticOperandOutcomeKind::subtraction_effect_overwritten_later:
                if (operand->operation != 2)
                    return false;
                break;
            case AnalyticOperandOutcomeKind::no_effect:
                break;
            default:
                return false;
            }
            const auto expected_begin = std::lower_bound(
                expected_event_sources_.begin(), expected_event_sources_.end(), event.operand_id,
                [](const OperandSource& value, std::uint64_t id) { return value.operand_id < id; });
            const auto expected_end = std::upper_bound(
                expected_begin, expected_event_sources_.end(), event.operand_id,
                [](std::uint64_t id, const OperandSource& value) { return id < value.operand_id; });
            if (static_cast<std::size_t>(expected_end - expected_begin) != event.sources.count)
                return false;
            for (std::uint32_t source_offset = 0; source_offset < event.sources.count;
                 ++source_offset)
                if (!source_equal(input_.event_sources[event.sources.begin + source_offset],
                                  expected_begin[source_offset].source))
                    return false;
            for (std::uint32_t offset = 0; offset < event.result_references.count; ++offset)
            {
                const auto ref = input_.event_references[event.result_references.begin + offset];
                std::uint32_t packet_index = 0;
                if (ref.kind == AnalyticFilteredResultReferenceKind::ring)
                {
                    if (ref.local_index >= input_.old_ring_to_normalized.size() ||
                        input_.old_ring_to_normalized[ref.local_index] ==
                            kNoAnalyticNormalizedIndex)
                        return false;
                    packet_index =
                        normalized_ring_to_packet_[input_.old_ring_to_normalized[ref.local_index]];
                    raw_references.push_back({index, (std::uint64_t{1} << 32U) | packet_index});
                }
                else if (ref.kind == AnalyticFilteredResultReferenceKind::region)
                {
                    if (ref.local_index >= input_.old_region_to_normalized.size() ||
                        input_.old_region_to_normalized[ref.local_index] ==
                            kNoAnalyticNormalizedIndex)
                        return false;
                    packet_index = normalized_region_to_packet_
                        [input_.old_region_to_normalized[ref.local_index]];
                    raw_references.push_back({index, (std::uint64_t{2} << 32U) | packet_index});
                }
                else
                    return false;
            }
            const bool contributes =
                event.kind == AnalyticOperandOutcomeKind::contributes_final_material;
            const bool survives =
                event.kind == AnalyticOperandOutcomeKind::subtraction_effect_survives;
            if (contributes)
            {
                if (event.result_references.count == 0)
                    return false;
            }
            else if (survives)
            {
                if (operand->operation != 2)
                    return false;
            }
            else if (event.result_references.count != 0)
                return false;
            events.push_back(std::move(temp));
        }
        if (std::find(operand_seen.begin(), operand_seen.end(), 0) != operand_seen.end())
            return false;
        return true;
    }

    bool build_event_reference_sequences(std::vector<EventTemp>& events,
                                         std::vector<RawReference>& raw_references,
                                         std::vector<std::uint64_t>& reference_values)
    {
        if (!budget_.charge_sort(raw_references.size()))
            return fail_resource();
        std::sort(
            raw_references.begin(), raw_references.end(),
            [](const RawReference& left, const RawReference& right)
            { return std::tie(left.event, left.value) < std::tie(right.event, right.value); });
        raw_references.erase(
            std::unique(raw_references.begin(), raw_references.end(),
                        [](const RawReference& left, const RawReference& right)
                        { return left.event == right.event && left.value == right.value; }),
            raw_references.end());
        std::vector<SequenceRange> ranges;
        std::vector<std::uint32_t> labels;
        reference_values.reserve(raw_references.size());
        labels.resize(raw_references.size());
        ranges.resize(events.size());
        std::uint32_t raw_cursor = 0;
        for (std::uint32_t event = 0; event < events.size(); ++event)
        {
            const std::uint32_t begin = raw_cursor;
            while (raw_cursor < raw_references.size() && raw_references[raw_cursor].event == event)
            {
                reference_values.push_back(raw_references[raw_cursor].value);
                ++raw_cursor;
            }
            ranges[event] = {begin, raw_cursor - begin};
            events[event].references = ranges[event];
            if (events[event].record.kind ==
                    AnalyticOperandOutcomeKind::contributes_final_material &&
                std::any_of(reference_values.begin() + begin, reference_values.begin() + raw_cursor,
                            [](std::uint64_t value)
                            { return static_cast<std::uint32_t>(value >> 32U) != 2; }))
                return false;
        }
        if (!budget_.charge_sort(reference_values.size()))
            return fail_resource();
        std::vector<std::uint32_t> reference_order(reference_values.size());
        std::iota(reference_order.begin(), reference_order.end(), 0);
        std::sort(reference_order.begin(), reference_order.end(),
                  [&](std::uint32_t left, std::uint32_t right)
                  { return reference_values[left] < reference_values[right]; });
        std::uint32_t label = 0;
        for (std::uint32_t at = 0; at < reference_order.size(); ++at)
        {
            if (at != 0 &&
                reference_values[reference_order[at - 1]] < reference_values[reference_order[at]])
                ++label;
            labels[reference_order[at]] = label;
        }
        CanonicalSequences sequences;
        std::uint64_t event_scratch = 0;
        if (!add_array_bytes(events.size(), 64, event_scratch) ||
            !add_array_bytes(input_.event_references.size(), 16, event_scratch) ||
            !add_array_bytes(reference_values.size(), 8, event_scratch) ||
            !add_array_bytes(labels.size(), 4, event_scratch) ||
            !add_array_bytes(ranges.size(), 8, event_scratch) ||
            !add_array_bytes(reference_values.size(), 4, event_scratch) ||
            !check_phase(event_scratch))
            return false;
        std::uint64_t event_base = 0;
        if (!persistent_bytes(event_base) || !checked_add(event_base, event_scratch, event_base))
            return fail_resource();
        if (!canonicalize_sequences(labels, ranges, false, event_base, limits_.working_memory_bytes,
                                    budget_, sequences))
            return fail_resource();
        if (!checked_add(event_scratch, sequences.logical_bytes, event_scratch) ||
            !check_phase(event_scratch))
            return false;
        for (std::uint32_t index = 0; index < events.size(); ++index)
            events[index].sequence_rank = sequences.handles[index];
        return true;
    }

    bool publish_events()
    {
        std::vector<EventTemp> events;
        std::vector<RawReference> raw_references;
        std::vector<std::uint64_t> reference_values;
        if (!collect_events(events, raw_references) ||
            !build_event_reference_sequences(events, raw_references, reference_values))
            return false;
        if (!budget_.charge_sort(events.size()))
            return fail_resource();
        std::sort(events.begin(), events.end(),
                  [](const EventTemp& left, const EventTemp& right)
                  {
                      return std::tuple{left.record.operand_id,
                                        static_cast<std::uint16_t>(left.record.kind),
                                        left.sequence_rank, left.record.source_set} <
                             std::tuple{right.record.operand_id,
                                        static_cast<std::uint16_t>(right.record.kind),
                                        right.sequence_rank, right.record.source_set};
                  });
        records_out_.operand_events.reserve(events.size());
        records_out_.ring_region_references.reserve(input_.event_references.size());
        if (!budget_.charge(events.size() + reference_values.size()))
            return fail_resource();
        for (std::uint32_t index = 0; index < events.size(); ++index)
        {
            if (index != 0 &&
                std::tuple{events[index - 1].record.operand_id,
                           static_cast<std::uint16_t>(events[index - 1].record.kind),
                           events[index - 1].sequence_rank, events[index - 1].record.source_set} ==
                    std::tuple{events[index].record.operand_id,
                               static_cast<std::uint16_t>(events[index].record.kind),
                               events[index].sequence_rank, events[index].record.source_set})
                return false;
            auto record = events[index].record;
            record.result_reference_begin =
                events[index].references.count == 0
                    ? 0
                    : static_cast<std::uint32_t>(records_out_.ring_region_references.size());
            record.result_reference_count = events[index].references.count;
            records_out_.ring_region_references.insert(
                records_out_.ring_region_references.end(),
                reference_values.begin() + events[index].references.begin,
                reference_values.begin() + events[index].references.begin +
                    events[index].references.count);
            records_out_.operand_events.push_back(record);
        }
        return true;
    }

    bool finalize_success()
    {
        records_out_.source_references = std::move(source_tables_.sources);
        records_out_.source_sets = std::move(source_tables_.sets);
        records_out_.source_reference_indices = std::move(source_tables_.indices);
        const std::uint64_t job_id = records_.jobs[job_index_].job_id;
        records_out_.job_results.reserve(1);
        records_out_.job_results.push_back(
            {job_id, 0, 0, 0, records_out_.regions.empty() ? 0U : 0U,
             static_cast<std::uint32_t>(records_out_.regions.size()),
             records_out_.operand_events.empty() ? 0U : 0U,
             static_cast<std::uint32_t>(records_out_.operand_events.size())});
        const std::uint64_t packet_bytes =
            512ULL + records_out_.job_results.size() * 48ULL +
            records_out_.vertices.size() * 32ULL + records_out_.fragments.size() * 48ULL +
            records_out_.rings.size() * 32ULL + records_out_.fragment_references.size() * 4ULL +
            records_out_.regions.size() * 24ULL +
            records_out_.ring_region_references.size() * 8ULL +
            records_out_.source_sets.size() * 8ULL + records_out_.source_references.size() * 32ULL +
            records_out_.operand_events.size() * 48ULL +
            records_out_.source_reference_indices.size() * 4ULL;
        if (packet_bytes > 268'435'456ULL || !budget_.charge(packet_bytes / 8 + 1))
            return fail_resource();
        if (!check_phase(packet_bytes))
            return false;
        AnalyticResultPacketEncodeResult encoded =
            analytic_result_detail::encode_canonical_records_unchecked(records_out_);
        if (encoded.error != AnalyticResultPacketLayoutError::none || !encoded.value)
        {
            result_.error = AnalyticFilteredPacketError::encoding_failed;
            return false;
        }
        AnalyticStandaloneJob standalone;
        standalone.records = std::move(records_out_);
        standalone.bytes = std::move(*encoded.value);
        standalone.digest_sha256 = sha256_hex(standalone.bytes.data(), standalone.bytes.size());
        result_.telemetry.emitted_vertices = standalone.records.vertices.size();
        result_.telemetry.emitted_fragments = standalone.records.fragments.size();
        result_.telemetry.emitted_rings = standalone.records.rings.size();
        result_.telemetry.emitted_regions = standalone.records.regions.size();
        result_.telemetry.emitted_events = standalone.records.operand_events.size();
        result_.telemetry.emitted_packet_bytes = standalone.bytes.size();
        result_.standalone = std::move(standalone);
        return true;
    }

    void build_failed_packet(std::uint32_t code)
    {
        AnalyticResultPacketRecords failed;
        const std::uint64_t job_id = records_.jobs[job_index_].job_id;
        failed.job_results.push_back({job_id, 1, 0, 1, 0, 0, 0, 0});
        failed.diagnostics.push_back({code, 1, 1, job_id, 0, 0, 0, 0});
        AnalyticResultPacketEncodeResult encoded =
            analytic_result_detail::encode_canonical_records_unchecked(failed);
        if (encoded.error != AnalyticResultPacketLayoutError::none || !encoded.value)
        {
            result_.error = AnalyticFilteredPacketError::encoding_failed;
            return;
        }
        AnalyticStandaloneJob standalone;
        standalone.records = std::move(failed);
        standalone.bytes = std::move(*encoded.value);
        standalone.digest_sha256 = sha256_hex(standalone.bytes.data(), standalone.bytes.size());
        result_.telemetry.emitted_packet_bytes = standalone.bytes.size();
        result_.standalone = std::move(standalone);
    }

    void build_empty_success_packet()
    {
        AnalyticResultPacketRecords empty;
        const std::uint64_t job_id = records_.jobs[job_index_].job_id;
        empty.job_results.push_back({job_id, 0, 0, 0, 0, 0, 0, 0});
        AnalyticResultPacketEncodeResult encoded =
            analytic_result_detail::encode_canonical_records_unchecked(empty);
        if (encoded.error != AnalyticResultPacketLayoutError::none || !encoded.value)
        {
            result_.error = AnalyticFilteredPacketError::encoding_failed;
            return;
        }
        AnalyticStandaloneJob standalone;
        standalone.records = std::move(empty);
        standalone.bytes = std::move(*encoded.value);
        standalone.digest_sha256 = sha256_hex(standalone.bytes.data(), standalone.bytes.size());
        result_.telemetry.emitted_packet_bytes = standalone.bytes.size();
        result_.standalone = std::move(standalone);
    }

    void clear_publication()
    {
        result_.standalone.reset();
        result_.maps = {};
        records_out_ = {};
    }

    void finish_telemetry()
    {
        result_.telemetry.packet_work_units = preflight_work_ + budget_.used;
        result_.telemetry.predicate_calls =
            result_.telemetry.normalization_work_units + preflight_work_ + budget_.used;
        result_.telemetry.peak_working_memory_bytes =
            std::max({result_.telemetry.peak_working_memory_bytes,
                      result_.telemetry.normalization_peak_working_memory_bytes, reserved_memory_});
    }

    const AnalyticRequestPacketRecords& records_;
    std::uint32_t job_index_ = 0;
    const AnalyticFilteredGeometry& geometry_;
    const std::vector<AnalyticCurvePair>& pairs_;
    AnalyticSolverLimits limits_;
    AnalyticFilteredJobPacketResult result_;
    WorkBudget budget_;
    std::uint64_t reserved_work_ = 0;
    std::uint64_t reserved_memory_ = 0;
    std::uint64_t preflight_work_ = 0;
    std::uint64_t compact_bytes_ = 0;
    std::uint64_t binding_bytes_ = 0;
    std::uint64_t source_use_bytes_ = 0;
    std::uint64_t publication_bytes_ = 0;
    CompactInput input_;
    std::vector<OperandInfo> operands_;
    std::vector<SegmentBinding> segments_;
    std::vector<AnalyticSourceReference> allowed_geometry_sources_;
    std::vector<OperandSource> expected_event_sources_;
    std::vector<SourceUse> source_uses_;
    SourceTables source_tables_;
    std::uint32_t vertex_use_begin_ = 0;
    std::uint32_t fragment_use_begin_ = 0;
    std::uint32_t region_use_begin_ = 0;
    std::uint32_t event_use_begin_ = 0;
    std::vector<std::uint32_t> normalized_vertex_to_packet_;
    std::vector<std::uint32_t> normalized_fragment_to_packet_;
    std::vector<std::uint32_t> normalized_ring_to_packet_;
    std::vector<std::uint32_t> normalized_region_to_packet_;
    AnalyticResultPacketRecords records_out_;
};

} // namespace

AnalyticFilteredJobPacketResult build_analytic_filtered_job_packet(
    const AnalyticRequestPacketRecords& records, std::uint32_t job_index,
    const AnalyticFilteredGeometry& geometry, const std::vector<AnalyticCurvePair>& candidate_pairs,
    const AnalyticSolverLimits& limits)
{
    return PacketBuilder(records, job_index, geometry, candidate_pairs, limits).build();
}

} // namespace geometer

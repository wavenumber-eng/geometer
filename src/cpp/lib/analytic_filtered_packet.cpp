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
using analytic_packet_detail::admit_packet_encoding_memory;
using analytic_packet_detail::canonicalize_sequences;
using analytic_packet_detail::CanonicalSequences;
using analytic_packet_detail::checked_add;
using analytic_packet_detail::checked_multiply;
using analytic_packet_detail::result_packet_records_logical_bytes;
using analytic_packet_detail::result_packet_records_logical_capacity_bytes;
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
    std::uint64_t source_capacity = 0;
    std::uint64_t set_capacity = 0;
    std::uint64_t index_capacity = 0;
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
    case AnalyticSourceRole::swept_left_offset_line:
    case AnalyticSourceRole::swept_left_offset_arc:
    case AnalyticSourceRole::swept_right_offset_line:
    case AnalyticSourceRole::swept_right_offset_arc:
    case AnalyticSourceRole::swept_end_cap:
        return static_cast<std::uint32_t>(source.secondary_id >> 32U) != 0 &&
               static_cast<std::uint32_t>(source.secondary_id) == 0;
    case AnalyticSourceRole::swept_round_join:
    {
        const std::uint32_t incoming = static_cast<std::uint32_t>(source.secondary_id >> 32U);
        const std::uint32_t outgoing = static_cast<std::uint32_t>(source.secondary_id);
        return incoming != 0 && outgoing == incoming + 1U;
    }
    case AnalyticSourceRole::swept_start_cap:
        return source.secondary_id == (std::uint64_t{1} << 32U);
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
    case 5:
    {
        if (operand->geometry_index >= records.swept_paths.size())
            return false;
        const auto& swept = records.swept_paths[operand->geometry_index];
        if (swept.path_ring >= records.rings.size())
            return false;
        const std::uint32_t segment_count = records.rings[swept.path_ring].segment_count;
        const std::uint32_t high = static_cast<std::uint32_t>(source.secondary_id >> 32U);
        const std::uint32_t low = static_cast<std::uint32_t>(source.secondary_id);
        const bool offset = source.role == AnalyticSourceRole::swept_left_offset_line ||
                            source.role == AnalyticSourceRole::swept_left_offset_arc ||
                            source.role == AnalyticSourceRole::swept_right_offset_line ||
                            source.role == AnalyticSourceRole::swept_right_offset_arc;
        const bool line_offset = source.role == AnalyticSourceRole::swept_left_offset_line ||
                                 source.role == AnalyticSourceRole::swept_right_offset_line;
        const bool arc_offset = source.role == AnalyticSourceRole::swept_left_offset_arc ||
                                source.role == AnalyticSourceRole::swept_right_offset_arc;
        bool offset_matches_segment = !offset;
        if (offset && high >= 1 && high <= segment_count)
        {
            const auto& ring = records.rings[swept.path_ring];
            if (ring.segment_begin > records.segments.size() ||
                high > records.segments.size() - ring.segment_begin)
                return false;
            const std::uint8_t segment_kind = records.segments[ring.segment_begin + high - 1U].kind;
            offset_matches_segment =
                (line_offset && segment_kind == 1) || (arc_offset && segment_kind == 2);
        }
        const bool valid_key =
            (offset && offset_matches_segment && high >= 1 && high <= segment_count && low == 0) ||
            (source.role == AnalyticSourceRole::swept_start_cap && high == 1 && low == 0) ||
            (source.role == AnalyticSourceRole::swept_end_cap && high == segment_count + 1U &&
             low == 0) ||
            (source.role == AnalyticSourceRole::swept_round_join && high >= 1 &&
             high < segment_count && low == high + 1U);
        if (!valid_key)
            return false;
        feature = swept.feature_id;
        break;
    }
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

#include "analytic_filtered_packet_builder.h"

} // namespace

AnalyticFilteredJobPacketResult build_analytic_filtered_job_packet(
    const AnalyticRequestPacketRecords& records, std::uint32_t job_index,
    const AnalyticFilteredGeometry& geometry, const std::vector<AnalyticCurvePair>& candidate_pairs,
    const AnalyticSolverLimits& limits)
{
    return PacketBuilder(records, job_index, geometry, candidate_pairs, limits, true).build();
}

AnalyticFilteredJobRecordsResult build_analytic_filtered_job_records(
    const AnalyticRequestPacketRecords& records, std::uint32_t job_index,
    const AnalyticFilteredGeometry& geometry, const std::vector<AnalyticCurvePair>& candidate_pairs,
    const AnalyticSolverLimits& limits)
{
    AnalyticFilteredJobPacketResult packet =
        PacketBuilder(records, job_index, geometry, candidate_pairs, limits, false).build();
    AnalyticFilteredJobRecordsResult result;
    result.error = packet.error;
    result.normalization_error = packet.normalization_error;
    result.maps = std::move(packet.maps);
    result.telemetry = packet.telemetry;
    if (packet.standalone)
        result.records = std::move(packet.standalone->records);
    return result;
}

} // namespace geometer

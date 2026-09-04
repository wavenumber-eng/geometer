#include "geometer/analytic_filtered_batch.h"

#include "analytic_filtered_packet_sequences.h"
#include "analytic_filtered_relationships.h"
#include "analytic_result_packet_records_internal.h"
#include "geometer/analytic_curve_broad_phase.h"
#include "geometer/analytic_filtered_lowering.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <tuple>
#include <utility>
#include <vector>

namespace geometer
{
namespace
{

using analytic_packet_detail::admit_packet_encoding_memory;
using analytic_packet_detail::canonical_sequence_scratch_bytes;
using analytic_packet_detail::canonicalize_sequences;
using analytic_packet_detail::CanonicalSequences;
using analytic_packet_detail::checked_add;
using analytic_packet_detail::checked_multiply;
using analytic_packet_detail::result_packet_records_logical_bytes;
using analytic_packet_detail::result_packet_records_logical_capacity_bytes;
using analytic_packet_detail::SequenceRange;
using analytic_packet_detail::sort_units;
using analytic_packet_detail::WorkBudget;

constexpr std::uint64_t kBatchJobSlotBytes = 448;
constexpr std::uint64_t kBatchTelemetrySlotBytes = 80;
constexpr std::uint64_t kMinimumJobClosureBytes = 616;
constexpr std::uint64_t kSourceEntryBytes = 48;
constexpr std::uint64_t kSourceReferenceBytes = 32;
constexpr std::uint64_t kSourceMapBytes = 4;
constexpr std::uint64_t kSetMapBytes = 4;
constexpr std::uint64_t kOffsetBytes = 8;
constexpr std::uint64_t kMaximumPacketBytes = 268'435'456;

struct SourceEntry
{
    AnalyticSourceReference source;
    std::uint32_t flat_index = 0;
};

static_assert(sizeof(SourceEntry) <= kSourceEntryBytes);
static_assert(sizeof(AnalyticSourceReference) <= kSourceReferenceBytes);
static_assert(sizeof(AnalyticResultPacketRecords) + sizeof(AnalyticFilteredBatchJobTelemetry) <=
              kBatchJobSlotBytes);
static_assert(sizeof(AnalyticFilteredBatchJobTelemetry) <= kBatchTelemetrySlotBytes);

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

AnalyticResultPacketRecords failed_job(std::uint64_t job_id, std::uint32_t code)
{
    AnalyticResultPacketRecords records;
    records.job_results.push_back({job_id, 1, 0, 1, 0, 0, 0, 0});
    records.diagnostics.push_back({code, 1, 1, job_id, 0, 0, 0, 0});
    return records;
}

bool is_resource_failed_job(const AnalyticResultPacketRecords& records) noexcept
{
    return records.job_results.size() == 1 && records.job_results.front().status == 1 &&
           records.diagnostics.size() == 1 && records.diagnostics.front().code == 65'547;
}

std::uint32_t lowering_diagnostic(AnalyticFilteredLoweringError error) noexcept
{
    switch (error)
    {
    case AnalyticFilteredLoweringError::invalid_topology:
        return 65'539;
    case AnalyticFilteredLoweringError::invalid_arc:
        return 65'540;
    case AnalyticFilteredLoweringError::unsupported_geometry:
        return 65'541;
    case AnalyticFilteredLoweringError::resource_limit_exceeded:
        return 65'547;
    case AnalyticFilteredLoweringError::none:
        break;
    }
    return 65'546;
}

enum class LimitAvailability
{
    available,
    job_resource,
    batch_resource,
};

LimitAvailability remaining_limits(const AnalyticSolverLimits& source, std::uint64_t used_work,
                                   std::uint64_t job_live_bytes, std::uint64_t batch_live_bytes,
                                   std::uint64_t batch_memory, AnalyticSolverLimits& output,
                                   bool& batch_memory_constrained) noexcept
{
    batch_memory_constrained = false;
    if (used_work > source.predicate_calls || job_live_bytes > source.working_memory_bytes)
        return LimitAvailability::job_resource;
    if (batch_live_bytes > batch_memory)
        return LimitAvailability::batch_resource;
    output = source;
    output.predicate_calls -= used_work;
    const std::uint64_t job_remaining = source.working_memory_bytes - job_live_bytes;
    const std::uint64_t batch_remaining = batch_memory - batch_live_bytes;
    batch_memory_constrained = batch_remaining < job_remaining;
    output.working_memory_bytes = std::min(job_remaining, batch_remaining);
    return LimitAvailability::available;
}

bool add_telemetry(std::uint64_t value, std::uint64_t& total) noexcept
{
    return checked_add(total, value, total);
}

bool request_validation_requirements(const AnalyticRequestPacketRecords& records,
                                     std::uint64_t& work, std::uint64_t& bytes) noexcept
{
    const std::uint64_t feature_count = records.disks.size() + records.annuli.size() +
                                        records.capsules.size() + records.swept_paths.size();
    const std::array<std::uint64_t, 8> id_counts{
        records.jobs.size(),           records.stages.size(), records.operands.size(),
        records.planar_regions.size(), records.rings.size(),  records.vertices.size(),
        records.segments.size(),       feature_count,
    };
    const std::uint64_t record_visits = records.jobs.size() + records.stages.size() +
                                        records.operands.size() + records.planar_regions.size() +
                                        records.ring_references.size() + records.rings.size() +
                                        records.vertices.size() + records.segments.size() +
                                        feature_count + records.relationship_queries.size();
    if (!checked_multiply(record_visits, 4, work))
        return false;
    bytes = 0;
    std::uint64_t term = 0;
    for (std::uint64_t count : id_counts)
    {
        if (!checked_add(work, sort_units(count), work) || !checked_multiply(count, 8, term) ||
            !checked_add(bytes, term, bytes))
            return false;
    }
    std::uint64_t search_depth = 1;
    for (std::uint64_t count = records.jobs.size(); count > 1; count = (count + 1) / 2)
        ++search_depth;
    if (!checked_multiply(records.relationship_queries.size(), search_depth * 2, term) ||
        !checked_add(work, term, work))
        return false;
    // Ring and path identifiers are held in distinct, simultaneously live arrays.
    return checked_multiply(records.rings.size(), 8, term) && checked_add(bytes, term, bytes);
}

struct Totals
{
    std::uint64_t jobs = 0;
    std::uint64_t diagnostics = 0;
    std::uint64_t vertices = 0;
    std::uint64_t fragments = 0;
    std::uint64_t rings = 0;
    std::uint64_t fragment_references = 0;
    std::uint64_t regions = 0;
    std::uint64_t ring_region_references = 0;
    std::uint64_t source_sets = 0;
    std::uint64_t sources = 0;
    std::uint64_t events = 0;
    std::uint64_t memberships = 0;
};

bool add_count(std::size_t value, std::uint64_t& total) noexcept
{
    return checked_add(total, value, total) && total <= std::numeric_limits<std::uint32_t>::max();
}

bool collect_totals(const std::vector<AnalyticResultPacketRecords>& jobs, Totals& totals) noexcept
{
    for (const auto& job : jobs)
    {
        if (job.job_results.size() != 1 || !job.relationship_results.empty() ||
            !job.relationship_pairs.empty() || !add_count(job.job_results.size(), totals.jobs) ||
            !add_count(job.diagnostics.size(), totals.diagnostics) ||
            !add_count(job.vertices.size(), totals.vertices) ||
            !add_count(job.fragments.size(), totals.fragments) ||
            !add_count(job.rings.size(), totals.rings) ||
            !add_count(job.fragment_references.size(), totals.fragment_references) ||
            !add_count(job.regions.size(), totals.regions) ||
            !add_count(job.ring_region_references.size(), totals.ring_region_references) ||
            !add_count(job.source_sets.size(), totals.source_sets) ||
            !add_count(job.source_references.size(), totals.sources) ||
            !add_count(job.operand_events.size(), totals.events) ||
            !add_count(job.source_reference_indices.size(), totals.memberships))
            return false;
    }
    return true;
}

bool merged_capacity_requirements(const Totals& totals, std::uint64_t& records_bytes,
                                  std::uint64_t& packet_bytes) noexcept
{
    constexpr std::array<std::uint64_t, 14> sizes{48, 56, 32, 48, 32, 4,  24,
                                                  8,  8,  32, 48, 32, 32, 4};
    const std::array<std::uint64_t, 14> counts{
        totals.jobs,
        totals.diagnostics,
        totals.vertices,
        totals.fragments,
        totals.rings,
        totals.fragment_references,
        totals.regions,
        totals.ring_region_references,
        totals.source_sets,
        totals.sources,
        totals.events,
        0,
        0,
        totals.memberships,
    };
    records_bytes = 512;
    packet_bytes = 512;
    for (std::size_t index = 0; index < counts.size(); ++index)
    {
        std::uint64_t term = 0;
        if (!checked_multiply(counts[index], sizes[index], term) ||
            !checked_add(records_bytes, term, records_bytes) ||
            packet_bytes > std::numeric_limits<std::uint64_t>::max() - 7)
            return false;
        packet_bytes = (packet_bytes + 7) & ~std::uint64_t{7};
        if (!checked_add(packet_bytes, term, packet_bytes))
            return false;
    }
    return true;
}

bool valid_range(std::uint32_t begin, std::uint32_t count, std::size_t size) noexcept
{
    return count == 0 ? begin == 0
                      : begin <= size && count <= size - static_cast<std::size_t>(begin);
}

std::uint64_t merged_validation_visits(const Totals& totals) noexcept
{
    return totals.jobs + totals.diagnostics + totals.vertices + totals.fragments + totals.rings +
           totals.fragment_references + totals.regions + totals.ring_region_references +
           totals.source_sets + totals.sources + totals.events + totals.memberships;
}

bool validate_merged_batch(const AnalyticResultPacketRecords& merged,
                           const AnalyticRequestPacketRecords& request) noexcept
{
    if (merged.job_results.size() != request.jobs.size() ||
        merged.relationship_results.size() != request.relationship_queries.size())
        return false;

    for (std::size_t index = 0; index < merged.relationship_results.size(); ++index)
        if (merged.relationship_results[index].query_id !=
            request.relationship_queries[index].query_id)
            return false;

    std::size_t diagnostic_cursor = 0;
    std::size_t region_cursor = 0;
    std::size_t event_cursor = 0;
    for (std::size_t index = 0; index < merged.job_results.size(); ++index)
    {
        const auto& job = merged.job_results[index];
        if (job.job_id != request.jobs[index].job_id || job.status > 1 ||
            !valid_range(job.diagnostic_begin, job.diagnostic_count, merged.diagnostics.size()) ||
            !valid_range(job.result_region_begin, job.result_region_count, merged.regions.size()) ||
            !valid_range(job.operand_event_begin, job.operand_event_count,
                         merged.operand_events.size()) ||
            (job.diagnostic_count != 0 && job.diagnostic_begin != diagnostic_cursor) ||
            (job.result_region_count != 0 && job.result_region_begin != region_cursor) ||
            (job.operand_event_count != 0 && job.operand_event_begin != event_cursor) ||
            (job.status == 1 && (job.diagnostic_count == 0 || job.result_region_count != 0 ||
                                 job.operand_event_count != 0)))
            return false;
        bool has_error = false;
        for (std::uint32_t offset = 0; offset < job.diagnostic_count; ++offset)
        {
            const auto& diagnostic = merged.diagnostics[job.diagnostic_begin + offset];
            if (diagnostic.job_id != job.job_id || diagnostic.severity < 1 ||
                diagnostic.severity > 2)
                return false;
            has_error = has_error || diagnostic.severity == 1;
        }
        if ((job.status == 1) != has_error)
            return false;
        diagnostic_cursor += job.diagnostic_count;
        region_cursor += job.result_region_count;
        event_cursor += job.operand_event_count;
    }
    if (diagnostic_cursor != merged.diagnostics.size() || region_cursor != merged.regions.size() ||
        event_cursor != merged.operand_events.size())
        return false;

    for (std::size_t index = 0; index < merged.source_references.size(); ++index)
        if (index != 0 &&
            !source_less(merged.source_references[index - 1], merged.source_references[index]))
            return false;
    std::size_t source_index_cursor = 0;
    for (const auto& set : merged.source_sets)
    {
        if (set.source_reference_index_count == 0 ||
            set.source_reference_index_begin != source_index_cursor ||
            !valid_range(set.source_reference_index_begin, set.source_reference_index_count,
                         merged.source_reference_indices.size()))
            return false;
        std::uint32_t previous = 0;
        for (std::uint32_t offset = 0; offset < set.source_reference_index_count; ++offset)
        {
            const std::uint32_t source =
                merged.source_reference_indices[set.source_reference_index_begin + offset];
            if (source >= merged.source_references.size() || (offset != 0 && source <= previous))
                return false;
            previous = source;
        }
        source_index_cursor += set.source_reference_index_count;
    }
    if (source_index_cursor != merged.source_reference_indices.size())
        return false;

    const std::uint32_t set_count = static_cast<std::uint32_t>(merged.source_sets.size());
    for (std::size_t index = 0; index < merged.vertices.size(); ++index)
        if (merged.vertices[index].id != index + 1 ||
            merged.vertices[index].intersection_source_set > set_count)
            return false;
    for (std::size_t index = 0; index < merged.fragments.size(); ++index)
    {
        const auto& fragment = merged.fragments[index];
        if (fragment.id != index + 1 || fragment.start_vertex >= merged.vertices.size() ||
            fragment.end_vertex >= merged.vertices.size() ||
            fragment.positive_source_set > set_count || fragment.subtraction_source_set > set_count)
            return false;
    }
    std::size_t fragment_cursor = 0;
    for (std::size_t index = 0; index < merged.rings.size(); ++index)
    {
        const auto& ring = merged.rings[index];
        if (ring.id != index + 1 || ring.fragment_reference_begin != fragment_cursor ||
            !valid_range(ring.fragment_reference_begin, ring.fragment_reference_count,
                         merged.fragment_references.size()) ||
            (ring.parent_ring != std::numeric_limits<std::uint32_t>::max() &&
             ring.parent_ring >= merged.rings.size()))
            return false;
        fragment_cursor += ring.fragment_reference_count;
    }
    if (fragment_cursor != merged.fragment_references.size() ||
        std::any_of(merged.fragment_references.begin(), merged.fragment_references.end(),
                    [&](std::uint32_t fragment) { return fragment >= merged.fragments.size(); }))
        return false;
    for (std::size_t index = 0; index < merged.regions.size(); ++index)
        if (merged.regions[index].id != index + 1 ||
            merged.regions[index].outer_ring >= merged.rings.size() ||
            merged.regions[index].positive_source_set == 0 ||
            merged.regions[index].positive_source_set > set_count)
            return false;

    std::size_t reference_cursor = 0;
    for (const auto& event : merged.operand_events)
    {
        if (event.source_set == 0 || event.source_set > set_count ||
            event.result_reference_begin != (event.result_reference_count == 0
                                                 ? 0U
                                                 : static_cast<std::uint32_t>(reference_cursor)) ||
            !valid_range(event.result_reference_begin, event.result_reference_count,
                         merged.ring_region_references.size()))
            return false;
        reference_cursor += event.result_reference_count;
    }
    if (reference_cursor != merged.ring_region_references.size())
        return false;
    for (std::uint64_t reference : merged.ring_region_references)
    {
        const std::uint32_t kind = static_cast<std::uint32_t>(reference >> 32U);
        const std::uint32_t value = static_cast<std::uint32_t>(reference);
        if ((kind == 1 && value >= merged.rings.size()) ||
            (kind == 2 && value >= merged.regions.size()) || (kind != 1 && kind != 2))
            return false;
    }
    return true;
}

std::uint32_t mapped_set(std::uint32_t local_handle, std::uint64_t set_offset,
                         const std::vector<std::uint32_t>& set_map) noexcept
{
    if (local_handle == 0)
        return 0;
    const std::uint64_t index = set_offset + local_handle - 1;
    return index < set_map.size() ? set_map[static_cast<std::size_t>(index)] : 0;
}

enum class MergeError
{
    none,
    resource,
    solver,
    invalid,
    encoding,
};

MergeError merge_jobs(std::vector<AnalyticResultPacketRecords>& jobs,
                      const AnalyticRequestPacketRecords& request,
                      const AnalyticFilteredBatchLimits& limits, std::uint64_t validation_work,
                      AnalyticFilteredBatchResult& result)
{
    AnalyticFilteredPacketTelemetry merge_packet_telemetry;
    WorkBudget budget{limits.assembly_work_units, validation_work, &merge_packet_telemetry};
    struct MergeAccounting
    {
        AnalyticFilteredBatchResult& result;
        const WorkBudget& budget;
        const AnalyticFilteredPacketTelemetry& telemetry;
        ~MergeAccounting()
        {
            result.telemetry.merge_work_units = budget.used;
            result.telemetry.sequence_table_probes = telemetry.sequence_table_probes;
        }
    } accounting{result, budget, merge_packet_telemetry};
    if (!budget.charge(jobs.size()))
        return MergeError::resource;
    Totals totals;
    if (!collect_totals(jobs, totals))
        return MergeError::invalid;
    std::uint64_t expansion_visits = 0;
    std::uint64_t twice_fragments = 0;
    if (!checked_multiply(totals.fragments, 2, twice_fragments) ||
        !checked_add(totals.vertices, twice_fragments, expansion_visits) ||
        !checked_add(expansion_visits, totals.regions, expansion_visits) ||
        !checked_add(expansion_visits, totals.events, expansion_visits) ||
        !budget.charge(jobs.size()) || !budget.charge(expansion_visits))
        return MergeError::resource;
    std::uint64_t logical_source_expansions = 0;
    for (const auto& job : jobs)
    {
        const auto expansion = analytic_result_detail::charge_logical_source_reference_expansions(
            job, logical_source_expansions);
        if (expansion == AnalyticResultPacketLayoutError::limit_exceeded)
            return MergeError::resource;
        if (expansion != AnalyticResultPacketLayoutError::none)
            return MergeError::invalid;
    }

    std::uint64_t retained_jobs = result.telemetry.retained_job_records_bytes;
    std::uint64_t term = 0;
    std::uint64_t maximum_records_bytes = 0;
    std::uint64_t maximum_packet_bytes = 0;
    std::uint64_t telemetry_bytes = 0;
    std::uint64_t publication_peak = retained_jobs;
    std::uint64_t encoding_peak = 0;
    std::uint64_t encoding_scratch = 0;
    if (!merged_capacity_requirements(totals, maximum_records_bytes, maximum_packet_bytes) ||
        maximum_packet_bytes > kMaximumPacketBytes ||
        !checked_multiply(result.jobs.size(), kBatchTelemetrySlotBytes, telemetry_bytes) ||
        !checked_multiply(totals.sources, kSourceMapBytes, term) ||
        !checked_add(publication_peak, term, publication_peak) ||
        !checked_multiply(totals.source_sets, kSetMapBytes, term) ||
        !checked_add(publication_peak, term, publication_peak) ||
        !checked_multiply(jobs.size() + 1, kOffsetBytes * 2, term) ||
        !checked_add(publication_peak, term, publication_peak) ||
        !checked_add(publication_peak, maximum_records_bytes, publication_peak) ||
        !checked_add(maximum_records_bytes, telemetry_bytes, encoding_peak) ||
        !checked_multiply(maximum_packet_bytes, 2, encoding_scratch) ||
        !checked_add(encoding_peak, encoding_scratch, encoding_peak) ||
        std::max(publication_peak, encoding_peak) > limits.working_memory_bytes)
        return MergeError::resource;
    result.telemetry.peak_working_memory_bytes = std::max(
        result.telemetry.peak_working_memory_bytes, std::max(publication_peak, encoding_peak));

    const std::uint64_t copy_visits =
        totals.jobs + totals.diagnostics + totals.vertices + totals.fragments + totals.rings +
        totals.fragment_references + totals.regions + totals.ring_region_references + totals.events;
    std::uint64_t reserved_finish_work = 0;
    if (!checked_add(copy_visits, merged_validation_visits(totals), reserved_finish_work) ||
        !checked_add(reserved_finish_work, maximum_packet_bytes / 8 + 1, reserved_finish_work) ||
        !budget.charge(reserved_finish_work))
        return MergeError::resource;

    std::uint64_t source_phase = retained_jobs;
    if (!checked_multiply(totals.sources,
                          kSourceEntryBytes + kSourceReferenceBytes + kSourceMapBytes, term) ||
        !checked_add(source_phase, term, source_phase) ||
        !checked_multiply(jobs.size() + 1, kOffsetBytes * 2, term) ||
        !checked_add(source_phase, term, source_phase) ||
        source_phase > limits.working_memory_bytes)
        return MergeError::resource;
    std::uint64_t sequence_base = source_phase;
    std::uint64_t sequence_scratch = 0;
    std::uint64_t sequence_peak = 0;
    if (!checked_multiply(totals.memberships, 4, term) ||
        !checked_add(sequence_base, term, sequence_base) ||
        !checked_multiply(totals.source_sets, 8 + kSetMapBytes, term) ||
        !checked_add(sequence_base, term, sequence_base) ||
        (sequence_scratch =
             canonical_sequence_scratch_bytes(totals.memberships, totals.source_sets, true)) ==
            std::numeric_limits<std::uint64_t>::max() ||
        !checked_add(sequence_base, sequence_scratch, sequence_peak) ||
        sequence_peak > limits.working_memory_bytes)
        return MergeError::resource;
    result.telemetry.peak_working_memory_bytes =
        std::max(result.telemetry.peak_working_memory_bytes, sequence_peak);
    std::uint64_t fixed_source_work = 0;
    if (!checked_multiply(totals.sources, 2, fixed_source_work) ||
        !checked_add(fixed_source_work, sort_units(totals.sources), fixed_source_work) ||
        !checked_add(fixed_source_work, totals.source_sets, fixed_source_work) ||
        !checked_add(fixed_source_work, totals.memberships, fixed_source_work) ||
        !budget.charge(fixed_source_work))
        return MergeError::resource;

    try
    {
        std::vector<std::uint64_t> source_offsets(jobs.size() + 1);
        std::vector<std::uint64_t> set_offsets(jobs.size() + 1);
        for (std::size_t index = 0; index < jobs.size(); ++index)
        {
            source_offsets[index + 1] =
                source_offsets[index] + jobs[index].source_references.size();
            set_offsets[index + 1] = set_offsets[index] + jobs[index].source_sets.size();
        }

        std::vector<SourceEntry> entries;
        entries.reserve(static_cast<std::size_t>(totals.sources));
        std::vector<std::uint32_t> source_map(static_cast<std::size_t>(totals.sources));
        for (std::uint32_t job = 0; job < jobs.size(); ++job)
            for (std::uint32_t local = 0; local < jobs[job].source_references.size(); ++local)
                entries.push_back({jobs[job].source_references[local],
                                   static_cast<std::uint32_t>(source_offsets[job] + local)});
        std::sort(entries.begin(), entries.end(),
                  [](const SourceEntry& left, const SourceEntry& right)
                  {
                      if (source_less(left.source, right.source))
                          return true;
                      if (source_less(right.source, left.source))
                          return false;
                      return left.flat_index < right.flat_index;
                  });
        std::vector<AnalyticSourceReference> global_sources;
        global_sources.reserve(entries.size());
        for (const auto& entry : entries)
        {
            if (!global_sources.empty() && source_equal(global_sources.back(), entry.source))
                return MergeError::invalid;
            global_sources.push_back(entry.source);
            source_map[entry.flat_index] = static_cast<std::uint32_t>(global_sources.size() - 1);
        }

        std::vector<std::uint32_t> labels;
        labels.reserve(static_cast<std::size_t>(totals.memberships));
        std::vector<SequenceRange> ranges;
        ranges.reserve(static_cast<std::size_t>(totals.source_sets));
        for (std::uint32_t job = 0; job < jobs.size(); ++job)
            for (const auto& set : jobs[job].source_sets)
            {
                if (set.source_reference_index_begin > jobs[job].source_reference_indices.size() ||
                    set.source_reference_index_count > jobs[job].source_reference_indices.size() -
                                                           set.source_reference_index_begin)
                    return MergeError::invalid;
                ranges.push_back(
                    {static_cast<std::uint32_t>(labels.size()), set.source_reference_index_count});
                for (std::uint32_t offset = 0; offset < set.source_reference_index_count; ++offset)
                {
                    const std::uint32_t local =
                        jobs[job]
                            .source_reference_indices[set.source_reference_index_begin + offset];
                    if (local >= jobs[job].source_references.size())
                        return MergeError::invalid;
                    labels.push_back(source_map[source_offsets[job] + local]);
                }
            }

        CanonicalSequences sequences;
        if (!canonicalize_sequences(labels, ranges, true, sequence_base,
                                    limits.working_memory_bytes, budget, sequences))
            return MergeError::resource;
        std::vector<std::uint32_t> set_map = std::move(sequences.handles);
        std::vector<SourceEntry>().swap(entries);
        std::vector<std::uint32_t>().swap(labels);
        std::vector<SequenceRange>().swap(ranges);

        AnalyticResultPacketRecords merged;
        merged.source_references = std::move(global_sources);
        merged.source_sets = std::move(sequences.records);
        merged.source_reference_indices = std::move(sequences.indices);

        std::uint64_t publication_bytes = result_packet_records_logical_bytes(merged);
        // The empty-size calculation above includes persistent source tables;
        // add conservative fixed charges for every not-yet-filled output row.
        constexpr std::array<std::uint64_t, 10> row_bytes{48, 56, 32, 48, 32, 4, 24, 8, 48, 8};
        const std::array<std::uint64_t, 10> row_counts{
            totals.jobs,     totals.diagnostics,
            totals.vertices, totals.fragments,
            totals.rings,    totals.fragment_references,
            totals.regions,  totals.ring_region_references,
            totals.events,   0};
        for (std::size_t index = 0; index < row_bytes.size(); ++index)
            if (!checked_multiply(row_counts[index], row_bytes[index], term) ||
                !checked_add(publication_bytes, term, publication_bytes))
                return MergeError::resource;
        std::uint64_t actual_publication_peak = retained_jobs;
        if (!checked_add(actual_publication_peak, totals.sources * kSourceMapBytes,
                         actual_publication_peak) ||
            !checked_add(actual_publication_peak, totals.source_sets * kSetMapBytes,
                         actual_publication_peak) ||
            !checked_add(actual_publication_peak, (jobs.size() + 1) * kOffsetBytes * 2,
                         actual_publication_peak) ||
            !checked_add(actual_publication_peak, publication_bytes, actual_publication_peak) ||
            actual_publication_peak > limits.working_memory_bytes)
            return MergeError::resource;
        result.telemetry.peak_working_memory_bytes =
            std::max(result.telemetry.peak_working_memory_bytes, actual_publication_peak);

        // Reserve every remaining output table exactly once after the exact
        // publication phase has been admitted.
        merged.job_results.reserve(static_cast<std::size_t>(totals.jobs));
        merged.diagnostics.reserve(static_cast<std::size_t>(totals.diagnostics));
        merged.vertices.reserve(static_cast<std::size_t>(totals.vertices));
        merged.fragments.reserve(static_cast<std::size_t>(totals.fragments));
        merged.rings.reserve(static_cast<std::size_t>(totals.rings));
        merged.fragment_references.reserve(static_cast<std::size_t>(totals.fragment_references));
        merged.regions.reserve(static_cast<std::size_t>(totals.regions));
        merged.ring_region_references.reserve(
            static_cast<std::size_t>(totals.ring_region_references));
        merged.operand_events.reserve(static_cast<std::size_t>(totals.events));

        std::uint32_t vertex_base = 0;
        std::uint32_t fragment_base = 0;
        std::uint32_t ring_base = 0;
        std::uint32_t region_base = 0;
        for (std::uint32_t job_index = 0; job_index < jobs.size(); ++job_index)
        {
            const auto& local = jobs[job_index];
            const std::uint64_t local_set_offset = set_offsets[job_index];
            const std::uint32_t diagnostic_begin =
                local.diagnostics.empty() ? 0U
                                          : static_cast<std::uint32_t>(merged.diagnostics.size());
            const std::uint32_t region_begin =
                local.regions.empty() ? 0U : static_cast<std::uint32_t>(merged.regions.size());
            const std::uint32_t event_begin =
                local.operand_events.empty()
                    ? 0U
                    : static_cast<std::uint32_t>(merged.operand_events.size());
            merged.diagnostics.insert(merged.diagnostics.end(), local.diagnostics.begin(),
                                      local.diagnostics.end());
            for (auto vertex : local.vertices)
            {
                vertex.id = merged.vertices.size() + 1;
                vertex.intersection_source_set =
                    mapped_set(vertex.intersection_source_set, local_set_offset, set_map);
                merged.vertices.push_back(vertex);
            }
            for (auto fragment : local.fragments)
            {
                fragment.id = merged.fragments.size() + 1;
                fragment.start_vertex += vertex_base;
                fragment.end_vertex += vertex_base;
                fragment.positive_source_set =
                    mapped_set(fragment.positive_source_set, local_set_offset, set_map);
                fragment.subtraction_source_set =
                    mapped_set(fragment.subtraction_source_set, local_set_offset, set_map);
                merged.fragments.push_back(fragment);
            }
            for (const auto& local_ring : local.rings)
            {
                auto ring = local_ring;
                ring.id = merged.rings.size() + 1;
                ring.fragment_reference_begin = merged.fragment_references.size();
                if (ring.parent_ring != std::numeric_limits<std::uint32_t>::max())
                    ring.parent_ring += ring_base;
                for (std::uint32_t offset = 0; offset < local_ring.fragment_reference_count;
                     ++offset)
                    merged.fragment_references.push_back(
                        local.fragment_references[local_ring.fragment_reference_begin + offset] +
                        fragment_base);
                merged.rings.push_back(ring);
            }
            for (auto region : local.regions)
            {
                region.id = merged.regions.size() + 1;
                region.outer_ring += ring_base;
                region.positive_source_set =
                    mapped_set(region.positive_source_set, local_set_offset, set_map);
                merged.regions.push_back(region);
            }
            for (auto event : local.operand_events)
            {
                const std::uint32_t begin = event.result_reference_begin;
                event.result_reference_begin =
                    event.result_reference_count == 0
                        ? 0U
                        : static_cast<std::uint32_t>(merged.ring_region_references.size());
                event.source_set = mapped_set(event.source_set, local_set_offset, set_map);
                for (std::uint32_t offset = 0; offset < event.result_reference_count; ++offset)
                {
                    const std::uint64_t reference = local.ring_region_references[begin + offset];
                    const std::uint32_t kind = static_cast<std::uint32_t>(reference >> 32U);
                    std::uint32_t index = static_cast<std::uint32_t>(reference);
                    index += kind == 1 ? ring_base : region_base;
                    merged.ring_region_references.push_back(
                        (static_cast<std::uint64_t>(kind) << 32U) | index);
                }
                merged.operand_events.push_back(event);
            }
            const auto& local_job = local.job_results.front();
            merged.job_results.push_back(
                {local_job.job_id, local_job.status, diagnostic_begin,
                 static_cast<std::uint32_t>(local.diagnostics.size()), region_begin,
                 static_cast<std::uint32_t>(local.regions.size()), event_begin,
                 static_cast<std::uint32_t>(local.operand_events.size())});
            vertex_base += static_cast<std::uint32_t>(local.vertices.size());
            fragment_base += static_cast<std::uint32_t>(local.fragments.size());
            ring_base += static_cast<std::uint32_t>(local.rings.size());
            region_base += static_cast<std::uint32_t>(local.regions.size());
        }

        if (!request.relationship_queries.empty())
        {
            const std::uint64_t remaining_work = budget.used <= limits.assembly_work_units
                                                     ? limits.assembly_work_units - budget.used
                                                     : 0;
            std::uint64_t base_packet_bytes = 0;
            std::uint64_t base_encoding_peak = 0;
            if (!admit_packet_encoding_memory(merged, 0, limits.working_memory_bytes,
                                              base_packet_bytes, base_encoding_peak) ||
                base_packet_bytes > kMaximumPacketBytes)
                return MergeError::resource;
            auto relationships = analytic_relationship_detail::evaluate(
                request, merged, limits.per_job, remaining_work, limits.working_memory_bytes,
                actual_publication_peak, kMaximumPacketBytes - base_packet_bytes);
            result.telemetry.candidate_pairs += relationships.telemetry.candidate_pairs;
            result.telemetry.algebraic_fallback_calls +=
                relationships.telemetry.algebraic_fallback_calls;
            result.telemetry.peak_working_memory_bytes =
                std::max(result.telemetry.peak_working_memory_bytes,
                         relationships.telemetry.peak_working_memory_bytes);
            if (relationships.error ==
                analytic_relationship_detail::EvaluationError::resource_limit_exceeded)
                return MergeError::resource;
            if (relationships.error == analytic_relationship_detail::EvaluationError::solver_failed)
                return MergeError::solver;
            if (relationships.error != analytic_relationship_detail::EvaluationError::none)
                return MergeError::invalid;
            if (!budget.charge(relationships.telemetry.work_units))
                return MergeError::resource;
            merged.relationship_results = std::move(relationships.results);
            merged.relationship_pairs = std::move(relationships.pairs);
        }

        if (!validate_merged_batch(merged, request))
            return MergeError::invalid;

        // Release all per-job records and remaps before the single batch encode.
        std::vector<AnalyticResultPacketRecords>().swap(jobs);
        std::vector<std::uint32_t>().swap(source_map);
        std::vector<std::uint32_t>().swap(set_map);
        std::vector<std::uint64_t>().swap(source_offsets);
        std::vector<std::uint64_t>().swap(set_offsets);

        const std::uint64_t merged_bytes = result_packet_records_logical_capacity_bytes(
            merged, totals.sources, totals.source_sets, totals.memberships);
        std::uint64_t retained_encode_bytes = merged_bytes;
        if (!checked_multiply(result.jobs.size(), kBatchTelemetrySlotBytes, telemetry_bytes) ||
            !checked_add(retained_encode_bytes, telemetry_bytes, retained_encode_bytes))
            return MergeError::resource;
        std::uint64_t packet_bytes = 0;
        std::uint64_t encoding_peak = 0;
        if (!admit_packet_encoding_memory(merged, retained_encode_bytes,
                                          limits.working_memory_bytes, packet_bytes,
                                          encoding_peak) ||
            packet_bytes > kMaximumPacketBytes)
            return MergeError::resource;
        result.telemetry.peak_working_memory_bytes =
            std::max(result.telemetry.peak_working_memory_bytes, encoding_peak);
        AnalyticResultPacketEncodeResult encoded =
            analytic_result_detail::encode_canonical_records_unchecked(merged);
        if (encoded.error != AnalyticResultPacketLayoutError::none || !encoded.value ||
            encoded.value->size() != packet_bytes)
            return MergeError::encoding;
        AnalyticFilteredBatchPacket packet;
        packet.records = std::move(merged);
        packet.bytes = std::move(*encoded.value);
        result.telemetry.source_memberships = totals.memberships;
        result.telemetry.emitted_packet_bytes = packet.bytes.size();
        result.packet = std::move(packet);
        return MergeError::none;
    }
    catch (const std::bad_alloc&)
    {
        return MergeError::resource;
    }
}

} // namespace

AnalyticFilteredBatchResult
build_analytic_filtered_batch(const AnalyticRequestPacketRecords& records,
                              const AnalyticFilteredBatchLimits& limits)
{
    AnalyticFilteredBatchResult result;
    if (!analytic_solver_limits_within_hard_ceilings(limits.per_job) ||
        limits.assembly_work_units > kAnalyticSolverHardLimits.predicate_calls ||
        limits.working_memory_bytes > kAnalyticSolverHardLimits.working_memory_bytes)
    {
        result.error = AnalyticFilteredBatchError::invalid_argument;
        return result;
    }
    std::uint64_t validation_work = 0;
    std::uint64_t validation_bytes = 0;
    if (!request_validation_requirements(records, validation_work, validation_bytes) ||
        validation_work > limits.assembly_work_units ||
        validation_bytes > limits.working_memory_bytes)
    {
        result.error = AnalyticFilteredBatchError::resource_limit_exceeded;
        return result;
    }
    try
    {
        if (validate_analytic_request_packet_records(records) != AnalyticRequestPacketError::none)
        {
            result.error = AnalyticFilteredBatchError::invalid_argument;
            return result;
        }
    }
    catch (const std::bad_alloc&)
    {
        result.error = AnalyticFilteredBatchError::resource_limit_exceeded;
        return result;
    }
    result.telemetry.peak_working_memory_bytes = validation_bytes;
    result.telemetry.merge_work_units = validation_work;
    try
    {
        std::vector<AnalyticResultPacketRecords> job_records;
        std::uint64_t retained_records =
            records.jobs.size() * (kBatchJobSlotBytes + kMinimumJobClosureBytes);
        if (retained_records > limits.working_memory_bytes)
        {
            result.error = AnalyticFilteredBatchError::resource_limit_exceeded;
            return result;
        }
        job_records.reserve(records.jobs.size());
        result.jobs.reserve(records.jobs.size());
        for (std::uint32_t job_index = 0; job_index < records.jobs.size(); ++job_index)
        {
            const std::uint64_t processing_base = retained_records;
            const std::uint64_t job_id = records.jobs[job_index].job_id;
            AnalyticFilteredBatchJobTelemetry job_telemetry;
            job_telemetry.job_id = job_id;
            AnalyticResultPacketRecords published;
            std::uint64_t published_bytes = 0;
            std::uint64_t job_peak = 0;

            AnalyticSolverLimits lower_limits;
            bool lower_batch_constrained = false;
            const LimitAvailability lower_availability = remaining_limits(
                limits.per_job, 0, 0, retained_records, limits.working_memory_bytes, lower_limits,
                lower_batch_constrained);
            if (lower_availability == LimitAvailability::batch_resource)
            {
                result.error = AnalyticFilteredBatchError::resource_limit_exceeded;
                return result;
            }
            AnalyticFilteredLoweringResult lowered;
            if (lower_availability == LimitAvailability::job_resource)
                lowered.error = AnalyticFilteredLoweringError::resource_limit_exceeded;
            else
                lowered = lower_analytic_job_to_filtered_curves(records, job_index, lower_limits);
            job_telemetry.lowering_work_units = lowered.telemetry.work_units;
            job_peak = lowered.telemetry.peak_working_memory_bytes;
            result.telemetry.algebraic_fallback_calls += lowered.telemetry.algebraic_fallback_calls;
            if (lowered.telemetry.algebraic_fallback_calls >
                std::numeric_limits<std::uint32_t>::max())
            {
                result.error = AnalyticFilteredBatchError::internal_error;
                return result;
            }
            job_telemetry.algebraic_fallback_calls =
                static_cast<std::uint32_t>(lowered.telemetry.algebraic_fallback_calls);
            if (lowered.error == AnalyticFilteredLoweringError::none && !lowered.value)
            {
                result.error = AnalyticFilteredBatchError::internal_error;
                return result;
            }
            if (lowered.error != AnalyticFilteredLoweringError::none)
            {
                if (lowered.error == AnalyticFilteredLoweringError::resource_limit_exceeded &&
                    lower_batch_constrained &&
                    lowered.telemetry.required_working_memory_bytes >
                        lower_limits.working_memory_bytes)
                {
                    result.error = AnalyticFilteredBatchError::resource_limit_exceeded;
                    return result;
                }
                job_telemetry.diagnostic_code = lowering_diagnostic(lowered.error);
                published = failed_job(job_id, job_telemetry.diagnostic_code);
            }
            else
            {
                const std::uint64_t geometry_bytes = lowered.telemetry.retained_geometry_bytes;
                std::uint64_t live = 0;
                if (!checked_add(retained_records, geometry_bytes, live))
                {
                    result.error = AnalyticFilteredBatchError::resource_limit_exceeded;
                    return result;
                }
                AnalyticSolverLimits broad_limits;
                bool broad_batch_constrained = false;
                const LimitAvailability broad_availability = remaining_limits(
                    limits.per_job, lowered.telemetry.work_units, geometry_bytes, live,
                    limits.working_memory_bytes, broad_limits, broad_batch_constrained);
                if (broad_availability == LimitAvailability::batch_resource)
                {
                    result.error = AnalyticFilteredBatchError::resource_limit_exceeded;
                    return result;
                }
                if (broad_availability == LimitAvailability::job_resource)
                {
                    job_telemetry.diagnostic_code = 65'547;
                    published = failed_job(job_id, job_telemetry.diagnostic_code);
                }
                else
                {
                    AnalyticBroadPhaseResult broad =
                        build_analytic_curve_candidates(lowered.value->bounds, broad_limits);
                    job_telemetry.broad_phase_work_units = broad.telemetry.work_units;
                    job_telemetry.candidate_pairs = broad.telemetry.candidate_pairs;
                    result.telemetry.broad_examined_pairs += broad.telemetry.examined_curve_pairs;
                    result.telemetry.candidate_pairs += broad.telemetry.candidate_pairs;
                    result.telemetry.algebraic_fallback_calls +=
                        broad.telemetry.algebraic_fallback_calls;
                    if (broad.telemetry.algebraic_fallback_calls >
                        std::numeric_limits<std::uint32_t>::max() -
                            job_telemetry.algebraic_fallback_calls)
                    {
                        result.error = AnalyticFilteredBatchError::internal_error;
                        return result;
                    }
                    job_telemetry.algebraic_fallback_calls +=
                        static_cast<std::uint32_t>(broad.telemetry.algebraic_fallback_calls);
                    job_peak = std::max(job_peak,
                                        geometry_bytes + broad.telemetry.peak_working_memory_bytes);
                    if (broad.error != AnalyticBroadPhaseError::none)
                    {
                        if (broad.error != AnalyticBroadPhaseError::resource_limit_exceeded)
                        {
                            result.error = AnalyticFilteredBatchError::internal_error;
                            return result;
                        }
                        if (broad_batch_constrained &&
                            broad.telemetry.required_working_memory_bytes >
                                broad_limits.working_memory_bytes)
                        {
                            result.error = AnalyticFilteredBatchError::resource_limit_exceeded;
                            return result;
                        }
                        job_telemetry.diagnostic_code = 65'547;
                        published = failed_job(job_id, job_telemetry.diagnostic_code);
                    }
                    else
                    {
                        std::uint64_t packet_live = 0;
                        if (!checked_add(live, broad.telemetry.retained_pair_bytes, packet_live))
                        {
                            result.error = AnalyticFilteredBatchError::resource_limit_exceeded;
                            return result;
                        }
                        AnalyticSolverLimits packet_limits;
                        const std::uint64_t used_work =
                            lowered.telemetry.work_units + broad.telemetry.work_units;
                        const std::uint64_t job_packet_live =
                            geometry_bytes + broad.telemetry.retained_pair_bytes;
                        bool packet_batch_constrained = false;
                        const LimitAvailability packet_availability = remaining_limits(
                            limits.per_job, used_work, job_packet_live, packet_live,
                            limits.working_memory_bytes, packet_limits, packet_batch_constrained);
                        if (packet_availability == LimitAvailability::batch_resource)
                        {
                            result.error = AnalyticFilteredBatchError::resource_limit_exceeded;
                            return result;
                        }
                        if (packet_availability == LimitAvailability::job_resource)
                        {
                            job_telemetry.diagnostic_code = 65'547;
                            published = failed_job(job_id, job_telemetry.diagnostic_code);
                        }
                        else
                        {
                            AnalyticFilteredJobRecordsResult packet =
                                build_analytic_filtered_job_records(
                                    records, job_index, *lowered.value, broad.pairs, packet_limits);
                            job_telemetry.packet_work_units = packet.telemetry.predicate_calls;
                            job_telemetry.capsule_coalescences =
                                packet.telemetry.capsule_coalescences;
                            job_telemetry.maximum_capsule_adjustment_nm =
                                packet.telemetry.maximum_capsule_adjustment_nm;
                            result.telemetry.algebraic_fallback_calls +=
                                packet.telemetry.algebraic_fallback_calls;
                            if (packet.telemetry.algebraic_fallback_calls >
                                std::numeric_limits<std::uint32_t>::max() -
                                    job_telemetry.algebraic_fallback_calls)
                            {
                                result.error = AnalyticFilteredBatchError::internal_error;
                                return result;
                            }
                            job_telemetry.algebraic_fallback_calls += static_cast<std::uint32_t>(
                                packet.telemetry.algebraic_fallback_calls);
                            job_peak = std::max(
                                job_peak, geometry_bytes + broad.telemetry.retained_pair_bytes +
                                              packet.telemetry.peak_working_memory_bytes);
                            if (packet.error != AnalyticFilteredPacketError::none ||
                                !packet.records)
                            {
                                if (packet.error ==
                                    AnalyticFilteredPacketError::resource_limit_exceeded)
                                {
                                    if (packet_batch_constrained &&
                                        packet.telemetry.required_working_memory_bytes >
                                            packet_limits.working_memory_bytes)
                                    {
                                        result.error =
                                            AnalyticFilteredBatchError::resource_limit_exceeded;
                                        return result;
                                    }
                                    job_telemetry.diagnostic_code = 65'547;
                                    published = failed_job(job_id, job_telemetry.diagnostic_code);
                                }
                                else
                                {
                                    result.error =
                                        packet.error == AnalyticFilteredPacketError::encoding_failed
                                            ? AnalyticFilteredBatchError::encoding_failed
                                            : AnalyticFilteredBatchError::internal_error;
                                    return result;
                                }
                            }
                            else
                            {
                                if (packet_batch_constrained &&
                                    packet.telemetry.required_working_memory_bytes >
                                        packet_limits.working_memory_bytes &&
                                    is_resource_failed_job(*packet.records))
                                {
                                    result.error =
                                        AnalyticFilteredBatchError::resource_limit_exceeded;
                                    return result;
                                }
                                published_bytes = packet.telemetry.retained_records_bytes;
                                published = std::move(*packet.records);
                            }
                        }
                    }
                }
            }

            const std::uint64_t bytes = published_bytes == 0
                                            ? result_packet_records_logical_bytes(published)
                                            : published_bytes;
            std::uint64_t processing_peak = 0;
            if (!checked_add(processing_base, job_peak, processing_peak) ||
                processing_peak > limits.working_memory_bytes)
            {
                result.error = AnalyticFilteredBatchError::resource_limit_exceeded;
                return result;
            }
            result.telemetry.peak_working_memory_bytes =
                std::max(result.telemetry.peak_working_memory_bytes, processing_peak);
            const std::uint64_t additional_bytes =
                bytes > kMinimumJobClosureBytes ? bytes - kMinimumJobClosureBytes : 0;
            if (bytes == std::numeric_limits<std::uint64_t>::max() ||
                !checked_add(retained_records, additional_bytes, retained_records) ||
                retained_records > limits.working_memory_bytes)
            {
                result.error = AnalyticFilteredBatchError::resource_limit_exceeded;
                return result;
            }
            job_telemetry.peak_working_memory_bytes = job_peak;
            job_telemetry.emitted_record_bytes = result_packet_records_logical_bytes(published);
            result.telemetry.peak_working_memory_bytes =
                std::max(result.telemetry.peak_working_memory_bytes, retained_records);
            const bool failed = published.job_results.front().status != 0;
            if (failed && job_telemetry.diagnostic_code == 0 && !published.diagnostics.empty())
                job_telemetry.diagnostic_code = published.diagnostics.front().code;
            result.telemetry.jobs_visited++;
            result.telemetry.jobs_failed += failed ? 1 : 0;
            result.telemetry.jobs_succeeded += failed ? 0 : 1;
            result.telemetry.capsule_coalescences += job_telemetry.capsule_coalescences;
            result.telemetry.maximum_capsule_adjustment_nm =
                std::max(result.telemetry.maximum_capsule_adjustment_nm,
                         job_telemetry.maximum_capsule_adjustment_nm);
            if (!add_telemetry(job_telemetry.lowering_work_units,
                               result.telemetry.lowering_work_units) ||
                !add_telemetry(job_telemetry.broad_phase_work_units,
                               result.telemetry.broad_phase_work_units) ||
                !add_telemetry(job_telemetry.packet_work_units, result.telemetry.packet_work_units))
            {
                result.error = AnalyticFilteredBatchError::resource_limit_exceeded;
                return result;
            }
            job_records.push_back(std::move(published));
            result.jobs.push_back(job_telemetry);
        }
        result.telemetry.retained_job_records_bytes = retained_records;
        const MergeError merge = merge_jobs(job_records, records, limits, validation_work, result);
        if (merge == MergeError::resource)
            result.error = AnalyticFilteredBatchError::resource_limit_exceeded;
        else if (merge == MergeError::solver)
            result.error = AnalyticFilteredBatchError::solver_failed;
        else if (merge == MergeError::invalid)
            result.error = AnalyticFilteredBatchError::internal_error;
        else if (merge == MergeError::encoding)
            result.error = AnalyticFilteredBatchError::encoding_failed;
    }
    catch (const std::bad_alloc&)
    {
        result.error = AnalyticFilteredBatchError::resource_limit_exceeded;
    }
    if (result.error != AnalyticFilteredBatchError::none)
        result.packet.reset();
    return result;
}

} // namespace geometer

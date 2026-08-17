#include "geometer/analytic_filtered_batch.h"
#include "geometer/analytic_result_packet_records.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

namespace
{

using namespace geometer;

void require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

AnalyticRequestPacketRecords two_disks()
{
    AnalyticRequestPacketRecords records;
    records.jobs = {{10, 0, 1}, {20, 1, 1}};
    records.stages = {{100, 1, 0, 1}, {200, 1, 1, 1}};
    records.operands = {{1000, 2, 0}, {2000, 3, 0}};
    records.disks = {{5000, 0, 0, 1000}};
    records.annuli = {{6000, 100'000, 0, 500, 1500}};
    return records;
}

AnalyticRequestPacketRecords plain_disks(std::uint32_t count)
{
    AnalyticRequestPacketRecords records;
    for (std::uint32_t index = 0; index < count; ++index)
    {
        records.jobs.push_back({10 + index, index, 1});
        records.stages.push_back({100 + index, 1, index, 1});
        records.operands.push_back({1000 + index, 2, index});
        records.disks.push_back(
            {5000 + index, static_cast<std::int64_t>(index) * 100'000, 0, 1000});
    }
    return records;
}

AnalyticRequestPacketRecords unsupported_then_disk()
{
    AnalyticRequestPacketRecords records;
    records.jobs = {{10, 0, 1}, {20, 1, 1}};
    records.stages = {{100, 1, 0, 1}, {200, 1, 1, 1}};
    records.operands = {{1000, 5, 0}, {2000, 2, 0}};
    records.planar_regions = {};
    records.vertices = {{7000, 0, 0}, {7001, 1000, 0}};
    records.segments = {{7100, 7200, 1, 0, false, 0, 0}};
    records.rings = {{7300, 0, 2, 0, 1, 1}};
    records.swept_paths = {{7400, 0, 100}};
    records.disks = {{8000, 100'000, 0, 1000}};
    return records;
}

AnalyticRequestPacketRecords empty_jobs(std::uint32_t count)
{
    AnalyticRequestPacketRecords records;
    records.jobs.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index)
        records.jobs.push_back({1000 + index, 0, 0});
    return records;
}

void test_empty_batch()
{
    AnalyticRequestPacketRecords records;
    const auto result = build_analytic_filtered_batch(records);
    require(result.error == AnalyticFilteredBatchError::none && result.packet,
            "empty batch failed");
    require(result.packet->records.job_results.empty(), "empty batch emitted jobs");
    require(!result.packet->bytes.empty(), "empty batch did not encode");

    const auto empty = build_analytic_filtered_batch(empty_jobs(1));
    require(empty.error == AnalyticFilteredBatchError::none && empty.packet &&
                empty.packet->records.job_results.size() == 1,
            "empty job failed");
    const auto& job = empty.packet->records.job_results.front();
    require(job.diagnostic_begin == 0 && job.diagnostic_count == 0 &&
                job.result_region_begin == 0 && job.result_region_count == 0 &&
                job.operand_event_begin == 0 && job.operand_event_count == 0,
            "empty job ranges were not canonically zeroed");
}

void test_two_successful_jobs()
{
    const auto records = two_disks();
    require(validate_analytic_request_packet_records(records) == AnalyticRequestPacketError::none,
            "two-disk request invalid");
    const auto first = build_analytic_filtered_batch(records);
    const auto second = build_analytic_filtered_batch(records);
    require(first.error == AnalyticFilteredBatchError::none && first.packet,
            "two-disk batch failed");
    require(second.error == AnalyticFilteredBatchError::none && second.packet,
            "repeat two-disk batch failed");
    require(first.packet->bytes == second.packet->bytes, "batch output is not deterministic");
    const auto& output = first.packet->records;
    require(validate_analytic_result_packet_records(output) ==
                AnalyticResultPacketLayoutError::none,
            "merged batch records invalid");
    require(output.job_results.size() == 2 && output.job_results[0].job_id == 10 &&
                output.job_results[1].job_id == 20,
            "job-major merge order wrong");
    require(output.job_results[0].status == 0 && output.job_results[1].status == 0,
            "successful jobs were not preserved");
    require(output.job_results[0].result_region_begin == 0 &&
                output.job_results[0].result_region_count == 1 &&
                output.job_results[0].operand_event_begin == 0 &&
                output.job_results[0].operand_event_count == 1 &&
                output.job_results[1].result_region_begin == 1 &&
                output.job_results[1].result_region_count == 1 &&
                output.job_results[1].operand_event_begin == 1 &&
                output.job_results[1].operand_event_count == 1,
            "second-job region or event ranges were not rebased");
    require(output.rings.size() == 3 && output.regions.size() == 2 &&
                output.operand_events.size() == 2,
            "disk/annulus merged topology counts drifted");
    for (std::uint32_t index = 0; index < output.rings.size(); ++index)
        require(output.rings[index].id == index + 1,
                "job-local ring ID collisions were not globally remapped");
    for (std::uint32_t index = 0; index < output.regions.size(); ++index)
        require(output.regions[index].id == index + 1,
                "job-local region ID collisions were not globally remapped");

    const std::uint32_t annulus_outer = output.regions[1].outer_ring;
    require(annulus_outer >= 1 && annulus_outer < output.rings.size() &&
                output.rings[annulus_outer].parent_ring ==
                    std::numeric_limits<std::uint32_t>::max() &&
                output.rings[annulus_outer].depth == 0,
            "annulus outer-ring hierarchy was not preserved");
    bool found_annulus_hole = false;
    for (std::uint32_t index = 1; index < output.rings.size(); ++index)
        found_annulus_hole =
            found_annulus_hole ||
            (output.rings[index].parent_ring == annulus_outer && output.rings[index].depth == 1);
    require(found_annulus_hole, "annulus hole parent/depth was not preserved");

    const auto& disk_ring = output.rings.front();
    require(disk_ring.fragment_reference_begin == 0 && disk_ring.fragment_reference_count != 0,
            "first-job fragment range drifted");
    std::uint32_t first_job_fragment_count = 0;
    std::uint32_t first_job_vertex_count = 0;
    for (std::uint32_t offset = 0; offset < disk_ring.fragment_reference_count; ++offset)
    {
        const std::uint32_t fragment_index =
            output.fragment_references[disk_ring.fragment_reference_begin + offset];
        first_job_fragment_count = std::max(first_job_fragment_count, fragment_index + 1);
        first_job_vertex_count =
            std::max(first_job_vertex_count, std::max(output.fragments[fragment_index].start_vertex,
                                                      output.fragments[fragment_index].end_vertex) +
                                                 1);
    }
    for (std::uint32_t ring_index = 1; ring_index < output.rings.size(); ++ring_index)
    {
        const auto& ring = output.rings[ring_index];
        for (std::uint32_t offset = 0; offset < ring.fragment_reference_count; ++offset)
        {
            const std::uint32_t fragment_index =
                output.fragment_references[ring.fragment_reference_begin + offset];
            require(fragment_index >= first_job_fragment_count,
                    "second-job ring retained a local fragment index");
            require(output.fragments[fragment_index].start_vertex >= first_job_vertex_count &&
                        output.fragments[fragment_index].end_vertex >= first_job_vertex_count,
                    "second-job fragment retained a local vertex index");
        }
    }

    require(output.operand_events[0].operand_id == 1000 &&
                output.operand_events[1].operand_id == 2000 &&
                output.operand_events[1].result_reference_begin >=
                    output.operand_events[0].result_reference_begin +
                        output.operand_events[0].result_reference_count,
            "second-job event references were not appended job-major");
    const auto& second_event = output.operand_events[1];
    for (std::uint32_t offset = 0; offset < second_event.result_reference_count; ++offset)
    {
        const std::uint64_t reference =
            output.ring_region_references[second_event.result_reference_begin + offset];
        const std::uint32_t kind = static_cast<std::uint32_t>(reference >> 32U);
        const std::uint32_t index = static_cast<std::uint32_t>(reference);
        require((kind == 1 && index >= 1) || (kind == 2 && index >= 1),
                "second-job tagged result reference retained a local index");
    }
    require(first.telemetry.jobs_succeeded == 2 && first.telemetry.jobs_failed == 0 &&
                first.telemetry.algebraic_fallback_calls == 0,
            "batch telemetry wrong");
}

void test_job_local_failure_isolated()
{
    const auto records = unsupported_then_disk();
    require(validate_analytic_request_packet_records(records) == AnalyticRequestPacketError::none,
            "mixed request invalid");
    const auto result = build_analytic_filtered_batch(records);
    require(result.error == AnalyticFilteredBatchError::none && result.packet,
            "mixed batch failed outward");
    const auto& output = result.packet->records;
    require(output.job_results.size() == 2 && output.job_results[0].status == 1 &&
                output.job_results[1].status == 0,
            "job-local failure isolation failed");
    require(output.diagnostics.size() == 1 && output.diagnostics[0].code == 65'541,
            "unsupported geometry diagnostic wrong");
    require(output.job_results[0].result_region_begin == 0 &&
                output.job_results[0].result_region_count == 0 &&
                output.job_results[0].operand_event_begin == 0 &&
                output.job_results[0].operand_event_count == 0,
            "failed job ranges were not canonically zeroed");
    require(validate_analytic_result_packet_records(output) ==
                AnalyticResultPacketLayoutError::none,
            "mixed batch records invalid");
    require(result.telemetry.jobs_failed == 1 && result.telemetry.jobs_succeeded == 1,
            "mixed batch telemetry wrong");
}

void test_per_job_memory_is_independent_of_prior_outputs()
{
    const auto single = plain_disks(1);
    AnalyticFilteredBatchLimits limits;
    std::uint64_t low = 0;
    std::uint64_t high = limits.per_job.working_memory_bytes;
    while (low < high)
    {
        const std::uint64_t middle = low + (high - low) / 2;
        limits.per_job.working_memory_bytes = middle;
        const auto candidate = build_analytic_filtered_batch(single, limits);
        if (candidate.error == AnalyticFilteredBatchError::none &&
            candidate.telemetry.jobs_succeeded == 1)
            high = middle;
        else
            low = middle + 1;
    }
    limits.per_job.working_memory_bytes = low;
    const auto result = build_analytic_filtered_batch(plain_disks(2), limits);
    require(result.error == AnalyticFilteredBatchError::none && result.packet &&
                result.telemetry.jobs_succeeded == 2,
            "prior batch outputs reduced a later per-job memory allowance");
    require(low != 0, "per-job memory threshold unexpectedly zero");
    --limits.per_job.working_memory_bytes;
    const auto short_result = build_analytic_filtered_batch(single, limits);
    require(short_result.error == AnalyticFilteredBatchError::none && short_result.packet &&
                short_result.telemetry.jobs_failed == 1 &&
                short_result.packet->records.diagnostics.size() == 1 &&
                short_result.packet->records.diagnostics.front().code == 65'547,
            "one-byte-short per-job memory was not isolated as a job failure");
}

void test_per_job_nonmemory_limits_remain_job_local()
{
    const auto records = plain_disks(1);
    for (const bool work_limit : {true, false})
    {
        AnalyticFilteredBatchLimits limits;
        if (work_limit)
            limits.per_job.predicate_calls = 0;
        else
            limits.per_job.boundary_occurrences = 1;
        const auto result = build_analytic_filtered_batch(records, limits);
        require(result.error == AnalyticFilteredBatchError::none && result.packet &&
                    result.telemetry.jobs_failed == 1 &&
                    result.packet->records.diagnostics.size() == 1 &&
                    result.packet->records.diagnostics.front().code == 65'547,
                "per-job work/count exhaustion was misclassified as aggregate memory");
    }
}

void test_relationships_remain_gated()
{
    auto records = two_disks();
    records.relationship_queries = {{9000, 10, 20}};
    require(validate_analytic_request_packet_records(records) == AnalyticRequestPacketError::none,
            "relationship request invalid");
    const auto result = build_analytic_filtered_batch(records);
    require(result.error == AnalyticFilteredBatchError::relationships_not_implemented &&
                !result.packet,
            "query-free batch accepted relationships");
}

void test_merge_work_boundary()
{
    const auto records = two_disks();
    const auto baseline = build_analytic_filtered_batch(records);
    require(baseline.error == AnalyticFilteredBatchError::none && baseline.packet,
            "work baseline failed");
    AnalyticFilteredBatchLimits limits;
    limits.assembly_work_units = baseline.telemetry.merge_work_units;
    const auto exact = build_analytic_filtered_batch(records, limits);
    require(exact.error == AnalyticFilteredBatchError::none, "exact merge work failed");
    require(limits.assembly_work_units != 0, "merge work unexpectedly zero");
    --limits.assembly_work_units;
    const auto short_result = build_analytic_filtered_batch(records, limits);
    require(short_result.error == AnalyticFilteredBatchError::resource_limit_exceeded &&
                !short_result.packet,
            "one-short merge work did not fail closed");
}

void test_memory_boundary_and_many_job_scaling()
{
    const auto records = two_disks();
    const auto baseline = build_analytic_filtered_batch(records);
    require(baseline.error == AnalyticFilteredBatchError::none && baseline.packet,
            "memory baseline failed");
    AnalyticFilteredBatchLimits limits;
    std::uint64_t low = 0;
    std::uint64_t high = limits.working_memory_bytes;
    while (low < high)
    {
        const std::uint64_t middle = low + (high - low) / 2;
        limits.working_memory_bytes = middle;
        const auto candidate = build_analytic_filtered_batch(records, limits);
        if (candidate.error == AnalyticFilteredBatchError::none && candidate.packet &&
            candidate.telemetry.jobs_succeeded == baseline.telemetry.jobs_succeeded &&
            candidate.packet->bytes == baseline.packet->bytes)
            high = middle;
        else
            low = middle + 1;
    }
    limits.working_memory_bytes = low;
    const auto exact = build_analytic_filtered_batch(records, limits);
    require(exact.error == AnalyticFilteredBatchError::none && exact.packet &&
                exact.telemetry.jobs_succeeded == baseline.telemetry.jobs_succeeded &&
                exact.packet->bytes == baseline.packet->bytes,
            "exact batch memory boundary failed");
    require(limits.working_memory_bytes != 0, "batch memory unexpectedly zero");
    --limits.working_memory_bytes;
    const auto short_result = build_analytic_filtered_batch(records, limits);
    require(short_result.error == AnalyticFilteredBatchError::resource_limit_exceeded &&
                !short_result.packet,
            "one-short batch memory did not fail closed");

    const auto small = build_analytic_filtered_batch(empty_jobs(1024));
    const auto large = build_analytic_filtered_batch(empty_jobs(2048));
    require(small.error == AnalyticFilteredBatchError::none && small.packet &&
                large.error == AnalyticFilteredBatchError::none && large.packet,
            "many-empty-job scaling fixture failed");
    require(large.telemetry.merge_work_units < small.telemetry.merge_work_units * 3 &&
                large.telemetry.peak_working_memory_bytes <
                    small.telemetry.peak_working_memory_bytes * 3 &&
                large.packet->bytes.size() < small.packet->bytes.size() * 3,
            "many-job batch scaling exceeded linear envelope");

    const auto nonempty_small = build_analytic_filtered_batch(plain_disks(16));
    const auto nonempty_large = build_analytic_filtered_batch(plain_disks(32));
    require(nonempty_small.error == AnalyticFilteredBatchError::none && nonempty_small.packet &&
                nonempty_large.error == AnalyticFilteredBatchError::none && nonempty_large.packet,
            "nonempty batch scaling fixture failed");
    const std::uint64_t small_work = nonempty_small.telemetry.lowering_work_units +
                                     nonempty_small.telemetry.broad_phase_work_units +
                                     nonempty_small.telemetry.packet_work_units +
                                     nonempty_small.telemetry.merge_work_units;
    const std::uint64_t large_work = nonempty_large.telemetry.lowering_work_units +
                                     nonempty_large.telemetry.broad_phase_work_units +
                                     nonempty_large.telemetry.packet_work_units +
                                     nonempty_large.telemetry.merge_work_units;
    require(large_work < small_work * 3 &&
                nonempty_large.telemetry.peak_working_memory_bytes <
                    nonempty_small.telemetry.peak_working_memory_bytes * 3 &&
                nonempty_large.packet->bytes.size() < nonempty_small.packet->bytes.size() * 3,
            "nonempty batch scaling exceeded linear envelope");
}

void append_u64(std::vector<std::uint8_t>& output, std::uint64_t value)
{
    for (std::uint32_t shift = 0; shift < 64; shift += 8)
        output.push_back(static_cast<std::uint8_t>(value >> shift));
}

std::string parity_vector()
{
    const auto result = build_analytic_filtered_batch(two_disks());
    require(result.error == AnalyticFilteredBatchError::none && result.packet,
            "parity batch failed");
    std::vector<std::uint8_t> bytes = result.packet->bytes;
    for (const std::uint64_t value :
         {result.telemetry.jobs_visited, result.telemetry.jobs_succeeded,
          result.telemetry.jobs_failed, result.telemetry.lowering_work_units,
          result.telemetry.broad_phase_work_units, result.telemetry.packet_work_units,
          result.telemetry.broad_examined_pairs, result.telemetry.candidate_pairs,
          result.telemetry.merge_work_units, result.telemetry.source_memberships,
          result.telemetry.sequence_table_probes, result.telemetry.retained_job_records_bytes,
          result.telemetry.emitted_packet_bytes, result.telemetry.peak_working_memory_bytes,
          result.telemetry.algebraic_fallback_calls})
        append_u64(bytes, value);
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (const std::uint8_t value : bytes)
        output << std::setw(2) << static_cast<unsigned int>(value);
    return output.str();
}

} // namespace

int main(int argc, char** argv)
{
    test_empty_batch();
    test_two_successful_jobs();
    test_job_local_failure_isolated();
    test_relationships_remain_gated();
    test_per_job_memory_is_independent_of_prior_outputs();
    test_per_job_nonmemory_limits_remain_job_local();
    test_merge_work_boundary();
    test_memory_boundary_and_many_job_scaling();
    if (argc == 2 && std::string(argv[1]) == "--emit-parity")
        std::cout << "ANALYTIC_FILTERED_BATCH_VECTOR=" << parity_vector() << '\n';
    return 0;
}

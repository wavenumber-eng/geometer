#include "analytic_filtered_packet_sequences.h"
#include "geometer/analytic_curve_broad_phase.h"
#include "geometer/analytic_filtered_lowering.h"
#include "geometer/analytic_filtered_packet.h"
#include "geometer/analytic_result_packet_records.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{
using namespace geometer;

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct SegmentSpec
{
    std::uint64_t id = 0;
    std::uint64_t curve_id = 0;
};

AnalyticRequestPacketRecords square_records()
{
    AnalyticRequestPacketRecords records;
    records.jobs = {{10, 0, 1}};
    records.stages = {{100, 1, 0, 1}};
    records.operands = {{1000, 1, 0}};
    records.planar_regions = {{500, 0, 0, 0}};
    records.vertices = {{6000, 0, 0}, {6001, 1000, 0}, {6002, 1000, 1000}, {6003, 0, 1000}};
    records.segments = {{7000, 8000, 1, 0, false, 0, 0},
                        {7001, 8001, 1, 0, false, 0, 0},
                        {7002, 8002, 1, 0, false, 0, 0},
                        {7003, 8003, 1, 0, false, 0, 0}};
    records.rings = {{9000, 0, 4, 0, 4, 0}};
    return records;
}

AnalyticRequestPacketRecords disk_records()
{
    AnalyticRequestPacketRecords records;
    records.jobs = {{11, 0, 1}};
    records.stages = {{101, 1, 0, 1}};
    records.operands = {{1001, 2, 0}};
    records.disks = {{5001, 1200, -300, 1000}};
    return records;
}

AnalyticRequestPacketRecords overlapping_disks_records()
{
    AnalyticRequestPacketRecords records;
    records.jobs = {{12, 0, 1}};
    records.stages = {{102, 1, 0, 2}};
    records.operands = {{1001, 2, 0}, {1002, 2, 1}};
    records.disks = {{5001, 0, 0, 1000}, {5002, 1201, 0, 1000}};
    return records;
}

AnalyticRequestPacketRecords difference_records()
{
    AnalyticRequestPacketRecords records;
    records.jobs = {{13, 0, 2}};
    records.stages = {{103, 1, 0, 1}, {104, 2, 1, 1}};
    records.operands = {{1001, 2, 0}, {1002, 2, 1}};
    records.disks = {{5001, 0, 0, 1000}, {5002, 0, 0, 400}};
    return records;
}

AnalyticFilteredJobPacketResult build(const AnalyticRequestPacketRecords& records,
                                      const AnalyticSolverLimits& limits = {})
{
    const auto lowered = lower_analytic_job_to_filtered_curves(records, 0, limits);
    require(lowered.error == AnalyticFilteredLoweringError::none && lowered.value,
            "packet fixture lowering failed");
    const auto broad = build_analytic_curve_candidates(lowered.value->bounds, limits);
    require(broad.error == AnalyticBroadPhaseError::none, "packet fixture broad phase failed");
    return build_analytic_filtered_job_packet(records, 0, *lowered.value, broad.pairs, limits);
}

struct PreparedJob
{
    AnalyticRequestPacketRecords records;
    AnalyticFilteredGeometry geometry;
    std::vector<AnalyticCurvePair> pairs;
};

PreparedJob prepare(AnalyticRequestPacketRecords records)
{
    const auto lowered = lower_analytic_job_to_filtered_curves(records, 0);
    require(lowered.error == AnalyticFilteredLoweringError::none && lowered.value,
            "prepared packet lowering failed");
    const auto broad = build_analytic_curve_candidates(lowered.value->bounds);
    require(broad.error == AnalyticBroadPhaseError::none, "prepared packet broad phase failed");
    return {std::move(records), *lowered.value, broad.pairs};
}

AnalyticFilteredJobPacketResult build(const PreparedJob& job, const AnalyticSolverLimits& limits)
{
    return build_analytic_filtered_job_packet(job.records, 0, job.geometry, job.pairs, limits);
}

bool successful(const AnalyticFilteredJobPacketResult& result)
{
    return result.error == AnalyticFilteredPacketError::none && result.standalone &&
           result.standalone->records.job_results[0].status == 0;
}

void require_success(const AnalyticFilteredJobPacketResult& result, std::uint64_t job_id)
{
    if (result.error != AnalyticFilteredPacketError::none || !result.standalone)
        std::cerr << "packet error=" << static_cast<int>(result.error)
                  << " normalization=" << static_cast<int>(result.normalization_error)
                  << " job=" << job_id << '\n';
    require(result.error == AnalyticFilteredPacketError::none && result.standalone,
            "filtered packet build failed");
    const auto& standalone = *result.standalone;
    require(standalone.digest_sha256.size() == 64 && !standalone.bytes.empty(),
            "standalone digest or bytes missing");
    if (standalone.records.job_results.size() != 1 ||
        standalone.records.job_results[0].job_id != job_id ||
        standalone.records.job_results[0].status != 0)
        std::cerr << "job drift expected=" << job_id << " got="
                  << (standalone.records.job_results.empty()
                          ? 0
                          : standalone.records.job_results[0].job_id)
                  << " status="
                  << (standalone.records.job_results.empty()
                          ? 9
                          : standalone.records.job_results[0].status)
                  << " normalization=" << static_cast<int>(result.normalization_error) << '\n';
    require(standalone.records.job_results.size() == 1 &&
                standalone.records.job_results[0].job_id == job_id &&
                standalone.records.job_results[0].status == 0,
            "standalone job record drifted");
    const auto validation = validate_analytic_result_packet_records(standalone.records);
    if (validation != AnalyticResultPacketLayoutError::none)
    {
        std::cerr << "validation=" << static_cast<int>(validation)
                  << " v=" << standalone.records.vertices.size()
                  << " f=" << standalone.records.fragments.size()
                  << " r=" << standalone.records.rings.size()
                  << " g=" << standalone.records.regions.size()
                  << " s=" << standalone.records.source_sets.size()
                  << " x=" << standalone.records.source_references.size()
                  << " e=" << standalone.records.operand_events.size() << '\n';
        for (const auto& ring : standalone.records.rings)
            std::cerr << "ring " << ring.id << " b=" << ring.fragment_reference_begin
                      << " n=" << ring.fragment_reference_count << " p=" << ring.parent_ring
                      << " d=" << ring.depth << " fl=" << ring.flags << '\n';
        for (std::uint32_t index = 0; index < standalone.records.source_sets.size(); ++index)
        {
            const auto& set = standalone.records.source_sets[index];
            std::cerr << "set " << index + 1 << " b=" << set.source_reference_index_begin
                      << " n=" << set.source_reference_index_count << ':';
            for (std::uint32_t at = 0; at < set.source_reference_index_count; ++at)
                std::cerr << ' '
                          << standalone.records
                                 .source_reference_indices[set.source_reference_index_begin + at];
            std::cerr << '\n';
        }
    }
    require(validation == AnalyticResultPacketLayoutError::none,
            "owned packet did not satisfy the independent structural validator");
    const auto decoded =
        decode_analytic_result_packet_records(standalone.bytes.data(), standalone.bytes.size());
    require(decoded.error == AnalyticResultPacketLayoutError::none && decoded.value,
            "owned packet bytes did not decode canonically");
    require(result.telemetry.algebraic_fallback_calls == 0,
            "filtered packet invoked algebraic fallback");
}

void test_authored_square()
{
    const auto result = build(square_records());
    require_success(result, 10);
    const auto& records = result.standalone->records;
    require(records.vertices.size() == 4 && records.fragments.size() == 4 &&
                records.rings.size() == 1 && records.regions.size() == 1 &&
                records.operand_events.size() == 1,
            "square packet topology counts drifted");
    require(result.maps.arrangement_vertex_to_packet_vertex.size() == 4 &&
                result.maps.boundary_to_packet_fragment.size() == 4 &&
                result.maps.ring_to_packet_ring.size() == 1 &&
                result.maps.region_to_packet_region.size() == 1,
            "square packet maps are incomplete");
    require(records.source_references.size() == 4 && records.source_sets.size() >= 1,
            "square source table lost authored segments");
}

void test_compact_disk_and_determinism()
{
    const auto first = build(disk_records());
    const auto second = build(disk_records());
    require_success(first, 11);
    require_success(second, 11);
    require(first.standalone->bytes == second.standalone->bytes &&
                first.standalone->digest_sha256 == second.standalone->digest_sha256,
            "filtered packet bytes or digest are nondeterministic");
    require(first.standalone->records.fragments.size() == 2 &&
                first.standalone->records.source_references.size() == 1 &&
                first.standalone->records.source_sets.size() == 1,
            "disk packet did not intern its repeated compact source");
}

void test_irrational_union_and_subtraction()
{
    const auto irrational = build(overlapping_disks_records());
    require_success(irrational, 12);
    require(irrational.standalone->records.regions.size() == 1 &&
                irrational.standalone->records.operand_events.size() >= 2 &&
                irrational.telemetry.algebraic_fallback_calls == 0,
            "generic irrational disk union did not publish on the filtered path");

    const auto difference = build(difference_records());
    require_success(difference, 13);
    const auto& packet = difference.standalone->records;
    require(packet.rings.size() == 2 && packet.regions.size() == 1 &&
                packet.operand_events.size() >= 2,
            "disk subtraction packet topology/outcomes drifted");
    require(std::any_of(packet.source_references.begin(), packet.source_references.end(),
                        [](const AnalyticSourceReference& source)
                        { return source.kind == AnalyticSourceKind::subtractive_operand_effect; }),
            "surviving subtraction source was not published");
}

void test_failed_normalization_packet()
{
    const auto records = disk_records();
    const auto lowered = lower_analytic_job_to_filtered_curves(records, 0);
    require(lowered.error == AnalyticFilteredLoweringError::none && lowered.value,
            "failure fixture lowering failed");
    const auto broad = build_analytic_curve_candidates(lowered.value->bounds);
    AnalyticSolverLimits limits;
    limits.predicate_calls = 1;
    const auto result =
        build_analytic_filtered_job_packet(records, 0, *lowered.value, broad.pairs, limits);
    require((result.error == AnalyticFilteredPacketError::resource_limit_exceeded) ||
                (result.error == AnalyticFilteredPacketError::none && result.standalone &&
                 result.standalone->records.job_results[0].status == 1),
            "low-limit packet did not fail closed");
}

void test_empty_success_and_early_resource_packet()
{
    AnalyticRequestPacketRecords empty;
    empty.jobs = {{14, 0, 1}};
    empty.stages = {{105, 1, 0, 0}};
    const auto empty_result = build(empty);
    require_success(empty_result, 14);
    require(empty_result.standalone->records.vertices.empty() &&
                empty_result.standalone->records.fragments.empty() &&
                empty_result.standalone->records.regions.empty() &&
                empty_result.standalone->records.operand_events.empty(),
            "zero-operand job was not a successful empty packet");

    const auto baseline = build(disk_records());
    require_success(baseline, 11);
    AnalyticSolverLimits short_limits;
    require(baseline.telemetry.reserved_packet_work_units != 0,
            "packet work reservation was not reported");
    short_limits.predicate_calls = baseline.telemetry.reserved_packet_work_units;
    const auto early = build(disk_records(), short_limits);
    require(early.error == AnalyticFilteredPacketError::none && early.standalone &&
                early.standalone->records.job_results[0].status == 1 &&
                early.standalone->records.vertices.empty() &&
                early.telemetry.normalization_work_units == 0,
            "known-impossible packet work did not fail before normalization");
}

void test_exact_resource_boundaries()
{
    const PreparedJob job = prepare(disk_records());
    AnalyticSolverLimits limits;
    std::uint64_t low = 0;
    std::uint64_t high = limits.predicate_calls;
    while (low < high)
    {
        const std::uint64_t middle = low + (high - low) / 2;
        limits.predicate_calls = middle;
        if (successful(build(job, limits)))
            high = middle;
        else
            low = middle + 1;
    }
    const std::uint64_t exact_work = low;
    limits = {};
    limits.predicate_calls = exact_work;
    const auto exact = build(job, limits);
    require(successful(exact) && exact.telemetry.predicate_calls <= exact_work,
            "exact packet work boundary did not succeed");
    limits.predicate_calls = exact_work - 1;
    const auto work_short = build(job, limits);
    require(!successful(work_short) && work_short.standalone &&
                work_short.standalone->records.vertices.empty(),
            "one-unit-short packet work did not fail without partial geometry");

    limits = {};
    low = 0;
    high = limits.working_memory_bytes;
    while (low < high)
    {
        const std::uint64_t middle = low + (high - low) / 2;
        limits.working_memory_bytes = middle;
        if (successful(build(job, limits)))
            high = middle;
        else
            low = middle + 1;
    }
    const std::uint64_t exact_memory = low;
    limits = {};
    limits.working_memory_bytes = exact_memory;
    const auto memory_exact = build(job, limits);
    require(successful(memory_exact) &&
                memory_exact.telemetry.peak_working_memory_bytes <= exact_memory,
            "exact packet memory boundary did not succeed");
    limits.working_memory_bytes = exact_memory - 1;
    const auto memory_short = build(job, limits);
    require(!successful(memory_short) && memory_short.standalone &&
                memory_short.standalone->records.vertices.empty(),
            "one-byte-short packet memory did not fail without partial geometry");
}

std::uint64_t shared_prefix_work(std::uint32_t sequence_count)
{
    using namespace geometer::analytic_packet_detail;
    std::vector<std::uint32_t> labels;
    std::vector<SequenceRange> ranges;
    constexpr std::uint32_t prefix_count = 32;
    labels.reserve(static_cast<std::size_t>(sequence_count) * (prefix_count + 1));
    ranges.reserve(sequence_count);
    for (std::uint32_t sequence = 0; sequence < sequence_count; ++sequence)
    {
        const std::uint32_t begin = static_cast<std::uint32_t>(labels.size());
        for (std::uint32_t label = 0; label < prefix_count; ++label)
            labels.push_back(label);
        labels.push_back(prefix_count + sequence);
        ranges.push_back({begin, prefix_count + 1});
    }
    AnalyticFilteredPacketTelemetry telemetry;
    WorkBudget budget{std::numeric_limits<std::uint64_t>::max(), 0, &telemetry};
    CanonicalSequences output;
    require(canonicalize_sequences(labels, ranges, false, 0,
                                   std::numeric_limits<std::uint64_t>::max(), budget, output),
            "shared-prefix sequence canonicalization failed");
    require(output.handles.size() == sequence_count && output.records.size() == sequence_count,
            "shared-prefix sequences did not remain distinct");
    return budget.used;
}

void test_shared_prefix_sequence_scaling()
{
    const std::uint64_t one = shared_prefix_work(64);
    const std::uint64_t two = shared_prefix_work(128);
    require(two < one * 3, "shared-prefix source-set work grew superlinearly");
}

template <typename Value> void append_hex(std::ostringstream& output, Value value)
{
    output << std::hex << std::setfill('0') << std::setw(sizeof(Value) * 2)
           << static_cast<std::uint64_t>(value);
}

void append_parity_result(std::ostringstream& output, const AnalyticFilteredJobPacketResult& result)
{
    require(successful(result), "packet parity fixture failed");
    append_hex(output, static_cast<std::uint64_t>(result.standalone->bytes.size()));
    for (const std::uint8_t byte : result.standalone->bytes)
        append_hex(output, byte);
    const auto append_map = [&output](const std::vector<std::uint32_t>& values)
    {
        append_hex(output, static_cast<std::uint64_t>(values.size()));
        for (const std::uint32_t value : values)
            append_hex(output, value);
    };
    append_map(result.maps.arrangement_vertex_to_packet_vertex);
    append_map(result.maps.boundary_to_packet_fragment);
    append_map(result.maps.ring_to_packet_ring);
    append_map(result.maps.region_to_packet_region);
    const auto& telemetry = result.telemetry;
    append_hex(output, telemetry.normalization_work_units);
    append_hex(output, telemetry.normalization_peak_working_memory_bytes);
    append_hex(output, telemetry.packet_work_units);
    append_hex(output, telemetry.sequence_nodes);
    append_hex(output, telemetry.sequence_table_probes);
    append_hex(output, telemetry.predicate_calls);
    append_hex(output, telemetry.peak_working_memory_bytes);
    append_hex(output, telemetry.algebraic_fallback_calls);
}

std::string parity_vector()
{
    std::ostringstream output;
    append_parity_result(output, build(square_records()));
    append_parity_result(output, build(overlapping_disks_records()));
    append_parity_result(output, build(difference_records()));
    return output.str();
}

} // namespace

int main(int argc, char** argv)
{
    test_authored_square();
    test_compact_disk_and_determinism();
    test_irrational_union_and_subtraction();
    test_failed_normalization_packet();
    test_empty_success_and_early_resource_packet();
    test_exact_resource_boundaries();
    test_shared_prefix_sequence_scaling();
    if (argc == 2 && std::string(argv[1]) == "--emit-parity")
        std::cout << "ANALYTIC_FILTERED_PACKET_VECTOR=" << parity_vector() << '\n';
    return 0;
}

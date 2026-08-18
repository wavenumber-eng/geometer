#include "geometer/analytic_filtered_batch.h"
#include "geometer/analytic_result_packet_records.h"

#include "analytic_filtered_execution_policy.h"
#include "analytic_filtered_relationships.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

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
    records.vertices = {{7000, 0, 0}, {7001, 0, 0}};
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

struct RectangleSpec
{
    std::int64_t minimum_x = 0;
    std::int64_t minimum_y = 0;
    std::int64_t maximum_x = 0;
    std::int64_t maximum_y = 0;
};

AnalyticRequestPacketRecords rectangle_jobs(const std::vector<RectangleSpec>& rectangles)
{
    AnalyticRequestPacketRecords records;
    for (std::uint32_t index = 0; index < rectangles.size(); ++index)
    {
        const std::uint64_t base = 1000 + static_cast<std::uint64_t>(index) * 100;
        records.jobs.push_back({10 + index, index, 1});
        records.stages.push_back({base, 1, index, 1});
        records.operands.push_back({base + 1, 1, index});
        records.planar_regions.push_back({base + 2, index, 0, 0});
        const std::uint32_t vertex_begin = static_cast<std::uint32_t>(records.vertices.size());
        const std::uint32_t segment_begin = static_cast<std::uint32_t>(records.segments.size());
        const auto& value = rectangles[index];
        records.vertices.push_back({base + 10, value.minimum_x, value.minimum_y});
        records.vertices.push_back({base + 11, value.maximum_x, value.minimum_y});
        records.vertices.push_back({base + 12, value.maximum_x, value.maximum_y});
        records.vertices.push_back({base + 13, value.minimum_x, value.maximum_y});
        for (std::uint32_t edge = 0; edge < 4; ++edge)
            records.segments.push_back({base + 20 + edge, base + 30 + edge, 1, 0, false, 0, 0});
        records.rings.push_back({base + 3, vertex_begin, 4, segment_begin, 4, 0});
    }
    return records;
}

const AnalyticRelationshipPairRecord& single_pair(const AnalyticFilteredBatchResult& result,
                                                  std::uint8_t dimension)
{
    require(result.error == AnalyticFilteredBatchError::none && result.packet,
            "relationship batch failed");
    require(result.packet->records.relationship_results.size() == 1 &&
                result.packet->records.relationship_results[0].status == 0 &&
                result.packet->records.relationship_results[0].aggregate_dimension == dimension &&
                result.packet->records.relationship_results[0].pair_count == 1 &&
                result.packet->records.relationship_pairs.size() == 1,
            "relationship cardinality or aggregate drifted");
    return result.packet->records.relationship_pairs.front();
}

void test_rectangle_relationship_dimensions_and_containment()
{
    const auto evaluate = [](RectangleSpec left, RectangleSpec right)
    {
        auto records = rectangle_jobs({left, right});
        records.relationship_queries = {{9000, 10, 11}};
        require(validate_analytic_request_packet_records(records) ==
                    AnalyticRequestPacketError::none,
                "rectangle relationship request invalid");
        return build_analytic_filtered_batch(records);
    };

    const auto point = evaluate({0, 0, 1000, 1000}, {1000, 1000, 2000, 2000});
    require(single_pair(point, 1).dimension == 1, "point contact was not one-dimensional row 1");

    const auto curve = evaluate({0, 0, 1000, 1000}, {1000, 0, 2000, 1000});
    require(single_pair(curve, 2).dimension == 2, "shared edge was not curve contact");

    const auto area = evaluate({0, 0, 1500, 1000}, {1000, 0, 2000, 1000});
    const auto& area_pair = single_pair(area, 3);
    require(area_pair.dimension == 3 && !area_pair.equality && !area_pair.left_contains_right &&
                !area_pair.right_contains_left,
            "partial area overlap flags drifted");

    const auto contained = evaluate({0, 0, 2000, 2000}, {500, 500, 1500, 1500});
    const auto& contained_pair = single_pair(contained, 3);
    require(contained_pair.left_contains_right && !contained_pair.right_contains_left &&
                !contained_pair.equality,
            "strict containment flags drifted");

    const auto equal = evaluate({0, 0, 1000, 1000}, {0, 0, 1000, 1000});
    const auto& equal_pair = single_pair(equal, 3);
    require(equal_pair.equality && equal_pair.left_contains_right && equal_pair.right_contains_left,
            "equality flags drifted");
}

void test_relationship_query_orientation_and_strict_gap()
{
    auto records = rectangle_jobs({{0, 0, 1000, 1000}, {999, 0, 2000, 1000}});
    records.relationship_queries = {{9000, 10, 11}, {9001, 11, 10}, {9002, 10, 11}};
    const auto result = build_analytic_filtered_batch(records);
    require(result.error == AnalyticFilteredBatchError::none && result.packet &&
                result.packet->records.relationship_results.size() == 3 &&
                result.packet->records.relationship_pairs.size() == 3,
            "repeated/reverse relationship queries failed");
    const auto& pairs = result.packet->records.relationship_pairs;
    require(pairs[0].dimension == 3 && pairs[1].dimension == 3 && pairs[2].dimension == 3 &&
                pairs[0].left_result_region_id == pairs[1].right_result_region_id &&
                pairs[0].right_result_region_id == pairs[1].left_result_region_id,
            "reverse relationship orientation drifted");

    auto gap = rectangle_jobs({{0, 0, 1000, 1000}, {1001, 0, 2000, 1000}});
    gap.relationship_queries = {{9000, 10, 11}};
    const auto gap_result = build_analytic_filtered_batch(gap);
    require(gap_result.error == AnalyticFilteredBatchError::none && gap_result.packet &&
                gap_result.packet->records.relationship_results[0].aggregate_dimension == 0 &&
                gap_result.packet->records.relationship_pairs.empty(),
            "one-nanometre gap was repaired by relationship evaluation");
}

AnalyticRequestPacketRecords
disk_jobs(const std::vector<std::tuple<std::int64_t, std::int64_t, std::uint64_t>>& disks)
{
    AnalyticRequestPacketRecords records;
    for (std::uint32_t index = 0; index < disks.size(); ++index)
    {
        records.jobs.push_back({10 + index, index, 1});
        records.stages.push_back({100 + index, 1, index, 1});
        records.operands.push_back({1000 + index, 2, index});
        const auto [x, y, radius] = disks[index];
        records.disks.push_back({5000 + index, x, y, radius});
    }
    return records;
}

void test_arc_relationships_and_dependency_status()
{
    const auto evaluate = [](std::int64_t x, std::uint64_t left_radius, std::uint64_t right_radius)
    {
        auto records = disk_jobs({{0, 0, left_radius}, {x, 0, right_radius}});
        records.relationship_queries = {{9000, 10, 11}};
        return build_analytic_filtered_batch(records);
    };
    const auto gap = evaluate(2001, 1000, 1000);
    require(gap.error == AnalyticFilteredBatchError::none && gap.packet &&
                gap.packet->records.relationship_pairs.empty(),
            "one-nanometre disk gap was repaired closed");
    require(single_pair(evaluate(1999, 1000, 1000), 3).dimension == 3,
            "one-nanometre disk overlap was not area");
    require(single_pair(evaluate(2000, 1000, 1000), 1).dimension == 1,
            "externally tangent disks were not point contact");
    const auto& contained = single_pair(evaluate(0, 2000, 1000), 3);
    require(contained.left_contains_right && !contained.right_contains_left,
            "disk containment flags drifted");
    require(single_pair(evaluate(0, 1000, 1000), 3).equality, "equal disks were not equal");

    auto failed = unsupported_then_disk();
    failed.relationship_queries = {{9000, 10, 20}};
    const auto failed_result = build_analytic_filtered_batch(failed);
    require(failed_result.error == AnalyticFilteredBatchError::none && failed_result.packet &&
                failed_result.packet->records.relationship_results.size() == 1 &&
                failed_result.packet->records.relationship_results[0].status == 1 &&
                failed_result.packet->records.relationship_results[0].pair_begin == 0 &&
                failed_result.packet->records.relationship_results[0].pair_count == 0,
            "failed dependency was not query-locally skipped");

    auto empty = empty_jobs(2);
    empty.relationship_queries = {{9000, 1000, 1001}};
    const auto empty_result = build_analytic_filtered_batch(empty);
    require(empty_result.error == AnalyticFilteredBatchError::none && empty_result.packet &&
                empty_result.packet->records.relationship_results[0].status == 0 &&
                empty_result.packet->records.relationship_results[0].aggregate_dimension == 0 &&
                empty_result.packet->records.relationship_pairs.empty(),
            "empty successful jobs were not disjoint");
}

void test_self_query_multi_region_and_island()
{
    auto touching = rectangle_jobs({{0, 0, 1000, 1000}, {1000, 1000, 2000, 2000}});
    touching.jobs = {{10, 0, 1}};
    touching.stages = {{900, 1, 0, 2}};
    touching.relationship_queries = {{9000, 10, 10}};
    const auto touching_result = build_analytic_filtered_batch(touching);
    require(touching_result.error == AnalyticFilteredBatchError::none && touching_result.packet &&
                touching_result.packet->records.regions.size() == 2 &&
                touching_result.packet->records.relationship_results[0].aggregate_dimension == 3 &&
                touching_result.packet->records.relationship_pairs.size() == 4,
            "self-query point-touching regions failed");
    std::uint32_t diagonal = 0;
    std::uint32_t ordered_points = 0;
    for (const auto& pair : touching_result.packet->records.relationship_pairs)
    {
        if (pair.left_result_region_id == pair.right_result_region_id)
            diagonal += pair.dimension == 3 && pair.equality ? 1U : 0U;
        else
            ordered_points += pair.dimension == 1 ? 1U : 0U;
    }
    require(diagonal == 2 && ordered_points == 2,
            "self-query diagonal or ordered point rows drifted");

    AnalyticRequestPacketRecords island;
    island.jobs = {{10, 0, 1}};
    island.stages = {{100, 1, 0, 2}};
    island.operands = {{1000, 3, 0}, {1001, 2, 0}};
    island.annuli = {{5000, 0, 0, 1000, 2000}};
    island.disks = {{5001, 0, 0, 500}};
    island.relationship_queries = {{9000, 10, 10}};
    const auto island_result = build_analytic_filtered_batch(island);
    require(island_result.error == AnalyticFilteredBatchError::none && island_result.packet &&
                island_result.packet->records.regions.size() == 2 &&
                island_result.packet->records.rings.size() == 3 &&
                island_result.packet->records.relationship_pairs.size() == 2,
            "hole/island self relationship failed");
    for (const auto& pair : island_result.packet->records.relationship_pairs)
        require(pair.left_result_region_id == pair.right_result_region_id && pair.dimension == 3 &&
                    pair.equality,
                "hole/island emitted a false cross-region relationship");
}

void test_relationship_resource_boundaries()
{
    auto records = rectangle_jobs({{0, 0, 1500, 1000}, {1000, 0, 2000, 1000}});
    records.relationship_queries = {{9000, 10, 11}};
    const auto baseline = build_analytic_filtered_batch(records);
    require(baseline.error == AnalyticFilteredBatchError::none && baseline.packet &&
                baseline.telemetry.algebraic_fallback_calls == 0,
            "relationship resource baseline failed");
    AnalyticFilteredBatchLimits exact;
    exact.assembly_work_units = baseline.telemetry.merge_work_units;
    const auto exact_result = build_analytic_filtered_batch(records, exact);
    require(exact_result.error == AnalyticFilteredBatchError::none && exact_result.packet &&
                exact_result.packet->bytes == baseline.packet->bytes,
            "exact relationship work boundary failed");
    --exact.assembly_work_units;
    const auto short_result = build_analytic_filtered_batch(records, exact);
    require(short_result.error == AnalyticFilteredBatchError::resource_limit_exceeded &&
                !short_result.packet,
            "one-short relationship work did not fail closed");

    AnalyticFilteredBatchLimits memory;
    std::uint64_t low = 0;
    std::uint64_t high = memory.working_memory_bytes;
    while (low < high)
    {
        const std::uint64_t middle = low + (high - low) / 2;
        memory.working_memory_bytes = middle;
        const auto candidate = build_analytic_filtered_batch(records, memory);
        if (candidate.error == AnalyticFilteredBatchError::none && candidate.packet &&
            candidate.packet->bytes == baseline.packet->bytes)
            high = middle;
        else
            low = middle + 1;
    }
    memory.working_memory_bytes = low;
    const auto exact_memory = build_analytic_filtered_batch(records, memory);
    require(exact_memory.error == AnalyticFilteredBatchError::none && exact_memory.packet &&
                exact_memory.packet->bytes == baseline.packet->bytes,
            "exact relationship memory boundary failed");
    require(memory.working_memory_bytes != 0, "relationship memory threshold was zero");
    --memory.working_memory_bytes;
    const auto short_memory = build_analytic_filtered_batch(records, memory);
    require(short_memory.error == AnalyticFilteredBatchError::resource_limit_exceeded &&
                !short_memory.packet,
            "one-byte-short relationship memory did not fail closed");

    auto arc_records = disk_jobs({{0, 0, 1000}, {1999, 0, 1000}});
    arc_records.relationship_queries = {{9050, 10, 11}};
    const auto arc_baseline = build_analytic_filtered_batch(arc_records);
    require(arc_baseline.error == AnalyticFilteredBatchError::none && arc_baseline.packet,
            "arc relationship work baseline failed");
    AnalyticFilteredBatchLimits arc_work;
    arc_work.assembly_work_units = arc_baseline.telemetry.merge_work_units;
    const auto arc_exact = build_analytic_filtered_batch(arc_records, arc_work);
    require(arc_exact.error == AnalyticFilteredBatchError::none && arc_exact.packet &&
                arc_exact.packet->bytes == arc_baseline.packet->bytes,
            "exact arc relationship work boundary failed");
    --arc_work.assembly_work_units;
    const auto arc_short = build_analytic_filtered_batch(arc_records, arc_work);
    require(arc_short.error == AnalyticFilteredBatchError::resource_limit_exceeded &&
                !arc_short.packet,
            "one-short arc relationship work did not fail closed");
}

void test_relationship_bipartite_index_boundaries()
{
    const auto sparse = [](std::uint32_t count)
    {
        std::vector<AnalyticCurveBoundsNm> bounds;
        bounds.reserve(count * 2);
        for (std::uint32_t index = 0; index < count; ++index)
            bounds.push_back({index + 1, 0.0, static_cast<double>(index), 1000.0,
                              static_cast<double>(index + count)});
        for (std::uint32_t index = 0; index < count; ++index)
            bounds.push_back({count + index + 1, 10'000.0, static_cast<double>(index), 11'000.0,
                              static_cast<double>(index + count)});
        return analytic_execution_detail::build_bipartite_curve_candidates(
            bounds, count, {}, analytic_execution_detail::kStrictPublishedGeometry);
    };
    const auto small = sparse(32);
    const auto large = sparse(64);
    require(small.error == AnalyticBroadPhaseError::none && small.pairs.empty() &&
                large.error == AnalyticBroadPhaseError::none && large.pairs.empty(),
            "bipartite index emitted same-side or disjoint cross-side candidates");
    require(large.telemetry.work_units < small.telemetry.work_units * 3 &&
                large.telemetry.peak_working_memory_bytes <
                    small.telemetry.peak_working_memory_bytes * 3,
            "sparse bipartite index exceeded the 2x scaling envelope");

    std::vector<AnalyticCurveBoundsNm> dense;
    for (std::uint32_t index = 0; index < 9; ++index)
        dense.push_back({index + 1, 0.0, 0.0, 1000.0, 1000.0});
    AnalyticSolverLimits limits;
    limits.examined_curve_pairs = 19;
    const auto short_result = analytic_execution_detail::build_bipartite_curve_candidates(
        dense, 4, limits, analytic_execution_detail::kStrictPublishedGeometry);
    require(short_result.error == AnalyticBroadPhaseError::resource_limit_exceeded,
            "one-short bipartite candidate cap did not fail closed");
    limits.examined_curve_pairs = 20;
    const auto exact_result = analytic_execution_detail::build_bipartite_curve_candidates(
        dense, 4, limits, analytic_execution_detail::kStrictPublishedGeometry);
    require(exact_result.error == AnalyticBroadPhaseError::none && exact_result.pairs.size() == 20,
            "exact bipartite candidate cap drifted");
    limits.working_memory_bytes = exact_result.telemetry.peak_working_memory_bytes;
    const auto exact_memory = analytic_execution_detail::build_bipartite_curve_candidates(
        dense, 4, limits, analytic_execution_detail::kStrictPublishedGeometry);
    require(exact_memory.error == AnalyticBroadPhaseError::none && exact_memory.pairs.size() == 20,
            "exact bipartite index memory boundary failed");
    --limits.working_memory_bytes;
    const auto short_memory = analytic_execution_detail::build_bipartite_curve_candidates(
        dense, 4, limits, analytic_execution_detail::kStrictPublishedGeometry);
    require(short_memory.error == AnalyticBroadPhaseError::resource_limit_exceeded,
            "one-short bipartite index memory did not fail closed");

    std::vector<AnalyticCurveBoundsNm> sixty_five;
    sixty_five.reserve(66);
    for (std::uint32_t index = 0; index < 66; ++index)
        sixty_five.push_back({index + 1, 0.0, 0.0, 1000.0, 1000.0});
    AnalyticSolverLimits capacity_limits;
    const auto capacity = analytic_execution_detail::build_bipartite_curve_candidates(
        sixty_five, 1, capacity_limits, analytic_execution_detail::kStrictPublishedGeometry);
    require(capacity.error == AnalyticBroadPhaseError::none && capacity.pairs.size() == 65 &&
                capacity.telemetry.retained_pair_bytes == 128 * 8,
            "65-candidate retained capacity drifted");
    capacity_limits.working_memory_bytes = capacity.telemetry.peak_working_memory_bytes;
    require(analytic_execution_detail::build_bipartite_curve_candidates(
                sixty_five, 1, capacity_limits, analytic_execution_detail::kStrictPublishedGeometry)
                    .error == AnalyticBroadPhaseError::none,
            "65-candidate exact memory boundary failed");
    --capacity_limits.working_memory_bytes;
    require(analytic_execution_detail::build_bipartite_curve_candidates(
                sixty_five, 1, capacity_limits, analytic_execution_detail::kStrictPublishedGeometry)
                    .error == AnalyticBroadPhaseError::resource_limit_exceeded,
            "65-candidate one-short memory did not fail closed");
}

void test_relationship_cache_and_failed_query_memory_boundaries()
{
    const auto verify = [](const AnalyticRequestPacketRecords& records, const char* exact_message,
                           const char* short_message)
    {
        const auto baseline = build_analytic_filtered_batch(records);
        require(baseline.error == AnalyticFilteredBatchError::none && baseline.packet,
                "relationship memory fixture failed");
        AnalyticFilteredBatchLimits limits;
        std::uint64_t low = 0;
        std::uint64_t high = limits.working_memory_bytes;
        while (low < high)
        {
            const std::uint64_t middle = low + (high - low) / 2;
            limits.working_memory_bytes = middle;
            const auto candidate = build_analytic_filtered_batch(records, limits);
            if (candidate.error == AnalyticFilteredBatchError::none && candidate.packet &&
                candidate.packet->bytes == baseline.packet->bytes)
                high = middle;
            else
                low = middle + 1;
        }
        limits.working_memory_bytes = low;
        const auto exact = build_analytic_filtered_batch(records, limits);
        require(exact.error == AnalyticFilteredBatchError::none && exact.packet &&
                    exact.packet->bytes == baseline.packet->bytes,
                exact_message);
        require(low != 0, "relationship cache memory threshold was zero");
        --limits.working_memory_bytes;
        const auto one_short = build_analytic_filtered_batch(records, limits);
        require(one_short.error == AnalyticFilteredBatchError::resource_limit_exceeded &&
                    !one_short.packet,
                short_message);
    };

    auto failed = unsupported_then_disk();
    failed.relationship_queries = {{9000, 10, 20}, {9001, 20, 10}, {9002, 10, 10}, {9003, 10, 20}};
    verify(failed, "exact all-failed query memory boundary failed",
           "one-short all-failed query memory did not fail closed");

    auto repeated = rectangle_jobs({{0, 0, 1500, 1000}, {1000, 0, 2000, 1000}});
    repeated.relationship_queries = {
        {9100, 10, 11}, {9101, 11, 10}, {9102, 10, 11}, {9103, 11, 10}};
    verify(repeated, "exact repeated-query cache memory boundary failed",
           "one-short repeated-query cache memory did not fail closed");

    auto mixed = rectangle_jobs({{0, 0, 2000, 2000}});
    mixed.jobs.push_back({11, 1, 1});
    mixed.stages.push_back({200, 1, 1, 1});
    mixed.operands.push_back({2000, 2, 0});
    mixed.disks.push_back({5000, 1000, 1000, 750});
    mixed.relationship_queries = {{9200, 10, 11}};
    verify(mixed, "exact mixed line/arc relationship memory boundary failed",
           "one-short mixed line/arc relationship memory did not fail closed");
}

void test_relationship_remaining_packet_boundary()
{
    auto request = rectangle_jobs({{0, 0, 1000, 1000}, {1000, 1000, 2000, 2000}});
    const auto published = build_analytic_filtered_batch(request);
    require(published.error == AnalyticFilteredBatchError::none && published.packet,
            "relationship packet-boundary publication failed");
    request.relationship_queries = {{9300, 10, 11}};
    const auto exact = analytic_relationship_detail::evaluate(
        request, published.packet->records, {}, std::numeric_limits<std::uint64_t>::max(),
        std::numeric_limits<std::uint64_t>::max(), 0, 64);
    require(exact.error == analytic_relationship_detail::EvaluationError::none &&
                exact.results.size() == 1 && exact.pairs.size() == 1,
            "exact remaining relationship packet bytes failed");
    const auto one_short = analytic_relationship_detail::evaluate(
        request, published.packet->records, {}, std::numeric_limits<std::uint64_t>::max(),
        std::numeric_limits<std::uint64_t>::max(), 0, 63);
    require(one_short.error ==
                    analytic_relationship_detail::EvaluationError::resource_limit_exceeded &&
                one_short.results.empty() && one_short.pairs.empty(),
            "one-short remaining relationship packet bytes did not fail closed");
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
    require(first.jobs.size() == 2 && first.jobs[0].emitted_record_bytes > 0 &&
                first.jobs[1].emitted_record_bytes > 0 &&
                first.jobs[0].algebraic_fallback_calls == 0 &&
                first.jobs[1].algebraic_fallback_calls == 0,
            "per-job qualification telemetry wrong");
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
    require(output.diagnostics.size() == 1 && output.diagnostics[0].code == 65'539,
            "invalid swept topology diagnostic wrong");
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
    require(result.jobs.size() == 2 && result.jobs[0].diagnostic_code == 65'539 &&
                result.jobs[0].emitted_record_bytes > 0 && result.jobs[1].emitted_record_bytes > 0,
            "failed-job qualification telemetry wrong");
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

void test_disjoint_relationship()
{
    auto records = two_disks();
    records.relationship_queries = {{9000, 10, 20}};
    require(validate_analytic_request_packet_records(records) == AnalyticRequestPacketError::none,
            "relationship request invalid");
    const auto result = build_analytic_filtered_batch(records);
    require(result.error == AnalyticFilteredBatchError::none && result.packet,
            "disjoint relationship failed");
    require(result.packet->records.relationship_results.size() == 1 &&
                result.packet->records.relationship_results[0].query_id == 9000 &&
                result.packet->records.relationship_results[0].status == 0 &&
                result.packet->records.relationship_results[0].aggregate_dimension == 0 &&
                result.packet->records.relationship_results[0].pair_begin == 0 &&
                result.packet->records.relationship_results[0].pair_count == 0 &&
                result.packet->records.relationship_pairs.empty(),
            "disjoint relationship records drifted");
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

std::string relationship_parity_vector()
{
    std::vector<AnalyticRequestPacketRecords> cases;
    auto rectangles = rectangle_jobs({{0, 0, 1000, 1000}, {1000, 1000, 2000, 2000}});
    rectangles.relationship_queries = {{9000, 10, 11}, {9001, 11, 10}, {9002, 10, 10}};
    cases.push_back(std::move(rectangles));
    auto arcs = disk_jobs({{0, 0, 1000}, {1999, 0, 1000}, {4000, 0, 1000}});
    arcs.relationship_queries = {{9100, 10, 11}, {9101, 11, 10}, {9102, 10, 12}};
    cases.push_back(std::move(arcs));
    auto failed = unsupported_then_disk();
    failed.relationship_queries = {{9200, 10, 20}, {9201, 20, 10}};
    cases.push_back(std::move(failed));

    std::vector<std::uint8_t> bytes;
    for (const auto& records : cases)
    {
        const auto result = build_analytic_filtered_batch(records);
        require(result.error == AnalyticFilteredBatchError::none && result.packet,
                "relationship parity batch failed");
        append_u64(bytes, result.packet->bytes.size());
        bytes.insert(bytes.end(), result.packet->bytes.begin(), result.packet->bytes.end());
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
    }
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
    test_disjoint_relationship();
    test_rectangle_relationship_dimensions_and_containment();
    test_relationship_query_orientation_and_strict_gap();
    test_arc_relationships_and_dependency_status();
    test_self_query_multi_region_and_island();
    test_relationship_resource_boundaries();
    test_relationship_bipartite_index_boundaries();
    test_relationship_cache_and_failed_query_memory_boundaries();
    test_relationship_remaining_packet_boundary();
    test_per_job_memory_is_independent_of_prior_outputs();
    test_per_job_nonmemory_limits_remain_job_local();
    test_merge_work_boundary();
    test_memory_boundary_and_many_job_scaling();
    if (argc == 2 && std::string(argv[1]) == "--emit-parity")
    {
        std::cout << "ANALYTIC_FILTERED_BATCH_VECTOR=" << parity_vector() << '\n';
        std::cout << "ANALYTIC_FILTERED_RELATIONSHIP_VECTOR=" << relationship_parity_vector()
                  << '\n';
    }
    return 0;
}

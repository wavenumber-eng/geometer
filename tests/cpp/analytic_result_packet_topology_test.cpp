#include "geometer/analytic_result_packet_topology.h"
#include "geometer/exact_geometry.h"
#include "geometer/exact_normalization.h"
#include "geometer/sha256.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace
{

using namespace geometer;
using namespace geometer::exact;
constexpr std::uint32_t kNone = std::numeric_limits<std::uint32_t>::max();

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

void add_sources(AnalyticResultPacketRecords& records, AnalyticSourceRole role)
{
    records.source_references = {
        {AnalyticSourceKind::authored_segment_curve, role, 1, 1, 1},
        {AnalyticSourceKind::authored_segment_curve, role, 2, 1, 1},
    };
    records.source_reference_indices = {0, 1};
    records.source_sets = {{0, 2}};
}

void add_vertex(AnalyticResultPacketRecords& records, std::int64_t x, std::int64_t y)
{
    records.vertices.push_back(
        {static_cast<std::uint64_t>(records.vertices.size()) + 1, x, y, 0, 0});
}

void add_line_ring(AnalyticResultPacketRecords& records,
                   const std::vector<std::pair<std::int64_t, std::int64_t>>& points,
                   std::uint32_t parent, std::uint32_t depth)
{
    const std::uint32_t vertex_begin = static_cast<std::uint32_t>(records.vertices.size());
    const std::uint32_t fragment_begin = static_cast<std::uint32_t>(records.fragments.size());
    for (const auto [x, y] : points)
        add_vertex(records, x, y);
    for (std::uint32_t index = 0; index < points.size(); ++index)
    {
        const auto next_index = static_cast<std::uint32_t>((index + 1U) % points.size());
        records.fragments.push_back({static_cast<std::uint64_t>(records.fragments.size()) + 1,
                                     vertex_begin + index, vertex_begin + next_index, 1, 0, false,
                                     0, 1, 0});
        records.fragment_references.push_back(fragment_begin + index);
    }
    records.rings.push_back({static_cast<std::uint64_t>(records.rings.size()) + 1, fragment_begin,
                             static_cast<std::uint32_t>(points.size()), parent, depth, depth & 1U});
}

void finish_success(AnalyticResultPacketRecords& records,
                    const std::vector<std::uint32_t>& outer_rings)
{
    for (const std::uint32_t ring : outer_rings)
        records.regions.push_back(
            {static_cast<std::uint64_t>(records.regions.size()) + 1, ring, 1});
    records.job_results.push_back(
        {1, 0, 0, 0, 0, static_cast<std::uint32_t>(records.regions.size()), 0, 0});
}

AnalyticResultPacketRecords square()
{
    AnalyticResultPacketRecords records;
    add_sources(records, AnalyticSourceRole::authored_line);
    add_line_ring(records, {{0, 0}, {10, 0}, {10, 10}, {0, 10}}, kNone, 0);
    finish_success(records, {0});
    return records;
}

AnalyticResultPacketRecords circle()
{
    AnalyticResultPacketRecords records;
    add_sources(records, AnalyticSourceRole::authored_circular_arc);
    add_vertex(records, -10, 0);
    add_vertex(records, 10, 0);
    records.fragments = {
        {1, 0, 1, 2, 1, false, 10, 1, 0},
        {2, 1, 0, 2, 1, false, 10, 1, 0},
    };
    records.fragment_references = {0, 1};
    records.rings = {{1, 0, 2, kNone, 0, 0}};
    finish_success(records, {0});
    return records;
}

AnalyticResultPacketRecords nested_disjoint()
{
    AnalyticResultPacketRecords records;
    add_sources(records, AnalyticSourceRole::authored_line);
    add_line_ring(records, {{0, 0}, {20, 0}, {20, 20}, {0, 20}}, kNone, 0);
    add_line_ring(records, {{4, 4}, {4, 16}, {16, 16}, {16, 4}}, 0, 1);
    add_line_ring(records, {{8, 8}, {12, 8}, {12, 12}, {8, 12}}, 1, 2);
    add_line_ring(records, {{30, 0}, {40, 0}, {40, 10}, {30, 10}}, kNone, 0);
    finish_success(records, {0, 2, 3});
    return records;
}

AnalyticResultPacketRecords deeply_nested(std::uint32_t ring_count)
{
    AnalyticResultPacketRecords records;
    add_sources(records, AnalyticSourceRole::authored_line);
    std::vector<std::uint32_t> outer_rings;
    outer_rings.reserve((ring_count + 1) / 2);
    for (std::uint32_t ring = 0; ring < ring_count; ++ring)
    {
        const std::int64_t radius = static_cast<std::int64_t>(ring_count - ring);
        const bool hole = ring % 2 != 0;
        add_line_ring(records,
                      {{-radius, -radius},
                       {hole ? -radius : radius, hole ? radius : -radius},
                       {radius, radius},
                       {hole ? radius : -radius, hole ? -radius : radius}},
                      ring == 0 ? kNone : ring - 1, ring);
        if (!hole)
            outer_rings.push_back(ring);
    }
    finish_success(records, outer_rings);
    return records;
}

AnalyticResultPacketRecords many_empty_jobs()
{
    AnalyticResultPacketRecords records;
    records.job_results.reserve(65'535);
    for (std::uint32_t job = 1; job <= 65'535; ++job)
        records.job_results.push_back({job, 0, 0, 0, 0, 0, 0, 0});
    return records;
}

AnalyticResultPacketRecords point_tangent()
{
    AnalyticResultPacketRecords records;
    add_sources(records, AnalyticSourceRole::authored_line);
    add_line_ring(records, {{0, 0}, {2, 0}, {2, 2}, {0, 2}}, kNone, 0);
    add_line_ring(records, {{2, 2}, {4, 2}, {4, 4}, {2, 4}}, kNone, 0);
    finish_success(records, {0, 1});
    return records;
}

void reverse_ring(AnalyticResultPacketRecords& records, std::uint32_t ring)
{
    const auto value = records.rings[ring];
    auto begin = records.fragment_references.begin() + value.fragment_reference_begin;
    auto end = begin + value.fragment_reference_count;
    std::reverse(begin, end);
    for (auto at = begin; at != end; ++at)
    {
        auto& fragment = records.fragments[*at];
        std::swap(fragment.start_vertex, fragment.end_vertex);
        if (fragment.kind == 2)
            fragment.direction = fragment.direction == 1 ? 2 : 1;
    }
}

void require_structural_but_not_topological(const AnalyticResultPacketRecords& records,
                                            const std::string& mutation)
{
    require(validate_analytic_result_packet_records(records) ==
                AnalyticResultPacketLayoutError::none,
            mutation + " did not remain a structurally valid mutation sentinel");
    require(validate_analytic_result_packet_topology(records) ==
                AnalyticResultPacketLayoutError::invalid_packet,
            mutation + " escaped the independent exact topology oracle");
}

std::string digest(const AnalyticResultPacketRecords& records)
{
    const auto encoded = encode_analytic_result_packet_records(records);
    require(encoded.error == AnalyticResultPacketLayoutError::none && encoded.value,
            "lineage fixture failed to encode");
    return sha256_hex(encoded.value->data(), encoded.value->size());
}

void require_tie_policy_sentinel()
{
    Budget budget({5'000'000, 16'777'216});
    auto positive = make_canonical_rational(budget, 1, 2);
    auto negative = make_canonical_rational(budget, -1, 2);
    require(positive.error == Error::none && positive.value && negative.error == Error::none &&
                negative.value,
            "half-grid mutation fixture failed");
    auto positive_nm = normalize_exact_to_integer_nm(budget, *positive.value);
    auto negative_nm = normalize_exact_to_integer_nm(budget, *negative.value);
    require(positive_nm.error == Error::none && positive_nm.value && *positive_nm.value == 1 &&
                negative_nm.error == Error::none && negative_nm.value && *negative_nm.value == -1,
            "ties-to-even mutation escaped the governed ties-away oracle");
}

} // namespace

int main()
{
    for (const auto& valid : {square(), circle(), nested_disjoint(), point_tangent()})
        require(validate_analytic_result_packet_topology(valid) ==
                    AnalyticResultPacketLayoutError::none,
                "valid synthetic topology failed exact replay");

    auto reversed_line = square();
    reverse_ring(reversed_line, 0);
    require_structural_but_not_topological(reversed_line, "reversed line winding");

    auto reversed_arc = circle();
    reverse_ring(reversed_arc, 0);
    require_structural_but_not_topological(reversed_arc, "reversed arc winding");

    auto wrong_parent = nested_disjoint();
    wrong_parent.rings[1].parent_ring = 3;
    require_structural_but_not_topological(wrong_parent, "non-containing parent hierarchy");

    auto lost_island = nested_disjoint();
    lost_island.rings[2].parent_ring = 0;
    lost_island.rings[2].depth = 1;
    lost_island.rings[2].flags = 1;
    lost_island.regions.erase(lost_island.regions.begin() + 1);
    lost_island.regions[1].id = 2;
    lost_island.job_results[0].result_region_count = 2;
    require_structural_but_not_topological(lost_island, "nested island component ownership");

    auto tangent_merge = point_tangent();
    tangent_merge.rings[1].parent_ring = 0;
    tangent_merge.rings[1].depth = 1;
    tangent_merge.rings[1].flags = 1;
    tangent_merge.regions.pop_back();
    tangent_merge.job_results[0].result_region_count = 1;
    require_structural_but_not_topological(tangent_merge, "point-tangent hierarchy merge");

    const auto lineage = square();
    auto omitted_lineage = lineage;
    omitted_lineage.source_references.pop_back();
    omitted_lineage.source_reference_indices.pop_back();
    omitted_lineage.source_sets[0].source_reference_index_count = 1;
    require(validate_analytic_result_packet_topology(omitted_lineage) ==
                AnalyticResultPacketLayoutError::none,
            "lineage-only mutation unexpectedly changed geometry");
    require(digest(lineage) != digest(omitted_lineage),
            "omitted contributor escaped the canonical lineage oracle");

    require_tie_policy_sentinel();
    require(validate_analytic_result_packet_topology(many_empty_jobs()) ==
                AnalyticResultPacketLayoutError::none,
            "many-empty-job ownership indexing failed");
    require(validate_analytic_result_packet_topology(deeply_nested(16'384)) ==
                AnalyticResultPacketLayoutError::limit_exceeded,
            "deep hierarchy did not terminate at the governed exact replay bound");
    std::cout << "ANALYTIC_MUTATION_SENTINELS="
                 "reversed_line,reversed_arc,non_containing_parent,point_tangent_merge,"
                 "nested_island_ownership,omitted_lineage,ties_to_even,deep_hierarchy_bound,"
                 "many_empty_jobs\n";
    return 0;
}

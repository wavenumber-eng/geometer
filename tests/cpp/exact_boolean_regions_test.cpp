#include "geometer/exact_boolean_regions.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace
{

using namespace geometer::exact;

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

ConstructionNodeId rational(ConstructionArena& arena, const BigInt& value)
{
    auto result = arena.make_rational(value);
    require(result.error == Error::none && result.node, "region rational fixture failed");
    return *result.node;
}

ExactPoint point(ConstructionArena& arena, const BigInt& x, const BigInt& y)
{
    return {rational(arena, x), rational(arena, y)};
}

void append_box(ConstructionArena& arena, const BigInt& low_x, const BigInt& low_y,
                const BigInt& high_x, const BigInt& high_y, std::uint64_t occurrence_base,
                std::uint64_t coverage_id, std::vector<ExactAtomicCurve>& curves,
                std::vector<ExactCoverageOccurrence>& coverages)
{
    const ExactPoint a = point(arena, low_x, low_y);
    const ExactPoint b = point(arena, high_x, low_y);
    const ExactPoint c = point(arena, high_x, high_y);
    const ExactPoint d = point(arena, low_x, high_y);
    curves.push_back(
        {ExactAtomicCurveKind::line, a, b, {}, true, false, {{occurrence_base, true}}});
    curves.push_back(
        {ExactAtomicCurveKind::line, b, c, {}, true, false, {{occurrence_base + 1, true}}});
    curves.push_back(
        {ExactAtomicCurveKind::line, d, c, {}, true, false, {{occurrence_base + 2, true}}});
    curves.push_back(
        {ExactAtomicCurveKind::line, a, d, {}, true, false, {{occurrence_base + 3, true}}});
    coverages.push_back({occurrence_base, coverage_id, true});
    coverages.push_back({occurrence_base + 1, coverage_id, true});
    coverages.push_back({occurrence_base + 2, coverage_id, false});
    coverages.push_back({occurrence_base + 3, coverage_id, false});
}

std::string signature(const ExactBooleanRegions& result)
{
    std::ostringstream out;
    out << "R" << result.rings().size() << ':';
    for (const ExactResultRing& ring : result.rings())
    {
        out << ring.depth << ',';
        if (ring.parent_ring == kNoExactResultRing)
            out << 'n';
        else
            out << ring.parent_ring;
        out << ',' << (ring.counterclockwise ? '+' : '-') << '[';
        for (std::uint32_t index = 0; index < ring.half_edge_count; ++index)
            out << result.ring_half_edges()[ring.half_edge_begin + index] << ',';
        out << "];";
    }
    out << "G" << result.regions().size() << ':';
    for (const ExactResultRegion& region : result.regions())
    {
        out << region.outer_ring << '[';
        for (std::uint32_t index = 0; index < region.positive_source_count; ++index)
            out << result.positive_sources()[region.positive_source_begin + index] << ',';
        out << "];";
    }
    out << 'A';
    for (const ExactResultAssociation& association : result.associations())
        out << association.source_id << '>' << association.result_region << ',';
    return out.str();
}

std::string test_shared_edge_union()
{
    Budget geometry_budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(geometry_budget);
    std::vector<ExactAtomicCurve> curves;
    std::vector<ExactCoverageOccurrence> coverages;
    append_box(arena, 0, 0, 2, 2, 100, 10, curves, coverages);
    append_box(arena, 2, 0, 4, 2, 200, 20, curves, coverages);
    ExactArrangementResult arrangement_result = build_exact_arrangement(arena, curves, coverages);
    require(arrangement_result.error == Error::none && arrangement_result.value,
            "shared-edge arrangement failed: " +
                std::to_string(static_cast<unsigned>(arrangement_result.error)));

    Budget selection_budget({2'000'000'000, 268'435'456});
    ExactBooleanSelectionResult selection_result = evaluate_exact_boolean_stages(
        selection_budget, *arrangement_result.value,
        {{1, ExactBooleanStageOperation::union_, {{10, 1000}, {20, 2000}}}});
    require(selection_result.error == Error::none && selection_result.value,
            "shared-edge stage selection failed");
    Budget region_budget({2'000'000'000, 268'435'456});
    ExactBooleanRegionsResult regions = build_exact_boolean_regions(
        region_budget, *arrangement_result.value, *selection_result.value);
    require(regions.error == Error::none && regions.value, "shared-edge region merge failed");
    require(regions.value->regions().size() == 1 && regions.value->rings().size() == 1,
            "shared-edge union must produce one region and one ring");
    require(regions.value->regions()[0].positive_source_count == 2 &&
                regions.value->associations().size() == 2,
            "shared-edge union lost a many-to-one source association");

    std::set<std::uint32_t> retained_edges;
    const ExactResultRing& ring = regions.value->rings()[0];
    for (std::uint32_t index = 0; index < ring.half_edge_count; ++index)
    {
        const std::uint32_t half_edge =
            regions.value->ring_half_edges()[ring.half_edge_begin + index];
        retained_edges.insert(arrangement_result.value->half_edges()[half_edge].edge);
    }
    require(ring.half_edge_count == 6, "shared-edge union boundary fragment count changed");
    for (std::uint32_t edge = 0; edge < arrangement_result.value->edges().size(); ++edge)
    {
        if (arrangement_result.value->edges()[edge].membership_count == 2)
            require(retained_edges.count(edge) == 0, "internal shared-edge seam survived");
    }
    const std::string canonical = signature(*regions.value);

    Budget alternate_selection_budget({2'000'000'000, 268'435'456});
    ExactBooleanSelectionResult alternate_selection = evaluate_exact_boolean_stages(
        alternate_selection_budget, *arrangement_result.value,
        {{1, ExactBooleanStageOperation::union_, {{20, 2000}, {10, 1000}}}});
    require(alternate_selection.error == Error::none && alternate_selection.value,
            "permuted shared-edge selection failed");
    Budget alternate_region_budget({2'000'000'000, 268'435'456});
    ExactBooleanRegionsResult alternate_regions = build_exact_boolean_regions(
        alternate_region_budget, *arrangement_result.value, *alternate_selection.value);
    require(alternate_regions.error == Error::none && alternate_regions.value &&
                signature(*alternate_regions.value) == canonical,
            "same-stage operand permutation changed canonical regions");
    return canonical;
}

std::string test_point_tangent_regions()
{
    Budget geometry_budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(geometry_budget);
    std::vector<ExactAtomicCurve> curves;
    std::vector<ExactCoverageOccurrence> coverages;
    append_box(arena, 0, 0, 2, 2, 300, 30, curves, coverages);
    append_box(arena, 2, 2, 4, 4, 400, 30, curves, coverages);
    ExactArrangementResult arrangement_result = build_exact_arrangement(arena, curves, coverages);
    require(arrangement_result.error == Error::none && arrangement_result.value,
            "point-tangent arrangement failed");
    Budget selection_budget({2'000'000'000, 268'435'456});
    ExactBooleanSelectionResult selection_result =
        evaluate_exact_boolean_stages(selection_budget, *arrangement_result.value,
                                      {{2, ExactBooleanStageOperation::union_, {{30, 3000}}}});
    require(selection_result.error == Error::none && selection_result.value,
            "point-tangent selection failed");
    Budget region_budget({2'000'000'000, 268'435'456});
    ExactBooleanRegionsResult regions = build_exact_boolean_regions(
        region_budget, *arrangement_result.value, *selection_result.value);
    require(regions.error == Error::none && regions.value, "point-tangent region merge failed");
    require(regions.value->regions().size() == 2 && regions.value->rings().size() == 2,
            "point contact must not merge area components");
    require(regions.value->associations().size() == 2 &&
                regions.value->associations()[0].source_id == 3000 &&
                regions.value->associations()[0].result_region == 0 &&
                regions.value->associations()[1].source_id == 3000 &&
                regions.value->associations()[1].result_region == 1,
            "one source must associate with both point-tangent result regions");

    std::set<std::uint32_t> first_vertices;
    std::set<std::uint32_t> second_vertices;
    for (std::uint32_t ring_index = 0; ring_index < 2; ++ring_index)
    {
        const ExactResultRing& ring = regions.value->rings()[ring_index];
        require(ring.depth == 0 && ring.parent_ring == kNoExactResultRing && ring.counterclockwise,
                "point-tangent outer ring hierarchy changed");
        auto& vertices = ring_index == 0 ? first_vertices : second_vertices;
        for (std::uint32_t index = 0; index < ring.half_edge_count; ++index)
        {
            const std::uint32_t half_edge =
                regions.value->ring_half_edges()[ring.half_edge_begin + index];
            vertices.insert(arrangement_result.value->half_edges()[half_edge].origin_vertex);
        }
    }
    std::vector<std::uint32_t> shared;
    std::set_intersection(first_vertices.begin(), first_vertices.end(), second_vertices.begin(),
                          second_vertices.end(), std::back_inserter(shared));
    require(shared.size() == 1, "point-tangent regions must share exactly one vertex");
    return signature(*regions.value);
}

struct NestedResult
{
    std::string signature;
    std::uint64_t work = 0;
    std::uint64_t storage = 0;
};

NestedResult test_nested_hole_and_island()
{
    Budget geometry_budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(geometry_budget);
    std::vector<ExactAtomicCurve> curves;
    std::vector<ExactCoverageOccurrence> coverages;
    append_box(arena, 0, 0, 12, 12, 500, 50, curves, coverages);
    append_box(arena, 3, 3, 9, 9, 600, 60, curves, coverages);
    append_box(arena, 5, 5, 7, 7, 700, 70, curves, coverages);
    ExactArrangementResult arrangement_result = build_exact_arrangement(arena, curves, coverages);
    require(arrangement_result.error == Error::none && arrangement_result.value,
            "nested arrangement failed");
    const std::vector<ExactBooleanStage> stages{
        {10, ExactBooleanStageOperation::union_, {{50, 5000}}},
        {20, ExactBooleanStageOperation::difference, {{60, 6000}}},
        {30, ExactBooleanStageOperation::union_, {{70, 7000}}},
    };
    Budget selection_budget({2'000'000'000, 268'435'456});
    ExactBooleanSelectionResult selection_result =
        evaluate_exact_boolean_stages(selection_budget, *arrangement_result.value, stages);
    require(selection_result.error == Error::none && selection_result.value,
            "nested selection failed");
    Budget region_budget({2'000'000'000, 268'435'456});
    ExactBooleanRegionsResult regions = build_exact_boolean_regions(
        region_budget, *arrangement_result.value, *selection_result.value);
    require(regions.error == Error::none && regions.value, "nested region merge failed");
    require(regions.value->regions().size() == 2 && regions.value->rings().size() == 3,
            "nested frame and island topology changed");
    std::vector<std::uint32_t> ring_at_depth(3, kNoExactResultRing);
    for (std::uint32_t ring = 0; ring < regions.value->rings().size(); ++ring)
    {
        const ExactResultRing& value = regions.value->rings()[ring];
        require(value.depth < ring_at_depth.size() &&
                    ring_at_depth[value.depth] == kNoExactResultRing,
                "nested fixture must have one ring at each depth");
        ring_at_depth[value.depth] = ring;
        require(value.counterclockwise == (value.depth % 2 == 0),
                "nested ring winding parity changed");
    }
    require(regions.value->rings()[ring_at_depth[0]].parent_ring == kNoExactResultRing &&
                regions.value->rings()[ring_at_depth[1]].parent_ring == ring_at_depth[0] &&
                regions.value->rings()[ring_at_depth[2]].parent_ring == ring_at_depth[1],
            "nested ring parent chain changed");
    require(regions.value->associations().size() == 2 &&
                regions.value->associations()[0].source_id == 5000 &&
                regions.value->associations()[1].source_id == 7000,
            "nested many-to-many association projection changed");

    const BudgetUsage usage = region_budget.usage();
    Budget short_work({usage.work_units - 1, 268'435'456});
    ExactBooleanRegionsResult work_failure =
        build_exact_boolean_regions(short_work, *arrangement_result.value, *selection_result.value);
    require(work_failure.error == Error::resource_limit_exceeded && !work_failure.value,
            "one-unit-short region work must fail closed");
    Budget short_storage({2'000'000'000, usage.owned_bytes - 1});
    ExactBooleanRegionsResult storage_failure = build_exact_boolean_regions(
        short_storage, *arrangement_result.value, *selection_result.value);
    require(storage_failure.error == Error::resource_limit_exceeded && !storage_failure.value &&
                short_storage.usage().owned_bytes == 0,
            "one-byte-short region storage must fail without a logical leak");

    return {signature(*regions.value), usage.work_units, usage.owned_bytes};
}

} // namespace

int main()
{
    const std::string shared = test_shared_edge_union();
    const std::string tangent = test_point_tangent_regions();
    const NestedResult nested = test_nested_hole_and_island();
    std::cout << "EXACT_BOOLEAN_REGIONS_VECTOR=EBR1:" << shared << '|' << tangent << '|'
              << nested.signature << '\n';
    std::cout << "EXACT_BOOLEAN_REGIONS_WORK=" << nested.work << '\n';
    std::cout << "EXACT_BOOLEAN_REGIONS_STORAGE=" << nested.storage << '\n';
    return 0;
}

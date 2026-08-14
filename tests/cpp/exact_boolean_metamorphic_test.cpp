#include "geometer/exact_result_normalization.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace
{

using namespace geometer::exact;

struct Rectangle
{
    std::int64_t x0 = 0;
    std::int64_t y0 = 0;
    std::int64_t x1 = 0;
    std::int64_t y1 = 0;
};

struct Scenario
{
    std::string geometry;
    std::string lineage;
    std::int64_t signed_double_area = 0;
    std::size_t regions = 0;
};

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

ConstructionNodeId rational(ConstructionArena& arena, std::int64_t value)
{
    auto result = arena.make_rational(value);
    require(result.error == Error::none && result.node, "metamorphic rational fixture failed");
    return *result.node;
}

ExactPoint point(ConstructionArena& arena, std::int64_t x, std::int64_t y)
{
    return {rational(arena, x), rational(arena, y)};
}

void append_rectangle(ConstructionArena& arena, const Rectangle& rectangle,
                      std::uint64_t occurrence_base, std::uint64_t coverage_id,
                      std::vector<ExactAtomicCurve>& curves,
                      std::vector<ExactCoverageOccurrence>& coverages)
{
    auto append = [&](const ExactPoint& start, const ExactPoint& end, bool material_on_left)
    {
        curves.push_back(
            {ExactAtomicCurveKind::line, start, end, {}, true, false, {{occurrence_base, true}}});
        coverages.push_back({occurrence_base++, coverage_id, material_on_left});
    };
    for (std::int64_t x = rectangle.x0; x < rectangle.x1; ++x)
        append(point(arena, x, rectangle.y0), point(arena, x + 1, rectangle.y0), true);
    for (std::int64_t y = rectangle.y0; y < rectangle.y1; ++y)
        append(point(arena, rectangle.x1, y), point(arena, rectangle.x1, y + 1), true);
    for (std::int64_t x = rectangle.x0; x < rectangle.x1; ++x)
        append(point(arena, x, rectangle.y1), point(arena, x + 1, rectangle.y1), false);
    for (std::int64_t y = rectangle.y0; y < rectangle.y1; ++y)
        append(point(arena, rectangle.x0, y), point(arena, rectangle.x0, y + 1), false);
}

std::vector<ExactBooleanOperand> operands(const std::vector<std::uint64_t>& coverage_ids)
{
    std::vector<ExactBooleanOperand> result;
    for (const std::uint64_t coverage_id : coverage_ids)
        result.push_back({coverage_id, 1000 + coverage_id});
    return result;
}

std::string geometry_signature(const ExactNormalizedBooleanResult& result)
{
    std::ostringstream out;
    out << 'v';
    for (const ExactNormalizedResultVertex& vertex : result.vertices())
        out << vertex.x_nm << ',' << vertex.y_nm << ';';
    out << 'f';
    for (const ExactNormalizedResultFragment& fragment : result.fragments())
        out << fragment.start_vertex << ',' << fragment.end_vertex << ','
            << static_cast<unsigned>(fragment.kind) << ';';
    out << 'r';
    for (const ExactNormalizedResultRing& ring : result.rings())
        out << ring.fragment_begin << ',' << ring.fragment_count << ',' << ring.parent_ring << ','
            << ring.depth << ',' << ring.counterclockwise << ';';
    out << 'g';
    for (const ExactNormalizedResultRegion& region : result.regions())
        out << region.outer_ring << ';';
    return out.str();
}

std::string lineage_signature(const ExactBooleanSelection& selection,
                              const ExactBooleanRegions& regions)
{
    std::ostringstream out;
    out << 'f';
    for (const ExactSelectedFace& face : selection.faces())
    {
        out << face.material << ":p";
        for (std::uint32_t index = 0; index < face.positive_source_count; ++index)
            out << selection.positive_sources()[face.positive_source_begin + index] << ',';
        out << ":s";
        for (std::uint32_t index = 0; index < face.subtraction_source_count; ++index)
            out << selection.subtraction_sources()[face.subtraction_source_begin + index] << ',';
        out << ';';
    }
    out << 'r';
    for (const std::uint64_t source : regions.positive_sources())
        out << source << ',';
    out << 'a';
    for (const ExactResultAssociation& association : regions.associations())
        out << association.source_id << ',' << association.result_region << ';';
    return out.str();
}

Scenario run_scenario(const std::vector<Rectangle>& rectangles,
                      const std::vector<ExactBooleanStage>& stages)
{
    Budget budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    std::vector<ExactAtomicCurve> curves;
    std::vector<ExactCoverageOccurrence> coverages;
    for (std::uint32_t index = 0; index < rectangles.size(); ++index)
        append_rectangle(arena, rectangles[index], 100 * (index + 1), index + 1, curves, coverages);
    ExactArrangementResult arrangement = build_exact_arrangement(arena, curves, coverages);
    require(arrangement.error == Error::none && arrangement.value,
            "metamorphic arrangement failed");
    ExactBooleanSelectionResult selection =
        evaluate_exact_boolean_stages(budget, *arrangement.value, stages);
    require(selection.error == Error::none && selection.value, "metamorphic selection failed");
    ExactBooleanRegionsResult regions =
        build_exact_boolean_regions(budget, *arrangement.value, *selection.value);
    require(regions.error == Error::none && regions.value, "metamorphic regions failed");
    ExactNormalizedBooleanResultResult normalized =
        normalize_exact_boolean_result(arena, *arrangement.value, *selection.value, *regions.value);
    require(normalized.error == ExactResultNormalizationError::none && normalized.value,
            "metamorphic normalization failed");
    std::int64_t area = 0;
    for (const ExactNormalizedResultFragment& fragment : normalized.value->fragments())
    {
        const ExactNormalizedResultVertex& start =
            normalized.value->vertices()[fragment.start_vertex];
        const ExactNormalizedResultVertex& end = normalized.value->vertices()[fragment.end_vertex];
        area += start.x_nm * end.y_nm - start.y_nm * end.x_nm;
    }
    return {geometry_signature(*normalized.value),
            lineage_signature(*selection.value, *regions.value), area,
            normalized.value->regions().size()};
}

} // namespace

int main()
{
    const Rectangle lower_left{0, 0, 1, 1};
    const Rectangle lower_right{1, 0, 2, 1};
    const Rectangle upper_left{0, 1, 1, 2};
    const Rectangle upper_right{1, 1, 2, 2};
    const Rectangle full{0, 0, 2, 2};

    const std::vector<Rectangle> union_rectangles{lower_left, lower_right, upper_left};
    std::array<std::uint64_t, 3> union_order{1, 2, 3};
    const Scenario union_baseline = run_scenario(
        union_rectangles, {{10, ExactBooleanStageOperation::union_, operands({1, 2, 3})}});
    std::uint32_t union_permutations = 0;
    do
    {
        const Scenario candidate = run_scenario(
            union_rectangles, {{10, ExactBooleanStageOperation::union_,
                                operands({union_order[0], union_order[1], union_order[2]})}});
        require(candidate.geometry == union_baseline.geometry &&
                    candidate.lineage == union_baseline.lineage,
                "same-stage union operand permutation changed geometry or lineage");
        ++union_permutations;
    } while (std::next_permutation(union_order.begin(), union_order.end()));
    require(union_permutations == 6 && union_baseline.signed_double_area == 6 &&
                union_baseline.regions == 1,
            "same-stage union independent L-shape oracle changed");

    const std::vector<Rectangle> difference_rectangles{full, lower_left, upper_right};
    std::array<std::uint64_t, 2> difference_order{2, 3};
    const Scenario difference_baseline = run_scenario(
        difference_rectangles, {{10, ExactBooleanStageOperation::union_, operands({1})},
                                {20, ExactBooleanStageOperation::difference, operands({2, 3})}});
    std::uint32_t difference_permutations = 0;
    do
    {
        const Scenario candidate = run_scenario(
            difference_rectangles, {{10, ExactBooleanStageOperation::union_, operands({1})},
                                    {20, ExactBooleanStageOperation::difference,
                                     operands({difference_order[0], difference_order[1]})}});
        require(candidate.geometry == difference_baseline.geometry &&
                    candidate.lineage == difference_baseline.lineage,
                "same-stage difference operand permutation changed geometry or lineage");
        ++difference_permutations;
    } while (std::next_permutation(difference_order.begin(), difference_order.end()));
    require(difference_permutations == 2 && difference_baseline.signed_double_area == 4 &&
                difference_baseline.regions == 2,
            "same-stage difference independent disconnected-result oracle changed");

    const Scenario union_split =
        run_scenario(union_rectangles, {{10, ExactBooleanStageOperation::union_, operands({1})},
                                        {20, ExactBooleanStageOperation::union_, operands({2})},
                                        {30, ExactBooleanStageOperation::union_, operands({3})}});
    require(union_split.geometry == union_baseline.geometry,
            "equivalent union stage splitting changed normalized geometry");
    const Scenario difference_split = run_scenario(
        difference_rectangles, {{10, ExactBooleanStageOperation::union_, operands({1})},
                                {20, ExactBooleanStageOperation::difference, operands({2})},
                                {30, ExactBooleanStageOperation::difference, operands({3})}});
    require(difference_split.geometry == difference_baseline.geometry,
            "equivalent difference stage splitting changed normalized geometry");

    const Scenario ordered = run_scenario(
        {full, lower_left}, {{10, ExactBooleanStageOperation::union_, operands({1})},
                             {20, ExactBooleanStageOperation::difference, operands({2})}});
    const Scenario swapped = run_scenario(
        {full, lower_left}, {{20, ExactBooleanStageOperation::difference, operands({2})},
                             {10, ExactBooleanStageOperation::union_, operands({1})}});
    require(ordered.geometry != swapped.geometry && ordered.signed_double_area == 6 &&
                swapped.signed_double_area == 8,
            "ordered-stage-swap mutation sentinel was not detected");

    std::cout << "EXACT_BOOLEAN_METAMORPHIC=union_permutations:" << union_permutations << ':'
              << union_baseline.geometry << ':' << union_baseline.lineage
              << "|difference_permutations:" << difference_permutations << ':'
              << difference_baseline.geometry << ':' << difference_baseline.lineage
              << "|split:union,difference|mutation:ordered_stage_swap\n";
    return 0;
}

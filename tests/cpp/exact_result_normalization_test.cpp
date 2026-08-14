#include "geometer/exact_result_normalization.h"

#include <cstdlib>
#include <iostream>
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

ConstructionNodeId rational(ConstructionArena& arena, const BigInt& numerator,
                            const BigInt& denominator = 1)
{
    auto result = arena.make_rational(numerator, denominator);
    require(result.error == Error::none && result.node, "normalization rational failed");
    return *result.node;
}

ExactPoint point(ConstructionArena& arena, const BigInt& x_numerator, const BigInt& x_denominator,
                 const BigInt& y_numerator, const BigInt& y_denominator)
{
    return {rational(arena, x_numerator, x_denominator),
            rational(arena, y_numerator, y_denominator)};
}

void append_rectangle(ConstructionArena& arena, const ExactPoint& a, const ExactPoint& b,
                      const ExactPoint& c, const ExactPoint& d, std::uint64_t occurrence_base,
                      std::vector<ExactAtomicCurve>& curves,
                      std::vector<ExactCoverageOccurrence>& coverages)
{
    curves.push_back(
        {ExactAtomicCurveKind::line, a, b, {}, true, false, {{occurrence_base, true}}});
    curves.push_back(
        {ExactAtomicCurveKind::line, b, c, {}, true, false, {{occurrence_base + 1, true}}});
    curves.push_back(
        {ExactAtomicCurveKind::line, d, c, {}, true, false, {{occurrence_base + 2, true}}});
    curves.push_back(
        {ExactAtomicCurveKind::line, a, d, {}, true, false, {{occurrence_base + 3, true}}});
    coverages.push_back({occurrence_base, 10, true});
    coverages.push_back({occurrence_base + 1, 10, true});
    coverages.push_back({occurrence_base + 2, 10, false});
    coverages.push_back({occurrence_base + 3, 10, false});
}

struct Pipeline
{
    ExactArrangementResult arrangement;
    ExactBooleanSelectionResult selection;
    ExactBooleanRegionsResult regions;
};

Pipeline build_pipeline(ConstructionArena& arena, const std::vector<ExactAtomicCurve>& curves,
                        const std::vector<ExactCoverageOccurrence>& coverages)
{
    Pipeline pipeline;
    pipeline.arrangement = build_exact_arrangement(arena, curves, coverages);
    require(pipeline.arrangement.error == Error::none && pipeline.arrangement.value,
            "normalization arrangement failed");
    const std::vector<ExactBooleanStage> stages{
        {1, ExactBooleanStageOperation::union_, {{10, 1000}}},
    };
    pipeline.selection =
        evaluate_exact_boolean_stages(arena.budget(), *pipeline.arrangement.value, stages);
    require(pipeline.selection.error == Error::none && pipeline.selection.value,
            "normalization selection failed");
    pipeline.regions = build_exact_boolean_regions(arena.budget(), *pipeline.arrangement.value,
                                                   *pipeline.selection.value);
    require(pipeline.regions.error == Error::none && pipeline.regions.value,
            "normalization regions failed");
    return pipeline;
}

std::string signature(const ExactNormalizedBooleanResult& result)
{
    std::ostringstream out;
    out << "v";
    for (const auto& vertex : result.vertices())
        out << vertex.x_nm << ',' << vertex.y_nm << ',' << vertex.arrangement_vertex << ';';
    out << "f";
    for (const auto& fragment : result.fragments())
        out << fragment.start_vertex << ',' << fragment.end_vertex << ','
            << static_cast<unsigned>(fragment.kind) << ','
            << static_cast<unsigned>(fragment.direction) << ',' << fragment.major_arc << ','
            << fragment.radius_nm << ',' << fragment.arrangement_half_edge << ';';
    out << "r";
    for (const auto& ring : result.rings())
        out << ring.fragment_begin << ',' << ring.fragment_count << ',' << ring.parent_ring << ','
            << ring.depth << ',' << ring.counterclockwise << ',' << ring.exact_ring << ';';
    out << "g";
    for (const auto& region : result.regions())
        out << region.outer_ring << ',' << region.exact_region << ';';
    return out.str();
}

} // namespace

int main()
{
    Budget empty_budget({2'000'000'000, 268'435'456});
    ConstructionArena empty_arena(empty_budget);
    const std::vector<ExactAtomicCurve> empty_curves;
    const std::vector<ExactCoverageOccurrence> empty_coverages;
    Pipeline empty = build_pipeline(empty_arena, empty_curves, empty_coverages);
    ExactNormalizedBooleanResultResult normalized_empty = normalize_exact_boolean_result(
        empty_arena, *empty.arrangement.value, *empty.selection.value, *empty.regions.value);
    require(normalized_empty.error == ExactResultNormalizationError::none &&
                normalized_empty.value && normalized_empty.value->vertices().empty() &&
                normalized_empty.value->fragments().empty() &&
                normalized_empty.value->rings().empty() &&
                normalized_empty.value->regions().empty(),
            "successful empty result normalization failed");

    Budget box_budget({2'000'000'000, 268'435'456});
    ConstructionArena box_arena(box_budget);
    std::vector<ExactAtomicCurve> box_curves;
    std::vector<ExactCoverageOccurrence> box_coverages;
    append_rectangle(box_arena, point(box_arena, 0, 1, 0, 1), point(box_arena, 12, 1, 0, 1),
                     point(box_arena, 12, 1, 12, 1), point(box_arena, 0, 1, 12, 1), 100, box_curves,
                     box_coverages);
    Pipeline box = build_pipeline(box_arena, box_curves, box_coverages);
    ExactNormalizedBooleanResultResult normalized_box = normalize_exact_boolean_result(
        box_arena, *box.arrangement.value, *box.selection.value, *box.regions.value);
    require(normalized_box.error == ExactResultNormalizationError::none && normalized_box.value,
            "integer box normalization failed");
    require(normalized_box.value->vertices().size() == 4 &&
                normalized_box.value->fragments().size() == 4 &&
                normalized_box.value->rings().size() == 1 &&
                normalized_box.value->regions().size() == 1,
            "integer box normalization topology changed");
    const BudgetUsage box_usage = box_budget.usage();
    Budget short_work_budget({box_usage.work_units - 1, 268'435'456});
    ConstructionArena short_work_arena(short_work_budget);
    std::vector<ExactAtomicCurve> short_work_curves;
    std::vector<ExactCoverageOccurrence> short_work_coverages;
    append_rectangle(short_work_arena, point(short_work_arena, 0, 1, 0, 1),
                     point(short_work_arena, 12, 1, 0, 1), point(short_work_arena, 12, 1, 12, 1),
                     point(short_work_arena, 0, 1, 12, 1), 100, short_work_curves,
                     short_work_coverages);
    Pipeline short_work = build_pipeline(short_work_arena, short_work_curves, short_work_coverages);
    const std::uint64_t retained_before_short = short_work_budget.usage().owned_bytes;
    ExactNormalizedBooleanResultResult short_work_result =
        normalize_exact_boolean_result(short_work_arena, *short_work.arrangement.value,
                                       *short_work.selection.value, *short_work.regions.value);
    require(short_work_result.error == ExactResultNormalizationError::resource_limit_exceeded &&
                !short_work_result.value &&
                short_work_budget.usage().owned_bytes == retained_before_short,
            "one-unit-short result normalization must fail without retained storage");

    Budget circle_budget({2'000'000'000, 268'435'456});
    ConstructionArena circle_arena(circle_budget);
    const ExactPoint center = point(circle_arena, 0, 1, 0, 1);
    const ExactPoint left = point(circle_arena, -5, 1, 0, 1);
    const ExactPoint right = point(circle_arena, 5, 1, 0, 1);
    const ConstructionNodeId radius = rational(circle_arena, 5);
    std::vector<ExactAtomicCurve> circle_curves{
        {ExactAtomicCurveKind::circular_arc,
         left,
         right,
         {center, radius},
         true,
         false,
         {{200, true}}},
        {ExactAtomicCurveKind::circular_arc,
         right,
         left,
         {center, radius},
         true,
         false,
         {{201, true}}},
    };
    const std::vector<ExactCoverageOccurrence> circle_coverages{
        {200, 10, true},
        {201, 10, true},
    };
    Pipeline circle = build_pipeline(circle_arena, circle_curves, circle_coverages);
    ExactNormalizedBooleanResultResult normalized_circle = normalize_exact_boolean_result(
        circle_arena, *circle.arrangement.value, *circle.selection.value, *circle.regions.value);
    require(normalized_circle.error == ExactResultNormalizationError::none &&
                normalized_circle.value && normalized_circle.value->fragments().size() == 2,
            "two-half-arc circle normalization failed");
    for (const auto& fragment : normalized_circle.value->fragments())
        require(fragment.kind == ExactAtomicCurveKind::circular_arc && fragment.radius_nm == 5 &&
                    !fragment.major_arc,
                "normalized half-circle replay is incoherent");

    Budget moved_arc_budget({2'000'000'000, 268'435'456});
    ConstructionArena moved_arc_arena(moved_arc_budget);
    const ExactPoint moved_center = point(moved_arc_arena, 1, 4, 1, 4);
    const ExactPoint moved_left = point(moved_arc_arena, -19, 4, 1, 4);
    const ExactPoint moved_right = point(moved_arc_arena, 21, 4, 1, 4);
    const ConstructionNodeId moved_radius = rational(moved_arc_arena, 5);
    std::vector<ExactAtomicCurve> moved_arc_curves{
        {ExactAtomicCurveKind::circular_arc,
         moved_left,
         moved_right,
         {moved_center, moved_radius},
         true,
         false,
         {{250, true}}},
        {ExactAtomicCurveKind::circular_arc,
         moved_right,
         moved_left,
         {moved_center, moved_radius},
         true,
         false,
         {{251, true}}},
    };
    const std::vector<ExactCoverageOccurrence> moved_arc_coverages{
        {250, 10, true},
        {251, 10, true},
    };
    Pipeline moved_arc = build_pipeline(moved_arc_arena, moved_arc_curves, moved_arc_coverages);
    ExactNormalizedBooleanResultResult moved_arc_result =
        normalize_exact_boolean_result(moved_arc_arena, *moved_arc.arrangement.value,
                                       *moved_arc.selection.value, *moved_arc.regions.value);
    require(moved_arc_result.error == ExactResultNormalizationError::normalization_error_exceeded &&
                !moved_arc_result.value,
            "uncertified moved-arc replay must fail closed");

    Budget collapse_budget({2'000'000'000, 268'435'456});
    ConstructionArena collapse_arena(collapse_budget);
    std::vector<ExactAtomicCurve> collapse_curves;
    std::vector<ExactCoverageOccurrence> collapse_coverages;
    append_rectangle(collapse_arena, point(collapse_arena, 5, 4, 0, 1),
                     point(collapse_arena, 7, 5, 0, 1), point(collapse_arena, 7, 5, 10, 1),
                     point(collapse_arena, 5, 4, 10, 1), 300, collapse_curves, collapse_coverages);
    Pipeline collapse = build_pipeline(collapse_arena, collapse_curves, collapse_coverages);
    ExactNormalizedBooleanResultResult collapsed =
        normalize_exact_boolean_result(collapse_arena, *collapse.arrangement.value,
                                       *collapse.selection.value, *collapse.regions.value);
    require(collapsed.error == ExactResultNormalizationError::normalization_topology_collapse &&
                !collapsed.value,
            "distinct vertices sharing one nm representative must fail closed");

    std::cout << "EXACT_RESULT_NORMALIZATION_VECTOR=ERN1:" << signature(*normalized_empty.value)
              << '|' << signature(*normalized_box.value) << '|'
              << signature(*normalized_circle.value) << '\n';
    return 0;
}

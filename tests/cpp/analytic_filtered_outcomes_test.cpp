#include "geometer/analytic_curve_broad_phase.h"
#include "geometer/analytic_filtered_outcomes.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
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

AnalyticFilteredPointNm exact_point(double x, double y)
{
    return {{x, x}, {y, y}};
}

struct StageSpec
{
    std::uint8_t operation = 1;
    std::vector<std::uint64_t> operands;
};

AnalyticRequestPacketRecords records_for(const std::vector<StageSpec>& specs)
{
    AnalyticRequestPacketRecords records;
    records.jobs.push_back({1, 0, static_cast<std::uint32_t>(specs.size())});
    for (std::uint32_t stage = 0; stage < specs.size(); ++stage)
    {
        const std::uint32_t begin = static_cast<std::uint32_t>(records.operands.size());
        for (const std::uint64_t operand : specs[stage].operands)
            records.operands.push_back({operand, 2, 0});
        records.stages.push_back({stage + 1, specs[stage].operation, begin,
                                  static_cast<std::uint32_t>(specs[stage].operands.size())});
    }
    return records;
}

void append_line(AnalyticFilteredGeometry& geometry, std::uint64_t operand, double x1, double y1,
                 double x2, double y2)
{
    const std::uint32_t index = static_cast<std::uint32_t>(geometry.curves.size() + 1);
    const double dx = x2 - x1;
    const double dy = y2 - y1;
    const bool agrees = dx > 0.0 || (dx == 0.0 && dy > 0.0);
    AnalyticAtomicCurveNm curve;
    curve.curve_index = index;
    curve.start = exact_point(x1, y1);
    curve.end = exact_point(x2, y2);
    curve.construction_carrier_id = 1000 + index;
    curve.construction_family_id = 2000 + index;
    curve.has_construction_line_direction = true;
    curve.construction_line_dx = static_cast<std::int64_t>(agrees ? dx : -dx);
    curve.construction_line_dy = static_cast<std::int64_t>(agrees ? dy : -dy);
    if (curve.construction_line_dx == 0)
    {
        const std::uint64_t column =
            analytic_vertical_x_column_token(curve.construction_carrier_id);
        curve.start.construction_x_column_id = column;
        curve.end.construction_x_column_id = column;
    }
    geometry.curves.push_back(curve);
    geometry.bounds.push_back(
        {index, std::min(x1, x2), std::min(y1, y2), std::max(x1, x2), std::max(y1, y2)});
    AnalyticFilteredOccurrence occurrence;
    occurrence.occurrence_id = index;
    occurrence.coverage_id = operand;
    occurrence.agrees_with_carrier = agrees;
    occurrence.material_on_left = true;
    occurrence.source.kind = AnalyticFilteredSourceKind::authored_segment_curve;
    occurrence.source.role = AnalyticFilteredSourceRole::authored_line;
    occurrence.source.operand_id = operand;
    occurrence.source.primary_id = 10000 + index;
    occurrence.source.secondary_id = 20000 + index;
    geometry.occurrences.push_back(occurrence);
}

void append_box(AnalyticFilteredGeometry& geometry, std::uint64_t operand, double min_x,
                double min_y, double max_x, double max_y)
{
    append_line(geometry, operand, min_x, min_y, max_x, min_y);
    append_line(geometry, operand, max_x, min_y, max_x, max_y);
    append_line(geometry, operand, max_x, max_y, min_x, max_y);
    append_line(geometry, operand, min_x, max_y, min_x, min_y);
}

void share_box_carriers(AnalyticFilteredGeometry& geometry, std::uint32_t authority,
                        std::uint32_t target)
{
    for (std::uint32_t side = 0; side < 4; ++side)
    {
        geometry.curves[target + side].construction_carrier_id =
            geometry.curves[authority + side].construction_carrier_id;
        geometry.curves[target + side].construction_family_id =
            geometry.curves[authority + side].construction_family_id;
        geometry.curves[target + side].start.construction_x_column_id =
            geometry.curves[authority + side].start.construction_x_column_id;
        geometry.curves[target + side].end.construction_x_column_id =
            geometry.curves[authority + side].end.construction_x_column_id;
    }
}

AnalyticFilteredOutcomesResult build(const AnalyticRequestPacketRecords& records,
                                     const AnalyticFilteredGeometry& geometry,
                                     const AnalyticSolverLimits& limits = {})
{
    const AnalyticBroadPhaseResult broad = build_analytic_curve_candidates(geometry.bounds, limits);
    require(broad.error == AnalyticBroadPhaseError::none, "outcome broad phase failed");
    return build_analytic_filtered_outcomes(records, 0, geometry, broad.pairs, limits);
}

bool has(const AnalyticFilteredOutcomesResult& result, std::uint64_t operand,
         AnalyticOperandOutcomeKind kind)
{
    return std::any_of(result.events.begin(), result.events.end(), [&](const auto& event)
                       { return event.operand_id == operand && event.kind == kind; });
}

const AnalyticFilteredOperandOutcomeEvent& event(const AnalyticFilteredOutcomesResult& result,
                                                 std::uint64_t operand,
                                                 AnalyticOperandOutcomeKind kind)
{
    const auto found =
        std::find_if(result.events.begin(), result.events.end(), [&](const auto& value)
                     { return value.operand_id == operand && value.kind == kind; });
    require(found != result.events.end(), "required outcome event missing");
    return *found;
}

void test_single_and_coincident_union()
{
    AnalyticFilteredGeometry single_geometry;
    append_box(single_geometry, 20, 0, 0, 1000, 1000);
    const auto single = build(records_for({{1, {20}}}), single_geometry);
    require(single.error == AnalyticFilteredOutcomesError::none, "single outcome failed");
    require(single.events.size() == 1 &&
                has(single, 20, AnalyticOperandOutcomeKind::contributes_final_material),
            "single union outcome drifted");
    const auto& contribution =
        event(single, 20, AnalyticOperandOutcomeKind::contributes_final_material);
    require(contribution.result_references.count == 1 && contribution.sources.count == 4,
            "single union references or complete source set drifted");

    AnalyticFilteredGeometry coincident_geometry;
    append_box(coincident_geometry, 10, 0, 0, 1000, 1000);
    append_box(coincident_geometry, 20, 0, 0, 1000, 1000);
    share_box_carriers(coincident_geometry, 0, 4);
    const auto coincident = build(records_for({{1, {10, 20}}}), coincident_geometry);
    require(coincident.error == AnalyticFilteredOutcomesError::none,
            "coincident union outcomes failed");
    for (const std::uint64_t operand : {10ULL, 20ULL})
        require(has(coincident, operand, AnalyticOperandOutcomeKind::contributes_final_material) &&
                    has(coincident, operand,
                        AnalyticOperandOutcomeKind::redundant_or_absorbed_coverage),
                "same-stage coincident union lost symmetric outcome evidence");
}

void test_remove_refill_history()
{
    AnalyticFilteredGeometry geometry;
    append_box(geometry, 40, 0, 0, 1000, 1000);
    append_box(geometry, 10, 200, 200, 800, 800);
    append_box(geometry, 30, 300, 300, 700, 700);
    const auto result = build(records_for({{1, {40}}, {2, {10}}, {1, {30}}}), geometry);
    require(result.error == AnalyticFilteredOutcomesError::none, "remove/refill outcomes failed");
    require(has(result, 40, AnalyticOperandOutcomeKind::contributes_final_material) &&
                has(result, 40, AnalyticOperandOutcomeKind::partially_removed_later),
            "partially retained positive history drifted");
    require(has(result, 10, AnalyticOperandOutcomeKind::subtraction_effect_survives) &&
                has(result, 10, AnalyticOperandOutcomeKind::subtraction_effect_overwritten_later),
            "surviving/overwritten subtraction coexistence drifted");
    require(has(result, 30, AnalyticOperandOutcomeKind::contributes_final_material),
            "refill contribution missing");
    require(event(result, 10, AnalyticOperandOutcomeKind::subtraction_effect_survives)
                    .result_references.count >= 2,
            "surviving subtraction lost tagged ring/region handles");
}

void test_accumulator_redundancy_and_same_stage_subtractors()
{
    AnalyticFilteredGeometry redundancy;
    append_box(redundancy, 10, 0, 0, 1000, 1000);
    append_box(redundancy, 20, 200, 200, 800, 800);
    const auto redundant = build(records_for({{1, {10}}, {1, {20}}}), redundancy);
    require(redundant.error == AnalyticFilteredOutcomesError::none &&
                has(redundant, 20, AnalyticOperandOutcomeKind::redundant_or_absorbed_coverage) &&
                has(redundant, 20, AnalyticOperandOutcomeKind::contributes_final_material),
            "pre-existing accumulator redundancy drifted");

    AnalyticFilteredGeometry subtraction;
    append_box(subtraction, 40, 0, 0, 1000, 1000);
    append_box(subtraction, 10, 150, 150, 850, 850);
    append_box(subtraction, 20, 250, 250, 750, 750);
    append_box(subtraction, 30, 300, 300, 700, 700);
    const auto result = build(records_for({{1, {40}}, {2, {10, 20}}, {1, {30}}}), subtraction);
    require(result.error == AnalyticFilteredOutcomesError::none,
            "same-stage subtractor outcomes failed");
    for (const std::uint64_t operand : {10ULL, 20ULL})
        require(has(result, operand, AnalyticOperandOutcomeKind::subtraction_effect_survives) &&
                    has(result, operand,
                        AnalyticOperandOutcomeKind::subtraction_effect_overwritten_later),
                "same-stage subtractors lost symmetric independent credit");
}

void test_complete_removal_and_no_effect()
{
    AnalyticFilteredGeometry removed_geometry;
    append_box(removed_geometry, 100, 0, 0, 1000, 1000);
    append_box(removed_geometry, 200, 0, 0, 1000, 1000);
    share_box_carriers(removed_geometry, 0, 4);
    const auto removed = build(records_for({{1, {100}}, {2, {200}}}), removed_geometry);
    require(removed.error == AnalyticFilteredOutcomesError::none, "complete removal failed");
    require(has(removed, 100, AnalyticOperandOutcomeKind::completely_removed_later),
            "completely removed positive missing");
    require(has(removed, 200, AnalyticOperandOutcomeKind::subtraction_effect_survives),
            "complete subtraction effect missing");
    require(event(removed, 200, AnalyticOperandOutcomeKind::subtraction_effect_survives)
                    .result_references.count == 0,
            "complete removal invented final topology references");

    AnalyticFilteredGeometry outside_geometry;
    append_box(outside_geometry, 10, 0, 0, 1000, 1000);
    append_box(outside_geometry, 20, 2000, 2000, 3000, 3000);
    const auto outside = build(records_for({{1, {10}}, {2, {20}}}), outside_geometry);
    require(outside.error == AnalyticFilteredOutcomesError::none &&
                has(outside, 20, AnalyticOperandOutcomeKind::no_effect),
            "outside difference was not classified no-effect");
}

void test_remove_refill_remove_and_collapsed_no_effect()
{
    AnalyticFilteredGeometry geometry;
    append_box(geometry, 40, 0, 0, 1000, 1000);
    append_box(geometry, 10, 200, 200, 800, 800);
    append_box(geometry, 30, 250, 250, 750, 750);
    append_box(geometry, 20, 400, 400, 600, 600);
    const auto result = build(records_for({{1, {40}}, {2, {10}}, {1, {30}}, {2, {20}}}), geometry);
    require(result.error == AnalyticFilteredOutcomesError::none,
            "remove/refill/remove outcomes failed");
    require(has(result, 10, AnalyticOperandOutcomeKind::subtraction_effect_survives) &&
                has(result, 10, AnalyticOperandOutcomeKind::subtraction_effect_overwritten_later) &&
                has(result, 20, AnalyticOperandOutcomeKind::subtraction_effect_survives) &&
                !has(result, 20, AnalyticOperandOutcomeKind::subtraction_effect_overwritten_later),
            "active removal epochs were composed or overwritten incorrectly");

    AnalyticFilteredGeometry collapsed;
    append_line(collapsed, 1, 0, 0, 20, 20);
    const auto collapsed_result = build(records_for({{1, {1}}}), collapsed);
    require(collapsed_result.error == AnalyticFilteredOutcomesError::none &&
                collapsed_result.lineage.regions.selection.arrangement.collapsed_spans.size() ==
                    1 &&
                has(collapsed_result, 1, AnalyticOperandOutcomeKind::no_effect),
            "collapsed-only operand manufactured positive-area effect");
}

AnalyticFilteredOutcomesResult build_disjoint(std::uint32_t count,
                                              const AnalyticSolverLimits& limits = {})
{
    AnalyticFilteredGeometry geometry;
    std::vector<std::uint64_t> operands;
    operands.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index)
    {
        const std::uint64_t operand = index + 1;
        operands.push_back(operand);
        const double x = index * 2000.0;
        append_box(geometry, operand, x, 0, x + 1000, 1000);
    }
    return build(records_for({{1, operands}}), geometry, limits);
}

AnalyticFilteredOutcomesResult
build_disconnected_one_operand(std::uint32_t count, const AnalyticSolverLimits& limits = {})
{
    AnalyticFilteredGeometry geometry;
    for (std::uint32_t index = 0; index < count; ++index)
    {
        const double x = index * 2000.0;
        append_box(geometry, 1, x, 0, x + 1000, 1000);
    }
    return build(records_for({{1, {1}}}), geometry, limits);
}

AnalyticFilteredOutcomesResult build_empty_stages(std::uint32_t count,
                                                  const AnalyticSolverLimits& limits = {})
{
    std::vector<StageSpec> stages(count);
    for (std::uint32_t index = 0; index < count; ++index)
        stages[index].operation = index % 2 == 0 ? 1 : 2;
    return build(records_for(stages), {}, limits);
}

AnalyticFilteredOutcomesResult build_coincident(std::uint32_t count,
                                                const AnalyticSolverLimits& limits = {})
{
    AnalyticFilteredGeometry geometry;
    std::vector<std::uint64_t> operands;
    operands.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index)
    {
        operands.push_back(index + 1);
        const std::uint32_t begin = static_cast<std::uint32_t>(geometry.curves.size());
        append_box(geometry, index + 1, 0, 0, 1000, 1000);
        if (index != 0)
            share_box_carriers(geometry, 0, begin);
    }
    return build(records_for({{1, operands}}), geometry, limits);
}

AnalyticFilteredOutcomesResult build_duplicate_subtraction(std::uint32_t copies,
                                                           const AnalyticSolverLimits& limits = {})
{
    AnalyticFilteredGeometry geometry;
    append_box(geometry, 1, 0, 0, 2000, 2000);
    std::uint32_t authority = 0;
    for (std::uint32_t index = 0; index < copies; ++index)
    {
        const std::uint32_t begin = static_cast<std::uint32_t>(geometry.curves.size());
        append_box(geometry, 2, 500, 500, 1500, 1500);
        if (index == 0)
            authority = begin;
        else
            share_box_carriers(geometry, authority, begin);
    }
    return build(records_for({{1, {1}}, {2, {2}}}), geometry, limits);
}

void test_dense_stage_and_exact_reference_counting()
{
    const auto dense = build_coincident(16);
    require(dense.error == AnalyticFilteredOutcomesError::none,
            "dense same-stage outcome tracker failed");
    for (std::uint64_t operand = 1; operand <= 16; ++operand)
        require(has(dense, operand, AnalyticOperandOutcomeKind::contributes_final_material) &&
                    has(dense, operand, AnalyticOperandOutcomeKind::redundant_or_absorbed_coverage),
                "dense same-stage batch lost symmetric evidence");

    const auto duplicate = build_duplicate_subtraction(12);
    require(duplicate.error == AnalyticFilteredOutcomesError::none,
            "duplicate subtraction outcome fixture failed");
    const auto& removal =
        event(duplicate, 2, AnalyticOperandOutcomeKind::subtraction_effect_survives);
    require(removal.result_references.count == 2 && duplicate.result_references.size() == 3,
            "duplicate subtraction fragments inflated exact tagged references");
    const auto single_copy = build_duplicate_subtraction(1);
    require(single_copy.error == AnalyticFilteredOutcomesError::none &&
                single_copy.telemetry.lineage_source_visits ==
                    duplicate.telemetry.lineage_source_visits,
            "duplicate authored sources inflated topology-reference projection work");

    AnalyticSolverLimits limits;
    std::uint64_t low = 0;
    std::uint64_t high = limits.working_memory_bytes;
    while (low < high)
    {
        const std::uint64_t middle = low + (high - low) / 2;
        auto probe = limits;
        probe.working_memory_bytes = middle;
        if (build_coincident(8, probe).error == AnalyticFilteredOutcomesError::none)
            high = middle;
        else
            low = middle + 1;
    }
    auto exact_memory = limits;
    exact_memory.working_memory_bytes = low;
    require(build_coincident(8, exact_memory).error == AnalyticFilteredOutcomesError::none,
            "dense same-stage tracker failed at exact logical memory");
    --exact_memory.working_memory_bytes;
    const auto short_memory = build_coincident(8, exact_memory);
    require(short_memory.error == AnalyticFilteredOutcomesError::resource_limit_exceeded &&
                short_memory.events.empty() && short_memory.result_references.empty() &&
                short_memory.source_references.empty(),
            "one-byte-short dense stage memory leaked outcome publication");

    low = 0;
    high = limits.predicate_calls;
    while (low < high)
    {
        const std::uint64_t middle = low + (high - low) / 2;
        auto probe = limits;
        probe.predicate_calls = middle;
        if (build_duplicate_subtraction(8, probe).error == AnalyticFilteredOutcomesError::none)
            high = middle;
        else
            low = middle + 1;
    }
    auto exact_work = limits;
    exact_work.predicate_calls = low;
    require(build_duplicate_subtraction(8, exact_work).error == AnalyticFilteredOutcomesError::none,
            "duplicate-reference publication failed at exact governed work");
    --exact_work.predicate_calls;
    const auto short_work = build_duplicate_subtraction(8, exact_work);
    require(short_work.error == AnalyticFilteredOutcomesError::resource_limit_exceeded &&
                short_work.events.empty() && short_work.result_references.empty() &&
                short_work.source_references.empty(),
            "one-unit-short duplicate publication leaked output");
}

void test_output_association_scaling()
{
    const auto small = build_disconnected_one_operand(8);
    const auto large = build_disconnected_one_operand(16);
    require(small.error == AnalyticFilteredOutcomesError::none &&
                large.error == AnalyticFilteredOutcomesError::none,
            "disconnected single-operand outcome fixture failed");
    require(small.result_references.size() == 8 && large.result_references.size() == 16,
            "disconnected operand did not retain one reference per region");
    require(small.telemetry.lineage_source_visits == 16 &&
                large.telemetry.lineage_source_visits == 32,
            "count/fill topology-reference visits did not track emitted associations");
    require(large.telemetry.predicate_calls < small.telemetry.predicate_calls * 3,
            "topology-reference association work was not output-linear");
    require(large.telemetry.peak_working_memory_bytes <
                small.telemetry.peak_working_memory_bytes * 3,
            "topology-reference association storage was not output-linear: " +
                std::to_string(small.telemetry.peak_working_memory_bytes) + "/" +
                std::to_string(large.telemetry.peak_working_memory_bytes));

    AnalyticSolverLimits limits;
    std::uint64_t low = 0;
    std::uint64_t high = limits.predicate_calls;
    while (low < high)
    {
        const std::uint64_t middle = low + (high - low) / 2;
        auto probe = limits;
        probe.predicate_calls = middle;
        if (build_disconnected_one_operand(8, probe).error == AnalyticFilteredOutcomesError::none)
            high = middle;
        else
            low = middle + 1;
    }
    auto exact_work = limits;
    exact_work.predicate_calls = low;
    require(build_disconnected_one_operand(8, exact_work).error ==
                AnalyticFilteredOutcomesError::none,
            "association-heavy publication failed at exact governed work");
    --exact_work.predicate_calls;
    const auto short_work = build_disconnected_one_operand(8, exact_work);
    require(short_work.error == AnalyticFilteredOutcomesError::resource_limit_exceeded &&
                short_work.events.empty() && short_work.result_references.empty() &&
                short_work.source_references.empty(),
            "one-unit-short association publication leaked output");

    low = 0;
    high = limits.working_memory_bytes;
    while (low < high)
    {
        const std::uint64_t middle = low + (high - low) / 2;
        auto probe = limits;
        probe.working_memory_bytes = middle;
        if (build_disconnected_one_operand(8, probe).error == AnalyticFilteredOutcomesError::none)
            high = middle;
        else
            low = middle + 1;
    }
    auto exact_memory = limits;
    exact_memory.working_memory_bytes = low;
    require(build_disconnected_one_operand(8, exact_memory).error ==
                AnalyticFilteredOutcomesError::none,
            "association-heavy publication failed at exact logical memory");
    --exact_memory.working_memory_bytes;
    const auto short_memory = build_disconnected_one_operand(8, exact_memory);
    require(short_memory.error == AnalyticFilteredOutcomesError::resource_limit_exceeded &&
                short_memory.events.empty() && short_memory.result_references.empty() &&
                short_memory.source_references.empty(),
            "one-byte-short association publication leaked output");
}

void test_empty_stage_work_admission()
{
    constexpr std::uint32_t kStages = 1024;
    const auto baseline = build_empty_stages(kStages);
    require(baseline.error == AnalyticFilteredOutcomesError::none && baseline.events.empty(),
            "zero-operand stage outcome fixture failed");

    AnalyticSolverLimits limits;
    std::uint64_t low = 0;
    std::uint64_t high = limits.predicate_calls;
    while (low < high)
    {
        const std::uint64_t middle = low + (high - low) / 2;
        auto probe = limits;
        probe.predicate_calls = middle;
        if (build_empty_stages(kStages, probe).error == AnalyticFilteredOutcomesError::none)
            high = middle;
        else
            low = middle + 1;
    }
    auto exact = limits;
    exact.predicate_calls = low;
    require(build_empty_stages(kStages, exact).error == AnalyticFilteredOutcomesError::none,
            "zero-operand stages failed at exact governed work");
    --exact.predicate_calls;
    const auto short_result = build_empty_stages(kStages, exact);
    require(short_result.error == AnalyticFilteredOutcomesError::resource_limit_exceeded &&
                short_result.telemetry.arrangement_work_units == 0 && short_result.events.empty(),
            "one-short zero-operand stage work reached arrangement");
}

void test_governance_and_sparse_scaling()
{
    const auto small = build_disjoint(16);
    const auto large = build_disjoint(32);
    require(small.error == AnalyticFilteredOutcomesError::none &&
                large.error == AnalyticFilteredOutcomesError::none,
            "disjoint outcome scaling fixture failed");
    require(large.telemetry.predicate_calls < small.telemetry.predicate_calls * 3 &&
                large.telemetry.peak_working_memory_bytes <
                    small.telemetry.peak_working_memory_bytes * 3 &&
                large.lineage.regions.selection.telemetry.outcome_evidence_flags_set == 32,
            "filtered outcomes exceeded 3x work/memory or evidence at 2x sparse input");

    AnalyticSolverLimits limits;
    std::uint64_t low = 0;
    std::uint64_t high = limits.predicate_calls;
    while (low < high)
    {
        const std::uint64_t middle = low + (high - low) / 2;
        auto probe = limits;
        probe.predicate_calls = middle;
        if (build_disjoint(8, probe).error == AnalyticFilteredOutcomesError::none)
            high = middle;
        else
            low = middle + 1;
    }
    auto exact_work = limits;
    exact_work.predicate_calls = low;
    require(build_disjoint(8, exact_work).error == AnalyticFilteredOutcomesError::none,
            "exact outcome work boundary failed");
    --exact_work.predicate_calls;
    const auto short_work = build_disjoint(8, exact_work);
    require(short_work.error == AnalyticFilteredOutcomesError::resource_limit_exceeded &&
                short_work.events.empty() && short_work.result_references.empty() &&
                short_work.source_references.empty() && short_work.lineage.boundaries.empty() &&
                short_work.lineage.region_lineage.empty(),
            "one-unit-short outcome work leaked partial publication");

    low = 0;
    high = limits.working_memory_bytes;
    while (low < high)
    {
        const std::uint64_t middle = low + (high - low) / 2;
        auto probe = limits;
        probe.working_memory_bytes = middle;
        if (build_disjoint(8, probe).error == AnalyticFilteredOutcomesError::none)
            high = middle;
        else
            low = middle + 1;
    }
    auto exact_memory = limits;
    exact_memory.working_memory_bytes = low;
    const auto exact_memory_result = build_disjoint(8, exact_memory);
    require(exact_memory_result.error == AnalyticFilteredOutcomesError::none &&
                exact_memory_result.telemetry.peak_working_memory_bytes == low,
            "exact outcome memory boundary failed");
    --exact_memory.working_memory_bytes;
    const auto short_memory = build_disjoint(8, exact_memory);
    require(short_memory.error == AnalyticFilteredOutcomesError::resource_limit_exceeded &&
                short_memory.events.empty() && short_memory.result_references.empty() &&
                short_memory.source_references.empty() && short_memory.lineage.boundaries.empty() &&
                short_memory.lineage.region_lineage.empty(),
            "one-byte-short outcome memory leaked partial publication");

    low = 0;
    high = limits.working_memory_bytes;
    while (low < high)
    {
        const std::uint64_t middle = low + (high - low) / 2;
        auto probe = limits;
        probe.working_memory_bytes = middle;
        const auto value = build_disjoint(8, probe);
        if (value.telemetry.arrangement_work_units != 0)
            high = middle;
        else
            low = middle + 1;
    }
    require(low != 0, "outcome admission memory threshold was not found");
    auto before_arrangement = limits;
    before_arrangement.working_memory_bytes = low - 1;
    const auto rejected = build_disjoint(8, before_arrangement);
    require(rejected.error == AnalyticFilteredOutcomesError::resource_limit_exceeded &&
                rejected.telemetry.arrangement_work_units == 0 && rejected.events.empty(),
            "known-impossible outcome history memory reached arrangement");
}

void test_malformed_source_fails_before_arrangement()
{
    AnalyticFilteredGeometry geometry;
    append_box(geometry, 1, 0, 0, 1000, 1000);
    geometry.occurrences[0].source.kind = AnalyticFilteredSourceKind::subtractive_operand_effect;
    geometry.occurrences[0].source.role = AnalyticFilteredSourceRole::none;
    const auto result = build(records_for({{1, {1}}}), geometry);
    require(result.error == AnalyticFilteredOutcomesError::invalid_argument &&
                result.telemetry.arrangement_work_units == 0 && result.events.empty(),
            "contract-invalid outcome source reached arrangement or publication");
}

std::string parity_vector()
{
    AnalyticFilteredGeometry geometry;
    append_box(geometry, 40, 0, 0, 1000, 1000);
    append_box(geometry, 10, 200, 200, 800, 800);
    append_box(geometry, 30, 250, 250, 750, 750);
    append_box(geometry, 20, 400, 400, 600, 600);
    const auto result = build(records_for({{1, {40}}, {2, {10}}, {1, {30}}, {2, {20}}}), geometry);
    require(result.error == AnalyticFilteredOutcomesError::none, "outcome parity fixture failed");
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    const auto append = [&output](std::uint64_t value) { output << std::setw(16) << value; };
    append(static_cast<std::uint8_t>(result.error));
    append(result.events.size());
    for (const auto& value : result.events)
    {
        append(value.operand_id);
        append(static_cast<std::uint16_t>(value.kind));
        append(value.result_references.begin);
        append(value.result_references.count);
        append(value.sources.begin);
        append(value.sources.count);
    }
    append(result.result_references.size());
    for (const auto& value : result.result_references)
    {
        append(static_cast<std::uint8_t>(value.kind));
        append(value.local_index);
    }
    append(result.source_references.size());
    for (const auto& value : result.source_references)
    {
        append(static_cast<std::uint16_t>(value.kind));
        append(static_cast<std::uint16_t>(value.role));
        append(value.operand_id);
        append(value.primary_id);
        append(value.secondary_id);
    }
    const auto& evidence = result.lineage.regions.selection.outcome_evidence;
    append(evidence.size());
    for (const auto& value : evidence)
    {
        append(value.operand_id);
        append(value.covered_positive_area);
        append(value.redundant_or_absorbed);
        append(value.removed_later);
        append(value.attributed_removal);
        append(value.unfilled_removal);
        append(value.overwritten);
    }
    const auto& telemetry = result.telemetry;
    append(telemetry.lineage_work_units);
    append(telemetry.lineage_peak_working_memory_bytes);
    append(telemetry.arrangement_work_units);
    append(telemetry.operand_source_visits);
    append(telemetry.lineage_source_visits);
    append(telemetry.emitted_events);
    append(telemetry.emitted_result_references);
    append(telemetry.emitted_source_references);
    append(telemetry.sort_work_units);
    append(telemetry.reserved_outcomes_work_units);
    append(telemetry.outcome_work_units);
    append(telemetry.predicate_calls);
    append(telemetry.peak_working_memory_bytes);
    append(telemetry.algebraic_fallback_calls);
    return output.str();
}
} // namespace

int main(int argc, char** argv)
{
    test_single_and_coincident_union();
    test_remove_refill_history();
    test_accumulator_redundancy_and_same_stage_subtractors();
    test_complete_removal_and_no_effect();
    test_remove_refill_remove_and_collapsed_no_effect();
    test_dense_stage_and_exact_reference_counting();
    test_output_association_scaling();
    test_empty_stage_work_admission();
    test_governance_and_sparse_scaling();
    test_malformed_source_fails_before_arrangement();
    if (argc == 2 && std::string(argv[1]) == "--emit-parity")
        std::cout << "ANALYTIC_FILTERED_OUTCOMES_VECTOR=" << parity_vector() << '\n';
    std::cout << "analytic filtered outcomes tests passed\n";
    return 0;
}

#include "geometer/analytic_curve_broad_phase.h"
#include "geometer/analytic_filtered_lineage.h"

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
    occurrence.source.role = AnalyticFilteredSourceRole::authored_line;
    occurrence.source.operand_id = operand;
    occurrence.source.primary_id = 10000 + index;
    occurrence.source.secondary_id = 20000 + index;
    geometry.occurrences.push_back(occurrence);
}

void append_rectangle(AnalyticFilteredGeometry& geometry, std::uint64_t operand, double minimum,
                      double maximum)
{
    append_line(geometry, operand, minimum, minimum, maximum, minimum);
    append_line(geometry, operand, maximum, minimum, maximum, maximum);
    append_line(geometry, operand, maximum, maximum, minimum, maximum);
    append_line(geometry, operand, minimum, maximum, minimum, minimum);
}

void append_box(AnalyticFilteredGeometry& geometry, std::uint64_t operand, double min_x,
                double min_y, double max_x, double max_y)
{
    append_line(geometry, operand, min_x, min_y, max_x, min_y);
    append_line(geometry, operand, max_x, min_y, max_x, max_y);
    append_line(geometry, operand, max_x, max_y, min_x, max_y);
    append_line(geometry, operand, min_x, max_y, min_x, min_y);
}

void share_rectangle_carriers(AnalyticFilteredGeometry& geometry, std::uint32_t authority_begin,
                              std::uint32_t target_begin)
{
    for (std::uint32_t side = 0; side < 4; ++side)
    {
        auto& target = geometry.curves[target_begin + side];
        const auto& authority = geometry.curves[authority_begin + side];
        target.construction_carrier_id = authority.construction_carrier_id;
        target.construction_family_id = authority.construction_family_id;
        target.start.construction_x_column_id = authority.start.construction_x_column_id;
        target.end.construction_x_column_id = authority.end.construction_x_column_id;
    }
}

AnalyticFilteredLineageResult build(const AnalyticRequestPacketRecords& records,
                                    const AnalyticFilteredGeometry& geometry,
                                    const AnalyticSolverLimits& limits = {})
{
    const AnalyticBroadPhaseResult broad = build_analytic_curve_candidates(geometry.bounds, limits);
    require(broad.error == AnalyticBroadPhaseError::none, "lineage broad phase failed");
    return build_analytic_filtered_lineage(records, 0, geometry, broad.pairs, limits);
}

std::vector<AnalyticCurvePair> pairs_for(const AnalyticFilteredGeometry& geometry)
{
    const AnalyticBroadPhaseResult broad = build_analytic_curve_candidates(geometry.bounds);
    require(broad.error == AnalyticBroadPhaseError::none, "lineage pair fixture failed");
    return broad.pairs;
}

std::vector<AnalyticFilteredSourceReference> range(const AnalyticFilteredLineageResult& result,
                                                   AnalyticFilteredSourceRange value)
{
    require(value.begin <= result.source_references.size() &&
                value.count <= result.source_references.size() - value.begin,
            "lineage source range escaped table");
    return {result.source_references.begin() + value.begin,
            result.source_references.begin() + value.begin + value.count};
}

std::set<std::uint64_t> operands(const std::vector<AnalyticFilteredSourceReference>& sources)
{
    std::set<std::uint64_t> output;
    for (const auto& source : sources)
        output.insert(source.operand_id);
    return output;
}

void test_single_rectangle()
{
    AnalyticFilteredGeometry geometry;
    append_rectangle(geometry, 20, 0, 1000);
    const AnalyticFilteredLineageResult result = build(records_for({{1, {20}}}), geometry);
    require(result.error == AnalyticFilteredLineageError::none, "single lineage failed");
    require(result.boundaries.size() == 4 && result.vertices.size() == 4 &&
                result.region_lineage.size() == 1,
            "single lineage record counts drifted");
    for (const auto& boundary : result.boundaries)
        require(boundary.positive.count == 1 && boundary.subtraction.count == 0,
                "single boundary lineage drifted");
    for (const auto& vertex : result.vertices)
        require(vertex.intersection.count == 2, "single vertex incidence lineage drifted");
    require(result.region_lineage[0].positive_contributors.count == 4 &&
                operands(range(result, result.region_lineage[0].positive_contributors)) ==
                    std::set<std::uint64_t>{20},
            "single region contributor lineage drifted");
}

void test_difference_and_refill_epoch()
{
    AnalyticFilteredGeometry geometry;
    append_rectangle(geometry, 40, 0, 1000);
    append_rectangle(geometry, 10, 200, 800);
    append_rectangle(geometry, 30, 250, 750);
    append_rectangle(geometry, 20, 400, 600);
    const AnalyticFilteredLineageResult result =
        build(records_for({{1, {40}}, {2, {10}}, {1, {30}}, {2, {20}}}), geometry);
    require(result.error == AnalyticFilteredLineageError::none,
            "remove/refill/remove lineage failed error=" +
                std::to_string(static_cast<unsigned>(result.error)) +
                " region-error=" + std::to_string(static_cast<unsigned>(result.regions.error)));
    bool saw_refill_hole = false;
    bool saw_refill_outer = false;
    std::set<std::uint64_t> subtractors;
    for (const auto& boundary : result.boundaries)
    {
        const auto subtraction = range(result, boundary.subtraction);
        const auto positive = operands(range(result, boundary.positive));
        if (!subtraction.empty())
        {
            const auto values = operands(subtraction);
            require(values.size() == 1, "subtraction epochs were composed on one boundary");
            subtractors.insert(*values.begin());
            if (*values.begin() == 20)
            {
                saw_refill_hole = true;
                require(boundary.positive.count == 0,
                        "noncoincident refill source claimed subtractive boundary geometry");
            }
            if (*values.begin() == 10 && positive == std::set<std::uint64_t>{30})
                saw_refill_outer = true;
        }
    }
    require(saw_refill_outer && saw_refill_hole && subtractors == std::set<std::uint64_t>({10, 20}),
            "surviving subtraction epochs were not attributed separately");
    std::set<std::uint64_t> contributors;
    for (const auto& region : result.region_lineage)
    {
        const auto values = operands(range(result, region.positive_contributors));
        contributors.insert(values.begin(), values.end());
    }
    require(contributors == std::set<std::uint64_t>({30, 40}),
            "stage-aware region contributor sets drifted");
}

void test_branched_refill_contributors()
{
    const auto run = [](bool reflected)
    {
        AnalyticFilteredGeometry geometry;
        const auto reflect = [reflected](double value) { return reflected ? -value : value; };
        const auto rectangle = [&](std::uint64_t operand, double minimum, double maximum)
        {
            if (reflected)
                append_rectangle(geometry, operand, reflect(maximum), reflect(minimum));
            else
                append_rectangle(geometry, operand, minimum, maximum);
        };
        rectangle(10, 200, 800);
        rectangle(20, 300, 700);
        rectangle(30, 0, 1000);
        return build(records_for({{1, {10}}, {2, {20}}, {1, {30}}}), geometry);
    };
    for (const bool reflected : {false, true})
    {
        const auto result = run(reflected);
        require(result.error == AnalyticFilteredLineageError::none &&
                    result.region_lineage.size() == 1,
                "branched refill lineage failed");
        require(operands(range(result, result.region_lineage[0].positive_contributors)) ==
                    std::set<std::uint64_t>({10, 30}),
                "branch-local contributor epoch omitted a positive operand");
    }
}

void test_malformed_sources_fail_before_arrangement()
{
    const auto records = records_for({{1, {20}}});
    const auto check = [&](const AnalyticFilteredSourceReference& source)
    {
        AnalyticFilteredGeometry geometry;
        append_rectangle(geometry, 20, 0, 1000);
        geometry.occurrences[0].source = source;
        const AnalyticFilteredLineageResult result = build(records, geometry);
        require(result.error == AnalyticFilteredLineageError::invalid_argument &&
                    result.regions.selection.telemetry.arrangement_predicate_calls == 0 &&
                    result.telemetry.predicate_calls == result.telemetry.regions_work_units &&
                    result.telemetry.peak_working_memory_bytes ==
                        result.telemetry.regions_peak_working_memory_bytes &&
                    result.boundaries.empty() && result.source_references.empty(),
                "malformed source escaped owned lineage admission error=" +
                    std::to_string(static_cast<unsigned>(result.error)));
    };
    AnalyticFilteredSourceReference source{AnalyticFilteredSourceKind::authored_segment_curve,
                                           AnalyticFilteredSourceRole::authored_line, 20, 10001,
                                           20001};
    source.role = AnalyticFilteredSourceRole::none;
    check(source);
    source.role = AnalyticFilteredSourceRole::authored_line;
    source.primary_id = 0;
    check(source);
    source.primary_id = 10001;
    source.kind = AnalyticFilteredSourceKind::subtractive_operand_effect;
    source.role = AnalyticFilteredSourceRole::none;
    source.secondary_id = 0;
    check(source);
    source.kind = AnalyticFilteredSourceKind::compact_feature_role;
    source.role = AnalyticFilteredSourceRole::primitive_outer_circle;
    check(source);
    source.role = AnalyticFilteredSourceRole::swept_start_cap;
    source.secondary_id = std::uint64_t{2} << 32U;
    check(source);
}

void test_governed_admission_and_publication()
{
    AnalyticFilteredGeometry geometry;
    append_rectangle(geometry, 20, 0, 1000);
    const auto records = records_for({{1, {20}}});
    const auto pairs = pairs_for(geometry);
    const auto run = [&](const AnalyticSolverLimits& limits)
    { return build_analytic_filtered_lineage(records, 0, geometry, pairs, limits); };

    AnalyticSolverLimits limits;
    std::uint64_t low = 0;
    std::uint64_t high = limits.working_memory_bytes;
    while (low < high)
    {
        const std::uint64_t middle = low + (high - low) / 2;
        auto probe = limits;
        probe.working_memory_bytes = middle;
        if (run(probe).error == AnalyticFilteredLineageError::none)
            high = middle;
        else
            low = middle + 1;
    }
    auto exact_memory = limits;
    exact_memory.working_memory_bytes = low;
    require(run(exact_memory).error == AnalyticFilteredLineageError::none,
            "exact lineage memory admission failed");
    auto short_memory = exact_memory;
    --short_memory.working_memory_bytes;
    const auto memory_failure = run(short_memory);
    require(memory_failure.error == AnalyticFilteredLineageError::resource_limit_exceeded &&
                memory_failure.regions.selection.telemetry.arrangement_predicate_calls == 0 &&
                memory_failure.source_references.empty(),
            "one-byte-short lineage admission was late");

    low = 0;
    high = limits.predicate_calls;
    while (low < high)
    {
        const std::uint64_t middle = low + (high - low) / 2;
        auto probe = limits;
        probe.predicate_calls = middle;
        if (run(probe).telemetry.arrangement_work_units != 0)
            high = middle;
        else
            low = middle + 1;
    }
    auto exact_work = limits;
    exact_work.predicate_calls = low;
    require(run(exact_work).telemetry.arrangement_work_units != 0,
            "exact lineage structural-work admission failed");
    auto short_work = exact_work;
    --short_work.predicate_calls;
    const auto work_failure = run(short_work);
    require(work_failure.error == AnalyticFilteredLineageError::resource_limit_exceeded &&
                work_failure.telemetry.arrangement_work_units == 0 &&
                work_failure.source_references.empty(),
            "one-unit-short lineage admission was late");

    const auto success = run(limits);
    low = 0;
    high = limits.predicate_calls;
    while (low < high)
    {
        const std::uint64_t middle = low + (high - low) / 2;
        auto probe = limits;
        probe.predicate_calls = middle;
        if (run(probe).error == AnalyticFilteredLineageError::none)
            high = middle;
        else
            low = middle + 1;
    }
    auto exact_total_work = limits;
    exact_total_work.predicate_calls = low;
    require(run(exact_total_work).error == AnalyticFilteredLineageError::none,
            "exact total lineage work failed");
    auto short_total_work = exact_total_work;
    --short_total_work.predicate_calls;
    const auto total_work_failure = run(short_total_work);
    require(total_work_failure.error == AnalyticFilteredLineageError::resource_limit_exceeded &&
                total_work_failure.telemetry.publication_capacity_records == 0 &&
                total_work_failure.source_references.empty(),
            "one-unit-short publication work was admitted after allocation");
    auto short_output = limits;
    short_output.provenance_references = success.source_references.size() - 1;
    const auto output_failure = run(short_output);
    require(output_failure.error == AnalyticFilteredLineageError::resource_limit_exceeded &&
                output_failure.source_references.empty() && output_failure.boundaries.empty(),
            "one-reference-short lineage publication escaped count preflight error=" +
                std::to_string(static_cast<unsigned>(output_failure.error)) +
                " refs=" + std::to_string(output_failure.source_references.size()) +
                " success=" + std::to_string(success.source_references.size()));
}

AnalyticFilteredLineageResult build_disjoint(std::uint32_t count)
{
    AnalyticFilteredGeometry geometry;
    std::vector<std::uint64_t> ids;
    ids.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index)
    {
        const std::uint64_t operand = 1000 + index;
        ids.push_back(operand);
        const double minimum = static_cast<double>(index) * 2000.0;
        append_rectangle(geometry, operand, minimum, minimum + 100.0);
    }
    return build(records_for({{1, ids}}), geometry);
}

void test_sparse_scaling()
{
    const auto small = build_disjoint(16);
    const auto large = build_disjoint(32);
    require(small.error == AnalyticFilteredLineageError::none &&
                large.error == AnalyticFilteredLineageError::none,
            "sparse lineage scaling fixture failed");
    require(large.telemetry.predicate_calls <= small.telemetry.predicate_calls * 3 &&
                large.telemetry.peak_working_memory_bytes <=
                    small.telemetry.peak_working_memory_bytes * 3,
            "sparse lineage scaling exceeded 3x at 2x input");
}

void test_publication_dense_output_preflight()
{
    AnalyticFilteredGeometry geometry;
    for (std::uint32_t index = 0; index < 8; ++index)
        append_rectangle(geometry, 1, index * 2000.0, index * 2000.0 + 1000.0);
    const auto records = records_for({{1, {1}}});
    const auto pairs = pairs_for(geometry);
    const auto run = [&](const AnalyticSolverLimits& limits)
    { return build_analytic_filtered_lineage(records, 0, geometry, pairs, limits); };
    AnalyticSolverLimits limits;
    const auto success = run(limits);
    require(success.error == AnalyticFilteredLineageError::none &&
                success.region_lineage.size() == 8,
            "publication-dense lineage fixture failed error=" +
                std::to_string(static_cast<unsigned>(success.error)) +
                " region_error=" + std::to_string(static_cast<unsigned>(success.regions.error)) +
                " regions=" + std::to_string(success.region_lineage.size()));
    std::uint64_t contributor_references = 0;
    for (const auto& region : success.region_lineage)
        contributor_references += region.positive_contributors.count;
    require(contributor_references >= 8 * 32,
            "publication-dense fixture did not create output-proportional lineage");

    std::uint64_t low = 0;
    std::uint64_t high = limits.predicate_calls;
    while (low < high)
    {
        const std::uint64_t middle = low + (high - low) / 2;
        auto probe = limits;
        probe.predicate_calls = middle;
        if (run(probe).error == AnalyticFilteredLineageError::none)
            high = middle;
        else
            low = middle + 1;
    }
    auto exact = limits;
    exact.predicate_calls = low;
    require(run(exact).error == AnalyticFilteredLineageError::none,
            "publication-dense exact work failed");
    --exact.predicate_calls;
    const auto short_result = run(exact);
    require(short_result.error == AnalyticFilteredLineageError::resource_limit_exceeded &&
                short_result.telemetry.publication_capacity_records == 0 &&
                short_result.source_references.empty(),
            "publication-dense one-short work allocated publication storage");

    low = 0;
    high = limits.working_memory_bytes;
    while (low < high)
    {
        const std::uint64_t middle = low + (high - low) / 2;
        auto probe = limits;
        probe.working_memory_bytes = middle;
        if (run(probe).error == AnalyticFilteredLineageError::none)
            high = middle;
        else
            low = middle + 1;
    }
    auto exact_memory = limits;
    exact_memory.working_memory_bytes = low;
    const auto exact_memory_result = run(exact_memory);
    require(exact_memory_result.error == AnalyticFilteredLineageError::none &&
                exact_memory_result.telemetry.peak_working_memory_bytes == low,
            "publication-dense exact memory failed threshold=" + std::to_string(low) +
                " peak=" + std::to_string(exact_memory_result.telemetry.peak_working_memory_bytes));
    --exact_memory.working_memory_bytes;
    const auto short_memory_result = run(exact_memory);
    require(short_memory_result.error == AnalyticFilteredLineageError::resource_limit_exceeded &&
                short_memory_result.telemetry.publication_capacity_records == 0 &&
                short_memory_result.source_references.empty(),
            "publication-dense one-byte-short memory allocated publication storage");
}

AnalyticFilteredLineageResult build_branch_dense(std::uint32_t count,
                                                 const AnalyticSolverLimits& limits = {})
{
    AnalyticFilteredGeometry geometry;
    std::vector<std::uint64_t> first_stage;
    first_stage.reserve(count * 2);
    std::uint32_t authority_begin = 0;
    for (std::uint32_t index = 0; index < count; ++index)
    {
        const std::uint64_t central = index * 2 + 1;
        const std::uint64_t remote = central + 1;
        first_stage.push_back(central);
        first_stage.push_back(remote);
        const std::uint32_t begin = static_cast<std::uint32_t>(geometry.curves.size());
        append_box(geometry, central, 0, 0, 10000, 1000);
        if (index == 0)
            authority_begin = begin;
        else
            share_rectangle_carriers(geometry, authority_begin, begin);
        const double remote_x = 100000.0 + index * 1000.0;
        append_box(geometry, remote, remote_x, 0, remote_x + 500.0, 500.0);
    }
    constexpr std::uint64_t difference = 1000000;
    append_box(geometry, difference, 99500, -500, 100000.0 + count * 1000.0, 1000);
    for (std::uint32_t index = 0; index < count; ++index)
    {
        const double x = 250.0 + index * (9000.0 / count);
        append_box(geometry, difference, x, 200, x + 100.0, 800);
    }
    constexpr std::uint64_t refill = 2000000;
    const std::uint32_t refill_begin = static_cast<std::uint32_t>(geometry.curves.size());
    append_box(geometry, refill, 0, 0, 10000, 1000);
    share_rectangle_carriers(geometry, authority_begin, refill_begin);
    const auto records = records_for({{1, first_stage}, {2, {difference}}, {1, {refill}}});
    const auto pairs = pairs_for(geometry);
    return build_analytic_filtered_lineage(records, 0, geometry, pairs, limits);
}

void test_branch_dense_reporter_scaling()
{
    const auto small = build_branch_dense(8);
    const auto large = build_branch_dense(16);
    require(small.error == AnalyticFilteredLineageError::none &&
                large.error == AnalyticFilteredLineageError::none,
            "branch-dense reporter fixture failed");
    require(large.telemetry.coverage_node_visits < small.telemetry.coverage_node_visits * 3 &&
                large.telemetry.lineage_work_units < small.telemetry.lineage_work_units * 3,
            "active-unreported reporter exceeded 3x work at 2x input");

    AnalyticSolverLimits limits;
    std::uint64_t low = 0;
    std::uint64_t high = limits.working_memory_bytes;
    while (low < high)
    {
        const std::uint64_t middle = low + (high - low) / 2;
        auto probe = limits;
        probe.working_memory_bytes = middle;
        if (build_branch_dense(8, probe).error == AnalyticFilteredLineageError::none)
            high = middle;
        else
            low = middle + 1;
    }
    auto exact = limits;
    exact.working_memory_bytes = low;
    const auto exact_result = build_branch_dense(8, exact);
    require(exact_result.error == AnalyticFilteredLineageError::none,
            "connected branch-dense exact memory failed threshold=" + std::to_string(low) +
                " peak=" + std::to_string(exact_result.telemetry.peak_working_memory_bytes));
    --exact.working_memory_bytes;
    const auto short_result = build_branch_dense(8, exact);
    require(short_result.error == AnalyticFilteredLineageError::resource_limit_exceeded &&
                short_result.regions.selection.telemetry.arrangement_predicate_calls == 0 &&
                short_result.telemetry.publication_capacity_records == 0 &&
                short_result.source_references.empty(),
            "connected branch-dense one-byte-short memory reached arrangement or publication");
}

void test_disconnected_many_to_many()
{
    AnalyticFilteredGeometry geometry;
    append_rectangle(geometry, 100, 0, 1000);
    append_rectangle(geometry, 100, 2000, 3000);
    append_rectangle(geometry, 200, 500, 1500);
    append_rectangle(geometry, 200, 2500, 3500);
    const AnalyticFilteredLineageResult result = build(records_for({{1, {100, 200}}}), geometry);
    require(result.error == AnalyticFilteredLineageError::none && result.region_lineage.size() == 2,
            "many-to-many lineage failed");
    for (const auto& region : result.region_lineage)
        require(operands(range(result, region.positive_contributors)) ==
                    std::set<std::uint64_t>({100, 200}),
                "disconnected contributor association drifted");
}

void test_same_stage_subtractors_and_coincident_positives()
{
    AnalyticFilteredGeometry geometry;
    append_rectangle(geometry, 40, 0, 1000);
    append_rectangle(geometry, 10, 200, 800);
    append_rectangle(geometry, 20, 250, 750);
    append_rectangle(geometry, 30, 300, 700);
    const auto result = build(records_for({{1, {40}}, {2, {10, 20}}, {1, {30}}}), geometry);
    require(result.error == AnalyticFilteredLineageError::none,
            "same-stage subtraction lineage failed");
    bool saw_both = false;
    for (const auto& boundary : result.boundaries)
        if (operands(range(result, boundary.positive)) == std::set<std::uint64_t>{30} &&
            operands(range(result, boundary.subtraction)) == std::set<std::uint64_t>({10, 20}))
            saw_both = true;
    require(saw_both, "refill boundary lost same-stage noncoincident subtractors");

    AnalyticFilteredGeometry coincident;
    append_rectangle(coincident, 100, 0, 1000);
    append_rectangle(coincident, 200, 0, 1000);
    for (std::uint32_t index = 4; index < 8; ++index)
    {
        coincident.curves[index].construction_carrier_id =
            coincident.curves[index - 4].construction_carrier_id;
        coincident.curves[index].construction_family_id =
            coincident.curves[index - 4].construction_family_id;
        coincident.curves[index].start.construction_x_column_id =
            coincident.curves[index - 4].start.construction_x_column_id;
        coincident.curves[index].end.construction_x_column_id =
            coincident.curves[index - 4].end.construction_x_column_id;
    }
    const auto coincident_result = build(records_for({{1, {100, 200}}}), coincident);
    require(coincident_result.error == AnalyticFilteredLineageError::none &&
                coincident_result.region_lineage.size() == 1 &&
                operands(range(coincident_result,
                               coincident_result.region_lineage[0].positive_contributors)) ==
                    std::set<std::uint64_t>({100, 200}),
            "coincident positive contributors were not retained error=" +
                std::to_string(static_cast<unsigned>(coincident_result.error)) +
                " regions=" + std::to_string(coincident_result.region_lineage.size()));
}

void test_isolated_collapsed_lineage_stays_internal()
{
    AnalyticFilteredGeometry geometry;
    append_line(geometry, 1, 0, 0, 20, 20);
    const auto result = build(records_for({{1, {1}}}), geometry);
    require(result.error == AnalyticFilteredLineageError::none && result.boundaries.empty() &&
                result.vertices.empty() && result.region_lineage.empty() &&
                result.regions.selection.arrangement.collapsed_spans.size() == 1,
            "isolated collapsed lineage manufactured public topology");
}

void test_production_lowered_compact_sources()
{
    AnalyticRequestPacketRecords records;
    records.jobs = {{1, 0, 1}};
    records.stages = {{1, 1, 0, 1}};
    records.operands = {{1, 2, 0}};
    records.disks = {{10, 0, 0, 1000}};
    const auto lowered = lower_analytic_job_to_filtered_curves(records, 0);
    require(lowered.error == AnalyticFilteredLoweringError::none && lowered.value,
            "compact lineage lowering failed");
    const auto result = build(records, *lowered.value);
    require(result.error == AnalyticFilteredLineageError::none && !result.source_references.empty(),
            "production-lowered compact lineage failed");
    for (const auto& source : result.source_references)
        require(source.kind == AnalyticFilteredSourceKind::compact_feature_role &&
                    source.role == AnalyticFilteredSourceRole::primitive_outer_circle &&
                    source.primary_id == 10 && source.secondary_id == 0,
                "compact source identity drifted during lineage publication");
}

std::string parity_vector()
{
    AnalyticFilteredGeometry geometry;
    append_rectangle(geometry, 40, 0, 1000);
    append_rectangle(geometry, 10, 200, 800);
    append_rectangle(geometry, 30, 250, 750);
    append_rectangle(geometry, 20, 400, 600);
    const AnalyticFilteredLineageResult result =
        build(records_for({{1, {40}}, {2, {10}}, {1, {30}}, {2, {20}}}), geometry);
    require(result.error == AnalyticFilteredLineageError::none, "lineage parity fixture failed");
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    const auto append = [&output](std::uint64_t value) { output << std::setw(16) << value; };
    append(static_cast<std::uint8_t>(result.error));
    append(static_cast<std::uint64_t>(result.regions.selection.origin_x_nm));
    append(static_cast<std::uint64_t>(result.regions.selection.origin_y_nm));
    append(result.boundaries.size());
    for (const auto& boundary : result.boundaries)
    {
        append(boundary.half_edge);
        append(boundary.positive.begin);
        append(boundary.positive.count);
        append(boundary.subtraction.begin);
        append(boundary.subtraction.count);
    }
    append(result.vertices.size());
    for (const auto& vertex : result.vertices)
    {
        append(vertex.arrangement_vertex);
        append(vertex.intersection.begin);
        append(vertex.intersection.count);
    }
    append(result.region_lineage.size());
    for (const auto& region : result.region_lineage)
    {
        append(region.region);
        append(region.positive_contributors.begin);
        append(region.positive_contributors.count);
    }
    append(result.source_references.size());
    for (const auto& source : result.source_references)
    {
        append(static_cast<std::uint16_t>(source.kind));
        append(static_cast<std::uint16_t>(source.role));
        append(source.operand_id);
        append(source.primary_id);
        append(source.secondary_id);
    }
    const auto& telemetry = result.telemetry;
    append(telemetry.regions_work_units);
    append(telemetry.regions_peak_working_memory_bytes);
    append(telemetry.arrangement_work_units);
    append(telemetry.coverage_node_visits);
    append(telemetry.component_transition_visits);
    append(telemetry.boundary_membership_visits);
    append(telemetry.vertex_membership_visits);
    append(telemetry.emitted_boundary_records);
    append(telemetry.emitted_vertex_records);
    append(telemetry.emitted_region_records);
    append(telemetry.emitted_source_references);
    append(telemetry.publication_capacity_records);
    append(telemetry.sort_work_units);
    append(telemetry.reserved_lineage_work_units);
    append(telemetry.lineage_work_units);
    append(telemetry.predicate_calls);
    append(telemetry.peak_working_memory_bytes);
    append(telemetry.algebraic_fallback_calls);
    return output.str();
}
} // namespace

int main(int argc, char** argv)
{
    test_single_rectangle();
    test_difference_and_refill_epoch();
    test_branched_refill_contributors();
    test_disconnected_many_to_many();
    test_same_stage_subtractors_and_coincident_positives();
    test_isolated_collapsed_lineage_stays_internal();
    test_production_lowered_compact_sources();
    test_malformed_sources_fail_before_arrangement();
    test_governed_admission_and_publication();
    test_sparse_scaling();
    test_publication_dense_output_preflight();
    test_branch_dense_reporter_scaling();
    if (argc == 2 && std::string(argv[1]) == "--emit-parity")
        std::cout << "ANALYTIC_FILTERED_LINEAGE_VECTOR=" << parity_vector() << '\n';
    std::cout << "ANALYTIC_FILTERED_LINEAGE_TEST=ok\n";
    return 0;
}

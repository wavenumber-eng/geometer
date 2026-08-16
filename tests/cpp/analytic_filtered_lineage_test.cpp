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
    occurrence.source.operand_id = operand;
    occurrence.source.primary_id = 10000 + index;
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

AnalyticFilteredLineageResult build(const AnalyticRequestPacketRecords& records,
                                    const AnalyticFilteredGeometry& geometry,
                                    const AnalyticSolverLimits& limits = {})
{
    const AnalyticBroadPhaseResult broad = build_analytic_curve_candidates(geometry.bounds, limits);
    require(broad.error == AnalyticBroadPhaseError::none, "lineage broad phase failed");
    return build_analytic_filtered_lineage(records, 0, geometry, broad.pairs, limits);
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
    std::set<std::uint64_t> subtractors;
    for (const auto& boundary : result.boundaries)
    {
        const auto subtraction = range(result, boundary.subtraction);
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
        }
    }
    require(saw_refill_hole && subtractors == std::set<std::uint64_t>({10, 20}),
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
    append(telemetry.coverage_node_visits);
    append(telemetry.set_union_visits);
    append(telemetry.boundary_membership_visits);
    append(telemetry.vertex_membership_visits);
    append(telemetry.emitted_boundary_records);
    append(telemetry.emitted_vertex_records);
    append(telemetry.emitted_region_records);
    append(telemetry.emitted_source_references);
    append(telemetry.sort_work_units);
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
    test_disconnected_many_to_many();
    if (argc == 2 && std::string(argv[1]) == "--emit-parity")
        std::cout << "ANALYTIC_FILTERED_LINEAGE_VECTOR=" << parity_vector() << '\n';
    std::cout << "ANALYTIC_FILTERED_LINEAGE_TEST=ok\n";
    return 0;
}

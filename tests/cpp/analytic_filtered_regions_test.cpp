#include "geometer/analytic_curve_broad_phase.h"
#include "geometer/analytic_filtered_regions.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
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

void append_regular_polygon(AnalyticFilteredGeometry& geometry, std::uint64_t operand,
                            std::uint32_t segment_count)
{
    constexpr double pi = 3.14159265358979323846;
    constexpr double radius = 1'000'000.0;
    std::vector<std::pair<double, double>> points;
    points.reserve(segment_count);
    for (std::uint32_t index = 0; index < segment_count; ++index)
    {
        const double angle = 2.0 * pi * index / segment_count;
        points.emplace_back(static_cast<double>(std::llround(std::cos(angle) * radius)),
                            static_cast<double>(std::llround(std::sin(angle) * radius)));
    }
    for (std::uint32_t index = 0; index < segment_count; ++index)
    {
        const auto [x1, y1] = points[index];
        const auto [x2, y2] = points[(index + 1) % segment_count];
        append_line(geometry, operand, x1, y1, x2, y2);
    }
}

AnalyticFilteredRegionsResult build(const AnalyticRequestPacketRecords& records,
                                    const AnalyticFilteredGeometry& geometry,
                                    const AnalyticSolverLimits& limits = {})
{
    const AnalyticBroadPhaseResult broad = build_analytic_curve_candidates(geometry.bounds);
    require(broad.error == AnalyticBroadPhaseError::none, "regions broad phase failed");
    return build_analytic_filtered_regions(records, 0, geometry, broad.pairs, limits);
}

double signed_line_ring_area_twice(const AnalyticFilteredRegionsResult& result,
                                   const AnalyticFilteredMaterialRing& ring)
{
    double area = 0.0;
    for (std::uint32_t offset = 0; offset < ring.half_edge_count; ++offset)
    {
        const std::uint32_t current = result.ring_half_edges[ring.half_edge_begin + offset];
        const std::uint32_t next =
            result.ring_half_edges[ring.half_edge_begin + (offset + 1) % ring.half_edge_count];
        const AnalyticFilteredPointNm& left =
            result.selection.arrangement
                .vertices[result.selection.arrangement.half_edges[current].origin_vertex]
                .point;
        const AnalyticFilteredPointNm& right =
            result.selection.arrangement
                .vertices[result.selection.arrangement.half_edges[next].origin_vertex]
                .point;
        area += left.x.lower * right.y.lower - right.x.lower * left.y.lower;
    }
    return area;
}

void test_single_and_disjoint_regions()
{
    AnalyticFilteredGeometry single_geometry;
    append_rectangle(single_geometry, 1, 0, 1000);
    const AnalyticFilteredRegionsResult single = build(records_for({{1, {1}}}), single_geometry);
    require(single.error == AnalyticFilteredRegionsError::none, "single region failed");
    require(single.rings.size() == 1 && single.regions.size() == 1 &&
                single.ring_half_edges.size() == 4,
            "single region topology drifted");
    require(single.rings[0].parent_ring == kNoAnalyticFilteredRing && single.rings[0].depth == 0 &&
                single.rings[0].counterclockwise,
            "single outer ring classification drifted");

    AnalyticFilteredGeometry disjoint_geometry;
    append_rectangle(disjoint_geometry, 1, 0, 1000);
    append_rectangle(disjoint_geometry, 2, 2000, 3000);
    const AnalyticFilteredRegionsResult disjoint =
        build(records_for({{1, {1, 2}}}), disjoint_geometry);
    require(disjoint.error == AnalyticFilteredRegionsError::none, "disjoint regions failed");
    require(disjoint.rings.size() == 2 && disjoint.regions.size() == 2,
            "disjoint material components were not retained");
}

void test_nested_annulus_and_island()
{
    AnalyticFilteredGeometry geometry;
    append_rectangle(geometry, 1, 0, 1000);
    append_rectangle(geometry, 2, 200, 800);
    append_rectangle(geometry, 3, 400, 600);
    const AnalyticFilteredRegionsResult result =
        build(records_for({{1, {1}}, {2, {2}}, {1, {3}}}), geometry);
    require(result.error == AnalyticFilteredRegionsError::none, "nested regions failed");
    require(result.rings.size() == 3 && result.regions.size() == 2,
            "nested region/ring count drifted");
    std::vector<std::uint32_t> depths;
    for (const AnalyticFilteredMaterialRing& ring : result.rings)
        depths.push_back(ring.depth);
    std::sort(depths.begin(), depths.end());
    require(depths == std::vector<std::uint32_t>({0, 1, 2}), "nested ring depths drifted");
    for (std::uint32_t ring = 0; ring < result.rings.size(); ++ring)
    {
        const AnalyticFilteredMaterialRing& value = result.rings[ring];
        require(value.counterclockwise == (value.depth % 2 == 0), "nested ring direction drifted");
        require((signed_line_ring_area_twice(result, value) > 0.0) == value.counterclockwise,
                "nested ring flag disagreed with directed line geometry");
        if (value.depth == 0)
            require(value.parent_ring == kNoAnalyticFilteredRing,
                    "outer ring unexpectedly has a parent");
        else
            require(value.parent_ring < result.rings.size() &&
                        result.rings[value.parent_ring].depth + 1 == value.depth,
                    "nested ring parent drifted");
    }
}

void test_collapsed_tangent_and_arc_topology()
{
    AnalyticFilteredGeometry collapsed_geometry;
    append_line(collapsed_geometry, 1, 0, 0, 20, 20);
    const AnalyticFilteredRegionsResult collapsed =
        build(records_for({{1, {1}}}), collapsed_geometry);
    require(collapsed.error == AnalyticFilteredRegionsError::none && collapsed.rings.empty() &&
                collapsed.regions.empty() &&
                collapsed.selection.arrangement.collapsed_spans.size() == 1,
            "collapsed isolated lineage was rejected by region extraction");

    AnalyticRequestPacketRecords disk_records;
    disk_records.jobs = {{1, 0, 1}};
    disk_records.stages = {{1, 1, 0, 2}};
    disk_records.operands = {{1, 2, 0}, {2, 2, 1}};
    disk_records.disks = {{10, 0, 0, 1000}, {11, 2000, 0, 1000}};
    const AnalyticFilteredLoweringResult lowered =
        lower_analytic_job_to_filtered_curves(disk_records, 0);
    require(lowered.error == AnalyticFilteredLoweringError::none && lowered.value,
            "disk region lowering failed");
    const AnalyticFilteredRegionsResult disk = build(disk_records, *lowered.value);
    require(disk.error == AnalyticFilteredRegionsError::none && disk.rings.size() == 2 &&
                disk.regions.size() == 2 && disk.selection.arrangement.edges.size() >= 4,
            "point-tangent circular material components were joined");
}

void test_shared_material_seam_is_suppressed()
{
    AnalyticRequestPacketRecords records = records_for({{1, {1, 2}}});
    records.operands[0].geometry_kind = 1;
    records.operands[0].geometry_index = 0;
    records.operands[1].geometry_kind = 1;
    records.operands[1].geometry_index = 1;
    const auto append_region = [&](std::uint64_t region_id, std::uint64_t ring_id,
                                   const std::vector<std::pair<std::int64_t, std::int64_t>>& points)
    {
        const std::uint32_t ring_index = static_cast<std::uint32_t>(records.rings.size());
        const std::uint32_t vertex_begin = static_cast<std::uint32_t>(records.vertices.size());
        const std::uint32_t segment_begin = static_cast<std::uint32_t>(records.segments.size());
        for (std::uint32_t point = 0; point < points.size(); ++point)
            records.vertices.push_back(
                {ring_id * 100 + point, points[point].first, points[point].second});
        for (std::uint32_t segment = 0; segment < points.size(); ++segment)
            records.segments.push_back(
                {ring_id * 1000 + segment, ring_id * 1000 + segment, 1, 0, false, 0, 0});
        records.rings.push_back({ring_id, vertex_begin, static_cast<std::uint32_t>(points.size()),
                                 segment_begin, static_cast<std::uint32_t>(points.size()), 0});
        records.planar_regions.push_back({region_id, ring_index, 0, 0});
    };
    append_region(10, 100, {{0, 0}, {1000, 0}, {1000, 1000}, {0, 1000}});
    append_region(11, 101, {{1000, 0}, {2000, 0}, {2000, 1000}, {1000, 1000}});
    const AnalyticFilteredLoweringResult lowered =
        lower_analytic_job_to_filtered_curves(records, 0);
    require(lowered.error == AnalyticFilteredLoweringError::none && lowered.value,
            "shared material seam lowering failed");
    const AnalyticFilteredRegionsResult result = build(records, *lowered.value);
    require(result.error == AnalyticFilteredRegionsError::none && result.rings.size() == 1 &&
                result.regions.size() == 1 && result.ring_half_edges.size() == 6,
            "shared material/material seam was published as a result boundary");
}

void test_empty_and_exact_resource_boundaries()
{
    const AnalyticFilteredRegionsResult empty = build(records_for({}), {});
    require(empty.error == AnalyticFilteredRegionsError::none && empty.rings.empty() &&
                empty.regions.empty(),
            "empty job was not a successful no-op");

    AnalyticFilteredGeometry geometry;
    append_rectangle(geometry, 1, 0, 1000);
    const AnalyticRequestPacketRecords records = records_for({{1, {1}}});
    const AnalyticFilteredRegionsResult baseline = build(records, geometry);
    require(baseline.error == AnalyticFilteredRegionsError::none,
            "resource baseline regions failed");
    AnalyticSolverLimits exact;
    exact.predicate_calls = baseline.telemetry.predicate_calls;
    const AnalyticFilteredRegionsResult success = build(records, geometry, exact);
    require(success.error == AnalyticFilteredRegionsError::none,
            "exact region work ceiling failed");
    --exact.predicate_calls;
    const AnalyticFilteredRegionsResult failure = build(records, geometry, exact);
    require(failure.error == AnalyticFilteredRegionsError::resource_limit_exceeded &&
                failure.rings.empty() && failure.regions.empty(),
            "one-short region work ceiling did not fail closed");

    const AnalyticBroadPhaseResult broad = build_analytic_curve_candidates(geometry.bounds);
    require(broad.error == AnalyticBroadPhaseError::none,
            "region admission fixture broad phase failed");
    std::uint64_t low = 0;
    std::uint64_t high = baseline.telemetry.predicate_calls;
    while (low < high)
    {
        const std::uint64_t middle = low + (high - low) / 2;
        AnalyticSolverLimits probe_limits;
        probe_limits.predicate_calls = middle;
        const AnalyticFilteredRegionsResult probe =
            build_analytic_filtered_regions(records, 0, geometry, broad.pairs, probe_limits);
        if (probe.selection.telemetry.arrangement_predicate_calls == 0)
            low = middle + 1;
        else
            high = middle;
    }
    require(low > 0, "region admission threshold was not found");
    AnalyticSolverLimits one_short_admission;
    one_short_admission.predicate_calls = low - 1;
    const AnalyticFilteredRegionsResult early_failure =
        build_analytic_filtered_regions(records, 0, geometry, broad.pairs, one_short_admission);
    require(early_failure.error == AnalyticFilteredRegionsError::resource_limit_exceeded &&
                early_failure.selection.telemetry.arrangement_predicate_calls == 0 &&
                early_failure.selection.telemetry.arrangement_peak_working_memory_bytes == 0,
            "known-doomed region work ran arrangement before rejection");
}

void test_many_edge_ring_scales_without_seam_walks()
{
    const auto run = [](std::uint32_t segment_count)
    {
        AnalyticFilteredGeometry geometry;
        append_regular_polygon(geometry, 1, segment_count);
        return build(records_for({{1, {1}}}), geometry);
    };
    const AnalyticFilteredRegionsResult small = run(32);
    const AnalyticFilteredRegionsResult large = run(64);
    require(small.error == AnalyticFilteredRegionsError::none &&
                large.error == AnalyticFilteredRegionsError::none && small.rings.size() == 1 &&
                large.rings.size() == 1 &&
                large.ring_half_edges.size() == small.ring_half_edges.size() * 2,
            "many-edge material ring extraction failed");
    require(large.telemetry.region_work_units <= small.telemetry.region_work_units * 3 &&
                large.telemetry.peak_working_memory_bytes <=
                    small.telemetry.peak_working_memory_bytes * 3,
            "many-edge material ring regressed toward a seam-walk cross product");

    AnalyticFilteredGeometry memory_geometry;
    append_regular_polygon(memory_geometry, 1, 64);
    const AnalyticRequestPacketRecords memory_records = records_for({{1, {1}}});
    const AnalyticBroadPhaseResult memory_broad =
        build_analytic_curve_candidates(memory_geometry.bounds);
    require(memory_broad.error == AnalyticBroadPhaseError::none,
            "region memory fixture broad phase failed");
    std::uint64_t low = 0;
    std::uint64_t high = kAnalyticSolverHardLimits.working_memory_bytes;
    while (low < high)
    {
        const std::uint64_t middle = low + (high - low) / 2;
        AnalyticSolverLimits probe_limits;
        probe_limits.working_memory_bytes = middle;
        const AnalyticFilteredRegionsResult probe = build_analytic_filtered_regions(
            memory_records, 0, memory_geometry, memory_broad.pairs, probe_limits);
        if (probe.error == AnalyticFilteredRegionsError::none)
            high = middle;
        else
            low = middle + 1;
    }
    require(low > 0, "region memory admission threshold was not found");
    AnalyticSolverLimits exact_memory;
    exact_memory.working_memory_bytes = low;
    const AnalyticFilteredRegionsResult memory_success = build_analytic_filtered_regions(
        memory_records, 0, memory_geometry, memory_broad.pairs, exact_memory);
    require(memory_success.error == AnalyticFilteredRegionsError::none,
            "exact region live-memory admission failed");
    --exact_memory.working_memory_bytes;
    const AnalyticFilteredRegionsResult memory_failure = build_analytic_filtered_regions(
        memory_records, 0, memory_geometry, memory_broad.pairs, exact_memory);
    require(memory_failure.error == AnalyticFilteredRegionsError::resource_limit_exceeded &&
                memory_failure.selection.telemetry.arrangement_predicate_calls == 0 &&
                memory_failure.selection.telemetry.arrangement_peak_working_memory_bytes == 0,
            "one-byte-short region live memory ran arrangement before rejection");
}

void test_full_circle_memory_is_reserved_before_arrangement()
{
    AnalyticFilteredGeometry geometry;
    AnalyticAtomicCurveNm curve;
    curve.curve_index = 1;
    curve.kind = AnalyticAtomicCurveKind::circular_arc;
    curve.start = exact_point(312, 25);
    curve.end = exact_point(312, -25);
    curve.circle.center = exact_point(0, 0);
    curve.circle.radius = {313, 313};
    curve.counterclockwise = true;
    curve.major_arc = true;
    curve.construction_carrier_id = 1;
    curve.construction_family_id = 1;
    curve.has_arc_sweep_certificate = true;
    geometry.curves.push_back(curve);
    geometry.bounds.push_back({1, -313, -313, 313, 313});
    AnalyticFilteredOccurrence occurrence;
    occurrence.occurrence_id = 1;
    occurrence.coverage_id = 1;
    occurrence.agrees_with_carrier = true;
    occurrence.material_on_left = true;
    occurrence.source.operand_id = 1;
    occurrence.source.primary_id = 1;
    geometry.occurrences.push_back(occurrence);

    const AnalyticRequestPacketRecords records = records_for({{1, {1}}});
    std::uint64_t low = 0;
    std::uint64_t high = kAnalyticSolverHardLimits.working_memory_bytes;
    while (low < high)
    {
        const std::uint64_t middle = low + (high - low) / 2;
        AnalyticSolverLimits limits;
        limits.working_memory_bytes = middle;
        const AnalyticFilteredRegionsResult probe =
            build_analytic_filtered_regions(records, 0, geometry, {}, limits);
        if (probe.error == AnalyticFilteredRegionsError::none)
            high = middle;
        else
            low = middle + 1;
    }
    require(low > 0, "full-circle memory admission threshold was not found");
    AnalyticSolverLimits exact;
    exact.working_memory_bytes = low;
    require(build_analytic_filtered_regions(records, 0, geometry, {}, exact).error ==
                AnalyticFilteredRegionsError::none,
            "exact full-circle memory admission failed");
    --exact.working_memory_bytes;
    const AnalyticFilteredRegionsResult failure =
        build_analytic_filtered_regions(records, 0, geometry, {}, exact);
    require(failure.error == AnalyticFilteredRegionsError::resource_limit_exceeded &&
                failure.selection.telemetry.arrangement_predicate_calls == 0 &&
                failure.selection.telemetry.arrangement_peak_working_memory_bytes == 0,
            "one-byte-short full-circle memory ran arrangement before rejection");
}

std::string parity_vector()
{
    AnalyticFilteredGeometry nested_geometry;
    append_rectangle(nested_geometry, 1, 0, 1000);
    append_rectangle(nested_geometry, 2, 200, 800);
    append_rectangle(nested_geometry, 3, 400, 600);
    const AnalyticFilteredRegionsResult nested =
        build(records_for({{1, {1}}, {2, {2}}, {1, {3}}}), nested_geometry);

    AnalyticFilteredGeometry disjoint_geometry;
    append_rectangle(disjoint_geometry, 4, -2000, -1000);
    append_rectangle(disjoint_geometry, 5, 1000, 2000);
    const AnalyticFilteredRegionsResult disjoint =
        build(records_for({{1, {4, 5}}}), disjoint_geometry);
    require(nested.error == AnalyticFilteredRegionsError::none &&
                disjoint.error == AnalyticFilteredRegionsError::none,
            "regions parity fixtures failed");

    std::ostringstream output;
    output << std::hex << std::setfill('0');
    const auto append_u64 = [&output](std::uint64_t value) { output << std::setw(16) << value; };
    const auto append_result = [&](const AnalyticFilteredRegionsResult& result)
    {
        append_u64(static_cast<std::uint8_t>(result.error));
        append_u64(static_cast<std::uint64_t>(result.selection.origin_x_nm));
        append_u64(static_cast<std::uint64_t>(result.selection.origin_y_nm));
        append_u64(result.rings.size());
        for (const AnalyticFilteredMaterialRing& ring : result.rings)
        {
            append_u64(ring.half_edge_begin);
            append_u64(ring.half_edge_count);
            append_u64(ring.parent_ring);
            append_u64(ring.depth);
            append_u64(ring.counterclockwise ? 1 : 0);
        }
        append_u64(result.ring_half_edges.size());
        for (const std::uint32_t half_edge : result.ring_half_edges)
            append_u64(half_edge);
        append_u64(result.regions.size());
        for (const AnalyticFilteredMaterialRegion& region : result.regions)
        {
            append_u64(region.outer_ring);
            append_u64(region.material_component);
        }
        append_u64(result.face_components.size());
        for (const std::uint32_t component : result.face_components)
            append_u64(component);
        const AnalyticFilteredRegionsTelemetry& telemetry = result.telemetry;
        append_u64(telemetry.selection_predicate_calls);
        append_u64(telemetry.selection_peak_working_memory_bytes);
        append_u64(telemetry.disjoint_set_node_visits);
        append_u64(telemetry.boundary_half_edges);
        append_u64(telemetry.vertex_rotation_visits);
        append_u64(telemetry.emitted_rings);
        append_u64(telemetry.emitted_regions);
        append_u64(telemetry.sort_work_units);
        append_u64(telemetry.region_work_units);
        append_u64(telemetry.reserved_region_work_units);
        append_u64(telemetry.predicate_calls);
        append_u64(telemetry.peak_working_memory_bytes);
        append_u64(telemetry.algebraic_fallback_calls);
    };
    append_result(nested);
    append_result(disjoint);
    return output.str();
}

} // namespace

int main(int argc, char** argv)
{
    test_single_and_disjoint_regions();
    test_nested_annulus_and_island();
    test_collapsed_tangent_and_arc_topology();
    test_shared_material_seam_is_suppressed();
    test_empty_and_exact_resource_boundaries();
    test_many_edge_ring_scales_without_seam_walks();
    test_full_circle_memory_is_reserved_before_arrangement();
    if (argc == 2 && std::string(argv[1]) == "--emit-parity")
        std::cout << "ANALYTIC_FILTERED_REGIONS_VECTOR=" << parity_vector() << '\n';
    return 0;
}

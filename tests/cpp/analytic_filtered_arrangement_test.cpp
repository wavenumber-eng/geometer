#include "geometer/analytic_curve_broad_phase.h"
#include "geometer/analytic_filtered_arrangement.h"

#include "analytic_interval_index.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
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

AnalyticFilteredPointNm interval_point(double x, double y, double half_width)
{
    return {{x - half_width, x + half_width}, {y - half_width, y + half_width}};
}

AnalyticFilteredOccurrence occurrence(std::uint32_t index)
{
    AnalyticFilteredOccurrence value;
    value.occurrence_id = index;
    value.coverage_id = 1000 + index;
    value.agrees_with_carrier = true;
    value.material_on_left = true;
    value.source.operand_id = 2000 + index;
    value.source.primary_id = 3000 + index;
    value.source.secondary_id = 4000 + index;
    return value;
}

void append_line(AnalyticFilteredGeometry& geometry, double x1, double y1, double x2, double y2)
{
    const std::uint32_t index = static_cast<std::uint32_t>(geometry.curves.size() + 1);
    AnalyticAtomicCurveNm curve;
    curve.curve_index = index;
    curve.start = exact_point(x1, y1);
    curve.end = exact_point(x2, y2);
    curve.construction_carrier_id = 10000 + index;
    curve.construction_family_id = 20000 + index;
    geometry.curves.push_back(curve);
    geometry.bounds.push_back(
        {index, std::min(x1, x2), std::min(y1, y2), std::max(x1, x2), std::max(y1, y2)});
    geometry.occurrences.push_back(occurrence(index));
}

void append_line_curve(AnalyticFilteredGeometry& geometry, double x1, double y1, double x2,
                       double y2)
{
    append_line(geometry, x1, y1, x2, y2);
}

void append_arc_curve(AnalyticFilteredGeometry& geometry, double x1, double y1, double x2,
                      double y2, bool major)
{
    const std::uint32_t index = static_cast<std::uint32_t>(geometry.curves.size() + 1);
    AnalyticAtomicCurveNm curve;
    curve.curve_index = index;
    curve.kind = AnalyticAtomicCurveKind::circular_arc;
    curve.start = exact_point(x1, y1);
    curve.end = exact_point(x2, y2);
    curve.circle.center = exact_point(0, 0);
    curve.circle.radius = {100, 100};
    curve.counterclockwise = true;
    curve.major_arc = major;
    curve.construction_carrier_id = 30000 + index;
    curve.construction_family_id = 40000 + index;
    geometry.curves.push_back(curve);
    geometry.bounds.push_back({index, -100, -100, 100, 100});
    geometry.occurrences.push_back(occurrence(index));
}

AnalyticFilteredArrangementResult run_pipeline(const AnalyticFilteredGeometry& geometry)
{
    const AnalyticBroadPhaseResult broad = build_analytic_curve_candidates(geometry.bounds);
    require(broad.error == AnalyticBroadPhaseError::none, "arrangement broad phase failed");
    return build_analytic_filtered_arrangement(geometry, broad.pairs);
}

AnalyticFilteredArrangementResult arrange(const AnalyticFilteredGeometry& geometry,
                                          const AnalyticSolverLimits& limits = {})
{
    return build_analytic_filtered_arrangement(geometry, {}, limits);
}

void append_triangle(AnalyticFilteredGeometry& geometry, double anchor_x, double ax, double ay,
                     double bx, double by)
{
    append_line(geometry, anchor_x, 0, ax, ay);
    append_line(geometry, ax, ay, bx, by);
    append_line(geometry, bx, by, anchor_x, 0);
}

AnalyticFilteredGeometry square()
{
    AnalyticFilteredGeometry geometry;
    append_line(geometry, 0, 0, 1000, 0);
    append_line(geometry, 1000, 0, 1000, 1000);
    append_line(geometry, 1000, 1000, 0, 1000);
    append_line(geometry, 0, 1000, 0, 0);
    return geometry;
}

void verify_half_edge_links(const AnalyticFilteredArrangementResult& result)
{
    for (std::uint32_t index = 0; index < result.half_edges.size(); ++index)
    {
        const AnalyticArrangementHalfEdge& half_edge = result.half_edges[index];
        require(half_edge.twin < result.half_edges.size() &&
                    result.half_edges[half_edge.twin].twin == index,
                "half-edge twin links are not involutive");
        require(half_edge.next < result.half_edges.size() &&
                    result.half_edges[half_edge.next].previous == index,
                "half-edge next/previous links are inconsistent");
        require(half_edge.cycle < result.cycles.size(), "half-edge has no cycle");
    }
}

void test_square_topology()
{
    const AnalyticFilteredGeometry geometry = square();
    const AnalyticFilteredArrangementResult result = arrange(geometry);
    require(result.error == AnalyticFilteredArrangementError::none, "square arrangement failed");
    require(result.vertices.size() == 4 && result.edges.size() == 4 &&
                result.half_edges.size() == 8 && result.cycles.size() == 2,
            "square arrangement counts drifted");
    require(std::count_if(result.cycles.begin(), result.cycles.end(),
                          [](const AnalyticArrangementCycle& cycle)
                          { return cycle.counterclockwise; }) == 1,
            "square must have one bounded and one exterior cycle");
    require(result.memberships.size() == 4 && result.telemetry.algebraic_fallback_calls == 0,
            "square memberships or fallback telemetry drifted");
    verify_half_edge_links(result);
}

void test_circle_topology()
{
    AnalyticFilteredGeometry geometry;
    for (std::uint32_t local = 0; local < 2; ++local)
    {
        const std::uint32_t index = local + 1;
        AnalyticAtomicCurveNm curve;
        curve.curve_index = index;
        curve.kind = AnalyticAtomicCurveKind::circular_arc;
        curve.start = local == 0 ? exact_point(100, 0) : exact_point(-100, 0);
        curve.end = local == 0 ? exact_point(-100, 0) : exact_point(100, 0);
        curve.circle.center = exact_point(0, 0);
        curve.circle.radius = {100, 100};
        curve.counterclockwise = true;
        curve.construction_carrier_id = 77;
        curve.construction_family_id = 77;
        geometry.curves.push_back(curve);
        geometry.bounds.push_back({index, -100, -100, 100, 100});
        geometry.occurrences.push_back(occurrence(index));
    }
    const AnalyticFilteredArrangementResult result = arrange(geometry);
    require(result.error == AnalyticFilteredArrangementError::none,
            "two-semicircle arrangement failed");
    require(result.vertices.size() == 2 && result.edges.size() == 2 &&
                result.half_edges.size() == 4 && result.cycles.size() == 2,
            "two-semicircle topology counts drifted");
    require(std::count_if(result.cycles.begin(), result.cycles.end(),
                          [](const AnalyticArrangementCycle& cycle)
                          { return cycle.counterclockwise; }) == 1,
            "circle must have one bounded and one exterior cycle");
    verify_half_edge_links(result);
}

void test_nontransitive_resolution_chain()
{
    AnalyticFilteredGeometry geometry;
    append_triangle(geometry, 0, 10000, 1000, 10000, 3000);
    append_triangle(geometry, 40, 12000, 5000, 12000, 7000);
    append_triangle(geometry, 80, 14000, 9000, 14000, 11000);
    const AnalyticFilteredArrangementResult result = arrange(geometry);
    require(result.error == AnalyticFilteredArrangementError::none,
            "resolution-chain arrangement failed error=" +
                std::to_string(static_cast<int>(result.error)) +
                " overlay_work=" + std::to_string(result.telemetry.overlay_predicate_calls) +
                " work=" + std::to_string(result.telemetry.predicate_calls));
    require(result.vertices.size() == 8, "0/40/80 nm endpoint chain collapsed transitively");
    require(result.telemetry.merged_endpoint_records == 10,
            "resolution-chain endpoint merge accounting drifted");
}

AnalyticFilteredArrangementResult separated_squares(double gap)
{
    AnalyticFilteredGeometry geometry;
    append_line(geometry, -1000, 0, 0, 0);
    append_line(geometry, 0, 0, 0, 1000);
    append_line(geometry, 0, 1000, -1000, 1000);
    append_line(geometry, -1000, 1000, -1000, 0);
    append_line(geometry, gap, 0, 1000 + gap, 0);
    append_line(geometry, 1000 + gap, 0, 1000 + gap, -1000);
    append_line(geometry, 1000 + gap, -1000, gap, -1000);
    append_line(geometry, gap, -1000, gap, 0);
    return arrange(geometry);
}

void test_global_resolution_threshold()
{
    const AnalyticFilteredArrangementResult below = separated_squares(49);
    const AnalyticFilteredArrangementResult exact = separated_squares(50);
    const AnalyticFilteredArrangementResult above = separated_squares(51);
    require(below.error == AnalyticFilteredArrangementError::none && below.vertices.size() == 7,
            "49 nm global endpoint gap did not merge error=" +
                std::to_string(static_cast<int>(below.error)) +
                " merged=" + std::to_string(below.telemetry.merged_endpoint_records));
    require(exact.error == AnalyticFilteredArrangementError::none && exact.vertices.size() == 7,
            "50 nm global endpoint gap did not merge inclusively error=" +
                std::to_string(static_cast<int>(exact.error)) +
                " merged=" + std::to_string(exact.telemetry.merged_endpoint_records));
    require(above.error == AnalyticFilteredArrangementError::none && above.vertices.size() == 8,
            "51 nm global endpoint gap was collapsed");
}

void test_collapsed_span_lineage()
{
    AnalyticFilteredGeometry geometry;
    append_line(geometry, 0, 0, 30, 40);
    const AnalyticFilteredArrangementResult result = arrange(geometry);
    require(result.error == AnalyticFilteredArrangementError::none && result.vertices.size() == 1 &&
                result.edges.empty() && result.collapsed_spans.size() == 1 &&
                result.memberships.size() == 1 && result.cycles.empty(),
            "50 nm collapsed span lost its vertex or lineage error=" +
                std::to_string(static_cast<int>(result.error)) +
                " vertices=" + std::to_string(result.vertices.size()) +
                " edges=" + std::to_string(result.edges.size()) +
                " collapsed=" + std::to_string(result.collapsed_spans.size()) +
                " memberships=" + std::to_string(result.memberships.size()) +
                " overlay_work=" + std::to_string(result.telemetry.overlay_predicate_calls) +
                " work=" + std::to_string(result.telemetry.predicate_calls) +
                " memory=" + std::to_string(result.telemetry.peak_working_memory_bytes));
    require(std::all_of(result.collapsed_spans.begin(), result.collapsed_spans.end(),
                        [](const AnalyticArrangementCollapsedSpan& span)
                        { return span.vertex == 0 && span.membership_count == 1; }),
            "collapsed-span lineage ranges drifted");
}

void test_crossing_rectangles_pipeline()
{
    AnalyticFilteredGeometry geometry;
    append_line_curve(geometry, 0, 0, 1000, 0);
    append_line_curve(geometry, 1000, 0, 1000, 1000);
    append_line_curve(geometry, 1000, 1000, 0, 1000);
    append_line_curve(geometry, 0, 1000, 0, 0);
    append_line_curve(geometry, 500, 250, 1500, 250);
    append_line_curve(geometry, 1500, 250, 1500, 750);
    append_line_curve(geometry, 1500, 750, 500, 750);
    append_line_curve(geometry, 500, 750, 500, 250);

    const AnalyticFilteredArrangementResult result = run_pipeline(geometry);
    require(result.error == AnalyticFilteredArrangementError::none,
            "crossing-rectangle pipeline arrangement failed");
    require(result.vertices.size() == 10 && result.edges.size() == 12 &&
                result.half_edges.size() == 24 && result.cycles.size() == 4,
            "crossing-rectangle arrangement counts drifted");
    require(std::count_if(result.cycles.begin(), result.cycles.end(),
                          [](const AnalyticArrangementCycle& cycle)
                          { return cycle.counterclockwise; }) == 3,
            "crossing rectangles must expose three bounded cycles");
    verify_half_edge_links(result);
}

void test_right_partition_makes_major_arc_x_monotone()
{
    AnalyticFilteredGeometry geometry;
    append_arc_curve(geometry, -100, 0, 0, 100, true);
    append_line_curve(geometry, 0, 100, -100, 0);

    const AnalyticBroadPhaseResult broad = build_analytic_curve_candidates(geometry.bounds);
    require(broad.error == AnalyticBroadPhaseError::none, "major-arc partition broad phase failed");
    const AnalyticFilteredOverlayResult overlay =
        build_analytic_filtered_overlay(geometry, broad.pairs);
    require(overlay.error == AnalyticFilteredOverlayError::none,
            "major-arc partition overlay failed");
    require(std::count_if(overlay.spans.begin(), overlay.spans.end(),
                          [](const AnalyticAtomicSpanNm& span)
                          { return span.kind == AnalyticAtomicCurveKind::circular_arc; }) == 2,
            "major arc was not partitioned at its interior rightmost extremum");
    require(std::none_of(overlay.spans.begin(), overlay.spans.end(),
                         [](const AnalyticAtomicSpanNm& span) { return span.major_arc; }),
            "partitioned overlay retained a non-x-monotone major arc");

    const AnalyticFilteredArrangementResult result =
        build_analytic_filtered_arrangement(geometry, broad.pairs);
    require(result.error == AnalyticFilteredArrangementError::none && result.cycles.size() == 2,
            "partitioned major-arc arrangement failed error=" +
                std::to_string(static_cast<int>(result.error)) +
                " vertices=" + std::to_string(result.vertices.size()) +
                " edges=" + std::to_string(result.edges.size()) +
                " cycles=" + std::to_string(result.cycles.size()) +
                " emitted_v=" + std::to_string(result.telemetry.emitted_vertices) +
                " emitted_e=" + std::to_string(result.telemetry.emitted_edges) +
                " emitted_h=" + std::to_string(result.telemetry.emitted_half_edges));
    require(result.telemetry.overlay_predicate_calls == overlay.telemetry.predicate_calls &&
                result.telemetry.predicate_calls > overlay.telemetry.predicate_calls,
            "arrangement did not inherit overlay work accounting");
}

AnalyticFilteredArrangementResult irrational_capsule_arrangement()
{
    AnalyticRequestPacketRecords records;
    records.jobs = {{10, 0, 1}};
    records.stages = {{100, 1, 0, 2}};
    records.operands = {{1000, 4, 0}, {1001, 4, 1}};
    records.capsules = {
        {8000, -700, 1100, 1600, 2800, 211},
        {8001, -700, 1100, 1600, 2800, 211},
    };
    const AnalyticFilteredLoweringResult lowered =
        lower_analytic_job_to_filtered_curves(records, 0);
    require(lowered.error == AnalyticFilteredLoweringError::none && lowered.value.has_value(),
            "irrational duplicate capsules did not lower for arrangement");
    const AnalyticBroadPhaseResult broad = build_analytic_curve_candidates(lowered.value->bounds);
    require(broad.error == AnalyticBroadPhaseError::none,
            "irrational capsule arrangement broad phase failed");
    const AnalyticFilteredOverlayResult overlay =
        build_analytic_filtered_overlay(*lowered.value, broad.pairs);
    require(overlay.error == AnalyticFilteredOverlayError::none,
            "irrational capsule arrangement overlay failed");
    return build_analytic_filtered_arrangement(*lowered.value, broad.pairs);
}

void test_lowered_irrational_capsule_arrangement()
{
    const AnalyticFilteredArrangementResult result = irrational_capsule_arrangement();
    require(result.error == AnalyticFilteredArrangementError::none && result.cycles.size() == 2 &&
                result.memberships.size() >= result.edges.size() * 2 &&
                result.telemetry.algebraic_fallback_calls == 0,
            "irrational duplicate capsules failed filtered arrangement error=" +
                std::to_string(static_cast<int>(result.error)) +
                " vertices=" + std::to_string(result.vertices.size()) +
                " edges=" + std::to_string(result.edges.size()) +
                " cycles=" + std::to_string(result.cycles.size()));
}

AnalyticFilteredArrangementResult overlapping_disks_arrangement()
{
    AnalyticRequestPacketRecords records;
    records.jobs = {{10, 0, 1}};
    records.stages = {{100, 1, 0, 2}};
    records.operands = {{1000, 2, 0}, {1001, 2, 1}};
    records.disks = {{7000, 0, 0, 100}, {7001, 150, 0, 100}};
    const AnalyticFilteredLoweringResult lowered =
        lower_analytic_job_to_filtered_curves(records, 0);
    require(lowered.error == AnalyticFilteredLoweringError::none && lowered.value.has_value(),
            "overlapping disks did not lower");
    const AnalyticBroadPhaseResult broad = build_analytic_curve_candidates(lowered.value->bounds);
    require(broad.error == AnalyticBroadPhaseError::none && broad.pairs.size() == 6,
            "overlapping disks broad phase drifted");
    return build_analytic_filtered_arrangement(*lowered.value, broad.pairs);
}

void test_covering_span_carriers()
{
    const AnalyticFilteredArrangementResult disks = overlapping_disks_arrangement();
    require(disks.error == AnalyticFilteredArrangementError::none && disks.vertices.size() == 5 &&
                disks.edges.size() == 8 && disks.cycles.size() == 3,
            "overlapping irrational disks failed arrangement error=" +
                std::to_string(static_cast<int>(disks.error)) +
                " vertices=" + std::to_string(disks.vertices.size()) +
                " edges=" + std::to_string(disks.edges.size()) +
                " cycles=" + std::to_string(disks.cycles.size()));

    AnalyticFilteredGeometry lines;
    append_line(lines, 0, 0, 200, 0);
    append_line(lines, 200, 0, 200, 200);
    append_line(lines, 200, 200, 0, 200);
    append_line(lines, 0, 200, 0, 0);
    append_line(lines, 100, 0, 500, 0);
    append_line(lines, 500, 0, 500, 100);
    append_line(lines, 500, 100, 100, 100);
    append_line(lines, 100, 100, 100, 0);
    lines.curves[4].construction_carrier_id = lines.curves[0].construction_carrier_id;
    lines.curves[4].construction_family_id = lines.curves[0].construction_family_id;
    const AnalyticBroadPhaseResult broad = build_analytic_curve_candidates(lines.bounds);
    require(broad.error == AnalyticBroadPhaseError::none,
            "partial same-carrier line broad phase failed");
    const AnalyticFilteredOverlayResult line_overlay =
        build_analytic_filtered_overlay(lines, broad.pairs);
    require(line_overlay.error == AnalyticFilteredOverlayError::none,
            "partial same-carrier line overlay failed");
    for (const AnalyticAtomicSpanNm& span : line_overlay.spans)
    {
        const AnalyticFilteredPointCurveStatus start = classify_analytic_filtered_point_on_curve(
            lines.curves[span.carrier_curve_index - 1], span.start);
        const AnalyticFilteredPointCurveStatus end = classify_analytic_filtered_point_on_curve(
            lines.curves[span.carrier_curve_index - 1], span.end);
        require(start == AnalyticFilteredPointCurveStatus::certified_on_domain &&
                    end == AnalyticFilteredPointCurveStatus::certified_on_domain,
                "overlay did not publish a finite-domain covering line carrier span=" +
                    std::to_string(span.span_index) +
                    " carrier=" + std::to_string(span.carrier_curve_index) +
                    " start_status=" + std::to_string(static_cast<int>(start)) +
                    " end_status=" + std::to_string(static_cast<int>(end)));
    }
    const AnalyticFilteredArrangementResult partial =
        build_analytic_filtered_arrangement(lines, broad.pairs);
    require(partial.error == AnalyticFilteredArrangementError::none &&
                partial.memberships.size() > partial.edges.size(),
            "partial same-carrier lines lacked a covering span witness error=" +
                std::to_string(static_cast<int>(partial.error)) +
                " edges=" + std::to_string(partial.edges.size()) +
                " memberships=" + std::to_string(partial.memberships.size()) +
                " overlay_work=" + std::to_string(partial.telemetry.overlay_predicate_calls) +
                " work=" + std::to_string(partial.telemetry.predicate_calls) +
                " endpoints=" + std::to_string(partial.telemetry.endpoint_records) +
                " angular=" + std::to_string(partial.telemetry.angular_predicates));
}

AnalyticFilteredGeometry disjoint_squares(std::uint32_t count)
{
    AnalyticFilteredGeometry geometry;
    for (std::uint32_t index = 0; index < count; ++index)
    {
        const double x = static_cast<double>(index) * 10000.0;
        append_line(geometry, x, 0, x + 1000, 0);
        append_line(geometry, x + 1000, 0, x + 1000, 1000);
        append_line(geometry, x + 1000, 1000, x, 1000);
        append_line(geometry, x, 1000, x, 0);
    }
    return geometry;
}

void test_downstream_preflight_stops_before_overlay()
{
    const AnalyticFilteredGeometry geometry = disjoint_squares(32);
    const AnalyticFilteredArrangementResult baseline = arrange(geometry);
    require(baseline.error == AnalyticFilteredArrangementError::none &&
                baseline.telemetry.admission_work_units == geometry.curves.size() &&
                baseline.telemetry.peak_working_memory_bytes >
                    baseline.telemetry.overlay_peak_working_memory_bytes &&
                baseline.telemetry.predicate_calls > baseline.telemetry.overlay_predicate_calls,
            "downstream preflight baseline does not separate phase requirements");

    AnalyticSolverLimits limits;
    limits.working_memory_bytes = baseline.telemetry.overlay_peak_working_memory_bytes;
    AnalyticFilteredArrangementResult rejected = arrange(geometry, limits);
    require(rejected.error == AnalyticFilteredArrangementError::resource_limit_exceeded &&
                rejected.telemetry.overlay_predicate_calls == 0 &&
                rejected.telemetry.overlay_peak_working_memory_bytes == 0 &&
                rejected.telemetry.admission_work_units == geometry.curves.size() &&
                rejected.telemetry.predicate_calls == geometry.curves.size() &&
                rejected.telemetry.peak_working_memory_bytes == 0,
            "known-impossible arrangement allocated overlay memory before rejection");

    limits = {};
    limits.predicate_calls = baseline.telemetry.overlay_predicate_calls;
    rejected = arrange(geometry, limits);
    require(rejected.error == AnalyticFilteredArrangementError::resource_limit_exceeded &&
                rejected.telemetry.overlay_predicate_calls == 0 &&
                rejected.telemetry.admission_work_units == geometry.curves.size() &&
                rejected.telemetry.predicate_calls == geometry.curves.size(),
            "known-impossible arrangement performed overlay work before rejection");

    limits = {};
    limits.predicate_calls = geometry.curves.size() - 1;
    rejected = arrange(geometry, limits);
    require(rejected.error == AnalyticFilteredArrangementError::resource_limit_exceeded &&
                rejected.telemetry.admission_work_units == 0 &&
                rejected.telemetry.overlay_predicate_calls == 0 &&
                rejected.telemetry.predicate_calls == 0,
            "admission scan traversed curves before its bulk work charge");
}

void test_limits_and_malformed_inputs()
{
    const AnalyticFilteredGeometry geometry = square();
    const AnalyticFilteredArrangementResult baseline = arrange(geometry);
    require(baseline.error == AnalyticFilteredArrangementError::none, "limit baseline failed");

    AnalyticSolverLimits limits;
    limits.working_memory_bytes = baseline.telemetry.peak_working_memory_bytes;
    require(arrange(geometry, limits).error == AnalyticFilteredArrangementError::none,
            "exact arrangement memory budget failed");
    --limits.working_memory_bytes;
    require(arrange(geometry, limits).error ==
                AnalyticFilteredArrangementError::resource_limit_exceeded,
            "one-byte-short arrangement memory budget succeeded");

    limits = {};
    limits.predicate_calls = baseline.telemetry.predicate_calls;
    require(arrange(geometry, limits).error == AnalyticFilteredArrangementError::none,
            "exact arrangement work budget failed");
    --limits.predicate_calls;
    const AnalyticFilteredArrangementResult short_work = arrange(geometry, limits);
    require(short_work.error == AnalyticFilteredArrangementError::resource_limit_exceeded &&
                short_work.telemetry.predicate_calls == limits.predicate_calls,
            "one-unit-short arrangement work budget did not stop at its ceiling");

    AnalyticFilteredGeometry malformed_geometry = square();
    malformed_geometry.curves[0].kind = static_cast<AnalyticAtomicCurveKind>(255);
    require(arrange(malformed_geometry).error == AnalyticFilteredArrangementError::invalid_argument,
            "invalid curve kind was accepted");

    const std::vector<AnalyticCurvePair> malformed_pairs = {{2, 1}};
    require(build_analytic_filtered_arrangement(geometry, malformed_pairs).error ==
                AnalyticFilteredArrangementError::invalid_argument,
            "noncanonical candidate pair was accepted");

    limits = {};
    limits.predicate_calls = baseline.telemetry.overlay_predicate_calls - 1;
    const AnalyticFilteredArrangementResult overlay_short_work = arrange(geometry, limits);
    require(overlay_short_work.error == AnalyticFilteredArrangementError::resource_limit_exceeded &&
                overlay_short_work.vertices.empty() && overlay_short_work.edges.empty() &&
                overlay_short_work.telemetry.predicate_calls <= limits.predicate_calls,
            "internal overlay work bypassed the shared job ceiling");

    limits = {};
    limits.arrangement_vertices = 3;
    require(arrange(geometry, limits).error ==
                AnalyticFilteredArrangementError::resource_limit_exceeded,
            "arrangement vertex ceiling was bypassed");
    limits = {};
    limits.arrangement_half_edges = 6;
    require(arrange(geometry, limits).error ==
                AnalyticFilteredArrangementError::resource_limit_exceeded,
            "arrangement half-edge ceiling was bypassed");
}

void test_empty_arrangement()
{
    const AnalyticFilteredArrangementResult result = build_analytic_filtered_arrangement({}, {});
    require(result.error == AnalyticFilteredArrangementError::none && result.vertices.empty() &&
                result.edges.empty() && result.cycles.empty(),
            "empty arrangement was not a successful no-op");
}

void test_endpoint_index_budget_propagation()
{
    AnalyticFilteredGeometry geometry;
    constexpr std::uint32_t count = 256;
    for (std::uint32_t local = 0; local < count; ++local)
    {
        const std::uint32_t index = local + 1;
        const double far_y = 1000.0 + static_cast<double>(local) * 1000.0;
        AnalyticAtomicCurveNm curve;
        curve.curve_index = index;
        curve.start = exact_point(0, 0);
        curve.end = exact_point(100000, far_y);
        curve.construction_carrier_id = 50000 + index;
        curve.construction_family_id = 60000 + index;
        geometry.curves.push_back(curve);
        geometry.bounds.push_back({index, 0, 0, 100000, far_y});
        geometry.occurrences.push_back(occurrence(index));
    }
    AnalyticSolverLimits limits;
    limits.predicate_calls = 29960;
    const AnalyticFilteredArrangementResult result = arrange(geometry, limits);
    require(result.error == AnalyticFilteredArrangementError::resource_limit_exceeded &&
                result.vertices.empty() && result.edges.empty() &&
                result.telemetry.endpoint_index_node_visits != 0 &&
                result.telemetry.predicate_calls == limits.predicate_calls,
            "endpoint-index budget did not propagate exactly error=" +
                std::to_string(static_cast<int>(result.error)) +
                " visits=" + std::to_string(result.telemetry.endpoint_index_node_visits) +
                " work=" + std::to_string(result.telemetry.predicate_calls));
}

struct DenseIndexResult
{
    std::uint64_t node_visits = 0;
    std::uint64_t matches = 0;
    bool stopped = false;
};

DenseIndexResult dense_interval_index_result()
{
    constexpr std::uint32_t count = 128;
    constexpr std::uint64_t visit_limit = 4096;
    geometer::detail::AnalyticIntervalIndex index(count);
    DenseIndexResult result;
    for (std::uint32_t item = 0; item < count; ++item)
    {
        const bool completed = index.query(-30.0, 30.0, result.node_visits, visit_limit,
                                           [&](std::size_t, std::uint32_t)
                                           {
                                               ++result.matches;
                                               return true;
                                           });
        if (!completed)
        {
            result.stopped = true;
            break;
        }
        require(index.insert(-30.0, 30.0, item, item + 1), "dense interval-index insert failed");
    }
    return result;
}

void test_dense_interval_index_stops_at_budget()
{
    const DenseIndexResult result = dense_interval_index_result();
    require(result.stopped && result.node_visits == 4096 && result.matches > 4000,
            "dense overlapping interval index did not stop exactly at its visit ceiling");
}

AnalyticFilteredArrangementResult wedge_fan(std::uint32_t count)
{
    AnalyticFilteredGeometry geometry;
    for (std::uint32_t index = 0; index < count; ++index)
    {
        const double lower = static_cast<double>(index) * 2000.0 + 100.0;
        append_triangle(geometry, 0, 100000, lower, 100000, lower + 800.0);
    }
    return arrange(geometry);
}

void test_indexed_scaling()
{
    const AnalyticFilteredArrangementResult small = wedge_fan(64);
    const AnalyticFilteredArrangementResult large = wedge_fan(128);
    require(small.error == AnalyticFilteredArrangementError::none &&
                large.error == AnalyticFilteredArrangementError::none,
            "indexed wedge-fan scaling fixture failed");
    require(large.telemetry.endpoint_index_node_visits <=
                small.telemetry.endpoint_index_node_visits * 3,
            "endpoint reconciliation scaled quadratically");
    require(large.telemetry.predicate_calls <= small.telemetry.predicate_calls * 3,
            "arrangement work scaled quadratically");
}

std::string parity_vector()
{
    const AnalyticFilteredGeometry geometry = square();
    const AnalyticFilteredArrangementResult square_result = arrange(geometry);
    const AnalyticFilteredArrangementResult threshold_result = separated_squares(50);

    AnalyticFilteredGeometry collapsed_geometry;
    append_line(collapsed_geometry, 0, 0, 30, 40);
    const AnalyticFilteredArrangementResult collapsed_result = arrange(collapsed_geometry);
    const AnalyticFilteredArrangementResult capsule_result = irrational_capsule_arrangement();
    const AnalyticFilteredArrangementResult disk_result = overlapping_disks_arrangement();
    const DenseIndexResult dense_index = dense_interval_index_result();
    require(square_result.error == AnalyticFilteredArrangementError::none &&
                threshold_result.error == AnalyticFilteredArrangementError::none &&
                collapsed_result.error == AnalyticFilteredArrangementError::none &&
                capsule_result.error == AnalyticFilteredArrangementError::none &&
                disk_result.error == AnalyticFilteredArrangementError::none && dense_index.stopped,
            "arrangement parity fixture failed");

    std::ostringstream output;
    output << std::hex << std::setfill('0');
    const auto append_u64 = [&output](std::uint64_t value) { output << std::setw(16) << value; };
    const auto append_double = [&append_u64](double value)
    {
        std::uint64_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        append_u64(bits);
    };
    const auto append_point = [&append_double](const AnalyticFilteredPointNm& point)
    {
        append_double(point.x.lower);
        append_double(point.x.upper);
        append_double(point.y.lower);
        append_double(point.y.upper);
    };
    const auto append_result = [&](const AnalyticFilteredArrangementResult& result)
    {
        append_u64(static_cast<std::uint8_t>(result.error));
        append_u64(result.vertices.size());
        for (const AnalyticArrangementVertexNm& vertex : result.vertices)
        {
            append_point(vertex.point);
            append_u64(vertex.outgoing_begin);
            append_u64(vertex.outgoing_count);
        }
        append_u64(result.edges.size());
        for (const AnalyticArrangementEdgeNm& edge : result.edges)
        {
            append_u64(edge.start_vertex);
            append_u64(edge.end_vertex);
            append_u64(edge.carrier_curve_index);
            append_u64(static_cast<std::uint8_t>(edge.kind));
            append_point(edge.carrier_start);
            append_point(edge.carrier_end);
            append_point(edge.circle.center);
            append_double(edge.circle.radius.lower);
            append_double(edge.circle.radius.upper);
            append_u64(edge.counterclockwise ? 1 : 0);
            append_u64(edge.major_arc ? 1 : 0);
            append_u64(edge.membership_begin);
            append_u64(edge.membership_count);
        }
        append_u64(result.half_edges.size());
        for (const AnalyticArrangementHalfEdge& half_edge : result.half_edges)
        {
            append_u64(half_edge.origin_vertex);
            append_u64(half_edge.twin);
            append_u64(half_edge.next);
            append_u64(half_edge.previous);
            append_u64(half_edge.edge);
            append_u64(half_edge.forward ? 1 : 0);
            append_u64(half_edge.cycle);
        }
        append_u64(result.outgoing_half_edges.size());
        for (std::uint32_t half_edge : result.outgoing_half_edges)
            append_u64(half_edge);
        append_u64(result.collapsed_spans.size());
        for (const AnalyticArrangementCollapsedSpan& span : result.collapsed_spans)
        {
            append_u64(span.vertex);
            append_u64(span.carrier_curve_index);
            append_u64(span.membership_begin);
            append_u64(span.membership_count);
        }
        append_u64(result.memberships.size());
        for (const AnalyticSpanMembership& membership : result.memberships)
        {
            append_u64(membership.curve_index);
            append_u64(membership.agrees_with_span ? 1 : 0);
            append_u64(membership.material_on_span_left ? 1 : 0);
        }
        append_u64(result.cycles.size());
        for (const AnalyticArrangementCycle& cycle : result.cycles)
        {
            append_u64(cycle.half_edge_begin);
            append_u64(cycle.half_edge_count);
            append_u64(cycle.component);
            append_u64(cycle.counterclockwise ? 1 : 0);
        }
        append_u64(result.cycle_half_edges.size());
        for (std::uint32_t half_edge : result.cycle_half_edges)
            append_u64(half_edge);
        append_u64(result.telemetry.admission_work_units);
        append_u64(result.telemetry.endpoint_records);
        append_u64(result.telemetry.endpoint_index_node_visits);
        append_u64(result.telemetry.endpoint_index_update_work_units);
        append_u64(result.telemetry.merged_endpoint_records);
        append_u64(result.telemetry.collapsed_spans);
        append_u64(result.telemetry.sort_work_units);
        append_u64(result.telemetry.angular_predicates);
        append_u64(result.telemetry.predicate_calls);
        append_u64(result.telemetry.peak_working_memory_bytes);
        append_u64(result.telemetry.algebraic_fallback_calls);
    };
    append_result(square_result);
    append_result(threshold_result);
    append_result(collapsed_result);
    append_result(capsule_result);
    append_result(disk_result);
    append_u64(dense_index.node_visits);
    append_u64(dense_index.matches);
    append_u64(dense_index.stopped ? 1 : 0);
    return output.str();
}

} // namespace

int main()
{
    test_square_topology();
    test_circle_topology();
    test_nontransitive_resolution_chain();
    test_global_resolution_threshold();
    test_collapsed_span_lineage();
    test_crossing_rectangles_pipeline();
    test_right_partition_makes_major_arc_x_monotone();
    test_lowered_irrational_capsule_arrangement();
    test_covering_span_carriers();
    test_downstream_preflight_stops_before_overlay();
    test_limits_and_malformed_inputs();
    test_empty_arrangement();
    test_endpoint_index_budget_propagation();
    test_dense_interval_index_stops_at_budget();
    test_indexed_scaling();
    std::cout << "ANALYTIC_FILTERED_ARRANGEMENT_VECTOR=" << parity_vector() << '\n';
    return 0;
}

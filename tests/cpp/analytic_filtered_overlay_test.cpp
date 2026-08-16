#include "geometer/analytic_filtered_overlay.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
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

AnalyticAtomicCurveNm line(std::uint32_t index, double x1, double y1, double x2, double y2,
                           std::uint64_t carrier)
{
    AnalyticAtomicCurveNm curve;
    curve.curve_index = index;
    curve.start = exact_point(x1, y1);
    curve.end = exact_point(x2, y2);
    curve.construction_carrier_id = carrier;
    curve.construction_family_id = carrier;
    return curve;
}

AnalyticAtomicCurveNm arc(std::uint32_t index, double x1, double y1, double x2, double y2,
                          bool counterclockwise, bool major, std::uint64_t carrier)
{
    AnalyticAtomicCurveNm curve;
    curve.curve_index = index;
    curve.kind = AnalyticAtomicCurveKind::circular_arc;
    curve.start = exact_point(x1, y1);
    curve.end = exact_point(x2, y2);
    curve.circle.center = exact_point(0, 0);
    curve.circle.radius = {100, 100};
    curve.counterclockwise = counterclockwise;
    curve.major_arc = major;
    curve.construction_carrier_id = carrier;
    curve.construction_family_id = carrier;
    return curve;
}

AnalyticFilteredOccurrence occurrence(std::uint32_t index, bool agrees = true,
                                      bool material_left = true)
{
    AnalyticFilteredOccurrence value;
    value.occurrence_id = index;
    value.coverage_id = 1000 + index;
    value.agrees_with_carrier = agrees;
    value.material_on_left = material_left;
    value.source.operand_id = 1000 + index;
    value.source.primary_id = 2000 + index;
    value.source.secondary_id = 3000 + index;
    return value;
}

void append_curve(AnalyticFilteredGeometry& geometry, AnalyticAtomicCurveNm curve,
                  AnalyticFilteredOccurrence source)
{
    geometry.bounds.push_back({curve.curve_index, std::min(curve.start.x.lower, curve.end.x.lower),
                               std::min(curve.start.y.lower, curve.end.y.lower),
                               std::max(curve.start.x.upper, curve.end.x.upper),
                               std::max(curve.start.y.upper, curve.end.y.upper)});
    geometry.curves.push_back(curve);
    geometry.occurrences.push_back(source);
}

AnalyticNarrowPhaseResult empty_narrow()
{
    return {};
}

void require_span(const AnalyticFilteredOverlayResult& result, std::size_t index, double start_x,
                  double start_y, double end_x, double end_y, std::uint32_t memberships)
{
    require(index < result.spans.size(), "missing expected overlay span");
    const AnalyticAtomicSpanNm& span = result.spans[index];
    require(span.start.x.lower == start_x && span.start.y.lower == start_y &&
                span.end.x.lower == end_x && span.end.y.lower == end_y &&
                span.membership_count == memberships,
            "overlay span geometry or membership count drifted");
}

void test_partial_line_overlap_and_orientation()
{
    AnalyticFilteredGeometry geometry;
    append_curve(geometry, line(1, 0, 0, 1000, 0, 10), occurrence(1));
    append_curve(geometry, line(2, 1500, 0, 500, 0, 10), occurrence(2, false, true));
    AnalyticNarrowPhaseResult narrow;
    narrow.intersections.push_back({{1, 2}, AnalyticPairRelation::coincident});
    const AnalyticFilteredOverlayResult result = build_analytic_filtered_overlay(geometry, narrow);
    require(result.error == AnalyticFilteredOverlayError::none && result.spans.size() == 3 &&
                result.memberships.size() == 4,
            "partial line same-domain overlay failed error=" +
                std::to_string(static_cast<int>(result.error)) +
                " spans=" + std::to_string(result.spans.size()) +
                " memberships=" + std::to_string(result.memberships.size()));
    require_span(result, 0, 0, 0, 500, 0, 1);
    require_span(result, 1, 500, 0, 1000, 0, 2);
    require_span(result, 2, 1000, 0, 1500, 0, 1);
    require(result.memberships[1].curve_index == 1 && result.memberships[2].curve_index == 2 &&
                !result.memberships[2].agrees_with_span &&
                !result.memberships[2].material_on_span_left,
            "canonical span membership orientation drifted");
}

AnalyticFilteredOverlayResult separated_lines(double gap)
{
    AnalyticFilteredGeometry geometry;
    append_curve(geometry, line(1, 0, 0, 1000, 0, 10), occurrence(1));
    append_curve(geometry, line(2, 1000 + gap, 0, 2000, 0, 10), occurrence(2));
    return build_analytic_filtered_overlay(geometry, empty_narrow());
}

void test_resolution_merge_threshold()
{
    const AnalyticFilteredOverlayResult merged = separated_lines(49);
    require(merged.error == AnalyticFilteredOverlayError::none && merged.spans.size() == 2 &&
                merged.telemetry.resolution_merges != 0,
            "49 nm same-carrier endpoint gap was not merged");
    const AnalyticFilteredOverlayResult exact = separated_lines(50);
    require(exact.error == AnalyticFilteredOverlayError::none && exact.spans.size() == 2 &&
                exact.telemetry.resolution_merges != 0,
            "50 nm same-carrier endpoint gap was not merged inclusively");
    const AnalyticFilteredOverlayResult preserved = separated_lines(51);
    require(preserved.error == AnalyticFilteredOverlayError::none && preserved.spans.size() == 2 &&
                preserved.telemetry.resolution_merges == 0,
            "51 nm same-carrier endpoint gap was collapsed");
    require_span(preserved, 0, 0, 0, 1000, 0, 1);
    require_span(preserved, 1, 1051, 0, 2000, 0, 1);
}

void test_crossing_split_events()
{
    AnalyticFilteredGeometry geometry;
    append_curve(geometry, line(1, 0, 0, 1000, 0, 10), occurrence(1));
    append_curve(geometry, line(2, 500, -500, 500, 500, 20), occurrence(2));
    AnalyticPairIntersection intersection;
    intersection.pair = {1, 2};
    intersection.relation = AnalyticPairRelation::point;
    intersection.point_count = 1;
    intersection.points[0] = exact_point(500, 0);
    AnalyticNarrowPhaseResult narrow;
    narrow.intersections.push_back(intersection);
    const AnalyticFilteredOverlayResult result = build_analytic_filtered_overlay(geometry, narrow);
    require(result.error == AnalyticFilteredOverlayError::none && result.spans.size() == 4 &&
                result.memberships.size() == 4,
            "candidate intersection was not attached to both carrier groups");
    require_span(result, 0, 0, 0, 500, 0, 1);
    require_span(result, 1, 500, 0, 1000, 0, 1);
    require_span(result, 2, 500, -500, 500, 0, 1);
    require_span(result, 3, 500, 0, 500, 500, 1);
}

void test_circle_seam_and_coincident_arcs()
{
    AnalyticFilteredGeometry geometry;
    append_curve(geometry, arc(1, 100, 0, 0, 100, true, false, 30), occurrence(1));
    append_curve(geometry, arc(2, 0, 100, 100, 0, false, false, 30), occurrence(2, false, true));
    AnalyticNarrowPhaseResult narrow;
    narrow.intersections.push_back({{1, 2}, AnalyticPairRelation::coincident});
    const AnalyticFilteredOverlayResult result = build_analytic_filtered_overlay(geometry, narrow);
    require(result.error == AnalyticFilteredOverlayError::none && result.spans.size() == 1 &&
                result.memberships.size() == 2 &&
                result.spans[0].kind == AnalyticAtomicCurveKind::circular_arc &&
                !result.spans[0].major_arc,
            "coincident reversed quarter arcs did not overlay once");
    require_span(result, 0, 100, 0, 0, 100, 2);

    AnalyticFilteredGeometry disk;
    append_curve(disk, arc(1, -100, 0, 100, 0, true, false, 40), occurrence(1));
    append_curve(disk, arc(2, 100, 0, -100, 0, true, false, 40), occurrence(2));
    const AnalyticFilteredOverlayResult halves =
        build_analytic_filtered_overlay(disk, empty_narrow());
    require(halves.error == AnalyticFilteredOverlayError::none && halves.spans.size() == 2 &&
                halves.memberships.size() == 2 && !halves.spans[0].major_arc &&
                !halves.spans[1].major_arc,
            "paired semicircle seam partition failed");
    require_span(halves, 0, -100, 0, 100, 0, 1);
    require_span(halves, 1, 100, 0, -100, 0, 1);

    AnalyticFilteredGeometry almost_full;
    AnalyticAtomicCurveNm major = arc(1, 1000, 20, 1000, -20, true, true, 50);
    major.circle.radius = {1000, 1000};
    major.has_arc_sweep_certificate = true;
    append_curve(almost_full, major, occurrence(1));
    const AnalyticFilteredOverlayResult bridged =
        build_analytic_filtered_overlay(almost_full, empty_narrow());
    require(bridged.error == AnalyticFilteredOverlayError::none && bridged.spans.size() == 2 &&
                bridged.memberships.size() == 2 && bridged.telemetry.collapsed_domains == 1 &&
                bridged.spans[0].start.x.lower == -1000 && bridged.spans[0].end.x.lower == 1000 &&
                bridged.spans[1].start.x.lower == 1000 && bridged.spans[1].end.x.lower == -1000,
            "near-seam major arc did not bridge its <=50 nm endpoint gap canonically");
}

void test_lowered_irrational_capsule_pipeline()
{
    AnalyticRequestPacketRecords records;
    records.jobs = {{10, 0, 1}};
    records.stages = {{100, 1, 0, 2}};
    records.operands = {{1000, 4, 0}, {1001, 4, 1}};
    records.capsules = {{8000, -700, 1100, 1600, 2800, 211}, {8001, -700, 1100, 1600, 2800, 211}};
    const AnalyticFilteredLoweringResult lowered =
        lower_analytic_job_to_filtered_curves(records, 0);
    require(lowered.error == AnalyticFilteredLoweringError::none && lowered.value.has_value(),
            "irrational duplicate capsules did not lower");
    const AnalyticBroadPhaseResult broad = build_analytic_curve_candidates(lowered.value->bounds);
    require(broad.error == AnalyticBroadPhaseError::none,
            "irrational duplicate capsules failed broad phase");
    const AnalyticNarrowPhaseResult narrow =
        intersect_analytic_curve_candidates(lowered.value->curves, broad.pairs);
    require(narrow.error == AnalyticNarrowPhaseError::none,
            "irrational duplicate capsules failed narrow phase");
    const AnalyticFilteredOverlayResult overlay =
        build_analytic_filtered_overlay(*lowered.value, narrow);
    require(overlay.error == AnalyticFilteredOverlayError::none && !overlay.spans.empty() &&
                overlay.memberships.size() >= overlay.spans.size() * 2 &&
                overlay.telemetry.algebraic_fallback_calls == 0,
            "irrational duplicate capsules failed filtered same-domain overlay error=" +
                std::to_string(static_cast<int>(overlay.error)) +
                " spans=" + std::to_string(overlay.spans.size()) +
                " memberships=" + std::to_string(overlay.memberships.size()) +
                " groups=" + std::to_string(overlay.telemetry.carrier_groups) +
                " raw=" + std::to_string(overlay.telemetry.raw_events) +
                " unique=" + std::to_string(overlay.telemetry.unique_events) +
                " collapsed=" + std::to_string(overlay.telemetry.collapsed_domains) +
                " updates=" + std::to_string(overlay.telemetry.active_set_updates) +
                " visits=" + std::to_string(overlay.telemetry.membership_visits) +
                " work=" + std::to_string(overlay.telemetry.predicate_calls));
    for (const AnalyticAtomicSpanNm& span : overlay.spans)
        require(span.start.x.lower <= span.start.x.upper &&
                    span.start.y.lower <= span.start.y.upper &&
                    span.end.x.lower <= span.end.x.upper && span.end.y.lower <= span.end.y.upper,
                "irrational capsule overlay returned an invalid coordinate enclosure");
}

AnalyticFilteredGeometry sparse_lines(std::uint32_t count)
{
    AnalyticFilteredGeometry geometry;
    for (std::uint32_t index = 0; index < count; ++index)
    {
        const double x = static_cast<double>(index) * 1000.0;
        append_curve(geometry, line(index + 1, x, 0, x + 100, 0, index + 1), occurrence(index + 1));
    }
    return geometry;
}

void test_limits_and_sparse_scaling()
{
    const AnalyticFilteredOverlayResult empty =
        build_analytic_filtered_overlay(AnalyticFilteredGeometry{}, empty_narrow());
    require(empty.error == AnalyticFilteredOverlayError::none && empty.spans.empty() &&
                empty.memberships.empty() && empty.telemetry.predicate_calls == 0,
            "empty filtered overlay was not a deterministic no-op");

    const AnalyticFilteredOverlayResult small =
        build_analytic_filtered_overlay(sparse_lines(64), empty_narrow());
    const AnalyticFilteredOverlayResult large =
        build_analytic_filtered_overlay(sparse_lines(128), empty_narrow());
    require(small.error == AnalyticFilteredOverlayError::none &&
                large.error == AnalyticFilteredOverlayError::none && small.spans.size() == 64 &&
                large.spans.size() == 128 &&
                large.telemetry.predicate_calls <= small.telemetry.predicate_calls * 3,
            "sparse overlay work did not remain near n-log-n at 2x input");

    AnalyticFilteredGeometry duplicates;
    for (std::uint32_t index = 0; index < 8; ++index)
        append_curve(duplicates, line(index + 1, 0, 0, 100, 0, 10), occurrence(index + 1));
    AnalyticSolverLimits limits = kAnalyticSolverHardLimits;
    limits.source_reference_memberships = 7;
    AnalyticFilteredOverlayResult result =
        build_analytic_filtered_overlay(duplicates, empty_narrow(), limits);
    require(result.error == AnalyticFilteredOverlayError::resource_limit_exceeded &&
                result.spans.empty() && result.memberships.empty(),
            "same-domain membership limit did not fail before output allocation");

    const AnalyticFilteredOverlayResult success =
        build_analytic_filtered_overlay(duplicates, empty_narrow());
    require(success.error == AnalyticFilteredOverlayError::none && success.spans.size() == 1 &&
                success.memberships.size() == 8,
            "duplicate-line overlay fixture failed");
    limits = kAnalyticSolverHardLimits;
    limits.working_memory_bytes = success.telemetry.peak_working_memory_bytes - 1;
    result = build_analytic_filtered_overlay(duplicates, empty_narrow(), limits);
    require(result.error == AnalyticFilteredOverlayError::resource_limit_exceeded &&
                result.spans.empty() && result.memberships.empty(),
            "one-byte-short overlay memory limit did not fail closed");
    limits = kAnalyticSolverHardLimits;
    limits.predicate_calls = success.telemetry.predicate_calls - 1;
    result = build_analytic_filtered_overlay(duplicates, empty_narrow(), limits);
    require(result.error == AnalyticFilteredOverlayError::resource_limit_exceeded &&
                result.telemetry.predicate_calls <= limits.predicate_calls &&
                result.telemetry.predicate_calls != 0,
            "one-unit-short overlay work limit did not stop deterministically");

    limits = kAnalyticSolverHardLimits;
    limits.arrangement_vertices = 1;
    result = build_analytic_filtered_overlay(sparse_lines(1), empty_narrow(), limits);
    require(result.error == AnalyticFilteredOverlayError::resource_limit_exceeded &&
                result.spans.empty(),
            "overlay vertex ceiling did not account for both line endpoints");
}

std::string parity_vector()
{
    AnalyticFilteredGeometry geometry;
    append_curve(geometry, line(1, 0, 0, 1000, 0, 10), occurrence(1));
    append_curve(geometry, line(2, 1500, 0, 500, 0, 10), occurrence(2, false, true));
    append_curve(geometry, line(3, 750, -500, 750, 500, 20), occurrence(3));
    AnalyticNarrowPhaseResult narrow;
    narrow.intersections.push_back({{1, 2}, AnalyticPairRelation::coincident});
    AnalyticPairIntersection first;
    first.pair = {1, 3};
    first.relation = AnalyticPairRelation::point;
    first.point_count = 1;
    first.points[0] = exact_point(750, 0);
    narrow.intersections.push_back(first);
    AnalyticPairIntersection second = first;
    second.pair = {2, 3};
    narrow.intersections.push_back(second);
    const AnalyticFilteredOverlayResult result = build_analytic_filtered_overlay(geometry, narrow);
    require(result.error == AnalyticFilteredOverlayError::none,
            "filtered overlay parity fixture failed");
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    const auto append_u64 = [&output](std::uint64_t value) { output << std::setw(16) << value; };
    const auto append_double = [&append_u64](double value)
    {
        std::uint64_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        append_u64(bits);
    };
    append_u64(result.spans.size());
    for (const AnalyticAtomicSpanNm& span : result.spans)
    {
        append_u64(span.span_index);
        append_u64(span.carrier_curve_index);
        append_u64(static_cast<std::uint8_t>(span.kind));
        append_double(span.start.x.lower);
        append_double(span.start.x.upper);
        append_double(span.start.y.lower);
        append_double(span.start.y.upper);
        append_double(span.end.x.lower);
        append_double(span.end.x.upper);
        append_double(span.end.y.lower);
        append_double(span.end.y.upper);
        append_u64(span.major_arc ? 1 : 0);
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
    append_u64(result.telemetry.raw_events);
    append_u64(result.telemetry.unique_events);
    append_u64(result.telemetry.resolution_merges);
    append_u64(result.telemetry.collapsed_domains);
    append_u64(result.telemetry.emitted_spans);
    append_u64(result.telemetry.emitted_memberships);
    append_u64(result.telemetry.peak_working_memory_bytes);
    return output.str();
}

} // namespace

int main()
{
    test_partial_line_overlap_and_orientation();
    test_resolution_merge_threshold();
    test_crossing_split_events();
    test_circle_seam_and_coincident_arcs();
    test_lowered_irrational_capsule_pipeline();
    test_limits_and_sparse_scaling();
    std::cout << "ANALYTIC_FILTERED_OVERLAY_VECTOR=" << parity_vector() << '\n';
    return 0;
}

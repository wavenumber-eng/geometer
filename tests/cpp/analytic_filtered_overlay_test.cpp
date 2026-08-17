#include "geometer/analytic_filtered_overlay.h"

#include "analytic_endpoint_arc_reconstruction.h"
#include "analytic_filtered_execution_policy.h"

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

AnalyticFilteredPointNm interval_point(double x, double y, double half_width)
{
    return {{x - half_width, x + half_width}, {y - half_width, y + half_width}};
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

AnalyticAtomicCurveNm endpoint_authoritative_arc(std::uint32_t index, std::int64_t start_y,
                                                 std::uint64_t carrier)
{
    AnalyticAtomicCurveNm curve;
    curve.curve_index = index;
    curve.kind = AnalyticAtomicCurveKind::circular_arc;
    curve.start = exact_point(100.0, static_cast<double>(start_y));
    curve.end = exact_point(-100.0, static_cast<double>(start_y));
    curve.integer_start = {100, start_y};
    curve.integer_end = {-100, start_y};
    curve.circle.radius = {100.0, 100.0};
    curve.counterclockwise = true;
    curve.has_integer_radius_certificate = true;
    curve.integer_radius = 100;
    curve.has_arc_sweep_certificate = true;
    curve.has_endpoint_authoritative_arc_certificate = true;
    curve.has_endpoint_authoritative_x_monotone_certificate = true;
    curve.endpoint_authoritative_upper_branch = true;
    analytic_detail::Point center;
    require(analytic_detail::reconstruct_endpoint_authoritative_arc_center(
                100, start_y, -100, start_y, 100, true, false, center),
            "test endpoint-authoritative center reconstruction failed");
    curve.circle.center = {{center.x.lower, center.x.upper}, {center.y.lower, center.y.upper}};
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
    const AnalyticFilteredOverlayResult result =
        build_analytic_filtered_overlay(geometry, {{1, 2}});
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
    return build_analytic_filtered_overlay(geometry, {});
}

void test_resolution_merge_threshold()
{
    AnalyticFilteredGeometry collapsed_geometry;
    append_curve(collapsed_geometry, line(1, 0, 0, 30, 40, 10), occurrence(1));
    const AnalyticFilteredOverlayResult collapsed =
        build_analytic_filtered_overlay(collapsed_geometry, {});
    require(collapsed.error == AnalyticFilteredOverlayError::none && collapsed.spans.empty() &&
                collapsed.memberships.empty() && collapsed.telemetry.collapsed_domains == 1,
            "standalone 50 nm domain was not retained as a successful resolution collapse");

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

void test_endpoint_partition_tokens_are_canonical()
{
    AnalyticFilteredGeometry geometry;
    auto first = endpoint_authoritative_arc(1, 0, 10);
    auto second = endpoint_authoritative_arc(2, 1, 20);
    const std::uint64_t forged = analytic_endpoint_arc_partition_column_token(1, true);
    first.start.construction_x_column_id = forged;
    second.start.construction_x_column_id = forged;
    append_curve(geometry, first, occurrence(1));
    append_curve(geometry, second, occurrence(2));
    require(build_analytic_filtered_overlay(geometry, {}).error ==
                AnalyticFilteredOverlayError::invalid_argument,
            "unrelated endpoint/cardinal identities must not share a correlation token");

    geometry.curves[1].start.construction_x_column_id =
        analytic_endpoint_arc_partition_column_token(2, true);
    require(build_analytic_filtered_overlay(geometry, {}).error ==
                AnalyticFilteredOverlayError::none,
            "canonical endpoint/cardinal correlation groups were rejected");
    require(analytic_execution_detail::build_overlay(
                geometry, {}, kAnalyticSolverHardLimits,
                analytic_execution_detail::kStrictPublishedGeometry)
                    .error == AnalyticFilteredOverlayError::none,
            "strict overlay rejected trusted endpoint-authoritative construction roots");

    geometry.curves[0].start.construction_x_column_id = kAnalyticEndpointArcRightColumnTag;
    require(build_analytic_filtered_overlay(geometry, {}).error ==
                AnalyticFilteredOverlayError::invalid_argument,
            "a zero-payload endpoint/cardinal token must be rejected");
}

void test_strict_unresolved_event_equality_fails_closed()
{
    AnalyticFilteredGeometry geometry;
    AnalyticAtomicCurveNm first = line(1, 0, 0, 1000, 0, 10);
    AnalyticAtomicCurveNm second = line(2, 1000, 0, 2000, 0, 10);
    first.end = interval_point(1000, 0, 0.5);
    second.start = interval_point(1000, 0, 0.5);
    append_curve(geometry, first, occurrence(1));
    append_curve(geometry, second, occurrence(2));
    const AnalyticFilteredOverlayResult result = analytic_execution_detail::build_overlay(
        geometry, {}, kAnalyticSolverHardLimits,
        analytic_execution_detail::kStrictPublishedGeometry);
    require(result.error == AnalyticFilteredOverlayError::resource_limit_exceeded &&
                result.telemetry.unresolved_predicate_failure,
            "strict overlay guessed equality for unresolved filtered events");
}

AnalyticFilteredOverlayResult diagonal_endpoint_pair(double offset)
{
    AnalyticFilteredGeometry geometry;
    append_curve(geometry, line(1, -1000, 0, 0, 0, 10), occurrence(1));
    append_curve(geometry, line(2, offset, offset, offset, 1000, 20), occurrence(2));
    return build_analytic_filtered_overlay(geometry, {{1, 2}});
}

AnalyticFilteredOverlayResult parallel_pair(double gap)
{
    AnalyticFilteredGeometry geometry;
    append_curve(geometry, line(1, 0, 0, 1000, 0, 10), occurrence(1));
    append_curve(geometry, line(2, 0, gap, 1000, gap, 20), occurrence(2));
    return build_analytic_filtered_overlay(geometry, {{1, 2}});
}

void test_integrated_pair_resolution_boundary()
{
    const AnalyticFilteredOverlayResult diagonal_35 = diagonal_endpoint_pair(35);
    const AnalyticFilteredOverlayResult diagonal_36 = diagonal_endpoint_pair(36);
    require(diagonal_35.error == AnalyticFilteredOverlayError::none &&
                diagonal_35.telemetry.input_point_intersections == 1 &&
                diagonal_36.error == AnalyticFilteredOverlayError::none &&
                diagonal_36.telemetry.input_point_intersections == 0,
            "integrated overlay must accept a 35/35 nm endpoint bridge and reject 36/36 nm");

    AnalyticFilteredGeometry diagonal_geometry;
    append_curve(diagonal_geometry, line(1, -1000, 0, 0, 0, 10), occurrence(1));
    append_curve(diagonal_geometry, line(2, 35, 35, 35, 1000, 20), occurrence(2));
    AnalyticSolverLimits limits = kAnalyticSolverHardLimits;
    limits.predicate_calls = diagonal_35.telemetry.predicate_calls - 1;
    AnalyticFilteredOverlayResult limited =
        build_analytic_filtered_overlay(diagonal_geometry, {{1, 2}}, limits);
    require(limited.error == AnalyticFilteredOverlayError::resource_limit_exceeded &&
                limited.telemetry.predicate_calls <= limits.predicate_calls,
            "pair-level witness work escaped the integrated predicate budget");
    limits = kAnalyticSolverHardLimits;
    limits.working_memory_bytes = diagonal_35.telemetry.peak_working_memory_bytes - 1;
    limited = build_analytic_filtered_overlay(diagonal_geometry, {{1, 2}}, limits);
    require(limited.error == AnalyticFilteredOverlayError::resource_limit_exceeded &&
                limited.spans.empty(),
            "retained narrow pairs escaped the integrated live-memory budget");

    const AnalyticFilteredOverlayResult parallel_50 = parallel_pair(50);
    const AnalyticFilteredOverlayResult parallel_51 = parallel_pair(51);
    require(parallel_50.error == AnalyticFilteredOverlayError::none &&
                parallel_51.error == AnalyticFilteredOverlayError::none &&
                parallel_50.telemetry.input_point_intersections == 0 &&
                parallel_51.telemetry.input_point_intersections == 0 &&
                parallel_50.spans.size() == 2 && parallel_51.spans.size() == 2,
            "parallel 50/51 nm carriers must not acquire an injected split point");
}

void test_crossing_split_events()
{
    AnalyticFilteredGeometry geometry;
    append_curve(geometry, line(1, 0, 0, 1000, 0, 10), occurrence(1));
    append_curve(geometry, line(2, 500, -500, 500, 500, 20), occurrence(2));
    const AnalyticFilteredOverlayResult result =
        build_analytic_filtered_overlay(geometry, {{1, 2}});
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
    const AnalyticFilteredOverlayResult result =
        build_analytic_filtered_overlay(geometry, {{1, 2}});
    require(result.error == AnalyticFilteredOverlayError::none && result.spans.size() == 1 &&
                result.memberships.size() == 2 &&
                result.spans[0].kind == AnalyticAtomicCurveKind::circular_arc &&
                !result.spans[0].major_arc,
            "coincident reversed quarter arcs did not overlay once");
    require_span(result, 0, 100, 0, 0, 100, 2);

    AnalyticFilteredGeometry disk;
    append_curve(disk, arc(1, -100, 0, 100, 0, true, false, 40), occurrence(1));
    append_curve(disk, arc(2, 100, 0, -100, 0, true, false, 40), occurrence(2));
    const AnalyticFilteredOverlayResult halves = build_analytic_filtered_overlay(disk, {});
    require(halves.error == AnalyticFilteredOverlayError::none && halves.spans.size() == 2 &&
                halves.memberships.size() == 2 && !halves.spans[0].major_arc &&
                !halves.spans[1].major_arc,
            "paired semicircle seam partition failed");
    require_span(halves, 0, -100, 0, 100, 0, 1);
    require_span(halves, 1, 100, 0, -100, 0, 1);

    AnalyticFilteredGeometry almost_full;
    AnalyticAtomicCurveNm major = arc(1, 312, 25, 312, -25, true, true, 50);
    major.circle.radius = {313, 313};
    major.has_arc_sweep_certificate = true;
    append_curve(almost_full, major, occurrence(1));
    const AnalyticFilteredOverlayResult bridged = build_analytic_filtered_overlay(almost_full, {});
    require(bridged.error == AnalyticFilteredOverlayError::none && bridged.spans.size() == 2 &&
                bridged.memberships.size() == 2 && bridged.telemetry.collapsed_domains == 1 &&
                bridged.spans[0].start.x.lower == -313 && bridged.spans[0].end.x.lower == 313 &&
                bridged.spans[1].start.x.lower == 313 && bridged.spans[1].end.x.lower == -313,
            "near-seam major arc did not bridge its <=50 nm endpoint gap canonically");
    AnalyticSolverLimits exact_work;
    exact_work.predicate_calls = bridged.telemetry.predicate_calls;
    require(build_analytic_filtered_overlay(almost_full, {}, exact_work).error ==
                AnalyticFilteredOverlayError::none,
            "exact circular seam-support work budget failed");
    --exact_work.predicate_calls;
    const AnalyticFilteredOverlayResult short_work =
        build_analytic_filtered_overlay(almost_full, {}, exact_work);
    require(short_work.error == AnalyticFilteredOverlayError::resource_limit_exceeded &&
                short_work.telemetry.predicate_calls == exact_work.predicate_calls,
            "one-unit-short circular seam-support work was not fully metered");
}

void test_invalid_pipeline_inputs_fail_closed()
{
    AnalyticFilteredGeometry geometry;
    append_curve(geometry, line(1, 0, 0, 1000, 0, 10), occurrence(1));
    append_curve(geometry, line(2, 500, -500, 500, 500, 20), occurrence(2));
    AnalyticFilteredOverlayResult result = build_analytic_filtered_overlay(geometry, {{2, 1}});
    require(result.error == AnalyticFilteredOverlayError::invalid_argument && result.spans.empty(),
            "noncanonical candidate pair was accepted by the integrated narrow boundary");

    AnalyticFilteredGeometry invalid_curve = geometry;
    invalid_curve.curves[0].kind = static_cast<AnalyticAtomicCurveKind>(255);
    result = build_analytic_filtered_overlay(invalid_curve, {});
    require(result.error == AnalyticFilteredOverlayError::invalid_argument,
            "invalid curve-kind discriminant was accepted");

    invalid_curve = geometry;
    invalid_curve.curves[0].kind = AnalyticAtomicCurveKind::circular_arc;
    invalid_curve.curves[0].circle.center = exact_point(0, 0);
    invalid_curve.curves[0].circle.radius = {-1, -1};
    result = build_analytic_filtered_overlay(invalid_curve, {});
    require(result.error == AnalyticFilteredOverlayError::invalid_argument,
            "invalid circular carrier fields were accepted");
}

AnalyticFilteredOverlayResult circular_sort_cycle_result()
{
    constexpr double radius = 1414213495.9050577;
    constexpr double half_width = 33.0;
    constexpr double coordinates[3][2] = {
        {999999920.0, 999999922.0},
        {999999952.0, 999999954.0},
        {999999983.0, 999999985.0},
    };
    AnalyticFilteredGeometry geometry;
    for (std::uint32_t index = 0; index < 3; ++index)
    {
        AnalyticAtomicCurveNm curve;
        curve.curve_index = index + 1;
        curve.kind = AnalyticAtomicCurveKind::circular_arc;
        curve.start = interval_point(coordinates[index][0], coordinates[index][1], half_width);
        curve.end = interval_point(-coordinates[index][0], -coordinates[index][1], half_width);
        curve.circle.center = exact_point(0, 0);
        curve.circle.radius = {radius, radius};
        curve.counterclockwise = true;
        curve.major_arc = false;
        curve.construction_carrier_id = 90;
        curve.construction_family_id = 90;
        curve.has_arc_sweep_certificate = true;
        append_curve(geometry, curve, occurrence(index + 1));
    }
    for (std::uint32_t index = 0; index < 3; ++index)
        append_curve(geometry,
                     line(index + 4, coordinates[index][0], coordinates[index][1],
                          coordinates[index][0] + 1000.0, coordinates[index][1], 100 + index),
                     occurrence(index + 4));

    std::vector<AnalyticCurvePair> candidates;
    for (std::uint32_t index = 0; index < 3; ++index)
        candidates.push_back({index + 1, index + 4});
    return build_analytic_filtered_overlay(geometry, candidates);
}

void test_circular_preorder_is_total_and_fails_closed()
{
    const AnalyticFilteredOverlayResult result = circular_sort_cycle_result();
    require(result.error != AnalyticFilteredOverlayError::none && result.spans.empty(),
            "adversarial circular-order input did not terminate deterministically before output "
            "error=" +
                std::to_string(static_cast<int>(result.error)) +
                " spans=" + std::to_string(result.spans.size()) +
                " raw=" + std::to_string(result.telemetry.raw_events) +
                " unique=" + std::to_string(result.telemetry.unique_events));
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
        build_analytic_filtered_overlay(*lowered.value, broad.pairs);
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
        build_analytic_filtered_overlay(AnalyticFilteredGeometry{}, {});
    require(empty.error == AnalyticFilteredOverlayError::none && empty.spans.empty() &&
                empty.memberships.empty() && empty.telemetry.predicate_calls == 0,
            "empty filtered overlay was not a deterministic no-op");

    const AnalyticFilteredOverlayResult small =
        build_analytic_filtered_overlay(sparse_lines(64), {});
    const AnalyticFilteredOverlayResult large =
        build_analytic_filtered_overlay(sparse_lines(128), {});
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
    AnalyticFilteredOverlayResult result = build_analytic_filtered_overlay(duplicates, {}, limits);
    require(result.error == AnalyticFilteredOverlayError::resource_limit_exceeded &&
                result.spans.empty() && result.memberships.empty(),
            "same-domain membership limit did not fail before output allocation");

    const AnalyticFilteredOverlayResult success = build_analytic_filtered_overlay(duplicates, {});
    require(success.error == AnalyticFilteredOverlayError::none && success.spans.size() == 1 &&
                success.memberships.size() == 8,
            "duplicate-line overlay fixture failed");
    limits = kAnalyticSolverHardLimits;
    limits.working_memory_bytes = success.telemetry.peak_working_memory_bytes - 1;
    result = build_analytic_filtered_overlay(duplicates, {}, limits);
    require(result.error == AnalyticFilteredOverlayError::resource_limit_exceeded &&
                result.spans.empty() && result.memberships.empty(),
            "one-byte-short overlay memory limit did not fail closed");
    limits = kAnalyticSolverHardLimits;
    limits.predicate_calls = success.telemetry.predicate_calls - 1;
    result = build_analytic_filtered_overlay(duplicates, {}, limits);
    require(result.error == AnalyticFilteredOverlayError::resource_limit_exceeded &&
                result.telemetry.predicate_calls <= limits.predicate_calls &&
                result.telemetry.predicate_calls != 0,
            "one-unit-short overlay work limit did not stop deterministically");

    limits = kAnalyticSolverHardLimits;
    limits.arrangement_vertices = 1;
    result = build_analytic_filtered_overlay(sparse_lines(1), {}, limits);
    require(result.error == AnalyticFilteredOverlayError::resource_limit_exceeded &&
                result.spans.empty(),
            "overlay vertex ceiling did not account for both line endpoints");
}

void test_endpoint_column_work_precedes_event_allocation()
{
    const AnalyticFilteredGeometry geometry = sparse_lines(4);
    const AnalyticNarrowPhaseResult narrow =
        intersect_analytic_curve_candidates(geometry.curves, {});
    require(narrow.error == AnalyticNarrowPhaseError::none,
            "endpoint-column precharge fixture narrow phase failed");
    const std::uint64_t initial_memory =
        narrow.telemetry.peak_working_memory_bytes +
        geometry.curves.size() * kAnalyticOverlayCurveGroupLogicalBytes;
    AnalyticSolverLimits limits = kAnalyticSolverHardLimits;
    limits.predicate_calls = 31;
    const AnalyticFilteredOverlayResult one_short =
        build_analytic_filtered_overlay(geometry, {}, limits);
    require(one_short.error == AnalyticFilteredOverlayError::resource_limit_exceeded &&
                one_short.telemetry.predicate_calls == 12 &&
                one_short.telemetry.peak_working_memory_bytes == initial_memory &&
                one_short.telemetry.raw_events == 0,
            "one-short endpoint-column work escaped the pre-allocation gate: work=" +
                std::to_string(one_short.telemetry.predicate_calls) +
                " peak=" + std::to_string(one_short.telemetry.peak_working_memory_bytes) +
                " initial=" + std::to_string(initial_memory) +
                " raw=" + std::to_string(one_short.telemetry.raw_events));

    limits.predicate_calls = 32;
    const AnalyticFilteredOverlayResult exact =
        build_analytic_filtered_overlay(geometry, {}, limits);
    require(exact.error == AnalyticFilteredOverlayError::resource_limit_exceeded &&
                exact.telemetry.predicate_calls == 32 &&
                exact.telemetry.peak_working_memory_bytes > initial_memory,
            "exact endpoint-column work did not enter the governed event phase");
}

AnalyticFilteredOverlayResult impossible_combined_memory_result()
{
    AnalyticFilteredGeometry geometry;
    append_curve(geometry, line(1, 0, 0, 1000, 0, 10), occurrence(1));
    append_curve(geometry, line(2, 500, -500, 500, 500, 20), occurrence(2));
    AnalyticSolverLimits limits = kAnalyticSolverHardLimits;
    limits.working_memory_bytes = kAnalyticNarrowPhasePairLogicalBytes;
    return build_analytic_filtered_overlay(geometry, {{1, 2}}, limits);
}

AnalyticFilteredOverlayResult impossible_combined_work_result()
{
    AnalyticFilteredGeometry geometry;
    append_curve(geometry, line(1, 0, 0, 1000, 0, 10), occurrence(1));
    append_curve(geometry, line(2, 500, -500, 500, 500, 20), occurrence(2));
    AnalyticSolverLimits limits = kAnalyticSolverHardLimits;
    limits.predicate_calls = 0;
    return build_analytic_filtered_overlay(geometry, {{1, 2}}, limits);
}

void test_integrated_preflight_rejects_before_narrow_work()
{
    const AnalyticFilteredOverlayResult memory = impossible_combined_memory_result();
    require(memory.error == AnalyticFilteredOverlayError::resource_limit_exceeded &&
                memory.spans.empty() && memory.memberships.empty() &&
                memory.telemetry.narrow_phase_predicate_calls == 0 &&
                memory.telemetry.narrow_phase_peak_working_memory_bytes == 0 &&
                memory.telemetry.predicate_calls == 0 &&
                memory.telemetry.peak_working_memory_bytes == 0,
            "known-impossible combined memory escaped the allocation-free preflight");

    const AnalyticFilteredOverlayResult work = impossible_combined_work_result();
    require(
        work.error == AnalyticFilteredOverlayError::resource_limit_exceeded && work.spans.empty() &&
            work.memberships.empty() && work.telemetry.narrow_phase_predicate_calls == 0 &&
            work.telemetry.narrow_phase_peak_working_memory_bytes == 0 &&
            work.telemetry.predicate_calls == 0 && work.telemetry.peak_working_memory_bytes == 0,
        "known-impossible combined work escaped the allocation-free preflight");
}

std::string parity_vector()
{
    AnalyticFilteredGeometry geometry;
    append_curve(geometry, line(1, 0, 0, 1000, 0, 10), occurrence(1));
    append_curve(geometry, line(2, 1500, 0, 500, 0, 10), occurrence(2, false, true));
    append_curve(geometry, line(3, 750, -500, 750, 500, 20), occurrence(3));
    const AnalyticFilteredOverlayResult result =
        build_analytic_filtered_overlay(geometry, {{1, 2}, {1, 3}, {2, 3}});
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
    const AnalyticFilteredOverlayResult circular_cycle = circular_sort_cycle_result();
    append_u64(static_cast<std::uint8_t>(circular_cycle.error));
    append_u64(circular_cycle.telemetry.raw_events);
    append_u64(circular_cycle.telemetry.unique_events);
    append_u64(circular_cycle.telemetry.predicate_calls);
    const AnalyticFilteredOverlayResult impossible_memory = impossible_combined_memory_result();
    append_u64(static_cast<std::uint8_t>(impossible_memory.error));
    append_u64(impossible_memory.telemetry.narrow_phase_predicate_calls);
    append_u64(impossible_memory.telemetry.narrow_phase_peak_working_memory_bytes);
    append_u64(impossible_memory.telemetry.predicate_calls);
    append_u64(impossible_memory.telemetry.peak_working_memory_bytes);
    const AnalyticFilteredOverlayResult impossible_work = impossible_combined_work_result();
    append_u64(static_cast<std::uint8_t>(impossible_work.error));
    append_u64(impossible_work.telemetry.narrow_phase_predicate_calls);
    append_u64(impossible_work.telemetry.narrow_phase_peak_working_memory_bytes);
    append_u64(impossible_work.telemetry.predicate_calls);
    append_u64(impossible_work.telemetry.peak_working_memory_bytes);
    return output.str();
}

} // namespace

int main()
{
    test_partial_line_overlap_and_orientation();
    test_resolution_merge_threshold();
    test_endpoint_partition_tokens_are_canonical();
    test_strict_unresolved_event_equality_fails_closed();
    test_integrated_pair_resolution_boundary();
    test_crossing_split_events();
    test_circle_seam_and_coincident_arcs();
    test_invalid_pipeline_inputs_fail_closed();
    test_circular_preorder_is_total_and_fails_closed();
    test_lowered_irrational_capsule_pipeline();
    test_limits_and_sparse_scaling();
    test_endpoint_column_work_precedes_event_allocation();
    test_integrated_preflight_rejects_before_narrow_work();
    std::cout << "ANALYTIC_FILTERED_OVERLAY_VECTOR=" << parity_vector() << '\n';
    return 0;
}

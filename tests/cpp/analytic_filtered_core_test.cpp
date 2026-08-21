#include "geometer/analytic_curve_broad_phase.h"
#include "geometer/analytic_curve_narrow_phase.h"

#include "analytic_filtered_execution_policy.h"
#include "analytic_filtered_interval.h"
#include "geometer/analytic_numeric_filter.h"
#include "geometer/analytic_solver_limits.h"

#include "analytic_endpoint_arc_reconstruction.h"

#include <cmath>
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

void test_exact_zero_interval_multiplication()
{
    using analytic_detail::exact;
    using analytic_detail::Interval;
    using analytic_detail::multiply;

    const Interval range{-17.0, 23.0};
    const Interval left = multiply(exact(0.0), range);
    const Interval right = multiply(range, exact(-0.0));
    require(left.lower == 0.0 && left.upper == 0.0 && right.lower == 0.0 && right.upper == 0.0,
            "exact zero times a valid interval must remain exact zero");
}

bool same_pairs(const std::vector<AnalyticCurvePair>& actual,
                const std::vector<AnalyticCurvePair>& expected)
{
    if (actual.size() != expected.size())
        return false;
    for (std::size_t index = 0; index < actual.size(); ++index)
    {
        if (actual[index].first != expected[index].first ||
            actual[index].second != expected[index].second)
            return false;
    }
    return true;
}

AnalyticFilteredPointNm filtered_point(double x, double y)
{
    return {{x, x}, {y, y}};
}

AnalyticAtomicCurveNm line(std::uint32_t index, std::int64_t x1, std::int64_t y1, std::int64_t x2,
                           std::int64_t y2)
{
    AnalyticAtomicCurveNm result;
    result.curve_index = index;
    result.kind = AnalyticAtomicCurveKind::line;
    result.start = filtered_point(static_cast<double>(x1), static_cast<double>(y1));
    result.end = filtered_point(static_cast<double>(x2), static_cast<double>(y2));
    result.has_integer_certificate = true;
    result.integer_start = {x1, y1};
    result.integer_end = {x2, y2};
    return result;
}

AnalyticAtomicCurveNm filtered_line(std::uint32_t index, double x1, double y1, double x2, double y2)
{
    AnalyticAtomicCurveNm result;
    result.curve_index = index;
    result.kind = AnalyticAtomicCurveKind::line;
    result.start = filtered_point(x1, y1);
    result.end = filtered_point(x2, y2);
    return result;
}

AnalyticAtomicCurveNm filtered_arc(std::uint32_t index, double x1, double y1, double x2, double y2,
                                   double cx, double cy, AnalyticCoordinateIntervalNm radius,
                                   bool counterclockwise, bool major = false)
{
    AnalyticAtomicCurveNm result;
    result.curve_index = index;
    result.kind = AnalyticAtomicCurveKind::circular_arc;
    result.start = filtered_point(x1, y1);
    result.end = filtered_point(x2, y2);
    result.circle.center = filtered_point(cx, cy);
    result.circle.radius = radius;
    result.counterclockwise = counterclockwise;
    result.major_arc = major;
    return result;
}

AnalyticAtomicCurveNm arc(std::uint32_t index, std::int64_t x1, std::int64_t y1, std::int64_t x2,
                          std::int64_t y2, std::int64_t cx, std::int64_t cy, std::uint64_t radius,
                          bool counterclockwise, bool major = false)
{
    AnalyticAtomicCurveNm result = line(index, x1, y1, x2, y2);
    result.kind = AnalyticAtomicCurveKind::circular_arc;
    result.circle.center = {{static_cast<double>(cx), static_cast<double>(cx)},
                            {static_cast<double>(cy), static_cast<double>(cy)}};
    result.circle.radius = {static_cast<double>(radius), static_cast<double>(radius)};
    result.counterclockwise = counterclockwise;
    result.major_arc = major;
    result.integer_center = {cx, cy};
    result.has_integer_radius_certificate = true;
    result.integer_radius = radius;
    return result;
}

AnalyticAtomicCurveNm endpoint_authoritative_arc(std::uint32_t index, std::int64_t x1,
                                                 std::int64_t y1, std::int64_t x2, std::int64_t y2,
                                                 std::uint64_t radius, bool counterclockwise,
                                                 bool upper_branch)
{
    AnalyticAtomicCurveNm result;
    result.curve_index = index;
    result.kind = AnalyticAtomicCurveKind::circular_arc;
    result.start = filtered_point(static_cast<double>(x1), static_cast<double>(y1));
    result.end = filtered_point(static_cast<double>(x2), static_cast<double>(y2));
    result.integer_start = {x1, y1};
    result.integer_end = {x2, y2};
    result.circle.radius = {static_cast<double>(radius), static_cast<double>(radius)};
    result.counterclockwise = counterclockwise;
    result.has_integer_radius_certificate = true;
    result.integer_radius = radius;
    result.has_arc_sweep_certificate = true;
    result.has_endpoint_authoritative_arc_certificate = true;
    result.has_endpoint_authoritative_x_monotone_certificate = true;
    result.endpoint_authoritative_upper_branch = upper_branch;
    geometer::analytic_detail::Point center;
    require(geometer::analytic_detail::reconstruct_endpoint_authoritative_arc_center(
                x1, y1, x2, y2, radius, counterclockwise, false, center),
            "test endpoint-authoritative center reconstruction failed");
    result.circle.center = {{center.x.lower, center.x.upper}, {center.y.lower, center.y.upper}};
    return result;
}

bool contains(AnalyticCoordinateIntervalNm interval, double value)
{
    return interval.lower <= value && value <= interval.upper;
}

std::string narrow_phase_parity_vector()
{
    const std::vector<AnalyticAtomicCurveNm> curves = {
        arc(1, 100, 0, -100, 0, 0, 0, 100, true),
        arc(2, 190, 0, -10, 0, 90, 0, 100, true),
        line(3, -200, 100, 200, 100),
    };
    const AnalyticNarrowPhaseResult result =
        intersect_analytic_curve_candidates(curves, {{1, 2}, {1, 3}});
    require(result.error == AnalyticNarrowPhaseError::none && result.intersections.size() == 2,
            "narrow-phase parity vector failed");
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    auto append_u64 = [&output](std::uint64_t value) { output << std::setw(16) << value; };
    auto append_double = [&append_u64](double value)
    {
        std::uint64_t bits = 0;
        static_assert(sizeof(bits) == sizeof(value));
        std::memcpy(&bits, &value, sizeof(bits));
        append_u64(bits);
    };
    for (const AnalyticPairIntersection& intersection : result.intersections)
    {
        append_u64(intersection.pair.first);
        append_u64(intersection.pair.second);
        append_u64(static_cast<std::uint8_t>(intersection.relation));
        append_u64(intersection.point_count);
        append_u64(intersection.resolution_collapsed ? 1U : 0U);
        for (std::uint8_t index = 0; index < intersection.point_count; ++index)
        {
            append_double(intersection.points[index].x.lower);
            append_double(intersection.points[index].x.upper);
            append_double(intersection.points[index].y.lower);
            append_double(intersection.points[index].y.upper);
            append_u64(intersection.points[index].construction_x_column_id);
        }
    }
    append_u64(result.telemetry.candidate_pairs_consumed);
    append_u64(result.telemetry.curve_table_entries);
    append_u64(result.telemetry.curve_references_resolved);
    append_u64(result.telemetry.predicate_calls);
    append_u64(result.telemetry.square_root_calls);
    append_u64(result.telemetry.tangent_contacts);
    append_u64(result.telemetry.point_intersections);
    append_u64(result.telemetry.peak_working_memory_bytes);
    append_u64(result.telemetry.algebraic_fallback_calls);

    const std::vector<AnalyticAtomicCurveNm> endpoint_pairs = {
        line(1, -1000, 0, 0, 0),  line(2, 35, 35, 35, 1000), line(3, 36, 36, 36, 1000),
        line(4, -1000, 0, 10, 0), line(5, 0, -1000, 0, 10),
    };
    const AnalyticNarrowPhaseResult endpoint_result =
        intersect_analytic_curve_candidates(endpoint_pairs, {{1, 2}, {1, 3}, {4, 5}});
    require(endpoint_result.error == AnalyticNarrowPhaseError::none &&
                endpoint_result.intersections.size() == 3,
            "pair-level endpoint parity fixture failed");
    for (const AnalyticPairIntersection& intersection : endpoint_result.intersections)
    {
        append_u64(static_cast<std::uint8_t>(intersection.relation));
        append_u64(intersection.resolution_collapsed ? 1U : 0U);
    }
    const std::vector<AnalyticAtomicCurveNm> authoritative = {
        endpoint_authoritative_arc(1, 0, 0, 1000, 0, 600, true, false),
        line(2, 0, 0, -1000, 0),
    };
    const auto authoritative_result = intersect_analytic_curve_candidates(authoritative, {{1, 2}});
    require(authoritative_result.error == AnalyticNarrowPhaseError::none &&
                authoritative_result.intersections.size() == 1 &&
                authoritative_result.intersections[0].point_count == 1,
            "endpoint-authoritative parity fixture failed");
    append_u64(static_cast<std::uint8_t>(authoritative_result.intersections[0].relation));
    append_double(authoritative_result.intersections[0].points[0].x.lower);
    append_double(authoritative_result.intersections[0].points[0].y.lower);
    return output.str();
}

void test_endpoint_authoritative_arc_certificate()
{
    std::vector<AnalyticAtomicCurveNm> curves = {
        endpoint_authoritative_arc(1, 0, 0, 1000, 0, 600, true, false),
        line(2, 0, 0, -1000, 0),
    };
    const auto result = intersect_analytic_curve_candidates(curves, {{1, 2}});
    require(result.error == AnalyticNarrowPhaseError::none && result.intersections.size() == 1 &&
                result.intersections[0].relation == AnalyticPairRelation::point &&
                result.intersections[0].point_count == 1 &&
                result.intersections[0].points[0].x.lower == 0.0 &&
                result.intersections[0].points[0].y.lower == 0.0 &&
                !result.intersections[0].resolution_collapsed,
            "known-root factoring must retain the exact shared normalized endpoint: " +
                std::to_string(static_cast<int>(result.error)) + "/" +
                std::to_string(result.intersections.size()) + "/" +
                (result.intersections.empty()
                     ? std::string("empty")
                     : std::to_string(static_cast<int>(result.intersections[0].relation)) + "/" +
                           std::to_string(result.intersections[0].point_count)));

    std::vector<AnalyticAtomicCurveNm> near_tangent = {
        endpoint_authoritative_arc(1, 0, 0, 1000, 0, 600, true, false),
        line(2, 0, 0, 3330, -5000),
    };
    const auto uncertified_near_tangent =
        intersect_analytic_curve_candidates(near_tangent, {{1, 2}});
    require(uncertified_near_tangent.error == AnalyticNarrowPhaseError::none &&
                uncertified_near_tangent.intersections.size() == 1 &&
                uncertified_near_tangent.intersections[0].point_count == 2,
            "a distinct on-domain near-tangent root was suppressed without construction identity");
    near_tangent[0].construction_carrier_id = 1;
    near_tangent[1].construction_carrier_id = 2;
    const std::uint64_t tangent = analytic_endpoint_tangent_token(2, true, 1, true);
    near_tangent[0].construction_start_tangent_id = tangent;
    near_tangent[1].construction_start_tangent_id = tangent;
    const auto certified_near_tangent = intersect_analytic_curve_candidates(near_tangent, {{1, 2}});
    require(certified_near_tangent.error == AnalyticNarrowPhaseError::none &&
                certified_near_tangent.intersections.size() == 1 &&
                certified_near_tangent.intersections[0].point_count == 1,
            "matching endpoint tangent construction identity did not factor the phantom root");
    near_tangent[1].construction_start_tangent_id =
        analytic_endpoint_tangent_token(3, true, 1, true);
    require(intersect_analytic_curve_candidates(near_tangent, {{1, 2}}).error ==
                AnalyticNarrowPhaseError::invalid_argument,
            "a tangent token naming a different line carrier was accepted");
    near_tangent[1].construction_start_tangent_id = 0;
    near_tangent[1].construction_end_tangent_id = tangent;
    require(intersect_analytic_curve_candidates(near_tangent, {{1, 2}}).error ==
                AnalyticNarrowPhaseError::invalid_argument,
            "a start tangent token migrated to the line end was accepted");
    near_tangent[1].construction_end_tangent_id = 0;
    near_tangent[1].construction_start_tangent_id = tangent;
    near_tangent[1].construction_carrier_id = 3;
    require(intersect_analytic_curve_candidates(near_tangent, {{1, 2}}).error ==
                AnalyticNarrowPhaseError::invalid_argument,
            "a matching tangent token was trusted after carrier migration");

    std::vector<AnalyticAtomicCurveNm> near_circle_tangent = {
        endpoint_authoritative_arc(1, 0, 0, 50, 0, 25, false, false),
        endpoint_authoritative_arc(2, 0, 0, 48, 14, 25, true, false),
    };
    near_circle_tangent[0].has_endpoint_authoritative_x_monotone_certificate = false;
    near_circle_tangent[1].has_endpoint_authoritative_x_monotone_certificate = false;
    const auto two_circle_roots =
        intersect_analytic_curve_candidates(near_circle_tangent, {{1, 2}});
    require(two_circle_roots.error == AnalyticNarrowPhaseError::none &&
                two_circle_roots.intersections.size() == 1 &&
                two_circle_roots.intersections[0].point_count == 2,
            "circle endpoint fixture did not expose its strict second root error=" +
                std::to_string(static_cast<int>(two_circle_roots.error)) + " count=" +
                std::to_string(two_circle_roots.intersections.empty()
                                   ? 0
                                   : two_circle_roots.intersections[0].point_count));
    near_circle_tangent[0].construction_carrier_id = 1;
    near_circle_tangent[1].construction_carrier_id = 2;
    const std::uint64_t circle_tangent =
        analytic_circle_endpoint_tangent_token(1, true, 2, true, 17);
    near_circle_tangent[0].construction_start_tangent_id = circle_tangent;
    near_circle_tangent[1].construction_start_tangent_id = circle_tangent;
    const auto certified_circle_tangent =
        intersect_analytic_curve_candidates(near_circle_tangent, {{1, 2}});
    require(certified_circle_tangent.error == AnalyticNarrowPhaseError::none &&
                certified_circle_tangent.intersections.size() == 1 &&
                certified_circle_tangent.intersections[0].point_count == 1 &&
                analytic_circle_endpoint_tangent_identity(circle_tangent) == 17,
            "matching circle endpoint construction identity did not suppress the phantom root");
    near_circle_tangent[1].construction_start_tangent_id =
        analytic_circle_endpoint_tangent_token(1, true, 2, true, 18);
    const auto mismatched_circle_identity =
        intersect_analytic_curve_candidates(near_circle_tangent, {{1, 2}});
    require(mismatched_circle_identity.error == AnalyticNarrowPhaseError::none &&
                mismatched_circle_identity.intersections[0].point_count == 2,
            "different circle construction identities suppressed a strict second root");
    near_circle_tangent[1].construction_start_tangent_id = 0;
    near_circle_tangent[1].construction_end_tangent_id = circle_tangent;
    const auto migrated_circle_endpoint =
        intersect_analytic_curve_candidates(near_circle_tangent, {{1, 2}});
    require(migrated_circle_endpoint.error == AnalyticNarrowPhaseError::invalid_argument ||
                (migrated_circle_endpoint.error == AnalyticNarrowPhaseError::none &&
                 migrated_circle_endpoint.intersections[0].point_count == 2),
            "a circle tangent token migrated to the wrong endpoint suppressed a root");
    near_circle_tangent[1].construction_end_tangent_id = 0;
    near_circle_tangent[1].construction_start_tangent_id = circle_tangent;
    near_circle_tangent[1].construction_carrier_id = 3;
    require(intersect_analytic_curve_candidates(near_circle_tangent, {{1, 2}}).error ==
                AnalyticNarrowPhaseError::invalid_argument,
            "a circle tangent token was trusted after carrier migration");

    curves[0].circle.center.x.lower += 1.0;
    curves[0].circle.center.x.upper += 1.0;
    require(intersect_analytic_curve_candidates(curves, {{1, 2}}).error ==
                AnalyticNarrowPhaseError::invalid_argument,
            "a forged endpoint-authoritative center enclosure must be rejected");

    const auto forged_half = endpoint_authoritative_arc(1, 3, -4, 4, 3, 5, true, true);
    require(intersect_analytic_curve_candidates({forged_half}, {}).error ==
                AnalyticNarrowPhaseError::invalid_argument,
            "an endpoint-authoritative arc crossing a cardinal seam must be rejected");

    auto forged_major_semicircle = endpoint_authoritative_arc(1, -5, 0, 5, 0, 5, true, false);
    forged_major_semicircle.major_arc = true;
    require(intersect_analytic_curve_candidates({forged_major_semicircle}, {}).error ==
                AnalyticNarrowPhaseError::invalid_argument,
            "an ambiguous endpoint-authoritative major semicircle certificate was accepted");

    curves = {
        endpoint_authoritative_arc(1, 0, 0, -34, 68, 85, false, false),
        endpoint_authoritative_arc(2, 0, 0, -77, 49, 85, true, true),
    };
    const auto nearby_crossing = intersect_analytic_curve_candidates(curves, {{1, 2}});
    require(nearby_crossing.error == AnalyticNarrowPhaseError::none &&
                nearby_crossing.intersections.size() == 1 &&
                nearby_crossing.intersections[0].point_count == 2 &&
                !nearby_crossing.intersections[0].resolution_collapsed,
            "strict endpoint-authoritative roots within 50 nm must remain distinct");
}

void test_limits()
{
    require(kAnalyticCoordinateGridNm == 1 && kAnalyticTopologyResolutionNm == 50,
            "analytic coordinate and topology constants drifted");
    require(analytic_solver_limits_within_hard_ceilings(kAnalyticSolverHardLimits),
            "hard limits must accept themselves");
    AnalyticSolverLimits lower = kAnalyticSolverHardLimits;
    lower.examined_curve_pairs = 1;
    lower.working_memory_bytes = sizeof(AnalyticCurvePair);
    require(analytic_solver_limits_within_hard_ceilings(lower),
            "lower effective limits must be accepted");
    AnalyticSolverLimits excessive = kAnalyticSolverHardLimits;
    ++excessive.examined_curve_pairs;
    require(!analytic_solver_limits_within_hard_ceilings(excessive),
            "a limit above the governed ceiling must be rejected");
    require(kAnalyticSolverHardLimits.algebraic_fallback_calls == 0,
            "the filtered production core must default to no algebraic fallback");
}

void test_resolution_filter()
{
    require(classify_analytic_resolution({49.0, 0.0}) ==
                AnalyticResolutionClass::at_or_below_resolution,
            "49 nm must be below the resolution boundary");
    require(classify_analytic_resolution({50.0, 0.0}) ==
                AnalyticResolutionClass::at_or_below_resolution,
            "50 nm must be inside the allowed collapse boundary");
    require(classify_analytic_resolution({51.0, 0.0}) == AnalyticResolutionClass::above_resolution,
            "51 nm must preserve topology");
    require(classify_analytic_resolution({50.0, 0.25}) == AnalyticResolutionClass::uncertain,
            "an error interval straddling 50 nm must not guess");
    require(classify_analytic_resolution({50.5, 0.25}) == AnalyticResolutionClass::above_resolution,
            "a certified interval above 50 nm must preserve topology");
    const double below_50 = std::nextafter(50.0, 0.0);
    const double above_50 = std::nextafter(50.0, std::numeric_limits<double>::infinity());
    require(classify_analytic_resolution({below_50, 8e-15}) == AnalyticResolutionClass::uncertain,
            "an outward-rounded upper endpoint above 50 nm must not collapse");
    require(classify_analytic_resolution({above_50, 8e-15}) == AnalyticResolutionClass::uncertain,
            "an outward-rounded lower endpoint below 50 nm must not preserve topology");
    require(classify_analytic_resolution({above_50, 0.0}) ==
                AnalyticResolutionClass::above_resolution,
            "an exact value one ULP above 50 nm must preserve topology");
    require(classify_analytic_resolution({-1.0, 0.0}) == AnalyticResolutionClass::invalid,
            "negative distances must be invalid");
    require(classify_analytic_resolution({std::numeric_limits<double>::quiet_NaN(), 0.0}) ==
                AnalyticResolutionClass::invalid,
            "non-finite distances must be invalid");
}

void test_broad_phase_threshold_and_order()
{
    const std::vector<AnalyticCurveBoundsNm> ordered = {
        {30, 0.0, 0.0, 10.0, 10.0},  {10, 60.0, 0.0, 70.0, 10.0}, {40, 121.0, 0.0, 131.0, 10.0},
        {20, 0.0, 61.0, 10.0, 71.0}, {50, 0.0, 60.0, 10.0, 70.0},
    };
    const std::vector<AnalyticCurvePair> expected = {{10, 30}, {10, 50}, {20, 50}, {30, 50}};
    AnalyticBroadPhaseResult first = build_analytic_curve_candidates(ordered);
    require(first.error == AnalyticBroadPhaseError::none,
            "threshold broad phase unexpectedly failed");
    require(same_pairs(first.pairs, expected), "49/50/51 nm broad-phase boundary drifted");
    require(first.telemetry.algebraic_fallback_calls == 0,
            "broad phase must not invoke the algebraic backend");
    require(first.telemetry.peak_working_memory_bytes <=
                kAnalyticSolverHardLimits.working_memory_bytes,
            "broad-phase working storage must remain inside the governed budget");
    require(first.telemetry.work_units != 0 && first.telemetry.retained_pair_bytes == 64 * 8,
            "broad phase did not expose its governed work and retained pair capacity");
    AnalyticSolverLimits exact_work = kAnalyticSolverHardLimits;
    exact_work.predicate_calls = first.telemetry.work_units;
    require(build_analytic_curve_candidates(ordered, exact_work).error ==
                AnalyticBroadPhaseError::none,
            "exact broad-phase work boundary failed");
    --exact_work.predicate_calls;
    require(build_analytic_curve_candidates(ordered, exact_work).error ==
                AnalyticBroadPhaseError::resource_limit_exceeded,
            "one-short broad-phase work boundary did not fail closed");

    std::vector<AnalyticCurveBoundsNm> reversed(ordered.rbegin(), ordered.rend());
    AnalyticBroadPhaseResult second = build_analytic_curve_candidates(reversed);
    require(second.error == AnalyticBroadPhaseError::none && same_pairs(second.pairs, expected),
            "candidate ordering must be independent of input traversal");
}

void test_broad_phase_limits_and_validation()
{
    const std::vector<AnalyticCurveBoundsNm> dense = {
        {1, 0.0, 0.0, 10.0, 10.0},
        {2, 0.0, 0.0, 10.0, 10.0},
        {3, 0.0, 0.0, 10.0, 10.0},
    };
    AnalyticSolverLimits one_pair = kAnalyticSolverHardLimits;
    one_pair.examined_curve_pairs = 1;
    require(build_analytic_curve_candidates(dense, one_pair).error ==
                AnalyticBroadPhaseError::resource_limit_exceeded,
            "examined-pair limit must fail before accepting extra work");

    AnalyticSolverLimits one_predicate = kAnalyticSolverHardLimits;
    one_predicate.predicate_calls = 1;
    require(build_analytic_curve_candidates(dense, one_predicate).error ==
                AnalyticBroadPhaseError::resource_limit_exceeded,
            "predicate limit must fail before an extra test");

    AnalyticSolverLimits short_memory = kAnalyticSolverHardLimits;
    short_memory.working_memory_bytes = sizeof(AnalyticCurvePair) - 1;
    require(build_analytic_curve_candidates(dense, short_memory).error ==
                AnalyticBroadPhaseError::resource_limit_exceeded,
            "pair storage must respect the effective memory budget");

    AnalyticSolverLimits base_memory_only = kAnalyticSolverHardLimits;
    base_memory_only.working_memory_bytes = 1;
    require(build_analytic_curve_candidates(dense, base_memory_only).error ==
                AnalyticBroadPhaseError::resource_limit_exceeded,
            "candidate storage must account for broad-phase workspace first");

    require(build_analytic_curve_candidates({{1, 1.0, 0.0, 0.0, 1.0}}).error ==
                AnalyticBroadPhaseError::invalid_argument,
            "inverted bounds must be rejected");
    require(
        build_analytic_curve_candidates({{1, 0.0, 0.0, 1.0, 1.0}, {1, 2.0, 2.0, 3.0, 3.0}}).error ==
            AnalyticBroadPhaseError::invalid_argument,
        "duplicate curve indices must be rejected");
}

void test_broad_phase_chooses_sparse_axis()
{
    auto horizontal_curves = [](std::uint32_t count)
    {
        std::vector<AnalyticCurveBoundsNm> curves;
        curves.reserve(count);
        for (std::uint32_t index = 0; index < count; ++index)
        {
            const double y = static_cast<double>(index) * 52.0;
            curves.push_back({index + 1, 0.0, y, 1'000'000.0, y + 1.0});
        }
        return curves;
    };

    AnalyticBroadPhaseResult small = build_analytic_curve_candidates(horizontal_curves(512));
    AnalyticBroadPhaseResult large = build_analytic_curve_candidates(horizontal_curves(1024));
    require(small.error == AnalyticBroadPhaseError::none && small.pairs.empty() &&
                large.error == AnalyticBroadPhaseError::none && large.pairs.empty(),
            "separated horizontal curves must have no candidates");
    require(small.telemetry.primary_axis == 1 && small.telemetry.primary_axis_pairs == 0 &&
                small.telemetry.examined_curve_pairs == 0 && large.telemetry.primary_axis == 1 &&
                large.telemetry.primary_axis_pairs == 0 &&
                large.telemetry.examined_curve_pairs == 0,
            "broad phase must choose the sparse y axis instead of quadratic x overlap");
    require(small.telemetry.sort_comparisons < 512 * 64 &&
                large.telemetry.sort_comparisons < 1024 * 64 &&
                large.telemetry.sort_comparisons < small.telemetry.sort_comparisons * 3,
            "doubling sparse input must retain n-log-n sorting work, not all-pairs work");
}

void test_broad_phase_avoids_crossed_projection_quadratic_work()
{
    auto crossed_curves = [](std::uint32_t count)
    {
        std::vector<AnalyticCurveBoundsNm> curves;
        curves.reserve(2 * count);
        for (std::uint32_t index = 0; index < count; ++index)
        {
            const double y = static_cast<double>(index) * 101.0;
            curves.push_back({index + 1, 0.0, y, 1'000'000.0, y + 1.0});
        }
        for (std::uint32_t index = 0; index < count; ++index)
        {
            const double x = 2'000'000.0 + static_cast<double>(index) * 101.0;
            curves.push_back({count + index + 1, x, 0.0, x + 1.0, 1'000'000.0});
        }
        return curves;
    };

    const AnalyticBroadPhaseResult small = build_analytic_curve_candidates(crossed_curves(512));
    const AnalyticBroadPhaseResult large = build_analytic_curve_candidates(crossed_curves(1024));
    require(small.error == AnalyticBroadPhaseError::none && small.pairs.empty() &&
                large.error == AnalyticBroadPhaseError::none && large.pairs.empty(),
            "crossed dense projections must not produce 2D candidates");
    require(small.telemetry.primary_axis_pairs > 100'000 &&
                large.telemetry.primary_axis_pairs > 400'000 &&
                small.telemetry.examined_curve_pairs == 0 &&
                large.telemetry.examined_curve_pairs == 0,
            "projection density must not be charged as examined 2D pairs");
    require(large.telemetry.spatial_index_node_visits <
                small.telemetry.spatial_index_node_visits * 3,
            "doubling crossed projections must retain near n-log-n index work");
}

void test_broad_phase_memory_charge_is_cross_runtime_canonical()
{
    std::vector<AnalyticCurveBoundsNm> curves;
    curves.reserve(257);
    for (std::uint32_t index = 0; index < 257; ++index)
    {
        const double x = static_cast<double>(index) * 101.0;
        curves.push_back({index + 1, x, 0.0, x + 1.0, 1.0});
    }

    constexpr std::uint64_t canonical_sweep_bytes = 257 * 96;
    AnalyticSolverLimits exact_memory = kAnalyticSolverHardLimits;
    exact_memory.working_memory_bytes = canonical_sweep_bytes;
    const AnalyticBroadPhaseResult accepted = build_analytic_curve_candidates(curves, exact_memory);
    require(accepted.error == AnalyticBroadPhaseError::none && accepted.pairs.empty() &&
                accepted.telemetry.peak_working_memory_bytes == canonical_sweep_bytes,
            "a non-power-of-two sweep must use the canonical cross-runtime memory charge");

    AnalyticSolverLimits one_byte_short = exact_memory;
    --one_byte_short.working_memory_bytes;
    require(build_analytic_curve_candidates(curves, one_byte_short).error ==
                AnalyticBroadPhaseError::resource_limit_exceeded,
            "native and WASM must reject one byte below the canonical sweep charge");
}

void test_broad_phase_pair_capacity_growth_is_canonical()
{
    auto dense = [](std::uint32_t count)
    {
        std::vector<AnalyticCurveBoundsNm> curves;
        curves.reserve(count);
        for (std::uint32_t index = 0; index < count; ++index)
            curves.push_back({index + 1, 0.0, 0.0, 1.0, 1.0});
        return curves;
    };
    const auto below = build_analytic_curve_candidates(dense(11));
    const auto above = build_analytic_curve_candidates(dense(12));
    require(below.error == AnalyticBroadPhaseError::none && below.pairs.size() == 55 &&
                below.telemetry.retained_pair_bytes == 64 * 8,
            "below-growth broad pair capacity changed");
    require(above.error == AnalyticBroadPhaseError::none && above.pairs.size() == 66 &&
                above.telemetry.retained_pair_bytes == 128 * 8,
            "above-growth broad pair capacity changed");
}

void test_narrow_phase_line_line()
{
    const std::vector<AnalyticAtomicCurveNm> curves = {
        line(1, -10, 0, 10, 0), line(2, 0, -10, 0, 10), line(3, -10, 5, 10, 5),
        line(4, -5, 0, 5, 0),   line(5, 10, 0, 20, 0),
    };
    const std::vector<AnalyticCurvePair> pairs = {{1, 2}, {1, 3}, {1, 4}, {1, 5}};
    const AnalyticNarrowPhaseResult result = intersect_analytic_curve_candidates(curves, pairs);
    require(result.error == AnalyticNarrowPhaseError::none && result.intersections.size() == 4,
            "line/line narrow phase failed");
    require(result.intersections[0].relation == AnalyticPairRelation::point &&
                result.intersections[0].point_count == 1 &&
                contains(result.intersections[0].points[0].x, 0.0) &&
                contains(result.intersections[0].points[0].y, 0.0),
            "line crossing changed");
    require(result.intersections[1].relation == AnalyticPairRelation::disjoint,
            "parallel lines must remain disjoint");
    require(result.intersections[2].relation == AnalyticPairRelation::coincident,
            "collinear overlap must reach the overlay stage");
    require(result.intersections[3].relation == AnalyticPairRelation::point &&
                contains(result.intersections[3].points[0].x, 10.0),
            "collinear endpoint touch changed");
    require(result.telemetry.line_line_pairs == 4 &&
                result.telemetry.candidate_pairs_consumed == 4 &&
                result.telemetry.algebraic_fallback_calls == 0,
            "line/line telemetry changed");
}

void test_exact_construction_line_domain_rejection()
{
    auto certified =
        [](AnalyticAtomicCurveNm value, std::uint64_t carrier, std::int64_t dx, std::int64_t dy)
    {
        value.construction_carrier_id = carrier;
        value.has_construction_line_direction = true;
        value.construction_line_dx = dx;
        value.construction_line_dy = dy;
        return value;
    };
    auto reverse = [](AnalyticAtomicCurveNm value)
    {
        std::swap(value.start, value.end);
        std::swap(value.integer_start, value.integer_end);
        return value;
    };
    const auto ordered_pair =
        [](AnalyticAtomicCurveNm left, AnalyticAtomicCurveNm right, bool swap_order)
    {
        std::array<AnalyticAtomicCurveNm, 2> curves = {left, right};
        if (swap_order)
            std::swap(curves[0], curves[1]);
        curves[0].curve_index = 1;
        curves[1].curve_index = 2;
        return std::vector<AnalyticAtomicCurveNm>{curves.begin(), curves.end()};
    };

    const AnalyticAtomicCurveNm far_left =
        certified(line(1, 12'259'406, 15'717'310, 12'604'719, 16'062'623), 101, 1, 1);
    const AnalyticAtomicCurveNm far_right =
        certified(line(2, 12'061'472, 15'224'577, 12'294'263, 15'457'371), 102, 77'597, 77'598);
    for (std::uint8_t permutation = 0; permutation != 8; ++permutation)
    {
        const AnalyticAtomicCurveNm left = (permutation & 1U) != 0 ? reverse(far_left) : far_left;
        const AnalyticAtomicCurveNm right =
            (permutation & 2U) != 0 ? reverse(far_right) : far_right;
        const AnalyticNarrowPhaseResult result = intersect_analytic_curve_candidates(
            ordered_pair(left, right, (permutation & 4U) != 0), {{1, 2}});
        require(result.error == AnalyticNarrowPhaseError::none &&
                    result.intersections.size() == 1 &&
                    result.intersections[0].relation == AnalyticPairRelation::disjoint &&
                    result.telemetry.domain_predicates == 1,
                "an exact far-off line intersection was not rejected independently of direction");
    }

    const AnalyticAtomicCurveNm near_left = certified(line(1, -1000, 0, 0, 0), 111, 1, 0);
    const AnalyticAtomicCurveNm near_right = certified(line(2, 35, 35, 35, 1000), 112, 0, 1);
    for (std::uint8_t permutation = 0; permutation != 8; ++permutation)
    {
        const AnalyticAtomicCurveNm left = (permutation & 1U) != 0 ? reverse(near_left) : near_left;
        const AnalyticAtomicCurveNm right =
            (permutation & 2U) != 0 ? reverse(near_right) : near_right;
        const std::vector<AnalyticAtomicCurveNm> curves =
            ordered_pair(left, right, (permutation & 4U) != 0);
        const AnalyticNarrowPhaseResult normal =
            intersect_analytic_curve_candidates(curves, {{1, 2}});
        const AnalyticNarrowPhaseResult strict =
            analytic_execution_detail::intersect_curve_candidates(
                curves, {{1, 2}}, kAnalyticSolverHardLimits,
                analytic_execution_detail::kStrictPublishedGeometry);
        require(normal.error == AnalyticNarrowPhaseError::none &&
                    normal.intersections[0].relation == AnalyticPairRelation::point &&
                    normal.intersections[0].resolution_collapsed &&
                    strict.error == AnalyticNarrowPhaseError::none &&
                    strict.intersections[0].relation == AnalyticPairRelation::disjoint,
                "strict finite domains must discard any outside root while normal policy may "
                "bridge 35/35 nm");
    }

    const AnalyticAtomicCurveNm crossing_left = certified(line(1, 0, 0, 10, 10), 201, 1, 1);
    const AnalyticAtomicCurveNm crossing_right = certified(line(2, 0, 10, 10, 0), 202, 1, -1);
    const AnalyticNarrowPhaseResult crossing =
        intersect_analytic_curve_candidates({crossing_left, crossing_right}, {{1, 2}});
    require(crossing.error == AnalyticNarrowPhaseError::none &&
                crossing.intersections[0].relation == AnalyticPairRelation::point &&
                contains(crossing.intersections[0].points[0].x, 5.0) &&
                contains(crossing.intersections[0].points[0].y, 5.0),
            "an in-domain exact line intersection was discarded");

    const AnalyticAtomicCurveNm endpoint_left = certified(line(1, 0, 0, 10, 10), 301, 1, 1);
    const AnalyticAtomicCurveNm endpoint_right = certified(line(2, 5, 15, 15, 5), 302, 1, -1);
    const AnalyticNarrowPhaseResult endpoint =
        intersect_analytic_curve_candidates({endpoint_left, endpoint_right}, {{1, 2}});
    require(endpoint.error == AnalyticNarrowPhaseError::none &&
                endpoint.intersections[0].relation == AnalyticPairRelation::point &&
                contains(endpoint.intersections[0].points[0].x, 10.0) &&
                contains(endpoint.intersections[0].points[0].y, 10.0),
            "an exact endpoint line intersection was discarded");

    AnalyticAtomicCurveNm untrusted_integer_left = far_left;
    untrusted_integer_left.has_construction_line_direction = false;
    untrusted_integer_left.construction_line_dx = 0;
    untrusted_integer_left.construction_line_dy = 0;
    const AnalyticNarrowPhaseResult untrusted_integer =
        intersect_analytic_curve_candidates({untrusted_integer_left, far_right}, {{1, 2}});
    require(untrusted_integer.error == AnalyticNarrowPhaseError::none &&
                untrusted_integer.intersections[0].relation == AnalyticPairRelation::disjoint &&
                untrusted_integer.telemetry.domain_predicates == 1,
            "an integer line without construction authority missed interval-domain rejection");

    AnalyticAtomicCurveNm noninteger_left = far_left;
    noninteger_left.has_integer_certificate = false;
    for (std::uint8_t permutation = 0; permutation != 8; ++permutation)
    {
        const AnalyticAtomicCurveNm left =
            (permutation & 1U) != 0 ? reverse(noninteger_left) : noninteger_left;
        const AnalyticAtomicCurveNm right =
            (permutation & 2U) != 0 ? reverse(far_right) : far_right;
        const AnalyticNarrowPhaseResult noninteger = intersect_analytic_curve_candidates(
            ordered_pair(left, right, (permutation & 4U) != 0), {{1, 2}});
        require(noninteger.error == AnalyticNarrowPhaseError::none &&
                    noninteger.intersections[0].relation == AnalyticPairRelation::disjoint &&
                    noninteger.telemetry.domain_predicates == 1,
                "a noninteger offset line missed interval-domain rejection under reversal/order");
    }

    AnalyticAtomicCurveNm boundary_left = filtered_line(1, 0.0, 0.0, 100.0, 0.0);
    AnalyticAtomicCurveNm boundary_right = filtered_line(2, 100.0, -100.0, 100.0, 100.0);
    boundary_right.start.x = {99.0, 101.0};
    boundary_right.end.x = {99.0, 101.0};
    const AnalyticNarrowPhaseResult uncertain_boundary =
        intersect_analytic_curve_candidates({boundary_left, boundary_right}, {{1, 2}});
    require(uncertain_boundary.error == AnalyticNarrowPhaseError::none &&
                uncertain_boundary.intersections[0].relation == AnalyticPairRelation::point &&
                uncertain_boundary.intersections[0].resolution_collapsed,
            "an interval parameter straddling the finite-domain boundary was discarded");
}

void test_vertical_construction_column_tokens()
{
    std::vector<AnalyticAtomicCurveNm> curves = {line(1, 100, -1000, 100, 1000),
                                                 line(2, -1000, -200, 1000, -200),
                                                 line(3, -1000, 300, 1000, 300)};
    curves[0].construction_carrier_id = 7;
    curves[0].construction_family_id = 11;
    curves[0].has_construction_line_direction = true;
    curves[0].construction_line_dy = 1;
    const std::uint64_t column = analytic_vertical_x_column_token(7);
    curves[0].start.construction_x_column_id = column;
    curves[0].end.construction_x_column_id = column;
    curves[1].construction_carrier_id = 8;
    curves[2].construction_carrier_id = 9;

    const AnalyticNarrowPhaseResult result =
        intersect_analytic_curve_candidates(curves, {{1, 2}, {1, 3}});
    require(result.error == AnalyticNarrowPhaseError::none && result.intersections.size() == 2 &&
                result.intersections[0].point_count == 1 &&
                result.intersections[1].point_count == 1 &&
                result.intersections[0].points[0].construction_x_column_id == column &&
                result.intersections[1].points[0].construction_x_column_id == column,
            "vertical carrier intersections did not preserve one construction column");

    curves[0].end.construction_x_column_id ^= 1U;
    require(intersect_analytic_curve_candidates(curves, {}).error ==
                AnalyticNarrowPhaseError::invalid_argument,
            "an inconsistent vertical construction column token was accepted");
}

void test_narrow_phase_line_circle()
{
    const std::vector<AnalyticAtomicCurveNm> curves = {
        line(1, -200, 0, 200, 0),     arc(2, 100, 0, -100, 0, 0, 0, 100, true),
        line(3, -200, 100, 200, 100), line(4, -200, 101, 200, 101),
        line(5, 0, -200, 0, 200),
    };
    const std::vector<AnalyticCurvePair> pairs = {{1, 2}, {2, 3}, {2, 4}, {2, 5}};
    const AnalyticNarrowPhaseResult result = intersect_analytic_curve_candidates(curves, pairs);
    require(result.error == AnalyticNarrowPhaseError::none && result.intersections.size() == 4,
            "line/circle narrow phase failed");
    require(result.intersections[0].relation == AnalyticPairRelation::two_points &&
                result.intersections[0].point_count == 2 &&
                contains(result.intersections[0].points[0].x, -100.0) &&
                contains(result.intersections[0].points[1].x, 100.0),
            "line/circle secant changed");
    require(result.intersections[1].relation == AnalyticPairRelation::point &&
                contains(result.intersections[1].points[0].x, 0.0) &&
                contains(result.intersections[1].points[0].y, 100.0),
            "line/circle tangent changed");
    require(result.intersections[2].relation == AnalyticPairRelation::disjoint,
            "line outside circle must remain disjoint");
    require(result.intersections[3].relation == AnalyticPairRelation::point &&
                contains(result.intersections[3].points[0].x, 0.0) &&
                contains(result.intersections[3].points[0].y, 100.0),
            "finite upper arc must reject the lower carrier intersection");
    require(result.telemetry.line_circle_pairs == 4 && result.telemetry.square_root_calls == 7 &&
                result.telemetry.tangent_contacts == 1 &&
                !result.intersections[1].resolution_collapsed,
            "line/circle work telemetry changed");
}

void test_horizontal_mirror_line_circle_root_tokens()
{
    const std::uint64_t mirror = analytic_horizontal_mirror_construction_id(10, 11);
    require(mirror != 0 && analytic_horizontal_mirror_contains_carrier(mirror, 10) &&
                analytic_horizontal_mirror_contains_carrier(mirror, 11) &&
                !analytic_horizontal_mirror_contains_carrier(mirror, 12),
            "horizontal mirror construction identity packing drifted");

    auto certified_line = [&](std::uint32_t index, std::int64_t y, std::uint64_t carrier)
    {
        AnalyticAtomicCurveNm value = line(index, -200, y, 200, y);
        value.construction_carrier_id = carrier;
        value.construction_family_id = 20;
        value.has_construction_line_direction = true;
        value.construction_line_dx = 1;
        value.construction_horizontal_mirror_id = mirror;
        return value;
    };
    auto certified_arc =
        [&](std::uint32_t index, std::int64_t center_y, bool upper, std::uint64_t carrier)
    {
        AnalyticAtomicCurveNm value =
            upper ? arc(index, 100, center_y, -100, center_y, 0, center_y, 100, true)
                  : arc(index, -100, center_y, 100, center_y, 0, center_y, 100, true);
        value.construction_carrier_id = carrier;
        value.construction_family_id = 30;
        return value;
    };

    AnalyticAtomicCurveNm upper_line = certified_line(1, 50, 10);
    AnalyticAtomicCurveNm upper_arc = certified_arc(2, 0, true, 30);
    AnalyticAtomicCurveNm lower_line = certified_line(1, -50, 11);
    AnalyticAtomicCurveNm lower_arc = certified_arc(2, 0, false, 30);
    const AnalyticNarrowPhaseResult upper =
        intersect_analytic_curve_candidates({upper_line, upper_arc}, {{1, 2}});
    const AnalyticNarrowPhaseResult lower =
        intersect_analytic_curve_candidates({lower_line, lower_arc}, {{1, 2}});
    require(upper.error == AnalyticNarrowPhaseError::none &&
                lower.error == AnalyticNarrowPhaseError::none &&
                upper.intersections[0].point_count == 2 && lower.intersections[0].point_count == 2,
            "certified horizontal mirror intersections failed");
    const std::uint64_t left_token = upper.intersections[0].points[0].construction_x_column_id;
    const std::uint64_t right_token = upper.intersections[0].points[1].construction_x_column_id;
    require(left_token != 0 && right_token != 0 && left_token != right_token &&
                analytic_is_symmetric_line_circle_root_x_column_token(left_token) &&
                analytic_is_symmetric_line_circle_root_x_column_token(right_token) &&
                lower.intersections[0].points[0].construction_x_column_id == left_token &&
                lower.intersections[0].points[1].construction_x_column_id == right_token,
            "corresponding roots from one mirror/circle construction were not correlated");

    AnalyticAtomicCurveNm unrelated_line = upper_line;
    unrelated_line.construction_horizontal_mirror_id = 0;
    const auto unrelated =
        intersect_analytic_curve_candidates({unrelated_line, upper_arc}, {{1, 2}});
    require(unrelated.error == AnalyticNarrowPhaseError::none &&
                unrelated.intersections[0].points[0].construction_x_column_id == 0 &&
                unrelated.intersections[0].points[1].construction_x_column_id == 0,
            "an unrelated horizontal line acquired a mirror-root token");

    AnalyticAtomicCurveNm off_axis_arc = certified_arc(2, 1, true, 30);
    const auto off_axis = intersect_analytic_curve_candidates({upper_line, off_axis_arc}, {{1, 2}});
    require(off_axis.error == AnalyticNarrowPhaseError::none &&
                off_axis.intersections[0].points[0].construction_x_column_id == 0 &&
                off_axis.intersections[0].points[1].construction_x_column_id == 0,
            "an off-axis circle acquired a mirror-root token");

    const std::uint64_t other_mirror = analytic_horizontal_mirror_construction_id(10, 12);
    AnalyticAtomicCurveNm other_line = upper_line;
    other_line.construction_horizontal_mirror_id = other_mirror;
    const auto other_construction =
        intersect_analytic_curve_candidates({other_line, upper_arc}, {{1, 2}});
    require(
        other_construction.error == AnalyticNarrowPhaseError::none &&
            other_construction.intersections[0].points[0].construction_x_column_id != left_token &&
            other_construction.intersections[0].points[1].construction_x_column_id != right_token,
        "different mirror constructions shared root tokens");

    AnalyticAtomicCurveNm other_circle = upper_arc;
    other_circle.construction_carrier_id = 31;
    const auto other_circle_result =
        intersect_analytic_curve_candidates({upper_line, other_circle}, {{1, 2}});
    require(
        other_circle_result.error == AnalyticNarrowPhaseError::none &&
            other_circle_result.intersections[0].points[0].construction_x_column_id != left_token &&
            other_circle_result.intersections[0].points[1].construction_x_column_id != right_token,
        "different circle constructions shared mirror-root tokens");
}

void test_narrow_phase_circle_circle_and_irrational_output()
{
    const std::vector<AnalyticAtomicCurveNm> curves = {
        arc(1, 30, 40, 30, -40, 0, 0, 50, true, true), arc(2, 30, 40, 30, -40, 60, 0, 50, true),
        arc(3, 100, 0, -100, 0, 0, 0, 100, true),      arc(4, 190, 0, -10, 0, 90, 0, 100, true),
        arc(5, 300, 0, 100, 0, 200, 0, 100, true),
    };
    const std::vector<AnalyticCurvePair> pairs = {{1, 2}, {3, 4}, {3, 5}};
    const AnalyticNarrowPhaseResult result = intersect_analytic_curve_candidates(curves, pairs);
    require(result.error == AnalyticNarrowPhaseError::none && result.intersections.size() == 3,
            "circle/circle narrow phase failed");
    require(result.intersections[0].relation == AnalyticPairRelation::two_points &&
                contains(result.intersections[0].points[0].x, 30.0) &&
                contains(result.intersections[0].points[0].y, -40.0) &&
                contains(result.intersections[0].points[1].y, 40.0),
            "integer circle/circle secant changed");
    const double irrational_y = std::sqrt(7'975.0);
    require(
        result.intersections[1].relation == AnalyticPairRelation::point &&
            contains(result.intersections[1].points[0].x, 45.0) &&
            contains(result.intersections[1].points[0].y, irrational_y) &&
            result.intersections[1].points[0].x.upper - result.intersections[1].points[0].x.lower <
                1e-10 &&
            result.intersections[1].points[0].y.upper - result.intersections[1].points[0].y.lower <
                1e-10,
        "ordinary irrational circle crossing must remain a tiny filtered interval");
    require(result.intersections[2].relation == AnalyticPairRelation::point,
            "external circle tangency changed");
    require(result.telemetry.circle_circle_pairs == 3 && result.telemetry.tangent_contacts == 1 &&
                !result.intersections[2].resolution_collapsed &&
                result.telemetry.algebraic_fallback_calls == 0,
            "ordinary circle intersections must not enter algebraic fallback");
}

void test_narrow_phase_filtered_authored_arcs()
{
    const double root_two = std::sqrt(2.0);
    AnalyticAtomicCurveNm irrational_radius =
        filtered_arc(1, 1.0, 1.0, -1.0, 1.0, 0.0, 0.0,
                     {std::nextafter(root_two, 0.0),
                      std::nextafter(root_two, std::numeric_limits<double>::infinity())},
                     true);
    irrational_radius.has_integer_certificate = true;
    irrational_radius.integer_start = {1, 1};
    irrational_radius.integer_end = {-1, 1};
    irrational_radius.integer_center = {0, 0};

    const AnalyticAtomicCurveNm noninteger_offset =
        filtered_arc(2, 10.75, 0.0, -10.25, 0.0, 0.25, 0.0, {10.5, 10.5}, true);
    const AnalyticNarrowPhaseResult accepted =
        intersect_analytic_curve_candidates({irrational_radius, noninteger_offset}, {});
    require(accepted.error == AnalyticNarrowPhaseError::none,
            "filtered authored arcs must not require an integer radius or center");

    AnalyticAtomicCurveNm incomplete_radius_certificate = irrational_radius;
    incomplete_radius_certificate.circle.radius = {
        root_two, std::nextafter(root_two, std::numeric_limits<double>::infinity())};
    require(intersect_analytic_curve_candidates({incomplete_radius_certificate}, {}).error ==
                AnalyticNarrowPhaseError::invalid_argument,
            "a non-integral certified radius interval must enclose the exact outward root");

    AnalyticAtomicCurveNm too_wide = noninteger_offset;
    too_wide.circle.radius = {1.0, 102.0};
    require(intersect_analytic_curve_candidates({irrational_radius, too_wide}, {}).error ==
                AnalyticNarrowPhaseError::invalid_argument,
            "an input interval wider than the output displacement envelope must fail closed");

    const double offset = 5.0 / std::sqrt(2.0);
    AnalyticAtomicCurveNm irrational_cap =
        filtered_arc(1, 100.0 - offset, 100.0 + offset, 100.0 + offset, 100.0 - offset, 100.0,
                     100.0, {5.0, 5.0}, true);
    irrational_cap.start.x = {
        std::nextafter(irrational_cap.start.x.lower, 0.0),
        std::nextafter(irrational_cap.start.x.upper, std::numeric_limits<double>::infinity())};
    irrational_cap.start.y = {
        std::nextafter(irrational_cap.start.y.lower, 0.0),
        std::nextafter(irrational_cap.start.y.upper, std::numeric_limits<double>::infinity())};
    irrational_cap.end.x = {
        std::nextafter(irrational_cap.end.x.lower, 0.0),
        std::nextafter(irrational_cap.end.x.upper, std::numeric_limits<double>::infinity())};
    irrational_cap.end.y = {
        std::nextafter(irrational_cap.end.y.lower, 0.0),
        std::nextafter(irrational_cap.end.y.upper, std::numeric_limits<double>::infinity())};
    irrational_cap.has_arc_sweep_certificate = true;
    irrational_cap.construction_family_id = 91;
    irrational_cap.construction_carrier_id = 92;
    AnalyticAtomicCurveNm duplicate_cap = irrational_cap;
    duplicate_cap.curve_index = 2;
    const AnalyticNarrowPhaseResult cap_overlay =
        intersect_analytic_curve_candidates({irrational_cap, duplicate_cap}, {{1, 2}});
    require(cap_overlay.error == AnalyticNarrowPhaseError::none &&
                cap_overlay.intersections[0].relation == AnalyticPairRelation::coincident,
            "an arbitrary-angle irrational capsule cap must validate and reach overlay");

    AnalyticAtomicCurveNm first_offset = filtered_line(1, 0.25, 0.25, 100.25, 0.25);
    AnalyticAtomicCurveNm second_offset = filtered_line(2, 0.25, 10.75, 100.25, 10.75);
    first_offset.construction_family_id = second_offset.construction_family_id = 101;
    first_offset.construction_carrier_id = 102;
    second_offset.construction_carrier_id = 103;
    const AnalyticNarrowPhaseResult parallel_offsets =
        intersect_analytic_curve_candidates({first_offset, second_offset}, {{1, 2}});
    require(parallel_offsets.error == AnalyticNarrowPhaseError::none &&
                parallel_offsets.intersections[0].relation == AnalyticPairRelation::disjoint,
            "certified non-integral parallel offset carriers must not fail uncertain");

    const AnalyticNarrowPhaseResult singleton_parallel = intersect_analytic_curve_candidates(
        {filtered_line(1, 0.25, 0.25, 100.25, 0.25), filtered_line(2, 0.25, 10.75, 100.25, 10.75)},
        {{1, 2}});
    require(singleton_parallel.error == AnalyticNarrowPhaseError::none &&
                singleton_parallel.intersections[0].relation == AnalyticPairRelation::disjoint,
            "singleton non-integral parallel lines must be decided without a token");

    const AnalyticNarrowPhaseResult singleton_concentric = intersect_analytic_curve_candidates(
        {filtered_arc(1, 10.75, 0.25, -10.25, 0.25, 0.25, 0.25, {10.5, 10.5}, true),
         filtered_arc(2, 10.75, 0.25, -10.25, 0.25, 0.25, 0.25, {10.5, 10.5}, true)},
        {{1, 2}});
    require(singleton_concentric.error == AnalyticNarrowPhaseError::none &&
                singleton_concentric.intersections[0].relation == AnalyticPairRelation::coincident,
            "singleton non-integral concentric circles must reach overlay without a token");
}

void test_narrow_phase_near_tangent_displacement_guard()
{
    constexpr std::int64_t radius = 1'000'000'000'000;
    const std::vector<AnalyticAtomicCurveNm> line_circle = {
        line(1, -100'019'999, 999'999'994'998, -99'979'999, 999'999'995'002),
        arc(2, radius, 0, -radius, 0, 0, 0, radius, true),
    };
    const AnalyticNarrowPhaseResult guarded_line_circle =
        intersect_analytic_curve_candidates(line_circle, {{1, 2}});
    require(guarded_line_circle.error == AnalyticNarrowPhaseError::resource_limit_exceeded &&
                guarded_line_circle.telemetry.unresolved_predicate_failure &&
                guarded_line_circle.telemetry.resolution_collapses == 0,
            "a small radial sagitta must not collapse line/circle points separated by 26 um");

    constexpr double filtered_radius = 500'000'000'000.0;
    const double nearly_external = std::nextafter(1'000'000'000'000.0, 0.0);
    const std::vector<AnalyticAtomicCurveNm> circle_circle = {
        filtered_arc(1, 0.0, -filtered_radius, 0.0, filtered_radius, 0.0, 0.0,
                     {filtered_radius, filtered_radius}, true),
        filtered_arc(2, nearly_external, filtered_radius, nearly_external, -filtered_radius,
                     nearly_external, 0.0, {filtered_radius, filtered_radius}, true),
    };
    const AnalyticNarrowPhaseResult guarded_circle_circle =
        intersect_analytic_curve_candidates(circle_circle, {{1, 2}});
    require(guarded_circle_circle.error == AnalyticNarrowPhaseError::resource_limit_exceeded &&
                guarded_circle_circle.telemetry.unresolved_predicate_failure &&
                guarded_circle_circle.telemetry.resolution_collapses == 0,
            "a cancellation-heavy circle tangency must preserve intersections beyond 50 nm");

    AnalyticAtomicCurveNm uncertain_line = line(2, 0, 0, 1'000, 0);
    uncertain_line.has_integer_certificate = false;
    uncertain_line.start.y = {0.1, 1.0};
    uncertain_line.end.y = {-1.0, 0.1};
    const AnalyticNarrowPhaseResult guarded_parallel =
        intersect_analytic_curve_candidates({line(1, 0, 0, 1'000, 0), uncertain_line}, {{1, 2}});
    require(guarded_parallel.error == AnalyticNarrowPhaseError::resource_limit_exceeded,
            "an unresolved near-parallel denominator must not be labeled disjoint");

    AnalyticAtomicCurveNm uncertain_circle =
        filtered_arc(2, 1'040.0, 500.0, 540.0, 0.0, 1'040.0, 0.0, {500.0, 500.0}, true);
    uncertain_circle.start.x = {1'000.0, 1'080.0};
    uncertain_circle.end.x = {500.0, 580.0};
    uncertain_circle.circle.center.x = {1'000.0, 1'080.0};
    const AnalyticNarrowPhaseResult guarded_separation = intersect_analytic_curve_candidates(
        {filtered_arc(1, 500.0, 0.0, 0.0, 500.0, 0.0, 0.0, {500.0, 500.0}, true), uncertain_circle},
        {{1, 2}});
    require(guarded_separation.error == AnalyticNarrowPhaseError::resource_limit_exceeded &&
                guarded_separation.telemetry.resolution_collapses == 0,
            "circle uncertainty spanning an 80 nm gap must not certify a collapse");

    constexpr double base_radius = 1'000'000.0;
    const double eighty_nm_roots = std::sqrt(base_radius * base_radius + 1'600.0);
    AnalyticAtomicCurveNm uncertain_semicircle =
        filtered_arc(2, eighty_nm_roots, 0.0, -eighty_nm_roots, 0.0, 0.0, 0.0,
                     {base_radius, eighty_nm_roots}, true);
    uncertain_semicircle.has_arc_sweep_certificate = true;
    const AnalyticNarrowPhaseResult guarded_root_pair = intersect_analytic_curve_candidates(
        {filtered_line(1, -100.0, base_radius, 100.0, base_radius), uncertain_semicircle},
        {{1, 2}});
    require(guarded_root_pair.error == AnalyticNarrowPhaseError::resource_limit_exceeded &&
                guarded_root_pair.telemetry.resolution_collapses == 0,
            "two possible roots 80 nm apart must not share one representative");
}

void test_narrow_phase_tangent_certificates_and_root_threshold()
{
    constexpr std::int64_t large_radius = 500'000'000'000;
    const std::vector<AnalyticAtomicCurveNm> external = {
        arc(1, -large_radius, -large_radius, -large_radius, large_radius, -large_radius, 0,
            large_radius, true),
        arc(2, large_radius, large_radius, large_radius, -large_radius, large_radius, 0,
            large_radius, true),
    };
    const AnalyticNarrowPhaseResult external_contact =
        intersect_analytic_curve_candidates(external, {{1, 2}});
    require(external_contact.error == AnalyticNarrowPhaseError::none &&
                external_contact.intersections[0].relation == AnalyticPairRelation::point &&
                !external_contact.intersections[0].resolution_collapsed &&
                external_contact.telemetry.tangent_contacts == 1,
            "large exact external circle tangency must use its fixed-width certificate");

    const std::vector<AnalyticAtomicCurveNm> internal = {
        arc(1, 0, -large_radius, 0, large_radius, 0, 0, large_radius, true),
        arc(2, large_radius / 2, -large_radius / 2, large_radius / 2, large_radius / 2,
            large_radius / 2, 0, large_radius / 2, true),
    };
    const AnalyticNarrowPhaseResult internal_contact =
        intersect_analytic_curve_candidates(internal, {{1, 2}});
    require(internal_contact.error == AnalyticNarrowPhaseError::none &&
                internal_contact.intersections[0].relation == AnalyticPairRelation::point &&
                internal_contact.telemetry.tangent_contacts == 1 &&
                internal_contact.telemetry.resolution_collapses == 0,
            "large exact internal circle tangency must use its fixed-width certificate");

    constexpr std::int64_t line_circle_radius = 1'000'000'000'000;
    const AnalyticNarrowPhaseResult line_contact = intersect_analytic_curve_candidates(
        {line(1, -line_circle_radius, line_circle_radius, line_circle_radius, line_circle_radius),
         arc(2, line_circle_radius, 0, -line_circle_radius, 0, 0, 0, line_circle_radius, true)},
        {{1, 2}});
    require(line_contact.error == AnalyticNarrowPhaseError::none &&
                line_contact.intersections[0].relation == AnalyticPairRelation::point &&
                line_contact.telemetry.tangent_contacts == 1 &&
                line_contact.telemetry.resolution_collapses == 0,
            "large exact line/circle tangency must use its fixed-width certificate");

    const AnalyticNarrowPhaseResult line_height_25 = intersect_analytic_curve_candidates(
        {line(1, -100, 60, 100, 60), arc(2, 65, 0, -65, 0, 0, 0, 65, true)}, {{1, 2}});
    const AnalyticNarrowPhaseResult line_height_26 = intersect_analytic_curve_candidates(
        {line(1, -200, 168, 200, 168), arc(2, 170, 0, -170, 0, 0, 0, 170, true)}, {{1, 2}});
    require(line_height_25.error == AnalyticNarrowPhaseError::none &&
                line_height_25.intersections[0].relation == AnalyticPairRelation::point &&
                line_height_25.intersections[0].resolution_collapsed &&
                line_height_26.error == AnalyticNarrowPhaseError::none &&
                line_height_26.intersections[0].relation == AnalyticPairRelation::two_points &&
                !line_height_26.intersections[0].resolution_collapsed,
            "line/circle roots 50 nm apart may collapse but roots 52 nm apart must survive");

    const AnalyticNarrowPhaseResult circle_height_25 = intersect_analytic_curve_candidates(
        {arc(1, 0, -65, 0, 65, 0, 0, 65, true), arc(2, 120, 65, 120, -65, 120, 0, 65, true)},
        {{1, 2}});
    const AnalyticNarrowPhaseResult circle_height_26 = intersect_analytic_curve_candidates(
        {arc(1, 0, -170, 0, 170, 0, 0, 170, true), arc(2, 336, 170, 336, -170, 336, 0, 170, true)},
        {{1, 2}});
    require(circle_height_25.error == AnalyticNarrowPhaseError::none &&
                circle_height_25.intersections[0].relation == AnalyticPairRelation::point &&
                circle_height_25.intersections[0].resolution_collapsed &&
                circle_height_26.error == AnalyticNarrowPhaseError::none &&
                circle_height_26.intersections[0].relation == AnalyticPairRelation::two_points &&
                !circle_height_26.intersections[0].resolution_collapsed,
            "circle roots 50 nm apart may collapse but roots 52 nm apart must survive");

    const AnalyticNarrowPhaseResult strict_line_height_25 =
        analytic_execution_detail::intersect_curve_candidates(
            {line(1, -100, 60, 100, 60), arc(2, 65, 0, -65, 0, 0, 0, 65, true)}, {{1, 2}},
            kAnalyticSolverHardLimits, analytic_execution_detail::kStrictPublishedGeometry);
    const AnalyticNarrowPhaseResult strict_circle_height_25 =
        analytic_execution_detail::intersect_curve_candidates(
            {arc(1, 0, -65, 0, 65, 0, 0, 65, true), arc(2, 120, 65, 120, -65, 120, 0, 65, true)},
            {{1, 2}}, kAnalyticSolverHardLimits,
            analytic_execution_detail::kStrictPublishedGeometry);
    require(
        strict_line_height_25.error == AnalyticNarrowPhaseError::none &&
            strict_line_height_25.intersections[0].relation == AnalyticPairRelation::two_points &&
            !strict_line_height_25.intersections[0].resolution_collapsed &&
            strict_circle_height_25.error == AnalyticNarrowPhaseError::none &&
            strict_circle_height_25.intersections[0].relation == AnalyticPairRelation::two_points &&
            !strict_circle_height_25.intersections[0].resolution_collapsed,
        "strict published geometry collapsed distinct near-tangent roots");
}

void test_narrow_phase_arc_domains_and_contacts()
{
    const std::vector<AnalyticAtomicCurveNm> domain_curves = {
        line(1, -1'000, -1'000, 1'000, 1'000), arc(2, 500, 0, -500, 0, 0, 0, 500, true),
        line(3, -1'000, -300, 1'000, -300),    arc(4, 500, 0, 300, -400, 0, 0, 500, false),
        line(5, -1'000, 300, 1'000, 300),      arc(6, 500, 0, 300, -400, 0, 0, 500, true, true),
        line(7, -1'000, 300, 1'000, 300),      arc(8, 500, 0, 300, 400, 0, 0, 500, true),
    };
    const AnalyticNarrowPhaseResult domains =
        intersect_analytic_curve_candidates(domain_curves, {{1, 2}, {3, 4}, {5, 6}, {7, 8}});
    require(domains.error == AnalyticNarrowPhaseError::none &&
                domains.intersections[0].relation == AnalyticPairRelation::point &&
                domains.intersections[1].relation == AnalyticPairRelation::point &&
                domains.intersections[2].relation == AnalyticPairRelation::two_points &&
                domains.intersections[3].relation == AnalyticPairRelation::point &&
                contains(domains.intersections[1].points[0].x, 400.0) &&
                contains(domains.intersections[3].points[0].x, 400.0),
            "CW, sloped, minor, and major arc-domain filtering changed");

    const std::vector<AnalyticAtomicCurveNm> contact_curves = {
        arc(1, 100, 0, -100, 0, 0, 0, 100, true),
        arc(2, 100, 0, 0, 0, 50, 0, 50, true),
        arc(3, -100, 0, 100, 0, 0, 0, 100, false),
    };
    const AnalyticNarrowPhaseResult contacts =
        intersect_analytic_curve_candidates(contact_curves, {{1, 2}, {1, 3}});
    require(contacts.error == AnalyticNarrowPhaseError::none &&
                contacts.intersections[0].relation == AnalyticPairRelation::point &&
                !contacts.intersections[0].resolution_collapsed &&
                contacts.intersections[1].relation == AnalyticPairRelation::coincident &&
                contacts.telemetry.tangent_contacts == 1 &&
                contacts.telemetry.resolution_collapses == 0,
            "internal tangency and reversed coincident carriers must remain distinct from repair");
}

void test_narrow_phase_large_local_coordinates_and_limits()
{
    constexpr std::int64_t extent = 1'000'000'000'000;
    const std::vector<AnalyticAtomicCurveNm> crossing = {
        line(1, -extent, -extent, extent, extent),
        line(2, -extent, extent, extent, -extent),
    };
    const std::vector<AnalyticCurvePair> one_pair = {{1, 2}};
    const AnalyticNarrowPhaseResult accepted =
        intersect_analytic_curve_candidates(crossing, one_pair);
    require(accepted.error == AnalyticNarrowPhaseError::none &&
                accepted.intersections[0].relation == AnalyticPairRelation::point &&
                contains(accepted.intersections[0].points[0].x, 0.0) &&
                contains(accepted.intersections[0].points[0].y, 0.0),
            "1e12 nm local-coordinate crossing changed");

    AnalyticSolverLimits no_pairs = kAnalyticSolverHardLimits;
    no_pairs.examined_curve_pairs = 0;
    require(intersect_analytic_curve_candidates(crossing, one_pair, no_pairs).error ==
                AnalyticNarrowPhaseError::resource_limit_exceeded,
            "narrow phase must enforce its supplied pair ceiling");
    AnalyticSolverLimits one_curve = kAnalyticSolverHardLimits;
    one_curve.boundary_occurrences = 1;
    require(intersect_analytic_curve_candidates(crossing, {}, one_curve).error ==
                AnalyticNarrowPhaseError::resource_limit_exceeded,
            "curve-table validation must enforce the boundary-occurrence ceiling first");
    AnalyticSolverLimits no_predicates = kAnalyticSolverHardLimits;
    no_predicates.predicate_calls = 0;
    require(intersect_analytic_curve_candidates(crossing, one_pair, no_predicates).error ==
                AnalyticNarrowPhaseError::resource_limit_exceeded,
            "narrow phase must fail closed before an unbudgeted predicate");
    AnalyticSolverLimits no_intersections = kAnalyticSolverHardLimits;
    no_intersections.intersections = 0;
    require(intersect_analytic_curve_candidates(crossing, one_pair, no_intersections).error ==
                AnalyticNarrowPhaseError::resource_limit_exceeded,
            "narrow phase must enforce the intersection ceiling");
    AnalyticSolverLimits exact_memory = kAnalyticSolverHardLimits;
    exact_memory.working_memory_bytes = 256;
    require(intersect_analytic_curve_candidates(crossing, one_pair, exact_memory).error ==
                AnalyticNarrowPhaseError::none,
            "canonical narrow-phase memory charge changed");
    --exact_memory.working_memory_bytes;
    require(intersect_analytic_curve_candidates(crossing, one_pair, exact_memory).error ==
                AnalyticNarrowPhaseError::resource_limit_exceeded,
            "one byte below the canonical narrow-phase charge must fail");
}

void test_narrow_phase_resolution_endpoint_boundary()
{
    const std::vector<AnalyticAtomicCurveNm> curves = {
        line(1, 0, 0, 100, 0),
        line(2, 149, -10, 149, 10),
        line(3, 150, -10, 150, 10),
        line(4, 151, -10, 151, 10),
    };
    const AnalyticNarrowPhaseResult result =
        intersect_analytic_curve_candidates(curves, {{1, 2}, {1, 3}, {1, 4}});
    require(result.error == AnalyticNarrowPhaseError::none, "49/50/51 nm endpoint fixture failed");
    require(result.intersections[0].relation == AnalyticPairRelation::point &&
                result.intersections[1].relation == AnalyticPairRelation::point &&
                result.intersections[2].relation == AnalyticPairRelation::disjoint,
            "finite-domain repair must include 49 and 50 nm but preserve 51 nm separation");
    require(result.intersections[0].resolution_collapsed &&
                result.intersections[1].resolution_collapsed &&
                result.telemetry.resolution_collapses == 2,
            "49 and 50 nm endpoint repairs must be visible in telemetry");
}

void test_narrow_phase_pair_resolution_boundary()
{
    const std::vector<AnalyticAtomicCurveNm> diagonal = {
        line(1, -1000, 0, 0, 0),
        line(2, 35, 35, 35, 1000),
        line(3, 36, 36, 36, 1000),
    };
    const AnalyticNarrowPhaseResult diagonal_result =
        intersect_analytic_curve_candidates(diagonal, {{1, 2}, {1, 3}});
    require(diagonal_result.error == AnalyticNarrowPhaseError::none &&
                diagonal_result.intersections[0].relation == AnalyticPairRelation::point &&
                diagonal_result.intersections[0].resolution_collapsed &&
                diagonal_result.intersections[1].relation == AnalyticPairRelation::disjoint,
            "pair-level repair must bridge a 35/35 nm diagonal gap but preserve 36/36 nm");

    const std::vector<AnalyticAtomicCurveNm> true_intersection = {
        line(1, -1000, 0, 10, 0),
        line(2, 0, -1000, 0, 10),
    };
    const AnalyticNarrowPhaseResult true_result =
        intersect_analytic_curve_candidates(true_intersection, {{1, 2}});
    require(true_result.error == AnalyticNarrowPhaseError::none &&
                true_result.intersections[0].relation == AnalyticPairRelation::point &&
                !true_result.intersections[0].resolution_collapsed &&
                contains(true_result.intersections[0].points[0].x, 0.0) &&
                contains(true_result.intersections[0].points[0].y, 0.0),
            "a true carrier intersection near both endpoints must not be mislabeled as a repair");
}

void test_narrow_phase_candidate_driven_linear_work()
{
    auto fixture = [](std::uint32_t pair_count)
    {
        std::vector<AnalyticAtomicCurveNm> curves;
        std::vector<AnalyticCurvePair> pairs;
        curves.reserve(pair_count * 2);
        pairs.reserve(pair_count);
        for (std::uint32_t index = 0; index < pair_count; ++index)
        {
            const std::uint32_t first = index * 2 + 1;
            const std::int64_t x = static_cast<std::int64_t>(index) * 100;
            curves.push_back(line(first, x, 0, x + 10, 0));
            curves.push_back(line(first + 1, x, 100, x + 10, 100));
            pairs.push_back({first, first + 1});
        }
        return std::make_pair(std::move(curves), std::move(pairs));
    };
    auto small_fixture = fixture(1'000);
    auto large_fixture = fixture(2'000);
    const AnalyticNarrowPhaseResult small =
        intersect_analytic_curve_candidates(small_fixture.first, small_fixture.second);
    const AnalyticNarrowPhaseResult large =
        intersect_analytic_curve_candidates(large_fixture.first, large_fixture.second);
    require(small.error == AnalyticNarrowPhaseError::none &&
                large.error == AnalyticNarrowPhaseError::none &&
                small.telemetry.predicate_calls == 1'000 &&
                large.telemetry.predicate_calls == 2'000 &&
                large.telemetry.candidate_pairs_consumed == 2'000,
            "narrow phase must do constant predicate work per supplied disjoint pair");
    require(large.telemetry.peak_working_memory_bytes ==
                small.telemetry.peak_working_memory_bytes * 2,
            "narrow-phase working memory must scale linearly with supplied candidates");
}

void test_narrow_phase_direct_dense_index_resolution()
{
    auto fixture = [](std::uint32_t curve_count)
    {
        std::vector<AnalyticAtomicCurveNm> curves;
        curves.reserve(curve_count);
        curves.push_back(line(1, 0, 0, 10, 0));
        curves.push_back(line(2, 0, 100, 10, 100));
        for (std::uint32_t index = 2; index < curve_count; ++index)
        {
            const std::int64_t x = static_cast<std::int64_t>(index) * 100;
            curves.push_back(line(index + 1, x, 0, x + 10, 0));
        }
        return curves;
    };
    const AnalyticNarrowPhaseResult small =
        intersect_analytic_curve_candidates(fixture(128), {{1, 2}});
    const AnalyticNarrowPhaseResult large =
        intersect_analytic_curve_candidates(fixture(4'096), {{1, 2}});
    require(small.error == AnalyticNarrowPhaseError::none &&
                large.error == AnalyticNarrowPhaseError::none &&
                small.telemetry.curve_table_entries == 128 &&
                large.telemetry.curve_table_entries == 4'096 &&
                small.telemetry.curve_references_resolved == 2 &&
                large.telemetry.curve_references_resolved == 2 &&
                small.telemetry.predicate_calls == large.telemetry.predicate_calls,
            "pair resolution must remain direct when the validated curve table grows");
}

void test_narrow_phase_rejects_noncanonical_or_invalid_input()
{
    const std::vector<AnalyticAtomicCurveNm> curves = {line(1, 0, 0, 10, 0), line(2, 0, -1, 0, 1),
                                                       line(3, 20, 0, 30, 0)};
    require(intersect_analytic_curve_candidates(curves, {{2, 3}, {1, 2}}).error ==
                AnalyticNarrowPhaseError::invalid_argument,
            "noncanonical candidate order must be rejected, not internally resorted");
    require(intersect_analytic_curve_candidates(curves, {{1, 4}}).error ==
                AnalyticNarrowPhaseError::invalid_argument,
            "unknown candidate references must be rejected");
    std::vector<AnalyticAtomicCurveNm> invalid = curves;
    invalid[0].end = invalid[0].start;
    require(intersect_analytic_curve_candidates(invalid, {}).error ==
                AnalyticNarrowPhaseError::invalid_argument,
            "degenerate narrow-phase curves must be rejected");

    invalid = curves;
    invalid[0].start.x = {0.0, 1.0};
    require(intersect_analytic_curve_candidates(invalid, {}).error ==
                AnalyticNarrowPhaseError::invalid_argument,
            "an integer point certificate must bind a singleton filtered coordinate");

    AnalyticAtomicCurveNm invalid_radius = arc(1, 5, 0, -5, 0, 0, 0, 5, true);
    invalid_radius.integer_radius = 6;
    invalid_radius.circle.radius = {5.0, 6.0};
    require(intersect_analytic_curve_candidates({invalid_radius}, {}).error ==
                AnalyticNarrowPhaseError::invalid_argument,
            "an integer radius certificate must bind the certified endpoint radius");
}

} // namespace

int main()
{
    test_exact_zero_interval_multiplication();
    test_limits();
    test_resolution_filter();
    test_broad_phase_threshold_and_order();
    test_broad_phase_limits_and_validation();
    test_broad_phase_chooses_sparse_axis();
    test_broad_phase_avoids_crossed_projection_quadratic_work();
    test_broad_phase_memory_charge_is_cross_runtime_canonical();
    test_broad_phase_pair_capacity_growth_is_canonical();
    test_narrow_phase_line_line();
    test_exact_construction_line_domain_rejection();
    test_vertical_construction_column_tokens();
    test_narrow_phase_line_circle();
    test_horizontal_mirror_line_circle_root_tokens();
    test_narrow_phase_circle_circle_and_irrational_output();
    test_narrow_phase_filtered_authored_arcs();
    test_narrow_phase_near_tangent_displacement_guard();
    test_narrow_phase_tangent_certificates_and_root_threshold();
    test_narrow_phase_arc_domains_and_contacts();
    test_narrow_phase_large_local_coordinates_and_limits();
    test_narrow_phase_resolution_endpoint_boundary();
    test_narrow_phase_pair_resolution_boundary();
    test_narrow_phase_candidate_driven_linear_work();
    test_narrow_phase_direct_dense_index_resolution();
    test_narrow_phase_rejects_noncanonical_or_invalid_input();
    test_endpoint_authoritative_arc_certificate();
    std::cout << "ANALYTIC_FILTERED_CORE_VECTOR=" << narrow_phase_parity_vector() << '\n';
    return 0;
}

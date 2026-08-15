#include "geometer/analytic_curve_broad_phase.h"
#include "geometer/analytic_curve_narrow_phase.h"
#include "geometer/analytic_numeric_filter.h"
#include "geometer/analytic_solver_limits.h"

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

AnalyticAtomicCurveNm line(std::uint32_t index, std::int64_t x1, std::int64_t y1, std::int64_t x2,
                           std::int64_t y2)
{
    return {index, AnalyticAtomicCurveKind::line, {x1, y1}, {x2, y2}, {}, true, false};
}

AnalyticAtomicCurveNm arc(std::uint32_t index, std::int64_t x1, std::int64_t y1, std::int64_t x2,
                          std::int64_t y2, std::int64_t cx, std::int64_t cy, std::uint64_t radius,
                          bool counterclockwise, bool major = false)
{
    return {index,
            AnalyticAtomicCurveKind::circular_arc,
            {x1, y1},
            {x2, y2},
            {{cx, cy}, radius},
            counterclockwise,
            major};
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
        }
    }
    append_u64(result.telemetry.candidate_pairs_consumed);
    append_u64(result.telemetry.predicate_calls);
    append_u64(result.telemetry.square_root_calls);
    append_u64(result.telemetry.point_intersections);
    append_u64(result.telemetry.peak_working_memory_bytes);
    append_u64(result.telemetry.algebraic_fallback_calls);
    return output.str();
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
    require(result.telemetry.line_circle_pairs == 4 && result.telemetry.square_root_calls == 5,
            "line/circle work telemetry changed");
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
    require(result.telemetry.circle_circle_pairs == 3 &&
                result.telemetry.algebraic_fallback_calls == 0,
            "ordinary circle intersections must not enter algebraic fallback");
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
}

} // namespace

int main()
{
    test_limits();
    test_resolution_filter();
    test_broad_phase_threshold_and_order();
    test_broad_phase_limits_and_validation();
    test_broad_phase_chooses_sparse_axis();
    test_broad_phase_avoids_crossed_projection_quadratic_work();
    test_broad_phase_memory_charge_is_cross_runtime_canonical();
    test_narrow_phase_line_line();
    test_narrow_phase_line_circle();
    test_narrow_phase_circle_circle_and_irrational_output();
    test_narrow_phase_large_local_coordinates_and_limits();
    test_narrow_phase_resolution_endpoint_boundary();
    test_narrow_phase_candidate_driven_linear_work();
    test_narrow_phase_rejects_noncanonical_or_invalid_input();
    std::cout << "ANALYTIC_FILTERED_CORE_VECTOR=" << narrow_phase_parity_vector() << '\n';
    return 0;
}

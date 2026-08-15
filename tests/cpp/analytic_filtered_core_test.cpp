#include "geometer/analytic_curve_broad_phase.h"
#include "geometer/analytic_numeric_filter.h"
#include "geometer/analytic_solver_limits.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
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

} // namespace

int main()
{
    test_limits();
    test_resolution_filter();
    test_broad_phase_threshold_and_order();
    test_broad_phase_limits_and_validation();
    test_broad_phase_chooses_sparse_axis();
    test_broad_phase_avoids_crossed_projection_quadratic_work();
    return 0;
}

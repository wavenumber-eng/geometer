#pragma once

#include "geometer/analytic_curve_broad_phase.h"
#include "geometer/analytic_solver_limits.h"

#include <array>
#include <cstdint>
#include <vector>

namespace geometer
{

// Narrow-phase inputs are job-local integer nanometers. The lowering stage is
// responsible for choosing an origin that keeps every coordinate within the
// governed 1e12 nm span before constructing these values.
struct AnalyticIntegerPointNm
{
    std::int64_t x = 0;
    std::int64_t y = 0;
};

struct AnalyticIntegerCircleNm
{
    AnalyticIntegerPointNm center;
    std::uint64_t radius = 0;
};

enum class AnalyticAtomicCurveKind : std::uint8_t
{
    line = 0,
    circular_arc = 1,
};

struct AnalyticAtomicCurveNm
{
    std::uint32_t curve_index = 0;
    AnalyticAtomicCurveKind kind = AnalyticAtomicCurveKind::line;
    AnalyticIntegerPointNm start;
    AnalyticIntegerPointNm end;
    AnalyticIntegerCircleNm circle;
    bool counterclockwise = true;
    bool major_arc = false;
};

struct AnalyticCoordinateIntervalNm
{
    double lower = 0.0;
    double upper = 0.0;
};

struct AnalyticFilteredPointNm
{
    AnalyticCoordinateIntervalNm x;
    AnalyticCoordinateIntervalNm y;
};

enum class AnalyticPairRelation : std::uint8_t
{
    disjoint = 0,
    point = 1,
    two_points = 2,
    coincident = 3,
};

struct AnalyticPairIntersection
{
    AnalyticCurvePair pair;
    AnalyticPairRelation relation = AnalyticPairRelation::disjoint;
    std::uint8_t point_count = 0;
    bool resolution_collapsed = false;
    std::array<AnalyticFilteredPointNm, 2> points{};
};

enum class AnalyticNarrowPhaseError : std::uint8_t
{
    none = 0,
    invalid_argument = 1,
    resource_limit_exceeded = 2,
};

struct AnalyticNarrowPhaseTelemetry
{
    std::uint64_t candidate_pairs_consumed = 0;
    std::uint64_t line_line_pairs = 0;
    std::uint64_t line_circle_pairs = 0;
    std::uint64_t circle_circle_pairs = 0;
    std::uint64_t carrier_predicates = 0;
    std::uint64_t domain_predicates = 0;
    std::uint64_t square_root_calls = 0;
    std::uint64_t uncertain_predicates = 0;
    std::uint64_t resolution_collapses = 0;
    std::uint64_t point_intersections = 0;
    std::uint64_t coincident_pairs = 0;
    std::uint64_t predicate_calls = 0;
    std::uint64_t peak_working_memory_bytes = 0;
    std::uint64_t algebraic_fallback_calls = 0;
};

struct AnalyticNarrowPhaseResult
{
    AnalyticNarrowPhaseError error = AnalyticNarrowPhaseError::none;
    std::vector<AnalyticPairIntersection> intersections;
    AnalyticNarrowPhaseTelemetry telemetry;
};

// Intersects only the supplied broad-phase candidates. Curves and pairs must
// both be in strictly increasing canonical curve-index order. No implicit
// all-curves cross product is performed. Coincident carriers are identified
// here; their finite overlap spans are deliberately owned by the subsequent
// same-domain overlay stage.
[[nodiscard]] AnalyticNarrowPhaseResult
intersect_analytic_curve_candidates(const std::vector<AnalyticAtomicCurveNm>& curves,
                                    const std::vector<AnalyticCurvePair>& candidate_pairs,
                                    const AnalyticSolverLimits& limits = {});

} // namespace geometer

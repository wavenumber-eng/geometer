#pragma once

#include "geometer/analytic_curve_broad_phase.h"
#include "geometer/analytic_solver_limits.h"

#include <array>
#include <cstdint>
#include <vector>

namespace geometer
{

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

struct AnalyticFilteredCircleNm
{
    AnalyticFilteredPointNm center;
    AnalyticCoordinateIntervalNm radius;
};

struct AnalyticIntegerPointNm
{
    std::int64_t x = 0;
    std::int64_t y = 0;
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
    AnalyticFilteredPointNm start;
    AnalyticFilteredPointNm end;
    AnalyticFilteredCircleNm circle;
    bool counterclockwise = true;
    bool major_arc = false;
    bool has_integer_certificate = false;
    AnalyticIntegerPointNm integer_start;
    AnalyticIntegerPointNm integer_end;
    AnalyticIntegerPointNm integer_center;
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
    std::uint64_t curve_table_entries = 0;
    std::uint64_t curve_references_resolved = 0;
    std::uint64_t candidate_pairs_consumed = 0;
    std::uint64_t line_line_pairs = 0;
    std::uint64_t line_circle_pairs = 0;
    std::uint64_t circle_circle_pairs = 0;
    std::uint64_t carrier_predicates = 0;
    std::uint64_t domain_predicates = 0;
    std::uint64_t square_root_calls = 0;
    std::uint64_t uncertain_predicates = 0;
    std::uint64_t tangent_contacts = 0;
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

// Intersects only the supplied broad-phase candidates. Input coordinates and
// radii are outward bounds on one authored curve; each bound must itself fit
// the fixed 50 nm displacement envelope. Optional integer certificates enable
// exact fixed-width signs but are not required for non-integral authored arcs.
// Curves use dense canonical indices 1..N, and pairs are in strictly increasing
// index order; this makes pair resolution direct O(1) work instead of a hidden
// per-pair search. No implicit all-curves cross product is performed.
// Coincident carriers are identified here; their finite overlap spans are
// deliberately owned by the subsequent same-domain overlay stage.
[[nodiscard]] AnalyticNarrowPhaseResult
intersect_analytic_curve_candidates(const std::vector<AnalyticAtomicCurveNm>& curves,
                                    const std::vector<AnalyticCurvePair>& candidate_pairs,
                                    const AnalyticSolverLimits& limits = {});

} // namespace geometer

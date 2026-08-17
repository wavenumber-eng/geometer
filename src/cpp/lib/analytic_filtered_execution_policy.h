#pragma once

#include "geometer/analytic_curve_broad_phase.h"
#include "geometer/analytic_curve_narrow_phase.h"
#include "geometer/analytic_filtered_arrangement.h"
#include "geometer/analytic_filtered_boolean_selection.h"
#include "geometer/analytic_filtered_overlay.h"
#include "geometer/analytic_filtered_regions.h"

namespace geometer::analytic_execution_detail
{

enum class TopologyPolicy
{
    resolution_50nm,
    strict_published_geometry,
};

inline constexpr TopologyPolicy kDefaultTopologyPolicy = TopologyPolicy::resolution_50nm;
inline constexpr TopologyPolicy kStrictPublishedGeometry =
    TopologyPolicy::strict_published_geometry;

[[nodiscard]] inline constexpr bool allows_resolution_topology(TopologyPolicy policy) noexcept
{
    return policy == TopologyPolicy::resolution_50nm;
}

[[nodiscard]] AnalyticBroadPhaseResult
build_curve_candidates(const std::vector<AnalyticCurveBoundsNm>& bounds,
                       const AnalyticSolverLimits& limits, TopologyPolicy policy);

// Indexed two-color sweep used by published-geometry relationship evaluation.
// Curves [0,left_curve_count) are queried only against the remaining curves;
// no same-side candidate is examined or emitted.
[[nodiscard]] AnalyticBroadPhaseResult
build_bipartite_curve_candidates(const std::vector<AnalyticCurveBoundsNm>& bounds,
                                 std::uint32_t left_curve_count, const AnalyticSolverLimits& limits,
                                 TopologyPolicy policy);

[[nodiscard]] AnalyticNarrowPhaseResult
intersect_curve_candidates(const std::vector<AnalyticAtomicCurveNm>& curves,
                           const std::vector<AnalyticCurvePair>& candidate_pairs,
                           const AnalyticSolverLimits& limits, TopologyPolicy policy);

[[nodiscard]] AnalyticFilteredOverlayResult
build_overlay(const AnalyticFilteredGeometry& geometry,
              const std::vector<AnalyticCurvePair>& candidate_pairs,
              const AnalyticSolverLimits& limits, TopologyPolicy policy);

[[nodiscard]] AnalyticFilteredArrangementResult
build_arrangement(const AnalyticFilteredGeometry& geometry,
                  const std::vector<AnalyticCurvePair>& candidate_pairs,
                  const AnalyticSolverLimits& limits, TopologyPolicy policy);

[[nodiscard]] bool estimate_arrangement_minimum_requirements(
    const AnalyticFilteredGeometry& geometry, std::uint64_t pair_count,
    AnalyticFilteredArrangementMinimumRequirements& requirements, TopologyPolicy policy) noexcept;

[[nodiscard]] AnalyticFilteredBooleanSelectionResult
build_boolean_selection(const AnalyticRequestPacketRecords& records, std::uint32_t job_index,
                        const AnalyticFilteredGeometry& geometry,
                        const std::vector<AnalyticCurvePair>& candidate_pairs,
                        const AnalyticSolverLimits& limits, TopologyPolicy policy);

[[nodiscard]] AnalyticFilteredRegionsResult
build_regions(const AnalyticRequestPacketRecords& records, std::uint32_t job_index,
              const AnalyticFilteredGeometry& geometry,
              const std::vector<AnalyticCurvePair>& candidate_pairs,
              const AnalyticSolverLimits& limits, TopologyPolicy policy);

} // namespace geometer::analytic_execution_detail

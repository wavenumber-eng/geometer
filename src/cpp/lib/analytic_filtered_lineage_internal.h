#pragma once

#include "geometer/analytic_filtered_lineage.h"

namespace geometer::analytic_lineage_detail
{

// Owned continuation used only by filtered operand outcomes. It asks the
// upstream admission pass to reserve outcome history/projection before any
// arrangement allocation and returns the complete retained lineage value.
[[nodiscard]] AnalyticFilteredLineageResult
build_lineage_for_outcomes(const AnalyticRequestPacketRecords& records, std::uint32_t job_index,
                           const AnalyticFilteredGeometry& geometry,
                           const std::vector<AnalyticCurvePair>& candidate_pairs,
                           const AnalyticSolverLimits& limits);

} // namespace geometer::analytic_lineage_detail

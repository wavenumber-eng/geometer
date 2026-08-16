#pragma once

#include "geometer/analytic_filtered_lineage.h"

namespace geometer::analytic_lineage_detail
{

struct OutcomeOperandAssociation
{
    std::uint32_t owner = 0;
    std::uint32_t operand = 0;
};

struct OutcomeLineageResult
{
    AnalyticFilteredLineageResult lineage;
    // Canonical owner-major associations captured during the existing sparse
    // lineage traversal. They deliberately carry operand ordinals rather than
    // repeating each operand's complete authored source tuple.
    std::vector<OutcomeOperandAssociation> region_operands;
    std::vector<OutcomeOperandAssociation> boundary_subtractors;
};

// Owned continuation used only by filtered operand outcomes. It asks the
// upstream admission pass to reserve outcome history/projection before any
// arrangement allocation and returns the complete retained lineage value.
[[nodiscard]] OutcomeLineageResult
build_lineage_for_outcomes(const AnalyticRequestPacketRecords& records, std::uint32_t job_index,
                           const AnalyticFilteredGeometry& geometry,
                           const std::vector<AnalyticCurvePair>& candidate_pairs,
                           const AnalyticSolverLimits& limits);

} // namespace geometer::analytic_lineage_detail

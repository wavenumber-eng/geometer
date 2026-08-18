#pragma once

#include "analytic_filtered_boolean_selection_support.h"
#include "geometer/analytic_filtered_regions.h"

namespace geometer
{
namespace analytic_regions_detail
{

struct LineageRegionsAdmission
{
    AnalyticFilteredRegionsResult regions;
    std::uint64_t reserved_lineage_work = 0;
    std::uint64_t reserved_outcomes_work = 0;
};

[[nodiscard]] LineageRegionsAdmission
build_regions_for_lineage(const AnalyticRequestPacketRecords& records, std::uint32_t job_index,
                          const AnalyticFilteredGeometry& geometry,
                          const std::vector<AnalyticCurvePair>& candidate_pairs,
                          const AnalyticSolverLimits& limits, bool reserve_outcomes = false);

} // namespace analytic_regions_detail
} // namespace geometer

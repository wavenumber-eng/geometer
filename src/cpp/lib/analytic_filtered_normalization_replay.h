#pragma once

#include "geometer/analytic_filtered_regions.h"

#include <cstdint>
#include <vector>

namespace geometer::analytic_normalization_detail
{

enum class ReplayError : std::uint8_t
{
    none,
    invalid_argument,
    resource_limit_exceeded,
    topology_collapse,
};

struct ReplayResult
{
    ReplayError error = ReplayError::none;
    std::uint64_t work_units = 0;
    std::uint64_t peak_working_memory_bytes = 0;
    std::uint64_t required_working_memory_bytes = 0;
    std::uint64_t candidate_pairs = 0;
};

[[nodiscard]] ReplayResult
validate_normalized_replay(std::int64_t origin_x_nm, std::int64_t origin_y_nm,
                           const std::vector<AnalyticAtomicCurveNm>& curves,
                           const std::vector<AnalyticCurveBoundsNm>& bounds,
                           const AnalyticFilteredRegionsResult& original,
                           const AnalyticSolverLimits& limits);

} // namespace geometer::analytic_normalization_detail

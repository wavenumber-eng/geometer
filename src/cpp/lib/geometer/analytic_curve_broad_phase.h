#pragma once

#include "geometer/analytic_solver_limits.h"

#include <cstdint>
#include <vector>

namespace geometer
{

struct AnalyticCurveBoundsNm
{
    std::uint32_t curve_index = 0;
    double min_x = 0.0;
    double min_y = 0.0;
    double max_x = 0.0;
    double max_y = 0.0;
};

struct AnalyticCurvePair
{
    std::uint32_t first = 0;
    std::uint32_t second = 0;
};

enum class AnalyticBroadPhaseError : std::uint8_t
{
    none = 0,
    invalid_argument = 1,
    resource_limit_exceeded = 2,
};

struct AnalyticBroadPhaseTelemetry
{
    std::uint64_t input_curves = 0;
    std::uint64_t sort_comparisons = 0;
    std::uint64_t primary_axis_pairs = 0;
    std::uint64_t spatial_index_node_visits = 0;
    std::uint64_t examined_curve_pairs = 0;
    std::uint64_t candidate_pairs = 0;
    std::uint64_t work_units = 0;
    std::uint64_t retained_pair_bytes = 0;
    std::uint64_t required_working_memory_bytes = 0;
    std::uint64_t peak_working_memory_bytes = 0;
    std::uint64_t algebraic_fallback_calls = 0;
    std::uint8_t primary_axis = 0; // 0 = x, 1 = y
};

struct AnalyticBroadPhaseResult
{
    AnalyticBroadPhaseError error = AnalyticBroadPhaseError::none;
    std::vector<AnalyticCurvePair> pairs;
    AnalyticBroadPhaseTelemetry telemetry;
};

// Deterministic sweep over conservative curve bounds, with an interval index
// on the secondary axis. Bounds whose axis separation is exactly 50 nm remain
// candidates so the narrow phase can apply the governed
// at-or-below-resolution rule.
[[nodiscard]] AnalyticBroadPhaseResult
build_analytic_curve_candidates(const std::vector<AnalyticCurveBoundsNm>& bounds,
                                const AnalyticSolverLimits& limits = {});

} // namespace geometer

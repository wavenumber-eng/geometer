#pragma once

#include <cstdint>

namespace geometer
{

// Governed hard ceilings from analytic-planar-boolean-a0-catalog.toml. Hosts
// may enforce a smaller value object, but no execution may exceed these values.
struct AnalyticSolverLimits
{
    std::uint64_t boundary_occurrences = 131'072;
    std::uint64_t examined_curve_pairs = 8'388'608;
    std::uint64_t intersections = 1'048'576;
    std::uint64_t arrangement_vertices = 1'048'576;
    std::uint64_t arrangement_half_edges = 2'097'152;
    std::uint64_t arrangement_faces = 1'048'576;
    std::uint64_t provenance_references = 8'388'608;
    std::uint64_t source_reference_memberships = 8'388'608;
    std::uint64_t predicate_calls = 100'000'000;
    std::uint64_t working_memory_bytes = 1'073'741'824;
    std::uint64_t algebraic_fallback_calls = 0;
};

inline constexpr AnalyticSolverLimits kAnalyticSolverHardLimits{};

// The topology rule is contract behavior, not a resource budget or a
// caller-programmable tolerance.
inline constexpr std::int64_t kAnalyticCoordinateGridNm = 1;
inline constexpr std::int64_t kAnalyticTopologyResolutionNm = 50;

[[nodiscard]] bool
analytic_solver_limits_within_hard_ceilings(const AnalyticSolverLimits& limits) noexcept;

} // namespace geometer

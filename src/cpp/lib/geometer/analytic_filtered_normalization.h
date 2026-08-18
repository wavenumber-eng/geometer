#pragma once

#include "geometer/analytic_filtered_outcomes.h"

#include <cstdint>
#include <limits>
#include <vector>

namespace geometer
{

inline constexpr std::uint32_t kNoAnalyticNormalizedIndex =
    std::numeric_limits<std::uint32_t>::max();

struct AnalyticNormalizedVertexNm
{
    std::int64_t x_nm = 0;
    std::int64_t y_nm = 0;
    std::uint32_t arrangement_vertex = 0;
};

struct AnalyticNormalizedFragmentNm
{
    std::uint32_t start_vertex = 0;
    std::uint32_t end_vertex = 0;
    AnalyticAtomicCurveKind kind = AnalyticAtomicCurveKind::line;
    std::uint64_t radius_nm = 0;
    bool counterclockwise = true;
    bool major_arc = false;
    std::uint32_t old_boundary = 0;
};

struct AnalyticNormalizedRing
{
    std::uint32_t fragment_begin = 0;
    std::uint32_t fragment_count = 0;
    std::uint32_t parent_ring = kNoAnalyticNormalizedIndex;
    std::uint32_t depth = 0;
    bool counterclockwise = true;
    std::uint32_t old_ring = 0;
};

struct AnalyticNormalizedRegion
{
    std::uint32_t outer_ring = 0;
    std::uint32_t old_region = 0;
};

enum class AnalyticFilteredNormalizationError : std::uint8_t
{
    none = 0,
    invalid_argument = 1,
    resource_limit_exceeded = 2,
    normalization_error_exceeded = 3,
    normalization_topology_collapse = 4,
};

struct AnalyticFilteredNormalizationTelemetry
{
    std::uint64_t outcomes_work_units = 0;
    std::uint64_t outcomes_peak_working_memory_bytes = 0;
    std::uint64_t normalized_vertices = 0;
    std::uint64_t normalized_fragments = 0;
    std::uint64_t normalized_rings = 0;
    std::uint64_t normalized_regions = 0;
    std::uint64_t arc_critical_candidates = 0;
    std::uint64_t strict_replay_candidate_pairs = 0;
    std::uint64_t reserved_normalization_work_units = 0;
    std::uint64_t reserved_normalization_memory_bytes = 0;
    std::uint64_t normalization_work_units = 0;
    std::uint64_t predicate_calls = 0;
    std::uint64_t peak_working_memory_bytes = 0;
    std::uint64_t required_working_memory_bytes = 0;
    std::uint64_t algebraic_fallback_calls = 0;
};

struct AnalyticFilteredNormalizationResult
{
    AnalyticFilteredNormalizationError error = AnalyticFilteredNormalizationError::none;
    AnalyticFilteredOutcomesResult outcomes;
    std::vector<AnalyticNormalizedVertexNm> vertices;
    std::vector<AnalyticNormalizedFragmentNm> fragments;
    std::vector<std::uint32_t> ring_fragments;
    std::vector<AnalyticNormalizedRing> rings;
    std::vector<AnalyticNormalizedRegion> regions;
    std::vector<std::uint32_t> old_vertex_to_normalized;
    std::vector<std::uint32_t> old_boundary_to_normalized;
    std::vector<std::uint32_t> old_ring_to_normalized;
    std::vector<std::uint32_t> old_region_to_normalized;
    AnalyticFilteredNormalizationTelemetry telemetry;
};

// Owned one-time publication stage. It consumes request records and trusted
// filtered geometry directly, retains the accepted outcome topology, chooses
// one integer-nm representative per used resolved vertex, reconstructs
// endpoint-authoritative line/arc fragments, certifies the complete 50 nm
// Hausdorff envelope, and rejects any strict replay topology change. It never
// accepts caller-built topology and never invokes the algebraic solver.
[[nodiscard]] AnalyticFilteredNormalizationResult build_analytic_filtered_normalization(
    const AnalyticRequestPacketRecords& records, std::uint32_t job_index,
    const AnalyticFilteredGeometry& geometry, const std::vector<AnalyticCurvePair>& candidate_pairs,
    const AnalyticSolverLimits& limits = {});

} // namespace geometer

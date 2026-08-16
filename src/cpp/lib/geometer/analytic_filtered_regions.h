#pragma once

#include "geometer/analytic_filtered_boolean_selection.h"

#include <cstdint>
#include <limits>
#include <vector>

namespace geometer
{

inline constexpr std::uint32_t kNoAnalyticFilteredRing = std::numeric_limits<std::uint32_t>::max();

struct AnalyticFilteredMaterialRing
{
    std::uint32_t half_edge_begin = 0;
    std::uint32_t half_edge_count = 0;
    std::uint32_t parent_ring = kNoAnalyticFilteredRing;
    std::uint32_t depth = 0;
    bool counterclockwise = true;
};

struct AnalyticFilteredMaterialRegion
{
    std::uint32_t outer_ring = 0;
    std::uint32_t material_component = 0;
};

enum class AnalyticFilteredRegionsError : std::uint8_t
{
    none = 0,
    invalid_argument = 1,
    resource_limit_exceeded = 2,
};

struct AnalyticFilteredRegionsTelemetry
{
    std::uint64_t selection_predicate_calls = 0;
    std::uint64_t selection_peak_working_memory_bytes = 0;
    std::uint64_t disjoint_set_node_visits = 0;
    std::uint64_t boundary_half_edges = 0;
    std::uint64_t vertex_rotation_visits = 0;
    std::uint64_t emitted_rings = 0;
    std::uint64_t emitted_regions = 0;
    std::uint64_t sort_work_units = 0;
    std::uint64_t region_work_units = 0;
    std::uint64_t reserved_region_work_units = 0;
    std::uint64_t predicate_calls = 0;
    std::uint64_t peak_working_memory_bytes = 0;
    std::uint64_t algebraic_fallback_calls = 0;
};

struct AnalyticFilteredRegionsResult
{
    AnalyticFilteredRegionsError error = AnalyticFilteredRegionsError::none;
    // Retained because the following publication/lineage slice consumes the
    // certified arrangement, coverage roots, and occurrence binding without a
    // caller-constructible intermediate topology boundary.
    AnalyticFilteredBooleanSelectionResult selection;
    std::vector<AnalyticFilteredMaterialRing> rings;
    std::vector<std::uint32_t> ring_half_edges;
    std::vector<AnalyticFilteredMaterialRegion> regions;
    std::vector<std::uint32_t> face_components;
    AnalyticFilteredRegionsTelemetry telemetry;
};

// Internal production pipeline stage. It owns face selection from trusted
// filtered lowering output and canonical broad-phase pairs, then extracts only
// material/empty boundary half-edges. One boundary successor is derived from
// each vertex's already-certified rotation before rings are traced, keeping
// total boundary work linear even at high-degree coincident seams. Equal-
// material faces are grouped through the indexed dual graph; ring parentage and
// regions are obtained by one component-graph traversal, never cycle-pair
// containment tests.
[[nodiscard]] AnalyticFilteredRegionsResult
build_analytic_filtered_regions(const AnalyticRequestPacketRecords& records,
                                std::uint32_t job_index, const AnalyticFilteredGeometry& geometry,
                                const std::vector<AnalyticCurvePair>& candidate_pairs,
                                const AnalyticSolverLimits& limits = {});

} // namespace geometer

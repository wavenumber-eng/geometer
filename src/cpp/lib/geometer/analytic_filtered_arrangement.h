#pragma once

#include "geometer/analytic_filtered_overlay.h"
#include "geometer/analytic_solver_limits.h"

#include <cstdint>
#include <vector>

namespace geometer
{

inline constexpr std::uint64_t kAnalyticOverlayMembershipLogicalBytes = 8;
inline constexpr std::uint64_t kAnalyticArrangementVertexLogicalBytes = 48;
inline constexpr std::uint64_t kAnalyticArrangementEdgeLogicalBytes = 192;
inline constexpr std::uint64_t kAnalyticArrangementHalfEdgeLogicalBytes = 32;
inline constexpr std::uint64_t kAnalyticArrangementCollapsedSpanLogicalBytes = 24;
inline constexpr std::uint64_t kAnalyticArrangementCycleLogicalBytes = 24;

struct AnalyticArrangementVertexNm
{
    AnalyticFilteredPointNm point;
    std::uint32_t outgoing_begin = 0;
    std::uint32_t outgoing_count = 0;
};

struct AnalyticArrangementEdgeNm
{
    std::uint32_t start_vertex = 0;
    std::uint32_t end_vertex = 0;
    std::uint32_t carrier_curve_index = 0;
    AnalyticAtomicCurveKind kind = AnalyticAtomicCurveKind::line;
    AnalyticFilteredPointNm carrier_start;
    AnalyticFilteredPointNm carrier_end;
    AnalyticFilteredCircleNm circle;
    bool counterclockwise = true;
    bool major_arc = false;
    std::uint32_t membership_begin = 0;
    std::uint32_t membership_count = 0;
    AnalyticXMonotoneBranch x_monotone_branch = AnalyticXMonotoneBranch::none;
    bool endpoint_authoritative_arc = false;
    bool has_construction_line_direction = false;
    std::int64_t construction_line_dx = 0;
    std::int64_t construction_line_dy = 0;
};

struct AnalyticArrangementHalfEdge
{
    std::uint32_t origin_vertex = 0;
    std::uint32_t twin = 0;
    std::uint32_t next = 0;
    std::uint32_t previous = 0;
    std::uint32_t edge = 0;
    bool forward = true;
    std::uint32_t cycle = 0;
};

struct AnalyticArrangementCollapsedSpan
{
    std::uint32_t vertex = 0;
    std::uint32_t carrier_curve_index = 0;
    std::uint32_t membership_begin = 0;
    std::uint32_t membership_count = 0;
};

struct AnalyticArrangementCycle
{
    std::uint32_t half_edge_begin = 0;
    std::uint32_t half_edge_count = 0;
    std::uint32_t component = 0;
    bool counterclockwise = true;
};

enum class AnalyticFilteredArrangementError : std::uint8_t
{
    none = 0,
    invalid_argument = 1,
    resource_limit_exceeded = 2,
};

struct AnalyticFilteredArrangementTelemetry
{
    std::uint64_t admission_work_units = 0;
    std::uint64_t input_spans = 0;
    std::uint64_t input_memberships = 0;
    std::uint64_t endpoint_records = 0;
    std::uint64_t endpoint_index_node_visits = 0;
    std::uint64_t endpoint_index_update_work_units = 0;
    std::uint64_t merged_endpoint_records = 0;
    std::uint64_t collapsed_spans = 0;
    std::uint64_t emitted_vertices = 0;
    std::uint64_t emitted_edges = 0;
    std::uint64_t emitted_half_edges = 0;
    std::uint64_t emitted_cycles = 0;
    std::uint64_t sort_work_units = 0;
    std::uint64_t angular_predicates = 0;
    std::uint64_t overlay_predicate_calls = 0;
    std::uint64_t overlay_peak_working_memory_bytes = 0;
    std::uint64_t predicate_calls = 0;
    std::uint64_t peak_working_memory_bytes = 0;
    std::uint64_t required_working_memory_bytes = 0;
    std::uint64_t algebraic_fallback_calls = 0;
};

struct AnalyticFilteredArrangementResult
{
    AnalyticFilteredArrangementError error = AnalyticFilteredArrangementError::none;
    std::vector<AnalyticArrangementVertexNm> vertices;
    std::vector<AnalyticArrangementEdgeNm> edges;
    std::vector<AnalyticArrangementHalfEdge> half_edges;
    std::vector<std::uint32_t> outgoing_half_edges;
    std::vector<AnalyticArrangementCollapsedSpan> collapsed_spans;
    std::vector<AnalyticSpanMembership> memberships;
    std::vector<AnalyticArrangementCycle> cycles;
    std::vector<std::uint32_t> cycle_half_edges;
    AnalyticFilteredArrangementTelemetry telemetry;
};

struct AnalyticFilteredArrangementMinimumRequirements
{
    std::uint64_t guaranteed_spans = 0;
    std::uint64_t guaranteed_collapsed_vertices = 0;
    std::uint64_t possible_base_spans = 0;
    std::uint64_t possible_base_memberships = 0;
    std::uint64_t possible_collapsed_domains = 0;
    std::uint64_t possible_circular_carrier_groups = 0;
    std::uint64_t working_memory_bytes = 0;
    // Excludes the arrangement entry point's separately reported admission
    // scan, matching AnalyticFilteredArrangementTelemetry.
    std::uint64_t predicate_calls = 0;
};

// Allocation-free estimate shared by integrated downstream stages so they can
// reserve their own unavoidable work and live output before arrangement work
// begins. False means checked arithmetic could not represent the estimate.
[[nodiscard]] bool estimate_analytic_filtered_arrangement_minimum_requirements(
    const AnalyticFilteredGeometry& geometry, std::uint64_t pair_count,
    AnalyticFilteredArrangementMinimumRequirements& requirements) noexcept;

// Runs narrow/same-carrier overlay internally for only the supplied canonical
// broad-phase pairs, reconciles the resulting span endpoints into one global
// vertex table, then constructs a deterministic half-edge graph and boundary
// cycles. No public boundary accepts a caller-constructed overlay. Endpoint
// clusters have a complete outward diameter no greater than 50 nm; proximity
// candidates come from an indexed sweep and every visit is budgeted. Outgoing
// germs are total-key sorted and then certified with outward tangent/curvature
// predicates before topology is linked. Domains already collapsed by the
// overlay retain isolated vertices and occurrence lineage. Face ownership is
// deliberately the next indexed stage; this boundary performs no cycle-pair
// containment scan. A zero-allocation admission pass reserves the unavoidable
// downstream memory/work for proven distinct carrier spans before narrow or
// overlay execution. The proportional admission scan is itself bulk-charged
// before traversal, so a job whose work ceiling cannot admit the scan performs
// no scan or upstream work.
[[nodiscard]] AnalyticFilteredArrangementResult
build_analytic_filtered_arrangement(const AnalyticFilteredGeometry& geometry,
                                    const std::vector<AnalyticCurvePair>& candidate_pairs,
                                    const AnalyticSolverLimits& limits = {});

} // namespace geometer

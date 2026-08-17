#pragma once

#include "geometer/analytic_filtered_arrangement.h"
#include "geometer/analytic_request_packet.h"
#include "geometer/analytic_solver_limits.h"

#include <cstdint>
#include <limits>
#include <vector>

namespace geometer
{

struct AnalyticFilteredSelectedFace
{
    std::uint32_t boundary_cycle_begin = 0;
    std::uint32_t boundary_cycle_count = 0;
    std::uint32_t coverage_state_root = 0;
    // Ordered-stage lineage descriptors. For a material face, surviving
    // positive operands are the active union operands in stages at or after
    // positive_stage_begin. For an empty face, active_removal_stage names the
    // first active difference after the last active union; it is the only
    // subtraction epoch that can own the final material/empty boundary.
    std::uint32_t positive_stage_begin = 0;
    std::uint32_t active_removal_stage = std::numeric_limits<std::uint32_t>::max();
    bool unbounded = false;
    bool material = false;
};

struct AnalyticFilteredCoverageStateNode
{
    std::uint32_t left = 0;
    std::uint32_t right = 0;
};

// Monotone positive-area history collected only by the owned filtered-outcome
// pipeline while the canonical face-dual traversal is already live. These are
// coordinate-free stage facts, not geometric classifications. Standalone face
// selection leaves this table empty.
struct AnalyticFilteredOperandOutcomeEvidence
{
    std::uint64_t operand_id = 0;
    bool covered_positive_area = false;
    bool redundant_or_absorbed = false;
    bool removed_later = false;
    bool attributed_removal = false;
    bool unfilled_removal = false;
    bool overwritten = false;
};

enum class AnalyticFilteredBooleanSelectionError : std::uint8_t
{
    none = 0,
    invalid_argument = 1,
    resource_limit_exceeded = 2,
};

struct AnalyticFilteredBooleanSelectionTelemetry
{
    std::uint64_t admission_work_units = 0;
    std::uint64_t input_stages = 0;
    std::uint64_t input_operands = 0;
    std::uint64_t input_cycles = 0;
    std::uint64_t event_columns = 0;
    std::uint64_t resolution_event_columns = 0;
    std::uint64_t sweep_status_node_visits = 0;
    std::uint64_t sweep_status_update_work_units = 0;
    std::uint64_t disjoint_set_node_visits = 0;
    std::uint64_t face_gap_unions = 0;
    std::uint64_t emitted_faces = 0;
    std::uint64_t transition_records = 0;
    std::uint64_t dual_adjacency_visits = 0;
    std::uint64_t non_tree_edge_validations = 0;
    std::uint64_t coverage_state_nodes = 0;
    std::uint64_t coverage_state_table_probes = 0;
    std::uint64_t coverage_state_update_work_units = 0;
    std::uint64_t stage_state_update_work_units = 0;
    std::uint64_t outcome_stage_state_update_work_units = 0;
    std::uint64_t outcome_reporter_node_visits = 0;
    std::uint64_t outcome_evidence_flags_set = 0;
    std::uint64_t material_faces = 0;
    std::uint64_t sort_work_units = 0;
    std::uint64_t arrangement_predicate_calls = 0;
    std::uint64_t arrangement_peak_working_memory_bytes = 0;
    std::uint64_t predicate_calls = 0;
    std::uint64_t peak_working_memory_bytes = 0;
    std::uint64_t required_working_memory_bytes = 0;
    std::uint64_t algebraic_fallback_calls = 0;
};

struct AnalyticFilteredBooleanSelectionResult
{
    AnalyticFilteredBooleanSelectionError error = AnalyticFilteredBooleanSelectionError::none;
    std::int64_t origin_x_nm = 0;
    std::int64_t origin_y_nm = 0;
    AnalyticFilteredArrangementResult arrangement;
    std::vector<AnalyticFilteredOccurrence> occurrences;
    std::vector<std::uint32_t> half_edge_faces;
    std::vector<AnalyticFilteredSelectedFace> faces;
    std::vector<std::uint32_t> face_boundary_cycles;
    // Node 0 is the all-zero subtree and node 1 is the set leaf. Remaining
    // nodes are canonical (left,right) pairs. A face root therefore represents
    // its complete active-operand set without copying that set per face.
    std::vector<AnalyticFilteredCoverageStateNode> coverage_state_nodes;
    std::vector<AnalyticFilteredOperandOutcomeEvidence> outcome_evidence;
    AnalyticFilteredBooleanSelectionTelemetry telemetry;
};

// Internal production pipeline stage. It owns arrangement construction from
// trusted filtered lowering output and canonical broad-phase pairs; no caller
// can inject a DCEL, face assignment, coverage state, or selected material.
// The selected job's ordered union/difference records are rebound to every
// lowering occurrence before an indexed vertical-slab sweep constructs face
// ownership. Coverage propagation and stage evaluation visit only actual
// edge/coverage and face/coverage incidences, never every face/operand pair.
[[nodiscard]] AnalyticFilteredBooleanSelectionResult build_analytic_filtered_boolean_selection(
    const AnalyticRequestPacketRecords& records, std::uint32_t job_index,
    const AnalyticFilteredGeometry& geometry, const std::vector<AnalyticCurvePair>& candidate_pairs,
    const AnalyticSolverLimits& limits = {});

} // namespace geometer

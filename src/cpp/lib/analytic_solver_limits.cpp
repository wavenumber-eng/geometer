#include "geometer/analytic_solver_limits.h"

namespace geometer
{

bool analytic_solver_limits_within_hard_ceilings(const AnalyticSolverLimits& limits) noexcept
{
    return limits.boundary_occurrences <= kAnalyticSolverHardLimits.boundary_occurrences &&
           limits.candidate_curve_pairs <= kAnalyticSolverHardLimits.candidate_curve_pairs &&
           limits.intersections <= kAnalyticSolverHardLimits.intersections &&
           limits.arrangement_vertices <= kAnalyticSolverHardLimits.arrangement_vertices &&
           limits.arrangement_half_edges <= kAnalyticSolverHardLimits.arrangement_half_edges &&
           limits.arrangement_faces <= kAnalyticSolverHardLimits.arrangement_faces &&
           limits.provenance_references <= kAnalyticSolverHardLimits.provenance_references &&
           limits.source_reference_memberships <=
               kAnalyticSolverHardLimits.source_reference_memberships &&
           limits.predicate_calls <= kAnalyticSolverHardLimits.predicate_calls &&
           limits.working_memory_bytes <= kAnalyticSolverHardLimits.working_memory_bytes &&
           limits.algebraic_fallback_calls <= kAnalyticSolverHardLimits.algebraic_fallback_calls;
}

} // namespace geometer

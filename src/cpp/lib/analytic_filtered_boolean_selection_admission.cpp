#include "analytic_filtered_boolean_selection_support.h"

namespace geometer
{
namespace analytic_selection_detail
{
SelectionAdmission prepare_boolean_selection_admission(
    const AnalyticRequestPacketRecords& records, std::uint32_t job_index,
    const AnalyticFilteredGeometry& geometry, const std::vector<AnalyticCurvePair>& candidate_pairs,
    const AnalyticSolverLimits& limits)
{
    SelectionAdmission admission;
    AnalyticFilteredBooleanSelectionResult& preflight = admission.result;
    if (!analytic_solver_limits_within_hard_ceilings(limits) || job_index >= records.jobs.size() ||
        geometry.curves.size() != geometry.bounds.size() ||
        geometry.curves.size() != geometry.occurrences.size())
    {
        preflight.error = AnalyticFilteredBooleanSelectionError::invalid_argument;
        return admission;
    }
    const AnalyticRequestJobRecord& job = records.jobs[job_index];
    if (job.stage_begin > records.stages.size() ||
        job.stage_count > records.stages.size() - job.stage_begin)
    {
        preflight.error = AnalyticFilteredBooleanSelectionError::invalid_argument;
        return admission;
    }
    if (geometry.curves.size() > limits.boundary_occurrences ||
        candidate_pairs.size() > limits.examined_curve_pairs)
    {
        preflight.error = AnalyticFilteredBooleanSelectionError::resource_limit_exceeded;
        return admission;
    }
    bool valid = true;
    std::uint64_t admission_work =
        checked_add(geometry.curves.size(), geometry.occurrences.size(), valid);
    admission_work = checked_add(admission_work, job.stage_count, valid);
    if (!valid || admission_work > limits.predicate_calls)
    {
        preflight.error = AnalyticFilteredBooleanSelectionError::resource_limit_exceeded;
        return admission;
    }
    preflight.telemetry.admission_work_units = admission_work;
    preflight.telemetry.predicate_calls = admission_work;
    std::uint64_t operands = 0;
    for (std::uint32_t local = 0; local < job.stage_count; ++local)
    {
        const AnalyticRequestStageRecord& stage = records.stages[job.stage_begin + local];
        if (stage.stage_id == 0 || (stage.operation != 1 && stage.operation != 2) ||
            stage.operand_begin > records.operands.size() ||
            stage.operand_count > records.operands.size() - stage.operand_begin)
        {
            preflight.error = AnalyticFilteredBooleanSelectionError::invalid_argument;
            return admission;
        }
        if (operands > limits.boundary_occurrences ||
            stage.operand_count > limits.boundary_occurrences - operands)
        {
            preflight.error = AnalyticFilteredBooleanSelectionError::resource_limit_exceeded;
            return admission;
        }
        operands += stage.operand_count;
    }
    admission_work = checked_add(admission_work, operands, valid);
    admission_work = checked_add(admission_work, sort_units(operands), valid);
    admission_work = checked_add(
        admission_work,
        checked_multiply(geometry.occurrences.size(), tree_operation_units(operands), valid),
        valid);
    if (!valid || admission_work > limits.predicate_calls)
    {
        preflight.error = AnalyticFilteredBooleanSelectionError::resource_limit_exceeded;
        return admission;
    }
    preflight.telemetry.admission_work_units = admission_work;
    preflight.telemetry.predicate_calls = admission_work;

    std::uint64_t admission_peak_memory = checked_multiply(operands, kIndexLogicalBytes, valid);
    admission_peak_memory = checked_add(
        admission_peak_memory, checked_multiply(operands, kByteLogicalBytes, valid), valid);
    if (!valid || admission_peak_memory > limits.working_memory_bytes)
    {
        preflight.error = AnalyticFilteredBooleanSelectionError::resource_limit_exceeded;
        return admission;
    }
    preflight.telemetry.peak_working_memory_bytes = admission_peak_memory;
    try
    {
        std::vector<std::uint64_t> operand_ids;
        operand_ids.reserve(static_cast<std::size_t>(operands));
        for (std::uint32_t local = 0; local < job.stage_count; ++local)
        {
            const AnalyticRequestStageRecord& stage = records.stages[job.stage_begin + local];
            for (std::uint32_t offset = 0; offset < stage.operand_count; ++offset)
            {
                const AnalyticRequestOperandRecord& operand =
                    records.operands[stage.operand_begin + offset];
                if (operand.operand_id == 0 ||
                    (offset != 0 && records.operands[stage.operand_begin + offset - 1].operand_id >=
                                        operand.operand_id))
                {
                    preflight.error = AnalyticFilteredBooleanSelectionError::invalid_argument;
                    return admission;
                }
                operand_ids.push_back(operand.operand_id);
            }
        }
        std::sort(operand_ids.begin(), operand_ids.end());
        if (std::adjacent_find(operand_ids.begin(), operand_ids.end()) != operand_ids.end())
        {
            preflight.error = AnalyticFilteredBooleanSelectionError::invalid_argument;
            return admission;
        }
        std::vector<std::uint8_t> used(operand_ids.size());
        for (std::uint32_t index = 0; index < geometry.occurrences.size(); ++index)
        {
            const AnalyticFilteredOccurrence& occurrence = geometry.occurrences[index];
            const auto found =
                std::lower_bound(operand_ids.begin(), operand_ids.end(), occurrence.coverage_id);
            if (occurrence.occurrence_id != static_cast<std::uint64_t>(index) + 1 ||
                occurrence.coverage_id == 0 ||
                occurrence.source.operand_id != occurrence.coverage_id ||
                found == operand_ids.end() || *found != occurrence.coverage_id)
            {
                preflight.error = AnalyticFilteredBooleanSelectionError::invalid_argument;
                return admission;
            }
            used[static_cast<std::size_t>(found - operand_ids.begin())] = 1;
        }
        if (std::find(used.begin(), used.end(), 0) != used.end())
        {
            preflight.error = AnalyticFilteredBooleanSelectionError::invalid_argument;
            return admission;
        }
    }
    catch (const std::bad_alloc&)
    {
        preflight.error = AnalyticFilteredBooleanSelectionError::resource_limit_exceeded;
        return admission;
    }

    AnalyticFilteredArrangementMinimumRequirements arrangement_minimum;
    if (!estimate_analytic_filtered_arrangement_minimum_requirements(
            geometry, candidate_pairs.size(), arrangement_minimum))
    {
        preflight.error = AnalyticFilteredBooleanSelectionError::resource_limit_exceeded;
        return admission;
    }
    std::uint64_t selection_memory =
        checked_multiply(geometry.occurrences.size(), kOccurrenceLogicalBytes, valid);
    selection_memory = checked_add(
        selection_memory,
        checked_multiply(operands, kOperandMetadataLogicalBytes + kByteLogicalBytes, valid), valid);
    selection_memory = checked_add(
        selection_memory, checked_multiply(job.stage_count, kByteLogicalBytes, valid), valid);
    selection_memory =
        checked_add(selection_memory, kFaceLogicalBytes + kCoverageNodeLogicalBytes * 2, valid);
    constexpr std::uint64_t per_span_selection_memory =
        kSweepEdgeLogicalBytes + kSweepNodeLogicalBytes + kIndexLogicalBytes +
        kEventReferenceLogicalBytes * 2 + kSweepTemporaryLogicalBytes + kIndexLogicalBytes * 2;
    selection_memory = checked_add(
        selection_memory,
        checked_multiply(arrangement_minimum.guaranteed_spans, per_span_selection_memory, valid),
        valid);
    const std::uint64_t integrated_minimum_memory =
        checked_add(arrangement_minimum.working_memory_bytes, selection_memory, valid);

    std::uint64_t selection_work = sort_units(operands);
    selection_work = checked_add(selection_work, job.stage_count + operands * 2, valid);
    selection_work = checked_add(
        selection_work,
        checked_multiply(geometry.occurrences.size(), tree_operation_units(operands), valid),
        valid);
    selection_work = checked_add(selection_work, geometry.curves.size(), valid);
    selection_work = checked_add(
        selection_work, checked_multiply(arrangement_minimum.guaranteed_spans, 11, valid), valid);
    selection_work = checked_add(
        selection_work,
        sort_units(checked_multiply(arrangement_minimum.guaranteed_spans, 2, valid)), valid);
    std::uint64_t stage_leaf_capacity = 1;
    while (stage_leaf_capacity < std::max<std::uint64_t>(1, job.stage_count))
        stage_leaf_capacity *= 2;
    selection_work = checked_add(selection_work,
                                 7 + job.stage_count + stage_leaf_capacity * 2 + operands, valid);
    std::uint64_t integrated_arrangement_work =
        checked_add(geometry.curves.size(), arrangement_minimum.predicate_calls, valid);
    integrated_arrangement_work = checked_add(integrated_arrangement_work, selection_work, valid);
    const std::uint64_t remaining_work = limits.predicate_calls - admission_work;
    if (!valid || integrated_minimum_memory > limits.working_memory_bytes ||
        integrated_arrangement_work > remaining_work)
    {
        preflight.error = AnalyticFilteredBooleanSelectionError::resource_limit_exceeded;
        return admission;
    }
    admission_peak_memory = std::max(admission_peak_memory, integrated_minimum_memory);
    preflight.telemetry.peak_working_memory_bytes = admission_peak_memory;

    AnalyticSolverLimits arrangement_limits = limits;
    arrangement_limits.predicate_calls = remaining_work - selection_work;
    AnalyticFilteredArrangementResult arrangement =
        build_analytic_filtered_arrangement(geometry, candidate_pairs, arrangement_limits);
    if (arrangement.error != AnalyticFilteredArrangementError::none)
    {
        preflight.error = arrangement.error == AnalyticFilteredArrangementError::invalid_argument
                              ? AnalyticFilteredBooleanSelectionError::invalid_argument
                              : AnalyticFilteredBooleanSelectionError::resource_limit_exceeded;
        preflight.telemetry.arrangement_predicate_calls = arrangement.telemetry.predicate_calls;
        preflight.telemetry.arrangement_peak_working_memory_bytes =
            arrangement.telemetry.peak_working_memory_bytes;
        preflight.telemetry.predicate_calls =
            admission_work + arrangement.telemetry.predicate_calls;
        preflight.telemetry.peak_working_memory_bytes =
            std::max(admission_peak_memory, arrangement.telemetry.peak_working_memory_bytes);
        preflight.telemetry.algebraic_fallback_calls =
            arrangement.telemetry.algebraic_fallback_calls;
        return admission;
    }
    admission.arrangement = std::move(arrangement);
    admission.admission_work = admission_work;
    admission.admission_peak_memory = admission_peak_memory;
    admission.ready = true;
    return admission;
}

} // namespace analytic_selection_detail
} // namespace geometer

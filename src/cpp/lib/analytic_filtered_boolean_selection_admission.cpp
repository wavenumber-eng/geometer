#include "analytic_filtered_boolean_selection_support.h"
#include "analytic_filtered_capacity.h"
#include "analytic_filtered_outcome_tracker.h"

namespace geometer
{
namespace analytic_selection_detail
{
namespace
{
bool same_singleton_point(const AnalyticFilteredPointNm& left,
                          const AnalyticFilteredPointNm& right) noexcept
{
    return left.x.lower == left.x.upper && left.y.lower == left.y.upper &&
           right.x.lower == right.x.upper && right.y.lower == right.y.upper &&
           left.x.lower == right.x.lower && left.y.lower == right.y.lower;
}

std::uint64_t shared_exact_endpoints(const AnalyticAtomicCurveNm& left,
                                     const AnalyticAtomicCurveNm& right) noexcept
{
    std::uint64_t count = 0;
    if (same_singleton_point(left.start, right.start) ||
        same_singleton_point(left.start, right.end))
        ++count;
    if (!same_singleton_point(left.start, left.end) &&
        (same_singleton_point(left.end, right.start) || same_singleton_point(left.end, right.end)))
        ++count;
    return std::min<std::uint64_t>(count, 2);
}
} // namespace

SelectionAdmission prepare_boolean_selection_admission(
    const AnalyticRequestPacketRecords& records, std::uint32_t job_index,
    const AnalyticFilteredGeometry& geometry, const std::vector<AnalyticCurvePair>& candidate_pairs,
    const AnalyticSolverLimits& limits, const SelectionAdmissionOptions& options)
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
    admission_work = checked_add(admission_work, job.stage_count, valid);
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
                ((options.reserve_lineage || options.reserve_outcomes) &&
                 !valid_occurrence_source_for_curve(occurrence.source,
                                                    geometry.curves[index].kind)) ||
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
        preflight.error = AnalyticFilteredBooleanSelectionError::invalid_argument;
        return admission;
    }
    admission_work =
        checked_add(admission_work, checked_multiply(candidate_pairs.size(), 6, valid), valid);
    if (!valid || admission_work > limits.predicate_calls)
    {
        preflight.error = AnalyticFilteredBooleanSelectionError::resource_limit_exceeded;
        return admission;
    }
    preflight.telemetry.admission_work_units = admission_work;
    preflight.telemetry.predicate_calls = admission_work;

    std::uint64_t possible_spans = arrangement_minimum.possible_base_spans;
    std::uint64_t possible_transitions = arrangement_minimum.possible_base_memberships;
    std::uint64_t possible_point_intersections = 0;
    AnalyticCurvePair previous_pair{};
    bool has_previous_pair = false;
    for (const AnalyticCurvePair& pair : candidate_pairs)
    {
        if (pair.first == 0 || pair.first >= pair.second || pair.second > geometry.curves.size())
        {
            preflight.error = AnalyticFilteredBooleanSelectionError::invalid_argument;
            return admission;
        }
        if (has_previous_pair &&
            (pair.first < previous_pair.first ||
             (pair.first == previous_pair.first && pair.second <= previous_pair.second)))
        {
            preflight.error = AnalyticFilteredBooleanSelectionError::invalid_argument;
            return admission;
        }
        previous_pair = pair;
        has_previous_pair = true;
        const AnalyticAtomicCurveNm& left = geometry.curves[pair.first - 1];
        const AnalyticAtomicCurveNm& right = geometry.curves[pair.second - 1];
        const std::uint64_t maximum_points = left.kind == AnalyticAtomicCurveKind::line &&
                                                     right.kind == AnalyticAtomicCurveKind::line
                                                 ? 1
                                                 : 2;
        const std::uint64_t existing =
            std::min(maximum_points, shared_exact_endpoints(left, right));
        possible_point_intersections =
            checked_add(possible_point_intersections, maximum_points, valid);
        possible_spans = checked_add(possible_spans,
                                     checked_multiply(maximum_points - existing, 2, valid), valid);
        possible_transitions = checked_add(
            possible_transitions, checked_multiply(maximum_points - existing, 2, valid), valid);
        if (left.construction_carrier_id == right.construction_carrier_id)
            possible_transitions = checked_add(possible_transitions, 4, valid);
    }
    if (!valid)
    {
        preflight.error = AnalyticFilteredBooleanSelectionError::resource_limit_exceeded;
        return admission;
    }
    std::uint64_t selection_memory =
        checked_multiply(geometry.occurrences.size(), kOccurrenceLogicalBytes, valid);
    selection_memory = checked_add(
        selection_memory,
        checked_multiply(geometry.occurrences.size(), kOperandOrdinalLogicalBytes, valid), valid);
    selection_memory = checked_add(
        selection_memory, checked_multiply(operands, kOperandMetadataLogicalBytes, valid), valid);
    selection_memory = checked_add(
        selection_memory, checked_multiply(job.stage_count, kByteLogicalBytes, valid), valid);

    const std::uint64_t guaranteed_spans = arrangement_minimum.guaranteed_spans;
    const std::uint64_t spans = possible_spans;
    const std::uint64_t collapsed_vertex_reservation =
        arrangement_minimum.guaranteed_collapsed_vertices;
    const std::uint64_t guaranteed_vertex_reservation = checked_add(
        checked_multiply(guaranteed_spans, 2, valid), collapsed_vertex_reservation, valid);
    const std::uint64_t vertex_reservation =
        checked_add(checked_multiply(spans, 2, valid), collapsed_vertex_reservation, valid);
    const std::uint64_t half_edge_reservation = checked_multiply(spans, 2, valid);
    const std::uint64_t cycle_reservation = half_edge_reservation;
    const std::uint64_t face_reservation = checked_add(cycle_reservation, 1, valid);

    std::uint64_t retained_arrangement =
        checked_multiply(vertex_reservation, kAnalyticArrangementVertexLogicalBytes, valid);
    retained_arrangement =
        checked_add(retained_arrangement,
                    checked_multiply(spans, kAnalyticArrangementEdgeLogicalBytes, valid), valid);
    retained_arrangement = checked_add(
        retained_arrangement,
        checked_multiply(half_edge_reservation,
                         kAnalyticArrangementHalfEdgeLogicalBytes + kIndexLogicalBytes, valid),
        valid);
    retained_arrangement =
        checked_add(retained_arrangement,
                    checked_multiply(collapsed_vertex_reservation,
                                     kAnalyticArrangementCollapsedSpanLogicalBytes, valid),
                    valid);
    retained_arrangement = checked_add(
        retained_arrangement,
        checked_multiply(possible_transitions, kAnalyticOverlayMembershipLogicalBytes, valid),
        valid);
    retained_arrangement = checked_add(
        retained_arrangement,
        checked_multiply(cycle_reservation,
                         kAnalyticArrangementCycleLogicalBytes + kIndexLogicalBytes, valid),
        valid);

    std::uint64_t retained_selection_outputs =
        checked_multiply(half_edge_reservation, kIndexLogicalBytes, valid);
    retained_selection_outputs =
        checked_add(retained_selection_outputs,
                    checked_multiply(face_reservation, kFaceLogicalBytes, valid), valid);
    retained_selection_outputs =
        checked_add(retained_selection_outputs,
                    checked_multiply(cycle_reservation, kIndexLogicalBytes, valid), valid);

    std::uint64_t topology_scratch = checked_multiply(
        vertex_reservation,
        kIndexLogicalBytes + kColumnLogicalBytes + kReferenceRangeLogicalBytes * 2, valid);
    topology_scratch = checked_add(
        topology_scratch,
        checked_multiply(spans,
                         kSweepEdgeLogicalBytes + kSweepNodeLogicalBytes + kIndexLogicalBytes +
                             kEventReferenceLogicalBytes * 2 + kSweepTemporaryLogicalBytes,
                         valid),
        valid);
    topology_scratch =
        checked_add(topology_scratch,
                    checked_multiply(face_reservation,
                                     kDisjointSetLogicalBytes + kIndexLogicalBytes * 3, valid),
                    valid);
    topology_scratch = checked_add(
        topology_scratch,
        checked_multiply(cycle_reservation, kAdjacencyLogicalBytes + kIndexLogicalBytes, valid),
        valid);
    topology_scratch =
        checked_add(topology_scratch,
                    checked_multiply(half_edge_reservation, kIndexLogicalBytes, valid), valid);
    topology_scratch = checked_add(
        topology_scratch, checked_multiply(face_reservation, kFaceLogicalBytes, valid), valid);

    const std::uint64_t topology_phase_memory = checked_add(
        checked_add(retained_arrangement, selection_memory, valid), topology_scratch, valid);

    const std::uint64_t maximum_coverage_nodes =
        coverage_maximum_nodes(possible_transitions, operands, valid);
    const std::uint64_t coverage_table = coverage_table_capacity(maximum_coverage_nodes, valid);
    std::uint64_t stage_leaf_capacity = 1;
    while (stage_leaf_capacity < std::max<std::uint64_t>(1, job.stage_count))
        stage_leaf_capacity *= 2;
    const std::uint64_t span_adjacencies = checked_multiply(spans, 2, valid);
    const std::uint64_t transition_ranges = checked_add(spans, 1, valid);
    const std::uint64_t adjacency_ranges = checked_add(face_reservation, 1, valid);
    const std::uint64_t stage_tree_nodes = checked_multiply(stage_leaf_capacity, 2, valid);
    std::uint64_t coverage_scratch =
        checked_multiply(possible_transitions, kTransitionLogicalBytes, valid);
    coverage_scratch = checked_add(
        coverage_scratch, checked_multiply(transition_ranges, kIndexLogicalBytes, valid), valid);
    coverage_scratch = checked_add(
        coverage_scratch,
        checked_multiply(span_adjacencies, kIndexLogicalBytes + kAdjacencyLogicalBytes, valid),
        valid);
    coverage_scratch = checked_add(
        coverage_scratch, checked_multiply(adjacency_ranges, kIndexLogicalBytes, valid), valid);
    coverage_scratch = checked_add(
        coverage_scratch,
        checked_multiply(face_reservation, kByteLogicalBytes + kDualFrameLogicalBytes, valid),
        valid);
    coverage_scratch = checked_add(
        coverage_scratch, checked_multiply(operands, kByteLogicalBytes + kIndexLogicalBytes, valid),
        valid);
    coverage_scratch = checked_add(
        coverage_scratch, checked_multiply(job.stage_count, kIndexLogicalBytes, valid), valid);
    coverage_scratch = checked_add(
        coverage_scratch,
        checked_multiply(checked_multiply(stage_tree_nodes, 2, valid), kIndexLogicalBytes, valid),
        valid);
    coverage_scratch = checked_add(
        coverage_scratch,
        checked_multiply(maximum_coverage_nodes, kCoverageNodeLogicalBytes, valid), valid);
    coverage_scratch = checked_add(
        coverage_scratch, checked_multiply(coverage_table, kCoverageTableEntryLogicalBytes, valid),
        valid);
    if (options.reserve_outcomes)
        coverage_scratch =
            checked_add(coverage_scratch,
                        outcome_tracker_logical_bytes(operands, job.stage_count, valid), valid);
    const std::uint64_t coverage_phase_memory =
        checked_add(checked_add(checked_add(retained_arrangement, selection_memory, valid),
                                retained_selection_outputs, valid),
                    coverage_scratch, valid);
    std::uint64_t integrated_minimum_memory =
        std::max(arrangement_minimum.working_memory_bytes,
                 std::max(topology_phase_memory, coverage_phase_memory));
    std::uint64_t possible_arrangement_memory = 0;
    const analytic_detail::AnalyticFilteredArrangementCapacityEnvelope arrangement_envelope{
        geometry.curves.size(),
        candidate_pairs.size(),
        possible_point_intersections,
        arrangement_minimum.possible_circular_carrier_groups,
        possible_spans,
        arrangement_minimum.possible_collapsed_domains,
        possible_transitions,
    };
    if (!analytic_detail::estimate_analytic_filtered_arrangement_possible_memory(
            arrangement_envelope, possible_arrangement_memory))
        valid = false;
    integrated_minimum_memory = std::max(integrated_minimum_memory, possible_arrangement_memory);

    std::uint64_t material_regions_work = 0;
    std::uint64_t lineage_work = 0;
    std::uint64_t outcomes_work = 0;
    if (options.reserve_material_regions || options.reserve_lineage || options.reserve_outcomes)
    {
        const std::uint64_t maximum_edges = spans;
        const std::uint64_t maximum_half_edges = checked_multiply(maximum_edges, 2, valid);
        const std::uint64_t maximum_faces = checked_add(maximum_half_edges, 1, valid);
        const std::uint64_t maximum_rings = maximum_half_edges;
        const std::uint64_t maximum_regions = maximum_faces;
        const std::uint64_t maximum_adjacency = checked_multiply(maximum_rings, 2, valid);

        std::uint64_t retained_selection =
            checked_add(checked_add(retained_arrangement, selection_memory, valid),
                        retained_selection_outputs, valid);
        retained_selection = checked_add(
            retained_selection,
            checked_multiply(maximum_coverage_nodes, kCoverageNodeLogicalBytes, valid), valid);
        if (options.reserve_outcomes)
            retained_selection =
                checked_add(retained_selection,
                            checked_multiply(operands, kOutcomeEvidenceLogicalBytes, valid), valid);

        std::uint64_t region_scratch =
            checked_multiply(maximum_edges, kIndexLogicalBytes * 2, valid);
        region_scratch =
            checked_add(region_scratch,
                        checked_multiply(maximum_half_edges,
                                         kByteLogicalBytes * 4 + kIndexLogicalBytes * 2, valid),
                        valid);
        region_scratch = checked_add(
            region_scratch,
            checked_multiply(maximum_faces,
                             kDisjointSetLogicalBytes + kIndexLogicalBytes * 4 + kByteLogicalBytes,
                             valid),
            valid);
        region_scratch =
            checked_add(region_scratch,
                        checked_multiply(maximum_rings,
                                         kMaterialRawRingLogicalBytes + kMaterialRingLogicalBytes +
                                             kIndexLogicalBytes * 3 + kByteLogicalBytes,
                                         valid),
                        valid);
        region_scratch = checked_add(
            region_scratch,
            checked_multiply(maximum_adjacency, kMaterialAdjacencyLogicalBytes, valid), valid);
        region_scratch = checked_add(
            region_scratch, checked_multiply(maximum_faces + 1, kIndexLogicalBytes, valid), valid);
        region_scratch =
            checked_add(region_scratch,
                        checked_multiply(maximum_faces,
                                         kMaterialRegionLogicalBytes + kIndexLogicalBytes, valid),
                        valid);
        region_scratch = checked_add(
            region_scratch, checked_multiply(maximum_half_edges, kIndexLogicalBytes, valid), valid);
        const std::uint64_t region_phase_memory =
            checked_add(retained_selection, region_scratch, valid);
        integrated_minimum_memory = std::max(integrated_minimum_memory, region_phase_memory);

        material_regions_work = checked_multiply(maximum_half_edges, 16, valid);
        material_regions_work =
            checked_add(material_regions_work, checked_multiply(maximum_edges, 4, valid), valid);
        material_regions_work =
            checked_add(material_regions_work, checked_multiply(maximum_faces, 12, valid), valid);
        material_regions_work =
            checked_add(material_regions_work, checked_multiply(maximum_rings, 12, valid), valid);
        material_regions_work =
            checked_add(material_regions_work,
                        checked_multiply(checked_add(maximum_edges, maximum_faces, valid),
                                         tree_operation_units(maximum_faces), valid),
                        valid);
        material_regions_work =
            checked_add(material_regions_work, sort_units(maximum_adjacency), valid);
        material_regions_work =
            checked_add(material_regions_work, sort_units(maximum_rings), valid);
        material_regions_work =
            checked_add(material_regions_work, sort_units(maximum_faces), valid);

        if (options.reserve_lineage || options.reserve_outcomes)
        {
            const std::uint64_t maximum_vertices = vertex_reservation;
            std::uint64_t operand_leaf_capacity = 1;
            while (operand_leaf_capacity < std::max<std::uint64_t>(1, operands))
                operand_leaf_capacity *= 2;
            const std::uint64_t reporter_nodes = checked_multiply(operand_leaf_capacity, 2, valid);
            std::uint64_t retained_regions = retained_selection;
            retained_regions =
                checked_add(retained_regions,
                            checked_multiply(maximum_rings,
                                             kMaterialRingLogicalBytes + kIndexLogicalBytes, valid),
                            valid);
            retained_regions = checked_add(
                retained_regions,
                checked_multiply(maximum_regions, kMaterialRegionLogicalBytes, valid), valid);
            retained_regions =
                checked_add(retained_regions,
                            checked_multiply(maximum_faces, kIndexLogicalBytes, valid), valid);

            std::uint64_t lineage_scratch = checked_multiply(
                operands,
                kOperandMetadataLogicalBytes + kOperandLookupLogicalBytes + kIndexLogicalBytes * 3,
                valid);
            lineage_scratch = checked_add(
                lineage_scratch,
                checked_multiply(geometry.occurrences.size(), kIndexLogicalBytes * 2, valid),
                valid);
            lineage_scratch = checked_add(
                lineage_scratch,
                checked_multiply(possible_transitions, kTransitionLogicalBytes, valid), valid);
            lineage_scratch = checked_add(
                lineage_scratch,
                checked_multiply(checked_add(maximum_edges, 1, valid), kIndexLogicalBytes, valid),
                valid);
            lineage_scratch = checked_add(
                lineage_scratch,
                checked_multiply(checked_add(job.stage_count, 1, valid), kIndexLogicalBytes, valid),
                valid);
            lineage_scratch =
                checked_add(lineage_scratch,
                            checked_multiply(
                                maximum_edges,
                                kIndexLogicalBytes * 2 + kMaterialAdjacencyLogicalBytes * 2, valid),
                            valid);
            lineage_scratch = checked_add(lineage_scratch,
                                          checked_multiply(maximum_faces,
                                                           kIndexLogicalBytes * 3 +
                                                               kLineageTraversalFrameLogicalBytes +
                                                               kByteLogicalBytes * 2,
                                                           valid),
                                          valid);
            lineage_scratch = checked_add(
                lineage_scratch,
                checked_multiply(maximum_half_edges,
                                 kReferenceRangeLogicalBytes * 2 + kIndexLogicalBytes, valid),
                valid);
            lineage_scratch =
                checked_add(lineage_scratch,
                            checked_multiply(maximum_vertices,
                                             kReferenceRangeLogicalBytes + kIndexLogicalBytes * 2 +
                                                 kByteLogicalBytes,
                                             valid),
                            valid);
            lineage_scratch = checked_add(
                lineage_scratch,
                checked_multiply(maximum_regions, kReferenceRangeLogicalBytes + kIndexLogicalBytes,
                                 valid),
                valid);
            lineage_scratch = checked_add(
                lineage_scratch, checked_multiply(operands, kIndexLogicalBytes, valid), valid);
            lineage_scratch =
                checked_add(lineage_scratch,
                            checked_multiply(reporter_nodes, kIndexLogicalBytes * 2, valid), valid);
            integrated_minimum_memory = std::max(
                integrated_minimum_memory, checked_add(retained_regions, lineage_scratch, valid));

            lineage_work = checked_add(checked_multiply(job.stage_count, 2, valid),
                                       checked_multiply(operands, 5, valid), valid);
            lineage_work = checked_add(lineage_work, sort_units(operands), valid);
            lineage_work = checked_add(lineage_work,
                                       checked_multiply(geometry.occurrences.size(),
                                                        tree_operation_units(operands), valid),
                                       valid);
            lineage_work = checked_add(lineage_work, geometry.occurrences.size(), valid);
            lineage_work = checked_add(lineage_work, maximum_coverage_nodes, valid);
            lineage_work = checked_add(
                lineage_work,
                checked_add(checked_add(checked_multiply(possible_transitions, 3, valid),
                                        sort_units(possible_transitions), valid),
                            maximum_edges, valid),
                valid);
            lineage_work = checked_add(
                lineage_work,
                checked_add(checked_multiply(reporter_nodes, 2, valid), operands, valid), valid);
            std::uint64_t contributor_pass =
                checked_add(maximum_half_edges, checked_multiply(maximum_edges, 4, valid), valid);
            contributor_pass =
                checked_add(contributor_pass, checked_multiply(maximum_faces, 3, valid), valid);
            contributor_pass = checked_add(contributor_pass, maximum_regions, valid);
            std::uint64_t incidence_pass =
                checked_add(checked_multiply(maximum_half_edges, 2, valid),
                            checked_multiply(maximum_vertices, 2, valid), valid);
            incidence_pass =
                checked_add(incidence_pass, checked_multiply(maximum_edges, 3, valid), valid);
            incidence_pass =
                checked_add(incidence_pass, arrangement_minimum.possible_collapsed_domains, valid);
            incidence_pass = checked_add(incidence_pass, maximum_regions, valid);
            lineage_work = checked_add(
                lineage_work,
                checked_multiply(checked_add(contributor_pass, incidence_pass, valid), 2, valid),
                valid);
            lineage_work =
                checked_add(lineage_work, checked_multiply(possible_transitions, 4, valid), valid);
            lineage_work =
                checked_add(lineage_work,
                            checked_multiply(checked_add(maximum_faces, maximum_half_edges, valid),
                                             tree_operation_units(operands), valid),
                            valid);
            lineage_work = checked_add(
                lineage_work,
                checked_multiply(checked_multiply(possible_transitions, 2, valid),
                                 checked_add(tree_operation_units(operands), 1, valid), valid),
                valid);

            if (options.reserve_outcomes)
            {
                // The structural history tracker runs inside selection. Its
                // target-independent O((T+F) log S + O log S) work is added to
                // selection below. This downstream reservation covers the
                // allocation-free lineage/reference count and canonical fill
                // pass. Source copying is bounded from the admitted occurrence
                // table here; output-sensitive lineage incidences are charged
                // in one bulk reservation before their exact count traversal.
                const std::uint64_t operand_tree = tree_operation_units(operands);
                outcomes_work = checked_multiply(operands, 16, valid);
                outcomes_work =
                    checked_add(outcomes_work,
                                checked_multiply(geometry.occurrences.size(),
                                                 checked_add(operand_tree, 4, valid), valid),
                                valid);
                outcomes_work =
                    checked_add(outcomes_work, checked_multiply(maximum_regions, 4, valid), valid);
                outcomes_work = checked_add(outcomes_work,
                                            checked_multiply(maximum_half_edges, 4, valid), valid);
                outcomes_work = checked_add(
                    outcomes_work, checked_multiply(possible_transitions, 2, valid), valid);
                outcomes_work =
                    checked_add(outcomes_work, sort_units(geometry.occurrences.size()), valid);
            }
        }
    }

    std::uint64_t selection_work = sort_units(operands);
    selection_work = checked_add(selection_work,
                                 checked_add(checked_multiply(job.stage_count, 2, valid),
                                             checked_multiply(operands, 2, valid), valid),
                                 valid);
    selection_work = checked_add(
        selection_work,
        checked_multiply(geometry.occurrences.size(), tree_operation_units(operands), valid),
        valid);
    selection_work = checked_add(selection_work, geometry.curves.size(), valid);
    selection_work =
        checked_add(selection_work, checked_multiply(guaranteed_spans, 11, valid), valid);
    selection_work = checked_add(selection_work,
                                 checked_multiply(guaranteed_vertex_reservation, 3, valid), valid);
    selection_work =
        checked_add(selection_work,
                    checked_multiply(sort_units(guaranteed_vertex_reservation), 2, valid), valid);
    std::uint64_t stage_initialization_work = checked_add(7, job.stage_count, valid);
    stage_initialization_work =
        checked_add(stage_initialization_work, checked_multiply(stage_tree_nodes, 2, valid), valid);
    stage_initialization_work = checked_add(stage_initialization_work, operands, valid);
    selection_work = checked_add(selection_work, stage_initialization_work, valid);
    selection_work = checked_add(
        selection_work,
        checked_multiply(
            face_reservation,
            checked_add(checked_multiply(coverage_operand_depth(job.stage_count), 2, valid), 1,
                        valid),
            valid),
        valid);
    selection_work = checked_add(selection_work, coverage_table, valid);
    selection_work = checked_add(selection_work, possible_transitions, valid);
    if (options.reserve_outcomes)
        selection_work =
            checked_add(selection_work,
                        outcome_tracker_work_upper_bound(possible_transitions, face_reservation,
                                                         operands, job.stage_count, valid),
                        valid);
    std::uint64_t integrated_arrangement_work =
        checked_add(geometry.curves.size(), arrangement_minimum.predicate_calls, valid);
    integrated_arrangement_work = checked_add(integrated_arrangement_work, selection_work, valid);
    integrated_arrangement_work =
        checked_add(integrated_arrangement_work, material_regions_work, valid);
    integrated_arrangement_work = checked_add(integrated_arrangement_work, lineage_work, valid);
    integrated_arrangement_work = checked_add(integrated_arrangement_work, outcomes_work, valid);
    const std::uint64_t remaining_work = limits.predicate_calls - admission_work;
    if (!valid || integrated_minimum_memory > limits.working_memory_bytes ||
        integrated_arrangement_work > remaining_work)
    {
        preflight.error = AnalyticFilteredBooleanSelectionError::resource_limit_exceeded;
        return admission;
    }
    AnalyticSolverLimits arrangement_limits = limits;
    arrangement_limits.predicate_calls =
        remaining_work - selection_work - material_regions_work - lineage_work - outcomes_work;
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
    admission.material_regions_reserved_work = material_regions_work;
    admission.lineage_reserved_work = lineage_work;
    admission.outcomes_reserved_work = outcomes_work;
    admission.downstream_reserved_work = material_regions_work + lineage_work + outcomes_work;
    admission.collect_outcomes = options.reserve_outcomes;
    admission.execution_limits = limits;
    admission.execution_limits.predicate_calls -= admission.downstream_reserved_work;
    admission.ready = true;
    return admission;
}

} // namespace analytic_selection_detail
} // namespace geometer

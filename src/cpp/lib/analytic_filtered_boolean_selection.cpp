#include "geometer/analytic_filtered_boolean_selection.h"

#include "analytic_filtered_boolean_selection_support.h"
#include "analytic_filtered_outcome_tracker.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

namespace geometer
{
namespace
{
using namespace analytic_selection_detail;
class SelectionBuilder
{
  public:
    SelectionBuilder(const AnalyticRequestPacketRecords& records, std::uint32_t job_index,
                     const AnalyticFilteredGeometry& geometry,
                     AnalyticFilteredArrangementResult arrangement, AnalyticSolverLimits limits,
                     std::uint64_t admission_work, std::uint64_t admission_peak_memory,
                     bool collect_outcomes)
        : records_(records), job_index_(job_index), geometry_(geometry), limits_(limits),
          collect_outcomes_(collect_outcomes)
    {
        result_.origin_x_nm = geometry_.origin_x_nm;
        result_.origin_y_nm = geometry_.origin_y_nm;
        result_.arrangement = std::move(arrangement);
        result_.telemetry.admission_work_units = admission_work;
        result_.telemetry.arrangement_predicate_calls =
            result_.arrangement.telemetry.predicate_calls;
        result_.telemetry.arrangement_peak_working_memory_bytes =
            result_.arrangement.telemetry.peak_working_memory_bytes;
        result_.telemetry.predicate_calls =
            admission_work + result_.arrangement.telemetry.predicate_calls;
        result_.telemetry.peak_working_memory_bytes = std::max(
            admission_peak_memory, result_.arrangement.telemetry.peak_working_memory_bytes);
        result_.telemetry.algebraic_fallback_calls =
            result_.arrangement.telemetry.algebraic_fallback_calls;
    }

    AnalyticFilteredBooleanSelectionResult build()
    {
        try
        {
            if (!preflight_input_memory() || !prepare_operands() || !preflight_memory() ||
                !build_face_topology())
            {
                if (result_.error == AnalyticFilteredBooleanSelectionError::none)
                    fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
                clear_output();
                return std::move(result_);
            }
            release_sweep_storage();
            if (!set_phase_memory(0) || !build_transitions() || !propagate_coverages())
            {
                if (result_.error == AnalyticFilteredBooleanSelectionError::none)
                    fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
                clear_output();
                return std::move(result_);
            }
            release_transition_storage();
            if (!set_phase_memory(0))
            {
                if (result_.error == AnalyticFilteredBooleanSelectionError::none)
                    fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
                clear_output();
                return std::move(result_);
            }
        }
        catch (const std::bad_alloc&)
        {
            result_.telemetry.required_working_memory_bytes = limits_.working_memory_bytes + 1;
            fail(AnalyticFilteredBooleanSelectionError::resource_limit_exceeded);
            clear_output();
        }
        return std::move(result_);
    }

  private:
    bool fail(AnalyticFilteredBooleanSelectionError error) noexcept
    {
        result_.error = error;
        return false;
    }

    void clear_output()
    {
        result_.arrangement.vertices.clear();
        result_.arrangement.edges.clear();
        result_.arrangement.half_edges.clear();
        result_.arrangement.outgoing_half_edges.clear();
        result_.arrangement.collapsed_spans.clear();
        result_.arrangement.memberships.clear();
        result_.arrangement.cycles.clear();
        result_.arrangement.cycle_half_edges.clear();
        result_.occurrences.clear();
        result_.half_edge_faces.clear();
        result_.faces.clear();
        result_.face_boundary_cycles.clear();
        result_.coverage_state_nodes.clear();
        result_.outcome_evidence.clear();
    }

    void release_sweep_storage()
    {
        std::vector<SweepEdge>().swap(sweep_edges_);
        std::vector<std::uint32_t>().swap(vertex_order_);
        std::vector<EventColumn>().swap(columns_);
        std::vector<EventReference>().swap(starts_);
        std::vector<EventReference>().swap(ends_);
        std::vector<std::pair<std::uint32_t, std::uint32_t>>().swap(start_ranges_);
        std::vector<std::pair<std::uint32_t, std::uint32_t>>().swap(end_ranges_);
    }

    void release_transition_storage()
    {
        std::vector<Transition>().swap(transitions_);
        std::vector<std::uint32_t>().swap(transition_begin_);
    }

    bool charge(std::uint64_t units)
    {
        if (result_.telemetry.predicate_calls > limits_.predicate_calls ||
            units > limits_.predicate_calls - result_.telemetry.predicate_calls)
            return fail(AnalyticFilteredBooleanSelectionError::resource_limit_exceeded);
        result_.telemetry.predicate_calls += units;
        return true;
    }

    bool charge_sort(std::uint64_t count)
    {
        const std::uint64_t units = sort_units(count);
        if (!charge(units))
            return false;
        result_.telemetry.sort_work_units += units;
        return true;
    }

    bool set_phase_memory(std::uint64_t scratch)
    {
        bool valid = true;
        const auto& arrangement = result_.arrangement;
        std::uint64_t retained = checked_multiply(arrangement.vertices.size(),
                                                  kAnalyticArrangementVertexLogicalBytes, valid);
        retained = checked_add(
            retained,
            checked_multiply(arrangement.edges.size(), kAnalyticArrangementEdgeLogicalBytes, valid),
            valid);
        retained = checked_add(retained,
                               checked_multiply(arrangement.half_edges.size(),
                                                kAnalyticArrangementHalfEdgeLogicalBytes, valid),
                               valid);
        retained = checked_add(
            retained,
            checked_multiply(arrangement.outgoing_half_edges.size(), kIndexLogicalBytes, valid),
            valid);
        retained =
            checked_add(retained,
                        checked_multiply(arrangement.collapsed_spans.size(),
                                         kAnalyticArrangementCollapsedSpanLogicalBytes, valid),
                        valid);
        retained = checked_add(retained,
                               checked_multiply(arrangement.memberships.size(),
                                                kAnalyticOverlayMembershipLogicalBytes, valid),
                               valid);
        retained = checked_add(retained,
                               checked_multiply(arrangement.cycles.size(),
                                                kAnalyticArrangementCycleLogicalBytes, valid),
                               valid);
        retained = checked_add(
            retained,
            checked_multiply(arrangement.cycle_half_edges.size(), kIndexLogicalBytes, valid),
            valid);
        retained = checked_add(
            retained, checked_multiply(result_.occurrences.size(), kOccurrenceLogicalBytes, valid),
            valid);
        retained = checked_add(
            retained, checked_multiply(operands_.size(), kOperandMetadataLogicalBytes, valid),
            valid);
        retained = checked_add(
            retained,
            checked_multiply(occurrence_operands_.size(), kOperandOrdinalLogicalBytes, valid),
            valid);
        retained = checked_add(
            retained, checked_multiply(stage_operations_.size(), kByteLogicalBytes, valid), valid);
        retained = checked_add(
            retained, checked_multiply(result_.half_edge_faces.size(), kIndexLogicalBytes, valid),
            valid);
        retained = checked_add(
            retained, checked_multiply(result_.faces.size(), kFaceLogicalBytes, valid), valid);
        retained = checked_add(
            retained,
            checked_multiply(result_.face_boundary_cycles.size(), kIndexLogicalBytes, valid),
            valid);
        retained = checked_add(
            retained,
            checked_multiply(result_.coverage_state_nodes.size(), kCoverageNodeLogicalBytes, valid),
            valid);
        retained = checked_add(
            retained,
            checked_multiply(result_.outcome_evidence.size(), kOutcomeEvidenceLogicalBytes, valid),
            valid);
        const std::uint64_t phase = checked_add(retained, scratch, valid);
        if (!valid || phase > limits_.working_memory_bytes)
        {
            if (valid)
                result_.telemetry.required_working_memory_bytes = phase;
            return fail(AnalyticFilteredBooleanSelectionError::resource_limit_exceeded);
        }
        result_.telemetry.peak_working_memory_bytes =
            std::max(result_.telemetry.peak_working_memory_bytes, phase);
        return true;
    }

    bool preflight_input_memory()
    {
        const AnalyticRequestJobRecord& job = records_.jobs[job_index_];
        if (!charge(job.stage_count))
            return false;
        std::uint64_t operand_count = 0;
        bool valid = true;
        for (std::uint32_t local = 0; local < job.stage_count; ++local)
            operand_count = checked_add(
                operand_count, records_.stages[job.stage_begin + local].operand_count, valid);
        expected_operand_count_ = operand_count;
        std::uint64_t scratch =
            checked_multiply(geometry_.occurrences.size(), kOccurrenceLogicalBytes, valid);
        scratch = checked_add(
            scratch, checked_multiply(operand_count, kOperandMetadataLogicalBytes, valid), valid);
        scratch = checked_add(
            scratch, checked_multiply(operand_count, kOperandLookupLogicalBytes, valid), valid);
        scratch = checked_add(
            scratch,
            checked_multiply(geometry_.occurrences.size(), kOperandOrdinalLogicalBytes, valid),
            valid);
        scratch =
            checked_add(scratch, checked_multiply(operand_count, kByteLogicalBytes, valid), valid);
        scratch = checked_add(scratch, checked_multiply(job.stage_count, kByteLogicalBytes, valid),
                              valid);
        if (!valid)
            return fail(AnalyticFilteredBooleanSelectionError::resource_limit_exceeded);
        return set_phase_memory(scratch);
    }

    bool prepare_operands()
    {
        if (job_index_ >= records_.jobs.size())
            return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
        const AnalyticRequestJobRecord& job = records_.jobs[job_index_];
        if (job.stage_begin > records_.stages.size() ||
            job.stage_count > records_.stages.size() - job.stage_begin)
            return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
        result_.telemetry.input_stages = job.stage_count;
        result_.telemetry.input_cycles = result_.arrangement.cycles.size();
        stage_operations_.reserve(job.stage_count);
        if (!charge(job.stage_count + expected_operand_count_))
            return false;
        operands_.reserve(static_cast<std::size_t>(expected_operand_count_));
        occurrence_operands_.resize(geometry_.occurrences.size());
        std::uint64_t operand_count = 0;
        for (std::uint32_t local = 0; local < job.stage_count; ++local)
        {
            const AnalyticRequestStageRecord& stage = records_.stages[job.stage_begin + local];
            if (stage.stage_id == 0 || (stage.operation != 1 && stage.operation != 2) ||
                stage.operand_begin > records_.operands.size() ||
                stage.operand_count > records_.operands.size() - stage.operand_begin)
                return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
            if (stage.operand_count > limits_.boundary_occurrences - operand_count)
                return fail(AnalyticFilteredBooleanSelectionError::resource_limit_exceeded);
            stage_operations_.push_back(stage.operation);
            operand_count += stage.operand_count;
            for (std::uint32_t offset = 0; offset < stage.operand_count; ++offset)
            {
                const AnalyticRequestOperandRecord& operand =
                    records_.operands[stage.operand_begin + offset];
                if (operand.operand_id == 0 ||
                    (offset != 0 &&
                     records_.operands[stage.operand_begin + offset - 1].operand_id >=
                         operand.operand_id))
                    return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
                operands_.push_back({operand.operand_id, stage.stage_id, local, stage.operation});
            }
        }
        result_.telemetry.input_operands = operand_count;
        bool valid = true;
        std::uint64_t validation_work = operands_.size();
        validation_work =
            checked_add(validation_work,
                        checked_multiply(geometry_.occurrences.size(),
                                         tree_operation_units(operands_.size()), valid),
                        valid);
        if (!valid)
            return fail(AnalyticFilteredBooleanSelectionError::resource_limit_exceeded);
        if (!charge(validation_work))
            return false;
        std::vector<OperandLookup> operand_lookup;
        operand_lookup.reserve(operands_.size());
        for (std::uint32_t index = 0; index < operands_.size(); ++index)
            operand_lookup.push_back({operands_[index].operand_id, index});
        if (!charge_sort(operand_lookup.size()))
            return false;
        std::sort(operand_lookup.begin(), operand_lookup.end(),
                  [](const OperandLookup& left, const OperandLookup& right)
                  { return left.operand_id < right.operand_id; });
        for (std::size_t index = 1; index < operand_lookup.size(); ++index)
            if (operand_lookup[index - 1].operand_id == operand_lookup[index].operand_id)
                return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);

        std::vector<std::uint8_t> used(operands_.size());
        for (std::uint32_t index = 0; index < geometry_.occurrences.size(); ++index)
        {
            const AnalyticFilteredOccurrence& occurrence = geometry_.occurrences[index];
            if (occurrence.occurrence_id != static_cast<std::uint64_t>(index) + 1 ||
                occurrence.coverage_id == 0 ||
                occurrence.source.operand_id != occurrence.coverage_id)
                return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
            const auto found = std::lower_bound(
                operand_lookup.begin(), operand_lookup.end(), occurrence.coverage_id,
                [](const OperandLookup& value, std::uint64_t id) { return value.operand_id < id; });
            if (found == operand_lookup.end() || found->operand_id != occurrence.coverage_id)
                return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
            const std::uint32_t operand = found->ordinal;
            used[operand] = 1;
            occurrence_operands_[index] = operand;
        }
        if (std::find(used.begin(), used.end(), 0) != used.end())
            return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
        result_.occurrences = geometry_.occurrences;
        return true;
    }

    bool preflight_memory()
    {
        bool valid = true;
        const std::uint64_t vertices = result_.arrangement.vertices.size();
        const std::uint64_t edges = result_.arrangement.edges.size();
        const std::uint64_t cycles = result_.arrangement.cycles.size();
        const std::uint64_t half_edges = result_.arrangement.half_edges.size();
        if (!charge(cycles))
            return false;
        std::uint64_t faces = 1;
        for (const AnalyticArrangementCycle& cycle : result_.arrangement.cycles)
            faces += cycle.counterclockwise ? 1 : 0;
        expected_face_capacity_ = faces;
        if (faces > limits_.arrangement_faces)
            return fail(AnalyticFilteredBooleanSelectionError::resource_limit_exceeded);
        std::uint64_t topology = checked_multiply(
            vertices, kIndexLogicalBytes + kColumnLogicalBytes + kReferenceRangeLogicalBytes * 2,
            valid);
        topology = checked_add(
            topology,
            checked_multiply(edges,
                             kSweepEdgeLogicalBytes + kSweepNodeLogicalBytes + kIndexLogicalBytes +
                                 kEventReferenceLogicalBytes * 2 + kSweepTemporaryLogicalBytes,
                             valid),
            valid);
        topology = checked_add(
            topology,
            checked_multiply(cycles + 1, kDisjointSetLogicalBytes + kIndexLogicalBytes * 3, valid),
            valid);
        topology = checked_add(
            topology, checked_multiply(cycles, kAdjacencyLogicalBytes + kIndexLogicalBytes, valid),
            valid);
        topology =
            checked_add(topology, checked_multiply(half_edges, kIndexLogicalBytes, valid), valid);
        topology = checked_add(topology, checked_multiply(faces, kFaceLogicalBytes, valid), valid);
        if (!valid)
            return fail(AnalyticFilteredBooleanSelectionError::resource_limit_exceeded);
        if (!set_phase_memory(topology))
            return false;
        const std::uint64_t validation_work =
            vertices * 3 + edges * 5 + result_.arrangement.half_edges.size() * 2 + cycles * 3;
        return charge(validation_work);
    }

    std::optional<std::int8_t> compare_vertex_to_edge(std::uint32_t vertex,
                                                      std::uint32_t edge_index) const noexcept
    {
        const AnalyticFilteredPointNm& query = result_.arrangement.vertices[vertex].point;
        const AnalyticArrangementEdgeNm& edge = result_.arrangement.edges[edge_index];
        if (edge.kind == AnalyticAtomicCurveKind::line)
        {
            Point start = point(edge.carrier_start);
            Point end = point(edge.carrier_end);
            if (sweep_edges_[edge_index].left_vertex == edge.end_vertex)
                std::swap(start, end);
            const Interval side = cross(subtract(end, start), subtract(point(query), start));
            if (side.lower > 0.0)
                return 1;
            if (side.upper < 0.0)
                return -1;
            return std::nullopt;
        }

        const Point radial = subtract(point(query), point(edge.circle.center));
        const Interval residual =
            subtract(add(square(radial.x), square(radial.y)),
                     square({edge.circle.radius.lower, edge.circle.radius.upper}));
        const Interval vertical = radial.y;
        if (edge.x_monotone_branch == AnalyticXMonotoneBranch::upper)
        {
            if (vertical.upper < 0.0 || residual.upper < 0.0)
                return -1;
            if (vertical.lower >= 0.0 && residual.lower > 0.0)
                return 1;
            return std::nullopt;
        }
        if (edge.x_monotone_branch == AnalyticXMonotoneBranch::lower)
        {
            if (vertical.lower > 0.0 || residual.upper < 0.0)
                return 1;
            if (vertical.upper <= 0.0 && residual.lower > 0.0)
                return -1;
            return std::nullopt;
        }
        return std::nullopt;
    }

    bool classify_sweep_edges()
    {
        const auto& arrangement = result_.arrangement;
        sweep_edges_.resize(arrangement.edges.size());
        std::vector<std::uint32_t> forward(arrangement.edges.size(), kNoIndex);
        std::vector<std::uint32_t> reverse(arrangement.edges.size(), kNoIndex);
        for (std::uint32_t half_edge = 0; half_edge < arrangement.half_edges.size(); ++half_edge)
        {
            const AnalyticArrangementHalfEdge& value = arrangement.half_edges[half_edge];
            if (value.edge >= arrangement.edges.size())
                return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
            std::uint32_t& slot = value.forward ? forward[value.edge] : reverse[value.edge];
            if (slot != kNoIndex)
                return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
            slot = half_edge;
        }
        for (std::uint32_t edge_index = 0; edge_index < arrangement.edges.size(); ++edge_index)
        {
            const AnalyticArrangementEdgeNm& edge = arrangement.edges[edge_index];
            if (edge.start_vertex >= arrangement.vertices.size() ||
                edge.end_vertex >= arrangement.vertices.size() ||
                edge.start_vertex == edge.end_vertex || forward[edge_index] == kNoIndex ||
                reverse[edge_index] == kNoIndex)
                return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
            SweepEdge& sweep = sweep_edges_[edge_index];
            if (edge.kind == AnalyticAtomicCurveKind::line)
            {
                if (edge.has_construction_line_direction)
                {
                    if (edge.construction_line_dx < 0 ||
                        (edge.construction_line_dx == 0 && edge.construction_line_dy <= 0))
                        return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
                    sweep.vertical = edge.construction_line_dx == 0;
                    sweep.left_vertex = edge.start_vertex;
                    sweep.right_vertex = edge.end_vertex;
                }
                else
                {
                    const auto& start = arrangement.vertices[edge.start_vertex].point.x;
                    const auto& end = arrangement.vertices[edge.end_vertex].point.x;
                    if (start.upper < end.lower)
                    {
                        sweep.left_vertex = edge.start_vertex;
                        sweep.right_vertex = edge.end_vertex;
                    }
                    else if (end.upper < start.lower)
                    {
                        sweep.left_vertex = edge.end_vertex;
                        sweep.right_vertex = edge.start_vertex;
                    }
                    else if (start.lower == start.upper && end.lower == end.upper &&
                             start.lower == end.lower)
                    {
                        sweep.vertical = true;
                        sweep.left_vertex = edge.start_vertex;
                        sweep.right_vertex = edge.end_vertex;
                    }
                    else
                        return fail(AnalyticFilteredBooleanSelectionError::resource_limit_exceeded);
                }
            }
            else if (edge.kind == AnalyticAtomicCurveKind::circular_arc &&
                     edge.x_monotone_branch == AnalyticXMonotoneBranch::lower)
            {
                sweep.left_vertex = edge.start_vertex;
                sweep.right_vertex = edge.end_vertex;
            }
            else if (edge.kind == AnalyticAtomicCurveKind::circular_arc &&
                     edge.x_monotone_branch == AnalyticXMonotoneBranch::upper)
            {
                sweep.left_vertex = edge.end_vertex;
                sweep.right_vertex = edge.start_vertex;
            }
            else
                return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);

            if (sweep.vertical)
                continue;
            const std::uint32_t east_half =
                arrangement.half_edges[forward[edge_index]].origin_vertex == sweep.left_vertex
                    ? forward[edge_index]
                    : reverse[edge_index];
            if (arrangement.half_edges[east_half].origin_vertex != sweep.left_vertex ||
                arrangement.half_edges[arrangement.half_edges[east_half].twin].origin_vertex !=
                    sweep.right_vertex)
                return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
            sweep.above_cycle = arrangement.half_edges[east_half].cycle;
            sweep.below_cycle =
                arrangement.half_edges[arrangement.half_edges[east_half].twin].cycle;
            if (sweep.above_cycle >= arrangement.cycles.size() ||
                sweep.below_cycle >= arrangement.cycles.size())
                return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
        }
        return true;
    }

    bool build_event_columns()
    {
        vertex_order_.resize(result_.arrangement.vertices.size());
        columns_.reserve(result_.arrangement.vertices.size());
        for (std::uint32_t vertex = 0; vertex < vertex_order_.size(); ++vertex)
            vertex_order_[vertex] = vertex;
        if (!charge_sort(vertex_order_.size()))
            return false;
        std::sort(vertex_order_.begin(), vertex_order_.end(),
                  [&](std::uint32_t left, std::uint32_t right)
                  {
                      const auto& a = result_.arrangement.vertices[left].point;
                      const auto& b = result_.arrangement.vertices[right].point;
                      return std::tie(a.x.lower, a.x.upper, a.y.lower, a.y.upper, left) <
                             std::tie(b.x.lower, b.x.upper, b.y.lower, b.y.upper, right);
                  });
        for (std::uint32_t cursor = 0; cursor < vertex_order_.size();)
        {
            const std::uint32_t begin = cursor;
            const auto& first = result_.arrangement.vertices[vertex_order_[cursor]].point.x;
            double minimum = first.lower;
            double maximum = first.upper;
            std::uint64_t column_token =
                result_.arrangement.vertices[vertex_order_[cursor]].point.construction_x_column_id;
            bool correlated = false;
            ++cursor;
            while (cursor < vertex_order_.size())
            {
                const auto& value = result_.arrangement.vertices[vertex_order_[cursor]].point.x;
                if (maximum < value.lower)
                    break;
                const std::uint64_t value_token =
                    result_.arrangement.vertices[vertex_order_[cursor]]
                        .point.construction_x_column_id;
                const bool same_singleton =
                    minimum == maximum && value.lower == value.upper && value.lower == minimum;
                const bool same_correlated = column_token != 0 && value_token == column_token;
                if (!same_singleton && !same_correlated)
                    return fail(AnalyticFilteredBooleanSelectionError::resource_limit_exceeded);
                minimum = std::min(minimum, value.lower);
                maximum = std::max(maximum, value.upper);
                correlated = correlated || same_correlated;
                ++cursor;
            }
            if (!charge_sort(cursor - begin))
                return false;
            std::sort(vertex_order_.begin() + begin, vertex_order_.begin() + cursor,
                      [&](std::uint32_t left, std::uint32_t right)
                      {
                          const auto& a = result_.arrangement.vertices[left].point;
                          const auto& b = result_.arrangement.vertices[right].point;
                          return std::tie(a.y.lower, a.y.upper, a.x.lower, a.x.upper, left) <
                                 std::tie(b.y.lower, b.y.upper, b.x.lower, b.x.upper, right);
                      });
            for (std::uint32_t index = begin + 1; index < cursor; ++index)
            {
                const auto& previous =
                    result_.arrangement.vertices[vertex_order_[index - 1]].point.y;
                const auto& current = result_.arrangement.vertices[vertex_order_[index]].point.y;
                if (previous.upper >= current.lower)
                    return fail(AnalyticFilteredBooleanSelectionError::resource_limit_exceeded);
            }
            columns_.push_back({begin, cursor - begin, minimum, maximum, correlated});
            result_.telemetry.resolution_event_columns += correlated ? 1 : 0;
        }
        result_.telemetry.event_columns = columns_.size();
        return true;
    }

    bool build_event_references()
    {
        starts_.reserve(result_.arrangement.edges.size());
        ends_.reserve(result_.arrangement.edges.size());
        for (std::uint32_t edge = 0; edge < sweep_edges_.size(); ++edge)
            if (!sweep_edges_[edge].vertical)
            {
                starts_.push_back({sweep_edges_[edge].left_vertex, edge, true});
                ends_.push_back({sweep_edges_[edge].right_vertex, edge, false});
            }
        if (!charge_sort(starts_.size()) || !charge_sort(ends_.size()))
            return false;
        const auto less = [](const EventReference& left, const EventReference& right)
        { return std::tie(left.vertex, left.edge) < std::tie(right.vertex, right.edge); };
        std::sort(starts_.begin(), starts_.end(), less);
        std::sort(ends_.begin(), ends_.end(), less);
        build_reference_ranges(starts_, start_ranges_);
        build_reference_ranges(ends_, end_ranges_);
        return true;
    }

    void build_reference_ranges(const std::vector<EventReference>& values,
                                std::vector<std::pair<std::uint32_t, std::uint32_t>>& ranges)
    {
        ranges.assign(result_.arrangement.vertices.size(), {0, 0});
        std::uint32_t cursor = 0;
        while (cursor < values.size())
        {
            const std::uint32_t vertex = values[cursor].vertex;
            const std::uint32_t begin = cursor;
            while (cursor < values.size() && values[cursor].vertex == vertex)
                ++cursor;
            ranges[vertex] = {begin, cursor};
        }
    }

    std::pair<std::vector<EventReference>::const_iterator,
              std::vector<EventReference>::const_iterator>
    references_for(const std::vector<EventReference>& values,
                   const std::vector<std::pair<std::uint32_t, std::uint32_t>>& ranges,
                   std::uint32_t vertex) const
    {
        if (vertex >= ranges.size())
            return {values.end(), values.end()};
        return {values.begin() + ranges[vertex].first, values.begin() + ranges[vertex].second};
    }

    std::optional<bool> lower_tangent_group(std::uint32_t edge_index) const noexcept
    {
        const AnalyticArrangementEdgeNm& edge = result_.arrangement.edges[edge_index];
        Interval dy;
        std::int8_t curvature = 0;
        if (edge.kind == AnalyticAtomicCurveKind::line)
        {
            if (edge.has_construction_line_direction)
                dy = exact(static_cast<double>(edge.construction_line_dy));
            else
            {
                Point start = point(edge.carrier_start);
                Point end = point(edge.carrier_end);
                if (sweep_edges_[edge_index].left_vertex == edge.end_vertex)
                    std::swap(start, end);
                dy = subtract(end.y, start.y);
            }
        }
        else
        {
            const AnalyticFilteredPointNm& left =
                edge.x_monotone_branch == AnalyticXMonotoneBranch::lower ? edge.carrier_start
                                                                         : edge.carrier_end;
            const Interval radial_x =
                subtract(Interval{left.x.lower, left.x.upper},
                         Interval{edge.circle.center.x.lower, edge.circle.center.x.upper});
            if (edge.x_monotone_branch == AnalyticXMonotoneBranch::lower)
            {
                dy = radial_x;
                curvature = 1;
            }
            else
            {
                dy = {-radial_x.upper, -radial_x.lower};
                curvature = -1;
            }
        }
        if (dy.upper < 0.0)
            return true;
        if (dy.lower > 0.0)
            return false;
        if (dy.lower == 0.0 && dy.upper == 0.0)
            return curvature < 0;
        return std::nullopt;
    }

    bool ordered_starts(std::uint32_t vertex, std::vector<std::uint32_t>& ordered)
    {
        const auto [begin, end] = references_for(starts_, start_ranges_, vertex);
        if (begin == end)
            return true;
        std::vector<std::uint32_t> expected;
        expected.reserve(static_cast<std::size_t>(end - begin));
        for (auto value = begin; value != end; ++value)
            expected.push_back(value->edge);
        if (!charge_sort(expected.size()))
            return false;
        std::sort(expected.begin(), expected.end());
        std::vector<std::uint32_t> lower;
        std::vector<std::uint32_t> upper;
        lower.reserve(expected.size());
        upper.reserve(expected.size());
        ordered.reserve(expected.size());
        const AnalyticArrangementVertexNm& value = result_.arrangement.vertices[vertex];
        if (value.outgoing_begin > result_.arrangement.outgoing_half_edges.size() ||
            value.outgoing_count >
                result_.arrangement.outgoing_half_edges.size() - value.outgoing_begin)
            return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
        bool valid = true;
        const std::uint64_t search_work =
            checked_multiply(value.outgoing_count, tree_operation_units(expected.size()), valid);
        if (!valid || !charge(search_work))
            return false;
        for (std::uint32_t offset = 0; offset < value.outgoing_count; ++offset)
        {
            const std::uint32_t half_edge =
                result_.arrangement.outgoing_half_edges[value.outgoing_begin + offset];
            if (half_edge >= result_.arrangement.half_edges.size())
                return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
            const std::uint32_t edge = result_.arrangement.half_edges[half_edge].edge;
            if (!std::binary_search(expected.begin(), expected.end(), edge))
                continue;
            const std::optional<bool> is_lower = lower_tangent_group(edge);
            if (!is_lower)
                return fail(AnalyticFilteredBooleanSelectionError::resource_limit_exceeded);
            (*is_lower ? lower : upper).push_back(edge);
        }
        ordered.insert(ordered.end(), lower.begin(), lower.end());
        ordered.insert(ordered.end(), upper.begin(), upper.end());
        std::vector<std::uint32_t> observed = ordered;
        if (!charge_sort(observed.size()))
            return false;
        std::sort(observed.begin(), observed.end());
        if (observed != expected)
            return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
        return true;
    }

    bool charge_status_operation(bool update)
    {
        const std::uint64_t units = tree_operation_units(result_.arrangement.edges.size());
        if (!charge(units))
            return false;
        if (update)
            result_.telemetry.sweep_status_update_work_units += units;
        return true;
    }

    std::optional<std::uint32_t> insertion_rank(SweepOrder& status, std::uint32_t vertex)
    {
        if (!charge_status_operation(false))
            return std::nullopt;
        std::uint64_t visits = 0;
        const std::optional<std::uint32_t> rank = status.insertion_rank(
            [&](std::uint32_t edge) { return compare_vertex_to_edge(vertex, edge); }, visits);
        result_.telemetry.sweep_status_node_visits += visits;
        return rank;
    }

    bool unite_gap(SweepOrder& status, DisjointSet& faces, std::uint32_t rank)
    {
        if (rank > status.size() || !charge(1))
            return false;
        const std::uint32_t sentinel =
            static_cast<std::uint32_t>(result_.arrangement.cycles.size());
        std::uint32_t lower_edge = kNoIndex;
        std::uint32_t upper_edge = kNoIndex;
        if (rank != 0)
        {
            if (!charge_status_operation(false))
                return false;
            lower_edge = status.edge_at(rank - 1);
        }
        if (rank != status.size())
        {
            if (!charge_status_operation(false))
                return false;
            upper_edge = status.edge_at(rank);
        }
        const std::uint32_t lower_cycle =
            lower_edge == kNoIndex ? sentinel : sweep_edges_[lower_edge].above_cycle;
        const std::uint32_t upper_cycle =
            upper_edge == kNoIndex ? sentinel : sweep_edges_[upper_edge].below_cycle;
        if (lower_cycle > sentinel || upper_cycle > sentinel)
            return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
        bool valid = true;
        const std::uint64_t dsu_work =
            checked_multiply(2, tree_operation_units(result_.arrangement.cycles.size() + 1), valid);
        if (!valid || !charge(dsu_work))
            return false;
        std::uint64_t visits = 0;
        if (faces.unite(lower_cycle, upper_cycle, visits))
            ++result_.telemetry.face_gap_unions;
        result_.telemetry.disjoint_set_node_visits += visits;
        return true;
    }

    bool sweep_columns(DisjointSet& faces)
    {
        SweepOrder status(result_.arrangement.edges.size());
        std::vector<std::uint32_t> inserted;
        std::vector<std::uint32_t> removal_vertices;
        std::vector<std::pair<std::uint32_t, std::uint32_t>> removals;
        inserted.reserve(result_.arrangement.edges.size());
        removal_vertices.reserve(result_.arrangement.edges.size());
        removals.reserve(result_.arrangement.edges.size());
        for (const EventColumn& column : columns_)
        {
            for (std::uint32_t offset = 0; offset < column.vertex_count; ++offset)
            {
                const std::uint32_t vertex = vertex_order_[column.vertex_begin + offset];
                const auto [begin, end] = references_for(ends_, end_ranges_, vertex);
                std::vector<std::uint32_t> ranks;
                ranks.reserve(static_cast<std::size_t>(end - begin));
                for (auto value = begin; value != end; ++value)
                {
                    if (!charge_status_operation(false))
                        return false;
                    const std::uint32_t rank = status.rank_of(value->edge);
                    if (rank == kNoIndex)
                        return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
                    ranks.push_back(rank);
                    removals.push_back({rank, value->edge});
                }
                if (!charge_sort(ranks.size()))
                    return false;
                std::sort(ranks.begin(), ranks.end());
                for (std::size_t index = 1; index < ranks.size(); ++index)
                    if (ranks[index] != ranks[index - 1] + 1)
                        return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
                const auto [start_begin, start_end] =
                    references_for(starts_, start_ranges_, vertex);
                if (!ranks.empty() && start_begin == start_end)
                    removal_vertices.push_back(vertex);
            }
            if (!charge_sort(removals.size()))
                return false;
            std::sort(removals.begin(), removals.end(),
                      [](const auto& left, const auto& right) { return left.first > right.first; });
            for (const auto [rank, edge] : removals)
            {
                (void)rank;
                if (!charge_status_operation(true) || !status.erase(edge))
                    return false;
            }

            for (std::uint32_t offset = 0; offset < column.vertex_count; ++offset)
            {
                const std::uint32_t vertex = vertex_order_[column.vertex_begin + offset];
                std::vector<std::uint32_t> block;
                if (!ordered_starts(vertex, block))
                    return false;
                if (block.empty())
                    continue;
                const std::optional<std::uint32_t> rank = insertion_rank(status, vertex);
                if (!rank)
                    return fail(AnalyticFilteredBooleanSelectionError::resource_limit_exceeded);
                for (std::uint32_t local = 0; local < block.size(); ++local)
                {
                    if (!charge_status_operation(true) ||
                        !status.insert(*rank + local, block[local]))
                        return false;
                    inserted.push_back(block[local]);
                }
            }

            for (const std::uint32_t edge : inserted)
            {
                if (!charge_status_operation(false))
                    return false;
                const std::uint32_t rank = status.rank_of(edge);
                if (rank == kNoIndex || !unite_gap(status, faces, rank) ||
                    !unite_gap(status, faces, rank + 1))
                    return false;
            }
            for (const std::uint32_t vertex : removal_vertices)
            {
                const std::optional<std::uint32_t> rank = insertion_rank(status, vertex);
                if (!rank || !unite_gap(status, faces, *rank))
                    return false;
            }
            inserted.clear();
            removal_vertices.clear();
            removals.clear();
        }
        return status.size() == 0;
    }

    bool publish_faces(DisjointSet& ownership)
    {
        const std::uint32_t cycle_count =
            static_cast<std::uint32_t>(result_.arrangement.cycles.size());
        bool valid = true;
        const std::uint64_t root_work =
            checked_multiply(cycle_count + 1, tree_operation_units(cycle_count + 1), valid);
        if (!valid || !charge(root_work))
            return false;
        std::vector<std::uint32_t> root_by_cycle(cycle_count + 1);
        std::uint64_t visits = 0;
        for (std::uint32_t cycle = 0; cycle <= cycle_count; ++cycle)
            root_by_cycle[cycle] = ownership.find(cycle, visits);
        result_.telemetry.disjoint_set_node_visits += visits;
        const std::uint32_t sentinel_root = root_by_cycle[cycle_count];
        std::vector<std::uint32_t> ccw_by_root(cycle_count + 1, kNoIndex);
        for (std::uint32_t cycle = 0; cycle < cycle_count; ++cycle)
        {
            if (!result_.arrangement.cycles[cycle].counterclockwise)
                continue;
            const std::uint32_t root = root_by_cycle[cycle];
            if (root == sentinel_root || ccw_by_root[root] != kNoIndex)
                return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
            ccw_by_root[root] = cycle;
        }
        std::vector<std::uint32_t> face_by_root(cycle_count + 1, kNoIndex);
        face_by_root[sentinel_root] = 0;
        if (limits_.arrangement_faces == 0)
            return fail(AnalyticFilteredBooleanSelectionError::resource_limit_exceeded);
        result_.faces.reserve(static_cast<std::size_t>(expected_face_capacity_));
        result_.face_boundary_cycles.reserve(cycle_count);
        result_.faces.push_back({0, 0, 0, 0, kNoIndex, true, false});
        for (std::uint32_t cycle = 0; cycle < cycle_count; ++cycle)
        {
            if (!result_.arrangement.cycles[cycle].counterclockwise)
                continue;
            const std::uint32_t root = root_by_cycle[cycle];
            if (result_.faces.size() == limits_.arrangement_faces)
                return fail(AnalyticFilteredBooleanSelectionError::resource_limit_exceeded);
            face_by_root[root] = static_cast<std::uint32_t>(result_.faces.size());
            result_.faces.push_back({});
        }
        std::vector<std::pair<std::uint32_t, std::uint32_t>> boundaries;
        boundaries.reserve(cycle_count);
        result_.half_edge_faces.resize(result_.arrangement.half_edges.size());
        for (std::uint32_t cycle = 0; cycle < cycle_count; ++cycle)
        {
            const std::uint32_t root = root_by_cycle[cycle];
            if (root != sentinel_root && ccw_by_root[root] == kNoIndex)
                return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
            const std::uint32_t face = face_by_root[root];
            if (face == kNoIndex)
                return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
            boundaries.push_back({face, cycle});
            const AnalyticArrangementCycle& value = result_.arrangement.cycles[cycle];
            if (value.half_edge_begin > result_.arrangement.cycle_half_edges.size() ||
                value.half_edge_count >
                    result_.arrangement.cycle_half_edges.size() - value.half_edge_begin)
                return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
            for (std::uint32_t offset = 0; offset < value.half_edge_count; ++offset)
            {
                const std::uint32_t half_edge =
                    result_.arrangement.cycle_half_edges[value.half_edge_begin + offset];
                if (half_edge >= result_.half_edge_faces.size())
                    return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
                result_.half_edge_faces[half_edge] = face;
            }
        }
        if (!charge_sort(boundaries.size()))
            return false;
        std::sort(boundaries.begin(), boundaries.end(),
                  [&](const auto& left, const auto& right)
                  {
                      if (left.first != right.first)
                          return left.first < right.first;
                      const bool left_primary =
                          ccw_by_root[root_by_cycle[left.second]] == left.second;
                      const bool right_primary =
                          ccw_by_root[root_by_cycle[right.second]] == right.second;
                      if (left_primary != right_primary)
                          return left_primary;
                      return left.second < right.second;
                  });
        std::uint32_t cursor = 0;
        for (std::uint32_t face = 0; face < result_.faces.size(); ++face)
        {
            const std::uint32_t begin = cursor;
            while (cursor < boundaries.size() && boundaries[cursor].first == face)
                result_.face_boundary_cycles.push_back(boundaries[cursor++].second);
            result_.faces[face].boundary_cycle_begin = begin;
            result_.faces[face].boundary_cycle_count = cursor - begin;
            if (face != 0 && result_.faces[face].boundary_cycle_count == 0)
                return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
        }
        result_.telemetry.emitted_faces = result_.faces.size();
        return cursor == boundaries.size();
    }

    bool build_face_topology()
    {
        if (!classify_sweep_edges() || !build_event_columns() || !build_event_references())
            return false;
        DisjointSet ownership(result_.arrangement.cycles.size() + 1);
        if (!sweep_columns(ownership) || !publish_faces(ownership))
            return false;
        return true;
    }

    std::pair<std::uint32_t, std::uint32_t> transition_range(std::uint32_t edge) const
    {
        if (edge + 1 >= transition_begin_.size())
            return {0, 0};
        return {transition_begin_[edge], transition_begin_[edge + 1]};
    }

    bool build_transitions()
    {
        const auto& arrangement = result_.arrangement;
        bool valid = true;
        std::uint64_t scratch =
            checked_multiply(arrangement.memberships.size(), kTransitionLogicalBytes, valid);
        scratch = checked_add(
            scratch, checked_multiply(arrangement.edges.size() + 1, kIndexLogicalBytes, valid),
            valid);
        if (!valid)
            return fail(AnalyticFilteredBooleanSelectionError::resource_limit_exceeded);
        if (!set_phase_memory(scratch) || !charge(arrangement.memberships.size()))
            return false;
        transitions_.reserve(arrangement.memberships.size());
        for (std::uint32_t edge_index = 0; edge_index < arrangement.edges.size(); ++edge_index)
        {
            const AnalyticArrangementEdgeNm& edge = arrangement.edges[edge_index];
            if (edge.membership_begin > arrangement.memberships.size() ||
                edge.membership_count > arrangement.memberships.size() - edge.membership_begin)
                return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
            for (std::uint32_t offset = 0; offset < edge.membership_count; ++offset)
            {
                const AnalyticSpanMembership& membership =
                    arrangement.memberships[edge.membership_begin + offset];
                if (membership.curve_index == 0 ||
                    membership.curve_index > geometry_.occurrences.size())
                    return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
                const AnalyticFilteredOccurrence& occurrence =
                    geometry_.occurrences[membership.curve_index - 1];
                const std::uint32_t operand = occurrence_operands_[membership.curve_index - 1];
                if (operand >= operands_.size() ||
                    operands_[operand].operand_id != occurrence.coverage_id)
                    return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
                transitions_.push_back({edge_index, operand, occurrence.coverage_id,
                                        membership.material_on_span_left});
            }
        }
        if (!charge_sort(transitions_.size()))
            return false;
        std::sort(transitions_.begin(), transitions_.end(),
                  [](const Transition& left, const Transition& right)
                  {
                      return std::tie(left.edge, left.coverage_id, left.expected_left) <
                             std::tie(right.edge, right.coverage_id, right.expected_left);
                  });
        if (!charge(transitions_.size()))
            return false;
        std::size_t output = 0;
        for (const Transition transition : transitions_)
        {
            if (output != 0 && transitions_[output - 1].edge == transition.edge &&
                transitions_[output - 1].coverage_id == transition.coverage_id)
            {
                if (transitions_[output - 1].expected_left != transition.expected_left)
                    return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
                continue;
            }
            transitions_[output++] = transition;
        }
        transitions_.resize(output);
        if (!charge(arrangement.edges.size() + transitions_.size()))
            return false;
        transition_begin_.resize(arrangement.edges.size() + 1);
        std::uint32_t cursor = 0;
        for (std::uint32_t edge = 0; edge < arrangement.edges.size(); ++edge)
        {
            transition_begin_[edge] = cursor;
            while (cursor < transitions_.size() && transitions_[cursor].edge == edge)
                ++cursor;
        }
        transition_begin_.back() = cursor;
        if (cursor != transitions_.size())
            return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
        result_.telemetry.transition_records = transitions_.size();
        return true;
    }

    bool build_edge_faces(std::vector<std::uint32_t>& left, std::vector<std::uint32_t>& right)
    {
        if (!charge(result_.arrangement.half_edges.size() + result_.arrangement.edges.size()))
            return false;
        left.assign(result_.arrangement.edges.size(), kNoIndex);
        right.assign(result_.arrangement.edges.size(), kNoIndex);
        for (std::uint32_t half_edge = 0; half_edge < result_.arrangement.half_edges.size();
             ++half_edge)
        {
            const AnalyticArrangementHalfEdge& value = result_.arrangement.half_edges[half_edge];
            if (value.edge >= left.size() || half_edge >= result_.half_edge_faces.size())
                return false;
            std::uint32_t& slot = value.forward ? left[value.edge] : right[value.edge];
            if (slot != kNoIndex)
                return false;
            slot = result_.half_edge_faces[half_edge];
        }
        for (std::uint32_t edge = 0; edge < left.size(); ++edge)
            if (left[edge] == kNoIndex || right[edge] == kNoIndex || left[edge] == right[edge] ||
                left[edge] >= result_.faces.size() || right[edge] >= result_.faces.size())
                return false;
        return true;
    }

    bool toggle_tree_edge(std::uint32_t edge, std::uint32_t face,
                          const std::vector<std::uint32_t>& left_faces,
                          CanonicalCoverageSet& coverage, ActiveStageTree& stages,
                          std::vector<std::uint8_t>& active, std::uint32_t& root,
                          bool validate_side, bool update_coverage,
                          OutcomeHistoryTracker* outcome_tracker)
    {
        const auto [begin, end] = transition_range(edge);
        if (!charge(1 + end - begin))
            return false;
        if (outcome_tracker != nullptr)
            outcome_tracker->begin_batch();
        for (std::uint32_t index = begin; index < end; ++index)
        {
            const Transition& transition = transitions_[index];
            if (transition.operand >= active.size())
                return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
            const bool present = active[transition.operand] != 0;
            if (validate_side)
            {
                const bool expected =
                    face == left_faces[edge] ? transition.expected_left : !transition.expected_left;
                if (present != expected)
                    return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
            }
            if (update_coverage)
            {
                std::uint32_t updated = 0;
                if (!coverage.toggle(
                        root, transition.operand, updated,
                        [&](std::uint64_t units) { return charge(units); },
                        result_.telemetry.coverage_state_update_work_units,
                        result_.telemetry.coverage_state_table_probes))
                {
                    if (result_.error == AnalyticFilteredBooleanSelectionError::none)
                        return fail(AnalyticFilteredBooleanSelectionError::resource_limit_exceeded);
                    return false;
                }
                root = updated;
            }
            active[transition.operand] = present ? 0 : 1;
            if (outcome_tracker != nullptr && !outcome_tracker->record_toggle(transition.operand))
                return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
            if (!stages.toggle(
                    operands_[transition.operand].stage_ordinal, !present, [&](std::uint64_t units)
                    { return charge(units); }, result_.telemetry.stage_state_update_work_units))
            {
                if (result_.error == AnalyticFilteredBooleanSelectionError::none)
                    return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
                return false;
            }
        }
        if (outcome_tracker != nullptr && !outcome_tracker->finish_batch())
            return fail(outcome_tracker->resource_exhausted()
                            ? AnalyticFilteredBooleanSelectionError::resource_limit_exceeded
                            : AnalyticFilteredBooleanSelectionError::invalid_argument);
        return true;
    }

    bool candidate_root_across(std::uint32_t edge, std::uint32_t face,
                               const std::vector<std::uint32_t>& left_faces,
                               CanonicalCoverageSet& coverage,
                               const std::vector<std::uint8_t>& active, std::uint32_t root,
                               std::uint32_t& candidate)
    {
        const auto [begin, end] = transition_range(edge);
        if (!charge(1 + end - begin))
            return false;
        candidate = root;
        for (std::uint32_t index = begin; index < end; ++index)
        {
            const Transition& transition = transitions_[index];
            if (transition.operand >= active.size())
                return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
            const bool expected =
                face == left_faces[edge] ? transition.expected_left : !transition.expected_left;
            if ((active[transition.operand] != 0) != expected)
                return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
            std::uint32_t updated = 0;
            if (!coverage.toggle(
                    candidate, transition.operand, updated, [&](std::uint64_t units)
                    { return charge(units); }, result_.telemetry.coverage_state_update_work_units,
                    result_.telemetry.coverage_state_table_probes))
            {
                if (result_.error == AnalyticFilteredBooleanSelectionError::none)
                    return fail(AnalyticFilteredBooleanSelectionError::resource_limit_exceeded);
                return false;
            }
            candidate = updated;
        }
        return true;
    }

    bool propagate_coverages()
    {
        bool valid = true;
        const std::uint64_t maximum_nodes =
            coverage_maximum_nodes(transitions_.size(), operands_.size(), valid);
        if (maximum_nodes > std::numeric_limits<std::uint32_t>::max())
            valid = false;
        const std::uint64_t table_capacity = coverage_table_capacity(maximum_nodes, valid);
        std::uint64_t stage_leaf_capacity = 1;
        while (stage_leaf_capacity < std::max<std::uint64_t>(1, stage_operations_.size()))
            stage_leaf_capacity *= 2;

        std::uint64_t scratch = checked_multiply(result_.arrangement.memberships.size(),
                                                 kTransitionLogicalBytes, valid);
        scratch = checked_add(
            scratch,
            checked_multiply(result_.arrangement.edges.size() + 1, kIndexLogicalBytes, valid),
            valid);
        scratch = checked_add(scratch,
                              checked_multiply(result_.arrangement.edges.size() * 2,
                                               kIndexLogicalBytes + kAdjacencyLogicalBytes, valid),
                              valid);
        scratch = checked_add(
            scratch, checked_multiply(result_.faces.size() + 1, kIndexLogicalBytes, valid), valid);
        scratch = checked_add(scratch,
                              checked_multiply(result_.faces.size(),
                                               kByteLogicalBytes + kDualFrameLogicalBytes, valid),
                              valid);
        scratch = checked_add(
            scratch,
            checked_multiply(operands_.size(), kByteLogicalBytes + kIndexLogicalBytes, valid),
            valid);
        scratch = checked_add(
            scratch, checked_multiply(stage_operations_.size(), kIndexLogicalBytes, valid), valid);
        scratch = checked_add(
            scratch, checked_multiply(stage_leaf_capacity * 4, kIndexLogicalBytes, valid), valid);
        scratch = checked_add(
            scratch, checked_multiply(maximum_nodes, kCoverageNodeLogicalBytes, valid), valid);
        scratch = checked_add(
            scratch, checked_multiply(table_capacity, kCoverageTableEntryLogicalBytes, valid),
            valid);
        if (collect_outcomes_)
            scratch = checked_add(
                scratch,
                outcome_tracker_logical_bytes(operands_.size(), stage_operations_.size(), valid),
                valid);
        if (!valid)
            return fail(AnalyticFilteredBooleanSelectionError::resource_limit_exceeded);
        if (!set_phase_memory(scratch))
            return false;
        std::uint64_t initialization_work = table_capacity;
        initialization_work = checked_add(
            initialization_work, stage_operations_.size() + stage_leaf_capacity * 4, valid);
        initialization_work = checked_add(
            initialization_work,
            result_.faces.size() + result_.arrangement.edges.size() + operands_.size(), valid);
        if (!valid)
            return fail(AnalyticFilteredBooleanSelectionError::resource_limit_exceeded);
        if (!charge(initialization_work))
            return false;

        std::vector<std::uint32_t> left_faces;
        std::vector<std::uint32_t> right_faces;
        if (!build_edge_faces(left_faces, right_faces))
        {
            if (result_.error == AnalyticFilteredBooleanSelectionError::none)
                return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
            return false;
        }
        std::vector<Adjacency> adjacency;
        adjacency.reserve(result_.arrangement.edges.size() * 2);
        if (!charge(result_.arrangement.edges.size()))
            return false;
        for (std::uint32_t edge = 0; edge < result_.arrangement.edges.size(); ++edge)
        {
            adjacency.push_back({left_faces[edge], right_faces[edge], edge});
            adjacency.push_back({right_faces[edge], left_faces[edge], edge});
        }
        if (!charge_sort(adjacency.size()))
            return false;
        std::sort(adjacency.begin(), adjacency.end(),
                  [](const Adjacency& left, const Adjacency& right)
                  {
                      return std::tie(left.face, left.neighbor, left.edge) <
                             std::tie(right.face, right.neighbor, right.edge);
                  });
        std::vector<std::uint32_t> adjacency_begin(result_.faces.size() + 1);
        if (!charge(result_.faces.size() + 1))
            return false;
        std::uint32_t adjacency_cursor = 0;
        for (std::uint32_t face = 0; face < result_.faces.size(); ++face)
        {
            adjacency_begin[face] = adjacency_cursor;
            while (adjacency_cursor < adjacency.size() && adjacency[adjacency_cursor].face == face)
                ++adjacency_cursor;
        }
        adjacency_begin.back() = adjacency_cursor;
        if (adjacency_cursor != adjacency.size())
            return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);

        CanonicalCoverageSet coverage(result_.coverage_state_nodes,
                                      static_cast<std::uint32_t>(operands_.size()), maximum_nodes,
                                      table_capacity);
        ActiveStageTree stages(stage_operations_);
        std::unique_ptr<OutcomeHistoryTracker> outcome_tracker;
        if (collect_outcomes_)
        {
            outcome_tracker = std::make_unique<OutcomeHistoryTracker>(
                operands_, stage_operations_, result_.outcome_evidence, result_.telemetry,
                result_.telemetry.predicate_calls, limits_.predicate_calls);
            if (!outcome_tracker->initialize())
                return fail(AnalyticFilteredBooleanSelectionError::resource_limit_exceeded);
        }
        struct Frame
        {
            std::uint32_t face = 0;
            std::uint32_t cursor = 0;
            std::uint32_t end = 0;
            std::uint32_t entering_edge = kNoIndex;
            std::uint32_t previous_root = 0;
        };
        std::vector<std::uint8_t> visited(result_.faces.size());
        std::vector<std::uint8_t> edge_validated(result_.arrangement.edges.size());
        std::vector<std::uint8_t> active(operands_.size());
        std::vector<Frame> stack;
        stack.reserve(result_.faces.size());
        std::uint32_t root = 0;
        visited[0] = 1;
        result_.faces[0].coverage_state_root = 0;
        result_.faces[0].positive_stage_begin = 0;
        result_.faces[0].active_removal_stage = kNoIndex;
        result_.faces[0].material = false;
        if (!charge(1))
            return false;
        stack.push_back({0, adjacency_begin[0], adjacency_begin[1], kNoIndex, 0});
        while (!stack.empty())
        {
            Frame& frame = stack.back();
            if (frame.cursor == frame.end)
            {
                const Frame finished = frame;
                stack.pop_back();
                if (finished.entering_edge != kNoIndex)
                {
                    if (!toggle_tree_edge(finished.entering_edge, 0, left_faces, coverage, stages,
                                          active, root, false, false, outcome_tracker.get()))
                        return false;
                    root = finished.previous_root;
                }
                continue;
            }
            const Adjacency next = adjacency[frame.cursor++];
            ++result_.telemetry.dual_adjacency_visits;
            if (!charge(1) || next.neighbor >= visited.size() || next.edge >= edge_validated.size())
                return result_.error == AnalyticFilteredBooleanSelectionError::none
                           ? fail(AnalyticFilteredBooleanSelectionError::invalid_argument)
                           : false;
            if (!visited[next.neighbor])
            {
                const std::uint32_t previous_root = root;
                if (!toggle_tree_edge(next.edge, frame.face, left_faces, coverage, stages, active,
                                      root, true, true, outcome_tracker.get()))
                    return false;
                edge_validated[next.edge] = 1;
                visited[next.neighbor] = 1;
                result_.faces[next.neighbor].coverage_state_root = root;
                result_.faces[next.neighbor].material = stages.material();
                result_.faces[next.neighbor].positive_stage_begin = stages.positive_stage_begin();
                if (!charge(stages.depth() * 2 + 1))
                    return false;
                result_.faces[next.neighbor].active_removal_stage = stages.active_removal_stage();
                if (outcome_tracker != nullptr &&
                    !outcome_tracker->evaluate(result_.faces[next.neighbor]))
                    return fail(outcome_tracker->resource_exhausted()
                                    ? AnalyticFilteredBooleanSelectionError::resource_limit_exceeded
                                    : AnalyticFilteredBooleanSelectionError::invalid_argument);
                if (!charge(1))
                    return false;
                stack.push_back({next.neighbor, adjacency_begin[next.neighbor],
                                 adjacency_begin[next.neighbor + 1], next.edge, previous_root});
            }
            else if (!edge_validated[next.edge])
            {
                std::uint32_t candidate = 0;
                if (!candidate_root_across(next.edge, frame.face, left_faces, coverage, active,
                                           root, candidate))
                    return false;
                if (candidate != result_.faces[next.neighbor].coverage_state_root)
                    return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
                edge_validated[next.edge] = 1;
                ++result_.telemetry.non_tree_edge_validations;
            }
        }
        if (!charge(active.size() + visited.size() + edge_validated.size() + result_.faces.size()))
            return false;
        if (root != 0 || stages.material() ||
            std::find(active.begin(), active.end(), 1) != active.end() ||
            std::find(visited.begin(), visited.end(), 0) != visited.end() ||
            std::find(edge_validated.begin(), edge_validated.end(), 0) != edge_validated.end())
            return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
        if (outcome_tracker != nullptr && !outcome_tracker->empty())
            return fail(AnalyticFilteredBooleanSelectionError::invalid_argument);
        result_.telemetry.coverage_state_nodes = result_.coverage_state_nodes.size();
        result_.telemetry.material_faces =
            std::count_if(result_.faces.begin(), result_.faces.end(),
                          [](const AnalyticFilteredSelectedFace& face) { return face.material; });
        return !result_.faces.empty() && result_.faces[0].unbounded && !result_.faces[0].material;
    }

    const AnalyticRequestPacketRecords& records_;
    std::uint32_t job_index_ = 0;
    std::uint64_t expected_operand_count_ = 0;
    std::uint64_t expected_face_capacity_ = 0;
    const AnalyticFilteredGeometry& geometry_;
    AnalyticSolverLimits limits_;
    AnalyticFilteredBooleanSelectionResult result_;
    std::vector<OperandMetadata> operands_;
    std::vector<std::uint32_t> occurrence_operands_;
    std::vector<SweepEdge> sweep_edges_;
    std::vector<std::uint32_t> vertex_order_;
    std::vector<EventColumn> columns_;
    std::vector<EventReference> starts_;
    std::vector<EventReference> ends_;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> start_ranges_;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> end_ranges_;
    std::vector<Transition> transitions_;
    std::vector<std::uint32_t> transition_begin_;
    std::vector<std::uint8_t> stage_operations_;
    bool collect_outcomes_ = false;
};

static_assert(sizeof(SweepEdge) <= kSweepEdgeLogicalBytes);
static_assert(sizeof(EventReference) <= kEventReferenceLogicalBytes);
static_assert(sizeof(EventColumn) <= kColumnLogicalBytes);
static_assert(sizeof(Transition) <= kTransitionLogicalBytes);
static_assert(sizeof(Adjacency) <= kAdjacencyLogicalBytes);
static_assert(sizeof(OperandMetadata) <= kOperandMetadataLogicalBytes);
static_assert(sizeof(CoverageTableEntry) <= kCoverageTableEntryLogicalBytes);
static_assert(sizeof(AnalyticFilteredOccurrence) <= kOccurrenceLogicalBytes);
static_assert(sizeof(AnalyticFilteredSelectedFace) <= kFaceLogicalBytes);
static_assert(sizeof(AnalyticFilteredCoverageStateNode) <= kCoverageNodeLogicalBytes);

} // namespace

AnalyticFilteredBooleanSelectionResult
analytic_selection_detail::finish_boolean_selection_from_admission(
    const AnalyticRequestPacketRecords& records, std::uint32_t job_index,
    const AnalyticFilteredGeometry& geometry, SelectionAdmission admission)
{
    if (!admission.ready)
    {
        admission.result.origin_x_nm = geometry.origin_x_nm;
        admission.result.origin_y_nm = geometry.origin_y_nm;
        return std::move(admission.result);
    }
    return SelectionBuilder(records, job_index, geometry, std::move(admission.arrangement),
                            admission.execution_limits, admission.admission_work,
                            admission.admission_peak_memory, admission.collect_outcomes)
        .build();
}

AnalyticFilteredBooleanSelectionResult build_analytic_filtered_boolean_selection(
    const AnalyticRequestPacketRecords& records, std::uint32_t job_index,
    const AnalyticFilteredGeometry& geometry, const std::vector<AnalyticCurvePair>& candidate_pairs,
    const AnalyticSolverLimits& limits)
{
    analytic_selection_detail::SelectionAdmission admission =
        analytic_selection_detail::prepare_boolean_selection_admission(records, job_index, geometry,
                                                                       candidate_pairs, limits);
    return analytic_selection_detail::finish_boolean_selection_from_admission(
        records, job_index, geometry, std::move(admission));
}

} // namespace geometer

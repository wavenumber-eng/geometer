#include "analytic_filtered_relationships.h"

#include "analytic_endpoint_arc_reconstruction.h"
#include "analytic_filtered_execution_policy.h"
#include "analytic_filtered_interval.h"
#include "analytic_wide_integer.h"
#include "geometer/analytic_curve_broad_phase.h"
#include "geometer/analytic_curve_narrow_phase.h"
#include "geometer/analytic_filtered_boolean_selection.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <numeric>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

namespace geometer::analytic_relationship_detail
{
namespace
{
#include "analytic_filtered_relationship_geometry.h"

struct RelationEmissionInput
{
    const AnalyticResultPacketRecords& records;
    const AnalyticJobResultRecord& left_job;
    const AnalyticJobResultRecord& right_job;
    const GeometryBuild& built;
    const AnalyticFilteredBooleanSelectionResult& selection;
    std::vector<bool>& saw_face;
    std::vector<bool>& fully_covered;
    std::vector<std::uint32_t>& unique_cover;
    const std::vector<std::uint32_t>& edge_offsets;
    const std::vector<std::uint32_t>& edge_owners;
    std::vector<std::uint32_t>& scratch;
    bool self = false;
    bool& vertex_collection_resource_failure;
    std::uint64_t event_capacity = 0;
    std::uint64_t fixed_classification_bytes = 0;
    std::uint64_t memory_limit = 0;
    std::uint64_t work_limit = 0;
    std::uint64_t retained_memory = 0;
    std::uint64_t base_live = 0;
    std::uint32_t operand_count = 0;
    std::uint32_t coverage_capacity = 0;
};

template <typename Charge, typename IncidenceCount, typename CollectVertexOperands>
bool emit_relation_events(PairBuild& output, const RelationEmissionInput& input, Charge&& charge,
                          IncidenceCount&& incidence_count,
                          CollectVertexOperands&& collect_vertex_operands)
{
    const auto& records = input.records;
    const auto& left_job = input.left_job;
    const auto& right_job = input.right_job;
    const auto& built = input.built;
    const auto& selection = input.selection;
    auto& saw_face = input.saw_face;
    auto& fully_covered = input.fully_covered;
    auto& unique_cover = input.unique_cover;
    const auto& edge_offsets = input.edge_offsets;
    const auto& edge_owners = input.edge_owners;
    auto& scratch = input.scratch;
    const bool self = input.self;
    bool& vertex_collection_resource_failure = input.vertex_collection_resource_failure;
    const std::uint64_t event_capacity = input.event_capacity;
    const std::uint64_t fixed_classification_bytes = input.fixed_classification_bytes;
    const std::uint64_t memory_limit = input.memory_limit;
    const std::uint64_t work_limit = input.work_limit;
    const std::uint64_t retained_memory = input.retained_memory;
    const std::uint64_t base_live = input.base_live;
    const std::uint32_t operand_count = input.operand_count;
    const std::uint32_t coverage_capacity = input.coverage_capacity;
    std::uint64_t term = 0;
    std::uint64_t event_bytes = 0;
    std::uint64_t maximum_output_bytes = 0;
    if (!checked_multiply(event_capacity, kRelationEventBytes, event_bytes) ||
        !checked_multiply(event_capacity, kRelationshipRowBytes, maximum_output_bytes) ||
        !checked_add(fixed_classification_bytes, event_bytes, term) ||
        !checked_add(term, maximum_output_bytes, term) ||
        term > memory_limit - retained_memory - base_live)
    {
        output.error = EvaluationError::resource_limit_exceeded;
        output.required_memory = retained_memory + base_live + fixed_classification_bytes +
                                 event_bytes + maximum_output_bytes;
        return false;
    }
    output.peak_memory = std::max(output.peak_memory, base_live + term);
    std::vector<RelationEvent> events;
    events.reserve(static_cast<std::size_t>(event_capacity));
    const auto add_incidence =
        [&](const std::vector<std::uint32_t>& operands, std::uint8_t dimension)
    {
        std::uint64_t count = 0;
        if (!incidence_count(operands, count) || !charge(count))
            return false;
        if (self)
        {
            for (const std::uint32_t left : operands)
                for (const std::uint32_t right : operands)
                    if (left != right)
                        events.push_back({{left, right}, dimension, false, false, false});
        }
        else
        {
            for (const std::uint32_t left : operands)
                if (left < built.left_operand_count)
                    for (const std::uint32_t right : operands)
                        if (right >= built.left_operand_count)
                            events.push_back({{left, right}, dimension, false, false, false});
        }
        return true;
    };
    if (self)
    {
        if (!charge(operand_count))
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return false;
        }
        for (std::uint32_t operand = 0; operand < operand_count; ++operand)
            events.push_back({{operand, operand}, 3, true, true, true});
    }
    else
    {
        std::uint64_t containment_events = 0;
        if (!charge(operand_count))
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return false;
        }
        for (std::uint32_t operand = 0; operand < operand_count; ++operand)
            containment_events +=
                saw_face[operand] && fully_covered[operand] &&
                unique_cover[operand] != std::numeric_limits<std::uint32_t>::max();
        if (!charge(static_cast<std::uint64_t>(operand_count) + containment_events))
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return false;
        }
        for (std::uint32_t operand = 0; operand < operand_count; ++operand)
        {
            if (!saw_face[operand] || !fully_covered[operand] ||
                unique_cover[operand] == std::numeric_limits<std::uint32_t>::max())
                continue;
            const std::uint32_t other = unique_cover[operand];
            const RegionPair pair = operand < built.left_operand_count ? RegionPair{operand, other}
                                                                       : RegionPair{other, operand};
            events.push_back({pair, 0, false, operand >= built.left_operand_count,
                              operand < built.left_operand_count});
        }
        for (const auto& face : selection.faces)
        {
            if (face.unbounded)
                continue;
            scratch.clear();
            bool coverage_resource_failure = false;
            if (!collect_coverage(selection, face.coverage_state_root, 0, coverage_capacity,
                                  scratch, output.work, work_limit, coverage_resource_failure))
            {
                output.error = coverage_resource_failure ? EvaluationError::resource_limit_exceeded
                                                         : EvaluationError::solver_failed;
                return false;
            }
            if (!charge(sort_units(scratch.size()) + scratch.size()))
            {
                output.error = EvaluationError::resource_limit_exceeded;
                return false;
            }
            std::sort(scratch.begin(), scratch.end());
            scratch.erase(std::unique(scratch.begin(), scratch.end()), scratch.end());
            if (!add_incidence(scratch, 3))
            {
                output.error = EvaluationError::resource_limit_exceeded;
                return false;
            }
        }
    }
    for (std::uint32_t edge = 0; edge < selection.arrangement.edges.size(); ++edge)
    {
        const std::uint64_t owner_count = edge_offsets[edge + 1] - edge_offsets[edge];
        if (!charge(owner_count))
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return false;
        }
        scratch.assign(edge_owners.begin() + edge_offsets[edge],
                       edge_owners.begin() + edge_offsets[edge + 1]);
        if (!add_incidence(scratch, 2))
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return false;
        }
    }
    for (const auto& vertex : selection.arrangement.vertices)
    {
        if (!collect_vertex_operands(vertex))
        {
            output.error = vertex_collection_resource_failure
                               ? EvaluationError::resource_limit_exceeded
                               : EvaluationError::solver_failed;
            return false;
        }
        if (!add_incidence(scratch, 1))
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return false;
        }
    }
    if (events.size() > event_capacity || !charge(sort_units(events.size()) + events.size()))
    {
        output.error = EvaluationError::resource_limit_exceeded;
        return false;
    }
    std::sort(events.begin(), events.end(),
              [](const auto& left, const auto& right) { return left.pair < right.pair; });
    std::size_t output_pair_count = 0;
    for (std::size_t begin = 0; begin < events.size();)
    {
        std::size_t end = begin + 1;
        std::uint8_t dimension = events[begin].dimension;
        while (end < events.size() && events[end].pair == events[begin].pair)
        {
            dimension = std::max(dimension, events[end].dimension);
            ++end;
        }
        output_pair_count += dimension != 0;
        begin = end;
    }
    output.value.pairs.reserve(output_pair_count);
    std::uint64_t output_build_work = 0;
    if (!checked_multiply(output_pair_count, 4, output_build_work) ||
        !checked_add(output_build_work, events.size(), output_build_work) ||
        !charge(output_build_work))
    {
        output.error = EvaluationError::resource_limit_exceeded;
        return false;
    }
    for (std::size_t begin = 0; begin < events.size();)
    {
        std::size_t end = begin + 1;
        PairValue value;
        while (end < events.size() && events[end].pair == events[begin].pair)
            ++end;
        for (std::size_t index = begin; index < end; ++index)
        {
            value.dimension = std::max(value.dimension, events[index].dimension);
            value.equality = value.equality || events[index].equality;
            value.left_contains_right =
                value.left_contains_right || events[index].left_contains_right;
            value.right_contains_left =
                value.right_contains_left || events[index].right_contains_left;
        }
        value.equality = value.equality || (value.left_contains_right && value.right_contains_left);
        const RegionPair pair = events[begin].pair;
        begin = end;
        if (value.dimension == 0)
            continue;
        const std::uint32_t left_region =
            pair.first < built.left_operand_count
                ? left_job.result_region_begin + pair.first
                : right_job.result_region_begin + pair.first - built.left_operand_count;
        const std::uint32_t right_region =
            self ? left_job.result_region_begin + pair.second
                 : right_job.result_region_begin + pair.second - built.left_operand_count;
        output.value.pairs.push_back(
            {records.regions[left_region].id, records.regions[right_region].id, value.dimension,
             value.equality, value.left_contains_right, value.right_contains_left});
        output.value.aggregate = std::max(output.value.aggregate, value.dimension);
    }
    if (output.value.pairs.size() != output_pair_count ||
        !charge(sort_units(output.value.pairs.size())))
    {
        output.error = EvaluationError::resource_limit_exceeded;
        return false;
    }
    std::sort(output.value.pairs.begin(), output.value.pairs.end(),
              [](const auto& left, const auto& right)
              {
                  return std::tie(left.left_result_region_id, left.right_result_region_id,
                                  left.dimension, left.equality, left.left_contains_right,
                                  left.right_contains_left) <
                         std::tie(right.left_result_region_id, right.right_result_region_id,
                                  right.dimension, right.equality, right.left_contains_right,
                                  right.right_contains_left);
              });
    return true;
}

struct PreparedPairSelection
{
    PairBuild output;
    GeometryBuild built;
    std::vector<AnalyticCurvePair> candidates;
    AnalyticFilteredBooleanSelectionResult selection;
    std::uint64_t candidate_bytes = 0;
    bool ready = false;
};

PreparedPairSelection prepare_pair_selection(const AnalyticResultPacketRecords& records,
                                             const std::vector<std::uint32_t>& region_ring_offsets,
                                             const std::vector<std::uint32_t>& region_rings,
                                             const AnalyticJobResultRecord& left_job,
                                             const AnalyticJobResultRecord& right_job, bool self,
                                             const AnalyticSolverLimits& supplied_limits,
                                             std::uint64_t work_limit, std::uint64_t memory_limit,
                                             std::uint64_t retained_memory)
{
    PreparedPairSelection prepared;
    PairBuild& output = prepared.output;
    std::uint64_t expected_curves = 0;
    std::uint64_t expected_lines = 0;
    std::uint64_t expected_arcs = 0;
    std::uint64_t preflight_work = 0;
    bool invalid_fragment = false;
    const auto admit_preflight_work = [&](std::uint64_t amount)
    { return checked_add(preflight_work, amount, preflight_work) && preflight_work <= work_limit; };
    const auto count_job_fragments = [&](const AnalyticJobResultRecord& job)
    {
        const std::uint32_t end = job.result_region_begin + job.result_region_count;
        if (!admit_preflight_work(job.result_region_count))
            return false;
        for (std::uint32_t region = job.result_region_begin; region < end; ++region)
        {
            const std::uint64_t ring_count =
                region_ring_offsets[region + 1] - region_ring_offsets[region];
            if (!admit_preflight_work(ring_count))
                return false;
            for (std::uint32_t at = region_ring_offsets[region];
                 at < region_ring_offsets[region + 1]; ++at)
            {
                const auto& ring = records.rings[region_rings[at]];
                const std::uint64_t count = ring.fragment_reference_count;
                if (!admit_preflight_work(count) ||
                    !checked_add(expected_curves, count, expected_curves))
                    return false;
                for (std::uint32_t offset = 0; offset < ring.fragment_reference_count; ++offset)
                {
                    const std::uint32_t fragment =
                        records.fragment_references[ring.fragment_reference_begin + offset];
                    if (fragment >= records.fragments.size())
                    {
                        invalid_fragment = true;
                        return false;
                    }
                    if (records.fragments[fragment].kind == 1)
                        ++expected_lines;
                    else if (records.fragments[fragment].kind == 2)
                        ++expected_arcs;
                }
            }
        }
        return true;
    };
    if (!count_job_fragments(left_job) || (!self && !count_job_fragments(right_job)) ||
        expected_curves > std::numeric_limits<std::uint32_t>::max())
    {
        output.error = invalid_fragment ? EvaluationError::solver_failed
                                        : EvaluationError::resource_limit_exceeded;
        return prepared;
    }
    const std::uint64_t expected_regions =
        static_cast<std::uint64_t>(left_job.result_region_count) +
        (self ? 0 : right_job.result_region_count);
    std::uint64_t expected_retained = 0;
    std::uint64_t expected_peak = 0;
    std::uint64_t expected_term = 0;
    std::uint64_t line_scratch = 0;
    std::uint64_t arc_scratch = 0;
    std::uint64_t carrier_sort_work = 0;
    std::uint64_t carrier_term = 0;
    if (!checked_multiply(expected_curves, kGeometryCurveBytes, expected_retained) ||
        !checked_multiply(expected_regions, kGeometryOperandBytes, expected_term) ||
        !checked_add(expected_retained, expected_term, expected_retained) ||
        !checked_add(expected_retained, kGeometryFixedBytes, expected_retained) ||
        !checked_multiply(expected_lines, kLineCarrierScratchBytes, line_scratch) ||
        !checked_multiply(expected_arcs, kArcCarrierScratchBytes, arc_scratch) ||
        !checked_add(expected_retained, std::max(line_scratch, arc_scratch), expected_peak) ||
        !checked_multiply(expected_lines, 3, carrier_sort_work) ||
        !checked_add(carrier_sort_work, sort_units(expected_lines), carrier_sort_work) ||
        !checked_multiply(expected_arcs, 3, carrier_term) ||
        !checked_add(carrier_sort_work, carrier_term, carrier_sort_work) ||
        !checked_add(carrier_sort_work, sort_units(expected_arcs), carrier_sort_work) ||
        !checked_multiply(expected_curves, 4, carrier_term) ||
        !checked_add(carrier_sort_work, carrier_term, carrier_sort_work) ||
        !checked_multiply(expected_arcs, 2, carrier_term) ||
        !checked_add(carrier_sort_work, sort_units(carrier_term), carrier_sort_work) ||
        !checked_add(preflight_work, preflight_work, carrier_term) ||
        !checked_add(carrier_term, carrier_sort_work, carrier_term) ||
        !checked_multiply(expected_curves, 5, carrier_sort_work) ||
        !checked_add(carrier_term, carrier_sort_work, carrier_term) ||
        !checked_multiply(expected_regions, 3, carrier_sort_work) ||
        !checked_add(carrier_term, carrier_sort_work, carrier_term) || carrier_term > work_limit ||
        retained_memory > memory_limit || expected_peak > memory_limit - retained_memory)
    {
        output.error = EvaluationError::resource_limit_exceeded;
        output.required_memory = retained_memory + expected_peak;
        return prepared;
    }
    GeometryBuild built =
        build_geometry(records, region_ring_offsets, region_rings, left_job, right_job, self);
    if (!checked_add(preflight_work, built.work, output.work))
    {
        output.error = EvaluationError::resource_limit_exceeded;
        return prepared;
    }
    output.peak_memory = built.peak_memory;
    if (built.error != EvaluationError::none)
    {
        output.error = built.error;
        return prepared;
    }
    if (built.geometry.curves.empty())
        return prepared;
    if (output.work > work_limit || built.memory > memory_limit ||
        retained_memory > memory_limit - built.memory)
    {
        output.error = EvaluationError::resource_limit_exceeded;
        output.required_memory = retained_memory + built.memory;
        return prepared;
    }

    AnalyticSolverLimits limits = supplied_limits;
    limits.predicate_calls = std::min(limits.predicate_calls, work_limit - output.work);
    limits.working_memory_bytes =
        std::min(limits.working_memory_bytes, memory_limit - retained_memory - built.memory);
    AnalyticBroadPhaseResult broad =
        self ? analytic_execution_detail::build_curve_candidates(
                   built.geometry.bounds, limits,
                   analytic_execution_detail::kStrictPublishedGeometry)
             : analytic_execution_detail::build_bipartite_curve_candidates(
                   built.geometry.bounds, built.left_curve_count, limits,
                   analytic_execution_detail::kStrictPublishedGeometry);
    if (!checked_add(output.work, broad.telemetry.work_units, output.work) ||
        !checked_add(output.algebraic_fallback_calls, broad.telemetry.algebraic_fallback_calls,
                     output.algebraic_fallback_calls))
    {
        output.error = EvaluationError::resource_limit_exceeded;
        return prepared;
    }
    output.candidates = broad.telemetry.candidate_pairs;
    output.peak_memory =
        std::max(output.peak_memory, built.memory + broad.telemetry.peak_working_memory_bytes);
    if (broad.error != AnalyticBroadPhaseError::none)
    {
        output.error = broad.error == AnalyticBroadPhaseError::resource_limit_exceeded
                           ? EvaluationError::resource_limit_exceeded
                           : EvaluationError::solver_failed;
        output.required_memory =
            retained_memory + built.memory + broad.telemetry.required_working_memory_bytes;
        return prepared;
    }

    std::vector<AnalyticCurvePair> candidates = std::move(broad.pairs);
    output.candidates = candidates.size();
    if (candidates.size() > limits.examined_curve_pairs)
    {
        output.error = EvaluationError::resource_limit_exceeded;
        return prepared;
    }
    std::uint64_t minimum_candidate_bytes = 0;
    if (!checked_multiply(candidates.size(), kCandidatePairBytes, minimum_candidate_bytes) ||
        broad.telemetry.retained_pair_bytes < minimum_candidate_bytes)
    {
        output.error = EvaluationError::solver_failed;
        return prepared;
    }
    const std::uint64_t candidate_bytes = broad.telemetry.retained_pair_bytes;
    output.peak_memory = std::max(output.peak_memory, built.memory + candidate_bytes);

    if (output.work > work_limit)
    {
        output.error = EvaluationError::resource_limit_exceeded;
        return prepared;
    }
    limits.predicate_calls = std::min(supplied_limits.predicate_calls, work_limit - output.work);
    if (built.memory > memory_limit - retained_memory ||
        candidate_bytes > memory_limit - retained_memory - built.memory)
    {
        output.error = EvaluationError::resource_limit_exceeded;
        output.required_memory = retained_memory + built.memory + candidate_bytes;
        return prepared;
    }
    limits.working_memory_bytes =
        std::min(supplied_limits.working_memory_bytes,
                 memory_limit - retained_memory - built.memory - candidate_bytes);
    AnalyticFilteredBooleanSelectionResult selection =
        analytic_execution_detail::build_boolean_selection(
            built.request, 0, built.geometry, candidates, limits,
            analytic_execution_detail::kStrictPublishedGeometry);
    if (!checked_add(output.work, selection.telemetry.predicate_calls, output.work) ||
        !checked_add(output.algebraic_fallback_calls, selection.telemetry.algebraic_fallback_calls,
                     output.algebraic_fallback_calls))
    {
        output.error = EvaluationError::resource_limit_exceeded;
        return prepared;
    }
    output.peak_memory =
        std::max(output.peak_memory,
                 built.memory + candidate_bytes + selection.telemetry.peak_working_memory_bytes);
    if (selection.error != AnalyticFilteredBooleanSelectionError::none)
    {
        output.unresolved_predicate_failure = selection.telemetry.unresolved_predicate_failure;
        output.error = selection.error == AnalyticFilteredBooleanSelectionError::invalid_argument ||
                               selection.telemetry.unresolved_predicate_failure
                           ? EvaluationError::solver_failed
                           : EvaluationError::resource_limit_exceeded;
        output.required_memory = retained_memory + built.memory + candidate_bytes +
                                 selection.telemetry.required_working_memory_bytes;
        return prepared;
    }
    if (output.work > work_limit)
    {
        output.error = EvaluationError::resource_limit_exceeded;
        return prepared;
    }
    prepared.built = std::move(built);
    prepared.candidates = std::move(candidates);
    prepared.selection = std::move(selection);
    prepared.candidate_bytes = candidate_bytes;
    prepared.ready = true;
    return prepared;
}

PairBuild evaluate_pair(const AnalyticResultPacketRecords& records,
                        const std::vector<std::uint32_t>& region_ring_offsets,
                        const std::vector<std::uint32_t>& region_rings,
                        const AnalyticJobResultRecord& left_job,
                        const AnalyticJobResultRecord& right_job, bool self,
                        const AnalyticSolverLimits& supplied_limits, std::uint64_t work_limit,
                        std::uint64_t memory_limit, std::uint64_t retained_memory)
{
    PreparedPairSelection prepared =
        prepare_pair_selection(records, region_ring_offsets, region_rings, left_job, right_job,
                               self, supplied_limits, work_limit, memory_limit, retained_memory);
    if (!prepared.ready)
        return std::move(prepared.output);
    PairBuild output = std::move(prepared.output);
    GeometryBuild built = std::move(prepared.built);
    std::vector<AnalyticCurvePair> candidates = std::move(prepared.candidates);
    AnalyticFilteredBooleanSelectionResult selection = std::move(prepared.selection);
    const std::uint64_t candidate_bytes = prepared.candidate_bytes;

    const std::uint32_t operand_count = static_cast<std::uint32_t>(built.request.operands.size());
    const auto charge = [&](std::uint64_t units)
    {
        std::uint64_t next = 0;
        if (!checked_add(output.work, units, next) || next > work_limit)
            return false;
        output.work = next;
        return true;
    };
    const std::uint64_t base_live =
        built.memory + candidate_bytes + selection.telemetry.peak_working_memory_bytes;
    std::uint64_t fixed_classification_bytes = 0;
    std::uint64_t term = 0;
    if (!checked_multiply(operand_count, 14, fixed_classification_bytes) ||
        !checked_multiply(selection.arrangement.edges.size() + 1, kIndexBytes, term) ||
        !checked_add(fixed_classification_bytes, term, fixed_classification_bytes) ||
        !checked_multiply(selection.arrangement.memberships.size(), kIndexBytes, term) ||
        !checked_add(fixed_classification_bytes, term, fixed_classification_bytes) ||
        retained_memory > memory_limit || base_live > memory_limit - retained_memory ||
        fixed_classification_bytes > memory_limit - retained_memory - base_live)
    {
        output.error = EvaluationError::resource_limit_exceeded;
        output.required_memory = retained_memory + base_live + fixed_classification_bytes;
        return output;
    }
    output.peak_memory = std::max(output.peak_memory, base_live + fixed_classification_bytes);

    std::vector<bool> saw_face(operand_count);
    std::vector<bool> fully_covered(operand_count, true);
    std::vector<std::uint32_t> unique_cover(operand_count,
                                            std::numeric_limits<std::uint32_t>::max());
    std::vector<std::uint32_t> scratch;
    scratch.reserve(operand_count);
    std::vector<std::uint32_t> incidence_marks(operand_count);
    std::vector<std::uint32_t> edge_offsets(selection.arrangement.edges.size() + 1);
    std::vector<std::uint32_t> edge_owners;
    edge_owners.reserve(selection.arrangement.memberships.size());
    for (std::uint32_t edge = 0; edge < selection.arrangement.edges.size(); ++edge)
    {
        edge_offsets[edge] = static_cast<std::uint32_t>(edge_owners.size());
        const auto& value = selection.arrangement.edges[edge];
        if (!charge(value.membership_count))
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        for (std::uint32_t offset = 0; offset < value.membership_count; ++offset)
        {
            const auto& membership =
                selection.arrangement.memberships[value.membership_begin + offset];
            if (membership.curve_index != 0 &&
                membership.curve_index <= built.curve_operands.size())
                edge_owners.push_back(built.curve_operands[membership.curve_index - 1]);
        }
        auto begin = edge_owners.begin() + edge_offsets[edge];
        if (!charge(sort_units(edge_owners.size() - edge_offsets[edge])))
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        std::sort(begin, edge_owners.end());
        edge_owners.erase(std::unique(begin, edge_owners.end()), edge_owners.end());
    }
    edge_offsets.back() = static_cast<std::uint32_t>(edge_owners.size());

    std::uint32_t incidence_generation = 0;
    bool vertex_collection_resource_failure = false;
    const auto charge_vertex_collection = [&](std::uint64_t units)
    {
        if (charge(units))
            return true;
        vertex_collection_resource_failure = true;
        return false;
    };
    const auto collect_vertex_operands = [&](const auto& vertex)
    {
        if (!charge_vertex_collection(vertex.outgoing_count))
            return false;
        if (incidence_generation == std::numeric_limits<std::uint32_t>::max())
        {
            if (!charge_vertex_collection(incidence_marks.size()))
                return false;
            std::fill(incidence_marks.begin(), incidence_marks.end(), 0);
            incidence_generation = 1;
        }
        else
            ++incidence_generation;
        scratch.clear();
        for (std::uint32_t offset = 0; offset < vertex.outgoing_count; ++offset)
        {
            const std::uint32_t half_edge =
                selection.arrangement.outgoing_half_edges[vertex.outgoing_begin + offset];
            if (half_edge >= selection.arrangement.half_edges.size())
                return false;
            const std::uint32_t edge = selection.arrangement.half_edges[half_edge].edge;
            if (edge >= selection.arrangement.edges.size())
                return false;
            const std::uint32_t begin = edge_offsets[edge];
            const std::uint32_t end = edge_offsets[edge + 1];
            if (!charge_vertex_collection(end - begin))
                return false;
            for (std::uint32_t at = begin; at < end; ++at)
            {
                const std::uint32_t operand = edge_owners[at];
                if (incidence_marks[operand] == incidence_generation)
                    continue;
                incidence_marks[operand] = incidence_generation;
                scratch.push_back(operand);
            }
        }
        if (!charge_vertex_collection(sort_units(scratch.size())))
            return false;
        std::sort(scratch.begin(), scratch.end());
        return true;
    };

    const auto incidence_count =
        [&](const std::vector<std::uint32_t>& operands, std::uint64_t& count)
    {
        if (self)
            return checked_multiply(operands.size(), operands.size() - (operands.empty() ? 0 : 1),
                                    count);
        const auto split =
            std::lower_bound(operands.begin(), operands.end(), built.left_operand_count);
        return checked_multiply(static_cast<std::uint64_t>(split - operands.begin()),
                                static_cast<std::uint64_t>(operands.end() - split), count);
    };
    const auto append_count = [&](std::uint64_t& total, std::uint64_t amount)
    { return checked_add(total, amount, total); };

    std::uint64_t event_capacity = self ? operand_count : 0;
    std::uint32_t coverage_capacity = 1;
    while (coverage_capacity < std::max<std::uint32_t>(1, operand_count))
        coverage_capacity <<= 1U;
    for (const auto& face : selection.faces)
    {
        if (face.unbounded)
            continue;
        scratch.clear();
        bool coverage_resource_failure = false;
        if (!collect_coverage(selection, face.coverage_state_root, 0, coverage_capacity, scratch,
                              output.work, work_limit, coverage_resource_failure))
        {
            output.error = coverage_resource_failure ? EvaluationError::resource_limit_exceeded
                                                     : EvaluationError::solver_failed;
            return output;
        }
        if (!charge(sort_units(scratch.size()) + scratch.size() * 2))
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        std::sort(scratch.begin(), scratch.end());
        scratch.erase(std::unique(scratch.begin(), scratch.end()), scratch.end());
        for (const std::uint32_t operand : scratch)
        {
            saw_face[operand] = true;
            std::optional<std::uint32_t> other;
            for (const std::uint32_t candidate : scratch)
            {
                const bool opposite = self ? candidate != operand
                                           : (operand < built.left_operand_count) !=
                                                 (candidate < built.left_operand_count);
                if (!opposite)
                    continue;
                if (other)
                {
                    output.error = EvaluationError::solver_failed;
                    return output;
                }
                other = candidate;
            }
            if (!other)
                fully_covered[operand] = false;
            else if (unique_cover[operand] == std::numeric_limits<std::uint32_t>::max())
                unique_cover[operand] = *other;
            else if (unique_cover[operand] != *other)
                fully_covered[operand] = false;
        }
        if (!self)
        {
            std::uint64_t count = 0;
            if (!incidence_count(scratch, count) || !append_count(event_capacity, count))
            {
                output.error = EvaluationError::resource_limit_exceeded;
                return output;
            }
        }
    }
    for (std::uint32_t operand = 0; operand < operand_count; ++operand)
        if (!self && saw_face[operand] && fully_covered[operand] &&
            unique_cover[operand] != std::numeric_limits<std::uint32_t>::max() &&
            !append_count(event_capacity, 1))
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
    for (std::uint32_t edge = 0; edge < selection.arrangement.edges.size(); ++edge)
    {
        const std::uint64_t owner_count = edge_offsets[edge + 1] - edge_offsets[edge];
        if (!charge(owner_count + 1))
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        scratch.assign(edge_owners.begin() + edge_offsets[edge],
                       edge_owners.begin() + edge_offsets[edge + 1]);
        std::uint64_t count = 0;
        if (!incidence_count(scratch, count) || !append_count(event_capacity, count))
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
    }
    for (const auto& vertex : selection.arrangement.vertices)
    {
        if (!collect_vertex_operands(vertex))
        {
            output.error = vertex_collection_resource_failure
                               ? EvaluationError::resource_limit_exceeded
                               : EvaluationError::solver_failed;
            return output;
        }
        std::uint64_t count = 0;
        if (!charge(1) || !incidence_count(scratch, count) || !append_count(event_capacity, count))
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
    }

    const RelationEmissionInput emission{
        records,
        left_job,
        right_job,
        built,
        selection,
        saw_face,
        fully_covered,
        unique_cover,
        edge_offsets,
        edge_owners,
        scratch,
        self,
        vertex_collection_resource_failure,
        event_capacity,
        fixed_classification_bytes,
        memory_limit,
        work_limit,
        retained_memory,
        base_live,
        operand_count,
        coverage_capacity,
    };
    if (!emit_relation_events(output, emission, charge, incidence_count, collect_vertex_operands))
        return output;
    return output;
}

} // namespace

EvaluationResult evaluate(const AnalyticRequestPacketRecords& request,
                          const AnalyticResultPacketRecords& published,
                          const AnalyticSolverLimits& per_pair_limits, std::uint64_t work_limit,
                          std::uint64_t working_memory_limit, std::uint64_t retained_memory_bytes,
                          std::uint64_t maximum_packet_bytes)
{
    EvaluationResult output;
    if (request.relationship_queries.empty())
        return output;
    try
    {
        if (published.job_results.size() != request.jobs.size() ||
            !published.relationship_results.empty() || !published.relationship_pairs.empty())
        {
            output.error = EvaluationError::invalid_argument;
            return output;
        }
        std::uint64_t job_search_work = 0;
        std::uint64_t job_search_term = 0;
        if (!checked_multiply(ceil_log2(published.job_results.size()) + 1, 4, job_search_term) ||
            !checked_multiply(request.relationship_queries.size(), job_search_term,
                              job_search_work) ||
            job_search_work > work_limit)
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        output.telemetry.work_units = job_search_work;
        const auto find_job_index = [&](std::uint64_t id) -> std::optional<std::uint32_t>
        {
            const auto found =
                std::lower_bound(published.job_results.begin(), published.job_results.end(), id,
                                 [](const AnalyticJobResultRecord& job, std::uint64_t value)
                                 { return job.job_id < value; });
            if (found == published.job_results.end() || found->job_id != id)
                return std::nullopt;
            return static_cast<std::uint32_t>(found - published.job_results.begin());
        };
        std::uint64_t successful_queries = 0;
        for (const auto& query : request.relationship_queries)
        {
            const auto left = find_job_index(query.left_job_id);
            const auto right = find_job_index(query.right_job_id);
            if (!left || !right)
            {
                output.error = EvaluationError::invalid_argument;
                return output;
            }
            successful_queries += published.job_results[*left].status == 0 &&
                                  published.job_results[*right].status == 0;
        }
        std::uint64_t index_entries = 0;
        std::uint64_t index_bytes = 0;
        std::uint64_t preadmitted_bytes = 0;
        std::uint64_t preadmitted_term = 0;
        if (!checked_add(published.rings.size() * 2, published.regions.size() * 2 + 2,
                         index_entries) ||
            !checked_multiply(index_entries, kIndexBytes, index_bytes) ||
            !checked_add(preadmitted_bytes, index_bytes, preadmitted_bytes) ||
            !checked_multiply(request.relationship_queries.size(), kQueryPlanBytes,
                              preadmitted_term) ||
            !checked_add(preadmitted_bytes, preadmitted_term, preadmitted_bytes) ||
            !checked_multiply(successful_queries, kQueryKeyBytes, preadmitted_term) ||
            !checked_add(preadmitted_bytes, preadmitted_term, preadmitted_bytes) ||
            !checked_multiply(successful_queries, kCacheEntryBytes, preadmitted_term) ||
            !checked_add(preadmitted_bytes, preadmitted_term, preadmitted_bytes) ||
            !checked_multiply(request.relationship_queries.size(), kRelationshipRowBytes,
                              preadmitted_term) ||
            !checked_add(preadmitted_bytes, preadmitted_term, preadmitted_bytes) ||
            retained_memory_bytes > working_memory_limit ||
            preadmitted_bytes > working_memory_limit - retained_memory_bytes)
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        std::uint64_t ring_index_work = sort_units(published.rings.size());
        std::uint64_t ring_index_term = 0;
        if (!checked_multiply(published.rings.size(), 4, ring_index_term) ||
            !checked_add(ring_index_work, ring_index_term, ring_index_work) ||
            !checked_multiply(published.regions.size(), 2, ring_index_term) ||
            !checked_add(ring_index_work, ring_index_term, ring_index_work) ||
            !checked_add(output.telemetry.work_units, ring_index_work,
                         output.telemetry.work_units) ||
            output.telemetry.work_units > work_limit)
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        output.telemetry.peak_working_memory_bytes = retained_memory_bytes + index_bytes;
        const std::vector<std::uint32_t> ring_owners = ring_region_owners(published);
        if (ring_owners.size() != published.rings.size())
        {
            output.error = EvaluationError::invalid_argument;
            return output;
        }
        std::vector<std::uint32_t> region_ring_offsets(published.regions.size() + 1);
        for (const std::uint32_t owner : ring_owners)
        {
            if (owner >= published.regions.size())
            {
                output.error = EvaluationError::invalid_argument;
                return output;
            }
            ++region_ring_offsets[owner + 1];
        }
        for (std::size_t index = 1; index < region_ring_offsets.size(); ++index)
            region_ring_offsets[index] += region_ring_offsets[index - 1];
        std::vector<std::uint32_t> region_rings(published.rings.size());
        std::vector<std::uint32_t> ring_cursors = region_ring_offsets;
        for (std::uint32_t ring = 0; ring < ring_owners.size(); ++ring)
            region_rings[ring_cursors[ring_owners[ring]]++] = ring;
        const std::uint64_t maximum_relationship_rows =
            maximum_packet_bytes / kRelationshipRowBytes;
        if (request.relationship_queries.size() > maximum_relationship_rows)
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }

        std::vector<QueryPlan> plans;
        plans.reserve(request.relationship_queries.size());
        std::vector<std::pair<std::uint32_t, std::uint32_t>> unique_keys;
        unique_keys.reserve(static_cast<std::size_t>(successful_queries));
        for (const auto& query : request.relationship_queries)
        {
            const auto left = find_job_index(query.left_job_id);
            const auto right = find_job_index(query.right_job_id);
            if (!left || !right)
            {
                output.error = EvaluationError::invalid_argument;
                return output;
            }
            const auto& left_job = published.job_results[*left];
            const auto& right_job = published.job_results[*right];
            if (left_job.status != 0 || right_job.status != 0)
            {
                plans.push_back({*left, *right, std::numeric_limits<std::uint32_t>::max(), true});
                continue;
            }
            const auto key = std::pair{std::min(*left, *right), std::max(*left, *right)};
            plans.push_back({*left, *right, 0, false});
            unique_keys.push_back(key);
        }
        const std::uint64_t key_sort_work = sort_units(unique_keys.size()) + unique_keys.size() * 2;
        if (!checked_add(output.telemetry.work_units, key_sort_work, output.telemetry.work_units) ||
            output.telemetry.work_units > work_limit)
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        std::sort(unique_keys.begin(), unique_keys.end());
        unique_keys.erase(std::unique(unique_keys.begin(), unique_keys.end()), unique_keys.end());
        std::vector<CacheEntry> cache;
        cache.reserve(unique_keys.size());
        for (const auto key : unique_keys)
            cache.push_back({key.first, key.second, {}});
        std::uint64_t cache_lookup_work = 0;
        if (!checked_multiply(successful_queries, ceil_log2(unique_keys.size()) + 1,
                              cache_lookup_work) ||
            !checked_add(output.telemetry.work_units, cache_lookup_work,
                         output.telemetry.work_units) ||
            output.telemetry.work_units > work_limit)
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        for (QueryPlan& plan : plans)
            if (!plan.failed)
            {
                const auto key = std::pair{std::min(plan.left_job, plan.right_job),
                                           std::max(plan.left_job, plan.right_job)};
                plan.cache_index = static_cast<std::uint32_t>(
                    std::lower_bound(unique_keys.begin(), unique_keys.end(), key) -
                    unique_keys.begin());
            }

        std::uint64_t fixed_bytes = index_bytes;
        std::uint64_t term = 0;
        if (!checked_multiply(plans.size(), kQueryPlanBytes, term) ||
            !checked_add(fixed_bytes, term, fixed_bytes) ||
            !checked_multiply(successful_queries, kQueryKeyBytes, term) ||
            !checked_add(fixed_bytes, term, fixed_bytes) ||
            !checked_multiply(cache.size(), kCacheEntryBytes, term) ||
            !checked_add(fixed_bytes, term, fixed_bytes) ||
            !checked_multiply(request.relationship_queries.size(), kRelationshipRowBytes, term) ||
            !checked_add(fixed_bytes, term, fixed_bytes) ||
            retained_memory_bytes > working_memory_limit ||
            fixed_bytes > working_memory_limit - retained_memory_bytes)
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        output.telemetry.peak_working_memory_bytes = std::max(
            output.telemetry.peak_working_memory_bytes, retained_memory_bytes + fixed_bytes);

        std::uint64_t cached_pair_rows = 0;
        for (CacheEntry& entry : cache)
        {
            PairBuild built = evaluate_pair(
                published, region_ring_offsets, region_rings,
                published.job_results[entry.first_job], published.job_results[entry.second_job],
                entry.first_job == entry.second_job, per_pair_limits,
                work_limit - std::min(work_limit, output.telemetry.work_units),
                working_memory_limit,
                retained_memory_bytes + fixed_bytes + cached_pair_rows * kRelationshipRowBytes);
            if (!checked_add(output.telemetry.work_units, built.work,
                             output.telemetry.work_units) ||
                !checked_add(output.telemetry.candidate_pairs, built.candidates,
                             output.telemetry.candidate_pairs) ||
                !checked_add(output.telemetry.algebraic_fallback_calls,
                             built.algebraic_fallback_calls,
                             output.telemetry.algebraic_fallback_calls))
            {
                output.error = EvaluationError::resource_limit_exceeded;
                return output;
            }
            output.telemetry.peak_working_memory_bytes =
                std::max(output.telemetry.peak_working_memory_bytes,
                         retained_memory_bytes + fixed_bytes +
                             cached_pair_rows * kRelationshipRowBytes + built.peak_memory);
            output.telemetry.unresolved_predicate_failures +=
                built.unresolved_predicate_failure ? 1U : 0U;
            output.telemetry.required_working_memory_bytes =
                std::max(output.telemetry.required_working_memory_bytes, built.required_memory);
            if (built.error != EvaluationError::none)
            {
                output.error = built.error;
                return output;
            }
            entry.value = std::move(built.value);
            if (!checked_add(cached_pair_rows, entry.value.pairs.size(), cached_pair_rows) ||
                cached_pair_rows > std::numeric_limits<std::uint32_t>::max())
            {
                output.error = EvaluationError::resource_limit_exceeded;
                return output;
            }
        }

        std::uint64_t published_pair_rows = 0;
        for (const QueryPlan& plan : plans)
            if (!plan.failed &&
                !checked_add(published_pair_rows, cache[plan.cache_index].value.pairs.size(),
                             published_pair_rows))
            {
                output.error = EvaluationError::resource_limit_exceeded;
                return output;
            }
        std::uint64_t total_rows = 0;
        if (!checked_add(plans.size(), published_pair_rows, total_rows) ||
            total_rows > maximum_relationship_rows ||
            published_pair_rows > std::numeric_limits<std::uint32_t>::max())
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        std::uint64_t live_rows = 0;
        if (!checked_add(cached_pair_rows, published_pair_rows, live_rows) ||
            !checked_multiply(live_rows, kRelationshipRowBytes, term) ||
            fixed_bytes > working_memory_limit - retained_memory_bytes ||
            term > working_memory_limit - retained_memory_bytes - fixed_bytes)
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        output.telemetry.peak_working_memory_bytes = std::max(
            output.telemetry.peak_working_memory_bytes, retained_memory_bytes + fixed_bytes + term);
        std::uint64_t publication_work = 0;
        for (const QueryPlan& plan : plans)
        {
            std::uint64_t query_work = 1;
            if (!plan.failed)
            {
                const CacheEntry& entry = cache[plan.cache_index];
                const bool forward = plan.left_job == entry.first_job;
                std::uint64_t pair_work = 0;
                if (!checked_multiply(entry.value.pairs.size(),
                                      forward ? 1 : ceil_log2(entry.value.pairs.size()) + 2,
                                      pair_work) ||
                    !checked_add(query_work, pair_work, query_work))
                {
                    output.error = EvaluationError::resource_limit_exceeded;
                    return output;
                }
            }
            if (!checked_add(publication_work, query_work, publication_work))
            {
                output.error = EvaluationError::resource_limit_exceeded;
                return output;
            }
        }
        if (!checked_add(output.telemetry.work_units, publication_work,
                         output.telemetry.work_units) ||
            output.telemetry.work_units > work_limit)
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        output.results.reserve(plans.size());
        output.pairs.reserve(static_cast<std::size_t>(published_pair_rows));
        for (std::size_t query_index = 0; query_index < plans.size(); ++query_index)
        {
            const QueryPlan& plan = plans[query_index];
            const auto& query = request.relationship_queries[query_index];
            if (plan.failed)
            {
                output.results.push_back({query.query_id, 1, 0, 0, 0});
                continue;
            }
            const CacheEntry& entry = cache[plan.cache_index];
            const bool forward = plan.left_job == entry.first_job;
            const std::uint32_t begin =
                entry.value.pairs.empty() ? 0U : static_cast<std::uint32_t>(output.pairs.size());
            output.results.push_back({query.query_id, 0, entry.value.aggregate, begin,
                                      static_cast<std::uint32_t>(entry.value.pairs.size())});
            for (const auto& pair : entry.value.pairs)
                output.pairs.push_back(forward ? pair
                                               : AnalyticRelationshipPairRecord{
                                                     pair.right_result_region_id,
                                                     pair.left_result_region_id, pair.dimension,
                                                     pair.equality, pair.right_contains_left,
                                                     pair.left_contains_right});
            if (!forward)
                std::sort(
                    output.pairs.begin() + begin, output.pairs.end(),
                    [](const auto& left, const auto& right)
                    {
                        return std::tie(left.left_result_region_id, left.right_result_region_id,
                                        left.dimension, left.equality, left.left_contains_right,
                                        left.right_contains_left) <
                               std::tie(right.left_result_region_id, right.right_result_region_id,
                                        right.dimension, right.equality, right.left_contains_right,
                                        right.right_contains_left);
                    });
        }
        std::uint64_t row_count = 0;
        std::uint64_t row_bytes = 0;
        if (!checked_add(output.results.size(), output.pairs.size(), row_count) ||
            !checked_multiply(row_count, kRelationshipRowBytes, row_bytes))
        {
            output.error = EvaluationError::resource_limit_exceeded;
            output.results.clear();
            output.pairs.clear();
            return output;
        }
        if (output.telemetry.work_units > work_limit)
        {
            output.error = EvaluationError::resource_limit_exceeded;
            output.results.clear();
            output.pairs.clear();
        }
    }
    catch (const std::bad_alloc&)
    {
        output.error = EvaluationError::resource_limit_exceeded;
        output.results.clear();
        output.pairs.clear();
    }
    return output;
}

} // namespace geometer::analytic_relationship_detail

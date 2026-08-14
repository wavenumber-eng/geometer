#include "exact_arrangement_internal.h"

#include <algorithm>
#include <exception>
#include <limits>
#include <utility>

namespace geometer::exact::arrangement_detail
{
namespace
{

constexpr std::uint64_t kMaximumFaceCoverageMemberships = 8'388'608;

struct Mapping
{
    ExactCoverageOccurrence value;
    bool used = false;
};

struct Transition
{
    std::uint64_t coverage_id = 0;
    bool expected_left = false;
};

std::vector<std::uint64_t> toggle(const std::vector<std::uint64_t>& state,
                                  const std::vector<Transition>& transitions)
{
    std::vector<std::uint64_t> result;
    result.reserve(state.size() + transitions.size());
    std::size_t state_index = 0;
    std::size_t transition_index = 0;
    while (state_index < state.size() || transition_index < transitions.size())
    {
        if (transition_index == transitions.size() ||
            (state_index < state.size() &&
             state[state_index] < transitions[transition_index].coverage_id))
        {
            result.push_back(state[state_index++]);
            continue;
        }
        if (state_index == state.size() ||
            transitions[transition_index].coverage_id < state[state_index])
        {
            result.push_back(transitions[transition_index++].coverage_id);
            continue;
        }
        ++state_index;
        ++transition_index;
    }
    return result;
}

bool contains(const std::vector<std::uint64_t>& values, std::uint64_t value)
{
    return std::binary_search(values.begin(), values.end(), value);
}

} // namespace

Error classify_face_coverages(const std::vector<ExactCoverageOccurrence>& coverage_occurrences,
                              const std::vector<ExactArrangementEdge>& edges,
                              const std::vector<ExactCurveMembership>& memberships,
                              const std::vector<ExactArrangementHalfEdge>& half_edges,
                              std::vector<ExactArrangementFace>& faces,
                              std::vector<std::uint64_t>& face_coverages)
{
    try
    {
        if (coverage_occurrences.empty())
            return Error::none;
        std::vector<Mapping> mappings;
        mappings.reserve(coverage_occurrences.size());
        for (const ExactCoverageOccurrence& occurrence : coverage_occurrences)
        {
            if (occurrence.occurrence_id == 0 || occurrence.coverage_id == 0)
                return Error::invalid_argument;
            mappings.push_back({occurrence, false});
        }
        std::sort(mappings.begin(), mappings.end(), [](const Mapping& left, const Mapping& right)
                  { return left.value.occurrence_id < right.value.occurrence_id; });
        for (std::size_t index = 1; index < mappings.size(); ++index)
            if (mappings[index - 1].value.occurrence_id == mappings[index].value.occurrence_id)
                return Error::invalid_argument;

        std::vector<std::vector<Transition>> transitions(edges.size());
        for (std::size_t edge_index = 0; edge_index < edges.size(); ++edge_index)
        {
            const ExactArrangementEdge& edge = edges[edge_index];
            auto& edge_transitions = transitions[edge_index];
            for (std::uint32_t membership_index = 0; membership_index < edge.membership_count;
                 ++membership_index)
            {
                const ExactCurveMembership& membership =
                    memberships[edge.membership_begin + membership_index];
                auto mapping =
                    std::lower_bound(mappings.begin(), mappings.end(), membership.occurrence_id,
                                     [](const Mapping& candidate, std::uint64_t occurrence_id)
                                     { return candidate.value.occurrence_id < occurrence_id; });
                if (mapping == mappings.end() ||
                    mapping->value.occurrence_id != membership.occurrence_id)
                    return Error::invalid_argument;
                mapping->used = true;
                const bool edge_agrees_with_carrier =
                    edge.kind == ExactAtomicCurveKind::line || edge.counterclockwise;
                const bool occurrence_agrees_with_edge =
                    membership.agrees_with_carrier == edge_agrees_with_carrier;
                const bool expected_left = occurrence_agrees_with_edge
                                               ? mapping->value.material_on_left_of_occurrence
                                               : !mapping->value.material_on_left_of_occurrence;
                edge_transitions.push_back({mapping->value.coverage_id, expected_left});
            }
            std::sort(edge_transitions.begin(), edge_transitions.end(),
                      [](const Transition& left, const Transition& right)
                      {
                          return left.coverage_id != right.coverage_id
                                     ? left.coverage_id < right.coverage_id
                                     : left.expected_left < right.expected_left;
                      });
            std::size_t output = 0;
            for (const Transition transition : edge_transitions)
            {
                if (output != 0 &&
                    edge_transitions[output - 1].coverage_id == transition.coverage_id)
                {
                    if (edge_transitions[output - 1].expected_left != transition.expected_left)
                        return Error::invalid_argument;
                    continue;
                }
                edge_transitions[output++] = transition;
            }
            edge_transitions.resize(output);
        }
        if (std::find_if(mappings.begin(), mappings.end(),
                         [](const Mapping& mapping) { return !mapping.used; }) != mappings.end())
            return Error::invalid_argument;

        std::vector<std::uint32_t> forward(edges.size(), std::numeric_limits<std::uint32_t>::max());
        std::vector<std::uint32_t> reverse(edges.size(), std::numeric_limits<std::uint32_t>::max());
        for (std::uint32_t half_edge_id = 0; half_edge_id < half_edges.size(); ++half_edge_id)
        {
            const ExactArrangementHalfEdge& half_edge = half_edges[half_edge_id];
            std::uint32_t& slot =
                half_edge.forward ? forward[half_edge.edge] : reverse[half_edge.edge];
            if (slot != std::numeric_limits<std::uint32_t>::max())
                return Error::invalid_argument;
            slot = half_edge_id;
        }

        struct Adjacency
        {
            std::uint32_t neighbor = 0;
            std::uint32_t edge = 0;
        };
        std::vector<std::vector<Adjacency>> adjacency(faces.size());
        for (std::uint32_t edge = 0; edge < edges.size(); ++edge)
        {
            if (forward[edge] == std::numeric_limits<std::uint32_t>::max() ||
                reverse[edge] == std::numeric_limits<std::uint32_t>::max())
                return Error::invalid_argument;
            const std::uint32_t left = half_edges[forward[edge]].face;
            const std::uint32_t right = half_edges[reverse[edge]].face;
            if (left >= faces.size() || right >= faces.size() || left == right)
                return Error::invalid_argument;
            adjacency[left].push_back({right, edge});
            adjacency[right].push_back({left, edge});
        }

        std::vector<std::vector<std::uint64_t>> states(faces.size());
        std::vector<bool> assigned(faces.size());
        std::vector<std::uint32_t> queue;
        assigned[0] = true;
        queue.push_back(0);
        for (std::size_t queue_index = 0; queue_index < queue.size(); ++queue_index)
        {
            const std::uint32_t face = queue[queue_index];
            for (const Adjacency adjacent : adjacency[face])
            {
                std::vector<std::uint64_t> candidate =
                    toggle(states[face], transitions[adjacent.edge]);
                if (!assigned[adjacent.neighbor])
                {
                    assigned[adjacent.neighbor] = true;
                    states[adjacent.neighbor] = std::move(candidate);
                    queue.push_back(adjacent.neighbor);
                }
                else if (states[adjacent.neighbor] != candidate)
                    return Error::invalid_argument;
            }
        }
        if (std::find(assigned.begin(), assigned.end(), false) != assigned.end())
            return Error::invalid_argument;

        for (std::uint32_t edge = 0; edge < edges.size(); ++edge)
        {
            const std::uint32_t left = half_edges[forward[edge]].face;
            const std::uint32_t right = half_edges[reverse[edge]].face;
            for (const Transition transition : transitions[edge])
                if (contains(states[left], transition.coverage_id) != transition.expected_left ||
                    contains(states[right], transition.coverage_id) == transition.expected_left)
                    return Error::invalid_argument;
        }

        std::uint64_t total_coverages = 0;
        for (const auto& state : states)
        {
            if (state.size() > kMaximumFaceCoverageMemberships - total_coverages)
                return Error::resource_limit_exceeded;
            total_coverages += state.size();
        }
        face_coverages.reserve(static_cast<std::size_t>(total_coverages));
        for (std::uint32_t face = 0; face < faces.size(); ++face)
        {
            if (face_coverages.size() > std::numeric_limits<std::uint32_t>::max() ||
                states[face].size() > std::numeric_limits<std::uint32_t>::max())
                return Error::resource_limit_exceeded;
            faces[face].coverage_begin = static_cast<std::uint32_t>(face_coverages.size());
            faces[face].coverage_count = static_cast<std::uint32_t>(states[face].size());
            face_coverages.insert(face_coverages.end(), states[face].begin(), states[face].end());
        }
        return Error::none;
    }
    catch (const std::exception&)
    {
        return Error::resource_limit_exceeded;
    }
}

} // namespace geometer::exact::arrangement_detail

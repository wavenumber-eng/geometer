#include "fast_hlr_reconstruct.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <numeric>
#include <tuple>
#include <utility>

namespace geometer::fast_hlr_internal
{
namespace
{

struct EndpointReference
{
    std::size_t fragment = 0;
    std::size_t endpoint = 0;
};

struct DisjointSet
{
    explicit DisjointSet(std::size_t size) : parents(size)
    {
        std::iota(parents.begin(), parents.end(), 0);
    }

    std::size_t root(std::size_t item)
    {
        while (parents[item] != item)
        {
            parents[item] = parents[parents[item]];
            item = parents[item];
        }
        return item;
    }

    void join(std::size_t first, std::size_t second)
    {
        first = root(first);
        second = root(second);
        if (first == second)
        {
            return;
        }
        const std::size_t low = std::min(first, second);
        const std::size_t high = std::max(first, second);
        parents[high] = low;
    }

    std::vector<std::size_t> parents;
};

std::array<double, 2> endpoint(const ProjectedSegment& segment, std::size_t index)
{
    return index == 0 ? std::array<double, 2>{segment.x1, segment.y1}
                      : std::array<double, 2>{segment.x2, segment.y2};
}

std::array<double, 2> outward_vector(const ProjectedSegment& segment, std::size_t endpoint_index)
{
    if (endpoint_index == 0)
    {
        return {segment.x2 - segment.x1, segment.y2 - segment.y1};
    }
    return {segment.x1 - segment.x2, segment.y1 - segment.y2};
}

bool exact_collinear_continuation(const ProjectedFragment& first, std::size_t first_endpoint,
                                  const ProjectedFragment& second, std::size_t second_endpoint)
{
    const std::array<double, 2> first_vector = outward_vector(first.segment, first_endpoint);
    const std::array<double, 2> second_vector = outward_vector(second.segment, second_endpoint);
    const double first_length = std::hypot(first_vector[0], first_vector[1]);
    const double second_length = std::hypot(second_vector[0], second_vector[1]);
    if (!(first_length > 0.0) || !(second_length > 0.0))
    {
        return false;
    }
    const double cross = first_vector[0] * second_vector[1] - first_vector[1] * second_vector[0];
    const double dot = first_vector[0] * second_vector[0] + first_vector[1] * second_vector[1];
    return dot < 0.0 && cross == 0.0;
}

bool replacement_preserves_locus(const std::vector<ProjectedFragment>& fragments,
                                 const std::vector<std::size_t>& component,
                                 const std::array<double, 2>& first,
                                 const std::array<double, 2>& second)
{
    const double dx = second[0] - first[0];
    const double dy = second[1] - first[1];
    const double chord_length = std::hypot(dx, dy);
    if (!(chord_length > 0.0))
    {
        return false;
    }
    for (std::size_t index : component)
    {
        const ProjectedSegment& segment = fragments[index].segment;
        for (std::size_t endpoint_index = 0; endpoint_index < 2; ++endpoint_index)
        {
            const std::array<double, 2> point = endpoint(segment, endpoint_index);
            const double area = std::fabs(dx * (point[1] - first[1]) - dy * (point[0] - first[0]));
            if (area != 0.0)
            {
                return false;
            }
        }
    }
    return true;
}

} // namespace

bool FragmentProvenance::operator<(const FragmentProvenance& other) const
{
    return std::tie(category_mask, first_source_face, second_source_face, unique_edge) <
           std::tie(other.category_mask, other.first_source_face, other.second_source_face,
                    other.unique_edge);
}

std::vector<ProjectedSegment>
reconstruct_collinear_fragments(const std::vector<ProjectedFragment>& fragments,
                                ReconstructionStatistics* statistics)
{
    ReconstructionStatistics output_statistics;
    std::vector<ProjectedSegment> output;
    output.reserve(fragments.size());
    if (fragments.empty())
    {
        if (statistics != nullptr)
        {
            *statistics = output_statistics;
        }
        return output;
    }

    std::map<FragmentProvenance, std::vector<std::size_t>> groups;
    for (std::size_t index = 0; index < fragments.size(); ++index)
    {
        groups[fragments[index].provenance].push_back(index);
    }

    DisjointSet sets(fragments.size());
    std::vector<std::array<bool, 2>> connected(fragments.size(), {false, false});
    for (const auto& group : groups)
    {
        std::map<std::uint32_t, std::vector<EndpointReference>> vertices;
        for (std::size_t fragment_index : group.second)
        {
            const ProjectedFragment& fragment = fragments[fragment_index];
            if (fragment.start_vertex != kNoTopologyVertex)
            {
                vertices[fragment.start_vertex].push_back({fragment_index, 0});
            }
            if (fragment.end_vertex != kNoTopologyVertex)
            {
                vertices[fragment.end_vertex].push_back({fragment_index, 1});
            }
        }
        for (const auto& vertex : vertices)
        {
            if (vertex.second.size() != 2)
            {
                continue;
            }
            const EndpointReference& first = vertex.second[0];
            const EndpointReference& second = vertex.second[1];
            if (!exact_collinear_continuation(fragments[first.fragment], first.endpoint,
                                              fragments[second.fragment], second.endpoint))
            {
                continue;
            }
            sets.join(first.fragment, second.fragment);
            connected[first.fragment][first.endpoint] = true;
            connected[second.fragment][second.endpoint] = true;
        }
    }

    std::map<std::size_t, std::vector<std::size_t>> components;
    for (std::size_t index = 0; index < fragments.size(); ++index)
    {
        components[sets.root(index)].push_back(index);
    }
    for (const auto& entry : components)
    {
        const std::vector<std::size_t>& component = entry.second;
        if (component.size() == 1)
        {
            output.push_back(fragments[component.front()].segment);
            continue;
        }
        std::vector<std::array<double, 2>> outer_endpoints;
        for (std::size_t index : component)
        {
            for (std::size_t endpoint_index = 0; endpoint_index < 2; ++endpoint_index)
            {
                if (!connected[index][endpoint_index])
                {
                    outer_endpoints.push_back(endpoint(fragments[index].segment, endpoint_index));
                }
            }
        }
        if (outer_endpoints.size() != 2 ||
            !replacement_preserves_locus(fragments, component, outer_endpoints[0],
                                         outer_endpoints[1]))
        {
            ++output_statistics.rejected;
            for (std::size_t index : component)
            {
                output.push_back(fragments[index].segment);
            }
            continue;
        }
        output.push_back({outer_endpoints[0][0], outer_endpoints[0][1], outer_endpoints[1][0],
                          outer_endpoints[1][1]});
        output_statistics.joins += component.size() - 1;
    }

    if (statistics != nullptr)
    {
        *statistics = output_statistics;
    }
    return output;
}

} // namespace geometer::fast_hlr_internal

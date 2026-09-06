#pragma once

#include "mesh_illustration_internal.h"

#include <numeric>
#include <unordered_map>

namespace geometer::illustration_detail
{
using Pair = std::pair<std::size_t, std::size_t>;
struct FusionEdge
{
    std::size_t triangle;
    Vec2 start, end, third;
    std::string start_key, end_key, key;
};

struct Disjoint
{
    std::vector<std::size_t> parent;
    explicit Disjoint(std::size_t size) : parent(size)
    {
        std::iota(parent.begin(), parent.end(), 0);
    }
    std::size_t find(std::size_t value)
    {
        auto root = value;
        while (parent[root] != root)
            root = parent[root];
        while (parent[value] != value)
        {
            const auto next = parent[value];
            parent[value] = root;
            value = next;
        }
        return root;
    }
    void unite(std::size_t a, std::size_t b)
    {
        parent[find(b)] = find(a);
    }
};

// Preserve JavaScript Map insertion order, including groups encountered out of
// numeric order. Hash tables are only lookup accelerators, never output order.
template <typename Key, typename Value> struct OrderedGroups
{
    std::unordered_map<Key, std::size_t> lookup;
    std::vector<std::pair<Key, std::vector<Value>>> entries;
    void add(const Key& key, Value value)
    {
        const auto found = lookup.emplace(key, entries.size());
        if (found.second)
            entries.push_back({key, {}});
        entries[found.first->second].second.push_back(std::move(value));
    }
};

std::array<unsigned, 3> oriented_indices(const Triangle& triangle);
OrderedGroups<std::string, FusionEdge> fusion_edges(const std::vector<TriangleCommand>& commands,
                                                    const std::vector<std::size_t>& members,
                                                    double coordinate_tolerance,
                                                    double depth_tolerance, bool opaque_only);
std::optional<Surface> fused_component(const std::vector<TriangleCommand>& commands,
                                       const std::vector<std::size_t>& members,
                                       double coordinate_tolerance, double depth_tolerance,
                                       WorkBudget& budget);
std::optional<Surface> layered_surface(const std::vector<TriangleCommand>& commands,
                                       const std::vector<std::size_t>& members,
                                       const std::vector<std::vector<std::size_t>>& adjacency,
                                       double coordinate_tolerance, double depth_tolerance,
                                       WorkBudget& budget);
std::string surface_style_key(const TriangleCommand& command);
} // namespace geometer::illustration_detail

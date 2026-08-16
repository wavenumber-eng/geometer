#include "analytic_filtered_packet_sequences.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <tuple>
#include <vector>

namespace geometer::analytic_packet_detail
{
namespace
{

constexpr std::uint32_t kNone = std::numeric_limits<std::uint32_t>::max();

struct SequenceNode
{
    std::uint32_t parent = 0;
    std::uint32_t label = 0;
    bool terminal = false;
};

struct SequenceEdge
{
    std::uint32_t parent = 0;
    std::uint32_t label = 0;
    std::uint32_t node = 0;
};

std::uint64_t transition_hash(std::uint32_t parent, std::uint32_t label) noexcept
{
    std::uint64_t value = (static_cast<std::uint64_t>(parent) << 32U) | label;
    value ^= value >> 30U;
    value *= 0xbf58476d1ce4e5b9ULL;
    value ^= value >> 27U;
    value *= 0x94d049bb133111ebULL;
    return value ^ (value >> 31U);
}

bool next_table_capacity(std::uint64_t nodes, std::uint64_t& capacity) noexcept
{
    std::uint64_t required = 0;
    if (!checked_multiply(std::max<std::uint64_t>(nodes, 2), 2, required))
        return false;
    capacity = 4;
    while (capacity < required)
    {
        if (capacity > std::numeric_limits<std::uint64_t>::max() / 2)
            return false;
        capacity *= 2;
    }
    return capacity <= std::numeric_limits<std::uint32_t>::max();
}

} // namespace

bool checked_add(std::uint64_t left, std::uint64_t right, std::uint64_t& output) noexcept
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
        return false;
    output = left + right;
    return true;
}

bool checked_multiply(std::uint64_t left, std::uint64_t right, std::uint64_t& output) noexcept
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left)
        return false;
    output = left * right;
    return true;
}

std::uint64_t sort_units(std::uint64_t count) noexcept
{
    if (count < 2)
        return 0;
    std::uint64_t levels = 0;
    for (std::uint64_t value = count - 1; value != 0; value >>= 1U)
        ++levels;
    return count * levels;
}

bool WorkBudget::charge(std::uint64_t units) noexcept
{
    if (used > limit || units > limit - used)
        return false;
    used += units;
    return true;
}

bool WorkBudget::charge_sort(std::uint64_t count) noexcept
{
    const std::uint64_t units = sort_units(count);
    if (!charge(units))
        return false;
    telemetry->canonical_sort_work_units += units;
    return true;
}

bool canonicalize_sequences(const std::vector<std::uint32_t>& labels,
                            const std::vector<SequenceRange>& ranges, bool serialize,
                            std::uint64_t live_base_bytes, std::uint64_t memory_limit,
                            WorkBudget& budget, CanonicalSequences& output)
{
    std::uint64_t maximum_nodes = 0;
    if (!checked_add(labels.size(), 1, maximum_nodes) ||
        maximum_nodes > std::numeric_limits<std::uint32_t>::max())
        return false;
    std::uint64_t table_capacity = 0;
    if (!next_table_capacity(maximum_nodes, table_capacity))
        return false;
    std::uint64_t logical_bytes = 0;
    std::uint64_t term = 0;
    if (!checked_multiply(maximum_nodes, 16, logical_bytes) ||
        !checked_multiply(table_capacity, 4, term) ||
        !checked_add(logical_bytes, term, logical_bytes) ||
        !checked_multiply(maximum_nodes, 28, term) ||
        !checked_add(logical_bytes, term, logical_bytes) ||
        !checked_multiply(ranges.size(), 8, term) ||
        !checked_add(logical_bytes, term, logical_bytes))
        return false;
    output.logical_bytes = logical_bytes;
    std::uint64_t fixed_work = 0;
    std::uint64_t term_work = 0;
    if (!checked_multiply(ranges.size(), 3, fixed_work) ||
        !checked_multiply(maximum_nodes, 3, term_work) ||
        !checked_add(fixed_work, term_work, fixed_work) || !budget.charge(fixed_work))
        return false;
    std::uint64_t live_bytes = 0;
    if (!checked_add(live_base_bytes, logical_bytes, live_bytes) || live_bytes > memory_limit)
        return false;
    budget.telemetry->peak_working_memory_bytes =
        std::max(budget.telemetry->peak_working_memory_bytes, live_bytes);
    try
    {
        output.records.reserve(ranges.size());
        if (serialize)
            output.indices.reserve(labels.size());
        std::vector<SequenceNode> nodes;
        nodes.reserve(static_cast<std::size_t>(maximum_nodes));
        nodes.push_back({});
        std::vector<std::uint32_t> table(static_cast<std::size_t>(table_capacity), kNone);
        std::vector<std::uint32_t> terminal_by_range(ranges.size(), 0);
        for (std::uint32_t range_index = 0; range_index < ranges.size(); ++range_index)
        {
            const SequenceRange range = ranges[range_index];
            if (range.begin > labels.size() || range.count > labels.size() - range.begin)
                return false;
            std::uint32_t parent = 0;
            for (std::uint32_t offset = 0; offset < range.count; ++offset)
            {
                if (!budget.charge(1))
                    return false;
                const std::uint32_t label = labels[range.begin + offset];
                std::uint64_t slot = transition_hash(parent, label) & (table_capacity - 1);
                for (;;)
                {
                    if (!budget.charge(1))
                        return false;
                    ++budget.telemetry->sequence_table_probes;
                    const std::uint32_t existing = table[static_cast<std::size_t>(slot)];
                    if (existing == kNone)
                    {
                        if (nodes.size() >= maximum_nodes)
                            return false;
                        const std::uint32_t node = static_cast<std::uint32_t>(nodes.size());
                        nodes.push_back({parent, label, false});
                        table[static_cast<std::size_t>(slot)] = node;
                        parent = node;
                        break;
                    }
                    if (nodes[existing].parent == parent && nodes[existing].label == label)
                    {
                        parent = existing;
                        break;
                    }
                    slot = (slot + 1) & (table_capacity - 1);
                }
            }
            nodes[parent].terminal = true;
            terminal_by_range[range_index] = parent;
        }
        budget.telemetry->sequence_nodes += nodes.size();

        std::vector<SequenceEdge> edges;
        edges.reserve(nodes.size() - 1);
        for (std::uint32_t node = 1; node < nodes.size(); ++node)
            edges.push_back({nodes[node].parent, nodes[node].label, node});
        if (!budget.charge_sort(edges.size()))
            return false;
        std::sort(edges.begin(), edges.end(),
                  [](const SequenceEdge& left, const SequenceEdge& right)
                  {
                      return std::tie(left.parent, left.label, left.node) <
                             std::tie(right.parent, right.label, right.node);
                  });
        std::vector<SequenceRange> child_ranges(nodes.size());
        for (std::uint32_t begin = 0; begin < edges.size();)
        {
            std::uint32_t end = begin + 1;
            while (end < edges.size() && edges[end].parent == edges[begin].parent)
                ++end;
            child_ranges[edges[begin].parent] = {begin, end - begin};
            begin = end;
        }
        std::vector<std::uint32_t> rank_by_node(nodes.size(), 0);
        struct Frame
        {
            std::uint32_t node = 0;
            std::uint32_t next_child = 0;
            bool entered = false;
        };
        std::vector<Frame> stack;
        std::vector<std::uint32_t> path;
        stack.reserve(nodes.size());
        path.reserve(nodes.size());
        stack.push_back({0, 0, false});
        while (!stack.empty())
        {
            Frame& frame = stack.back();
            if (!frame.entered)
            {
                if (!budget.charge(1))
                    return false;
                frame.entered = true;
                if (frame.node != 0)
                    path.push_back(nodes[frame.node].label);
                if (frame.node != 0 && nodes[frame.node].terminal)
                {
                    const std::uint32_t rank =
                        static_cast<std::uint32_t>(output.records.size()) + 1;
                    rank_by_node[frame.node] = rank;
                    if (serialize)
                    {
                        if (!budget.charge(path.size()))
                            return false;
                        const std::uint32_t begin =
                            static_cast<std::uint32_t>(output.indices.size());
                        output.indices.insert(output.indices.end(), path.begin(), path.end());
                        output.records.push_back({begin, static_cast<std::uint32_t>(path.size())});
                    }
                    else
                        output.records.push_back({0, 0});
                }
            }
            const SequenceRange children = child_ranges[frame.node];
            if (frame.next_child < children.count)
            {
                const std::uint32_t child = edges[children.begin + frame.next_child].node;
                ++frame.next_child;
                stack.push_back({child, 0, false});
            }
            else
            {
                if (frame.node != 0)
                    path.pop_back();
                stack.pop_back();
            }
        }
        output.handles.resize(ranges.size());
        for (std::uint32_t index = 0; index < ranges.size(); ++index)
            output.handles[index] =
                ranges[index].count == 0 ? 0 : rank_by_node[terminal_by_range[index]];
        return true;
    }
    catch (const std::bad_alloc&)
    {
        return false;
    }
}

} // namespace geometer::analytic_packet_detail

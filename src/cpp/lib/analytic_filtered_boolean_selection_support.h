#pragma once

#include "geometer/analytic_filtered_boolean_selection.h"

#include "analytic_filtered_interval.h"

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
namespace analytic_selection_detail
{
using analytic_detail::add;
using analytic_detail::cross;
using analytic_detail::dot;
using analytic_detail::exact;
using analytic_detail::Interval;
using analytic_detail::multiply;
using analytic_detail::Point;
using analytic_detail::square;
using analytic_detail::subtract;

constexpr std::uint32_t kNoIndex = std::numeric_limits<std::uint32_t>::max();
constexpr std::uint64_t kFaceLogicalBytes = 48;
constexpr std::uint64_t kSweepEdgeLogicalBytes = 32;
constexpr std::uint64_t kSweepNodeLogicalBytes = 40;
constexpr std::uint64_t kEventReferenceLogicalBytes = 16;
constexpr std::uint64_t kReferenceRangeLogicalBytes = 8;
constexpr std::uint64_t kColumnLogicalBytes = 32;
constexpr std::uint64_t kDisjointSetLogicalBytes = 8;
constexpr std::uint64_t kTransitionLogicalBytes = 24;
constexpr std::uint64_t kAdjacencyLogicalBytes = 16;
constexpr std::uint64_t kIndexLogicalBytes = 8;
constexpr std::uint64_t kOccurrenceLogicalBytes = 64;
constexpr std::uint64_t kOperandMetadataLogicalBytes = 32;
constexpr std::uint64_t kOperandOrdinalLogicalBytes = 8;
constexpr std::uint64_t kByteLogicalBytes = 1;
constexpr std::uint64_t kSweepTemporaryLogicalBytes = 64;
constexpr std::uint64_t kCoverageNodeLogicalBytes = 8;
constexpr std::uint64_t kCoverageTableEntryLogicalBytes = 16;
constexpr std::uint64_t kDualFrameLogicalBytes = 24;

inline std::uint64_t checked_add(std::uint64_t left, std::uint64_t right, bool& valid) noexcept
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
    {
        valid = false;
        return 0;
    }
    return left + right;
}

inline std::uint64_t checked_multiply(std::uint64_t left, std::uint64_t right, bool& valid) noexcept
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left)
    {
        valid = false;
        return 0;
    }
    return left * right;
}

inline std::uint64_t sort_units(std::uint64_t count) noexcept
{
    if (count < 2)
        return 0;
    std::uint64_t levels = 0;
    for (std::uint64_t value = count - 1; value != 0; value >>= 1U)
        ++levels;
    return count * levels;
}

inline std::uint64_t tree_operation_units(std::uint64_t capacity) noexcept
{
    std::uint64_t levels = 1;
    for (std::uint64_t value = capacity; value > 1; value = (value + 1) / 2)
        ++levels;
    return levels * 4 + 4;
}

inline Point point(const AnalyticFilteredPointNm& value) noexcept
{
    return {{value.x.lower, value.x.upper}, {value.y.lower, value.y.upper}};
}

inline bool valid_interval(const AnalyticCoordinateIntervalNm& value) noexcept
{
    return std::isfinite(value.lower) && std::isfinite(value.upper) && value.lower <= value.upper;
}

struct DisjointSet
{
    explicit DisjointSet(std::size_t count) : parent(count), rank(count)
    {
        for (std::size_t index = 0; index < count; ++index)
            parent[index] = static_cast<std::uint32_t>(index);
    }

    std::uint32_t find(std::uint32_t value, std::uint64_t& visits) noexcept
    {
        ++visits;
        while (parent[value] != value)
        {
            ++visits;
            parent[value] = parent[parent[value]];
            value = parent[value];
        }
        return value;
    }

    bool unite(std::uint32_t left, std::uint32_t right, std::uint64_t& visits) noexcept
    {
        left = find(left, visits);
        right = find(right, visits);
        if (left == right)
            return false;
        if (rank[left] < rank[right] || (rank[left] == rank[right] && right < left))
            std::swap(left, right);
        parent[right] = left;
        if (rank[left] == rank[right])
            ++rank[left];
        return true;
    }

    std::vector<std::uint32_t> parent;
    std::vector<std::uint8_t> rank;
};

class SweepOrder
{
  public:
    explicit SweepOrder(std::size_t capacity) : nodes_(capacity), node_by_edge_(capacity, kNoIndex)
    {
    }

    [[nodiscard]] std::uint32_t size() const noexcept
    {
        return subtree_size(root_);
    }

    [[nodiscard]] bool active(std::uint32_t edge) const noexcept
    {
        return edge < node_by_edge_.size() && node_by_edge_[edge] != kNoIndex;
    }

    [[nodiscard]] std::uint32_t edge_at(std::uint32_t rank) const noexcept
    {
        std::uint32_t node = root_;
        while (node != kNoIndex)
        {
            const std::uint32_t left_size = subtree_size(nodes_[node].left);
            if (rank < left_size)
                node = nodes_[node].left;
            else if (rank == left_size)
                return nodes_[node].edge;
            else
            {
                rank -= left_size + 1;
                node = nodes_[node].right;
            }
        }
        return kNoIndex;
    }

    [[nodiscard]] std::uint32_t rank_of(std::uint32_t edge) const noexcept
    {
        if (!active(edge))
            return kNoIndex;
        std::uint32_t node = node_by_edge_[edge];
        std::uint32_t rank = subtree_size(nodes_[node].left);
        while (nodes_[node].parent != kNoIndex)
        {
            const std::uint32_t parent = nodes_[node].parent;
            if (nodes_[parent].right == node)
                rank += subtree_size(nodes_[parent].left) + 1;
            node = parent;
        }
        return rank;
    }

    bool insert(std::uint32_t rank, std::uint32_t edge) noexcept
    {
        if (edge >= node_by_edge_.size() || active(edge) || next_node_ == nodes_.size() ||
            rank > size())
            return false;
        const std::uint32_t node = static_cast<std::uint32_t>(next_node_++);
        nodes_[node] = {edge, kNoIndex, kNoIndex, kNoIndex, 1, 1};
        node_by_edge_[edge] = node;
        root_ = insert_at(root_, node, rank);
        nodes_[root_].parent = kNoIndex;
        return true;
    }

    bool erase(std::uint32_t edge) noexcept
    {
        const std::uint32_t rank = rank_of(edge);
        if (rank == kNoIndex)
            return false;
        bool removed = false;
        root_ = erase_at(root_, rank, removed);
        if (root_ != kNoIndex)
            nodes_[root_].parent = kNoIndex;
        return removed;
    }

    template <typename Compare>
    std::optional<std::uint32_t> insertion_rank(Compare&& compare, std::uint64_t& visits) const
    {
        std::uint32_t node = root_;
        std::uint32_t rank = 0;
        while (node != kNoIndex)
        {
            ++visits;
            const std::optional<std::int8_t> relation = compare(nodes_[node].edge);
            if (!relation || *relation == 0)
                return std::nullopt;
            if (*relation > 0)
            {
                rank += subtree_size(nodes_[node].left) + 1;
                node = nodes_[node].right;
            }
            else
                node = nodes_[node].left;
        }
        return rank;
    }

  private:
    struct Node
    {
        std::uint32_t edge = kNoIndex;
        std::uint32_t left = kNoIndex;
        std::uint32_t right = kNoIndex;
        std::uint32_t parent = kNoIndex;
        std::uint32_t height = 1;
        std::uint32_t size = 1;
    };

    static_assert(sizeof(Node) <= kSweepNodeLogicalBytes);

    [[nodiscard]] std::uint32_t subtree_size(std::uint32_t node) const noexcept
    {
        return node == kNoIndex ? 0 : nodes_[node].size;
    }

    [[nodiscard]] std::uint32_t height(std::uint32_t node) const noexcept
    {
        return node == kNoIndex ? 0 : nodes_[node].height;
    }

    void update(std::uint32_t node) noexcept
    {
        nodes_[node].height = 1 + std::max(height(nodes_[node].left), height(nodes_[node].right));
        nodes_[node].size = 1 + subtree_size(nodes_[node].left) + subtree_size(nodes_[node].right);
        if (nodes_[node].left != kNoIndex)
            nodes_[nodes_[node].left].parent = node;
        if (nodes_[node].right != kNoIndex)
            nodes_[nodes_[node].right].parent = node;
    }

    std::uint32_t rotate_left(std::uint32_t root) noexcept
    {
        const std::uint32_t pivot = nodes_[root].right;
        const std::uint32_t parent = nodes_[root].parent;
        nodes_[root].right = nodes_[pivot].left;
        nodes_[pivot].left = root;
        nodes_[root].parent = pivot;
        nodes_[pivot].parent = parent;
        update(root);
        update(pivot);
        return pivot;
    }

    std::uint32_t rotate_right(std::uint32_t root) noexcept
    {
        const std::uint32_t pivot = nodes_[root].left;
        const std::uint32_t parent = nodes_[root].parent;
        nodes_[root].left = nodes_[pivot].right;
        nodes_[pivot].right = root;
        nodes_[root].parent = pivot;
        nodes_[pivot].parent = parent;
        update(root);
        update(pivot);
        return pivot;
    }

    std::uint32_t rebalance(std::uint32_t root) noexcept
    {
        update(root);
        const int balance = static_cast<int>(height(nodes_[root].left)) -
                            static_cast<int>(height(nodes_[root].right));
        if (balance > 1)
        {
            const std::uint32_t left = nodes_[root].left;
            if (height(nodes_[left].left) < height(nodes_[left].right))
            {
                nodes_[root].left = rotate_left(left);
                nodes_[nodes_[root].left].parent = root;
            }
            return rotate_right(root);
        }
        if (balance < -1)
        {
            const std::uint32_t right = nodes_[root].right;
            if (height(nodes_[right].right) < height(nodes_[right].left))
            {
                nodes_[root].right = rotate_right(right);
                nodes_[nodes_[root].right].parent = root;
            }
            return rotate_left(root);
        }
        return root;
    }

    std::uint32_t insert_at(std::uint32_t root, std::uint32_t inserted, std::uint32_t rank) noexcept
    {
        if (root == kNoIndex)
            return inserted;
        const std::uint32_t left_size = subtree_size(nodes_[root].left);
        if (rank <= left_size)
        {
            nodes_[root].left = insert_at(nodes_[root].left, inserted, rank);
            nodes_[nodes_[root].left].parent = root;
        }
        else
        {
            nodes_[root].right = insert_at(nodes_[root].right, inserted, rank - left_size - 1);
            nodes_[nodes_[root].right].parent = root;
        }
        return rebalance(root);
    }

    std::uint32_t erase_at(std::uint32_t root, std::uint32_t rank, bool& removed) noexcept
    {
        const std::uint32_t left_size = subtree_size(nodes_[root].left);
        if (rank < left_size)
        {
            nodes_[root].left = erase_at(nodes_[root].left, rank, removed);
            if (nodes_[root].left != kNoIndex)
                nodes_[nodes_[root].left].parent = root;
        }
        else if (rank > left_size)
        {
            nodes_[root].right = erase_at(nodes_[root].right, rank - left_size - 1, removed);
            if (nodes_[root].right != kNoIndex)
                nodes_[nodes_[root].right].parent = root;
        }
        else
        {
            removed = true;
            const std::uint32_t removed_edge = nodes_[root].edge;
            if (nodes_[root].left == kNoIndex || nodes_[root].right == kNoIndex)
            {
                const std::uint32_t child =
                    nodes_[root].left != kNoIndex ? nodes_[root].left : nodes_[root].right;
                if (child != kNoIndex)
                    nodes_[child].parent = nodes_[root].parent;
                node_by_edge_[removed_edge] = kNoIndex;
                return child;
            }
            std::uint32_t successor = nodes_[root].right;
            while (nodes_[successor].left != kNoIndex)
                successor = nodes_[successor].left;
            const std::uint32_t successor_edge = nodes_[successor].edge;
            nodes_[root].edge = successor_edge;
            node_by_edge_[successor_edge] = root;
            bool successor_removed = false;
            nodes_[root].right = erase_at(nodes_[root].right, 0, successor_removed);
            if (nodes_[root].right != kNoIndex)
                nodes_[nodes_[root].right].parent = root;
            node_by_edge_[successor_edge] = root;
            node_by_edge_[removed_edge] = kNoIndex;
        }
        return rebalance(root);
    }

    std::vector<Node> nodes_;
    std::vector<std::uint32_t> node_by_edge_;
    std::size_t next_node_ = 0;
    std::uint32_t root_ = kNoIndex;
};

struct SweepEdge
{
    std::uint32_t left_vertex = 0;
    std::uint32_t right_vertex = 0;
    std::uint32_t above_cycle = 0;
    std::uint32_t below_cycle = 0;
    bool vertical = false;
};

struct EventReference
{
    std::uint32_t vertex = 0;
    std::uint32_t edge = 0;
    bool start = false;
};

struct EventColumn
{
    std::uint32_t vertex_begin = 0;
    std::uint32_t vertex_count = 0;
    double minimum_x = 0.0;
    double maximum_x = 0.0;
    bool resolution_group = false;
};

struct Transition
{
    std::uint32_t edge = 0;
    std::uint32_t operand = 0;
    std::uint64_t coverage_id = 0;
    bool expected_left = false;
};

struct Adjacency
{
    std::uint32_t face = 0;
    std::uint32_t neighbor = 0;
    std::uint32_t edge = 0;
};

struct OperandMetadata
{
    std::uint64_t operand_id = 0;
    std::uint64_t stage_id = 0;
    std::uint32_t stage_ordinal = 0;
    std::uint8_t operation = 0;
};

struct CoverageTableEntry
{
    std::uint32_t left = 0;
    std::uint32_t right = 0;
    std::uint32_t node = kNoIndex;
};

class CanonicalCoverageSet
{
  public:
    CanonicalCoverageSet(std::vector<AnalyticFilteredCoverageStateNode>& nodes,
                         std::uint32_t operand_count, std::uint64_t maximum_nodes,
                         std::uint64_t table_capacity)
        : nodes_(nodes), operand_count_(operand_count), table_(table_capacity)
    {
        nodes_.reserve(static_cast<std::size_t>(maximum_nodes));
        nodes_.push_back({0, 0});
        nodes_.push_back({kNoIndex, kNoIndex});
        leaf_capacity_ = 1;
        while (leaf_capacity_ < std::max<std::uint32_t>(1, operand_count_))
        {
            leaf_capacity_ <<= 1U;
            ++depth_;
        }
    }

    [[nodiscard]] std::uint32_t depth() const noexcept
    {
        return depth_;
    }

    [[nodiscard]] bool contains(std::uint32_t root, std::uint32_t operand) const noexcept
    {
        if (operand >= operand_count_)
            return false;
        std::uint32_t begin = 0;
        std::uint32_t width = leaf_capacity_;
        while (width > 1)
        {
            if (root >= nodes_.size() || root == 1)
                return false;
            const std::uint32_t half = width / 2;
            if (operand < begin + half)
                root = root == 0 ? 0 : nodes_[root].left;
            else
            {
                begin += half;
                root = root == 0 ? 0 : nodes_[root].right;
            }
            width = half;
        }
        return root == 1;
    }

    template <typename Charge>
    bool toggle(std::uint32_t root, std::uint32_t operand, std::uint32_t& output, Charge&& charge,
                std::uint64_t& update_work, std::uint64_t& probes)
    {
        if (operand >= operand_count_)
            return false;
        return update(root, 0, leaf_capacity_, operand, output, charge, update_work, probes);
    }

  private:
    static std::uint64_t hash_pair(std::uint32_t left, std::uint32_t right) noexcept
    {
        std::uint64_t value = (static_cast<std::uint64_t>(left) << 32U) | right;
        value ^= value >> 30U;
        value *= 0xbf58476d1ce4e5b9ULL;
        value ^= value >> 27U;
        value *= 0x94d049bb133111ebULL;
        return value ^ (value >> 31U);
    }

    template <typename Charge>
    bool intern(std::uint32_t left, std::uint32_t right, std::uint32_t& output, Charge&& charge,
                std::uint64_t& probes)
    {
        if (left == 0 && right == 0)
        {
            output = 0;
            return true;
        }
        const std::uint64_t mask = table_.size() - 1;
        std::uint64_t slot = hash_pair(left, right) & mask;
        for (std::uint64_t attempt = 0; attempt < table_.size(); ++attempt)
        {
            if (!charge(1))
                return false;
            ++probes;
            CoverageTableEntry& entry = table_[slot];
            if (entry.node == kNoIndex)
            {
                if (nodes_.size() == nodes_.capacity() ||
                    nodes_.size() >= std::numeric_limits<std::uint32_t>::max())
                    return false;
                output = static_cast<std::uint32_t>(nodes_.size());
                nodes_.push_back({left, right});
                entry = {left, right, output};
                return true;
            }
            if (entry.left == left && entry.right == right)
            {
                output = entry.node;
                return true;
            }
            slot = (slot + 1) & mask;
        }
        return false;
    }

    template <typename Charge>
    bool update(std::uint32_t root, std::uint32_t begin, std::uint32_t width, std::uint32_t operand,
                std::uint32_t& output, Charge&& charge, std::uint64_t& update_work,
                std::uint64_t& probes)
    {
        if (!charge(1))
            return false;
        ++update_work;
        if (width == 1)
        {
            output = root == 0 ? 1 : 0;
            return root <= 1;
        }
        if (root >= nodes_.size() || root == 1)
            return false;
        const std::uint32_t half = width / 2;
        std::uint32_t left = root == 0 ? 0 : nodes_[root].left;
        std::uint32_t right = root == 0 ? 0 : nodes_[root].right;
        if (operand < begin + half)
        {
            if (!update(left, begin, half, operand, left, charge, update_work, probes))
                return false;
        }
        else if (!update(right, begin + half, half, operand, right, charge, update_work, probes))
            return false;
        return intern(left, right, output, charge, probes);
    }

    std::vector<AnalyticFilteredCoverageStateNode>& nodes_;
    std::uint32_t operand_count_ = 0;
    std::uint32_t leaf_capacity_ = 1;
    std::uint32_t depth_ = 0;
    std::vector<CoverageTableEntry> table_;
};

class ActiveStageTree
{
  public:
    explicit ActiveStageTree(const std::vector<std::uint8_t>& operations)
        : operations_(operations), counts_(operations.size())
    {
        leaf_capacity_ = 1;
        while (leaf_capacity_ < std::max<std::size_t>(1, operations.size()))
        {
            leaf_capacity_ <<= 1U;
            ++depth_;
        }
        last_active_.assign(leaf_capacity_ * 2, -1);
    }

    template <typename Charge>
    bool toggle(std::uint32_t stage, bool activate, Charge&& charge, std::uint64_t& work)
    {
        if (stage >= counts_.size() || !charge(depth_ + 1))
            return false;
        work += depth_ + 1;
        if (activate)
            ++counts_[stage];
        else if (counts_[stage] == 0)
            return false;
        else
            --counts_[stage];
        std::size_t node = leaf_capacity_ + stage;
        last_active_[node] = counts_[stage] == 0 ? -1 : static_cast<std::int32_t>(stage);
        while (node > 1)
        {
            node /= 2;
            last_active_[node] = std::max(last_active_[node * 2], last_active_[node * 2 + 1]);
        }
        return true;
    }

    [[nodiscard]] bool material() const noexcept
    {
        const std::int32_t stage = last_active_[1];
        return stage >= 0 && operations_[static_cast<std::size_t>(stage)] == 1;
    }

    [[nodiscard]] std::uint32_t depth() const noexcept
    {
        return depth_;
    }

  private:
    const std::vector<std::uint8_t>& operations_;
    std::vector<std::uint32_t> counts_;
    std::vector<std::int32_t> last_active_;
    std::size_t leaf_capacity_ = 1;
    std::uint32_t depth_ = 0;
};

struct SelectionAdmission
{
    AnalyticFilteredBooleanSelectionResult result;
    AnalyticFilteredArrangementResult arrangement;
    std::uint64_t admission_work = 0;
    std::uint64_t admission_peak_memory = 0;
    bool ready = false;
};

[[nodiscard]] SelectionAdmission prepare_boolean_selection_admission(
    const AnalyticRequestPacketRecords& records, std::uint32_t job_index,
    const AnalyticFilteredGeometry& geometry, const std::vector<AnalyticCurvePair>& candidate_pairs,
    const AnalyticSolverLimits& limits);

} // namespace analytic_selection_detail
} // namespace geometer

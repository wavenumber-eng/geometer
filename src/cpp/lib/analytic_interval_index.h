#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace geometer::detail
{

// Deterministic AVL interval index used by the analytic broad phase. The tree
// is keyed by (minimum, curve_index) and augments every subtree with its
// greatest interval maximum, giving output-sensitive overlap reporting.
class AnalyticIntervalIndex
{
  public:
    explicit AnalyticIntervalIndex(std::size_t capacity);

    void insert(double minimum, double maximum, std::size_t payload, std::uint32_t curve_index);
    void erase(double minimum, std::uint32_t curve_index);

    [[nodiscard]] static constexpr std::uint64_t storage_bytes(std::size_t capacity) noexcept;

    template <typename Visitor>
    bool query(double minimum, double maximum, std::uint64_t& node_visits,
               std::uint64_t node_visit_limit, Visitor&& visitor) const
    {
        return query_node(root_, minimum, maximum, node_visits, node_visit_limit, visitor);
    }

  private:
    static constexpr std::size_t npos = std::numeric_limits<std::size_t>::max();

    struct Node
    {
        double minimum = 0.0;
        double maximum = 0.0;
        double subtree_maximum = 0.0;
        std::size_t payload = 0;
        std::size_t left = npos;
        std::size_t right = npos;
        std::uint32_t curve_index = 0;
        std::uint32_t height = 1;
    };

    [[nodiscard]] std::uint32_t height(std::size_t node) const noexcept;
    [[nodiscard]] int balance(std::size_t node) const noexcept;
    void update(std::size_t node) noexcept;
    [[nodiscard]] std::size_t rotate_left(std::size_t node) noexcept;
    [[nodiscard]] std::size_t rotate_right(std::size_t node) noexcept;
    [[nodiscard]] std::size_t rebalance(std::size_t node) noexcept;
    [[nodiscard]] std::size_t insert_node(std::size_t root, std::size_t inserted) noexcept;
    [[nodiscard]] std::size_t erase_node(std::size_t root, double minimum,
                                         std::uint32_t curve_index) noexcept;
    [[nodiscard]] std::size_t minimum_node(std::size_t root) const noexcept;

    template <typename Visitor>
    bool query_node(std::size_t node, double minimum, double maximum, std::uint64_t& node_visits,
                    std::uint64_t node_visit_limit, Visitor& visitor) const
    {
        if (node == npos)
            return true;
        if (node_visits == node_visit_limit)
            return false;
        ++node_visits;

        const Node& current = nodes_[node];
        if (current.left != npos && nodes_[current.left].subtree_maximum >= minimum &&
            !query_node(current.left, minimum, maximum, node_visits, node_visit_limit, visitor))
            return false;
        if (current.minimum <= maximum && current.maximum >= minimum &&
            !visitor(current.payload, current.curve_index))
            return false;
        if (current.minimum <= maximum &&
            !query_node(current.right, minimum, maximum, node_visits, node_visit_limit, visitor))
            return false;
        return true;
    }

    std::vector<Node> nodes_;
    std::size_t root_ = npos;
};

constexpr std::uint64_t AnalyticIntervalIndex::storage_bytes(std::size_t capacity) noexcept
{
    return static_cast<std::uint64_t>(capacity) * sizeof(Node);
}

} // namespace geometer::detail

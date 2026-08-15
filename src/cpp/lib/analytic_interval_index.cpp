#include "analytic_interval_index.h"

#include <tuple>

namespace geometer::detail
{
namespace
{

bool key_less(double left_minimum, std::uint32_t left_index, double right_minimum,
              std::uint32_t right_index)
{
    return std::tie(left_minimum, left_index) < std::tie(right_minimum, right_index);
}

} // namespace

AnalyticIntervalIndex::AnalyticIntervalIndex(std::size_t capacity)
{
    nodes_.reserve(capacity);
}

void AnalyticIntervalIndex::insert(double minimum, double maximum, std::size_t payload,
                                   std::uint32_t curve_index)
{
    const std::size_t inserted = nodes_.size();
    nodes_.push_back({minimum, maximum, maximum, payload, npos, npos, curve_index, 1});
    root_ = insert_node(root_, inserted);
}

void AnalyticIntervalIndex::erase(double minimum, std::uint32_t curve_index)
{
    root_ = erase_node(root_, minimum, curve_index);
}

std::uint32_t AnalyticIntervalIndex::height(std::size_t node) const noexcept
{
    return node == npos ? 0 : nodes_[node].height;
}

int AnalyticIntervalIndex::balance(std::size_t node) const noexcept
{
    return static_cast<int>(height(nodes_[node].left)) -
           static_cast<int>(height(nodes_[node].right));
}

void AnalyticIntervalIndex::update(std::size_t node) noexcept
{
    Node& current = nodes_[node];
    current.height = 1 + std::max(height(current.left), height(current.right));
    current.subtree_maximum = current.maximum;
    if (current.left != npos)
        current.subtree_maximum =
            std::max(current.subtree_maximum, nodes_[current.left].subtree_maximum);
    if (current.right != npos)
        current.subtree_maximum =
            std::max(current.subtree_maximum, nodes_[current.right].subtree_maximum);
}

std::size_t AnalyticIntervalIndex::rotate_left(std::size_t node) noexcept
{
    const std::size_t pivot = nodes_[node].right;
    const std::size_t middle = nodes_[pivot].left;
    nodes_[pivot].left = node;
    nodes_[node].right = middle;
    update(node);
    update(pivot);
    return pivot;
}

std::size_t AnalyticIntervalIndex::rotate_right(std::size_t node) noexcept
{
    const std::size_t pivot = nodes_[node].left;
    const std::size_t middle = nodes_[pivot].right;
    nodes_[pivot].right = node;
    nodes_[node].left = middle;
    update(node);
    update(pivot);
    return pivot;
}

std::size_t AnalyticIntervalIndex::rebalance(std::size_t node) noexcept
{
    update(node);
    const int node_balance = balance(node);
    if (node_balance > 1)
    {
        if (balance(nodes_[node].left) < 0)
            nodes_[node].left = rotate_left(nodes_[node].left);
        return rotate_right(node);
    }
    if (node_balance < -1)
    {
        if (balance(nodes_[node].right) > 0)
            nodes_[node].right = rotate_right(nodes_[node].right);
        return rotate_left(node);
    }
    return node;
}

std::size_t AnalyticIntervalIndex::insert_node(std::size_t root, std::size_t inserted) noexcept
{
    if (root == npos)
        return inserted;
    const Node& value = nodes_[inserted];
    if (key_less(value.minimum, value.curve_index, nodes_[root].minimum, nodes_[root].curve_index))
        nodes_[root].left = insert_node(nodes_[root].left, inserted);
    else
        nodes_[root].right = insert_node(nodes_[root].right, inserted);
    return rebalance(root);
}

std::size_t AnalyticIntervalIndex::minimum_node(std::size_t root) const noexcept
{
    while (nodes_[root].left != npos)
        root = nodes_[root].left;
    return root;
}

std::size_t AnalyticIntervalIndex::erase_node(std::size_t root, double minimum,
                                              std::uint32_t curve_index) noexcept
{
    if (root == npos)
        return npos;
    Node& current = nodes_[root];
    if (key_less(minimum, curve_index, current.minimum, current.curve_index))
        current.left = erase_node(current.left, minimum, curve_index);
    else if (key_less(current.minimum, current.curve_index, minimum, curve_index))
        current.right = erase_node(current.right, minimum, curve_index);
    else
    {
        if (current.left == npos)
            return current.right;
        if (current.right == npos)
            return current.left;
        const std::size_t successor_index = minimum_node(current.right);
        const Node successor = nodes_[successor_index];
        current.minimum = successor.minimum;
        current.maximum = successor.maximum;
        current.payload = successor.payload;
        current.curve_index = successor.curve_index;
        current.right = erase_node(current.right, successor.minimum, successor.curve_index);
    }
    return rebalance(root);
}

} // namespace geometer::detail

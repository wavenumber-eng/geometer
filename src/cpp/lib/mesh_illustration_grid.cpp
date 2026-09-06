#include "mesh_illustration_internal.h"

#include <algorithm>

namespace geometer::illustration_detail
{
void candidate_pairs(const std::vector<Bounds>& boxes, const Bounds& bounds, unsigned grid_size,
                     unsigned broad_limit, double minimum_cell, WorkBudget& budget,
                     const std::function<void(std::size_t, std::size_t)>& visit)
{
    const double cell_width = std::max((bounds.max_x - bounds.min_x) / grid_size, minimum_cell);
    const double cell_height = std::max((bounds.max_y - bounds.min_y) / grid_size, minimum_cell);
    if (!std::isfinite(cell_width) || !std::isfinite(cell_height))
        throw std::runtime_error("Illustration coordinate range overflow.");
    std::vector<std::vector<std::size_t>> buckets(grid_size * grid_size);
    std::vector<std::size_t> broad, marks(boxes.size(), 0);
    const auto cell = [grid_size](double value)
    { return static_cast<unsigned>(clamp(std::floor(value), 0, grid_size - 1)); };
    const auto check = [&](std::size_t index, std::size_t candidate)
    {
        budget.consume();
        visit(index, candidate);
    };
    for (std::size_t index = 0; index < boxes.size(); ++index)
    {
        const auto& box = boxes[index];
        const auto min_x = cell((box.min_x - bounds.min_x) / cell_width);
        const auto max_x = cell((box.max_x - bounds.min_x) / cell_width);
        const auto min_y = cell((box.min_y - bounds.min_y) / cell_height);
        const auto max_y = cell((box.max_y - bounds.min_y) / cell_height);
        if ((max_x - min_x + 1) * (max_y - min_y + 1) > broad_limit)
        {
            for (std::size_t candidate = 0; candidate < index; ++candidate)
                check(index, candidate);
            broad.push_back(index);
            continue;
        }
        for (auto candidate : broad)
            check(index, candidate);
        for (unsigned y = min_y; y <= max_y; ++y)
            for (unsigned x = min_x; x <= max_x; ++x)
                for (auto candidate : buckets[y * grid_size + x])
                {
                    if (marks[candidate] == index + 1)
                        continue;
                    marks[candidate] = index + 1;
                    check(index, candidate);
                }
        for (unsigned y = min_y; y <= max_y; ++y)
            for (unsigned x = min_x; x <= max_x; ++x)
                buckets[y * grid_size + x].push_back(index);
    }
}
} // namespace geometer::illustration_detail

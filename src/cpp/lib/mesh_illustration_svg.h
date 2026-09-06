#pragma once

#include "mesh_illustration_internal.h"

namespace geometer::illustration_detail
{
using PointMapper = std::function<Vec2(Vec2)>;
void append_bounded(std::string& target, const std::string& text, std::size_t maximum);
std::string chained_line_path(const std::vector<Line>& lines, std::size_t begin, std::size_t end,
                              const PointMapper& map_point);
} // namespace geometer::illustration_detail

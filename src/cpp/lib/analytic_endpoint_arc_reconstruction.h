#pragma once

#include "analytic_filtered_interval.h"
#include "analytic_wide_integer.h"

#include <cstdint>

namespace geometer::analytic_detail
{

inline bool reconstruct_endpoint_authoritative_arc_center(std::int64_t start_x,
                                                          std::int64_t start_y, std::int64_t end_x,
                                                          std::int64_t end_y, std::uint64_t radius,
                                                          bool counterclockwise, bool major_arc,
                                                          Point& center) noexcept
{
    const std::int64_t dx = end_x - start_x;
    const std::int64_t dy = end_y - start_y;
    if ((dx == 0 && dy == 0) || radius == 0 || radius > 1'000'000'000'000ULL)
        return false;
    const WideInteger chord_squared = wide_add(wide_multiply(dx, dx), wide_multiply(dy, dy));
    const auto radius_integer = static_cast<std::int64_t>(radius);
    const WideInteger radius_squared = wide_multiply(radius_integer, radius_integer);
    const WideInteger four_radius_squared = wide_add(wide_add(radius_squared, radius_squared),
                                                     wide_add(radius_squared, radius_squared));
    if (wide_compare(chord_squared, four_radius_squared) > 0)
        return false;

    const Interval chord =
        add(square(exact(static_cast<double>(dx))), square(exact(static_cast<double>(dy))));
    const Interval four_radius = multiply(exact(4.0), square(exact(static_cast<double>(radius))));
    const Interval root = square_root(divide(subtract(four_radius, chord), chord));
    if (!valid(root))
        return false;
    const Interval factor = multiply(exact(0.5), root);
    const Point start{exact(static_cast<double>(start_x)), exact(static_cast<double>(start_y))};
    const Point end{exact(static_cast<double>(end_x)), exact(static_cast<double>(end_y))};
    Point displacement = scale(
        perpendicular({exact(static_cast<double>(dx)), exact(static_cast<double>(dy))}), factor);
    if (counterclockwise == major_arc)
        displacement = scale(displacement, exact(-1.0));
    center = add(scale(add(start, end), exact(0.5)), displacement);
    return valid(center.x) && valid(center.y);
}

} // namespace geometer::analytic_detail

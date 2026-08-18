#include "geometer/exact_curve_domain.h"

#include <exception>
#include <utility>

namespace geometer::exact
{
namespace
{

bool valid_point(const ConstructionArena& arena, const ExactPoint& point)
{
    return static_cast<std::size_t>(point.x) < arena.size() &&
           static_cast<std::size_t>(point.y) < arena.size();
}

bool valid_line(const ConstructionArena& arena, const ExactLine& line)
{
    return valid_point(arena, line.first) && valid_point(arena, line.second);
}

bool valid_circle(const ConstructionArena& arena, const ExactCircle& circle)
{
    return valid_point(arena, circle.center) &&
           static_cast<std::size_t>(circle.radius) < arena.size();
}

ExactPredicateResult predicate_failure(Error error)
{
    return {error, std::nullopt};
}

ExactOrderingResult ordering_failure(Error error)
{
    return {error, std::nullopt};
}

ConstructionNodeId dot(ConstructionBuilder& builder, ConstructionNodeId ax, ConstructionNodeId ay,
                       ConstructionNodeId bx, ConstructionNodeId by)
{
    const ConstructionNodeId x_product = builder.product(ax, bx);
    const ConstructionNodeId y_product = builder.product(ay, by);
    return builder.sum(x_product, y_product);
}

ConstructionNodeId cross(ConstructionBuilder& builder, ConstructionNodeId ax, ConstructionNodeId ay,
                         ConstructionNodeId bx, ConstructionNodeId by)
{
    const ConstructionNodeId first = builder.product(ax, by);
    const ConstructionNodeId second = builder.product(ay, bx);
    return builder.subtract(first, second);
}

bool points_equal(ConstructionBuilder& builder, const ExactPoint& left, const ExactPoint& right)
{
    const std::int8_t x_order = builder.compare(left.x, right.x);
    const std::int8_t y_order = builder.compare(left.y, right.y);
    return builder.good() && x_order == 0 && y_order == 0;
}

struct VectorNodes
{
    ConstructionNodeId x = 0;
    ConstructionNodeId y = 0;
};

VectorNodes vector_from(ConstructionBuilder& builder, const ExactPoint& origin,
                        const ExactPoint& point)
{
    return {builder.subtract(point.x, origin.x), builder.subtract(point.y, origin.y)};
}

bool nondegenerate_line(ConstructionBuilder& builder, const ExactLine& line, VectorNodes& direction)
{
    direction = vector_from(builder, line.first, line.second);
    const ConstructionNodeId length =
        dot(builder, direction.x, direction.y, direction.x, direction.y);
    return builder.good() && builder.sign(length) > 0;
}

bool positive_circle(ConstructionBuilder& builder, const ExactCircle& circle)
{
    return builder.sign(circle.radius) > 0 && builder.good();
}

bool point_on_line(ConstructionBuilder& builder, const ExactLine& line,
                   const VectorNodes& direction, const ExactPoint& point)
{
    const VectorNodes offset = vector_from(builder, line.first, point);
    const ConstructionNodeId incidence =
        cross(builder, direction.x, direction.y, offset.x, offset.y);
    return builder.good() && builder.sign(incidence) == 0;
}

bool point_on_circle(ConstructionBuilder& builder, const ExactCircle& circle,
                     const ExactPoint& point)
{
    const VectorNodes offset = vector_from(builder, circle.center, point);
    const ConstructionNodeId distance = dot(builder, offset.x, offset.y, offset.x, offset.y);
    const ConstructionNodeId radius_squared = builder.square(circle.radius);
    const ConstructionNodeId difference = builder.subtract(distance, radius_squared);
    return builder.good() && builder.sign(difference) == 0;
}

std::int8_t compare_line_coordinates(ConstructionBuilder& builder, const VectorNodes& direction,
                                     const ExactPoint& left, const ExactPoint& right)
{
    const std::int8_t x_direction = builder.sign(direction.x);
    if (!builder.good())
        return 0;
    if (x_direction != 0)
        return builder.compare(left.x, right.x);
    return builder.compare(left.y, right.y);
}

std::int8_t circle_half(ConstructionBuilder& builder, const VectorNodes& value)
{
    const std::int8_t y_sign = builder.sign(value.y);
    if (!builder.good())
        return 0;
    if (y_sign < 0)
        return 0;
    if (y_sign > 0)
        return 1;
    return builder.sign(value.x) < 0 ? 0 : 1;
}

std::int8_t compare_circle_vectors(ConstructionBuilder& builder, const VectorNodes& left,
                                   const VectorNodes& right)
{
    const std::int8_t left_half = circle_half(builder, left);
    const std::int8_t right_half = circle_half(builder, right);
    if (!builder.good())
        return 0;
    if (left_half != right_half)
        return left_half < right_half ? -1 : 1;
    const ConstructionNodeId orientation = cross(builder, left.x, left.y, right.x, right.y);
    const std::int8_t orientation_sign = builder.sign(orientation);
    if (!builder.good())
        return 0;
    return orientation_sign > 0 ? -1 : orientation_sign < 0 ? 1 : 0;
}

ExactOrderingResult compare_circle_points_impl(ConstructionArena& arena, const ExactCircle& carrier,
                                               const ExactPoint& left, const ExactPoint& right)
{
    if (!valid_circle(arena, carrier) || !valid_point(arena, left) || !valid_point(arena, right))
        return ordering_failure(Error::invalid_argument);
    ConstructionArenaTransaction transaction(arena);
    ConstructionBuilder builder(arena);
    if (!positive_circle(builder, carrier) || !point_on_circle(builder, carrier, left) ||
        !point_on_circle(builder, carrier, right))
        return builder.good() ? ordering_failure(Error::invalid_argument)
                              : ordering_failure(builder.error());
    if (points_equal(builder, left, right))
        return builder.good() ? ExactOrderingResult{Error::none, 0}
                              : ordering_failure(builder.error());
    const VectorNodes left_vector = vector_from(builder, carrier.center, left);
    const VectorNodes right_vector = vector_from(builder, carrier.center, right);
    const std::int8_t order = compare_circle_vectors(builder, left_vector, right_vector);
    if (!builder.good() || order == 0)
        return ordering_failure(builder.good() ? Error::invalid_argument : builder.error());
    return {Error::none, order};
}

ExactPredicateResult same_line_impl(ConstructionArena& arena, const ExactLine& left,
                                    const ExactLine& right)
{
    if (!valid_line(arena, left) || !valid_line(arena, right))
        return predicate_failure(Error::invalid_argument);
    ConstructionArenaTransaction transaction(arena);
    ConstructionBuilder builder(arena);
    VectorNodes left_direction;
    VectorNodes right_direction;
    if (!nondegenerate_line(builder, left, left_direction) ||
        !nondegenerate_line(builder, right, right_direction))
        return builder.good() ? predicate_failure(Error::invalid_argument)
                              : predicate_failure(builder.error());
    const ConstructionNodeId direction_cross =
        cross(builder, left_direction.x, left_direction.y, right_direction.x, right_direction.y);
    if (builder.sign(direction_cross) != 0)
        return builder.good() ? ExactPredicateResult{Error::none, false}
                              : predicate_failure(builder.error());
    const VectorNodes offset = vector_from(builder, left.first, right.first);
    const ConstructionNodeId offset_cross =
        cross(builder, left_direction.x, left_direction.y, offset.x, offset.y);
    const std::int8_t offset_sign = builder.sign(offset_cross);
    return builder.good() ? ExactPredicateResult{Error::none, offset_sign == 0}
                          : predicate_failure(builder.error());
}

ExactPredicateResult same_circle_impl(ConstructionArena& arena, const ExactCircle& left,
                                      const ExactCircle& right)
{
    if (!valid_circle(arena, left) || !valid_circle(arena, right))
        return predicate_failure(Error::invalid_argument);
    ConstructionArenaTransaction transaction(arena);
    ConstructionBuilder builder(arena);
    if (!positive_circle(builder, left) || !positive_circle(builder, right))
        return builder.good() ? predicate_failure(Error::invalid_argument)
                              : predicate_failure(builder.error());
    const bool same_x = builder.compare(left.center.x, right.center.x) == 0;
    const bool same_y = builder.compare(left.center.y, right.center.y) == 0;
    const bool same_radius = builder.compare(left.radius, right.radius) == 0;
    return builder.good() ? ExactPredicateResult{Error::none, same_x && same_y && same_radius}
                          : predicate_failure(builder.error());
}

ExactPredicateResult segment_contains_impl(ConstructionArena& arena, const ExactLine& segment,
                                           const ExactPoint& point)
{
    if (!valid_line(arena, segment) || !valid_point(arena, point))
        return predicate_failure(Error::invalid_argument);
    ConstructionArenaTransaction transaction(arena);
    ConstructionBuilder builder(arena);
    VectorNodes direction;
    if (!nondegenerate_line(builder, segment, direction))
        return builder.good() ? predicate_failure(Error::invalid_argument)
                              : predicate_failure(builder.error());
    if (!point_on_line(builder, segment, direction, point))
        return builder.good() ? ExactPredicateResult{Error::none, false}
                              : predicate_failure(builder.error());
    const std::int8_t endpoints =
        compare_line_coordinates(builder, direction, segment.first, segment.second);
    const ExactPoint& lower = endpoints < 0 ? segment.first : segment.second;
    const ExactPoint& upper = endpoints < 0 ? segment.second : segment.first;
    const std::int8_t lower_order = compare_line_coordinates(builder, direction, lower, point);
    const std::int8_t upper_order = compare_line_coordinates(builder, direction, point, upper);
    return builder.good() ? ExactPredicateResult{Error::none, lower_order <= 0 && upper_order <= 0}
                          : predicate_failure(builder.error());
}

ExactOrderingResult line_order_impl(ConstructionArena& arena, const ExactLine& carrier,
                                    const ExactPoint& left, const ExactPoint& right)
{
    if (!valid_line(arena, carrier) || !valid_point(arena, left) || !valid_point(arena, right))
        return ordering_failure(Error::invalid_argument);
    ConstructionArenaTransaction transaction(arena);
    ConstructionBuilder builder(arena);
    VectorNodes direction;
    if (!nondegenerate_line(builder, carrier, direction) ||
        !point_on_line(builder, carrier, direction, left) ||
        !point_on_line(builder, carrier, direction, right))
        return builder.good() ? ordering_failure(Error::invalid_argument)
                              : ordering_failure(builder.error());
    const std::int8_t order = compare_line_coordinates(builder, direction, left, right);
    return builder.good() ? ExactOrderingResult{Error::none, order}
                          : ordering_failure(builder.error());
}

ExactPredicateResult arc_contains_impl(ConstructionArena& arena, const ExactCircularArc& arc,
                                       const ExactPoint& point)
{
    if (!valid_circle(arena, arc.circle) || !valid_point(arena, arc.start) ||
        !valid_point(arena, arc.end) || !valid_point(arena, point))
        return predicate_failure(Error::invalid_argument);
    ConstructionArenaTransaction transaction(arena);
    ConstructionBuilder builder(arena);
    if (!positive_circle(builder, arc.circle) || !point_on_circle(builder, arc.circle, arc.start) ||
        !point_on_circle(builder, arc.circle, arc.end))
        return builder.good() ? predicate_failure(Error::invalid_argument)
                              : predicate_failure(builder.error());
    if (points_equal(builder, arc.start, arc.end))
        return builder.good() ? predicate_failure(Error::invalid_argument)
                              : predicate_failure(builder.error());
    if (!point_on_circle(builder, arc.circle, point))
        return builder.good() ? ExactPredicateResult{Error::none, false}
                              : predicate_failure(builder.error());

    const ExactPoint& start = arc.counterclockwise ? arc.start : arc.end;
    const ExactPoint& end = arc.counterclockwise ? arc.end : arc.start;
    const VectorNodes start_vector = vector_from(builder, arc.circle.center, start);
    const VectorNodes end_vector = vector_from(builder, arc.circle.center, end);
    const ConstructionNodeId sweep_cross =
        cross(builder, start_vector.x, start_vector.y, end_vector.x, end_vector.y);
    const std::int8_t sweep_sign = builder.sign(sweep_cross);
    if (!builder.good() || arc.major_arc != (sweep_sign < 0))
        return builder.good() ? predicate_failure(Error::invalid_argument)
                              : predicate_failure(builder.error());

    const bool at_start = points_equal(builder, point, start);
    if (!builder.good())
        return predicate_failure(builder.error());
    const bool at_end = points_equal(builder, point, end);
    if (!builder.good())
        return predicate_failure(builder.error());
    if (at_start || at_end)
        return {Error::none, true};
    auto start_end = compare_circle_points_impl(arena, arc.circle, start, end);
    auto start_point = compare_circle_points_impl(arena, arc.circle, start, point);
    auto point_end = compare_circle_points_impl(arena, arc.circle, point, end);
    if (start_end.error != Error::none || start_point.error != Error::none ||
        point_end.error != Error::none || !start_end.ordering || !start_point.ordering ||
        !point_end.ordering)
        return predicate_failure(start_end.error != Error::none     ? start_end.error
                                 : start_point.error != Error::none ? start_point.error
                                                                    : point_end.error);
    const bool contained = *start_end.ordering < 0
                               ? *start_point.ordering <= 0 && *point_end.ordering <= 0
                               : *start_point.ordering <= 0 || *point_end.ordering <= 0;
    return {Error::none, contained};
}

} // namespace

ExactPredicateResult exact_points_equal(ConstructionArena& arena, const ExactPoint& left,
                                        const ExactPoint& right)
{
    try
    {
        if (!valid_point(arena, left) || !valid_point(arena, right))
            return predicate_failure(Error::invalid_argument);
        ConstructionArenaTransaction transaction(arena);
        ConstructionBuilder builder(arena);
        const bool equal = points_equal(builder, left, right);
        return builder.good() ? ExactPredicateResult{Error::none, equal}
                              : predicate_failure(builder.error());
    }
    catch (const std::exception&)
    {
        return predicate_failure(Error::resource_limit_exceeded);
    }
}

ExactPredicateResult same_exact_line_carrier(ConstructionArena& arena, const ExactLine& left,
                                             const ExactLine& right)
{
    try
    {
        return same_line_impl(arena, left, right);
    }
    catch (const std::exception&)
    {
        return predicate_failure(Error::resource_limit_exceeded);
    }
}

ExactPredicateResult same_exact_circle_carrier(ConstructionArena& arena, const ExactCircle& left,
                                               const ExactCircle& right)
{
    try
    {
        return same_circle_impl(arena, left, right);
    }
    catch (const std::exception&)
    {
        return predicate_failure(Error::resource_limit_exceeded);
    }
}

ExactPredicateResult point_on_closed_exact_segment(ConstructionArena& arena,
                                                   const ExactLine& segment,
                                                   const ExactPoint& point)
{
    try
    {
        return segment_contains_impl(arena, segment, point);
    }
    catch (const std::exception&)
    {
        return predicate_failure(Error::resource_limit_exceeded);
    }
}

ExactPredicateResult point_on_closed_exact_arc(ConstructionArena& arena,
                                               const ExactCircularArc& arc, const ExactPoint& point)
{
    try
    {
        return arc_contains_impl(arena, arc, point);
    }
    catch (const std::exception&)
    {
        return predicate_failure(Error::resource_limit_exceeded);
    }
}

ExactOrderingResult compare_points_on_exact_line(ConstructionArena& arena, const ExactLine& carrier,
                                                 const ExactPoint& left, const ExactPoint& right)
{
    try
    {
        return line_order_impl(arena, carrier, left, right);
    }
    catch (const std::exception&)
    {
        return ordering_failure(Error::resource_limit_exceeded);
    }
}

ExactOrderingResult compare_points_on_exact_circle(ConstructionArena& arena,
                                                   const ExactCircle& carrier,
                                                   const ExactPoint& left, const ExactPoint& right)
{
    try
    {
        return compare_circle_points_impl(arena, carrier, left, right);
    }
    catch (const std::exception&)
    {
        return ordering_failure(Error::resource_limit_exceeded);
    }
}

} // namespace geometer::exact

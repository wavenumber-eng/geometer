#include "exact_arrangement_internal.h"

#include <exception>
#include <utility>

namespace geometer::exact::arrangement_detail
{
namespace
{

Ordering failure(Error error)
{
    return {error, std::nullopt};
}

std::int8_t compare_u32(std::uint32_t left, std::uint32_t right)
{
    return left < right ? -1 : left > right ? 1 : 0;
}

std::int8_t compare_bool(bool left, bool right)
{
    return left == right ? 0 : left ? 1 : -1;
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

ConstructionNodeId cross(ConstructionBuilder& builder, const VectorNodes& left,
                         const VectorNodes& right)
{
    const ConstructionNodeId first = builder.product(left.x, right.y);
    const ConstructionNodeId second = builder.product(left.y, right.x);
    return builder.subtract(first, second);
}

ConstructionNodeId dot(ConstructionBuilder& builder, const VectorNodes& left,
                       const VectorNodes& right)
{
    const ConstructionNodeId x_product = builder.product(left.x, right.x);
    const ConstructionNodeId y_product = builder.product(left.y, right.y);
    return builder.sum(x_product, y_product);
}

struct Tangent
{
    VectorNodes direction;
    std::int8_t curvature_sign = 0;
    ConstructionNodeId radius = 0;
};

std::int8_t direction_half(ConstructionBuilder& builder, const Tangent& tangent)
{
    const std::int8_t y_sign = builder.sign(tangent.direction.y);
    if (!builder.good())
        return 0;
    if (y_sign > 0)
        return 0;
    if (y_sign < 0)
        return 1;
    if (builder.sign(tangent.direction.x) < 0)
        return 1;
    // A clockwise germ whose tangent lies exactly on the positive x axis dips
    // infinitesimally below that axis, so it wraps past both halves to the very
    // end of the rotational order. The other direction-half boundaries are not
    // wrap points: infinitesimal curvature perturbations there stay adjacent in
    // the circular order, so the exact tangent direction decides the half.
    return tangent.curvature_sign < 0 ? 2 : 0;
}

Tangent outgoing_tangent(ConstructionBuilder& builder, std::uint32_t half_edge_id,
                         const std::vector<ExactArrangementVertex>& vertices,
                         const std::vector<ExactArrangementEdge>& edges,
                         const std::vector<ExactArrangementHalfEdge>& half_edges)
{
    const ExactArrangementHalfEdge& half_edge = half_edges[half_edge_id];
    const ExactArrangementEdge& edge = edges[half_edge.edge];
    const std::uint32_t target_vertex = half_edge.forward ? edge.end_vertex : edge.start_vertex;
    const ExactPoint& origin = vertices[half_edge.origin_vertex].point;
    const ExactPoint& target = vertices[target_vertex].point;
    if (edge.kind == ExactAtomicCurveKind::line)
        return {vector_from(builder, origin, target), 0, 0};

    const VectorNodes radial = vector_from(builder, edge.circle.center, origin);
    const bool counterclockwise =
        half_edge.forward ? edge.counterclockwise : !edge.counterclockwise;
    if (counterclockwise)
        return {{builder.negate(radial.y), radial.x}, 1, edge.circle.radius};
    return {{radial.y, builder.negate(radial.x)}, -1, edge.circle.radius};
}

std::int8_t compare_curvature(ConstructionBuilder& builder, const Tangent& left,
                              const Tangent& right)
{
    if (left.curvature_sign != right.curvature_sign)
        return left.curvature_sign < right.curvature_sign ? -1 : 1;
    if (left.curvature_sign == 0)
        return 0;
    const std::int8_t radius_order = builder.compare(left.radius, right.radius);
    if (!builder.good())
        return 0;
    return left.curvature_sign > 0 ? static_cast<std::int8_t>(-radius_order) : radius_order;
}

} // namespace

Ordering compare_points(ConstructionArena& arena, const ExactPoint& left, const ExactPoint& right)
{
    try
    {
        if (static_cast<std::size_t>(left.x) >= arena.size() ||
            static_cast<std::size_t>(left.y) >= arena.size() ||
            static_cast<std::size_t>(right.x) >= arena.size() ||
            static_cast<std::size_t>(right.y) >= arena.size())
            return failure(Error::invalid_argument);
        ConstructionArenaTransaction transaction(arena);
        ConstructionBuilder builder(arena);
        const std::int8_t x_order = builder.compare(left.x, right.x);
        if (!builder.good())
            return failure(builder.error());
        const std::int8_t y_order = builder.compare(left.y, right.y);
        if (!builder.good())
            return failure(builder.error());
        return {Error::none, x_order != 0 ? x_order : y_order};
    }
    catch (const std::exception&)
    {
        return failure(Error::resource_limit_exceeded);
    }
}

Error validate_curve(ConstructionArena& arena, const ExactAtomicCurve& curve)
{
    if (curve.kind == ExactAtomicCurveKind::line)
    {
        const Ordering endpoints = compare_points(arena, curve.start, curve.end);
        if (endpoints.error != Error::none || !endpoints.value)
            return endpoints.error == Error::none ? Error::invalid_argument : endpoints.error;
        return *endpoints.value == 0 ? Error::invalid_argument : Error::none;
    }
    if (curve.kind != ExactAtomicCurveKind::circular_arc)
        return Error::invalid_argument;
    const ExactCircularArc arc{curve.circle, curve.start, curve.end, curve.counterclockwise,
                               curve.major_arc};
    const ExactPredicateResult start = point_on_closed_exact_arc(arena, arc, curve.start);
    if (start.error != Error::none || !start.value || !*start.value)
        return start.error == Error::none ? Error::invalid_argument : start.error;
    const ExactPredicateResult end = point_on_closed_exact_arc(arena, arc, curve.end);
    if (end.error != Error::none || !end.value || !*end.value)
        return end.error == Error::none ? Error::invalid_argument : end.error;
    return Error::none;
}

Ordering compare_edges(ConstructionArena& arena, const EdgeDraft& left, const EdgeDraft& right)
{
    const std::int8_t start_order = compare_u32(left.start_vertex, right.start_vertex);
    if (start_order != 0)
        return {Error::none, start_order};
    const std::int8_t end_order = compare_u32(left.end_vertex, right.end_vertex);
    if (end_order != 0)
        return {Error::none, end_order};
    const auto left_kind = static_cast<std::uint8_t>(left.kind);
    const auto right_kind = static_cast<std::uint8_t>(right.kind);
    if (left_kind != right_kind)
        return {Error::none, left_kind < right_kind ? -1 : 1};
    if (left.kind == ExactAtomicCurveKind::line)
        return {Error::none, 0};
    try
    {
        ConstructionArenaTransaction transaction(arena);
        ConstructionBuilder builder(arena);
        for (const auto [left_value, right_value] : {
                 std::pair{left.circle.center.x, right.circle.center.x},
                 std::pair{left.circle.center.y, right.circle.center.y},
                 std::pair{left.circle.radius, right.circle.radius},
             })
        {
            const std::int8_t order = builder.compare(left_value, right_value);
            if (!builder.good())
                return failure(builder.error());
            if (order != 0)
                return {Error::none, order};
        }
        const std::int8_t direction_order =
            compare_bool(left.counterclockwise, right.counterclockwise);
        if (direction_order != 0)
            return {Error::none, direction_order};
        return {Error::none, compare_bool(left.major_arc, right.major_arc)};
    }
    catch (const std::exception&)
    {
        return failure(Error::resource_limit_exceeded);
    }
}

Ordering compare_outgoing(ConstructionArena& arena, std::uint32_t left_half_edge,
                          std::uint32_t right_half_edge,
                          const std::vector<ExactArrangementVertex>& vertices,
                          const std::vector<ExactArrangementEdge>& edges,
                          const std::vector<ExactArrangementHalfEdge>& half_edges)
{
    try
    {
        ConstructionArenaTransaction transaction(arena);
        ConstructionBuilder builder(arena);
        const Tangent left = outgoing_tangent(builder, left_half_edge, vertices, edges, half_edges);
        const Tangent right =
            outgoing_tangent(builder, right_half_edge, vertices, edges, half_edges);
        const std::int8_t left_half = direction_half(builder, left);
        const std::int8_t right_half = direction_half(builder, right);
        if (!builder.good())
            return failure(builder.error());
        if (left_half != right_half)
            return {Error::none, left_half < right_half ? -1 : 1};
        const std::int8_t orientation =
            builder.sign(cross(builder, left.direction, right.direction));
        if (!builder.good())
            return failure(builder.error());
        if (orientation != 0)
            return {Error::none, orientation > 0 ? -1 : 1};
        const std::int8_t direction = builder.sign(dot(builder, left.direction, right.direction));
        if (!builder.good() || direction <= 0)
            return failure(builder.good() ? Error::invalid_argument : builder.error());
        const std::int8_t curvature = compare_curvature(builder, left, right);
        if (!builder.good())
            return failure(builder.error());
        return {Error::none, curvature};
    }
    catch (const std::exception&)
    {
        return failure(Error::resource_limit_exceeded);
    }
}

Ordering compare_cycle_germs_at_minimum(ConstructionArena& arena, std::uint32_t outgoing_half_edge,
                                        std::uint32_t reverse_incoming_half_edge,
                                        const std::vector<ExactArrangementVertex>& vertices,
                                        const std::vector<ExactArrangementEdge>& edges,
                                        const std::vector<ExactArrangementHalfEdge>& half_edges)
{
    try
    {
        ConstructionArenaTransaction transaction(arena);
        ConstructionBuilder builder(arena);
        const Tangent outgoing =
            outgoing_tangent(builder, outgoing_half_edge, vertices, edges, half_edges);
        const Tangent incoming =
            outgoing_tangent(builder, reverse_incoming_half_edge, vertices, edges, half_edges);
        const std::int8_t outgoing_x = builder.sign(outgoing.direction.x);
        const std::int8_t incoming_x = builder.sign(incoming.direction.x);
        const std::int8_t outgoing_y = builder.sign(outgoing.direction.y);
        const std::int8_t incoming_y = builder.sign(incoming.direction.y);
        if (!builder.good())
            return failure(builder.error());
        if (outgoing_x < 0 || incoming_x < 0)
            return failure(Error::invalid_argument);
        const std::int8_t outgoing_half = outgoing_y < 0 ? 0 : 1;
        const std::int8_t incoming_half = incoming_y < 0 ? 0 : 1;
        if (outgoing_half != incoming_half)
            return {Error::none, outgoing_half < incoming_half ? -1 : 1};
        const std::int8_t orientation =
            builder.sign(cross(builder, outgoing.direction, incoming.direction));
        if (!builder.good())
            return failure(builder.error());
        if (orientation != 0)
            return {Error::none, orientation > 0 ? -1 : 1};
        const std::int8_t direction =
            builder.sign(dot(builder, outgoing.direction, incoming.direction));
        if (!builder.good() || direction <= 0)
            return failure(builder.good() ? Error::invalid_argument : builder.error());
        const std::int8_t curvature = compare_curvature(builder, outgoing, incoming);
        if (!builder.good())
            return failure(builder.error());
        return {Error::none, curvature};
    }
    catch (const std::exception&)
    {
        return failure(Error::resource_limit_exceeded);
    }
}

} // namespace geometer::exact::arrangement_detail

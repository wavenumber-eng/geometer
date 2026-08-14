#include "exact_arrangement_internal.h"

#include <algorithm>
#include <exception>
#include <limits>
#include <optional>
#include <utility>

namespace geometer::exact::arrangement_detail
{
namespace
{

struct CycleDraft
{
    std::vector<std::uint32_t> half_edges;
    std::uint32_t component = 0;
    bool counterclockwise = true;
};

struct DisjointSet
{
    explicit DisjointSet(std::size_t count) : parents(count)
    {
        for (std::size_t index = 0; index < count; ++index)
            parents[index] = static_cast<std::uint32_t>(index);
    }

    std::uint32_t find(std::uint32_t value)
    {
        while (parents[value] != value)
        {
            parents[value] = parents[parents[value]];
            value = parents[value];
        }
        return value;
    }

    void unite(std::uint32_t left, std::uint32_t right)
    {
        left = find(left);
        right = find(right);
        if (left == right)
            return;
        if (right < left)
            std::swap(left, right);
        parents[right] = left;
    }

    std::vector<std::uint32_t> parents;
};

ExactPredicateResult failure(Error error)
{
    return {error, std::nullopt};
}

ExactPredicateResult point_in_cycle(ConstructionArena& arena, const ExactPoint& query,
                                    const CycleDraft& cycle,
                                    const std::vector<ExactArrangementVertex>& vertices,
                                    const std::vector<ExactArrangementEdge>& edges,
                                    const std::vector<ExactArrangementHalfEdge>& half_edges)
{
    try
    {
        ConstructionArenaTransaction transaction(arena);
        ConstructionBuilder builder(arena);
        const ConstructionNodeId one = builder.rational(1);
        if (!builder.good())
            return failure(builder.error());

        ExactLine ray;
        bool ray_selected = false;
        for (std::size_t slope_value = 0; slope_value <= cycle.half_edges.size(); ++slope_value)
        {
            const ConstructionNodeId slope = builder.rational(BigInt(slope_value));
            const ConstructionNodeId end_x = builder.sum(query.x, one);
            const ConstructionNodeId end_y = builder.sum(query.y, slope);
            if (!builder.good())
                return failure(builder.error());
            ray = {query, {end_x, end_y}};
            bool hits_vertex = false;
            for (const std::uint32_t half_edge_id : cycle.half_edges)
            {
                const ExactPoint& vertex = vertices[half_edges[half_edge_id].origin_vertex].point;
                const ConstructionNodeId dx = builder.subtract(vertex.x, query.x);
                const ConstructionNodeId dy = builder.subtract(vertex.y, query.y);
                const ConstructionNodeId slope_dx = builder.product(slope, dx);
                const ConstructionNodeId cross = builder.subtract(dy, slope_dx);
                const std::int8_t sign = builder.sign(cross);
                if (!builder.good())
                    return failure(builder.error());
                if (sign == 0)
                {
                    hits_vertex = true;
                    break;
                }
            }
            if (!hits_vertex)
            {
                ray_selected = true;
                break;
            }
        }
        if (!ray_selected)
            return failure(Error::invalid_argument);

        bool inside = false;
        for (const std::uint32_t half_edge_id : cycle.half_edges)
        {
            const ExactArrangementHalfEdge& half_edge = half_edges[half_edge_id];
            const ExactArrangementEdge& edge = edges[half_edge.edge];
            const ExactPoint& start = vertices[edge.start_vertex].point;
            const ExactPoint& end = vertices[edge.end_vertex].point;
            ExactIntersectionResult intersections;
            if (edge.kind == ExactAtomicCurveKind::line)
                intersections = intersect_exact_lines(arena, ray, {start, end});
            else
                intersections = intersect_exact_line_circle(arena, ray, edge.circle);
            if (intersections.error != Error::none)
                return failure(intersections.error);
            if (intersections.relation == IntersectionRelation::coincident)
                return failure(Error::invalid_argument);
            if (edge.kind == ExactAtomicCurveKind::circular_arc &&
                intersections.relation == IntersectionRelation::point)
                continue;

            for (std::uint32_t index = 0; index < intersections.point_count; ++index)
            {
                const ExactPoint& point = intersections.points[index];
                const ExactPredicateResult contained =
                    edge.kind == ExactAtomicCurveKind::line
                        ? point_on_closed_exact_segment(arena, {start, end}, point)
                        : point_on_closed_exact_arc(
                              arena,
                              {edge.circle, start, end, edge.counterclockwise, edge.major_arc},
                              point);
                if (contained.error != Error::none || !contained.value)
                    return failure(contained.error == Error::none ? Error::invalid_argument
                                                                  : contained.error);
                if (!*contained.value)
                    continue;
                const ConstructionNodeId dx = builder.subtract(point.x, query.x);
                const ConstructionNodeId dy = builder.subtract(point.y, query.y);
                const ConstructionNodeId ray_dx = builder.subtract(ray.second.x, ray.first.x);
                const ConstructionNodeId ray_dy = builder.subtract(ray.second.y, ray.first.y);
                const ConstructionNodeId x_projection = builder.product(dx, ray_dx);
                const ConstructionNodeId y_projection = builder.product(dy, ray_dy);
                const ConstructionNodeId projection = builder.sum(x_projection, y_projection);
                const std::int8_t forward = builder.sign(projection);
                if (!builder.good())
                    return failure(builder.error());
                if (forward > 0)
                    inside = !inside;
            }
        }
        return {Error::none, inside};
    }
    catch (const std::exception&)
    {
        return failure(Error::resource_limit_exceeded);
    }
}

struct ArcMinimumOrientation
{
    Error error = Error::none;
    std::optional<bool> counterclockwise;
};

ArcMinimumOrientation
orientation_from_interior_arc_minimum(ConstructionArena& arena, const CycleDraft& draft,
                                      const std::vector<ExactArrangementVertex>& vertices,
                                      const std::vector<ExactArrangementEdge>& edges,
                                      const std::vector<ExactArrangementHalfEdge>& half_edges)
{
    try
    {
        ConstructionArenaTransaction transaction(arena);
        ConstructionBuilder builder(arena);
        ExactPoint minimum = vertices[half_edges[draft.half_edges.front()].origin_vertex].point;
        std::optional<bool> orientation;
        for (const std::uint32_t half_edge_id : draft.half_edges)
        {
            const ExactArrangementHalfEdge& half_edge = half_edges[half_edge_id];
            const ExactArrangementEdge& edge = edges[half_edge.edge];
            if (edge.kind != ExactAtomicCurveKind::circular_arc)
                continue;
            const ExactPoint leftmost{builder.subtract(edge.circle.center.x, edge.circle.radius),
                                      edge.circle.center.y};
            if (!builder.good())
                return {builder.error(), std::nullopt};
            const ExactPoint& start = vertices[edge.start_vertex].point;
            const ExactPoint& end = vertices[edge.end_vertex].point;
            const ExactPredicateResult on_arc = point_on_closed_exact_arc(
                arena, {edge.circle, start, end, edge.counterclockwise, edge.major_arc}, leftmost);
            const ExactPredicateResult at_start = exact_points_equal(arena, leftmost, start);
            const ExactPredicateResult at_end = exact_points_equal(arena, leftmost, end);
            if (on_arc.error != Error::none || at_start.error != Error::none ||
                at_end.error != Error::none || !on_arc.value || !at_start.value || !at_end.value)
                return {on_arc.error != Error::none     ? on_arc.error
                        : at_start.error != Error::none ? at_start.error
                        : at_end.error != Error::none   ? at_end.error
                                                        : Error::invalid_argument,
                        std::nullopt};
            if (!*on_arc.value || *at_start.value || *at_end.value)
                continue;
            const Ordering order = compare_points(arena, leftmost, minimum);
            if (order.error != Error::none || !order.value)
                return {order.error == Error::none ? Error::invalid_argument : order.error,
                        std::nullopt};
            if (*order.value > 0)
                continue;
            const bool directed_counterclockwise =
                half_edge.forward ? edge.counterclockwise : !edge.counterclockwise;
            if (*order.value < 0)
            {
                minimum = leftmost;
                orientation = directed_counterclockwise;
            }
            else if (orientation && *orientation != directed_counterclockwise)
                return {Error::invalid_argument, std::nullopt};
        }
        return {Error::none, orientation};
    }
    catch (const std::exception&)
    {
        return {Error::resource_limit_exceeded, std::nullopt};
    }
}

Error enumerate_cycles(ConstructionArena& arena,
                       const std::vector<ExactArrangementVertex>& vertices,
                       const std::vector<ExactArrangementEdge>& edges,
                       std::vector<ExactArrangementHalfEdge>& half_edges,
                       std::vector<CycleDraft>& drafts,
                       std::vector<std::uint32_t>& component_vertices)
{
    DisjointSet components(vertices.size());
    for (const ExactArrangementEdge& edge : edges)
        components.unite(edge.start_vertex, edge.end_vertex);
    std::vector<std::uint32_t> component_by_root(vertices.size(),
                                                 std::numeric_limits<std::uint32_t>::max());
    for (std::uint32_t vertex = 0; vertex < vertices.size(); ++vertex)
    {
        const std::uint32_t root = components.find(vertex);
        if (component_by_root[root] == std::numeric_limits<std::uint32_t>::max())
        {
            component_by_root[root] = static_cast<std::uint32_t>(component_vertices.size());
            component_vertices.push_back(vertex);
        }
    }

    std::vector<bool> visited(half_edges.size());
    for (std::uint32_t start = 0; start < half_edges.size(); ++start)
    {
        if (visited[start])
            continue;
        CycleDraft draft;
        std::uint32_t current = start;
        while (!visited[current])
        {
            visited[current] = true;
            draft.half_edges.push_back(current);
            current = half_edges[current].next;
            if (draft.half_edges.size() > half_edges.size())
                return Error::invalid_argument;
        }
        if (current != start || draft.half_edges.empty())
            return Error::invalid_argument;

        auto canonical = std::min_element(
            draft.half_edges.begin(), draft.half_edges.end(),
            [&half_edges](std::uint32_t left, std::uint32_t right)
            {
                const std::uint32_t left_vertex = half_edges[left].origin_vertex;
                const std::uint32_t right_vertex = half_edges[right].origin_vertex;
                return left_vertex != right_vertex ? left_vertex < right_vertex : left < right;
            });
        std::rotate(draft.half_edges.begin(), canonical, draft.half_edges.end());
        const ArcMinimumOrientation arc_orientation =
            orientation_from_interior_arc_minimum(arena, draft, vertices, edges, half_edges);
        if (arc_orientation.error != Error::none)
            return arc_orientation.error;
        const std::uint32_t outgoing = draft.half_edges.front();
        if (arc_orientation.counterclockwise)
            draft.counterclockwise = *arc_orientation.counterclockwise;
        else
        {
            const std::uint32_t reverse_incoming = half_edges[half_edges[outgoing].previous].twin;
            const Ordering order = compare_cycle_germs_at_minimum(arena, outgoing, reverse_incoming,
                                                                  vertices, edges, half_edges);
            if (order.error != Error::none || !order.value || *order.value == 0)
                return order.error == Error::none ? Error::invalid_argument : order.error;
            draft.counterclockwise = *order.value < 0;
        }
        const std::uint32_t root = components.find(half_edges[outgoing].origin_vertex);
        draft.component = component_by_root[root];
        drafts.push_back(std::move(draft));
    }
    std::sort(drafts.begin(), drafts.end(), [](const CycleDraft& left, const CycleDraft& right)
              { return left.half_edges.front() < right.half_edges.front(); });
    return Error::none;
}

} // namespace

Error build_face_topology(ConstructionArena& arena,
                          const std::vector<ExactArrangementVertex>& vertices,
                          const std::vector<ExactArrangementEdge>& edges,
                          std::vector<ExactArrangementHalfEdge>& half_edges,
                          std::vector<ExactArrangementCycle>& cycles,
                          std::vector<std::uint32_t>& cycle_half_edges,
                          std::vector<ExactArrangementFace>& faces,
                          std::vector<std::uint32_t>& face_boundary_cycles)
{
    try
    {
        std::vector<CycleDraft> drafts;
        std::vector<std::uint32_t> component_vertices;
        if (const Error error =
                enumerate_cycles(arena, vertices, edges, half_edges, drafts, component_vertices);
            error != Error::none)
            return error;
        std::vector<std::uint32_t> exterior_by_component(component_vertices.size(),
                                                         std::numeric_limits<std::uint32_t>::max());
        cycles.reserve(drafts.size());
        for (std::uint32_t cycle_id = 0; cycle_id < drafts.size(); ++cycle_id)
        {
            CycleDraft& draft = drafts[cycle_id];
            const std::uint32_t begin = static_cast<std::uint32_t>(cycle_half_edges.size());
            for (const std::uint32_t half_edge : draft.half_edges)
            {
                cycle_half_edges.push_back(half_edge);
                half_edges[half_edge].cycle = cycle_id;
            }
            cycles.push_back({begin, static_cast<std::uint32_t>(draft.half_edges.size()),
                              draft.component, 0, draft.counterclockwise});
            if (!draft.counterclockwise)
            {
                if (exterior_by_component[draft.component] !=
                    std::numeric_limits<std::uint32_t>::max())
                    return Error::invalid_argument;
                exterior_by_component[draft.component] = cycle_id;
            }
        }
        if (std::find(exterior_by_component.begin(), exterior_by_component.end(),
                      std::numeric_limits<std::uint32_t>::max()) != exterior_by_component.end())
            return Error::invalid_argument;

        faces.push_back({0, 0, 0, 0, true});
        std::vector<std::vector<std::uint32_t>> boundaries(1);
        for (std::uint32_t cycle_id = 0; cycle_id < cycles.size(); ++cycle_id)
        {
            if (!cycles[cycle_id].counterclockwise)
                continue;
            const std::uint32_t face = static_cast<std::uint32_t>(faces.size());
            cycles[cycle_id].face = face;
            faces.push_back({0, 0, 0, 0, false});
            boundaries.emplace_back(1, cycle_id);
            for (const std::uint32_t half_edge : drafts[cycle_id].half_edges)
                half_edges[half_edge].face = face;
        }

        for (std::uint32_t component = 0; component < component_vertices.size(); ++component)
        {
            const std::uint32_t exterior = exterior_by_component[component];
            const ExactPoint& query = vertices[component_vertices[component]].point;
            std::optional<std::uint32_t> best;
            for (std::uint32_t candidate = 0; candidate < cycles.size(); ++candidate)
            {
                if (!cycles[candidate].counterclockwise || cycles[candidate].component == component)
                    continue;
                const ExactPredicateResult contains =
                    point_in_cycle(arena, query, drafts[candidate], vertices, edges, half_edges);
                if (contains.error != Error::none || !contains.value)
                    return contains.error == Error::none ? Error::invalid_argument : contains.error;
                if (!*contains.value)
                    continue;
                if (!best)
                {
                    best = candidate;
                    continue;
                }
                const ExactPoint& candidate_query =
                    vertices[component_vertices[cycles[candidate].component]].point;
                const ExactPredicateResult candidate_inside_best = point_in_cycle(
                    arena, candidate_query, drafts[*best], vertices, edges, half_edges);
                if (candidate_inside_best.error != Error::none || !candidate_inside_best.value)
                    return candidate_inside_best.error == Error::none ? Error::invalid_argument
                                                                      : candidate_inside_best.error;
                if (*candidate_inside_best.value)
                    best = candidate;
            }
            const std::uint32_t face = best ? cycles[*best].face : 0;
            cycles[exterior].face = face;
            boundaries[face].push_back(exterior);
            for (const std::uint32_t half_edge : drafts[exterior].half_edges)
                half_edges[half_edge].face = face;
        }

        for (std::uint32_t face_id = 0; face_id < faces.size(); ++face_id)
        {
            auto& face_boundaries = boundaries[face_id];
            if (face_id == 0)
                std::sort(face_boundaries.begin(), face_boundaries.end());
            else if (face_boundaries.size() > 1)
                std::sort(face_boundaries.begin() + 1, face_boundaries.end());
            faces[face_id].boundary_cycle_begin =
                static_cast<std::uint32_t>(face_boundary_cycles.size());
            faces[face_id].boundary_cycle_count =
                static_cast<std::uint32_t>(face_boundaries.size());
            face_boundary_cycles.insert(face_boundary_cycles.end(), face_boundaries.begin(),
                                        face_boundaries.end());
        }
        return Error::none;
    }
    catch (const std::exception&)
    {
        return Error::resource_limit_exceeded;
    }
}

} // namespace geometer::exact::arrangement_detail

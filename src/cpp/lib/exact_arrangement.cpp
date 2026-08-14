#include "exact_arrangement_internal.h"

#include <algorithm>
#include <exception>
#include <limits>
#include <stdexcept>
#include <utility>

namespace geometer::exact
{
namespace
{

constexpr std::uint64_t kMaximumAtomicCurves = 131'072;
constexpr std::uint64_t kMaximumCurvePairs = 8'388'608;
constexpr std::uint64_t kMaximumMemberships = 8'388'608;

std::uint64_t checked_add(std::uint64_t left, std::uint64_t right)
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
        throw std::overflow_error("arrangement size addition overflow");
    return left + right;
}

std::uint64_t checked_multiply(std::uint64_t left, std::uint64_t right)
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left)
        throw std::overflow_error("arrangement size multiplication overflow");
    return left * right;
}

class StorageReservation
{
  public:
    StorageReservation(Budget& budget, std::uint64_t bytes) : budget_(&budget), bytes_(bytes)
    {
        acquired_ = budget.acquire_storage(bytes);
    }
    ~StorageReservation()
    {
        if (acquired_ && budget_ != nullptr)
            budget_->release_storage(bytes_);
    }
    StorageReservation(const StorageReservation&) = delete;
    StorageReservation& operator=(const StorageReservation&) = delete;

    [[nodiscard]] bool acquired() const
    {
        return acquired_;
    }
    [[nodiscard]] std::uint64_t transfer()
    {
        budget_ = nullptr;
        return bytes_;
    }

  private:
    Budget* budget_ = nullptr;
    std::uint64_t bytes_ = 0;
    bool acquired_ = false;
};

struct EndpointDraft
{
    ExactPoint point;
    std::uint32_t curve = 0;
    bool start = true;
};

Error normalize_memberships(std::vector<ExactCurveMembership>& memberships)
{
    for (std::size_t index = 1; index < memberships.size(); ++index)
    {
        ExactCurveMembership value = memberships[index];
        std::size_t insertion = index;
        while (insertion > 0 &&
               (value.occurrence_id < memberships[insertion - 1].occurrence_id ||
                (value.occurrence_id == memberships[insertion - 1].occurrence_id &&
                 value.agrees_with_carrier < memberships[insertion - 1].agrees_with_carrier)))
        {
            memberships[insertion] = memberships[insertion - 1];
            --insertion;
        }
        memberships[insertion] = value;
    }
    std::size_t output = 0;
    for (const ExactCurveMembership membership : memberships)
    {
        if (membership.occurrence_id == 0)
            return Error::invalid_argument;
        if (output != 0 && memberships[output - 1].occurrence_id == membership.occurrence_id)
        {
            if (memberships[output - 1].agrees_with_carrier != membership.agrees_with_carrier)
                return Error::invalid_argument;
            continue;
        }
        memberships[output++] = membership;
    }
    memberships.resize(output);
    return Error::none;
}

Error sort_endpoints(ConstructionArena& arena, std::vector<EndpointDraft>& endpoints)
{
    for (std::size_t index = 1; index < endpoints.size(); ++index)
    {
        EndpointDraft value = endpoints[index];
        std::size_t insertion = index;
        while (insertion > 0)
        {
            const arrangement_detail::Ordering order = arrangement_detail::compare_points(
                arena, value.point, endpoints[insertion - 1].point);
            if (order.error != Error::none || !order.value)
                return order.error == Error::none ? Error::invalid_argument : order.error;
            if (*order.value >= 0)
                break;
            endpoints[insertion] = endpoints[insertion - 1];
            --insertion;
        }
        endpoints[insertion] = value;
    }
    return Error::none;
}

Error sort_edges(ConstructionArena& arena, std::vector<arrangement_detail::EdgeDraft>& edges)
{
    for (std::size_t index = 1; index < edges.size(); ++index)
    {
        arrangement_detail::EdgeDraft value = std::move(edges[index]);
        std::size_t insertion = index;
        while (insertion > 0)
        {
            const arrangement_detail::Ordering order =
                arrangement_detail::compare_edges(arena, value, edges[insertion - 1]);
            if (order.error != Error::none || !order.value)
                return order.error == Error::none ? Error::invalid_argument : order.error;
            if (*order.value >= 0)
                break;
            edges[insertion] = std::move(edges[insertion - 1]);
            --insertion;
        }
        edges[insertion] = std::move(value);
    }
    return Error::none;
}

Error sort_outgoing(ConstructionArena& arena, const std::vector<ExactArrangementVertex>& vertices,
                    const std::vector<ExactArrangementEdge>& edges,
                    const std::vector<ExactArrangementHalfEdge>& half_edges,
                    std::vector<std::uint32_t>& outgoing)
{
    for (const ExactArrangementVertex& vertex : vertices)
    {
        const std::size_t begin = vertex.outgoing_begin;
        const std::size_t end = begin + vertex.outgoing_count;
        for (std::size_t index = begin + 1; index < end; ++index)
        {
            const std::uint32_t value = outgoing[index];
            std::size_t insertion = index;
            while (insertion > begin)
            {
                const arrangement_detail::Ordering order = arrangement_detail::compare_outgoing(
                    arena, value, outgoing[insertion - 1], vertices, edges, half_edges);
                if (order.error != Error::none || !order.value || *order.value == 0)
                    return order.error == Error::none ? Error::invalid_argument : order.error;
                if (*order.value > 0)
                    break;
                outgoing[insertion] = outgoing[insertion - 1];
                --insertion;
            }
            outgoing[insertion] = value;
        }
    }
    return Error::none;
}

ExactArrangementResult failure(Error error)
{
    return {error, std::nullopt};
}

} // namespace

ExactArrangement::ExactArrangement(Budget& budget, std::uint64_t charged_bytes,
                                   std::vector<ExactArrangementVertex> vertices,
                                   std::vector<ExactArrangementEdge> edges,
                                   std::vector<ExactArrangementHalfEdge> half_edges,
                                   std::vector<std::uint32_t> outgoing_half_edges,
                                   std::vector<ExactCurveMembership> memberships) noexcept
    : budget_(&budget), charged_bytes_(charged_bytes), vertices_(std::move(vertices)),
      edges_(std::move(edges)), half_edges_(std::move(half_edges)),
      outgoing_half_edges_(std::move(outgoing_half_edges)), memberships_(std::move(memberships))
{
}

ExactArrangement::~ExactArrangement()
{
    release();
}

ExactArrangement::ExactArrangement(ExactArrangement&& other) noexcept
    : budget_(std::exchange(other.budget_, nullptr)),
      charged_bytes_(std::exchange(other.charged_bytes_, 0)), vertices_(std::move(other.vertices_)),
      edges_(std::move(other.edges_)), half_edges_(std::move(other.half_edges_)),
      outgoing_half_edges_(std::move(other.outgoing_half_edges_)),
      memberships_(std::move(other.memberships_))
{
}

ExactArrangement& ExactArrangement::operator=(ExactArrangement&& other) noexcept
{
    if (this != &other)
    {
        release();
        budget_ = std::exchange(other.budget_, nullptr);
        charged_bytes_ = std::exchange(other.charged_bytes_, 0);
        vertices_ = std::move(other.vertices_);
        edges_ = std::move(other.edges_);
        half_edges_ = std::move(other.half_edges_);
        outgoing_half_edges_ = std::move(other.outgoing_half_edges_);
        memberships_ = std::move(other.memberships_);
    }
    return *this;
}

void ExactArrangement::release()
{
    if (budget_ != nullptr)
        budget_->release_storage(charged_bytes_);
    budget_ = nullptr;
    charged_bytes_ = 0;
}

const std::vector<ExactArrangementVertex>& ExactArrangement::vertices() const
{
    return vertices_;
}
const std::vector<ExactArrangementEdge>& ExactArrangement::edges() const
{
    return edges_;
}
const std::vector<ExactArrangementHalfEdge>& ExactArrangement::half_edges() const
{
    return half_edges_;
}
const std::vector<std::uint32_t>& ExactArrangement::outgoing_half_edges() const
{
    return outgoing_half_edges_;
}
const std::vector<ExactCurveMembership>& ExactArrangement::memberships() const
{
    return memberships_;
}

ExactArrangementResult build_exact_arrangement(ConstructionArena& arena,
                                               const std::vector<ExactAtomicCurve>& curves)
{
    try
    {
        if (curves.size() > kMaximumAtomicCurves)
            return failure(Error::resource_limit_exceeded);
        const std::uint64_t curve_count = curves.size();
        const std::uint64_t curve_pairs =
            curve_count < 2 ? 0 : checked_multiply(curve_count, curve_count - 1) / 2;
        if (curve_pairs > kMaximumCurvePairs)
            return failure(Error::resource_limit_exceeded);
        std::uint64_t membership_count = 0;
        for (const ExactAtomicCurve& curve : curves)
            membership_count = checked_add(membership_count, curve.memberships.size());
        if (membership_count > kMaximumMemberships)
            return failure(Error::resource_limit_exceeded);
        const std::uint64_t charge =
            checked_add(4096, checked_add(checked_multiply(curve_count, 1024),
                                          checked_multiply(membership_count, 32)));
        if (!arena.budget().consume_work(
                checked_add(checked_multiply(curve_pairs, 16),
                            checked_add(256, checked_add(checked_multiply(curve_count, 128),
                                                         checked_multiply(membership_count, 16))))))
            return failure(Error::resource_limit_exceeded);
        StorageReservation reservation(arena.budget(), charge);
        if (!reservation.acquired())
            return failure(Error::resource_limit_exceeded);
        ConstructionArenaTransaction transaction(arena);

        std::vector<EndpointDraft> endpoints;
        endpoints.reserve(static_cast<std::size_t>(checked_multiply(curve_count, 2)));
        for (std::size_t index = 0; index < curves.size(); ++index)
        {
            const ExactAtomicCurve& curve = curves[index];
            if (curve.memberships.empty())
                return failure(Error::invalid_argument);
            const Error validation = arrangement_detail::validate_curve(arena, curve);
            if (validation != Error::none)
                return failure(validation);
            endpoints.push_back({curve.start, static_cast<std::uint32_t>(index), true});
            endpoints.push_back({curve.end, static_cast<std::uint32_t>(index), false});
        }
        if (const Error error = arrangement_detail::validate_atomic_pairs(arena, curves);
            error != Error::none)
            return failure(error);
        if (const Error error = sort_endpoints(arena, endpoints); error != Error::none)
            return failure(error);

        std::vector<ExactArrangementVertex> vertices;
        vertices.reserve(endpoints.size());
        std::vector<std::uint32_t> start_vertices(curves.size());
        std::vector<std::uint32_t> end_vertices(curves.size());
        for (const EndpointDraft& endpoint : endpoints)
        {
            bool new_vertex = vertices.empty();
            if (!new_vertex)
            {
                const arrangement_detail::Ordering order = arrangement_detail::compare_points(
                    arena, vertices.back().point, endpoint.point);
                if (order.error != Error::none || !order.value)
                    return failure(order.error == Error::none ? Error::invalid_argument
                                                              : order.error);
                new_vertex = *order.value != 0;
            }
            if (new_vertex)
            {
                if (vertices.size() >= std::numeric_limits<std::uint32_t>::max())
                    return failure(Error::resource_limit_exceeded);
                vertices.push_back({endpoint.point, 0, 0});
            }
            (endpoint.start ? start_vertices : end_vertices)[endpoint.curve] =
                static_cast<std::uint32_t>(vertices.size() - 1);
        }

        std::vector<arrangement_detail::EdgeDraft> drafts;
        drafts.reserve(curves.size());
        for (std::size_t index = 0; index < curves.size(); ++index)
        {
            const ExactAtomicCurve& curve = curves[index];
            const bool reversed = start_vertices[index] > end_vertices[index];
            arrangement_detail::EdgeDraft draft{
                reversed ? end_vertices[index] : start_vertices[index],
                reversed ? start_vertices[index] : end_vertices[index],
                curve.kind,
                curve.kind == ExactAtomicCurveKind::line ? ExactCircle{} : curve.circle,
                curve.kind == ExactAtomicCurveKind::line ? true
                : reversed                               ? !curve.counterclockwise
                                                         : curve.counterclockwise,
                curve.kind == ExactAtomicCurveKind::line ? false : curve.major_arc,
                curve.memberships,
            };
            if (const Error error = normalize_memberships(draft.memberships); error != Error::none)
                return failure(error);
            drafts.push_back(std::move(draft));
        }
        if (const Error error = sort_edges(arena, drafts); error != Error::none)
            return failure(error);

        std::vector<arrangement_detail::EdgeDraft> unique;
        unique.reserve(drafts.size());
        for (arrangement_detail::EdgeDraft& draft : drafts)
        {
            bool merge = false;
            if (!unique.empty())
            {
                const arrangement_detail::Ordering order =
                    arrangement_detail::compare_edges(arena, unique.back(), draft);
                if (order.error != Error::none || !order.value)
                    return failure(order.error == Error::none ? Error::invalid_argument
                                                              : order.error);
                merge = *order.value == 0;
            }
            if (!merge)
            {
                unique.push_back(std::move(draft));
                continue;
            }
            unique.back().memberships.insert(unique.back().memberships.end(),
                                             draft.memberships.begin(), draft.memberships.end());
            if (const Error error = normalize_memberships(unique.back().memberships);
                error != Error::none)
                return failure(error);
        }

        std::vector<ExactArrangementEdge> edges;
        std::vector<ExactCurveMembership> memberships;
        edges.reserve(unique.size());
        memberships.reserve(static_cast<std::size_t>(membership_count));
        for (arrangement_detail::EdgeDraft& draft : unique)
        {
            if (memberships.size() > std::numeric_limits<std::uint32_t>::max() ||
                draft.memberships.size() > std::numeric_limits<std::uint32_t>::max())
                return failure(Error::resource_limit_exceeded);
            const std::uint32_t begin = static_cast<std::uint32_t>(memberships.size());
            memberships.insert(memberships.end(), draft.memberships.begin(),
                               draft.memberships.end());
            edges.push_back({draft.start_vertex, draft.end_vertex, draft.kind, draft.circle,
                             draft.counterclockwise, draft.major_arc, begin,
                             static_cast<std::uint32_t>(draft.memberships.size())});
        }

        std::vector<ExactArrangementHalfEdge> half_edges;
        half_edges.reserve(edges.size() * 2);
        for (std::size_t edge_index = 0; edge_index < edges.size(); ++edge_index)
        {
            const ExactArrangementEdge& edge = edges[edge_index];
            const std::uint32_t forward = static_cast<std::uint32_t>(half_edges.size());
            const std::uint32_t reverse = forward + 1;
            half_edges.push_back(
                {edge.start_vertex, reverse, 0, 0, static_cast<std::uint32_t>(edge_index), true});
            half_edges.push_back(
                {edge.end_vertex, forward, 0, 0, static_cast<std::uint32_t>(edge_index), false});
            ++vertices[edge.start_vertex].outgoing_count;
            ++vertices[edge.end_vertex].outgoing_count;
        }
        std::uint32_t outgoing_total = 0;
        for (ExactArrangementVertex& vertex : vertices)
        {
            if (vertex.outgoing_count < 2 || vertex.outgoing_count % 2 != 0)
                return failure(Error::invalid_argument);
            vertex.outgoing_begin = outgoing_total;
            if (vertex.outgoing_count > std::numeric_limits<std::uint32_t>::max() - outgoing_total)
                return failure(Error::resource_limit_exceeded);
            outgoing_total += vertex.outgoing_count;
        }
        std::vector<std::uint32_t> outgoing(outgoing_total);
        std::vector<std::uint32_t> cursors(vertices.size());
        for (std::uint32_t half_edge = 0; half_edge < half_edges.size(); ++half_edge)
        {
            const std::uint32_t vertex = half_edges[half_edge].origin_vertex;
            outgoing[vertices[vertex].outgoing_begin + cursors[vertex]++] = half_edge;
        }
        if (const Error error = sort_outgoing(arena, vertices, edges, half_edges, outgoing);
            error != Error::none)
            return failure(error);

        const std::uint32_t unset = std::numeric_limits<std::uint32_t>::max();
        for (ExactArrangementHalfEdge& half_edge : half_edges)
            half_edge.next = half_edge.previous = unset;
        for (const ExactArrangementVertex& vertex : vertices)
        {
            for (std::uint32_t ordinal = 0; ordinal < vertex.outgoing_count; ++ordinal)
            {
                const std::uint32_t twin = outgoing[vertex.outgoing_begin + ordinal];
                const std::uint32_t incoming = half_edges[twin].twin;
                const std::uint32_t predecessor =
                    outgoing[vertex.outgoing_begin +
                             (ordinal + vertex.outgoing_count - 1) % vertex.outgoing_count];
                half_edges[incoming].next = predecessor;
                half_edges[predecessor].previous = incoming;
            }
        }
        for (std::uint32_t index = 0; index < half_edges.size(); ++index)
        {
            const ExactArrangementHalfEdge& half_edge = half_edges[index];
            if (half_edge.next == unset || half_edge.previous == unset ||
                half_edges[half_edge.twin].twin != index ||
                half_edges[half_edge.next].previous != index ||
                half_edges[half_edge.previous].next != index)
                return failure(Error::invalid_argument);
        }

        const std::uint64_t transferred = reservation.transfer();
        return {Error::none, ExactArrangement(arena.budget(), transferred, std::move(vertices),
                                              std::move(edges), std::move(half_edges),
                                              std::move(outgoing), std::move(memberships))};
    }
    catch (const std::exception&)
    {
        return failure(Error::resource_limit_exceeded);
    }
}

} // namespace geometer::exact

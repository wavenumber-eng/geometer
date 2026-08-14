#include "geometer/exact_boolean_regions.h"

#include <algorithm>
#include <deque>
#include <exception>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace geometer::exact
{
namespace
{

constexpr std::uint64_t kMaximumRegionFaces = 4'194'304;
constexpr std::uint64_t kMaximumRegionHalfEdges = 16'777'216;
constexpr std::uint64_t kMaximumRegionSources = 8'388'608;

std::uint64_t checked_add(std::uint64_t left, std::uint64_t right)
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
        throw std::overflow_error("region size addition overflow");
    return left + right;
}

std::uint64_t checked_multiply(std::uint64_t left, std::uint64_t right)
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left)
        throw std::overflow_error("region size multiplication overflow");
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

class DisjointSet
{
  public:
    explicit DisjointSet(std::size_t count) : parents_(count)
    {
        std::iota(parents_.begin(), parents_.end(), 0U);
    }

    std::uint32_t find(std::uint32_t value)
    {
        while (parents_[value] != value)
        {
            parents_[value] = parents_[parents_[value]];
            value = parents_[value];
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
        parents_[right] = left;
    }

  private:
    std::vector<std::uint32_t> parents_;
};

struct RawRing
{
    std::vector<std::uint32_t> half_edges;
    std::uint32_t material_component = 0;
    std::uint32_t empty_component = 0;
    std::uint32_t parent = kNoExactResultRing;
    std::uint32_t depth = 0;
    bool counterclockwise = true;
};

struct RegionDraft
{
    std::uint32_t outer_ring = 0;
    std::vector<std::uint64_t> positive_sources;
};

ExactBooleanRegionsResult failure(Error error)
{
    return {error, std::nullopt};
}

void insert_unique(std::vector<std::uint64_t>& values, std::uint64_t value)
{
    const auto position = std::lower_bound(values.begin(), values.end(), value);
    if (position == values.end() || *position != value)
        values.insert(position, value);
}

bool selected_face_ranges_valid(const ExactBooleanSelection& selection)
{
    for (const ExactSelectedFace& face : selection.faces())
    {
        const std::uint64_t positive_end =
            static_cast<std::uint64_t>(face.positive_source_begin) + face.positive_source_count;
        const std::uint64_t subtraction_end =
            static_cast<std::uint64_t>(face.subtraction_source_begin) +
            face.subtraction_source_count;
        if (positive_end > selection.positive_sources().size() ||
            subtraction_end > selection.subtraction_sources().size() ||
            (!face.material && face.positive_source_count != 0))
            return false;
    }
    return true;
}

bool half_edge_key_less(const std::vector<ExactArrangementHalfEdge>& half_edges, std::uint32_t left,
                        std::uint32_t right)
{
    const ExactArrangementHalfEdge& left_value = half_edges[left];
    const ExactArrangementHalfEdge& right_value = half_edges[right];
    if (left_value.origin_vertex != right_value.origin_vertex)
        return left_value.origin_vertex < right_value.origin_vertex;
    if (left_value.edge != right_value.edge)
        return left_value.edge < right_value.edge;
    if (left_value.forward != right_value.forward)
        return left_value.forward < right_value.forward;
    return left < right;
}

bool ring_key_less(const std::vector<ExactArrangementHalfEdge>& half_edges, const RawRing& left,
                   const RawRing& right)
{
    return std::lexicographical_compare(
        left.half_edges.begin(), left.half_edges.end(), right.half_edges.begin(),
        right.half_edges.end(),
        [&half_edges](std::uint32_t left_half_edge, std::uint32_t right_half_edge)
        { return half_edge_key_less(half_edges, left_half_edge, right_half_edge); });
}

} // namespace

ExactBooleanRegions::ExactBooleanRegions(Budget& budget, std::uint64_t charged_bytes,
                                         std::vector<ExactResultRing> rings,
                                         std::vector<std::uint32_t> ring_half_edges,
                                         std::vector<ExactResultRegion> regions,
                                         std::vector<std::uint64_t> positive_sources,
                                         std::vector<ExactResultAssociation> associations) noexcept
    : budget_(&budget), charged_bytes_(charged_bytes), rings_(std::move(rings)),
      ring_half_edges_(std::move(ring_half_edges)), regions_(std::move(regions)),
      positive_sources_(std::move(positive_sources)), associations_(std::move(associations))
{
}

ExactBooleanRegions::~ExactBooleanRegions()
{
    release();
}

ExactBooleanRegions::ExactBooleanRegions(ExactBooleanRegions&& other) noexcept
    : budget_(std::exchange(other.budget_, nullptr)),
      charged_bytes_(std::exchange(other.charged_bytes_, 0)), rings_(std::move(other.rings_)),
      ring_half_edges_(std::move(other.ring_half_edges_)), regions_(std::move(other.regions_)),
      positive_sources_(std::move(other.positive_sources_)),
      associations_(std::move(other.associations_))
{
}

ExactBooleanRegions& ExactBooleanRegions::operator=(ExactBooleanRegions&& other) noexcept
{
    if (this != &other)
    {
        release();
        budget_ = std::exchange(other.budget_, nullptr);
        charged_bytes_ = std::exchange(other.charged_bytes_, 0);
        rings_ = std::move(other.rings_);
        ring_half_edges_ = std::move(other.ring_half_edges_);
        regions_ = std::move(other.regions_);
        positive_sources_ = std::move(other.positive_sources_);
        associations_ = std::move(other.associations_);
    }
    return *this;
}

void ExactBooleanRegions::release()
{
    if (budget_ != nullptr)
        budget_->release_storage(charged_bytes_);
    budget_ = nullptr;
    charged_bytes_ = 0;
}

const std::vector<ExactResultRing>& ExactBooleanRegions::rings() const
{
    return rings_;
}

const std::vector<std::uint32_t>& ExactBooleanRegions::ring_half_edges() const
{
    return ring_half_edges_;
}

const std::vector<ExactResultRegion>& ExactBooleanRegions::regions() const
{
    return regions_;
}

const std::vector<std::uint64_t>& ExactBooleanRegions::positive_sources() const
{
    return positive_sources_;
}

const std::vector<ExactResultAssociation>& ExactBooleanRegions::associations() const
{
    return associations_;
}

ExactBooleanRegionsResult build_exact_boolean_regions(Budget& budget,
                                                      const ExactArrangement& arrangement,
                                                      const ExactBooleanSelection& selection)
{
    try
    {
        const std::uint64_t face_count = arrangement.faces().size();
        const std::uint64_t edge_count = arrangement.edges().size();
        const std::uint64_t half_edge_count = arrangement.half_edges().size();
        const std::uint64_t source_count = selection.positive_sources().size();
        if (face_count == 0 || face_count > kMaximumRegionFaces ||
            half_edge_count > kMaximumRegionHalfEdges || source_count > kMaximumRegionSources ||
            face_count > std::numeric_limits<std::uint32_t>::max() ||
            half_edge_count > std::numeric_limits<std::uint32_t>::max() ||
            edge_count > std::numeric_limits<std::uint32_t>::max())
            return failure(Error::resource_limit_exceeded);
        if (selection.faces().size() != face_count || selection.faces().front().material ||
            !selected_face_ranges_valid(selection) || half_edge_count != edge_count * 2)
            return failure(Error::invalid_argument);

        const std::uint64_t charge = checked_add(
            4096, checked_add(checked_multiply(face_count, 64),
                              checked_add(checked_multiply(edge_count, 16),
                                          checked_add(checked_multiply(half_edge_count, 48),
                                                      checked_multiply(source_count, 32)))));
        const std::uint64_t work = checked_add(
            256, checked_add(checked_multiply(face_count, 16),
                             checked_add(checked_multiply(edge_count, 16),
                                         checked_add(checked_multiply(half_edge_count, 16),
                                                     checked_multiply(source_count, 4)))));
        if (!budget.consume_work(work))
            return failure(Error::resource_limit_exceeded);
        StorageReservation reservation(budget, charge);
        if (!reservation.acquired())
            return failure(Error::resource_limit_exceeded);

        const auto& half_edges = arrangement.half_edges();
        std::vector<std::uint32_t> forward(edge_count, kNoExactResultRing);
        std::vector<std::uint32_t> reverse(edge_count, kNoExactResultRing);
        for (std::uint32_t half_edge_id = 0; half_edge_id < half_edge_count; ++half_edge_id)
        {
            const ExactArrangementHalfEdge& half_edge = half_edges[half_edge_id];
            if (half_edge.edge >= edge_count || half_edge.face >= face_count ||
                half_edge.twin >= half_edge_count ||
                half_edges[half_edge.twin].twin != half_edge_id)
                return failure(Error::invalid_argument);
            std::uint32_t& slot =
                half_edge.forward ? forward[half_edge.edge] : reverse[half_edge.edge];
            if (slot != kNoExactResultRing)
                return failure(Error::invalid_argument);
            slot = half_edge_id;
        }

        DisjointSet components(static_cast<std::size_t>(face_count));
        for (std::uint32_t edge = 0; edge < edge_count; ++edge)
        {
            if (forward[edge] == kNoExactResultRing || reverse[edge] == kNoExactResultRing)
                return failure(Error::invalid_argument);
            const std::uint32_t left = half_edges[forward[edge]].face;
            const std::uint32_t right = half_edges[reverse[edge]].face;
            if (left == right)
                return failure(Error::invalid_argument);
            if (selection.faces()[left].material == selection.faces()[right].material)
                components.unite(left, right);
        }

        std::vector<std::uint32_t> component_by_root(face_count, kNoExactResultRing);
        std::vector<std::uint32_t> component_by_face(face_count, kNoExactResultRing);
        std::vector<bool> component_material;
        for (std::uint32_t face = 0; face < face_count; ++face)
        {
            const std::uint32_t root = components.find(face);
            if (component_by_root[root] == kNoExactResultRing)
            {
                component_by_root[root] = static_cast<std::uint32_t>(component_material.size());
                component_material.push_back(selection.faces()[face].material);
            }
            component_by_face[face] = component_by_root[root];
        }

        std::vector<bool> boundary(half_edge_count);
        for (std::uint32_t half_edge_id = 0; half_edge_id < half_edge_count; ++half_edge_id)
        {
            const ExactArrangementHalfEdge& half_edge = half_edges[half_edge_id];
            boundary[half_edge_id] = selection.faces()[half_edge.face].material &&
                                     !selection.faces()[half_edges[half_edge.twin].face].material;
        }

        std::vector<bool> visited(half_edge_count);
        std::vector<RawRing> raw_rings;
        for (std::uint32_t start = 0; start < half_edge_count; ++start)
        {
            if (!boundary[start] || visited[start])
                continue;
            RawRing ring;
            std::uint32_t current = start;
            for (std::uint64_t ring_steps = 0; ring_steps <= half_edge_count; ++ring_steps)
            {
                if (!boundary[current] || (visited[current] && current != start))
                    return failure(Error::invalid_argument);
                if (current == start && !ring.half_edges.empty())
                    break;
                visited[current] = true;
                ring.half_edges.push_back(current);

                std::uint32_t candidate = half_edges[current].next;
                bool found_boundary = false;
                for (std::uint64_t seam_steps = 0; seam_steps <= half_edge_count; ++seam_steps)
                {
                    if (candidate >= half_edge_count)
                        return failure(Error::invalid_argument);
                    const std::uint32_t left = half_edges[candidate].face;
                    const std::uint32_t right = half_edges[half_edges[candidate].twin].face;
                    if (!selection.faces()[left].material)
                        return failure(Error::invalid_argument);
                    if (!selection.faces()[right].material)
                    {
                        found_boundary = true;
                        break;
                    }
                    candidate = half_edges[half_edges[candidate].twin].next;
                }
                if (!found_boundary)
                    return failure(Error::invalid_argument);
                current = candidate;
            }
            if (ring.half_edges.empty() || current != start)
                return failure(Error::invalid_argument);
            const auto canonical =
                std::min_element(ring.half_edges.begin(), ring.half_edges.end(),
                                 [&half_edges](std::uint32_t left, std::uint32_t right)
                                 { return half_edge_key_less(half_edges, left, right); });
            std::rotate(ring.half_edges.begin(), canonical, ring.half_edges.end());
            const ExactArrangementHalfEdge& first = half_edges[ring.half_edges.front()];
            ring.material_component = component_by_face[first.face];
            ring.empty_component = component_by_face[half_edges[first.twin].face];
            for (const std::uint32_t half_edge_id : ring.half_edges)
            {
                const ExactArrangementHalfEdge& half_edge = half_edges[half_edge_id];
                if (component_by_face[half_edge.face] != ring.material_component ||
                    component_by_face[half_edges[half_edge.twin].face] != ring.empty_component)
                    return failure(Error::invalid_argument);
            }
            raw_rings.push_back(std::move(ring));
        }
        for (std::uint32_t half_edge_id = 0; half_edge_id < half_edge_count; ++half_edge_id)
            if (boundary[half_edge_id] && !visited[half_edge_id])
                return failure(Error::invalid_argument);

        struct Adjacency
        {
            std::uint32_t neighbor = 0;
            std::uint32_t ring = 0;
        };
        std::vector<std::vector<Adjacency>> adjacency(component_material.size());
        for (std::uint32_t ring = 0; ring < raw_rings.size(); ++ring)
        {
            const RawRing& value = raw_rings[ring];
            if (value.material_component == value.empty_component ||
                !component_material[value.material_component] ||
                component_material[value.empty_component])
                return failure(Error::invalid_argument);
            adjacency[value.material_component].push_back({value.empty_component, ring});
            adjacency[value.empty_component].push_back({value.material_component, ring});
        }

        const std::uint32_t root_component = component_by_face.front();
        std::vector<std::uint32_t> component_depth(component_material.size(), kNoExactResultRing);
        std::vector<std::uint32_t> incoming_ring(component_material.size(), kNoExactResultRing);
        std::vector<bool> ring_seen(raw_rings.size());
        std::deque<std::uint32_t> queue;
        component_depth[root_component] = 0;
        queue.push_back(root_component);
        while (!queue.empty())
        {
            const std::uint32_t component = queue.front();
            queue.pop_front();
            for (const Adjacency edge : adjacency[component])
            {
                if (ring_seen[edge.ring])
                    continue;
                ring_seen[edge.ring] = true;
                if (component_depth[edge.neighbor] != kNoExactResultRing)
                    return failure(Error::invalid_argument);
                if (component_depth[component] == std::numeric_limits<std::uint32_t>::max() - 1)
                    return failure(Error::resource_limit_exceeded);
                component_depth[edge.neighbor] = component_depth[component] + 1;
                incoming_ring[edge.neighbor] = edge.ring;
                RawRing& ring = raw_rings[edge.ring];
                ring.depth = component_depth[component];
                ring.parent = incoming_ring[component];
                ring.counterclockwise = ring.depth % 2 == 0;
                const bool parent_is_material = component_material[component];
                if (parent_is_material == ring.counterclockwise ||
                    component_material[edge.neighbor] == parent_is_material)
                    return failure(Error::invalid_argument);
                queue.push_back(edge.neighbor);
            }
        }
        if (std::find(component_depth.begin(), component_depth.end(), kNoExactResultRing) !=
                component_depth.end() ||
            std::find(ring_seen.begin(), ring_seen.end(), false) != ring_seen.end())
            return failure(Error::invalid_argument);

        std::vector<RegionDraft> region_drafts;
        for (std::uint32_t component = 0; component < component_material.size(); ++component)
        {
            if (!component_material[component])
                continue;
            if (component_depth[component] % 2 == 0 ||
                incoming_ring[component] == kNoExactResultRing)
                return failure(Error::invalid_argument);
            RegionDraft draft;
            draft.outer_ring = incoming_ring[component];
            for (std::uint32_t face = 0; face < face_count; ++face)
            {
                if (component_by_face[face] != component)
                    continue;
                const ExactSelectedFace& selected = selection.faces()[face];
                for (std::uint32_t index = 0; index < selected.positive_source_count; ++index)
                    insert_unique(
                        draft.positive_sources,
                        selection.positive_sources()[selected.positive_source_begin + index]);
            }
            if (draft.positive_sources.empty())
                return failure(Error::invalid_argument);
            region_drafts.push_back(std::move(draft));
        }

        std::vector<std::uint32_t> ring_order(raw_rings.size());
        std::iota(ring_order.begin(), ring_order.end(), 0U);
        std::sort(ring_order.begin(), ring_order.end(),
                  [&raw_rings, &half_edges](std::uint32_t left, std::uint32_t right)
                  { return ring_key_less(half_edges, raw_rings[left], raw_rings[right]); });
        std::vector<std::uint32_t> ring_remap(raw_rings.size());
        for (std::uint32_t index = 0; index < ring_order.size(); ++index)
            ring_remap[ring_order[index]] = index;
        std::sort(region_drafts.begin(), region_drafts.end(),
                  [&raw_rings, &half_edges](const RegionDraft& left, const RegionDraft& right)
                  {
                      return ring_key_less(half_edges, raw_rings[left.outer_ring],
                                           raw_rings[right.outer_ring]);
                  });

        std::vector<ExactResultRing> rings;
        std::vector<std::uint32_t> ring_half_edges;
        rings.reserve(raw_rings.size());
        ring_half_edges.reserve(static_cast<std::size_t>(half_edge_count));
        for (const std::uint32_t old_ring : ring_order)
        {
            const RawRing& ring = raw_rings[old_ring];
            const std::uint32_t begin = static_cast<std::uint32_t>(ring_half_edges.size());
            ring_half_edges.insert(ring_half_edges.end(), ring.half_edges.begin(),
                                   ring.half_edges.end());
            rings.push_back(
                {begin, static_cast<std::uint32_t>(ring.half_edges.size()),
                 ring.parent == kNoExactResultRing ? kNoExactResultRing : ring_remap[ring.parent],
                 ring.depth, ring.counterclockwise});
        }

        std::vector<ExactResultRegion> regions;
        std::vector<std::uint64_t> positive_sources;
        std::vector<ExactResultAssociation> associations;
        regions.reserve(region_drafts.size());
        positive_sources.reserve(selection.positive_sources().size());
        associations.reserve(selection.positive_sources().size());
        for (std::uint32_t region = 0; region < region_drafts.size(); ++region)
        {
            const RegionDraft& draft = region_drafts[region];
            const std::uint32_t begin = static_cast<std::uint32_t>(positive_sources.size());
            positive_sources.insert(positive_sources.end(), draft.positive_sources.begin(),
                                    draft.positive_sources.end());
            regions.push_back({ring_remap[draft.outer_ring], begin,
                               static_cast<std::uint32_t>(draft.positive_sources.size())});
            for (const std::uint64_t source : draft.positive_sources)
                associations.push_back({source, region});
        }
        std::sort(associations.begin(), associations.end(),
                  [](const ExactResultAssociation& left, const ExactResultAssociation& right)
                  {
                      return left.source_id != right.source_id
                                 ? left.source_id < right.source_id
                                 : left.result_region < right.result_region;
                  });

        const std::uint64_t transferred = reservation.transfer();
        return {Error::none,
                ExactBooleanRegions(budget, transferred, std::move(rings),
                                    std::move(ring_half_edges), std::move(regions),
                                    std::move(positive_sources), std::move(associations))};
    }
    catch (const std::exception&)
    {
        return failure(Error::resource_limit_exceeded);
    }
}

} // namespace geometer::exact

#include "geometer/analytic_filtered_regions.h"

#include "analytic_filtered_boolean_selection_support.h"
#include "analytic_filtered_outcome_tracker.h"
#include "analytic_filtered_regions_internal.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <numeric>
#include <tuple>
#include <utility>
#include <vector>

namespace geometer
{
namespace
{
using analytic_selection_detail::checked_add;
using analytic_selection_detail::checked_multiply;
using analytic_selection_detail::sort_units;

constexpr std::uint64_t kLogicalIndexBytes = 8;
constexpr std::uint64_t kLogicalByteBytes = 1;
constexpr std::uint64_t kLogicalRingBytes = 32;
constexpr std::uint64_t kLogicalRegionBytes = 16;
constexpr std::uint64_t kLogicalRawRingBytes = 40;
constexpr std::uint64_t kLogicalAdjacencyBytes = 16;
constexpr std::uint64_t kLogicalDsuBytes = 8;

struct RawRing
{
    std::uint32_t half_edge_begin = 0;
    std::uint32_t half_edge_count = 0;
    std::uint32_t material_component = 0;
    std::uint32_t empty_component = 0;
    std::uint32_t parent = kNoAnalyticFilteredRing;
    std::uint32_t depth = 0;
    bool counterclockwise = true;
};

struct ComponentAdjacency
{
    std::uint32_t component = 0;
    std::uint32_t neighbor = 0;
    std::uint32_t ring = 0;
};

class RegionsBuilder;

class MeteredDisjointSet
{
  public:
    explicit MeteredDisjointSet(std::size_t count) : parent_(count), rank_(count)
    {
        std::iota(parent_.begin(), parent_.end(), 0U);
    }

    template <typename Charge>
    bool find(std::uint32_t value, std::uint32_t& root, Charge&& charge,
              std::uint64_t& visits) noexcept
    {
        if (!charge(1))
            return false;
        ++visits;
        while (parent_[value] != value)
        {
            if (!charge(1))
                return false;
            ++visits;
            parent_[value] = parent_[parent_[value]];
            value = parent_[value];
        }
        root = value;
        return true;
    }

    template <typename Charge>
    bool unite(std::uint32_t left, std::uint32_t right, Charge&& charge,
               std::uint64_t& visits) noexcept
    {
        std::uint32_t left_root = 0;
        std::uint32_t right_root = 0;
        if (!find(left, left_root, charge, visits) || !find(right, right_root, charge, visits))
            return false;
        if (left_root == right_root)
            return true;
        if (rank_[left_root] < rank_[right_root] ||
            (rank_[left_root] == rank_[right_root] && right_root < left_root))
            std::swap(left_root, right_root);
        parent_[right_root] = left_root;
        if (rank_[left_root] == rank_[right_root])
            ++rank_[left_root];
        return true;
    }

  private:
    std::vector<std::uint32_t> parent_;
    std::vector<std::uint8_t> rank_;
};

std::uint64_t retained_selection_bytes(const AnalyticFilteredBooleanSelectionResult& selection,
                                       bool& valid) noexcept
{
    const auto& arrangement = selection.arrangement;
    std::uint64_t bytes = checked_multiply(arrangement.vertices.size(),
                                           kAnalyticArrangementVertexLogicalBytes, valid);
    bytes = checked_add(
        bytes,
        checked_multiply(arrangement.edges.size(), kAnalyticArrangementEdgeLogicalBytes, valid),
        valid);
    bytes = checked_add(bytes,
                        checked_multiply(arrangement.half_edges.size(),
                                         kAnalyticArrangementHalfEdgeLogicalBytes, valid),
                        valid);
    bytes = checked_add(
        bytes, checked_multiply(arrangement.outgoing_half_edges.size(), kLogicalIndexBytes, valid),
        valid);
    bytes = checked_add(bytes,
                        checked_multiply(arrangement.collapsed_spans.size(),
                                         kAnalyticArrangementCollapsedSpanLogicalBytes, valid),
                        valid);
    bytes = checked_add(bytes,
                        checked_multiply(arrangement.memberships.size(),
                                         kAnalyticOverlayMembershipLogicalBytes, valid),
                        valid);
    bytes = checked_add(
        bytes,
        checked_multiply(arrangement.cycles.size(), kAnalyticArrangementCycleLogicalBytes, valid),
        valid);
    bytes = checked_add(
        bytes, checked_multiply(arrangement.cycle_half_edges.size(), kLogicalIndexBytes, valid),
        valid);
    bytes = checked_add(bytes,
                        checked_multiply(selection.occurrences.size(),
                                         analytic_selection_detail::kOccurrenceLogicalBytes, valid),
                        valid);
    bytes = checked_add(
        bytes, checked_multiply(selection.half_edge_faces.size(), kLogicalIndexBytes, valid),
        valid);
    bytes = checked_add(bytes,
                        checked_multiply(selection.faces.size(),
                                         analytic_selection_detail::kFaceLogicalBytes, valid),
                        valid);
    bytes = checked_add(
        bytes, checked_multiply(selection.face_boundary_cycles.size(), kLogicalIndexBytes, valid),
        valid);
    bytes =
        checked_add(bytes,
                    checked_multiply(selection.coverage_state_nodes.size(),
                                     analytic_selection_detail::kCoverageNodeLogicalBytes, valid),
                    valid);
    return checked_add(bytes,
                       checked_multiply(selection.outcome_evidence.size(),
                                        analytic_selection_detail::kOutcomeEvidenceLogicalBytes,
                                        valid),
                       valid);
}

bool half_edge_key_less(const std::vector<AnalyticArrangementHalfEdge>& half_edges,
                        std::uint32_t left, std::uint32_t right) noexcept
{
    const AnalyticArrangementHalfEdge& left_value = half_edges[left];
    const AnalyticArrangementHalfEdge& right_value = half_edges[right];
    return std::tie(left_value.origin_vertex, left_value.edge, left_value.forward, left) <
           std::tie(right_value.origin_vertex, right_value.edge, right_value.forward, right);
}

class RegionsBuilder
{
  public:
    RegionsBuilder(AnalyticFilteredBooleanSelectionResult selection, AnalyticSolverLimits limits,
                   std::uint64_t reserved_work)
        : limits_(limits), reserved_work_remaining_(reserved_work)
    {
        result_.telemetry.selection_predicate_calls = selection.telemetry.predicate_calls;
        result_.telemetry.selection_peak_working_memory_bytes =
            selection.telemetry.peak_working_memory_bytes;
        result_.telemetry.reserved_region_work_units = reserved_work;
        result_.telemetry.predicate_calls = selection.telemetry.predicate_calls + reserved_work;
        result_.telemetry.peak_working_memory_bytes = selection.telemetry.peak_working_memory_bytes;
        result_.telemetry.algebraic_fallback_calls = selection.telemetry.algebraic_fallback_calls;
        result_.selection = std::move(selection);
    }

    AnalyticFilteredRegionsResult build()
    {
        try
        {
            if (!preflight() || !build_components() || !build_boundary_successors() ||
                !trace_rings() || !build_component_graph() || !publish())
            {
                if (result_.error == AnalyticFilteredRegionsError::none)
                    fail(AnalyticFilteredRegionsError::invalid_argument);
                clear_geometry();
            }
        }
        catch (const std::bad_alloc&)
        {
            fail(AnalyticFilteredRegionsError::resource_limit_exceeded);
            clear_geometry();
        }
        return std::move(result_);
    }

  private:
    bool fail(AnalyticFilteredRegionsError error) noexcept
    {
        result_.error = error;
        return false;
    }

    bool charge(std::uint64_t units) noexcept
    {
        if (result_.telemetry.predicate_calls > limits_.predicate_calls ||
            units > reserved_work_remaining_)
            return fail(AnalyticFilteredRegionsError::resource_limit_exceeded);
        reserved_work_remaining_ -= units;
        result_.telemetry.region_work_units += units;
        return true;
    }

    bool charge_sort(std::uint64_t count) noexcept
    {
        const std::uint64_t units = sort_units(count);
        if (!charge(units))
            return false;
        result_.telemetry.sort_work_units += units;
        return true;
    }

    void clear_geometry()
    {
        const auto selection_telemetry = result_.selection.telemetry;
        const std::int64_t origin_x = result_.selection.origin_x_nm;
        const std::int64_t origin_y = result_.selection.origin_y_nm;
        result_.selection = {};
        result_.selection.origin_x_nm = origin_x;
        result_.selection.origin_y_nm = origin_y;
        result_.selection.telemetry = selection_telemetry;
        result_.rings.clear();
        result_.ring_half_edges.clear();
        result_.regions.clear();
        result_.face_components.clear();
    }

    bool preflight()
    {
        const auto& selection = result_.selection;
        const auto& arrangement = selection.arrangement;
        const std::uint64_t edge_count = arrangement.edges.size();
        const std::uint64_t half_edge_count = arrangement.half_edges.size();
        const std::uint64_t face_count = selection.faces.size();
        if (selection.error != AnalyticFilteredBooleanSelectionError::none || face_count == 0 ||
            edge_count > std::numeric_limits<std::uint32_t>::max() ||
            half_edge_count > std::numeric_limits<std::uint32_t>::max() ||
            face_count > std::numeric_limits<std::uint32_t>::max() ||
            edge_count > limits_.arrangement_half_edges / 2 || half_edge_count != edge_count * 2 ||
            half_edge_count > limits_.arrangement_half_edges ||
            face_count > limits_.arrangement_faces ||
            selection.half_edge_faces.size() != half_edge_count ||
            arrangement.outgoing_half_edges.size() != half_edge_count ||
            !selection.faces.front().unbounded || selection.faces.front().material)
            return fail(AnalyticFilteredRegionsError::invalid_argument);
        edge_count_ = static_cast<std::uint32_t>(edge_count);
        half_edge_count_ = static_cast<std::uint32_t>(half_edge_count);
        face_count_ = static_cast<std::uint32_t>(face_count);

        bool valid = true;
        const std::uint64_t maximum_rings = half_edge_count_;
        const std::uint64_t maximum_adjacency = checked_multiply(maximum_rings, 2, valid);
        std::uint64_t scratch = checked_multiply(edge_count_, kLogicalIndexBytes * 2, valid);
        scratch =
            checked_add(scratch,
                        checked_multiply(half_edge_count_,
                                         kLogicalByteBytes * 4 + kLogicalIndexBytes * 2, valid),
                        valid);
        scratch = checked_add(
            scratch,
            checked_multiply(face_count_,
                             kLogicalDsuBytes + kLogicalIndexBytes * 4 + kLogicalByteBytes, valid),
            valid);
        scratch = checked_add(scratch,
                              checked_multiply(maximum_rings,
                                               kLogicalRawRingBytes + kLogicalRingBytes +
                                                   kLogicalIndexBytes * 3 + kLogicalByteBytes,
                                               valid),
                              valid);
        scratch = checked_add(
            scratch, checked_multiply(maximum_adjacency, kLogicalAdjacencyBytes, valid), valid);
        scratch = checked_add(scratch, checked_multiply(face_count_ + 1, kLogicalIndexBytes, valid),
                              valid);
        scratch = checked_add(
            scratch, checked_multiply(face_count_, kLogicalRegionBytes + kLogicalIndexBytes, valid),
            valid);
        scratch = checked_add(scratch,
                              checked_multiply(half_edge_count_, kLogicalIndexBytes, valid), valid);
        const std::uint64_t retained = retained_selection_bytes(selection, valid);
        const std::uint64_t phase = checked_add(retained, scratch, valid);
        if (!valid || phase > limits_.working_memory_bytes)
            return fail(AnalyticFilteredRegionsError::resource_limit_exceeded);
        result_.telemetry.peak_working_memory_bytes =
            std::max(result_.telemetry.peak_working_memory_bytes, phase);

        if (!charge(half_edge_count_))
            return false;

        forward_by_edge_.assign(edge_count_, kNoAnalyticFilteredRing);
        reverse_by_edge_.assign(edge_count_, kNoAnalyticFilteredRing);
        boundary_.assign(half_edge_count_, 0);
        next_boundary_.assign(half_edge_count_, kNoAnalyticFilteredRing);
        visited_.assign(half_edge_count_, 0);
        outgoing_seen_.assign(half_edge_count_, 0);
        boundary_predecessors_.assign(half_edge_count_, 0);
        result_.face_components.assign(face_count_, kNoAnalyticFilteredRing);
        raw_rings_.reserve(static_cast<std::size_t>(maximum_rings));
        raw_half_edges_.reserve(half_edge_count_);
        return validate_half_edges();
    }

    bool validate_half_edges()
    {
        const auto& arrangement = result_.selection.arrangement;
        for (std::uint32_t half_edge_id = 0; half_edge_id < half_edge_count_; ++half_edge_id)
        {
            const AnalyticArrangementHalfEdge& half_edge = arrangement.half_edges[half_edge_id];
            if (half_edge.edge >= edge_count_ ||
                half_edge.origin_vertex >= arrangement.vertices.size() ||
                half_edge.twin >= half_edge_count_ ||
                arrangement.half_edges[half_edge.twin].twin != half_edge_id ||
                result_.selection.half_edge_faces[half_edge_id] >= face_count_)
                return fail(AnalyticFilteredRegionsError::invalid_argument);
            std::uint32_t& slot = half_edge.forward ? forward_by_edge_[half_edge.edge]
                                                    : reverse_by_edge_[half_edge.edge];
            if (slot != kNoAnalyticFilteredRing)
                return fail(AnalyticFilteredRegionsError::invalid_argument);
            slot = half_edge_id;
            const std::uint32_t left = result_.selection.half_edge_faces[half_edge_id];
            const std::uint32_t right = result_.selection.half_edge_faces[half_edge.twin];
            boundary_[half_edge_id] =
                result_.selection.faces[left].material && !result_.selection.faces[right].material;
            result_.telemetry.boundary_half_edges += boundary_[half_edge_id];
        }
        return true;
    }

    bool build_components()
    {
        if (!charge(static_cast<std::uint64_t>(edge_count_) + face_count_))
            return false;
        components_ = std::make_unique<MeteredDisjointSet>(face_count_);
        const auto& half_edges = result_.selection.arrangement.half_edges;
        for (std::uint32_t edge = 0; edge < edge_count_; ++edge)
        {
            if (forward_by_edge_[edge] == kNoAnalyticFilteredRing ||
                reverse_by_edge_[edge] == kNoAnalyticFilteredRing)
                return fail(AnalyticFilteredRegionsError::invalid_argument);
            const std::uint32_t left = result_.selection.half_edge_faces[forward_by_edge_[edge]];
            const std::uint32_t right = result_.selection.half_edge_faces[reverse_by_edge_[edge]];
            if (left == right || half_edges[forward_by_edge_[edge]].twin != reverse_by_edge_[edge])
                return fail(AnalyticFilteredRegionsError::invalid_argument);
            if (result_.selection.faces[left].material == result_.selection.faces[right].material &&
                !components_->unite(
                    left, right, [&](std::uint64_t units) { return charge(units); },
                    result_.telemetry.disjoint_set_node_visits))
                return false;
        }

        component_by_root_.assign(face_count_, kNoAnalyticFilteredRing);
        component_material_.reserve(face_count_);
        for (std::uint32_t face = 0; face < face_count_; ++face)
        {
            std::uint32_t root = 0;
            if (!components_->find(
                    face, root, [&](std::uint64_t units) { return charge(units); },
                    result_.telemetry.disjoint_set_node_visits))
                return false;
            if (component_by_root_[root] == kNoAnalyticFilteredRing)
            {
                component_by_root_[root] = static_cast<std::uint32_t>(component_material_.size());
                component_material_.push_back(result_.selection.faces[face].material);
            }
            const std::uint32_t component = component_by_root_[root];
            if ((component_material_[component] != 0) != result_.selection.faces[face].material)
                return fail(AnalyticFilteredRegionsError::invalid_argument);
            result_.face_components[face] = component;
        }
        return true;
    }

    bool build_boundary_successors()
    {
        const auto& arrangement = result_.selection.arrangement;
        bool valid = true;
        const std::uint64_t rotation_work = checked_add(
            arrangement.vertices.size(), checked_multiply(half_edge_count_, 4, valid), valid);
        if (!valid || !charge(rotation_work))
            return false;
        for (std::uint32_t vertex = 0; vertex < arrangement.vertices.size(); ++vertex)
        {
            const AnalyticArrangementVertexNm& value = arrangement.vertices[vertex];
            const std::uint64_t end =
                static_cast<std::uint64_t>(value.outgoing_begin) + value.outgoing_count;
            if (end > arrangement.outgoing_half_edges.size())
                return fail(AnalyticFilteredRegionsError::invalid_argument);
            if (value.outgoing_count == 0)
                continue;
            std::uint32_t previous_boundary = kNoAnalyticFilteredRing;
            for (std::uint64_t offset = end; offset > value.outgoing_begin; --offset)
            {
                const std::uint32_t outgoing = arrangement.outgoing_half_edges[offset - 1];
                if (outgoing >= half_edge_count_ ||
                    arrangement.half_edges[outgoing].origin_vertex != vertex ||
                    outgoing_seen_[outgoing] != 0)
                    return fail(AnalyticFilteredRegionsError::invalid_argument);
                if (boundary_[outgoing] != 0)
                {
                    previous_boundary = outgoing;
                    break;
                }
            }
            for (std::uint64_t offset = value.outgoing_begin; offset < end; ++offset)
            {
                const std::uint32_t outgoing = arrangement.outgoing_half_edges[offset];
                const std::uint32_t previous_outgoing =
                    arrangement
                        .outgoing_half_edges[offset == value.outgoing_begin ? end - 1 : offset - 1];
                if (outgoing >= half_edge_count_ ||
                    arrangement.half_edges[outgoing].origin_vertex != vertex ||
                    outgoing_seen_[outgoing] != 0 ||
                    result_.selection.half_edge_faces[arrangement.half_edges[outgoing].twin] !=
                        result_.selection.half_edge_faces[previous_outgoing])
                    return fail(AnalyticFilteredRegionsError::invalid_argument);
                outgoing_seen_[outgoing] = 1;
                ++result_.telemetry.vertex_rotation_visits;
                if (boundary_[outgoing] != 0)
                    previous_boundary = outgoing;
                const std::uint32_t incoming = arrangement.half_edges[outgoing].twin;
                if (boundary_[incoming] != 0)
                {
                    if (previous_boundary == kNoAnalyticFilteredRing)
                        return fail(AnalyticFilteredRegionsError::invalid_argument);
                    next_boundary_[incoming] = previous_boundary;
                    if (boundary_predecessors_[previous_boundary] ==
                        std::numeric_limits<std::uint8_t>::max())
                        return fail(AnalyticFilteredRegionsError::invalid_argument);
                    ++boundary_predecessors_[previous_boundary];
                }
            }
        }
        if (std::find(outgoing_seen_.begin(), outgoing_seen_.end(), 0) != outgoing_seen_.end())
            return fail(AnalyticFilteredRegionsError::invalid_argument);
        for (std::uint32_t half_edge = 0; half_edge < half_edge_count_; ++half_edge)
            if (boundary_[half_edge] != 0 &&
                (next_boundary_[half_edge] == kNoAnalyticFilteredRing ||
                 boundary_predecessors_[half_edge] != 1))
                return fail(AnalyticFilteredRegionsError::invalid_argument);
        return true;
    }

    bool trace_rings()
    {
        const auto& half_edges = result_.selection.arrangement.half_edges;
        if (!charge(static_cast<std::uint64_t>(half_edge_count_) * 5))
            return false;
        for (std::uint32_t start = 0; start < half_edge_count_; ++start)
        {
            if (boundary_[start] == 0 || visited_[start] != 0)
                continue;
            RawRing ring;
            ring.half_edge_begin = static_cast<std::uint32_t>(raw_half_edges_.size());
            std::uint32_t current = start;
            for (std::uint64_t steps = 0; steps <= half_edge_count_; ++steps)
            {
                if (current >= half_edge_count_ || boundary_[current] == 0 ||
                    (visited_[current] != 0 && current != start))
                    return fail(AnalyticFilteredRegionsError::invalid_argument);
                if (current == start && ring.half_edge_count != 0)
                    break;
                visited_[current] = 1;
                raw_half_edges_.push_back(current);
                ++ring.half_edge_count;
                const std::uint32_t next = next_boundary_[current];
                if (next >= half_edge_count_ ||
                    half_edges[next].origin_vertex !=
                        half_edges[half_edges[current].twin].origin_vertex)
                    return fail(AnalyticFilteredRegionsError::invalid_argument);
                current = next;
            }
            if (ring.half_edge_count == 0 || current != start)
                return fail(AnalyticFilteredRegionsError::invalid_argument);
            auto begin = raw_half_edges_.begin() + ring.half_edge_begin;
            auto end = begin + ring.half_edge_count;
            const auto canonical =
                std::min_element(begin, end, [&](std::uint32_t left, std::uint32_t right)
                                 { return half_edge_key_less(half_edges, left, right); });
            std::rotate(begin, canonical, end);
            const std::uint32_t first = raw_half_edges_[ring.half_edge_begin];
            ring.material_component =
                result_.face_components[result_.selection.half_edge_faces[first]];
            ring.empty_component =
                result_.face_components[result_.selection.half_edge_faces[half_edges[first].twin]];
            for (auto iterator = begin; iterator != end; ++iterator)
            {
                const AnalyticArrangementHalfEdge& half_edge = half_edges[*iterator];
                if (result_.face_components[result_.selection.half_edge_faces[*iterator]] !=
                        ring.material_component ||
                    result_.face_components[result_.selection.half_edge_faces[half_edge.twin]] !=
                        ring.empty_component)
                    return fail(AnalyticFilteredRegionsError::invalid_argument);
            }
            raw_rings_.push_back(ring);
        }
        for (std::uint32_t half_edge = 0; half_edge < half_edge_count_; ++half_edge)
            if (boundary_[half_edge] != 0 && visited_[half_edge] == 0)
                return fail(AnalyticFilteredRegionsError::invalid_argument);
        return true;
    }

    bool build_component_graph()
    {
        const std::uint32_t component_count =
            static_cast<std::uint32_t>(component_material_.size());
        if (!charge(raw_rings_.size()))
            return false;
        adjacency_.reserve(raw_rings_.size() * 2);
        for (std::uint32_t ring = 0; ring < raw_rings_.size(); ++ring)
        {
            const RawRing& value = raw_rings_[ring];
            if (value.material_component >= component_count ||
                value.empty_component >= component_count ||
                value.material_component == value.empty_component ||
                !component_material_[value.material_component] ||
                component_material_[value.empty_component])
                return fail(AnalyticFilteredRegionsError::invalid_argument);
            adjacency_.push_back({value.material_component, value.empty_component, ring});
            adjacency_.push_back({value.empty_component, value.material_component, ring});
        }
        if (!charge_sort(adjacency_.size()))
            return false;
        std::sort(adjacency_.begin(), adjacency_.end(),
                  [](const ComponentAdjacency& left, const ComponentAdjacency& right)
                  {
                      return std::tie(left.component, left.neighbor, left.ring) <
                             std::tie(right.component, right.neighbor, right.ring);
                  });
        bool valid = true;
        std::uint64_t graph_work = checked_multiply(raw_rings_.size(), 4, valid);
        graph_work = checked_add(graph_work, checked_multiply(component_count, 2, valid), valid);
        if (!valid || !charge(graph_work))
            return false;
        adjacency_begin_.assign(static_cast<std::size_t>(component_count) + 1, 0);
        for (const ComponentAdjacency& edge : adjacency_)
            ++adjacency_begin_[edge.component + 1];
        for (std::uint32_t component = 0; component < component_count; ++component)
            adjacency_begin_[component + 1] += adjacency_begin_[component];

        component_depth_.assign(component_count, kNoAnalyticFilteredRing);
        incoming_ring_.assign(component_count, kNoAnalyticFilteredRing);
        ring_seen_.assign(raw_rings_.size(), 0);
        queue_.reserve(component_count);
        const std::uint32_t root = result_.face_components.front();
        if (root >= component_count || component_material_[root])
            return fail(AnalyticFilteredRegionsError::invalid_argument);
        component_depth_[root] = 0;
        queue_.push_back(root);
        for (std::size_t cursor = 0; cursor < queue_.size(); ++cursor)
        {
            const std::uint32_t component = queue_[cursor];
            for (std::uint32_t index = adjacency_begin_[component];
                 index < adjacency_begin_[component + 1]; ++index)
            {
                const ComponentAdjacency edge = adjacency_[index];
                if (ring_seen_[edge.ring] != 0)
                    continue;
                ring_seen_[edge.ring] = 1;
                if (component_depth_[edge.neighbor] != kNoAnalyticFilteredRing ||
                    component_depth_[component] == kNoAnalyticFilteredRing ||
                    component_depth_[component] == std::numeric_limits<std::uint32_t>::max() - 1)
                    return fail(AnalyticFilteredRegionsError::invalid_argument);
                component_depth_[edge.neighbor] = component_depth_[component] + 1;
                incoming_ring_[edge.neighbor] = edge.ring;
                RawRing& ring = raw_rings_[edge.ring];
                ring.depth = component_depth_[component];
                ring.parent = incoming_ring_[component];
                ring.counterclockwise = ring.depth % 2 == 0;
                if ((component_material_[component] != 0) == ring.counterclockwise ||
                    component_material_[edge.neighbor] == component_material_[component])
                    return fail(AnalyticFilteredRegionsError::invalid_argument);
                queue_.push_back(edge.neighbor);
            }
        }
        if (std::find(component_depth_.begin(), component_depth_.end(), kNoAnalyticFilteredRing) !=
                component_depth_.end() ||
            std::find(ring_seen_.begin(), ring_seen_.end(), 0) != ring_seen_.end())
            return fail(AnalyticFilteredRegionsError::invalid_argument);
        return true;
    }

    bool publish()
    {
        const auto& half_edges = result_.selection.arrangement.half_edges;
        if (!charge(raw_rings_.size()))
            return false;
        std::vector<std::uint32_t> order(raw_rings_.size());
        std::iota(order.begin(), order.end(), 0U);
        if (!charge_sort(order.size()))
            return false;
        const auto ring_less = [&](std::uint32_t left, std::uint32_t right)
        {
            const RawRing& left_ring = raw_rings_[left];
            const RawRing& right_ring = raw_rings_[right];
            return std::lexicographical_compare(
                raw_half_edges_.begin() + left_ring.half_edge_begin,
                raw_half_edges_.begin() + left_ring.half_edge_begin + left_ring.half_edge_count,
                raw_half_edges_.begin() + right_ring.half_edge_begin,
                raw_half_edges_.begin() + right_ring.half_edge_begin + right_ring.half_edge_count,
                [&](std::uint32_t left_half_edge, std::uint32_t right_half_edge)
                { return half_edge_key_less(half_edges, left_half_edge, right_half_edge); });
        };
        std::sort(order.begin(), order.end(), ring_less);
        std::vector<std::uint32_t> remap(raw_rings_.size());
        bool valid = true;
        std::uint64_t publish_work = checked_multiply(raw_rings_.size(), 3, valid);
        publish_work = checked_add(publish_work, raw_half_edges_.size(), valid);
        publish_work = checked_add(publish_work,
                                   checked_multiply(component_material_.size(), 2, valid), valid);
        if (!valid || !charge(publish_work))
            return false;
        for (std::uint32_t index = 0; index < order.size(); ++index)
            remap[order[index]] = index;

        result_.rings.reserve(raw_rings_.size());
        result_.ring_half_edges.reserve(raw_half_edges_.size());
        for (const std::uint32_t old_ring : order)
        {
            const RawRing& ring = raw_rings_[old_ring];
            const std::uint32_t begin = static_cast<std::uint32_t>(result_.ring_half_edges.size());
            result_.ring_half_edges.insert(
                result_.ring_half_edges.end(), raw_half_edges_.begin() + ring.half_edge_begin,
                raw_half_edges_.begin() + ring.half_edge_begin + ring.half_edge_count);
            result_.rings.push_back({begin, ring.half_edge_count,
                                     ring.parent == kNoAnalyticFilteredRing
                                         ? kNoAnalyticFilteredRing
                                         : remap[ring.parent],
                                     ring.depth, ring.counterclockwise});
        }

        result_.regions.reserve(component_material_.size());
        for (std::uint32_t component = 0; component < component_material_.size(); ++component)
        {
            if (!component_material_[component])
                continue;
            if (component_depth_[component] % 2 == 0 ||
                incoming_ring_[component] == kNoAnalyticFilteredRing)
                return fail(AnalyticFilteredRegionsError::invalid_argument);
            result_.regions.push_back({remap[incoming_ring_[component]], component});
        }
        if (!charge_sort(result_.regions.size()))
            return false;
        std::sort(result_.regions.begin(), result_.regions.end(),
                  [](const AnalyticFilteredMaterialRegion& left,
                     const AnalyticFilteredMaterialRegion& right)
                  { return left.outer_ring < right.outer_ring; });
        result_.telemetry.emitted_rings = result_.rings.size();
        result_.telemetry.emitted_regions = result_.regions.size();
        return true;
    }

    AnalyticSolverLimits limits_;
    std::uint64_t reserved_work_remaining_ = 0;
    AnalyticFilteredRegionsResult result_;
    std::uint32_t edge_count_ = 0;
    std::uint32_t half_edge_count_ = 0;
    std::uint32_t face_count_ = 0;
    std::vector<std::uint32_t> forward_by_edge_;
    std::vector<std::uint32_t> reverse_by_edge_;
    std::vector<std::uint8_t> boundary_;
    std::vector<std::uint32_t> next_boundary_;
    std::vector<std::uint8_t> visited_;
    std::vector<std::uint8_t> outgoing_seen_;
    std::vector<std::uint8_t> boundary_predecessors_;
    std::unique_ptr<MeteredDisjointSet> components_;
    std::vector<std::uint32_t> component_by_root_;
    std::vector<std::uint8_t> component_material_;
    std::vector<RawRing> raw_rings_;
    std::vector<std::uint32_t> raw_half_edges_;
    std::vector<ComponentAdjacency> adjacency_;
    std::vector<std::uint32_t> adjacency_begin_;
    std::vector<std::uint32_t> component_depth_;
    std::vector<std::uint32_t> incoming_ring_;
    std::vector<std::uint8_t> ring_seen_;
    std::vector<std::uint32_t> queue_;
};

static_assert(sizeof(RawRing) <= kLogicalRawRingBytes);
static_assert(sizeof(ComponentAdjacency) <= kLogicalAdjacencyBytes);
static_assert(sizeof(AnalyticFilteredMaterialRing) <= kLogicalRingBytes);
static_assert(sizeof(AnalyticFilteredMaterialRegion) <= kLogicalRegionBytes);

} // namespace

AnalyticFilteredRegionsResult
build_analytic_filtered_regions(const AnalyticRequestPacketRecords& records,
                                std::uint32_t job_index, const AnalyticFilteredGeometry& geometry,
                                const std::vector<AnalyticCurvePair>& candidate_pairs,
                                const AnalyticSolverLimits& limits)
{
    analytic_selection_detail::SelectionAdmission admission =
        analytic_selection_detail::prepare_boolean_selection_admission(
            records, job_index, geometry, candidate_pairs, limits, {true, false});
    const std::uint64_t reserved_work = admission.material_regions_reserved_work;
    AnalyticFilteredBooleanSelectionResult selection =
        analytic_selection_detail::finish_boolean_selection_from_admission(
            records, job_index, geometry, std::move(admission));
    if (selection.error != AnalyticFilteredBooleanSelectionError::none)
    {
        AnalyticFilteredRegionsResult result;
        result.error = selection.error == AnalyticFilteredBooleanSelectionError::invalid_argument
                           ? AnalyticFilteredRegionsError::invalid_argument
                           : AnalyticFilteredRegionsError::resource_limit_exceeded;
        result.telemetry.selection_predicate_calls = selection.telemetry.predicate_calls;
        result.telemetry.selection_peak_working_memory_bytes =
            selection.telemetry.peak_working_memory_bytes;
        result.telemetry.predicate_calls = selection.telemetry.predicate_calls;
        result.telemetry.peak_working_memory_bytes = selection.telemetry.peak_working_memory_bytes;
        result.telemetry.algebraic_fallback_calls = selection.telemetry.algebraic_fallback_calls;
        result.selection.origin_x_nm = geometry.origin_x_nm;
        result.selection.origin_y_nm = geometry.origin_y_nm;
        result.selection.telemetry = selection.telemetry;
        return result;
    }
    return RegionsBuilder(std::move(selection), limits, reserved_work).build();
}

namespace analytic_regions_detail
{

LineageRegionsAdmission
build_regions_for_lineage(const AnalyticRequestPacketRecords& records, std::uint32_t job_index,
                          const AnalyticFilteredGeometry& geometry,
                          const std::vector<AnalyticCurvePair>& candidate_pairs,
                          const AnalyticSolverLimits& limits, bool reserve_outcomes)
{
    LineageRegionsAdmission output;
    analytic_selection_detail::SelectionAdmission admission =
        analytic_selection_detail::prepare_boolean_selection_admission(
            records, job_index, geometry, candidate_pairs, limits, {true, true, reserve_outcomes});
    const std::uint64_t region_work = admission.material_regions_reserved_work;
    output.reserved_lineage_work = admission.lineage_reserved_work;
    output.reserved_outcomes_work = admission.outcomes_reserved_work;
    AnalyticFilteredBooleanSelectionResult selection =
        analytic_selection_detail::finish_boolean_selection_from_admission(
            records, job_index, geometry, std::move(admission));
    if (selection.error != AnalyticFilteredBooleanSelectionError::none)
    {
        output.regions.error =
            selection.error == AnalyticFilteredBooleanSelectionError::invalid_argument
                ? AnalyticFilteredRegionsError::invalid_argument
                : AnalyticFilteredRegionsError::resource_limit_exceeded;
        output.regions.telemetry.selection_predicate_calls = selection.telemetry.predicate_calls;
        output.regions.telemetry.selection_peak_working_memory_bytes =
            selection.telemetry.peak_working_memory_bytes;
        output.regions.telemetry.predicate_calls = selection.telemetry.predicate_calls;
        output.regions.telemetry.peak_working_memory_bytes =
            selection.telemetry.peak_working_memory_bytes;
        output.regions.telemetry.algebraic_fallback_calls =
            selection.telemetry.algebraic_fallback_calls;
        output.regions.selection.origin_x_nm = geometry.origin_x_nm;
        output.regions.selection.origin_y_nm = geometry.origin_y_nm;
        output.regions.selection.telemetry = selection.telemetry;
        return output;
    }
    output.regions = RegionsBuilder(std::move(selection), limits, region_work).build();
    return output;
}

} // namespace analytic_regions_detail

} // namespace geometer

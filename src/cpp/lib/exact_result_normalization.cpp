#include "geometer/exact_result_normalization.h"

#include "geometer/exact_arc_distance.h"
#include "geometer/exact_normalization.h"

#include <algorithm>
#include <exception>
#include <limits>
#include <map>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace geometer::exact
{
namespace
{

constexpr std::uint32_t kNone = std::numeric_limits<std::uint32_t>::max();
constexpr std::uint64_t kMaximumRecords = 8'388'608;
constexpr std::uint64_t kVertexStorageBytes =
    256; // normalized/replay/output records plus two ordered-map nodes
constexpr std::uint64_t kFragmentStorageBytes =
    512; // replay records, copied curves/memberships, maps, and output records
constexpr std::uint64_t kRingStorageBytes = 128;
constexpr std::uint64_t kRegionStorageBytes = 128;
constexpr std::uint64_t kVertexWorkUnits =
    2304; // three ordered maps times the maximum 24 comparisons at the record cap

std::uint64_t checked_add(std::uint64_t left, std::uint64_t right)
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
        throw std::overflow_error("normalized-result size addition overflow");
    return left + right;
}

std::uint64_t checked_multiply(std::uint64_t left, std::uint64_t right)
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left)
        throw std::overflow_error("normalized-result size multiplication overflow");
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

ExactNormalizedBooleanResultResult failure(ExactResultNormalizationError error)
{
    return {error, std::nullopt};
}

ExactResultNormalizationError map_error(Error error)
{
    return error == Error::resource_limit_exceeded
               ? ExactResultNormalizationError::resource_limit_exceeded
               : ExactResultNormalizationError::invalid_argument;
}

struct OriginalTopology
{
    std::vector<std::uint32_t> half_edge_ring;
    std::vector<std::uint32_t> ring_region;
};

ExactResultNormalizationError validate_original_topology(const ExactArrangement& arrangement,
                                                         const ExactBooleanSelection& selection,
                                                         const ExactBooleanRegions& regions,
                                                         OriginalTopology& topology)
{
    if (selection.faces().size() != arrangement.faces().size())
        return ExactResultNormalizationError::invalid_argument;
    if (arrangement.half_edges().size() > kMaximumRecords ||
        regions.ring_half_edges().size() > kMaximumRecords ||
        regions.rings().size() > kMaximumRecords || regions.regions().size() > kMaximumRecords)
        return ExactResultNormalizationError::resource_limit_exceeded;
    topology.half_edge_ring.assign(arrangement.half_edges().size(), kNone);
    topology.ring_region.assign(regions.rings().size(), kNone);
    std::uint64_t cursor = 0;
    for (std::uint32_t ring_id = 0; ring_id < regions.rings().size(); ++ring_id)
    {
        const ExactResultRing& ring = regions.rings()[ring_id];
        const std::uint64_t end =
            static_cast<std::uint64_t>(ring.half_edge_begin) + ring.half_edge_count;
        if (ring.half_edge_begin != cursor || ring.half_edge_count == 0 ||
            end > regions.ring_half_edges().size() || ring.depth % 2 == ring.counterclockwise)
            return ExactResultNormalizationError::invalid_argument;
        if ((ring.parent_ring == kNoExactResultRing) != (ring.depth == 0) ||
            (ring.parent_ring != kNoExactResultRing &&
             (ring.parent_ring >= regions.rings().size() ||
              regions.rings()[ring.parent_ring].depth + 1 != ring.depth)))
            return ExactResultNormalizationError::invalid_argument;
        for (std::uint32_t index = 0; index < ring.half_edge_count; ++index)
        {
            const std::uint32_t half_edge = regions.ring_half_edges()[ring.half_edge_begin + index];
            const std::uint32_t next =
                regions
                    .ring_half_edges()[ring.half_edge_begin + (index + 1) % ring.half_edge_count];
            if (half_edge >= arrangement.half_edges().size() ||
                next >= arrangement.half_edges().size() ||
                topology.half_edge_ring[half_edge] != kNone)
                return ExactResultNormalizationError::invalid_argument;
            const ExactArrangementHalfEdge& value = arrangement.half_edges()[half_edge];
            if (value.twin >= arrangement.half_edges().size() ||
                value.face >= selection.faces().size() ||
                value.origin_vertex >= arrangement.vertices().size() ||
                arrangement.half_edges()[value.twin].origin_vertex >=
                    arrangement.vertices().size() ||
                arrangement.half_edges()[value.twin].face >= selection.faces().size() ||
                !selection.faces()[value.face].material ||
                selection.faces()[arrangement.half_edges()[value.twin].face].material ||
                arrangement.half_edges()[value.twin].origin_vertex !=
                    arrangement.half_edges()[next].origin_vertex)
                return ExactResultNormalizationError::invalid_argument;
            topology.half_edge_ring[half_edge] = ring_id;
        }
        cursor = end;
    }
    if (cursor != regions.ring_half_edges().size())
        return ExactResultNormalizationError::invalid_argument;
    for (std::uint32_t half_edge = 0; half_edge < arrangement.half_edges().size(); ++half_edge)
    {
        const ExactArrangementHalfEdge& value = arrangement.half_edges()[half_edge];
        if (value.twin >= arrangement.half_edges().size() ||
            value.face >= selection.faces().size() ||
            arrangement.half_edges()[value.twin].face >= selection.faces().size())
            return ExactResultNormalizationError::invalid_argument;
        const bool boundary =
            selection.faces()[value.face].material &&
            !selection.faces()[arrangement.half_edges()[value.twin].face].material;
        if ((topology.half_edge_ring[half_edge] != kNone) != boundary)
            return ExactResultNormalizationError::invalid_argument;
    }

    std::vector<bool> seen_outer(regions.rings().size());
    for (std::uint32_t region = 0; region < regions.regions().size(); ++region)
    {
        const std::uint32_t outer = regions.regions()[region].outer_ring;
        if (outer >= regions.rings().size() || regions.rings()[outer].depth % 2 != 0 ||
            seen_outer[outer])
            return ExactResultNormalizationError::invalid_argument;
        seen_outer[outer] = true;
        topology.ring_region[outer] = region;
    }
    for (std::uint32_t ring = 0; ring < regions.rings().size(); ++ring)
    {
        if (regions.rings()[ring].depth % 2 == 0)
        {
            if (!seen_outer[ring])
                return ExactResultNormalizationError::invalid_argument;
        }
        else
        {
            const std::uint32_t parent = regions.rings()[ring].parent_ring;
            if (parent == kNoExactResultRing || topology.ring_region[parent] == kNone)
                return ExactResultNormalizationError::invalid_argument;
            topology.ring_region[ring] = topology.ring_region[parent];
        }
    }
    return ExactResultNormalizationError::none;
}

struct NormalizedPoint
{
    std::int64_t x = 0;
    std::int64_t y = 0;
    ExactPoint exact;
    std::uint32_t original = 0;
};

ExactResultNormalizationError normalize_point(ConstructionArena& arena, const ExactPoint& point,
                                              std::uint32_t original, NormalizedPoint& output)
{
    if (point.x >= arena.size() || point.y >= arena.size())
        return ExactResultNormalizationError::invalid_argument;
    auto x = normalize_exact_to_integer_nm(arena.budget(), arena.at(point.x).value());
    auto y = normalize_exact_to_integer_nm(arena.budget(), arena.at(point.y).value());
    if (x.error != Error::none || y.error != Error::none || !x.value || !y.value)
        return map_error(x.error != Error::none ? x.error : y.error);
    ConstructionBuilder builder(arena);
    output.x = *x.value;
    output.y = *y.value;
    output.original = original;
    output.exact = {builder.rational(output.x), builder.rational(output.y)};
    const ConstructionNodeId dx = builder.subtract(point.x, output.exact.x);
    const ConstructionNodeId dy = builder.subtract(point.y, output.exact.y);
    const ConstructionNodeId dx_squared = builder.square(dx);
    const ConstructionNodeId dy_squared = builder.square(dy);
    const ConstructionNodeId displacement = builder.sum(dx_squared, dy_squared);
    const ConstructionNodeId limit = builder.rational(1, 2);
    const std::int8_t comparison = builder.compare(displacement, limit);
    if (!builder.good())
        return map_error(builder.error());
    return comparison <= 0 ? ExactResultNormalizationError::none
                           : ExactResultNormalizationError::normalization_error_exceeded;
}

struct ReplayFragment
{
    ExactAtomicCurve curve;
    ExactCoverageOccurrence coverage;
    ExactNormalizedResultFragment output;
    std::uint32_t exact_ring = 0;
};

ExactResultNormalizationError
make_replay_fragment(ConstructionArena& arena, const ExactArrangement& arrangement,
                     std::uint32_t half_edge_id, std::uint32_t exact_ring,
                     std::uint32_t exact_region,
                     const std::vector<NormalizedPoint>& normalized_points, ReplayFragment& replay)
{
    const ExactArrangementHalfEdge& half_edge = arrangement.half_edges()[half_edge_id];
    const ExactArrangementHalfEdge& twin = arrangement.half_edges()[half_edge.twin];
    if (half_edge.edge >= arrangement.edges().size() ||
        half_edge.origin_vertex >= normalized_points.size() ||
        twin.origin_vertex >= normalized_points.size())
        return ExactResultNormalizationError::invalid_argument;
    const ExactArrangementEdge& edge = arrangement.edges()[half_edge.edge];
    const NormalizedPoint& start = normalized_points[half_edge.origin_vertex];
    const NormalizedPoint& end = normalized_points[twin.origin_vertex];
    if (start.x == end.x && start.y == end.y)
        return ExactResultNormalizationError::normalization_topology_collapse;

    replay.output.kind = edge.kind;
    replay.curve.kind = edge.kind;
    replay.output.arrangement_half_edge = half_edge_id;
    replay.exact_ring = exact_ring;
    replay.coverage = {static_cast<std::uint64_t>(half_edge_id) + 1,
                       static_cast<std::uint64_t>(exact_region) + 1, true};
    replay.curve.start = start.exact;
    replay.curve.end = end.exact;
    if (edge.kind == ExactAtomicCurveKind::line)
    {
        replay.output.direction = NormalizedFragmentDirection::not_applicable;
        const bool agrees = start.x < end.x || (start.x == end.x && start.y < end.y);
        replay.curve.memberships.push_back({replay.coverage.occurrence_id, agrees});
        return ExactResultNormalizationError::none;
    }
    if (edge.kind != ExactAtomicCurveKind::circular_arc || edge.circle.radius >= arena.size())
        return ExactResultNormalizationError::invalid_argument;

    auto radius =
        normalize_exact_to_integer_nm(arena.budget(), arena.at(edge.circle.radius).value());
    if (radius.error != Error::none || !radius.value)
        return map_error(radius.error);
    if (*radius.value <= 0)
        return ExactResultNormalizationError::normalization_topology_collapse;
    const bool counterclockwise =
        half_edge.forward ? edge.counterclockwise : !edge.counterclockwise;
    replay.output.direction = counterclockwise ? NormalizedFragmentDirection::counterclockwise
                                               : NormalizedFragmentDirection::clockwise;
    replay.output.major_arc = edge.major_arc;
    replay.output.radius_nm = static_cast<std::uint64_t>(*radius.value);

    ConstructionBuilder builder(arena);
    const ConstructionNodeId radius_node = builder.rational(*radius.value);
    const ExactPoint& original_start = arrangement.vertices()[half_edge.origin_vertex].point;
    const ExactPoint& original_end = arrangement.vertices()[twin.origin_vertex].point;

    const ConstructionNodeId two = builder.rational(2);
    const ConstructionNodeId four = builder.rational(4);
    const ConstructionNodeId dx = builder.subtract(end.exact.x, start.exact.x);
    const ConstructionNodeId dy = builder.subtract(end.exact.y, start.exact.y);
    const ConstructionNodeId dx_squared = builder.square(dx);
    const ConstructionNodeId dy_squared = builder.square(dy);
    const ConstructionNodeId chord_squared = builder.sum(dx_squared, dy_squared);
    const ConstructionNodeId radius_squared = builder.square(radius_node);
    const ConstructionNodeId four_radius_squared = builder.product(four, radius_squared);
    const ConstructionNodeId gap = builder.subtract(four_radius_squared, chord_squared);
    const std::int8_t gap_sign = builder.sign(gap);
    if (!builder.good())
        return map_error(builder.error());
    if (gap_sign < 0 || (gap_sign == 0 && edge.major_arc))
        return ExactResultNormalizationError::normalization_topology_collapse;
    const ConstructionNodeId denominator = builder.product(four, chord_squared);
    const ConstructionNodeId scale_squared = builder.divide(gap, denominator);
    const ConstructionNodeId scale = builder.square_root(scale_squared);
    const ConstructionNodeId endpoint_x_sum = builder.sum(start.exact.x, end.exact.x);
    const ConstructionNodeId endpoint_y_sum = builder.sum(start.exact.y, end.exact.y);
    const ConstructionNodeId midpoint_x = builder.divide(endpoint_x_sum, two);
    const ConstructionNodeId midpoint_y = builder.divide(endpoint_y_sum, two);
    const bool positive_side = counterclockwise != edge.major_arc;
    const ConstructionNodeId signed_scale = positive_side ? scale : builder.negate(scale);
    const ConstructionNodeId x_offset = builder.product(dy, signed_scale);
    const ConstructionNodeId y_offset = builder.product(dx, signed_scale);
    const ConstructionNodeId center_x = builder.subtract(midpoint_x, x_offset);
    const ConstructionNodeId center_y = builder.sum(midpoint_y, y_offset);
    if (!builder.good())
        return map_error(builder.error());
    replay.curve.circle = {{center_x, center_y}, radius_node};
    replay.curve.counterclockwise = counterclockwise;
    replay.curve.major_arc = edge.major_arc;
    replay.curve.memberships.push_back({replay.coverage.occurrence_id, counterclockwise});
    const ExactCircularArc original_arc{edge.circle, original_start, original_end, counterclockwise,
                                        edge.major_arc};
    const ExactCircularArc replay_arc{replay.curve.circle, replay.curve.start, replay.curve.end,
                                      counterclockwise, edge.major_arc};
    ExactPredicateResult certified =
        exact_arc_hausdorff_within(arena, original_arc, replay_arc, 5, 4);
    if (certified.error != Error::none || !certified.value)
        return map_error(certified.error);
    if (!*certified.value)
        return ExactResultNormalizationError::normalization_error_exceeded;
    return ExactResultNormalizationError::none;
}

ExactResultNormalizationError
project_replay_vertices(const ExactArrangement& replay_arrangement,
                        const std::vector<NormalizedPoint>& normalized_points,
                        const std::vector<bool>& used_vertices, std::size_t expected_vertex_count,
                        std::vector<ExactNormalizedResultVertex>& output)
{
    std::map<std::pair<ConstructionNodeId, ConstructionNodeId>, NormalizedPoint> point_by_node;
    for (std::uint32_t vertex = 0; vertex < normalized_points.size(); ++vertex)
        if (used_vertices[vertex])
            point_by_node.emplace(
                std::pair{normalized_points[vertex].exact.x, normalized_points[vertex].exact.y},
                normalized_points[vertex]);
    output.reserve(replay_arrangement.vertices().size());
    for (const ExactArrangementVertex& vertex : replay_arrangement.vertices())
    {
        const auto found = point_by_node.find({vertex.point.x, vertex.point.y});
        if (found == point_by_node.end())
            return ExactResultNormalizationError::normalization_topology_collapse;
        output.push_back({found->second.x, found->second.y, found->second.original});
    }
    return output.size() == expected_vertex_count
               ? ExactResultNormalizationError::none
               : ExactResultNormalizationError::normalization_topology_collapse;
}

} // namespace

ExactNormalizedBooleanResult::ExactNormalizedBooleanResult(
    Budget& budget, std::uint64_t charged_bytes, std::vector<ExactNormalizedResultVertex> vertices,
    std::vector<ExactNormalizedResultFragment> fragments,
    std::vector<ExactNormalizedResultRing> rings,
    std::vector<ExactNormalizedResultRegion> regions) noexcept
    : budget_(&budget), charged_bytes_(charged_bytes), vertices_(std::move(vertices)),
      fragments_(std::move(fragments)), rings_(std::move(rings)), regions_(std::move(regions))
{
}

ExactNormalizedBooleanResult::~ExactNormalizedBooleanResult()
{
    release();
}

ExactNormalizedBooleanResult::ExactNormalizedBooleanResult(
    ExactNormalizedBooleanResult&& other) noexcept
    : budget_(std::exchange(other.budget_, nullptr)),
      charged_bytes_(std::exchange(other.charged_bytes_, 0)), vertices_(std::move(other.vertices_)),
      fragments_(std::move(other.fragments_)), rings_(std::move(other.rings_)),
      regions_(std::move(other.regions_))
{
}

ExactNormalizedBooleanResult&
ExactNormalizedBooleanResult::operator=(ExactNormalizedBooleanResult&& other) noexcept
{
    if (this != &other)
    {
        release();
        budget_ = std::exchange(other.budget_, nullptr);
        charged_bytes_ = std::exchange(other.charged_bytes_, 0);
        vertices_ = std::move(other.vertices_);
        fragments_ = std::move(other.fragments_);
        rings_ = std::move(other.rings_);
        regions_ = std::move(other.regions_);
    }
    return *this;
}

void ExactNormalizedBooleanResult::release()
{
    if (budget_ != nullptr)
        budget_->release_storage(charged_bytes_);
    budget_ = nullptr;
    charged_bytes_ = 0;
}

const std::vector<ExactNormalizedResultVertex>& ExactNormalizedBooleanResult::vertices() const
{
    return vertices_;
}

const std::vector<ExactNormalizedResultFragment>& ExactNormalizedBooleanResult::fragments() const
{
    return fragments_;
}

const std::vector<ExactNormalizedResultRing>& ExactNormalizedBooleanResult::rings() const
{
    return rings_;
}

const std::vector<ExactNormalizedResultRegion>& ExactNormalizedBooleanResult::regions() const
{
    return regions_;
}

ExactNormalizedBooleanResultResult
normalize_exact_boolean_result(ConstructionArena& arena, const ExactArrangement& arrangement,
                               const ExactBooleanSelection& selection,
                               const ExactBooleanRegions& regions)
{
    try
    {
        OriginalTopology original;
        if (const auto error =
                validate_original_topology(arrangement, selection, regions, original);
            error != ExactResultNormalizationError::none)
            return failure(error);
        const std::uint64_t vertex_count = arrangement.vertices().size();
        const std::uint64_t fragment_count = regions.ring_half_edges().size();
        const std::uint64_t ring_count = regions.rings().size();
        const std::uint64_t region_count = regions.regions().size();
        if (vertex_count > kMaximumRecords || fragment_count > kMaximumRecords)
            return failure(ExactResultNormalizationError::resource_limit_exceeded);
        const std::uint64_t charge = checked_add(
            4096,
            checked_add(
                checked_multiply(vertex_count, kVertexStorageBytes),
                checked_add(checked_multiply(fragment_count, kFragmentStorageBytes),
                            checked_add(checked_multiply(ring_count, kRingStorageBytes),
                                        checked_multiply(region_count, kRegionStorageBytes)))));
        const std::uint64_t work = checked_add(
            256, checked_add(checked_multiply(vertex_count, kVertexWorkUnits),
                             checked_add(checked_multiply(fragment_count, 256),
                                         checked_add(checked_multiply(ring_count, 128),
                                                     checked_multiply(region_count, 128)))));
        if (!arena.budget().consume_work(work))
            return failure(ExactResultNormalizationError::resource_limit_exceeded);
        StorageReservation reservation(arena.budget(), charge);
        if (!reservation.acquired())
            return failure(ExactResultNormalizationError::resource_limit_exceeded);
        ConstructionArenaTransaction transaction(arena);

        std::vector<bool> used_vertices(vertex_count);
        for (const std::uint32_t half_edge_id : regions.ring_half_edges())
        {
            const ExactArrangementHalfEdge& half_edge = arrangement.half_edges()[half_edge_id];
            used_vertices[half_edge.origin_vertex] = true;
            used_vertices[arrangement.half_edges()[half_edge.twin].origin_vertex] = true;
        }
        std::vector<NormalizedPoint> normalized_points(vertex_count);
        std::map<std::pair<std::int64_t, std::int64_t>, std::uint32_t> coordinate_owner;
        for (std::uint32_t vertex = 0; vertex < vertex_count; ++vertex)
        {
            if (!used_vertices[vertex])
                continue;
            if (const auto error = normalize_point(arena, arrangement.vertices()[vertex].point,
                                                   vertex, normalized_points[vertex]);
                error != ExactResultNormalizationError::none)
                return failure(error);
            const auto coordinate =
                std::pair{normalized_points[vertex].x, normalized_points[vertex].y};
            if (!coordinate_owner.emplace(coordinate, vertex).second)
                return failure(ExactResultNormalizationError::normalization_topology_collapse);
        }

        std::vector<ReplayFragment> replay_fragments;
        replay_fragments.reserve(fragment_count);
        std::vector<std::uint32_t> replay_index_by_half_edge(arrangement.half_edges().size(),
                                                             kNone);
        std::vector<ExactAtomicCurve> replay_curves;
        replay_curves.reserve(fragment_count);
        std::vector<ExactCoverageOccurrence> replay_coverages;
        replay_coverages.reserve(fragment_count);
        for (std::uint32_t exact_ring = 0; exact_ring < ring_count; ++exact_ring)
        {
            const ExactResultRing& ring = regions.rings()[exact_ring];
            for (std::uint32_t index = 0; index < ring.half_edge_count; ++index)
            {
                const std::uint32_t half_edge =
                    regions.ring_half_edges()[ring.half_edge_begin + index];
                ReplayFragment replay;
                if (const auto error = make_replay_fragment(
                        arena, arrangement, half_edge, exact_ring, original.ring_region[exact_ring],
                        normalized_points, replay);
                    error != ExactResultNormalizationError::none)
                    return failure(error);
                replay_curves.push_back(replay.curve);
                replay_coverages.push_back(replay.coverage);
                replay_index_by_half_edge[half_edge] =
                    static_cast<std::uint32_t>(replay_fragments.size());
                replay_fragments.push_back(std::move(replay));
            }
        }

        ExactArrangementResult replay_arrangement =
            build_exact_arrangement(arena, replay_curves, replay_coverages);
        if (replay_arrangement.error != Error::none || !replay_arrangement.value)
            return failure(replay_arrangement.error == Error::resource_limit_exceeded
                               ? ExactResultNormalizationError::resource_limit_exceeded
                               : ExactResultNormalizationError::normalization_topology_collapse);
        if (replay_arrangement.value->edges().size() != fragment_count ||
            replay_arrangement.value->memberships().size() != fragment_count)
            return failure(ExactResultNormalizationError::normalization_topology_collapse);
        std::vector<ExactBooleanOperand> replay_operands;
        replay_operands.reserve(region_count);
        for (std::uint32_t region = 0; region < region_count; ++region)
            replay_operands.push_back(
                {static_cast<std::uint64_t>(region) + 1, static_cast<std::uint64_t>(region) + 1});
        const std::vector<ExactBooleanStage> replay_stages{
            {1, ExactBooleanStageOperation::union_, std::move(replay_operands)},
        };
        ExactBooleanSelectionResult replay_selection =
            evaluate_exact_boolean_stages(arena.budget(), *replay_arrangement.value, replay_stages);
        if (replay_selection.error != Error::none || !replay_selection.value)
            return failure(replay_selection.error == Error::resource_limit_exceeded
                               ? ExactResultNormalizationError::resource_limit_exceeded
                               : ExactResultNormalizationError::normalization_topology_collapse);
        ExactBooleanRegionsResult replay_regions = build_exact_boolean_regions(
            arena.budget(), *replay_arrangement.value, *replay_selection.value);
        if (replay_regions.error != Error::none || !replay_regions.value)
            return failure(replay_regions.error == Error::resource_limit_exceeded
                               ? ExactResultNormalizationError::resource_limit_exceeded
                               : ExactResultNormalizationError::normalization_topology_collapse);
        if (replay_regions.value->rings().size() != ring_count ||
            replay_regions.value->regions().size() != region_count ||
            replay_regions.value->ring_half_edges().size() != fragment_count)
            return failure(ExactResultNormalizationError::normalization_topology_collapse);

        std::vector<ExactNormalizedResultVertex> output_vertices;
        if (const auto error =
                project_replay_vertices(*replay_arrangement.value, normalized_points, used_vertices,
                                        coordinate_owner.size(), output_vertices);
            error != ExactResultNormalizationError::none)
            return failure(error);

        std::vector<std::uint32_t> exact_to_replay_ring(ring_count, kNone);
        std::vector<ExactNormalizedResultFragment> output_fragments;
        std::vector<ExactNormalizedResultRing> output_rings;
        output_fragments.reserve(fragment_count);
        output_rings.reserve(ring_count);
        for (std::uint32_t replay_ring = 0; replay_ring < ring_count; ++replay_ring)
        {
            const ExactResultRing& ring = replay_regions.value->rings()[replay_ring];
            ExactNormalizedResultRing output_ring;
            output_ring.fragment_begin = static_cast<std::uint32_t>(output_fragments.size());
            output_ring.fragment_count = ring.half_edge_count;
            output_ring.parent_ring = ring.parent_ring;
            output_ring.depth = ring.depth;
            output_ring.counterclockwise = ring.counterclockwise;
            std::uint32_t mapped_exact_ring = kNone;
            for (std::uint32_t index = 0; index < ring.half_edge_count; ++index)
            {
                const std::uint32_t replay_half_edge =
                    replay_regions.value->ring_half_edges()[ring.half_edge_begin + index];
                const ExactArrangementHalfEdge& half_edge =
                    replay_arrangement.value->half_edges()[replay_half_edge];
                const ExactArrangementEdge& edge =
                    replay_arrangement.value->edges()[half_edge.edge];
                if (edge.membership_count != 1)
                    return failure(ExactResultNormalizationError::normalization_topology_collapse);
                const ExactCurveMembership& membership =
                    replay_arrangement.value->memberships()[edge.membership_begin];
                if (membership.occurrence_id == 0 ||
                    membership.occurrence_id > arrangement.half_edges().size())
                    return failure(ExactResultNormalizationError::normalization_topology_collapse);
                const std::uint32_t original_half_edge =
                    static_cast<std::uint32_t>(membership.occurrence_id - 1);
                const std::uint32_t exact_ring = original.half_edge_ring[original_half_edge];
                if (exact_ring == kNone ||
                    (mapped_exact_ring != kNone && mapped_exact_ring != exact_ring))
                    return failure(ExactResultNormalizationError::normalization_topology_collapse);
                mapped_exact_ring = exact_ring;
                const std::uint32_t replay_index = replay_index_by_half_edge[original_half_edge];
                if (replay_index == kNone || replay_index >= replay_fragments.size())
                    return failure(ExactResultNormalizationError::normalization_topology_collapse);
                const ReplayFragment& replay = replay_fragments[replay_index];
                const ExactPoint& replay_start =
                    replay_arrangement.value->vertices()[half_edge.origin_vertex].point;
                const ExactPoint& replay_end =
                    replay_arrangement.value
                        ->vertices()[replay_arrangement.value->half_edges()[half_edge.twin]
                                         .origin_vertex]
                        .point;
                if (replay_start.x != replay.curve.start.x ||
                    replay_start.y != replay.curve.start.y || replay_end.x != replay.curve.end.x ||
                    replay_end.y != replay.curve.end.y)
                    return failure(ExactResultNormalizationError::normalization_topology_collapse);
                ExactNormalizedResultFragment output = replay.output;
                output.start_vertex = half_edge.origin_vertex;
                output.end_vertex =
                    replay_arrangement.value->half_edges()[half_edge.twin].origin_vertex;
                output_fragments.push_back(output);
            }
            if (mapped_exact_ring == kNone || exact_to_replay_ring[mapped_exact_ring] != kNone)
                return failure(ExactResultNormalizationError::normalization_topology_collapse);
            output_ring.exact_ring = mapped_exact_ring;
            exact_to_replay_ring[mapped_exact_ring] = replay_ring;
            output_rings.push_back(output_ring);
        }
        for (std::uint32_t replay_ring = 0; replay_ring < ring_count; ++replay_ring)
        {
            const ExactNormalizedResultRing& output = output_rings[replay_ring];
            const ExactResultRing& exact = regions.rings()[output.exact_ring];
            const std::uint32_t mapped_parent = output.parent_ring == kNoExactResultRing
                                                    ? kNoExactResultRing
                                                    : output_rings[output.parent_ring].exact_ring;
            if (output.depth != exact.depth || output.counterclockwise != exact.counterclockwise ||
                mapped_parent != exact.parent_ring)
                return failure(ExactResultNormalizationError::normalization_topology_collapse);
        }

        std::vector<bool> seen_exact_region(region_count);
        std::vector<ExactNormalizedResultRegion> output_regions;
        output_regions.reserve(region_count);
        for (const ExactResultRegion& replay_region : replay_regions.value->regions())
        {
            if (replay_region.positive_source_count != 1 ||
                replay_region.positive_source_begin >=
                    replay_regions.value->positive_sources().size())
                return failure(ExactResultNormalizationError::normalization_topology_collapse);
            const std::uint64_t source =
                replay_regions.value->positive_sources()[replay_region.positive_source_begin];
            if (source == 0 || source > region_count || seen_exact_region[source - 1])
                return failure(ExactResultNormalizationError::normalization_topology_collapse);
            const std::uint32_t exact_region = static_cast<std::uint32_t>(source - 1);
            seen_exact_region[exact_region] = true;
            if (output_rings[replay_region.outer_ring].exact_ring !=
                regions.regions()[exact_region].outer_ring)
                return failure(ExactResultNormalizationError::normalization_topology_collapse);
            output_regions.push_back({replay_region.outer_ring, exact_region});
        }

        const std::uint64_t transferred = reservation.transfer();
        return {ExactResultNormalizationError::none,
                ExactNormalizedBooleanResult(arena.budget(), transferred,
                                             std::move(output_vertices),
                                             std::move(output_fragments), std::move(output_rings),
                                             std::move(output_regions))};
    }
    catch (const std::exception&)
    {
        return failure(ExactResultNormalizationError::resource_limit_exceeded);
    }
}

} // namespace geometer::exact

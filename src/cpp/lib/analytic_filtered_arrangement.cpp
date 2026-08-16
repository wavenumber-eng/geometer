#include "geometer/analytic_filtered_arrangement.h"

#include "analytic_filtered_capacity.h"
#include "analytic_filtered_interval.h"
#include "analytic_interval_index.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

namespace geometer
{
namespace
{
using analytic_detail::complete_distance_squared;
using analytic_detail::cross;
using analytic_detail::dot;
using analytic_detail::enclosure_radius_squared;
using analytic_detail::exact;
using analytic_detail::Interval;
using analytic_detail::negate;
using analytic_detail::Point;
using analytic_detail::subtract;

constexpr std::uint32_t kNoIndex = std::numeric_limits<std::uint32_t>::max();
constexpr std::uint64_t kEndpointLogicalBytes = 64;
constexpr std::uint64_t kEndpointSlotLogicalBytes = 8;
constexpr std::uint64_t kClusterLogicalBytes = 80;
constexpr std::uint64_t kExpiryLogicalBytes = 24;
constexpr std::uint64_t kVertexLogicalBytes = kAnalyticArrangementVertexLogicalBytes;
constexpr std::uint64_t kEdgeDraftLogicalBytes = 192;
constexpr std::uint64_t kEdgeLogicalBytes = kAnalyticArrangementEdgeLogicalBytes;
constexpr std::uint64_t kHalfEdgeLogicalBytes = kAnalyticArrangementHalfEdgeLogicalBytes;
constexpr std::uint64_t kCollapsedSpanLogicalBytes = kAnalyticArrangementCollapsedSpanLogicalBytes;
constexpr std::uint64_t kIndexLogicalBytes = 8;
constexpr std::uint64_t kCurveFlagLogicalBytes = 1;
constexpr std::uint64_t kCycleLogicalBytes = kAnalyticArrangementCycleLogicalBytes;

struct EndpointRecord
{
    AnalyticFilteredPointNm point;
    std::uint32_t endpoint_slot = 0;
    std::uint32_t span_offset = 0;
    bool start = true;
};

struct VertexCluster
{
    AnalyticFilteredPointNm seed;
    AnalyticFilteredPointNm hull;
};

struct ExpiryEntry
{
    double maximum_x = 0.0;
    double minimum_y = 0.0;
    std::uint32_t cluster = 0;
};

struct ExpiryLater
{
    bool operator()(const ExpiryEntry& left, const ExpiryEntry& right) const noexcept
    {
        return std::tie(left.maximum_x, left.cluster) > std::tie(right.maximum_x, right.cluster);
    }
};

struct EdgeDraft
{
    AnalyticArrangementEdgeNm edge;
    std::uint32_t span_offset = 0;
};

struct CollapsedDraft
{
    AnalyticArrangementCollapsedSpan span;
    std::uint32_t cluster = 0;
    std::uint32_t span_offset = 0;
};

struct Tangent
{
    Point direction;
    std::int8_t curvature = 0;
    Interval radius;
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

std::uint64_t checked_add(std::uint64_t left, std::uint64_t right, bool& valid) noexcept
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
    {
        valid = false;
        return 0;
    }
    return left + right;
}

std::uint64_t checked_multiply(std::uint64_t left, std::uint64_t right, bool& valid) noexcept
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left)
    {
        valid = false;
        return 0;
    }
    return left * right;
}

std::uint64_t sort_units(std::uint64_t count) noexcept
{
    if (count < 2)
        return 0;
    std::uint64_t levels = 0;
    for (std::uint64_t width = 1; width < count; width <<= 1)
    {
        ++levels;
        if (width > std::numeric_limits<std::uint64_t>::max() / 2)
            break;
    }
    bool valid = true;
    return checked_multiply(count, levels, valid);
}

std::uint64_t tree_update_units(std::uint64_t capacity) noexcept
{
    std::uint64_t levels = 1;
    for (std::uint64_t width = 1; width < capacity; width <<= 1)
    {
        ++levels;
        if (width > std::numeric_limits<std::uint64_t>::max() / 2)
            break;
    }
    return levels * 4 + 4;
}

std::uint64_t arrangement_memory_requirement(std::uint64_t curve_count, std::uint64_t spans,
                                             std::uint64_t collapsed_domains,
                                             std::uint64_t overlay_memberships,
                                             std::uint64_t result_memberships,
                                             std::uint64_t endpoints, std::uint64_t half_edges,
                                             std::uint64_t overlay_peak, bool& valid) noexcept
{
    const std::uint64_t maximum_collapsed = checked_add(spans, collapsed_domains, valid);
    std::uint64_t base = checked_multiply(spans, kAnalyticOverlaySpanLogicalBytes, valid);
    base = checked_add(
        base, checked_multiply(overlay_memberships, kAnalyticOverlayMembershipLogicalBytes, valid),
        valid);
    base = checked_add(base, checked_multiply(curve_count, kCurveFlagLogicalBytes, valid), valid);
    base = checked_add(base, checked_multiply(collapsed_domains, kIndexLogicalBytes, valid), valid);

    std::uint64_t cluster_phase = base;
    cluster_phase = checked_add(cluster_phase,
                                checked_multiply(endpoints, kEndpointLogicalBytes, valid), valid);
    cluster_phase = checked_add(
        cluster_phase, checked_multiply(endpoints, kEndpointSlotLogicalBytes, valid), valid);
    cluster_phase =
        checked_add(cluster_phase, checked_multiply(endpoints, kClusterLogicalBytes, valid), valid);
    cluster_phase =
        checked_add(cluster_phase, checked_multiply(endpoints, kExpiryLogicalBytes, valid), valid);
    cluster_phase = checked_add(
        cluster_phase,
        detail::AnalyticIntervalIndex::canonical_storage_bytes(static_cast<std::size_t>(endpoints)),
        valid);

    std::uint64_t edge_phase = base;
    edge_phase = checked_add(edge_phase,
                             checked_multiply(endpoints, kEndpointSlotLogicalBytes, valid), valid);
    edge_phase =
        checked_add(edge_phase, checked_multiply(endpoints, kClusterLogicalBytes, valid), valid);
    edge_phase =
        checked_add(edge_phase, checked_multiply(spans, kEdgeDraftLogicalBytes, valid), valid);
    edge_phase =
        checked_add(edge_phase, checked_multiply(endpoints, kIndexLogicalBytes * 2, valid), valid);
    edge_phase =
        checked_add(edge_phase, checked_multiply(endpoints, kVertexLogicalBytes, valid), valid);
    edge_phase = checked_add(edge_phase, checked_multiply(spans, kEdgeLogicalBytes, valid), valid);
    edge_phase = checked_add(
        edge_phase, checked_multiply(maximum_collapsed, kCollapsedSpanLogicalBytes, valid), valid);
    edge_phase = checked_add(
        edge_phase, checked_multiply(maximum_collapsed, kCollapsedSpanLogicalBytes, valid), valid);
    edge_phase = checked_add(
        edge_phase,
        checked_multiply(result_memberships, kAnalyticOverlayMembershipLogicalBytes, valid), valid);

    std::uint64_t cycle_phase = base;
    cycle_phase =
        checked_add(cycle_phase, checked_multiply(endpoints, kVertexLogicalBytes, valid), valid);
    cycle_phase =
        checked_add(cycle_phase, checked_multiply(spans, kEdgeLogicalBytes, valid), valid);
    cycle_phase = checked_add(
        cycle_phase, checked_multiply(maximum_collapsed, kCollapsedSpanLogicalBytes, valid), valid);
    cycle_phase = checked_add(
        cycle_phase,
        checked_multiply(result_memberships, kAnalyticOverlayMembershipLogicalBytes, valid), valid);
    cycle_phase =
        checked_add(cycle_phase, checked_multiply(half_edges, kHalfEdgeLogicalBytes, valid), valid);
    cycle_phase =
        checked_add(cycle_phase, checked_multiply(half_edges, kIndexLogicalBytes, valid), valid);
    cycle_phase =
        checked_add(cycle_phase, checked_multiply(endpoints, kIndexLogicalBytes * 2, valid), valid);
    cycle_phase =
        checked_add(cycle_phase, checked_multiply(half_edges, kIndexLogicalBytes, valid), valid);
    cycle_phase =
        checked_add(cycle_phase, checked_multiply(half_edges, kCycleLogicalBytes, valid), valid);
    cycle_phase =
        checked_add(cycle_phase, checked_multiply(half_edges, kIndexLogicalBytes, valid), valid);

    return std::max({cluster_phase, edge_phase, cycle_phase, overlay_peak});
}

double midpoint(const AnalyticCoordinateIntervalNm& value) noexcept
{
    return value.lower + (value.upper - value.lower) * 0.5;
}

Point point(const AnalyticFilteredPointNm& value) noexcept
{
    return {{value.x.lower, value.x.upper}, {value.y.lower, value.y.upper}};
}

bool valid_coordinate(const AnalyticCoordinateIntervalNm& value) noexcept
{
    return std::isfinite(value.lower) && std::isfinite(value.upper) && value.lower <= value.upper;
}

bool valid_point(const AnalyticFilteredPointNm& value) noexcept
{
    if (!valid_coordinate(value.x) || !valid_coordinate(value.y))
        return false;
    return enclosure_radius_squared(point(value)).upper <=
           static_cast<double>(kAnalyticTopologyResolutionNm * kAnalyticTopologyResolutionNm);
}

bool complete_points_within_resolution(const AnalyticFilteredPointNm& left,
                                       const AnalyticFilteredPointNm& right) noexcept
{
    const Interval distance_squared = complete_distance_squared(point(left), point(right));
    return distance_squared.upper <=
           static_cast<double>(kAnalyticTopologyResolutionNm * kAnalyticTopologyResolutionNm);
}

bool curve_guarantees_overlay_span(const AnalyticAtomicCurveNm& curve) noexcept
{
    if (!valid_point(curve.start) || !valid_point(curve.end))
        return false;
    return (curve.kind == AnalyticAtomicCurveKind::circular_arc && curve.major_arc) ||
           !complete_points_within_resolution(curve.start, curve.end);
}

struct GuaranteedCarrierCounts
{
    std::uint64_t spans = 0;
    std::uint64_t collapsed_vertices = 0;
    std::uint64_t possible_base_spans = 0;
    std::uint64_t possible_repeated_base_spans = 0;
    std::uint64_t possible_base_memberships = 0;
    std::uint64_t possible_collapsed_domains = 0;
    std::uint64_t possible_circular_carrier_groups = 0;
};

bool guaranteed_carrier_counts(const AnalyticFilteredGeometry& geometry,
                               GuaranteedCarrierCounts& counts) noexcept
{
    counts = {};
    if (geometry.curves.size() != geometry.bounds.size() ||
        geometry.curves.size() != geometry.occurrences.size())
        return false;
    // Lowering assigns dense carrier ids on first use. Track that stream with
    // O(1) state; a strictly increasing prefix also admits hand-built internal
    // fixtures. If a malformed/noncanonical stream appears, retain only the
    // already-proven distinct prefix/groups and let overlay validate it.
    std::uint64_t dense_maximum = 0;
    std::uint64_t dense_spans = 0;
    bool dense = true;
    std::uint64_t increasing_previous = 0;
    std::uint64_t increasing_spans = 0;
    bool increasing = true;
    bool first = true;
    bool separated_collapsed = !geometry.curves.empty();
    std::uint64_t previous_carrier = 0;
    double previous_maximum_x = 0.0;
    for (std::size_t index = 0; index < geometry.curves.size(); ++index)
    {
        const AnalyticAtomicCurveNm& curve = geometry.curves[index];
        const AnalyticCurveBoundsNm& bounds = geometry.bounds[index];
        if (!valid_point(curve.start) || !valid_point(curve.end))
            return false;
        if (curve.kind != AnalyticAtomicCurveKind::line &&
            curve.kind != AnalyticAtomicCurveKind::circular_arc)
            return false;
        bool count_valid = true;
        counts.possible_base_spans =
            checked_add(counts.possible_base_spans,
                        curve.kind == AnalyticAtomicCurveKind::circular_arc ? 3 : 1, count_valid);
        counts.possible_repeated_base_spans =
            checked_add(counts.possible_repeated_base_spans,
                        curve.kind == AnalyticAtomicCurveKind::circular_arc ? 4 : 2, count_valid);
        counts.possible_base_memberships =
            checked_add(counts.possible_base_memberships,
                        curve.kind == AnalyticAtomicCurveKind::circular_arc ? 3 : 1, count_valid);
        counts.possible_collapsed_domains = checked_add(
            counts.possible_collapsed_domains,
            complete_points_within_resolution(curve.start, curve.end) ? 1 : 0, count_valid);
        counts.possible_circular_carrier_groups =
            checked_add(counts.possible_circular_carrier_groups,
                        curve.kind == AnalyticAtomicCurveKind::circular_arc ? 1 : 0, count_valid);
        if (!count_valid)
            return false;
        const std::uint64_t carrier = curve.construction_carrier_id;
        const bool guarantees_span = curve_guarantees_overlay_span(curve);
        if (dense)
        {
            if (carrier == dense_maximum + 1)
            {
                ++dense_maximum;
                dense_spans += guarantees_span ? 1 : 0;
            }
            else if (carrier == 0 || carrier > dense_maximum)
                dense = false;
        }
        if (increasing)
        {
            if (carrier != 0 && (first || carrier > increasing_previous))
            {
                increasing_previous = carrier;
                increasing_spans += guarantees_span ? 1 : 0;
            }
            else
                increasing = false;
        }

        if (separated_collapsed)
        {
            const bool valid_bounds = bounds.curve_index == curve.curve_index &&
                                      std::isfinite(bounds.min_x) && std::isfinite(bounds.min_y) &&
                                      std::isfinite(bounds.max_x) && std::isfinite(bounds.max_y) &&
                                      bounds.min_x <= bounds.max_x && bounds.min_y <= bounds.max_y;
            if (guarantees_span || carrier == 0 || carrier <= previous_carrier || !valid_bounds)
                separated_collapsed = false;
            else if (index != 0)
            {
                const double separated = std::nextafter(
                    previous_maximum_x + static_cast<double>(kAnalyticTopologyResolutionNm),
                    std::numeric_limits<double>::infinity());
                if (!(separated < bounds.min_x))
                    separated_collapsed = false;
            }
            if (separated_collapsed)
            {
                previous_carrier = carrier;
                previous_maximum_x = bounds.max_x;
            }
        }
        first = false;
    }
    counts.spans = std::max(dense_spans, increasing_spans);
    counts.collapsed_vertices = separated_collapsed ? geometry.curves.size() : 0;
    if (!increasing)
        counts.possible_base_spans = counts.possible_repeated_base_spans;
    return true;
}

bool calculate_arrangement_minimum_requirements(const AnalyticFilteredGeometry& geometry,
                                                std::uint64_t pair_count,
                                                const GuaranteedCarrierCounts& guaranteed,
                                                std::uint64_t& memory, std::uint64_t& work) noexcept
{
    bool valid = true;
    const std::uint64_t curve_count = geometry.curves.size();
    const std::uint64_t spans = guaranteed.spans;
    // A monotone sequence of short, distinct carriers whose conservative
    // bounds are separated by more than 50 nm must publish one isolated vertex
    // per carrier. Count that allocation-free case before running overlay.
    const std::uint64_t collapsed_domains = guaranteed.collapsed_vertices;
    const std::uint64_t endpoints =
        checked_add(checked_multiply(spans, 2, valid), collapsed_domains, valid);
    const std::uint64_t half_edges = checked_multiply(spans, 2, valid);
    memory = arrangement_memory_requirement(curve_count, spans, collapsed_domains, spans,
                                            curve_count, endpoints, half_edges, 0, valid);

    // Overlay consumes at least two units per candidate, visits every curve and
    // raw endpoint event, sorts the curve/event tables, and sweeps every
    // guaranteed carrier span twice. Arrangement then validates every curve,
    // span endpoint, and membership, visits/sorts every endpoint, builds every
    // guaranteed span, and republishes at least one lineage membership per
    // source curve.
    work = checked_multiply(pair_count, 2, valid);
    work = checked_add(work, checked_multiply(curve_count, 6, valid), valid);
    work = checked_add(work, sort_units(curve_count), valid);
    work = checked_add(work, sort_units(checked_multiply(curve_count, 2, valid)), valid);
    work = checked_add(work, checked_multiply(spans, 11, valid), valid);
    work = checked_add(work, checked_multiply(sort_units(endpoints), 2, valid), valid);
    work = checked_add(work, checked_multiply(sort_units(spans), 2, valid), valid);
    work = checked_add(work, checked_multiply(collapsed_domains, 52, valid), valid);
    work = checked_add(work, spans == 0 ? 0 : 1, valid);
    return valid;
}

AnalyticFilteredPointNm point_hull(const AnalyticFilteredPointNm& left,
                                   const AnalyticFilteredPointNm& right) noexcept
{
    const std::uint64_t column =
        left.construction_x_column_id == right.construction_x_column_id
            ? left.construction_x_column_id
        : left.construction_x_column_id == 0  ? right.construction_x_column_id
        : right.construction_x_column_id == 0 ? left.construction_x_column_id
                                              : 0;
    return {{std::min(left.x.lower, right.x.lower), std::max(left.x.upper, right.x.upper)},
            {std::min(left.y.lower, right.y.lower), std::max(left.y.upper, right.y.upper)},
            column};
}

double expanded_lower(double value) noexcept
{
    return std::nextafter(value - static_cast<double>(kAnalyticTopologyResolutionNm),
                          -std::numeric_limits<double>::infinity());
}

double expanded_upper(double value) noexcept
{
    return std::nextafter(value + static_cast<double>(kAnalyticTopologyResolutionNm),
                          std::numeric_limits<double>::infinity());
}

double approximate_angle_key(double x, double y) noexcept
{
    const double absolute_x = std::fabs(x);
    const double absolute_y = std::fabs(y);
    const double fraction =
        absolute_x + absolute_y == 0.0 ? 0.0 : absolute_y / (absolute_x + absolute_y);
    if (x >= 0.0 && y >= 0.0)
        return fraction;
    if (x < 0.0 && y >= 0.0)
        return 2.0 - fraction;
    if (x < 0.0)
        return 2.0 + fraction;
    return 4.0 - fraction;
}

std::optional<std::int8_t> sign(Interval value) noexcept
{
    if (value.lower > 0.0)
        return 1;
    if (value.upper < 0.0)
        return -1;
    if (value.lower == 0.0 && value.upper == 0.0)
        return 0;
    return std::nullopt;
}

Tangent outgoing_tangent(std::uint32_t half_edge_id,
                         const AnalyticFilteredArrangementResult& result) noexcept
{
    const AnalyticArrangementHalfEdge& half_edge = result.half_edges[half_edge_id];
    const AnalyticArrangementEdgeNm& edge = result.edges[half_edge.edge];
    const Point origin = point(half_edge.forward ? edge.carrier_start : edge.carrier_end);
    const Point target = point(half_edge.forward ? edge.carrier_end : edge.carrier_start);
    if (edge.kind == AnalyticAtomicCurveKind::line)
    {
        if (edge.has_construction_line_direction)
        {
            Point direction{exact(static_cast<double>(edge.construction_line_dx)),
                            exact(static_cast<double>(edge.construction_line_dy))};
            if (!half_edge.forward)
                direction = {negate(direction.x), negate(direction.y)};
            return {direction, 0, {0.0, 0.0}};
        }
        return {subtract(target, origin), 0, {0.0, 0.0}};
    }

    Point radial = subtract(origin, point(edge.circle.center));
    if (origin.y.lower == edge.circle.center.y.lower &&
        origin.y.upper == edge.circle.center.y.upper &&
        (origin.x.upper < edge.circle.center.x.lower ||
         origin.x.lower > edge.circle.center.x.upper))
        radial.y = exact(0.0);
    const bool counterclockwise =
        half_edge.forward ? edge.counterclockwise : !edge.counterclockwise;
    if (counterclockwise)
        return {
            {negate(radial.y), radial.x}, 1, {edge.circle.radius.lower, edge.circle.radius.upper}};
    return {{radial.y, negate(radial.x)}, -1, {edge.circle.radius.lower, edge.circle.radius.upper}};
}

std::optional<std::int8_t> direction_half(const Tangent& tangent) noexcept
{
    const std::optional<std::int8_t> y = sign(tangent.direction.y);
    if (!y)
        return std::nullopt;
    if (*y > 0)
        return 0;
    if (*y < 0)
        return 1;
    const std::optional<std::int8_t> x = sign(tangent.direction.x);
    if (!x || *x == 0)
        return std::nullopt;
    if (*x < 0)
        return 1;
    return tangent.curvature < 0 ? 2 : 0;
}

std::optional<std::int8_t> compare_collinear_tangents(const Tangent& left,
                                                      const Tangent& right) noexcept
{
    const Interval product = dot(left.direction, right.direction);
    if (product.lower <= 0.0)
        return std::nullopt;
    if (left.curvature != right.curvature)
        return left.curvature < right.curvature ? -1 : 1;
    if (left.curvature == 0)
        return 0;
    if (left.radius.upper < right.radius.lower)
        return left.curvature > 0 ? 1 : -1;
    if (right.radius.upper < left.radius.lower)
        return left.curvature > 0 ? -1 : 1;
    if (left.radius.lower == right.radius.lower && left.radius.upper == right.radius.upper)
        return 0;
    return std::nullopt;
}

std::optional<std::int8_t> compare_or_defer_collinear(const Tangent& left,
                                                      const Tangent& right) noexcept
{
    const Interval determinant = cross(left.direction, right.direction);
    if (determinant.lower > 0.0)
        return -1;
    if (determinant.upper < 0.0)
        return 1;
    if (determinant.lower != 0.0 || determinant.upper != 0.0)
        return std::nullopt;
    return compare_collinear_tangents(left, right);
}

std::optional<std::int8_t> compare_tangents(const Tangent& left, const Tangent& right) noexcept
{
    const std::optional<std::int8_t> left_half = direction_half(left);
    const std::optional<std::int8_t> right_half = direction_half(right);
    if (!left_half || !right_half)
        return std::nullopt;
    if (*left_half != *right_half)
        return *left_half < *right_half ? -1 : 1;
    return compare_or_defer_collinear(left, right);
}

std::optional<std::int8_t> cycle_germ_half(const Tangent& tangent) noexcept
{
    const std::optional<std::int8_t> x = sign(tangent.direction.x);
    const std::optional<std::int8_t> y = sign(tangent.direction.y);
    if (!x || !y || *x < 0)
        return std::nullopt;
    return *y < 0 ? 0 : 1;
}

std::optional<std::int8_t> compare_cycle_germs(const Tangent& outgoing,
                                               const Tangent& reverse_incoming) noexcept
{
    const std::optional<std::int8_t> outgoing_half = cycle_germ_half(outgoing);
    const std::optional<std::int8_t> incoming_half = cycle_germ_half(reverse_incoming);
    if (!outgoing_half || !incoming_half)
        return std::nullopt;
    if (*outgoing_half != *incoming_half)
        return *outgoing_half < *incoming_half ? -1 : 1;
    return compare_or_defer_collinear(outgoing, reverse_incoming);
}

std::tuple<double, std::int8_t, double, std::uint32_t, std::uint32_t>
outgoing_key(std::uint32_t half_edge_id, const AnalyticFilteredArrangementResult& result) noexcept
{
    const Tangent tangent = outgoing_tangent(half_edge_id, result);
    const double x =
        tangent.direction.x.lower + (tangent.direction.x.upper - tangent.direction.x.lower) * 0.5;
    const double y =
        tangent.direction.y.lower + (tangent.direction.y.upper - tangent.direction.y.lower) * 0.5;
    const double angle_key =
        y == 0.0 && x > 0.0 && tangent.curvature < 0 ? 4.0 : approximate_angle_key(x, y);
    double radius_key = tangent.radius.lower + (tangent.radius.upper - tangent.radius.lower) * 0.5;
    if (tangent.curvature > 0)
        radius_key = -radius_key;
    return {angle_key, tangent.curvature, radius_key, result.half_edges[half_edge_id].edge,
            half_edge_id};
}

class ArrangementBuilder
{
  public:
    ArrangementBuilder(const AnalyticFilteredGeometry& geometry,
                       const AnalyticFilteredOverlayResult& overlay, AnalyticSolverLimits limits,
                       std::uint64_t admission_work_units)
        : geometry_(geometry), overlay_(overlay), limits_(limits)
    {
        result_.telemetry.admission_work_units = admission_work_units;
        result_.telemetry.input_spans = overlay.spans.size();
        result_.telemetry.input_memberships = overlay.memberships.size();
        result_.telemetry.overlay_predicate_calls = overlay.telemetry.predicate_calls;
        result_.telemetry.overlay_peak_working_memory_bytes =
            overlay.telemetry.peak_working_memory_bytes;
        result_.telemetry.predicate_calls =
            admission_work_units + overlay.telemetry.predicate_calls;
        result_.telemetry.peak_working_memory_bytes = overlay.telemetry.peak_working_memory_bytes;
        result_.telemetry.algebraic_fallback_calls = overlay.telemetry.algebraic_fallback_calls;
    }

    AnalyticFilteredArrangementResult build()
    {
        try
        {
            if (!preflight() || !validate_inputs() || !build_endpoint_clusters() ||
                !build_edges() || !build_half_edges() || !build_cycles())
            {
                clear_output();
                return result_;
            }
        }
        catch (const std::bad_alloc&)
        {
            result_.error = AnalyticFilteredArrangementError::resource_limit_exceeded;
            clear_output();
        }
        return result_;
    }

  private:
    bool fail(AnalyticFilteredArrangementError error)
    {
        result_.error = error;
        return false;
    }

    void clear_output()
    {
        result_.vertices.clear();
        result_.edges.clear();
        result_.half_edges.clear();
        result_.outgoing_half_edges.clear();
        result_.collapsed_spans.clear();
        result_.memberships.clear();
        result_.cycles.clear();
        result_.cycle_half_edges.clear();
    }

    bool charge(std::uint64_t units)
    {
        if (result_.telemetry.predicate_calls > limits_.predicate_calls ||
            units > limits_.predicate_calls - result_.telemetry.predicate_calls)
            return fail(AnalyticFilteredArrangementError::resource_limit_exceeded);
        result_.telemetry.predicate_calls += units;
        return true;
    }

    bool charge_sort(std::uint64_t count)
    {
        const std::uint64_t units = sort_units(count);
        if (!charge(units))
            return false;
        result_.telemetry.sort_work_units += units;
        return true;
    }

    bool valid_preflight_shape() const noexcept
    {
        return analytic_solver_limits_within_hard_ceilings(limits_) &&
               overlay_.error == AnalyticFilteredOverlayError::none &&
               geometry_.curves.size() == geometry_.bounds.size() &&
               geometry_.curves.size() == geometry_.occurrences.size();
    }

    bool base_limits_exceeded() const noexcept
    {
        return geometry_.curves.size() > limits_.boundary_occurrences ||
               overlay_.spans.size() > limits_.arrangement_half_edges / 2 ||
               overlay_.memberships.size() > limits_.source_reference_memberships ||
               overlay_.memberships.size() > limits_.provenance_references ||
               overlay_.telemetry.predicate_calls > limits_.predicate_calls ||
               overlay_.telemetry.algebraic_fallback_calls > limits_.algebraic_fallback_calls;
    }

    bool derived_limits_exceeded(std::uint64_t collapsed_domains, std::uint64_t maximum_collapsed,
                                 std::uint64_t memberships) const noexcept
    {
        return collapsed_domains > geometry_.curves.size() ||
               maximum_collapsed > limits_.arrangement_half_edges / 2 ||
               memberships > limits_.source_reference_memberships ||
               memberships > limits_.provenance_references;
    }

    bool preflight()
    {
        if (!valid_preflight_shape())
            return fail(AnalyticFilteredArrangementError::invalid_argument);
        if (base_limits_exceeded())
            return fail(AnalyticFilteredArrangementError::resource_limit_exceeded);

        bool valid = true;
        const std::uint64_t spans = static_cast<std::uint64_t>(overlay_.spans.size());
        const std::uint64_t collapsed_domains = overlay_.telemetry.collapsed_domains;
        const std::uint64_t memberships = checked_add(
            static_cast<std::uint64_t>(overlay_.memberships.size()), collapsed_domains, valid);
        const std::uint64_t endpoints =
            checked_add(checked_multiply(spans, 2, valid), collapsed_domains, valid);
        const std::uint64_t half_edges = checked_multiply(spans, 2, valid);
        const std::uint64_t maximum_collapsed = checked_add(spans, collapsed_domains, valid);
        if (!valid || derived_limits_exceeded(collapsed_domains, maximum_collapsed, memberships))
            return fail(AnalyticFilteredArrangementError::resource_limit_exceeded);
        const std::uint64_t bytes = arrangement_memory_requirement(
            geometry_.curves.size(), spans, collapsed_domains, overlay_.memberships.size(),
            memberships, endpoints, half_edges, overlay_.telemetry.peak_working_memory_bytes,
            valid);
        if (!valid || bytes > limits_.working_memory_bytes)
            return fail(AnalyticFilteredArrangementError::resource_limit_exceeded);
        result_.telemetry.peak_working_memory_bytes = bytes;
        return true;
    }

    bool validate_geometry()
    {
        for (std::size_t index = 0; index < geometry_.curves.size(); ++index)
        {
            const AnalyticAtomicCurveNm& curve = geometry_.curves[index];
            const AnalyticFilteredOccurrence& occurrence = geometry_.occurrences[index];
            if (curve.curve_index != index + 1 ||
                geometry_.bounds[index].curve_index != index + 1 ||
                occurrence.occurrence_id != index + 1 || curve.construction_carrier_id == 0 ||
                !analytic_filtered_curve_is_valid(curve) ||
                (curve.kind == AnalyticAtomicCurveKind::circular_arc &&
                 occurrence.agrees_with_carrier != curve.counterclockwise))
                return fail(AnalyticFilteredArrangementError::invalid_argument);
        }
        return true;
    }

    bool validate_span_shape(const AnalyticAtomicSpanNm& span, std::size_t span_offset,
                             std::uint64_t membership_cursor)
    {
        if (span.span_index != span_offset + 1 || span.carrier_curve_index == 0 ||
            span.carrier_curve_index > geometry_.curves.size() || span.membership_count == 0 ||
            span.membership_begin != membership_cursor ||
            membership_cursor > overlay_.memberships.size() ||
            span.membership_count > overlay_.memberships.size() - membership_cursor ||
            !valid_point(span.start) || !valid_point(span.end))
            return fail(AnalyticFilteredArrangementError::invalid_argument);
        return true;
    }

    bool validate_span_carrier(const AnalyticAtomicSpanNm& span,
                               const AnalyticAtomicCurveNm& carrier)
    {
        const AnalyticFilteredPointCurveStatus start_status =
            classify_analytic_filtered_point_on_curve(carrier, span.start);
        const AnalyticFilteredPointCurveStatus end_status =
            classify_analytic_filtered_point_on_curve(carrier, span.end);
        if (start_status == AnalyticFilteredPointCurveStatus::uncertain ||
            end_status == AnalyticFilteredPointCurveStatus::uncertain)
            return fail(AnalyticFilteredArrangementError::resource_limit_exceeded);
        if (span.kind != carrier.kind || span.major_arc ||
            start_status != AnalyticFilteredPointCurveStatus::certified_on_domain ||
            end_status != AnalyticFilteredPointCurveStatus::certified_on_domain)
            return fail(AnalyticFilteredArrangementError::invalid_argument);
        return true;
    }

    bool validate_span_memberships(const AnalyticAtomicSpanNm& span,
                                   const AnalyticAtomicCurveNm& carrier)
    {
        std::uint32_t previous_curve = 0;
        for (std::uint32_t local = 0; local < span.membership_count; ++local)
        {
            const AnalyticSpanMembership& membership =
                overlay_.memberships[span.membership_begin + local];
            if (membership.curve_index == 0 || membership.curve_index > geometry_.curves.size() ||
                membership.curve_index <= previous_curve)
                return fail(AnalyticFilteredArrangementError::invalid_argument);
            previous_curve = membership.curve_index;
            const AnalyticAtomicCurveNm& curve = geometry_.curves[membership.curve_index - 1];
            const AnalyticFilteredOccurrence& occurrence =
                geometry_.occurrences[membership.curve_index - 1];
            if (curve.construction_carrier_id != carrier.construction_carrier_id ||
                membership.agrees_with_span != occurrence.agrees_with_carrier ||
                membership.material_on_span_left != (occurrence.agrees_with_carrier
                                                         ? occurrence.material_on_left
                                                         : !occurrence.material_on_left))
                return fail(AnalyticFilteredArrangementError::invalid_argument);
        }
        return true;
    }

    bool validate_inputs()
    {
        if (!charge(geometry_.curves.size() * 2 + overlay_.spans.size() * 2 +
                    overlay_.memberships.size()) ||
            !validate_geometry())
            return false;
        curve_referenced_.assign(geometry_.curves.size(), 0);
        collapsed_curve_indices_.reserve(
            static_cast<std::size_t>(overlay_.telemetry.collapsed_domains));
        std::uint64_t membership_cursor = 0;
        for (std::size_t span_offset = 0; span_offset < overlay_.spans.size(); ++span_offset)
        {
            const AnalyticAtomicSpanNm& span = overlay_.spans[span_offset];
            if (!validate_span_shape(span, span_offset, membership_cursor))
                return false;
            const AnalyticAtomicCurveNm& carrier = geometry_.curves[span.carrier_curve_index - 1];
            if (!validate_span_carrier(span, carrier) || !validate_span_memberships(span, carrier))
                return false;
            for (std::uint32_t local = 0; local < span.membership_count; ++local)
                curve_referenced_[overlay_.memberships[span.membership_begin + local].curve_index -
                                  1] = 1;
            membership_cursor += span.membership_count;
        }
        if (membership_cursor != overlay_.memberships.size())
            return fail(AnalyticFilteredArrangementError::invalid_argument);
        for (std::uint32_t curve_offset = 0; curve_offset < geometry_.curves.size(); ++curve_offset)
            if (curve_referenced_[curve_offset] == 0)
            {
                const AnalyticAtomicCurveNm& curve = geometry_.curves[curve_offset];
                const AnalyticFilteredPointNm representative = point_hull(curve.start, curve.end);
                if (!complete_points_within_resolution(curve.start, curve.end) ||
                    !valid_point(representative))
                    return fail(AnalyticFilteredArrangementError::invalid_argument);
                collapsed_curve_indices_.push_back(curve_offset);
            }
        if (collapsed_curve_indices_.size() > overlay_.telemetry.collapsed_domains)
            return fail(AnalyticFilteredArrangementError::invalid_argument);
        return true;
    }

    bool build_endpoint_clusters()
    {
        const std::size_t span_endpoint_count = overlay_.spans.size() * 2;
        const std::size_t endpoint_count = span_endpoint_count + collapsed_curve_indices_.size();
        result_.telemetry.endpoint_records = endpoint_count;
        if (!charge(endpoint_count) || !charge_sort(endpoint_count))
            return false;
        endpoints_.reserve(endpoint_count);
        endpoint_vertices_.resize(endpoint_count);
        for (std::uint32_t span = 0; span < overlay_.spans.size(); ++span)
        {
            endpoints_.push_back({overlay_.spans[span].start, span * 2, span, true});
            endpoints_.push_back({overlay_.spans[span].end, span * 2 + 1, span, false});
        }
        for (std::uint32_t local = 0; local < collapsed_curve_indices_.size(); ++local)
        {
            const std::uint32_t curve_offset = collapsed_curve_indices_[local];
            const AnalyticAtomicCurveNm& curve = geometry_.curves[curve_offset];
            endpoints_.push_back({point_hull(curve.start, curve.end),
                                  static_cast<std::uint32_t>(span_endpoint_count + local),
                                  static_cast<std::uint32_t>(overlay_.spans.size() + local),
                                  false});
        }
        std::sort(endpoints_.begin(), endpoints_.end(),
                  [](const EndpointRecord& left, const EndpointRecord& right)
                  {
                      return std::make_tuple(left.point.x.lower, left.point.x.upper,
                                             left.point.y.lower, left.point.y.upper,
                                             left.span_offset, left.start) <
                             std::make_tuple(right.point.x.lower, right.point.x.upper,
                                             right.point.y.lower, right.point.y.upper,
                                             right.span_offset, right.start);
                  });

        detail::AnalyticIntervalIndex index(endpoint_count);
        std::unique_ptr<ExpiryEntry[]> expiry =
            endpoint_count == 0 ? nullptr : std::make_unique<ExpiryEntry[]>(endpoint_count);
        std::size_t expiry_size = 0;
        clusters_.reserve(endpoint_count);
        for (const EndpointRecord& endpoint : endpoints_)
        {
            const double minimum_x = expanded_lower(endpoint.point.x.lower);
            while (expiry_size != 0 && expiry[0].maximum_x < minimum_x)
            {
                std::pop_heap(expiry.get(), expiry.get() + expiry_size, ExpiryLater{});
                const ExpiryEntry expired = expiry[--expiry_size];
                index.erase(expired.minimum_y, expired.cluster + 1);
            }

            std::uint32_t selected = kNoIndex;
            const std::uint64_t visits_before = result_.telemetry.predicate_calls;
            const bool completed = index.query(
                expanded_lower(endpoint.point.y.lower), expanded_upper(endpoint.point.y.upper),
                result_.telemetry.predicate_calls, limits_.predicate_calls,
                [&](std::size_t payload, std::uint32_t)
                {
                    const std::uint32_t cluster = static_cast<std::uint32_t>(payload);
                    if (complete_points_within_resolution(clusters_[cluster].hull, endpoint.point))
                        selected = std::min(selected, cluster);
                    return true;
                });
            result_.telemetry.endpoint_index_node_visits +=
                result_.telemetry.predicate_calls - visits_before;
            if (!completed)
                return fail(AnalyticFilteredArrangementError::resource_limit_exceeded);

            if (selected == kNoIndex)
            {
                if (clusters_.size() == std::numeric_limits<std::uint32_t>::max())
                    return fail(AnalyticFilteredArrangementError::resource_limit_exceeded);
                selected = static_cast<std::uint32_t>(clusters_.size());
                const std::uint64_t update_units = tree_update_units(endpoint_count);
                if (!charge(update_units))
                    return false;
                result_.telemetry.endpoint_index_update_work_units += update_units;
                clusters_.push_back({endpoint.point, endpoint.point});
                if (!index.insert(endpoint.point.y.lower, endpoint.point.y.upper, selected,
                                  selected + 1))
                    return fail(AnalyticFilteredArrangementError::resource_limit_exceeded);
                expiry[expiry_size++] = {expanded_upper(endpoint.point.x.upper),
                                         endpoint.point.y.lower, selected};
                std::push_heap(expiry.get(), expiry.get() + expiry_size, ExpiryLater{});
            }
            else
            {
                clusters_[selected].hull = point_hull(clusters_[selected].hull, endpoint.point);
                ++result_.telemetry.merged_endpoint_records;
            }
            endpoint_vertices_[endpoint.endpoint_slot] = selected;
        }
        std::vector<EndpointRecord>().swap(endpoints_);
        return true;
    }

    void collect_edge_drafts(std::vector<EdgeDraft>& drafts, std::vector<CollapsedDraft>& collapsed,
                             std::vector<bool>& used)
    {
        for (std::uint32_t span_offset = 0; span_offset < overlay_.spans.size(); ++span_offset)
        {
            const AnalyticAtomicSpanNm& span = overlay_.spans[span_offset];
            const std::uint32_t start = endpoint_vertices_[span_offset * 2];
            const std::uint32_t end = endpoint_vertices_[span_offset * 2 + 1];
            if (start == end)
            {
                used[start] = true;
                collapsed.push_back(
                    {{0, span.carrier_curve_index, 0, span.membership_count}, start, span_offset});
                ++result_.telemetry.collapsed_spans;
                continue;
            }
            used[start] = true;
            used[end] = true;
            const AnalyticAtomicCurveNm& carrier = geometry_.curves[span.carrier_curve_index - 1];
            AnalyticArrangementEdgeNm edge;
            edge.start_vertex = start;
            edge.end_vertex = end;
            edge.carrier_curve_index = span.carrier_curve_index;
            edge.kind = span.kind;
            edge.carrier_start = span.start;
            edge.carrier_end = span.end;
            edge.circle = carrier.circle;
            edge.counterclockwise = true;
            edge.major_arc = span.major_arc;
            edge.membership_count = span.membership_count;
            edge.x_monotone_branch = span.x_monotone_branch;
            edge.has_construction_line_direction = carrier.has_construction_line_direction;
            edge.construction_line_dx = carrier.construction_line_dx;
            edge.construction_line_dy = carrier.construction_line_dy;
            drafts.push_back({edge, span_offset});
        }
        const std::uint32_t collapsed_endpoint_begin =
            static_cast<std::uint32_t>(overlay_.spans.size() * 2);
        for (std::uint32_t local = 0; local < collapsed_curve_indices_.size(); ++local)
        {
            const std::uint32_t curve_offset = collapsed_curve_indices_[local];
            const std::uint32_t cluster = endpoint_vertices_[collapsed_endpoint_begin + local];
            used[cluster] = true;
            collapsed.push_back({{0, curve_offset + 1, 0, 1}, cluster, kNoIndex});
            ++result_.telemetry.collapsed_spans;
        }
    }

    bool publish_vertices(std::vector<EdgeDraft>& drafts, std::vector<CollapsedDraft>& collapsed,
                          const std::vector<bool>& used)
    {
        std::vector<std::uint32_t> compact(clusters_.size(), kNoIndex);
        result_.vertices.reserve(clusters_.size());
        for (std::uint32_t cluster = 0; cluster < clusters_.size(); ++cluster)
            if (used[cluster])
            {
                if (result_.vertices.size() == limits_.arrangement_vertices)
                    return fail(AnalyticFilteredArrangementError::resource_limit_exceeded);
                compact[cluster] = static_cast<std::uint32_t>(result_.vertices.size());
                result_.vertices.push_back({clusters_[cluster].hull, 0, 0});
            }
        for (EdgeDraft& draft : drafts)
        {
            draft.edge.start_vertex = compact[draft.edge.start_vertex];
            draft.edge.end_vertex = compact[draft.edge.end_vertex];
        }
        for (CollapsedDraft& draft : collapsed)
            draft.span.vertex = compact[draft.cluster];
        return true;
    }

    static void sort_drafts(std::vector<EdgeDraft>& drafts, std::vector<CollapsedDraft>& collapsed)
    {
        std::sort(drafts.begin(), drafts.end(),
                  [](const EdgeDraft& left, const EdgeDraft& right)
                  {
                      return std::make_tuple(std::min(left.edge.start_vertex, left.edge.end_vertex),
                                             std::max(left.edge.start_vertex, left.edge.end_vertex),
                                             left.edge.start_vertex, left.edge.end_vertex,
                                             left.edge.kind, left.edge.carrier_curve_index,
                                             left.span_offset) <
                             std::make_tuple(
                                 std::min(right.edge.start_vertex, right.edge.end_vertex),
                                 std::max(right.edge.start_vertex, right.edge.end_vertex),
                                 right.edge.start_vertex, right.edge.end_vertex, right.edge.kind,
                                 right.edge.carrier_curve_index, right.span_offset);
                  });
        std::sort(collapsed.begin(), collapsed.end(),
                  [](const CollapsedDraft& left, const CollapsedDraft& right)
                  {
                      return std::tie(left.span.vertex, left.span.carrier_curve_index,
                                      left.span_offset) < std::tie(right.span.vertex,
                                                                   right.span.carrier_curve_index,
                                                                   right.span_offset);
                  });
    }

    void append_span_memberships(const AnalyticAtomicSpanNm& span)
    {
        result_.memberships.insert(
            result_.memberships.end(), overlay_.memberships.begin() + span.membership_begin,
            overlay_.memberships.begin() + span.membership_begin + span.membership_count);
    }

    void append_collapsed_memberships(const CollapsedDraft& draft)
    {
        if (draft.span_offset != kNoIndex)
        {
            append_span_memberships(overlay_.spans[draft.span_offset]);
            return;
        }
        const std::uint32_t curve_offset = draft.span.carrier_curve_index - 1;
        const AnalyticFilteredOccurrence& occurrence = geometry_.occurrences[curve_offset];
        result_.memberships.push_back({curve_offset + 1, occurrence.agrees_with_carrier,
                                       occurrence.agrees_with_carrier
                                           ? occurrence.material_on_left
                                           : !occurrence.material_on_left});
    }

    bool publish_edges(std::vector<EdgeDraft>& drafts, std::vector<CollapsedDraft>& collapsed)
    {
        result_.edges.reserve(drafts.size());
        result_.collapsed_spans.reserve(collapsed.size());
        result_.memberships.reserve(overlay_.memberships.size() + collapsed_curve_indices_.size());
        if (!charge(overlay_.memberships.size() + collapsed_curve_indices_.size()))
            return false;
        for (EdgeDraft& draft : drafts)
        {
            const AnalyticAtomicSpanNm& span = overlay_.spans[draft.span_offset];
            draft.edge.membership_begin = static_cast<std::uint32_t>(result_.memberships.size());
            append_span_memberships(span);
            result_.edges.push_back(draft.edge);
        }
        for (CollapsedDraft& draft : collapsed)
        {
            draft.span.membership_begin = static_cast<std::uint32_t>(result_.memberships.size());
            append_collapsed_memberships(draft);
            result_.collapsed_spans.push_back(draft.span);
        }
        return true;
    }

    bool build_edges()
    {
        const std::uint64_t basic_work = static_cast<std::uint64_t>(overlay_.spans.size()) * 2 +
                                         collapsed_curve_indices_.size() + clusters_.size() +
                                         overlay_.memberships.size() +
                                         collapsed_curve_indices_.size();
        if (!charge(basic_work) || !charge_sort(overlay_.spans.size()) ||
            !charge_sort(overlay_.spans.size() + collapsed_curve_indices_.size()))
            return false;
        std::vector<EdgeDraft> drafts;
        std::vector<CollapsedDraft> collapsed;
        drafts.reserve(overlay_.spans.size());
        collapsed.reserve(overlay_.spans.size() + collapsed_curve_indices_.size());
        std::vector<bool> used(clusters_.size());
        collect_edge_drafts(drafts, collapsed, used);
        if (!publish_vertices(drafts, collapsed, used))
            return false;
        sort_drafts(drafts, collapsed);
        if (!publish_edges(drafts, collapsed))
            return false;
        result_.telemetry.emitted_vertices = result_.vertices.size();
        result_.telemetry.emitted_edges = result_.edges.size();
        std::vector<std::uint32_t>().swap(endpoint_vertices_);
        std::vector<VertexCluster>().swap(clusters_);
        std::vector<std::uint8_t>().swap(curve_referenced_);
        std::vector<std::uint32_t>().swap(collapsed_curve_indices_);
        return true;
    }

    void initialize_half_edges()
    {
        result_.half_edges.resize(result_.edges.size() * 2);
        result_.outgoing_half_edges.resize(result_.half_edges.size());
        for (std::uint32_t edge = 0; edge < result_.edges.size(); ++edge)
        {
            const std::uint32_t forward = edge * 2;
            const std::uint32_t reverse = forward + 1;
            result_.half_edges[forward] = {result_.edges[edge].start_vertex,
                                           reverse,
                                           kNoIndex,
                                           kNoIndex,
                                           edge,
                                           true,
                                           kNoIndex};
            result_.half_edges[reverse] = {
                result_.edges[edge].end_vertex, forward, kNoIndex, kNoIndex, edge, false, kNoIndex};
            result_.outgoing_half_edges[forward] = forward;
            result_.outgoing_half_edges[reverse] = reverse;
        }
    }

    bool certify_and_link_vertex(std::size_t begin, std::size_t end)
    {
        const std::uint32_t vertex =
            result_.half_edges[result_.outgoing_half_edges[begin]].origin_vertex;
        result_.vertices[vertex].outgoing_begin = static_cast<std::uint32_t>(begin);
        result_.vertices[vertex].outgoing_count = static_cast<std::uint32_t>(end - begin);
        for (std::size_t index = begin + 1; index < end; ++index)
        {
            if (!charge(1))
                return false;
            ++result_.telemetry.angular_predicates;
            const std::optional<std::int8_t> order =
                compare_tangents(outgoing_tangent(result_.outgoing_half_edges[index - 1], result_),
                                 outgoing_tangent(result_.outgoing_half_edges[index], result_));
            if (!order)
                return fail(AnalyticFilteredArrangementError::resource_limit_exceeded);
            if (*order >= 0)
                return fail(AnalyticFilteredArrangementError::invalid_argument);
        }
        std::uint32_t previous = result_.outgoing_half_edges[end - 1];
        for (std::size_t index = begin; index < end; ++index)
        {
            const std::uint32_t outgoing = result_.outgoing_half_edges[index];
            result_.half_edges[result_.half_edges[outgoing].twin].next = previous;
            previous = outgoing;
        }
        return true;
    }

    bool build_rotation_system()
    {
        std::size_t begin = 0;
        while (begin < result_.outgoing_half_edges.size())
        {
            const std::uint32_t vertex =
                result_.half_edges[result_.outgoing_half_edges[begin]].origin_vertex;
            std::size_t end = begin + 1;
            while (end < result_.outgoing_half_edges.size() &&
                   result_.half_edges[result_.outgoing_half_edges[end]].origin_vertex == vertex)
                ++end;
            if (!certify_and_link_vertex(begin, end))
                return false;
            begin = end;
        }
        return true;
    }

    bool fill_previous_links()
    {
        for (std::uint32_t half_edge = 0; half_edge < result_.half_edges.size(); ++half_edge)
        {
            const std::uint32_t next = result_.half_edges[half_edge].next;
            if (next == kNoIndex || next >= result_.half_edges.size() ||
                result_.half_edges[next].previous != kNoIndex)
                return fail(AnalyticFilteredArrangementError::invalid_argument);
            result_.half_edges[next].previous = half_edge;
        }
        return true;
    }

    bool build_half_edges()
    {
        if (result_.edges.size() > limits_.arrangement_half_edges / 2)
            return fail(AnalyticFilteredArrangementError::resource_limit_exceeded);
        const std::uint64_t half_edge_count = result_.edges.size() * 2;
        if (!charge(result_.edges.size() + half_edge_count * 3) || !charge_sort(half_edge_count))
            return false;
        initialize_half_edges();
        std::sort(result_.outgoing_half_edges.begin(), result_.outgoing_half_edges.end(),
                  [&](std::uint32_t left, std::uint32_t right)
                  {
                      const std::uint32_t left_origin = result_.half_edges[left].origin_vertex;
                      const std::uint32_t right_origin = result_.half_edges[right].origin_vertex;
                      return left_origin != right_origin
                                 ? left_origin < right_origin
                                 : outgoing_key(left, result_) < outgoing_key(right, result_);
                  });

        if (!build_rotation_system() || !fill_previous_links())
            return false;
        result_.telemetry.emitted_half_edges = result_.half_edges.size();
        return true;
    }

    void build_components(DisjointSet& components, std::vector<std::uint32_t>& component_by_root)
    {
        for (const AnalyticArrangementEdgeNm& edge : result_.edges)
            components.unite(edge.start_vertex, edge.end_vertex);
        std::uint32_t component_count = 0;
        for (std::uint32_t vertex = 0; vertex < result_.vertices.size(); ++vertex)
        {
            const std::uint32_t root = components.find(vertex);
            if (component_by_root[root] == kNoIndex)
                component_by_root[root] = component_count++;
        }
    }

    bool trace_cycle(std::uint32_t start, std::vector<bool>& visited, std::size_t cycle_begin)
    {
        std::uint32_t current = start;
        while (!visited[current])
        {
            visited[current] = true;
            result_.cycle_half_edges.push_back(current);
            current = result_.half_edges[current].next;
            if (current >= result_.half_edges.size() ||
                result_.cycle_half_edges.size() - cycle_begin > result_.half_edges.size())
                return fail(AnalyticFilteredArrangementError::invalid_argument);
        }
        if (current != start || result_.cycle_half_edges.size() == cycle_begin)
            return fail(AnalyticFilteredArrangementError::invalid_argument);
        return true;
    }

    bool publish_cycle(std::size_t cycle_begin, DisjointSet& components,
                       const std::vector<std::uint32_t>& component_by_root)
    {
        auto first = result_.cycle_half_edges.begin() + cycle_begin;
        auto last = result_.cycle_half_edges.end();
        auto canonical = std::min_element(
            first, last,
            [&](std::uint32_t left, std::uint32_t right)
            {
                const std::uint32_t left_vertex = result_.half_edges[left].origin_vertex;
                const std::uint32_t right_vertex = result_.half_edges[right].origin_vertex;
                return left_vertex != right_vertex ? left_vertex < right_vertex : left < right;
            });
        std::rotate(first, canonical, last);
        const std::uint32_t outgoing = *first;
        const std::uint32_t reverse_incoming =
            result_.half_edges[result_.half_edges[outgoing].previous].twin;
        if (!charge(1))
            return false;
        ++result_.telemetry.angular_predicates;
        const std::optional<std::int8_t> orientation = compare_cycle_germs(
            outgoing_tangent(outgoing, result_), outgoing_tangent(reverse_incoming, result_));
        if (!orientation)
            return fail(AnalyticFilteredArrangementError::resource_limit_exceeded);
        if (*orientation == 0)
            return fail(AnalyticFilteredArrangementError::invalid_argument);
        if (result_.cycles.size() == std::numeric_limits<std::uint32_t>::max())
            return fail(AnalyticFilteredArrangementError::resource_limit_exceeded);

        const std::uint32_t cycle = static_cast<std::uint32_t>(result_.cycles.size());
        const std::uint32_t count =
            static_cast<std::uint32_t>(result_.cycle_half_edges.size() - cycle_begin);
        const std::uint32_t root = components.find(result_.half_edges[outgoing].origin_vertex);
        result_.cycles.push_back({static_cast<std::uint32_t>(cycle_begin), count,
                                  component_by_root[root], *orientation < 0});
        for (std::size_t index = cycle_begin; index < result_.cycle_half_edges.size(); ++index)
            result_.half_edges[result_.cycle_half_edges[index]].cycle = cycle;
        return true;
    }

    bool build_cycles()
    {
        const std::uint64_t basic_work =
            result_.vertices.size() + result_.edges.size() + result_.half_edges.size() * 5;
        if (!charge(basic_work))
            return false;
        DisjointSet components(result_.vertices.size());
        std::vector<std::uint32_t> component_by_root(result_.vertices.size(), kNoIndex);
        build_components(components, component_by_root);

        std::vector<bool> visited(result_.half_edges.size());
        result_.cycles.reserve(result_.half_edges.size());
        result_.cycle_half_edges.reserve(result_.half_edges.size());
        for (std::uint32_t start = 0; start < result_.half_edges.size(); ++start)
        {
            if (visited[start])
                continue;
            const std::size_t cycle_begin = result_.cycle_half_edges.size();
            if (!trace_cycle(start, visited, cycle_begin) ||
                !publish_cycle(cycle_begin, components, component_by_root))
                return false;
        }
        result_.telemetry.emitted_cycles = result_.cycles.size();
        return true;
    }

    const AnalyticFilteredGeometry& geometry_;
    const AnalyticFilteredOverlayResult& overlay_;
    AnalyticSolverLimits limits_;
    AnalyticFilteredArrangementResult result_;
    std::vector<EndpointRecord> endpoints_;
    std::vector<std::uint32_t> endpoint_vertices_;
    std::vector<VertexCluster> clusters_;
    std::vector<std::uint8_t> curve_referenced_;
    std::vector<std::uint32_t> collapsed_curve_indices_;
};

static_assert(sizeof(EndpointRecord) <= kEndpointLogicalBytes);
static_assert(sizeof(VertexCluster) <= kClusterLogicalBytes);
static_assert(sizeof(ExpiryEntry) <= kExpiryLogicalBytes);
static_assert(sizeof(EdgeDraft) <= kEdgeDraftLogicalBytes);
static_assert(sizeof(CollapsedDraft) <= kCollapsedSpanLogicalBytes);
static_assert(sizeof(AnalyticArrangementVertexNm) <= kVertexLogicalBytes);
static_assert(sizeof(AnalyticArrangementEdgeNm) <= kEdgeLogicalBytes);
static_assert(sizeof(AnalyticArrangementHalfEdge) <= kHalfEdgeLogicalBytes);
static_assert(sizeof(AnalyticArrangementCollapsedSpan) <= kCollapsedSpanLogicalBytes);
static_assert(sizeof(AnalyticArrangementCycle) <= kCycleLogicalBytes);

} // namespace

bool estimate_analytic_filtered_arrangement_minimum_requirements(
    const AnalyticFilteredGeometry& geometry, std::uint64_t pair_count,
    AnalyticFilteredArrangementMinimumRequirements& requirements) noexcept
{
    requirements = {};
    GuaranteedCarrierCounts guaranteed;
    if (!guaranteed_carrier_counts(geometry, guaranteed))
        return false;
    requirements.guaranteed_spans = guaranteed.spans;
    requirements.guaranteed_collapsed_vertices = guaranteed.collapsed_vertices;
    requirements.possible_base_spans = guaranteed.possible_base_spans;
    requirements.possible_base_memberships = guaranteed.possible_base_memberships;
    requirements.possible_collapsed_domains = guaranteed.possible_collapsed_domains;
    requirements.possible_circular_carrier_groups = guaranteed.possible_circular_carrier_groups;
    return calculate_arrangement_minimum_requirements(geometry, pair_count, guaranteed,
                                                      requirements.working_memory_bytes,
                                                      requirements.predicate_calls);
}

bool analytic_detail::estimate_analytic_filtered_arrangement_possible_memory(
    const AnalyticFilteredArrangementCapacityEnvelope& envelope,
    std::uint64_t& working_memory_bytes) noexcept
{
    bool valid = true;
    const std::uint64_t narrow_peak =
        checked_multiply(envelope.pair_count, kAnalyticNarrowPhasePairLogicalBytes, valid);
    std::uint64_t raw_events = checked_multiply(envelope.curve_count, 2, valid);
    raw_events =
        checked_add(raw_events, checked_multiply(envelope.point_intersections, 2, valid), valid);
    raw_events = checked_add(raw_events,
                             checked_multiply(envelope.circular_carrier_groups, 2, valid), valid);
    std::uint64_t overlay_peak = checked_add(
        narrow_peak,
        checked_multiply(envelope.curve_count, kAnalyticOverlayCurveGroupLogicalBytes, valid),
        valid);
    overlay_peak = checked_add(
        overlay_peak, checked_multiply(raw_events, kOverlayRawEventLogicalBytes, valid), valid);
    overlay_peak = checked_add(
        overlay_peak, checked_multiply(raw_events, kOverlayUniqueEventLogicalBytes, valid), valid);
    overlay_peak = checked_add(overlay_peak,
                               checked_multiply(checked_multiply(envelope.curve_count, 2, valid),
                                                kOverlayActionLogicalBytes, valid),
                               valid);
    overlay_peak = checked_add(
        overlay_peak, checked_multiply(envelope.spans, kAnalyticOverlaySpanLogicalBytes, valid),
        valid);
    overlay_peak = checked_add(
        overlay_peak,
        checked_multiply(envelope.memberships, kAnalyticOverlayMembershipLogicalBytes, valid),
        valid);

    const std::uint64_t result_memberships =
        checked_add(envelope.memberships, envelope.collapsed_domains, valid);
    const std::uint64_t endpoints =
        checked_add(checked_multiply(envelope.spans, 2, valid), envelope.collapsed_domains, valid);
    const std::uint64_t half_edges = checked_multiply(envelope.spans, 2, valid);
    working_memory_bytes = arrangement_memory_requirement(
        envelope.curve_count, envelope.spans, envelope.collapsed_domains, envelope.memberships,
        result_memberships, endpoints, half_edges, overlay_peak, valid);
    return valid;
}

AnalyticFilteredArrangementResult
build_analytic_filtered_arrangement(const AnalyticFilteredGeometry& geometry,
                                    const std::vector<AnalyticCurvePair>& candidate_pairs,
                                    const AnalyticSolverLimits& limits)
{
    AnalyticFilteredArrangementResult preflight;
    if (!analytic_solver_limits_within_hard_ceilings(limits) ||
        geometry.curves.size() != geometry.bounds.size() ||
        geometry.curves.size() != geometry.occurrences.size())
    {
        preflight.error = AnalyticFilteredArrangementError::invalid_argument;
        return preflight;
    }
    if (geometry.curves.size() > limits.boundary_occurrences ||
        candidate_pairs.size() > limits.examined_curve_pairs)
    {
        preflight.error = AnalyticFilteredArrangementError::resource_limit_exceeded;
        return preflight;
    }
    const std::uint64_t admission_work = geometry.curves.size();
    if (admission_work > limits.predicate_calls)
    {
        preflight.error = AnalyticFilteredArrangementError::resource_limit_exceeded;
        return preflight;
    }
    preflight.telemetry.admission_work_units = admission_work;
    preflight.telemetry.predicate_calls = admission_work;
    GuaranteedCarrierCounts guaranteed;
    if (!guaranteed_carrier_counts(geometry, guaranteed))
    {
        preflight.error = AnalyticFilteredArrangementError::invalid_argument;
        return preflight;
    }
    std::uint64_t minimum_memory = 0;
    std::uint64_t minimum_work = 0;
    bool valid = calculate_arrangement_minimum_requirements(
        geometry, static_cast<std::uint64_t>(candidate_pairs.size()), guaranteed, minimum_memory,
        minimum_work);
    const std::uint64_t curve_count = geometry.curves.size();
    const std::uint64_t pair_count = candidate_pairs.size();
    std::uint64_t overlay_memory =
        checked_multiply(pair_count, kAnalyticNarrowPhasePairLogicalBytes, valid);
    overlay_memory = checked_add(
        overlay_memory,
        checked_multiply(curve_count, kAnalyticOverlayCurveGroupLogicalBytes, valid), valid);
    minimum_memory = std::max(minimum_memory, overlay_memory);
    const std::uint64_t remaining_work = limits.predicate_calls - admission_work;
    if (!valid || minimum_memory > limits.working_memory_bytes || minimum_work > remaining_work)
    {
        preflight.error = AnalyticFilteredArrangementError::resource_limit_exceeded;
        return preflight;
    }

    AnalyticSolverLimits overlay_limits = limits;
    overlay_limits.predicate_calls = remaining_work;
    const AnalyticFilteredOverlayResult overlay =
        build_analytic_filtered_overlay(geometry, candidate_pairs, overlay_limits);
    if (overlay.error != AnalyticFilteredOverlayError::none)
    {
        AnalyticFilteredArrangementResult result;
        result.telemetry.admission_work_units = admission_work;
        result.error = overlay.error == AnalyticFilteredOverlayError::invalid_argument
                           ? AnalyticFilteredArrangementError::invalid_argument
                           : AnalyticFilteredArrangementError::resource_limit_exceeded;
        result.telemetry.overlay_predicate_calls = overlay.telemetry.predicate_calls;
        result.telemetry.overlay_peak_working_memory_bytes =
            overlay.telemetry.peak_working_memory_bytes;
        result.telemetry.predicate_calls = admission_work + overlay.telemetry.predicate_calls;
        result.telemetry.peak_working_memory_bytes = overlay.telemetry.peak_working_memory_bytes;
        result.telemetry.algebraic_fallback_calls = overlay.telemetry.algebraic_fallback_calls;
        return result;
    }
    return ArrangementBuilder(geometry, overlay, limits, admission_work).build();
}

} // namespace geometer

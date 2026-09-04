#include "geometer/analytic_filtered_arrangement.h"

#include "analytic_filtered_arrangement_cycle_orientation.h"
#include "analytic_filtered_arrangement_tangent_identity.h"
#include "analytic_filtered_capacity.h"
#include "analytic_filtered_execution_policy.h"
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
using analytic_arrangement_detail::compare_canonical_tangent_class;
using analytic_arrangement_detail::merge_certified_cycle_orientation;
using analytic_arrangement_detail::shares_canonical_tangent_class;
using analytic_arrangement_detail::shares_exact_tangent_contact;
using analytic_arrangement_detail::tangent_token_names_endpoint;
using analytic_arrangement_detail::TangentEndpointIdentity;
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
constexpr std::uint64_t kClusterLogicalBytes = 96;
constexpr std::uint64_t kExpiryLogicalBytes = 24;
constexpr std::uint64_t kVertexLogicalBytes = kAnalyticArrangementVertexLogicalBytes;
constexpr std::uint64_t kEdgeDraftLogicalBytes = 216;
constexpr std::uint64_t kEdgeLogicalBytes = kAnalyticArrangementEdgeLogicalBytes;
constexpr std::uint64_t kHalfEdgeLogicalBytes = kAnalyticArrangementHalfEdgeLogicalBytes;
constexpr std::uint64_t kCollapsedSpanLogicalBytes = kAnalyticArrangementCollapsedSpanLogicalBytes;
constexpr std::uint64_t kIndexLogicalBytes = 8;
constexpr std::uint64_t kCurveFlagLogicalBytes = 1;
constexpr std::uint64_t kCycleLogicalBytes = kAnalyticArrangementCycleLogicalBytes;
constexpr std::uint64_t kTangentWitnessLogicalBytes = 16;
constexpr std::uint64_t kTangentAngleLogicalBytes = 8;
constexpr std::uint64_t kTangentIdentityLogicalBytes = 8;
constexpr std::uint64_t kTangentClassIndexLogicalBytes = 8;
constexpr std::uint64_t kTangentProjectionFlagLogicalBytes = 1;

struct EndpointRecord
{
    AnalyticFilteredPointNm point;
    std::uint32_t endpoint_slot = 0;
    std::uint32_t span_offset = 0;
    std::uint32_t endpoint_authoritative_curve = 0;
    bool start = true;
    bool singleton_integer_construction_endpoint = false;
};

struct VertexCluster
{
    AnalyticFilteredPointNm hull;
    AnalyticFilteredPointNm singleton_integer_point;
    std::uint32_t endpoint_authoritative_curve = 0;
    bool singleton_integer_construction_endpoint = false;
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

struct EdgeTangentWitness
{
    std::uint64_t start = 0;
    std::uint64_t end = 0;
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
    edge_phase =
        checked_add(edge_phase, checked_multiply(spans, kTangentWitnessLogicalBytes, valid), valid);
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
    cycle_phase = checked_add(cycle_phase,
                              checked_multiply(spans, kTangentWitnessLogicalBytes, valid), valid);
    cycle_phase = checked_add(
        cycle_phase, checked_multiply(half_edges, kTangentAngleLogicalBytes, valid), valid);
    cycle_phase = checked_add(
        cycle_phase, checked_multiply(half_edges, kTangentIdentityLogicalBytes, valid), valid);
    cycle_phase = checked_add(
        cycle_phase, checked_multiply(half_edges, kTangentClassIndexLogicalBytes, valid), valid);
    cycle_phase =
        checked_add(cycle_phase,
                    checked_multiply(half_edges, kTangentProjectionFlagLogicalBytes, valid), valid);
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

bool same_point_enclosure(const AnalyticFilteredPointNm& left,
                          const AnalyticFilteredPointNm& right) noexcept
{
    return left.x.lower == right.x.lower && left.x.upper == right.x.upper &&
           left.y.lower == right.y.lower && left.y.upper == right.y.upper;
}

bool same_singleton_point(const AnalyticFilteredPointNm& left,
                          const AnalyticFilteredPointNm& right) noexcept
{
    return left.x.lower == left.x.upper && left.y.lower == left.y.upper &&
           right.x.lower == right.x.upper && right.y.lower == right.y.upper &&
           left.x.lower == right.x.lower && left.y.lower == right.y.lower;
}

bool endpoint_can_join_cluster(const VertexCluster& cluster, const EndpointRecord& endpoint,
                               analytic_execution_detail::TopologyPolicy policy) noexcept
{
    if (!analytic_execution_detail::allows_resolution_topology(policy))
    {
        if (same_singleton_point(cluster.hull, endpoint.point))
            return true;
        if (cluster.hull.construction_x_column_id != 0 &&
            cluster.hull.construction_x_column_id == endpoint.point.construction_x_column_id &&
            same_point_enclosure(cluster.hull, endpoint.point))
            return true;
        return cluster.endpoint_authoritative_curve != 0 &&
               cluster.endpoint_authoritative_curve == endpoint.endpoint_authoritative_curve &&
               same_point_enclosure(cluster.hull, endpoint.point);
    }
    if (analytic_execution_detail::preserves_integer_construction_endpoints(policy) &&
        cluster.singleton_integer_construction_endpoint &&
        endpoint.singleton_integer_construction_endpoint)
        return same_singleton_point(cluster.singleton_integer_point, endpoint.point);
    if (cluster.endpoint_authoritative_curve == 0 && endpoint.endpoint_authoritative_curve == 0)
        return complete_points_within_resolution(cluster.hull, endpoint.point);
    if (cluster.endpoint_authoritative_curve != 0 &&
        cluster.endpoint_authoritative_curve == endpoint.endpoint_authoritative_curve &&
        same_point_enclosure(cluster.hull, endpoint.point))
        return true;
    return same_singleton_point(cluster.hull, endpoint.point);
}

bool proven_singleton_integer_endpoint(const AnalyticFilteredPointNm& point,
                                       const AnalyticAtomicCurveNm& curve) noexcept
{
    if (!curve.has_integer_certificate)
        return false;
    const auto matches = [&](const AnalyticIntegerPointNm& integer)
    {
        const double x = static_cast<double>(integer.x);
        const double y = static_cast<double>(integer.y);
        return point.x.lower == x && point.x.upper == x && point.y.lower == y && point.y.upper == y;
    };
    return matches(curve.integer_start) || matches(curve.integer_end);
}

bool proven_singleton_integer_intersection(const AnalyticFilteredPointNm& point,
                                           const AnalyticAtomicCurveNm& curve) noexcept
{
    const std::uint64_t token = point.construction_x_column_id;
    if (!analytic_integer_line_intersection_contains_carrier(token,
                                                             curve.construction_carrier_id) ||
        point.x.lower != point.x.upper || point.y.lower != point.y.upper)
        return false;
    return std::nearbyint(point.x.lower) == point.x.lower &&
           std::nearbyint(point.y.lower) == point.y.lower;
}

bool curve_guarantees_overlay_span(const AnalyticAtomicCurveNm& curve,
                                   analytic_execution_detail::TopologyPolicy policy) noexcept
{
    if (!valid_point(curve.start) || !valid_point(curve.end))
        return false;
    return (curve.kind == AnalyticAtomicCurveKind::circular_arc && curve.major_arc) ||
           (!analytic_execution_detail::allows_resolution_topology(policy)
                ? !same_singleton_point(curve.start, curve.end)
                : !complete_points_within_resolution(curve.start, curve.end));
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
                               GuaranteedCarrierCounts& counts,
                               analytic_execution_detail::TopologyPolicy policy) noexcept
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
        counts.possible_collapsed_domains =
            checked_add(counts.possible_collapsed_domains,
                        (analytic_execution_detail::allows_resolution_topology(policy)
                             ? complete_points_within_resolution(curve.start, curve.end)
                             : same_singleton_point(curve.start, curve.end))
                            ? 1
                            : 0,
                        count_valid);
        counts.possible_circular_carrier_groups =
            checked_add(counts.possible_circular_carrier_groups,
                        curve.kind == AnalyticAtomicCurveKind::circular_arc ? 1 : 0, count_valid);
        if (!count_valid)
            return false;
        const std::uint64_t carrier = curve.construction_carrier_id;
        const bool guarantees_span = curve_guarantees_overlay_span(curve, policy);
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
                const double expansion =
                    analytic_execution_detail::allows_resolution_topology(policy)
                        ? static_cast<double>(kAnalyticTopologyResolutionNm)
                        : 0.0;
                const double separated = std::nextafter(previous_maximum_x + expansion,
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
    work = checked_add(work, checked_multiply(curve_count, 11, valid), valid);
    work = checked_add(work, sort_units(curve_count), valid);
    work = checked_add(work, sort_units(checked_multiply(curve_count, 2, valid)), valid);
    work = checked_add(work, checked_multiply(spans, 14, valid), valid);
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

double expanded_lower(double value, analytic_execution_detail::TopologyPolicy policy) noexcept
{
    const double expansion = analytic_execution_detail::allows_resolution_topology(policy)
                                 ? static_cast<double>(kAnalyticTopologyResolutionNm)
                                 : 0.0;
    return std::nextafter(value - expansion, -std::numeric_limits<double>::infinity());
}

double expanded_upper(double value, analytic_execution_detail::TopologyPolicy policy) noexcept
{
    const double expansion = analytic_execution_detail::allows_resolution_topology(policy)
                                 ? static_cast<double>(kAnalyticTopologyResolutionNm)
                                 : 0.0;
    return std::nextafter(value + expansion, std::numeric_limits<double>::infinity());
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

TangentEndpointIdentity
outgoing_tangent_identity(std::uint32_t half_edge_id,
                          const AnalyticFilteredArrangementResult& result) noexcept
{
    const AnalyticArrangementHalfEdge& half_edge = result.half_edges[half_edge_id];
    const AnalyticArrangementEdgeNm& edge = result.edges[half_edge.edge];
    return {edge.kind, edge.construction_carrier_id,
            half_edge.forward ? edge.construction_start_tangent_id
                              : edge.construction_end_tangent_id,
            half_edge.forward ? edge.carrier_start : edge.carrier_end};
}

std::optional<std::int8_t>
endpoint_authoritative_cycle_half(std::uint32_t half_edge_id,
                                  const AnalyticFilteredArrangementResult& result) noexcept
{
    const AnalyticArrangementHalfEdge& half_edge = result.half_edges[half_edge_id];
    const AnalyticArrangementEdgeNm& edge = result.edges[half_edge.edge];
    if (!edge.endpoint_authoritative_arc || edge.x_monotone_branch == AnalyticXMonotoneBranch::none)
        return std::nullopt;
    const AnalyticFilteredPointNm& origin =
        half_edge.forward ? edge.carrier_start : edge.carrier_end;
    const AnalyticFilteredPointNm& target =
        half_edge.forward ? edge.carrier_end : edge.carrier_start;
    if (target.x.lower <= origin.x.upper)
        return std::nullopt;
    return edge.x_monotone_branch == AnalyticXMonotoneBranch::lower ? 0 : 1;
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

bool resolution_collinear_tangents(const Tangent& left, const Tangent& right) noexcept
{
    const Interval product = dot(left.direction, right.direction);
    if (product.lower <= 0.0)
        return false;
    const Interval determinant = cross(left.direction, right.direction);
    if (determinant.lower > 0.0 || determinant.upper < 0.0)
        return false;
    const Interval left_length_squared = dot(left.direction, left.direction);
    const Interval right_length_squared = dot(right.direction, right.direction);
    if (left_length_squared.lower <= 0.0 || right_length_squared.lower <= 0.0)
        return false;
    constexpr double kMaximumLocalSpanNm = 1'000'000'000'000.0;
    const double normalized_limit =
        static_cast<double>(kAnalyticTopologyResolutionNm) / kMaximumLocalSpanNm;
    const double determinant_limit = normalized_limit * std::sqrt(left_length_squared.lower) *
                                     std::sqrt(right_length_squared.lower);
    return std::max(std::fabs(determinant.lower), std::fabs(determinant.upper)) <=
           determinant_limit;
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

#include "analytic_filtered_arrangement_builder.h"
} // namespace

bool analytic_execution_detail::estimate_arrangement_minimum_requirements(
    const AnalyticFilteredGeometry& geometry, std::uint64_t pair_count,
    AnalyticFilteredArrangementMinimumRequirements& requirements, TopologyPolicy policy) noexcept
{
    requirements = {};
    GuaranteedCarrierCounts guaranteed;
    if (!guaranteed_carrier_counts(geometry, guaranteed, policy))
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

bool estimate_analytic_filtered_arrangement_minimum_requirements(
    const AnalyticFilteredGeometry& geometry, std::uint64_t pair_count,
    AnalyticFilteredArrangementMinimumRequirements& requirements) noexcept
{
    return analytic_execution_detail::estimate_arrangement_minimum_requirements(
        geometry, pair_count, requirements, analytic_execution_detail::kDefaultTopologyPolicy);
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

AnalyticFilteredArrangementResult analytic_execution_detail::build_arrangement(
    const AnalyticFilteredGeometry& geometry, const std::vector<AnalyticCurvePair>& candidate_pairs,
    const AnalyticSolverLimits& limits, TopologyPolicy policy)
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
    if (!guaranteed_carrier_counts(geometry, guaranteed, policy))
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
        if (valid && minimum_memory > limits.working_memory_bytes)
            preflight.telemetry.required_working_memory_bytes = minimum_memory;
        preflight.error = AnalyticFilteredArrangementError::resource_limit_exceeded;
        return preflight;
    }

    AnalyticSolverLimits overlay_limits = limits;
    overlay_limits.predicate_calls = remaining_work;
    const AnalyticFilteredOverlayResult overlay =
        build_overlay(geometry, candidate_pairs, overlay_limits, policy);
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
        result.telemetry.unresolved_predicate_failure =
            overlay.telemetry.unresolved_predicate_failure;
        result.telemetry.predicate_calls = admission_work + overlay.telemetry.predicate_calls;
        result.telemetry.peak_working_memory_bytes = overlay.telemetry.peak_working_memory_bytes;
        result.telemetry.required_working_memory_bytes =
            overlay.telemetry.required_working_memory_bytes;
        result.telemetry.algebraic_fallback_calls = overlay.telemetry.algebraic_fallback_calls;
        return result;
    }
    return ArrangementBuilder(geometry, overlay, limits, admission_work, policy).build();
}

AnalyticFilteredArrangementResult
build_analytic_filtered_arrangement(const AnalyticFilteredGeometry& geometry,
                                    const std::vector<AnalyticCurvePair>& candidate_pairs,
                                    const AnalyticSolverLimits& limits)
{
    return analytic_execution_detail::build_arrangement(
        geometry, candidate_pairs, limits, analytic_execution_detail::kDefaultTopologyPolicy);
}

} // namespace geometer

#include "analytic_filtered_normalization_replay.h"

#include "analytic_filtered_interval.h"

#include "geometer/analytic_curve_broad_phase.h"
#include "geometer/analytic_curve_narrow_phase.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <tuple>
#include <utility>
#include <vector>

namespace geometer::analytic_normalization_detail
{
namespace
{
using analytic_detail::cross;
using analytic_detail::exact;
using analytic_detail::negate;
using analytic_detail::Point;
using analytic_detail::subtract;

constexpr std::uint64_t kIndexLogicalBytes = 8;
constexpr std::uint64_t kReplayGeometryLogicalBytesPerCurve = 384;
constexpr std::uint64_t kReplayRingScratchLogicalBytes = 16;
constexpr std::uint64_t kReplayRegionScratchLogicalBytes = 16;
constexpr std::uint64_t kReplayFixedLogicalBytes = 512;
constexpr std::uint64_t kIntersectionValidationWork = 16;
constexpr std::uint32_t kNoIndex = std::numeric_limits<std::uint32_t>::max();

Point point(const AnalyticFilteredPointNm& value) noexcept
{
    return {{value.x.lower, value.x.upper}, {value.y.lower, value.y.upper}};
}

Point exact_point(std::int64_t x, std::int64_t y) noexcept
{
    return {exact(static_cast<double>(x)), exact(static_cast<double>(y))};
}

bool checked_add(std::uint64_t left, std::uint64_t right, std::uint64_t& output) noexcept
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
        return false;
    output = left + right;
    return true;
}

bool checked_multiply(std::uint64_t left, std::uint64_t right, std::uint64_t& output) noexcept
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left)
        return false;
    output = left * right;
    return true;
}

std::uint64_t sort_units(std::uint64_t count) noexcept
{
    if (count < 2)
        return 0;
    std::uint64_t levels = 0;
    for (std::uint64_t value = count - 1; value != 0; value >>= 1U)
        ++levels;
    std::uint64_t output = 0;
    return checked_multiply(count, levels, output) ? output
                                                   : std::numeric_limits<std::uint64_t>::max();
}

enum class Domain : std::uint8_t
{
    inside,
    outside,
    uncertain,
};

struct Arc
{
    Point center;
    Point start;
    Point end;
    bool counterclockwise = true;
    bool major_arc = false;
};

bool same_exact_point(Point left, Point right) noexcept
{
    return left.x.lower == left.x.upper && left.y.lower == left.y.upper &&
           right.x.lower == right.x.upper && right.y.lower == right.y.upper &&
           left.x.lower == right.x.lower && left.y.lower == right.y.lower;
}

Domain arc_domain(const Arc& arc, Point candidate) noexcept
{
    if (same_exact_point(candidate, arc.start) || same_exact_point(candidate, arc.end))
        return Domain::inside;
    const Point start = subtract(arc.start, arc.center);
    const Point end = subtract(arc.end, arc.center);
    const Point radial = subtract(candidate, arc.center);
    auto from_start = cross(start, radial);
    auto to_end = cross(radial, end);
    if (!arc.counterclockwise)
    {
        from_start = negate(from_start);
        to_end = negate(to_end);
    }
    if (!arc.major_arc)
    {
        if (from_start.lower >= 0.0 && to_end.lower >= 0.0)
            return Domain::inside;
        if (from_start.upper < 0.0 || to_end.upper < 0.0)
            return Domain::outside;
    }
    else
    {
        if (from_start.lower > 0.0 || to_end.lower > 0.0)
            return Domain::inside;
        if (from_start.upper <= 0.0 && to_end.upper <= 0.0)
            return Domain::outside;
    }
    return Domain::uncertain;
}

std::uint8_t shared_endpoint_count(const AnalyticAtomicCurveNm& left,
                                   const AnalyticAtomicCurveNm& right) noexcept
{
    std::uint8_t count = 0;
    for (const auto& a : {left.integer_start, left.integer_end})
        for (const auto& b : {right.integer_start, right.integer_end})
            if (a.x == b.x && a.y == b.y)
                ++count;
    return count;
}

bool contains_shared_endpoint(const AnalyticAtomicCurveNm& left, const AnalyticAtomicCurveNm& right,
                              const AnalyticFilteredPointNm& value) noexcept
{
    for (const auto& a : {left.integer_start, left.integer_end})
        for (const auto& b : {right.integer_start, right.integer_end})
            if (a.x == b.x && a.y == b.y && value.x.lower <= a.x && value.x.upper >= a.x &&
                value.y.lower <= a.y && value.y.upper >= a.y)
                return true;
    return false;
}

bool coincident_arc_domains_are_disjoint(const AnalyticAtomicCurveNm& left,
                                         const AnalyticAtomicCurveNm& right,
                                         std::uint8_t shared) noexcept
{
    if (left.kind != AnalyticAtomicCurveKind::circular_arc ||
        right.kind != AnalyticAtomicCurveKind::circular_arc || shared > 1)
        return false;
    const auto is_shared =
        [](const AnalyticIntegerPointNm& value, const AnalyticAtomicCurveNm& other)
    {
        return (value.x == other.integer_start.x && value.y == other.integer_start.y) ||
               (value.x == other.integer_end.x && value.y == other.integer_end.y);
    };
    const Arc left_arc{point(left.circle.center), point(left.start), point(left.end),
                       left.counterclockwise, left.major_arc};
    const Arc right_arc{point(right.circle.center), point(right.start), point(right.end),
                        right.counterclockwise, right.major_arc};
    for (const auto& endpoint : {left.integer_start, left.integer_end})
        if (!is_shared(endpoint, right) &&
            arc_domain(right_arc, exact_point(endpoint.x, endpoint.y)) != Domain::outside)
            return false;
    for (const auto& endpoint : {right.integer_start, right.integer_end})
        if (!is_shared(endpoint, left) &&
            arc_domain(left_arc, exact_point(endpoint.x, endpoint.y)) != Domain::outside)
            return false;
    return true;
}

bool complementary_arcs(const AnalyticAtomicCurveNm& left, const AnalyticAtomicCurveNm& right,
                        std::uint8_t shared) noexcept
{
    const bool reversed = left.integer_start.x == right.integer_end.x &&
                          left.integer_start.y == right.integer_end.y &&
                          left.integer_end.x == right.integer_start.x &&
                          left.integer_end.y == right.integer_start.y;
    const bool aligned = left.integer_start.x == right.integer_start.x &&
                         left.integer_start.y == right.integer_start.y &&
                         left.integer_end.x == right.integer_end.x &&
                         left.integer_end.y == right.integer_end.y;
    return left.kind == AnalyticAtomicCurveKind::circular_arc &&
           right.kind == AnalyticAtomicCurveKind::circular_arc && shared == 2 && !left.major_arc &&
           !right.major_arc &&
           ((reversed && left.counterclockwise == right.counterclockwise) ||
            (aligned && left.counterclockwise != right.counterclockwise));
}

bool valid_intersection(const AnalyticPairIntersection& intersection,
                        const std::vector<AnalyticAtomicCurveNm>& curves) noexcept
{
    if (intersection.pair.first == 0 || intersection.pair.second == 0 ||
        intersection.pair.first > curves.size() || intersection.pair.second > curves.size())
        return false;
    const auto& left = curves[intersection.pair.first - 1];
    const auto& right = curves[intersection.pair.second - 1];
    const std::uint8_t shared = shared_endpoint_count(left, right);
    if (intersection.relation != AnalyticPairRelation::coincident &&
        intersection.resolution_collapsed && (shared == 0 || intersection.point_count != shared))
        return false;
    if (intersection.relation == AnalyticPairRelation::disjoint)
        return shared == 0;
    if (intersection.relation == AnalyticPairRelation::coincident)
        return complementary_arcs(left, right, shared) ||
               coincident_arc_domains_are_disjoint(left, right, shared);
    if (intersection.point_count != shared)
        return false;
    for (std::uint8_t index = 0; index < intersection.point_count; ++index)
        if (!contains_shared_endpoint(left, right, intersection.points[index]))
            return false;
    return true;
}

class Validator
{
  public:
    Validator(std::int64_t origin_x_nm, std::int64_t origin_y_nm,
              const std::vector<AnalyticAtomicCurveNm>& curves,
              const std::vector<AnalyticCurveBoundsNm>& bounds,
              const AnalyticFilteredRegionsResult& original, const AnalyticSolverLimits& limits)
        : origin_x_nm_(origin_x_nm), origin_y_nm_(origin_y_nm), curves_(curves), bounds_(bounds),
          original_(original), limits_(limits)
    {
    }

    ReplayResult run()
    {
        AnalyticBroadPhaseResult broad = build_analytic_curve_candidates(bounds_, limits_);
        if (broad.error != AnalyticBroadPhaseError::none)
            return fail(ReplayError::resource_limit_exceeded);
        result_.candidate_pairs = broad.pairs.size();
        result_.peak_working_memory_bytes = broad.telemetry.peak_working_memory_bytes;
        if (!checked_multiply(broad.pairs.size(), kIndexLogicalBytes, pair_bytes_))
            return fail(ReplayError::resource_limit_exceeded);
        if (!charge(broad.telemetry.examined_curve_pairs))
            return result_;
        if (!validate_narrow(broad.pairs))
            return result_;
        if (!prepare_geometry())
            return result_;
        return validate_regions(broad.pairs);
    }

  private:
    bool charge(std::uint64_t units)
    {
        if (result_.work_units > limits_.predicate_calls ||
            units > limits_.predicate_calls - result_.work_units)
        {
            result_.error = ReplayError::resource_limit_exceeded;
            return false;
        }
        result_.work_units += units;
        return true;
    }

    ReplayResult fail(ReplayError error)
    {
        result_.error = error;
        return result_;
    }

    bool validate_narrow(const std::vector<AnalyticCurvePair>& pairs)
    {
        AnalyticSolverLimits limits = limits_;
        limits.predicate_calls -= result_.work_units;
        std::uint64_t pair_bytes = 0;
        if (!checked_multiply(pairs.size(),
                              kAnalyticNarrowPhasePairLogicalBytes + kIndexLogicalBytes,
                              pair_bytes) ||
            pair_bytes > limits.working_memory_bytes)
        {
            result_.error = ReplayError::resource_limit_exceeded;
            return false;
        }
        AnalyticNarrowPhaseResult narrow =
            intersect_analytic_curve_candidates(curves_, pairs, limits);
        result_.peak_working_memory_bytes =
            std::max(result_.peak_working_memory_bytes, narrow.telemetry.peak_working_memory_bytes);
        if (!charge(narrow.telemetry.predicate_calls))
            return false;
        if (narrow.error != AnalyticNarrowPhaseError::none)
        {
            result_.error = narrow.error == AnalyticNarrowPhaseError::invalid_argument
                                ? ReplayError::invalid_argument
                                : ReplayError::resource_limit_exceeded;
            return false;
        }
        std::uint64_t validation_work = 0;
        if (!checked_multiply(narrow.intersections.size(), kIntersectionValidationWork,
                              validation_work) ||
            !charge(validation_work))
            return false;
        for (const auto& intersection : narrow.intersections)
            if (!valid_intersection(intersection, curves_))
            {
                result_.error = ReplayError::topology_collapse;
                return false;
            }
        return true;
    }

    bool prepare_geometry()
    {
        std::uint64_t curve_bytes = 0;
        std::uint64_t ring_bytes = 0;
        std::uint64_t region_bytes = 0;
        if (!checked_multiply(curves_.size(), kReplayGeometryLogicalBytesPerCurve, curve_bytes) ||
            !checked_multiply(original_.rings.size(), kReplayRingScratchLogicalBytes, ring_bytes) ||
            !checked_multiply(original_.regions.size(), kReplayRegionScratchLogicalBytes,
                              region_bytes) ||
            !checked_add(curve_bytes, ring_bytes, own_bytes_) ||
            !checked_add(own_bytes_, region_bytes, own_bytes_) ||
            !checked_add(own_bytes_, pair_bytes_, own_bytes_) ||
            !checked_add(own_bytes_, kReplayFixedLogicalBytes, own_bytes_) ||
            own_bytes_ > limits_.working_memory_bytes)
        {
            result_.error = ReplayError::resource_limit_exceeded;
            return false;
        }
        std::uint64_t traversal_work = 0;
        if (!checked_multiply(curves_.size(), 3, traversal_work) ||
            !checked_add(traversal_work, sort_units(curves_.size()), traversal_work) ||
            !charge(traversal_work))
            return false;
        geometry_.origin_x_nm = origin_x_nm_;
        geometry_.origin_y_nm = origin_y_nm_;
        geometry_.curves = curves_;
        geometry_.bounds = bounds_;
        geometry_.occurrences.reserve(curves_.size());
        std::vector<std::uint32_t> circles;
        circles.reserve(curves_.size());
        for (std::uint32_t index = 0; index < curves_.size(); ++index)
            if (curves_[index].kind == AnalyticAtomicCurveKind::circular_arc)
                circles.push_back(index);
        const auto key = [this](std::uint32_t index)
        {
            const auto& circle = geometry_.curves[index].circle;
            return std::tie(circle.center.x.lower, circle.center.x.upper, circle.center.y.lower,
                            circle.center.y.upper, circle.radius.lower, circle.radius.upper);
        };
        std::sort(circles.begin(), circles.end(), [&key](std::uint32_t left, std::uint32_t right)
                  { return key(left) != key(right) ? key(left) < key(right) : left < right; });
        assign_circle_carriers(circles, key);
        for (std::uint32_t index = 0; index < geometry_.curves.size(); ++index)
            append_occurrence(index);
        return true;
    }

    template <typename Key>
    void assign_circle_carriers(const std::vector<std::uint32_t>& circles, const Key& key)
    {
        std::uint64_t carrier = geometry_.curves.size() + 1;
        for (std::size_t begin = 0; begin < circles.size();)
        {
            std::size_t end = begin + 1;
            while (end < circles.size() && key(circles[begin]) == key(circles[end]))
                ++end;
            for (std::size_t at = begin; at < end; ++at)
            {
                geometry_.curves[circles[at]].construction_carrier_id = carrier;
                geometry_.curves[circles[at]].construction_family_id = carrier;
            }
            ++carrier;
            begin = end;
        }
    }

    void append_occurrence(std::uint32_t index)
    {
        auto& curve = geometry_.curves[index];
        bool agrees = true;
        if (curve.kind == AnalyticAtomicCurveKind::line)
        {
            curve.construction_carrier_id = index + 1;
            curve.construction_family_id = index + 1;
            const std::int64_t dx = curve.integer_end.x - curve.integer_start.x;
            const std::int64_t dy = curve.integer_end.y - curve.integer_start.y;
            agrees = dx > 0 || (dx == 0 && dy > 0);
            curve.has_construction_line_direction = true;
            curve.construction_line_dx = agrees ? dx : -dx;
            curve.construction_line_dy = agrees ? dy : -dy;
            if (dx == 0)
            {
                const std::uint64_t column =
                    analytic_vertical_x_column_token(curve.construction_carrier_id);
                curve.start.construction_x_column_id = column;
                curve.end.construction_x_column_id = column;
            }
        }
        AnalyticFilteredOccurrence occurrence;
        occurrence.occurrence_id = index + 1;
        occurrence.coverage_id = 1;
        occurrence.agrees_with_carrier = agrees;
        occurrence.material_on_left = true;
        occurrence.source.kind = AnalyticFilteredSourceKind::authored_segment_curve;
        occurrence.source.role = curve.kind == AnalyticAtomicCurveKind::line
                                     ? AnalyticFilteredSourceRole::authored_line
                                     : AnalyticFilteredSourceRole::authored_circular_arc;
        occurrence.source.operand_id = 1;
        occurrence.source.primary_id = index + 1;
        occurrence.source.secondary_id = index + 1;
        geometry_.occurrences.push_back(occurrence);
    }

    ReplayResult validate_regions(const std::vector<AnalyticCurvePair>& pairs)
    {
        AnalyticRequestPacketRecords records;
        records.jobs.push_back({1, 0, 1});
        records.stages.push_back({1, 1, 0, 1});
        records.operands.push_back({1, 2, 0});
        AnalyticSolverLimits limits = limits_;
        limits.predicate_calls -= result_.work_units;
        AnalyticFilteredRegionsResult replay =
            build_analytic_filtered_regions(records, 0, geometry_, pairs, limits);
        std::uint64_t replay_peak = 0;
        if (!checked_add(own_bytes_, replay.telemetry.peak_working_memory_bytes, replay_peak) ||
            replay_peak > limits_.working_memory_bytes)
            return fail(ReplayError::resource_limit_exceeded);
        result_.peak_working_memory_bytes =
            std::max(result_.peak_working_memory_bytes, replay_peak);
        if (!charge(replay.telemetry.predicate_calls))
            return result_;
        if (replay.error != AnalyticFilteredRegionsError::none)
            return fail(replay.error == AnalyticFilteredRegionsError::resource_limit_exceeded
                            ? ReplayError::resource_limit_exceeded
                            : ReplayError::topology_collapse);
        if (!validate_ring_mapping(replay) || !validate_region_mapping(replay))
            return fail(ReplayError::topology_collapse);
        return result_;
    }

    bool build_boundary_map(const AnalyticFilteredRegionsResult& replay,
                            std::vector<std::uint32_t>& boundary_ring,
                            std::vector<std::uint32_t>& boundary_visits,
                            std::vector<std::uint32_t>& ring_counts)
    {
        if (!charge(replay.ring_half_edges.size() +
                    replay.selection.arrangement.memberships.size()))
            return false;
        for (std::uint32_t ring = 0; ring < replay.rings.size(); ++ring)
            for (std::uint32_t offset = 0; offset < replay.rings[ring].half_edge_count; ++offset)
            {
                const std::uint32_t half_index =
                    replay.ring_half_edges[replay.rings[ring].half_edge_begin + offset];
                if (half_index >= replay.selection.arrangement.half_edges.size())
                    return false;
                const auto& half = replay.selection.arrangement.half_edges[half_index];
                if (half.edge >= replay.selection.arrangement.edges.size())
                    return false;
                const auto& edge = replay.selection.arrangement.edges[half.edge];
                if (edge.membership_begin > replay.selection.arrangement.memberships.size() ||
                    edge.membership_count == 0 ||
                    edge.membership_count >
                        replay.selection.arrangement.memberships.size() - edge.membership_begin)
                    return false;
                for (std::uint32_t at = 0; at < edge.membership_count; ++at)
                {
                    const std::uint32_t curve =
                        replay.selection.arrangement.memberships[edge.membership_begin + at]
                            .curve_index;
                    if (curve == 0 || curve > boundary_ring.size())
                        return false;
                    const std::uint32_t boundary = curve - 1;
                    if (boundary_ring[boundary] != kNoIndex && boundary_ring[boundary] != ring)
                        return false;
                    boundary_ring[boundary] = ring;
                    if (++boundary_visits[boundary] == 1)
                        ++ring_counts[ring];
                }
            }
        return true;
    }

    bool validate_ring_mapping(const AnalyticFilteredRegionsResult& replay)
    {
        if (replay.rings.size() != original_.rings.size() ||
            replay.regions.size() != original_.regions.size())
            return false;
        std::vector<std::uint32_t> boundary_ring(curves_.size(), kNoIndex);
        std::vector<std::uint32_t> boundary_visits(curves_.size());
        std::vector<std::uint32_t> ring_counts(replay.rings.size());
        if (!build_boundary_map(replay, boundary_ring, boundary_visits, ring_counts) ||
            !charge(original_.ring_half_edges.size() + original_.rings.size() * 2))
            return false;
        old_to_replay_.assign(original_.rings.size(), kNoIndex);
        for (std::uint32_t old = 0; old < original_.rings.size(); ++old)
            if (!match_ring(old, replay, boundary_ring, boundary_visits, ring_counts))
                return false;
        for (std::uint32_t old = 0; old < original_.rings.size(); ++old)
        {
            const std::uint32_t parent = original_.rings[old].parent_ring;
            if (parent != kNoAnalyticFilteredRing && parent >= old_to_replay_.size())
                return false;
            const std::uint32_t expected = parent == kNoAnalyticFilteredRing
                                               ? kNoAnalyticFilteredRing
                                               : old_to_replay_[parent];
            if (replay.rings[old_to_replay_[old]].parent_ring != expected)
                return false;
        }
        return true;
    }

    bool match_ring(std::uint32_t old, const AnalyticFilteredRegionsResult& replay,
                    const std::vector<std::uint32_t>& boundary_ring,
                    const std::vector<std::uint32_t>& boundary_visits,
                    const std::vector<std::uint32_t>& ring_counts)
    {
        const auto& source = original_.rings[old];
        std::uint32_t matched = kNoIndex;
        for (std::uint32_t offset = 0; offset < source.half_edge_count; ++offset)
        {
            const std::uint32_t boundary = source.half_edge_begin + offset;
            if (boundary >= boundary_ring.size() || boundary_visits[boundary] != 1 ||
                boundary_ring[boundary] == kNoIndex)
                return false;
            if (matched == kNoIndex)
                matched = boundary_ring[boundary];
            else if (matched != boundary_ring[boundary])
                return false;
        }
        if (matched == kNoIndex || ring_counts[matched] != source.half_edge_count)
            return false;
        const auto& target = replay.rings[matched];
        if (target.counterclockwise != source.counterclockwise || target.depth != source.depth)
            return false;
        old_to_replay_[old] = matched;
        return true;
    }

    bool validate_region_mapping(const AnalyticFilteredRegionsResult& replay)
    {
        if (!charge(sort_units(replay.regions.size()) + sort_units(original_.regions.size()) +
                    replay.regions.size() + original_.regions.size()))
            return false;
        std::vector<std::uint32_t> actual;
        actual.reserve(replay.regions.size());
        for (const auto& region : replay.regions)
            actual.push_back(region.outer_ring);
        std::sort(actual.begin(), actual.end());
        std::vector<std::uint32_t> expected;
        expected.reserve(original_.regions.size());
        for (const auto& region : original_.regions)
        {
            if (region.outer_ring >= old_to_replay_.size())
                return false;
            expected.push_back(old_to_replay_[region.outer_ring]);
        }
        std::sort(expected.begin(), expected.end());
        return actual == expected;
    }

    std::int64_t origin_x_nm_ = 0;
    std::int64_t origin_y_nm_ = 0;
    const std::vector<AnalyticAtomicCurveNm>& curves_;
    const std::vector<AnalyticCurveBoundsNm>& bounds_;
    const AnalyticFilteredRegionsResult& original_;
    const AnalyticSolverLimits& limits_;
    ReplayResult result_;
    AnalyticFilteredGeometry geometry_;
    std::vector<std::uint32_t> old_to_replay_;
    std::uint64_t pair_bytes_ = 0;
    std::uint64_t own_bytes_ = 0;
};
} // namespace

ReplayResult validate_normalized_replay(std::int64_t origin_x_nm, std::int64_t origin_y_nm,
                                        const std::vector<AnalyticAtomicCurveNm>& curves,
                                        const std::vector<AnalyticCurveBoundsNm>& bounds,
                                        const AnalyticFilteredRegionsResult& original,
                                        const AnalyticSolverLimits& limits)
{
    return Validator(origin_x_nm, origin_y_nm, curves, bounds, original, limits).run();
}

static_assert(sizeof(AnalyticAtomicCurveNm) <= 256);
static_assert(sizeof(AnalyticCurveBoundsNm) <= 48);
static_assert(sizeof(AnalyticFilteredOccurrence) <= 56);

} // namespace geometer::analytic_normalization_detail

#include "geometer/analytic_operand_lowering.h"

#include "geometer/exact_arrangement.h"
#include "geometer/exact_construction.h"
#include "geometer/exact_curve_domain.h"
#include "geometer/exact_geometry.h"

#include <limits>
#include <utility>

namespace geometer
{
namespace
{

using exact::BigInt;

constexpr std::uint64_t kMaximumSpanNm = 1'000'000'000'000;
constexpr std::uint64_t kMaximumJobOccurrences = 131'072;

BigInt cross(const BigInt& ax, const BigInt& ay, const BigInt& bx, const BigInt& by)
{
    return ax * by - ay * bx;
}

BigInt dot(const BigInt& ax, const BigInt& ay, const BigInt& bx, const BigInt& by)
{
    return ax * bx + ay * by;
}

// Boundary attribution of one local pre-arrangement occurrence of a swept
// operand. Construction chords close each per-segment offset strip but lie
// strictly inside the vertex disks, so they may never survive onto the
// emitted union boundary.
struct SweptPieceSource
{
    exact::ExactSourceRole role = exact::ExactSourceRole::none;
    std::uint64_t secondary_id = 0;
    bool construction = false;
};

// Precomputed exact geometry of one authored ring segment in traversal order.
struct SegmentGeometry
{
    bool is_arc = false;
    bool ccw = true;
    bool major = false;
    std::int64_t sx = 0;
    std::int64_t sy = 0;
    std::int64_t ex = 0;
    std::int64_t ey = 0;
    std::int64_t cx = 0;
    std::int64_t cy = 0;
    BigInt squared_radius;
};

class JobLowerer
{
  public:
    JobLowerer(exact::ConstructionArena& arena, const AnalyticRequestPacketRecords& records)
        : arena_(arena), records_(records)
    {
    }

    AnalyticLoweredGeometryResult lower(std::uint32_t job_index)
    {
        const AnalyticRequestJobRecord& job = records_.jobs[job_index];
        for (std::uint32_t stage = 0; stage < job.stage_count; ++stage)
        {
            const AnalyticRequestStageRecord& record = records_.stages[job.stage_begin + stage];
            for (std::uint32_t index = 0; index < record.operand_count; ++index)
                if (!lower_operand(records_.operands[record.operand_begin + index]))
                    return {error_, std::nullopt};
        }
        if (!span_within_limit())
            return {AnalyticOperandLoweringError::resource_limit_exceeded, std::nullopt};
        return {AnalyticOperandLoweringError::none, std::move(out_)};
    }

  private:
    bool fail(AnalyticOperandLoweringError error)
    {
        error_ = error;
        return false;
    }

    void include_span(std::int64_t x, std::int64_t y)
    {
        include_span_doubled(BigInt(x) * 2, BigInt(y) * 2);
    }

    // The span accumulator tracks doubled coordinates so the exact width/2
    // boundary inflation of capsules and swept paths stays integral.
    void include_span_doubled(const BigInt& x, const BigInt& y)
    {
        if (!has_bounds_)
        {
            min_x_ = max_x_ = x;
            min_y_ = max_y_ = y;
            has_bounds_ = true;
            return;
        }
        if (x < min_x_)
            min_x_ = x;
        if (x > max_x_)
            max_x_ = x;
        if (y < min_y_)
            min_y_ = y;
        if (y > max_y_)
            max_y_ = y;
    }

    [[nodiscard]] bool span_within_limit() const
    {
        if (!has_bounds_)
            return true;
        const BigInt limit = BigInt(kMaximumSpanNm) * 2;
        return max_x_ - min_x_ <= limit && max_y_ - min_y_ <= limit;
    }

    [[nodiscard]] bool node(const BigInt& numerator, exact::ConstructionNodeId& value)
    {
        const exact::ConstructionResult result = arena_.make_rational(numerator);
        if (result.error != exact::Error::none || !result.node)
            return fail(AnalyticOperandLoweringError::resource_limit_exceeded);
        value = *result.node;
        return true;
    }

    [[nodiscard]] bool point(std::int64_t x, std::int64_t y, exact::ExactPoint& value)
    {
        return node(BigInt(x), value.x) && node(BigInt(y), value.y);
    }

    [[nodiscard]] bool square_root_node(const BigInt& square, exact::ConstructionNodeId& value)
    {
        exact::ConstructionNodeId square_node = 0;
        if (!node(square, square_node))
            return false;
        const exact::ConstructionResult result = arena_.make_nonnegative_square_root(square_node);
        if (result.error != exact::Error::none || !result.node)
            return fail(AnalyticOperandLoweringError::resource_limit_exceeded);
        value = *result.node;
        return true;
    }

    [[nodiscard]] bool rational_node(const BigInt& numerator, const BigInt& denominator,
                                     exact::ConstructionNodeId& value)
    {
        const exact::ConstructionResult result = arena_.make_rational(numerator, denominator);
        if (result.error != exact::Error::none || !result.node)
            return fail(AnalyticOperandLoweringError::resource_limit_exceeded);
        value = *result.node;
        return true;
    }

    [[nodiscard]] bool builder_ok(const exact::ConstructionBuilder& builder)
    {
        return builder.good() || fail(AnalyticOperandLoweringError::resource_limit_exceeded);
    }

    [[nodiscard]] bool points_equal(const exact::ExactPoint& left, const exact::ExactPoint& right,
                                    bool& equal)
    {
        const exact::ExactPredicateResult result = exact_points_equal(arena_, left, right);
        if (result.error != exact::Error::none || !result.value)
            return fail(AnalyticOperandLoweringError::resource_limit_exceeded);
        equal = *result.value;
        return true;
    }

    // agrees_with_carrier relates the emitted occurrence's travel direction to
    // the canonical carrier direction (positive first nonzero tangent for a
    // line, counterclockwise for a circle); material_on_left is relative to
    // the occurrence's own travel direction.
    [[nodiscard]] bool emit(exact::ExactAtomicCurve curve, bool agrees_with_carrier,
                            bool material_on_left, std::uint64_t coverage_id,
                            const exact::ExactSourceReference& source)
    {
        if (next_occurrence_ > kMaximumJobOccurrences)
            return fail(AnalyticOperandLoweringError::resource_limit_exceeded);
        const std::uint64_t occurrence = next_occurrence_++;
        curve.memberships = {{occurrence, agrees_with_carrier}};
        out_.curves.push_back(std::move(curve));
        out_.coverages.push_back({occurrence, coverage_id, material_on_left});
        out_.occurrence_sources.push_back({occurrence, coverage_id, source});
        return true;
    }

    [[nodiscard]] bool lower_operand(const AnalyticRequestOperandRecord& operand)
    {
        switch (operand.geometry_kind)
        {
        case 1:
            return lower_planar_region(operand);
        case 2:
        {
            const AnalyticRequestDiskRecord& disk = records_.disks[operand.geometry_index];
            const BigInt radius = BigInt(disk.radius_nm) * 2;
            include_span_doubled(BigInt(disk.center_x_nm) * 2 - radius,
                                 BigInt(disk.center_y_nm) * 2 - radius);
            include_span_doubled(BigInt(disk.center_x_nm) * 2 + radius,
                                 BigInt(disk.center_y_nm) * 2 + radius);
            return lower_full_circle(disk.center_x_nm, disk.center_y_nm, disk.radius_nm, true,
                                     operand.operand_id,
                                     {exact::ExactSourceKind::compact_feature_role,
                                      exact::ExactSourceRole::primitive_outer_circle,
                                      operand.operand_id, disk.feature_id, 0});
        }
        case 3:
        {
            const AnalyticRequestAnnulusRecord& annulus = records_.annuli[operand.geometry_index];
            const BigInt outer = BigInt(annulus.outer_radius_nm) * 2;
            include_span_doubled(BigInt(annulus.center_x_nm) * 2 - outer,
                                 BigInt(annulus.center_y_nm) * 2 - outer);
            include_span_doubled(BigInt(annulus.center_x_nm) * 2 + outer,
                                 BigInt(annulus.center_y_nm) * 2 + outer);
            return lower_full_circle(annulus.center_x_nm, annulus.center_y_nm,
                                     annulus.outer_radius_nm, true, operand.operand_id,
                                     {exact::ExactSourceKind::compact_feature_role,
                                      exact::ExactSourceRole::primitive_outer_circle,
                                      operand.operand_id, annulus.feature_id, 0}) &&
                   lower_full_circle(annulus.center_x_nm, annulus.center_y_nm,
                                     annulus.inner_radius_nm, false, operand.operand_id,
                                     {exact::ExactSourceKind::compact_feature_role,
                                      exact::ExactSourceRole::primitive_inner_circle,
                                      operand.operand_id, annulus.feature_id, 0});
        }
        case 4:
            return lower_capsule(operand);
        case 5:
            return lower_swept_path(operand);
        default:
            // Packet validation admits only kinds 1..5; anything else is a
            // future geometry class this build cannot execute.
            return fail(AnalyticOperandLoweringError::unsupported_geometry);
        }
    }

    // Emits one full circle as the exact two-half-arc convention: the split
    // vertices are the circle's leftmost and rightmost points, both halves
    // travel counterclockwise, and the material-side flag encodes whether
    // the operand interior is inside (outer boundary) or outside (hole).
    [[nodiscard]] bool lower_full_circle(std::int64_t cx, std::int64_t cy, std::uint64_t radius,
                                         bool interior_inside, std::uint64_t coverage_id,
                                         const exact::ExactSourceReference& source)
    {
        const BigInt r(radius);
        const BigInt center_x(cx);
        const BigInt center_y(cy);
        exact::ExactPoint left;
        exact::ExactPoint right;
        exact::ExactCircle circle;
        if (!node(center_x - r, left.x) || !node(center_y, left.y) ||
            !node(center_x + r, right.x) || !node(center_y, right.y) ||
            !node(center_x, circle.center.x) || !node(center_y, circle.center.y) ||
            !node(r, circle.radius))
            return false;
        exact::ExactAtomicCurve lower_half{
            exact::ExactAtomicCurveKind::circular_arc, left, right, circle, true, false, {}};
        exact::ExactAtomicCurve upper_half{
            exact::ExactAtomicCurveKind::circular_arc, right, left, circle, true, false, {}};
        return emit(std::move(lower_half), true, interior_inside, coverage_id, source) &&
               emit(std::move(upper_half), true, interior_inside, coverage_id, source);
    }

    [[nodiscard]] bool lower_planar_region(const AnalyticRequestOperandRecord& operand)
    {
        const AnalyticRequestPlanarRegionRecord& region =
            records_.planar_regions[operand.geometry_index];
        if (!lower_authored_ring(records_.rings[region.outer_ring], false, operand.operand_id))
            return false;
        for (std::uint32_t index = 0; index < region.hole_reference_count; ++index)
        {
            const std::uint32_t ring =
                records_.ring_references[region.hole_reference_begin + index];
            if (!lower_authored_ring(records_.rings[ring], true, operand.operand_id))
                return false;
        }
        return true;
    }

    // A capsule's union boundary is directly constructible: two exact offset
    // lines and two counterclockwise semicircular end caps traversed
    // counterclockwise around the interior. The half-width offset vector
    // (-dy, dx) * (w / (2 * length)) is built in the construction arena, so
    // odd widths and irrational segment lengths stay exact.
    [[nodiscard]] bool lower_capsule(const AnalyticRequestOperandRecord& operand)
    {
        const AnalyticRequestCapsuleRecord& capsule = records_.capsules[operand.geometry_index];
        if (capsule.start_x_nm == capsule.end_x_nm && capsule.start_y_nm == capsule.end_y_nm)
            return fail(AnalyticOperandLoweringError::invalid_topology);
        const BigInt width(capsule.width_nm);
        include_span_doubled(BigInt(capsule.start_x_nm) * 2 - width,
                             BigInt(capsule.start_y_nm) * 2 - width);
        include_span_doubled(BigInt(capsule.start_x_nm) * 2 + width,
                             BigInt(capsule.start_y_nm) * 2 + width);
        include_span_doubled(BigInt(capsule.end_x_nm) * 2 - width,
                             BigInt(capsule.end_y_nm) * 2 - width);
        include_span_doubled(BigInt(capsule.end_x_nm) * 2 + width,
                             BigInt(capsule.end_y_nm) * 2 + width);

        const BigInt dx = BigInt(capsule.end_x_nm) - capsule.start_x_nm;
        const BigInt dy = BigInt(capsule.end_y_nm) - capsule.start_y_nm;
        exact::ConstructionBuilder builder(arena_);
        exact::ConstructionNodeId half_width = 0;
        exact::ConstructionNodeId length = 0;
        exact::ConstructionNodeId dx_node = 0;
        exact::ConstructionNodeId minus_dy = 0;
        if (!rational_node(width, BigInt(2), half_width) ||
            !square_root_node(dx * dx + dy * dy, length) || !node(dx, dx_node) ||
            !node(-dy, minus_dy))
            return false;
        const exact::ConstructionNodeId factor = builder.divide(half_width, length);
        const exact::ConstructionNodeId offset_x = builder.product(minus_dy, factor);
        const exact::ConstructionNodeId offset_y = builder.product(dx_node, factor);
        exact::ExactPoint start;
        exact::ExactPoint end;
        if (!point(capsule.start_x_nm, capsule.start_y_nm, start) ||
            !point(capsule.end_x_nm, capsule.end_y_nm, end))
            return false;
        const exact::ExactPoint start_left{builder.sum(start.x, offset_x),
                                           builder.sum(start.y, offset_y)};
        const exact::ExactPoint start_right{builder.subtract(start.x, offset_x),
                                            builder.subtract(start.y, offset_y)};
        const exact::ExactPoint end_left{builder.sum(end.x, offset_x),
                                         builder.sum(end.y, offset_y)};
        const exact::ExactPoint end_right{builder.subtract(end.x, offset_x),
                                          builder.subtract(end.y, offset_y)};
        if (!builder_ok(builder))
            return false;

        const bool forward_agrees = dx != 0 ? dx > 0 : dy > 0;
        const exact::ExactCircle start_circle{start, half_width};
        const exact::ExactCircle end_circle{end, half_width};
        const auto reference = [&](exact::ExactSourceRole role)
        {
            return exact::ExactSourceReference{exact::ExactSourceKind::compact_feature_role, role,
                                               operand.operand_id, capsule.feature_id, 0};
        };
        return emit(
                   {exact::ExactAtomicCurveKind::line, start_right, end_right, {}, true, false, {}},
                   forward_agrees, true, operand.operand_id,
                   reference(exact::ExactSourceRole::capsule_right_line)) &&
               emit({exact::ExactAtomicCurveKind::circular_arc,
                     end_right,
                     end_left,
                     end_circle,
                     true,
                     false,
                     {}},
                    true, true, operand.operand_id,
                    reference(exact::ExactSourceRole::capsule_end_cap)) &&
               emit({exact::ExactAtomicCurveKind::line, end_left, start_left, {}, true, false, {}},
                    !forward_agrees, true, operand.operand_id,
                    reference(exact::ExactSourceRole::capsule_left_line)) &&
               emit({exact::ExactAtomicCurveKind::circular_arc,
                     start_left,
                     start_right,
                     start_circle,
                     true,
                     false,
                     {}},
                    true, true, operand.operand_id,
                    reference(exact::ExactSourceRole::capsule_start_cap));
    }

    // Swept paths follow the design's exact-offset construction: every
    // centerline vertex contributes one full width/2 disk (round caps and
    // joins) and every centerline segment one offset strip, each as its own
    // local coverage. Overlap among the pieces is resolved by a job-local
    // exact pre-arrangement; the union boundary is then re-emitted as this
    // operand's occurrences.
    [[nodiscard]] bool lower_swept_path(const AnalyticRequestOperandRecord& operand)
    {
        const AnalyticRequestSweptPathRecord& swept = records_.swept_paths[operand.geometry_index];
        const AnalyticRequestRingRecord& ring = records_.rings[swept.path_ring];
        const BigInt width(swept.width_nm);

        for (std::uint32_t index = 0; index < ring.vertex_count; ++index)
        {
            const AnalyticRequestVertexRecord& vertex =
                records_.vertices[ring.vertex_begin + index];
            include_span_doubled(BigInt(vertex.x_nm) * 2 - width, BigInt(vertex.y_nm) * 2 - width);
            include_span_doubled(BigInt(vertex.x_nm) * 2 + width, BigInt(vertex.y_nm) * 2 + width);
        }

        std::vector<SegmentGeometry> segments(ring.segment_count);
        for (std::uint32_t index = 0; index < ring.segment_count; ++index)
        {
            const AnalyticRequestVertexRecord& start = records_.vertices[ring.vertex_begin + index];
            const AnalyticRequestVertexRecord& end =
                records_.vertices[ring.vertex_begin + index + 1];
            SegmentGeometry& segment = segments[index];
            segment.sx = start.x_nm;
            segment.sy = start.y_nm;
            segment.ex = end.x_nm;
            segment.ey = end.y_nm;
            if (!validate_segment(records_.segments[ring.segment_begin + index], segment))
                return false;
            if (segment.is_arc)
            {
                include_span(segment.cx, segment.cy);
                // A circular centerline needs a radius strictly greater than
                // width / 2 so the inner offset stays a real arc.
                if (BigInt(4) * segment.squared_radius <= width * width)
                    return fail(AnalyticOperandLoweringError::invalid_arc);
            }
        }

        // Consecutive segments may not reverse direction through an exact
        // 180-degree cusp.
        for (std::uint32_t index = 0; index + 1 < ring.segment_count; ++index)
        {
            BigInt out_x;
            BigInt out_y;
            tangent(segments[index], false, out_x, out_y);
            BigInt in_x;
            BigInt in_y;
            tangent(segments[index + 1], true, in_x, in_y);
            if (cross(out_x, out_y, in_x, in_y) == 0 && dot(out_x, out_y, in_x, in_y) < 0)
                return fail(AnalyticOperandLoweringError::invalid_topology);
        }

        return validate_path_simplicity(segments) &&
               lower_swept_boundary(operand, swept, segments, width);
    }

    // The centerline may not retrace or intersect itself: any carrier
    // intersection or overlap point lying on both closed segment domains is
    // rejected, except the single shared vertex of an adjacent pair. The
    // pairwise scan is quadratic in the segment count; the governed segment
    // limit and the arena budget bound its cost.
    [[nodiscard]] bool validate_path_simplicity(const std::vector<SegmentGeometry>& segments)
    {
        struct Carrier
        {
            exact::ExactPoint start;
            exact::ExactPoint end;
            exact::ExactCircularArc arc;
            bool is_arc = false;
        };
        std::vector<Carrier> carriers(segments.size());
        for (std::size_t index = 0; index < segments.size(); ++index)
        {
            const SegmentGeometry& segment = segments[index];
            Carrier& carrier = carriers[index];
            if (!point(segment.sx, segment.sy, carrier.start) ||
                !point(segment.ex, segment.ey, carrier.end))
                return false;
            if (!segment.is_arc)
                continue;
            carrier.is_arc = true;
            carrier.arc.start = carrier.start;
            carrier.arc.end = carrier.end;
            carrier.arc.counterclockwise = segment.ccw;
            carrier.arc.major_arc = segment.major;
            if (!point(segment.cx, segment.cy, carrier.arc.circle.center) ||
                !square_root_node(segment.squared_radius, carrier.arc.circle.radius))
                return false;
        }

        const auto on_carrier =
            [&](const Carrier& carrier, const exact::ExactPoint& probe, bool& on)
        {
            const exact::ExactPredicateResult result =
                carrier.is_arc
                    ? point_on_closed_exact_arc(arena_, carrier.arc, probe)
                    : point_on_closed_exact_segment(arena_, {carrier.start, carrier.end}, probe);
            if (result.error != exact::Error::none || !result.value)
                return fail(AnalyticOperandLoweringError::resource_limit_exceeded);
            on = *result.value;
            return true;
        };

        for (std::size_t a = 0; a < segments.size(); ++a)
        {
            for (std::size_t b = a + 1; b < segments.size(); ++b)
            {
                const Carrier& first = carriers[a];
                const Carrier& second = carriers[b];
                const bool adjacent = b == a + 1;
                exact::ExactIntersectionResult intersection;
                if (first.is_arc && second.is_arc)
                    intersection =
                        intersect_exact_circles(arena_, first.arc.circle, second.arc.circle);
                else if (first.is_arc)
                    intersection = intersect_exact_line_circle(arena_, {second.start, second.end},
                                                               first.arc.circle);
                else if (second.is_arc)
                    intersection = intersect_exact_line_circle(arena_, {first.start, first.end},
                                                               second.arc.circle);
                else
                    intersection = intersect_exact_lines(arena_, {first.start, first.end},
                                                         {second.start, second.end});
                if (intersection.error != exact::Error::none)
                    return fail(AnalyticOperandLoweringError::resource_limit_exceeded);

                const auto reject_shared = [&](const exact::ExactPoint& candidate, bool& reject)
                {
                    reject = true;
                    bool allowed = false;
                    if (adjacent && !points_equal(candidate, second.start, allowed))
                        return false;
                    reject = !allowed;
                    return true;
                };

                if (intersection.relation == exact::IntersectionRelation::coincident)
                {
                    const exact::ExactPoint probes[4] = {second.start, second.end, first.start,
                                                         first.end};
                    const Carrier* hosts[4] = {&first, &first, &second, &second};
                    for (std::size_t probe = 0; probe < 4; ++probe)
                    {
                        bool on = false;
                        if (!on_carrier(*hosts[probe], probes[probe], on))
                            return false;
                        if (!on)
                            continue;
                        bool reject = false;
                        if (!reject_shared(probes[probe], reject))
                            return false;
                        if (reject)
                            return fail(AnalyticOperandLoweringError::invalid_topology);
                    }
                    continue;
                }
                for (std::uint32_t index = 0; index < intersection.point_count; ++index)
                {
                    const exact::ExactPoint& candidate = intersection.points[index];
                    bool on_first = false;
                    bool on_second = false;
                    if (!on_carrier(first, candidate, on_first) ||
                        !on_carrier(second, candidate, on_second))
                        return false;
                    if (!on_first || !on_second)
                        continue;
                    bool reject = false;
                    if (!reject_shared(candidate, reject))
                        return false;
                    if (reject)
                        return fail(AnalyticOperandLoweringError::invalid_topology);
                }
            }
        }
        return true;
    }

    // Exact sign of cross(p - c, q - c).
    [[nodiscard]] bool radial_cross_sign(const exact::ExactPoint& c, const exact::ExactPoint& p,
                                         const exact::ExactPoint& q, int& sign)
    {
        exact::ConstructionBuilder builder(arena_);
        const exact::ConstructionNodeId ux = builder.subtract(p.x, c.x);
        const exact::ConstructionNodeId uy = builder.subtract(p.y, c.y);
        const exact::ConstructionNodeId vx = builder.subtract(q.x, c.x);
        const exact::ConstructionNodeId vy = builder.subtract(q.y, c.y);
        const std::int8_t value =
            builder.sign(builder.subtract(builder.product(ux, vy), builder.product(uy, vx)));
        if (!builder_ok(builder))
            return false;
        sign = value;
        return true;
    }

    // Exact sign of dot(p - c, q - c).
    [[nodiscard]] bool radial_dot_sign(const exact::ExactPoint& c, const exact::ExactPoint& p,
                                       const exact::ExactPoint& q, int& sign)
    {
        exact::ConstructionBuilder builder(arena_);
        const exact::ConstructionNodeId ux = builder.subtract(p.x, c.x);
        const exact::ConstructionNodeId uy = builder.subtract(p.y, c.y);
        const exact::ConstructionNodeId vx = builder.subtract(q.x, c.x);
        const exact::ConstructionNodeId vy = builder.subtract(q.y, c.y);
        const std::int8_t value =
            builder.sign(builder.sum(builder.product(ux, vx), builder.product(uy, vy)));
        if (!builder_ok(builder))
            return false;
        sign = value;
        return true;
    }

    // Counterclockwise angular class of a point measured from the arc start
    // around the center: 0 for (0, pi), 1 for exactly pi, 2 for (pi, 2 pi).
    [[nodiscard]] bool arc_angle_class(const exact::ExactAtomicCurve& curve,
                                       const exact::ExactPoint& point, int& angle_class)
    {
        int cross_sign = 0;
        if (!radial_cross_sign(curve.circle.center, curve.start, point, cross_sign))
            return false;
        if (cross_sign > 0)
        {
            angle_class = 0;
            return true;
        }
        if (cross_sign < 0)
        {
            angle_class = 2;
            return true;
        }
        int dot_sign = 0;
        if (!radial_dot_sign(curve.circle.center, curve.start, point, dot_sign))
            return false;
        // A split point coinciding with the arc start was excluded earlier,
        // so a nonnegative dot cannot occur; fail closed if it does.
        if (dot_sign >= 0)
            return fail(AnalyticOperandLoweringError::invalid_topology);
        angle_class = 1;
        return true;
    }

    // Orders two interior split points along the curve's own travel
    // direction: -1 when a precedes b, 0 when they are the same point.
    [[nodiscard]] bool travel_order(const exact::ExactAtomicCurve& curve,
                                    const exact::ExactPoint& a, const exact::ExactPoint& b,
                                    int& order)
    {
        if (curve.kind == exact::ExactAtomicCurveKind::line)
        {
            exact::ConstructionBuilder builder(arena_);
            const std::int8_t direction_x = builder.compare(curve.end.x, curve.start.x);
            const std::int8_t direction_y = builder.compare(curve.end.y, curve.start.y);
            const std::int8_t order_x = builder.compare(a.x, b.x);
            const std::int8_t order_y = builder.compare(a.y, b.y);
            if (!builder_ok(builder))
                return false;
            if (direction_x != 0 && order_x != 0)
                order = direction_x > 0 ? order_x : -order_x;
            else if (direction_y != 0 && order_y != 0)
                order = direction_y > 0 ? order_y : -order_y;
            else
                order = 0;
            return true;
        }
        int class_a = 0;
        int class_b = 0;
        if (!arc_angle_class(curve, a, class_a) || !arc_angle_class(curve, b, class_b))
            return false;
        int ccw_order = 0;
        if (class_a != class_b)
            ccw_order = class_a < class_b ? -1 : 1;
        else
        {
            int cross_sign = 0;
            if (!radial_cross_sign(curve.circle.center, a, b, cross_sign))
                return false;
            ccw_order = cross_sign > 0 ? -1 : (cross_sign < 0 ? 1 : 0);
        }
        order = curve.counterclockwise ? ccw_order : -ccw_order;
        return true;
    }

    // Major flag for the sub-arc traveled from p to q in the parent arc's
    // direction; an exact semicircle is encoded with major false.
    [[nodiscard]] bool sub_arc_major(const exact::ExactAtomicCurve& curve,
                                     const exact::ExactPoint& p, const exact::ExactPoint& q,
                                     bool& major)
    {
        int cross_sign = 0;
        if (!radial_cross_sign(curve.circle.center, p, q, cross_sign))
            return false;
        major = curve.counterclockwise ? cross_sign < 0 : cross_sign > 0;
        return true;
    }

    // build_exact_arrangement consumes atomic curves that may meet only at
    // shared endpoints, so every swept piece is pre-split at each mutual
    // intersection or same-carrier overlap endpoint before the job-local
    // pre-arrangement is built. Memberships (and therefore local coverage
    // and source attribution) carry over to every fragment unchanged.
    [[nodiscard]] bool atomize_swept_pieces(std::vector<exact::ExactAtomicCurve>& curves)
    {
        const auto arc_of = [](const exact::ExactAtomicCurve& curve)
        {
            return exact::ExactCircularArc{curve.circle, curve.start, curve.end,
                                           curve.counterclockwise, curve.major_arc};
        };
        const auto on_curve =
            [&](const exact::ExactAtomicCurve& curve, const exact::ExactPoint& probe, bool& on)
        {
            const exact::ExactPredicateResult result =
                curve.kind == exact::ExactAtomicCurveKind::circular_arc
                    ? point_on_closed_exact_arc(arena_, arc_of(curve), probe)
                    : point_on_closed_exact_segment(arena_, {curve.start, curve.end}, probe);
            if (result.error != exact::Error::none || !result.value)
                return fail(AnalyticOperandLoweringError::resource_limit_exceeded);
            on = *result.value;
            return true;
        };

        std::vector<std::vector<exact::ExactPoint>> splits(curves.size());
        const auto add_split = [&](std::size_t index, const exact::ExactPoint& candidate)
        {
            const exact::ExactAtomicCurve& curve = curves[index];
            bool at_start = false;
            bool at_end = false;
            if (!points_equal(candidate, curve.start, at_start) ||
                !points_equal(candidate, curve.end, at_end))
                return false;
            if (at_start || at_end)
                return true;
            bool on = false;
            if (!on_curve(curve, candidate, on))
                return false;
            if (on)
                splits[index].push_back(candidate);
            return true;
        };

        for (std::size_t a = 0; a < curves.size(); ++a)
        {
            for (std::size_t b = a + 1; b < curves.size(); ++b)
            {
                const exact::ExactAtomicCurve& first = curves[a];
                const exact::ExactAtomicCurve& second = curves[b];
                const bool first_arc = first.kind == exact::ExactAtomicCurveKind::circular_arc;
                const bool second_arc = second.kind == exact::ExactAtomicCurveKind::circular_arc;
                exact::ExactIntersectionResult intersection;
                if (first_arc && second_arc)
                    intersection = intersect_exact_circles(arena_, first.circle, second.circle);
                else if (first_arc)
                    intersection = intersect_exact_line_circle(arena_, {second.start, second.end},
                                                               first.circle);
                else if (second_arc)
                    intersection = intersect_exact_line_circle(arena_, {first.start, first.end},
                                                               second.circle);
                else
                    intersection = intersect_exact_lines(arena_, {first.start, first.end},
                                                         {second.start, second.end});
                if (intersection.error != exact::Error::none)
                    return fail(AnalyticOperandLoweringError::resource_limit_exceeded);
                if (intersection.relation == exact::IntersectionRelation::coincident)
                {
                    if (!add_split(a, second.start) || !add_split(a, second.end) ||
                        !add_split(b, first.start) || !add_split(b, first.end))
                        return false;
                    continue;
                }
                for (std::uint32_t index = 0; index < intersection.point_count; ++index)
                {
                    const exact::ExactPoint& candidate = intersection.points[index];
                    bool on_first = false;
                    bool on_second = false;
                    if (!on_curve(first, candidate, on_first) ||
                        !on_curve(second, candidate, on_second))
                        return false;
                    if (!on_first || !on_second)
                        continue;
                    if (!add_split(a, candidate) || !add_split(b, candidate))
                        return false;
                }
            }
        }

        std::vector<exact::ExactAtomicCurve> output;
        output.reserve(curves.size());
        for (std::size_t index = 0; index < curves.size(); ++index)
        {
            exact::ExactAtomicCurve& curve = curves[index];
            if (splits[index].empty())
            {
                output.push_back(std::move(curve));
                continue;
            }
            std::vector<exact::ExactPoint> ordered;
            ordered.reserve(splits[index].size());
            for (const exact::ExactPoint& candidate : splits[index])
            {
                std::size_t position = ordered.size();
                bool duplicate = false;
                for (std::size_t at = 0; at < ordered.size(); ++at)
                {
                    int order = 0;
                    if (!travel_order(curve, candidate, ordered[at], order))
                        return false;
                    if (order == 0)
                    {
                        duplicate = true;
                        break;
                    }
                    if (order < 0)
                    {
                        position = at;
                        break;
                    }
                }
                if (!duplicate)
                    ordered.insert(ordered.begin() + position, candidate);
            }
            exact::ExactPoint previous = curve.start;
            for (std::size_t at = 0; at <= ordered.size(); ++at)
            {
                const exact::ExactPoint next = at == ordered.size() ? curve.end : ordered[at];
                exact::ExactAtomicCurve piece;
                piece.kind = curve.kind;
                piece.start = previous;
                piece.end = next;
                piece.circle = curve.circle;
                piece.counterclockwise = curve.counterclockwise;
                piece.memberships = curve.memberships;
                if (piece.kind == exact::ExactAtomicCurveKind::circular_arc &&
                    !sub_arc_major(curve, previous, next, piece.major_arc))
                    return false;
                output.push_back(std::move(piece));
                previous = next;
            }
        }
        curves = std::move(output);
        return true;
    }

    [[nodiscard]] bool lower_swept_boundary(const AnalyticRequestOperandRecord& operand,
                                            const AnalyticRequestSweptPathRecord& swept,
                                            const std::vector<SegmentGeometry>& segments,
                                            const BigInt& width)
    {
        exact::ConstructionNodeId half_width = 0;
        if (!rational_node(width, BigInt(2), half_width))
            return false;

        std::vector<exact::ExactAtomicCurve> piece_curves;
        std::vector<exact::ExactCoverageOccurrence> piece_coverages;
        std::vector<SweptPieceSource> piece_sources;
        std::uint64_t local_coverage = 0;
        const auto add_curve = [&](exact::ExactAtomicCurve curve, bool agrees,
                                   bool material_on_left, exact::ExactSourceRole role,
                                   std::uint64_t secondary, bool construction)
        {
            const std::uint64_t occurrence = piece_curves.size() + 1;
            curve.memberships = {{occurrence, agrees}};
            piece_curves.push_back(std::move(curve));
            piece_coverages.push_back({occurrence, local_coverage, material_on_left});
            piece_sources.push_back({role, secondary, construction});
        };

        const std::size_t segment_count = segments.size();
        for (std::size_t index = 0; index <= segment_count; ++index)
        {
            const bool last = index == segment_count;
            const std::int64_t vx = last ? segments[segment_count - 1].ex : segments[index].sx;
            const std::int64_t vy = last ? segments[segment_count - 1].ey : segments[index].sy;
            exact::ExactSourceRole role = exact::ExactSourceRole::swept_round_join;
            std::uint64_t secondary = (std::uint64_t(index) << 32) | std::uint64_t(index + 1);
            if (index == 0)
            {
                role = exact::ExactSourceRole::swept_start_cap;
                secondary = std::uint64_t(1) << 32;
            }
            else if (last)
            {
                role = exact::ExactSourceRole::swept_end_cap;
                secondary = std::uint64_t(segment_count + 1) << 32;
            }
            exact::ExactPoint center;
            exact::ExactPoint left;
            exact::ExactPoint right;
            if (!point(vx, vy, center) ||
                !rational_node(BigInt(vx) * 2 - width, BigInt(2), left.x) ||
                !node(BigInt(vy), left.y) ||
                !rational_node(BigInt(vx) * 2 + width, BigInt(2), right.x) ||
                !node(BigInt(vy), right.y))
                return false;
            const exact::ExactCircle circle{center, half_width};
            ++local_coverage;
            add_curve(
                {exact::ExactAtomicCurveKind::circular_arc, left, right, circle, true, false, {}},
                true, true, role, secondary, false);
            add_curve(
                {exact::ExactAtomicCurveKind::circular_arc, right, left, circle, true, false, {}},
                true, true, role, secondary, false);
        }

        exact::ConstructionBuilder builder(arena_);
        for (std::size_t index = 0; index < segment_count; ++index)
        {
            const SegmentGeometry& segment = segments[index];
            const std::uint64_t offset_key = std::uint64_t(index + 1) << 32;
            exact::ExactPoint start;
            exact::ExactPoint end;
            if (!point(segment.sx, segment.sy, start) || !point(segment.ex, segment.ey, end))
                return false;
            ++local_coverage;
            if (!segment.is_arc)
            {
                const BigInt dx = BigInt(segment.ex) - segment.sx;
                const BigInt dy = BigInt(segment.ey) - segment.sy;
                exact::ConstructionNodeId length = 0;
                exact::ConstructionNodeId dx_node = 0;
                exact::ConstructionNodeId minus_dy = 0;
                if (!square_root_node(dx * dx + dy * dy, length) || !node(dx, dx_node) ||
                    !node(-dy, minus_dy))
                    return false;
                const exact::ConstructionNodeId factor = builder.divide(half_width, length);
                const exact::ConstructionNodeId offset_x = builder.product(minus_dy, factor);
                const exact::ConstructionNodeId offset_y = builder.product(dx_node, factor);
                const exact::ExactPoint start_left{builder.sum(start.x, offset_x),
                                                   builder.sum(start.y, offset_y)};
                const exact::ExactPoint start_right{builder.subtract(start.x, offset_x),
                                                    builder.subtract(start.y, offset_y)};
                const exact::ExactPoint end_left{builder.sum(end.x, offset_x),
                                                 builder.sum(end.y, offset_y)};
                const exact::ExactPoint end_right{builder.subtract(end.x, offset_x),
                                                  builder.subtract(end.y, offset_y)};
                if (!builder_ok(builder))
                    return false;
                const bool forward_agrees = dx != 0 ? dx > 0 : dy > 0;
                const bool chord_agrees = dy != 0 ? dy < 0 : dx > 0;
                add_curve({exact::ExactAtomicCurveKind::line,
                           start_right,
                           end_right,
                           {},
                           true,
                           false,
                           {}},
                          forward_agrees, true, exact::ExactSourceRole::swept_right_offset_line,
                          offset_key, false);
                add_curve(
                    {exact::ExactAtomicCurveKind::line, start_left, end_left, {}, true, false, {}},
                    forward_agrees, false, exact::ExactSourceRole::swept_left_offset_line,
                    offset_key, false);
                // The closing chords bisect the endpoint disks, so they can
                // never survive onto the union boundary.
                add_curve({exact::ExactAtomicCurveKind::line,
                           start_right,
                           start_left,
                           {},
                           true,
                           false,
                           {}},
                          chord_agrees, false, exact::ExactSourceRole::none, 0, true);
                add_curve(
                    {exact::ExactAtomicCurveKind::line, end_right, end_left, {}, true, false, {}},
                    chord_agrees, true, exact::ExactSourceRole::none, 0, true);
                continue;
            }

            exact::ExactPoint center;
            exact::ConstructionNodeId radius = 0;
            if (!point(segment.cx, segment.cy, center) ||
                !square_root_node(segment.squared_radius, radius))
                return false;
            const exact::ConstructionNodeId inner_radius = builder.subtract(radius, half_width);
            const exact::ConstructionNodeId outer_radius = builder.sum(radius, half_width);
            const exact::ConstructionNodeId inner_scale = builder.divide(inner_radius, radius);
            const exact::ConstructionNodeId outer_scale = builder.divide(outer_radius, radius);
            const auto offset_point =
                [&](const exact::ExactPoint& base, exact::ConstructionNodeId scale)
            {
                const exact::ConstructionNodeId radial_x = builder.subtract(base.x, center.x);
                const exact::ConstructionNodeId radial_y = builder.subtract(base.y, center.y);
                return exact::ExactPoint{builder.sum(center.x, builder.product(radial_x, scale)),
                                         builder.sum(center.y, builder.product(radial_y, scale))};
            };
            const exact::ExactPoint start_inner = offset_point(start, inner_scale);
            const exact::ExactPoint start_outer = offset_point(start, outer_scale);
            const exact::ExactPoint end_inner = offset_point(end, inner_scale);
            const exact::ExactPoint end_outer = offset_point(end, outer_scale);
            if (!builder_ok(builder))
                return false;
            const exact::ExactCircle inner_circle{center, inner_radius};
            const exact::ExactCircle outer_circle{center, outer_radius};
            // Traveling the segment direction, the center is on the left for
            // counterclockwise arcs, so the inner offset is the left offset.
            const exact::ExactSourceRole inner_role =
                segment.ccw ? exact::ExactSourceRole::swept_left_offset_arc
                            : exact::ExactSourceRole::swept_right_offset_arc;
            const exact::ExactSourceRole outer_role =
                segment.ccw ? exact::ExactSourceRole::swept_right_offset_arc
                            : exact::ExactSourceRole::swept_left_offset_arc;
            add_curve({exact::ExactAtomicCurveKind::circular_arc,
                       start_inner,
                       end_inner,
                       inner_circle,
                       segment.ccw,
                       segment.major,
                       {}},
                      segment.ccw, !segment.ccw, inner_role, offset_key, false);
            add_curve({exact::ExactAtomicCurveKind::circular_arc,
                       start_outer,
                       end_outer,
                       outer_circle,
                       segment.ccw,
                       segment.major,
                       {}},
                      segment.ccw, segment.ccw, outer_role, offset_key, false);
            const auto radial_agrees = [](const BigInt& radial_x, const BigInt& radial_y)
            { return radial_x != 0 ? radial_x > 0 : radial_y > 0; };
            add_curve(
                {exact::ExactAtomicCurveKind::line, start_inner, start_outer, {}, true, false, {}},
                radial_agrees(BigInt(segment.sx) - segment.cx, BigInt(segment.sy) - segment.cy),
                segment.ccw, exact::ExactSourceRole::none, 0, true);
            add_curve(
                {exact::ExactAtomicCurveKind::line, end_inner, end_outer, {}, true, false, {}},
                radial_agrees(BigInt(segment.ex) - segment.cx, BigInt(segment.ey) - segment.cy),
                !segment.ccw, exact::ExactSourceRole::none, 0, true);
        }

        if (!atomize_swept_pieces(piece_curves))
            return false;

        const exact::ExactArrangementResult arrangement =
            build_exact_arrangement(arena_, piece_curves, piece_coverages);
        if (arrangement.error != exact::Error::none || !arrangement.value)
        {
            return fail(arrangement.error == exact::Error::resource_limit_exceeded
                            ? AnalyticOperandLoweringError::resource_limit_exceeded
                            : AnalyticOperandLoweringError::invalid_topology);
        }
        return emit_swept_union_boundary(operand, swept, *arrangement.value, piece_coverages,
                                         piece_sources);
    }

    // Re-emits the union boundary of the swept operand's local
    // pre-arrangement as this job's occurrences: every arrangement edge with
    // material on exactly one side survives, attributed to the
    // lowest-numbered non-construction local occurrence whose piece covers
    // the material side.
    [[nodiscard]] bool
    emit_swept_union_boundary(const AnalyticRequestOperandRecord& operand,
                              const AnalyticRequestSweptPathRecord& swept,
                              const exact::ExactArrangement& arrangement,
                              const std::vector<exact::ExactCoverageOccurrence>& piece_coverages,
                              const std::vector<SweptPieceSource>& piece_sources)
    {
        constexpr std::uint32_t kNoHalfEdge = std::numeric_limits<std::uint32_t>::max();
        const std::vector<exact::ExactArrangementEdge>& edges = arrangement.edges();
        const std::vector<exact::ExactArrangementHalfEdge>& half_edges = arrangement.half_edges();
        std::vector<std::uint32_t> forward_half(edges.size(), kNoHalfEdge);
        std::vector<std::uint32_t> reverse_half(edges.size(), kNoHalfEdge);
        for (std::uint32_t index = 0; index < half_edges.size(); ++index)
            (half_edges[index].forward ? forward_half : reverse_half)[half_edges[index].edge] =
                index;
        const std::vector<exact::ExactArrangementFace>& faces = arrangement.faces();
        const std::vector<std::uint64_t>& face_coverages = arrangement.face_coverages();

        for (std::uint32_t edge_index = 0; edge_index < edges.size(); ++edge_index)
        {
            const exact::ExactArrangementEdge& edge = edges[edge_index];
            const std::uint32_t forward = forward_half[edge_index];
            const std::uint32_t reverse = reverse_half[edge_index];
            if (forward == kNoHalfEdge || reverse == kNoHalfEdge)
            {
                return fail(AnalyticOperandLoweringError::invalid_topology);
            }
            // The forward half-edge's face lies on the left of the edge's
            // stored start-to-end direction.
            const bool left_covered = faces[half_edges[forward].face].coverage_count > 0;
            const bool right_covered = faces[half_edges[reverse].face].coverage_count > 0;
            if (left_covered == right_covered)
                continue;
            const exact::ExactArrangementFace& material_face =
                faces[left_covered ? half_edges[forward].face : half_edges[reverse].face];

            const SweptPieceSource* chosen = nullptr;
            std::uint64_t chosen_occurrence = 0;
            for (std::uint32_t offset = 0; offset < edge.membership_count; ++offset)
            {
                const exact::ExactCurveMembership& membership =
                    arrangement.memberships()[edge.membership_begin + offset];
                const std::size_t local = static_cast<std::size_t>(membership.occurrence_id) - 1;
                if (piece_sources[local].construction)
                    continue;
                const std::uint64_t coverage = piece_coverages[local].coverage_id;
                bool covers = false;
                for (std::uint32_t c = 0; c < material_face.coverage_count; ++c)
                    if (face_coverages[material_face.coverage_begin + c] == coverage)
                    {
                        covers = true;
                        break;
                    }
                if (!covers)
                    continue;
                if (chosen == nullptr || membership.occurrence_id < chosen_occurrence)
                {
                    chosen = &piece_sources[local];
                    chosen_occurrence = membership.occurrence_id;
                }
            }
            if (chosen == nullptr)
            {
                return fail(AnalyticOperandLoweringError::invalid_topology);
            }

            exact::ExactAtomicCurve curve;
            curve.kind = edge.kind;
            curve.start = arrangement.vertices()[edge.start_vertex].point;
            curve.end = arrangement.vertices()[edge.end_vertex].point;
            curve.circle = edge.circle;
            curve.counterclockwise = edge.counterclockwise;
            curve.major_arc = edge.major_arc;
            bool agrees = edge.counterclockwise;
            if (edge.kind == exact::ExactAtomicCurveKind::line)
            {
                exact::ConstructionBuilder builder(arena_);
                std::int8_t order = builder.compare(curve.end.x, curve.start.x);
                if (order == 0)
                    order = builder.compare(curve.end.y, curve.start.y);
                if (!builder_ok(builder))
                    return false;
                agrees = order > 0;
            }
            if (!emit(std::move(curve), agrees, left_covered, operand.operand_id,
                      {exact::ExactSourceKind::compact_feature_role, chosen->role,
                       operand.operand_id, swept.feature_id, chosen->secondary_id}))
                return false;
        }
        return true;
    }

    [[nodiscard]] bool validate_segment(const AnalyticRequestSegmentRecord& record,
                                        SegmentGeometry& segment)
    {
        if (segment.sx == segment.ex && segment.sy == segment.ey)
            return fail(AnalyticOperandLoweringError::invalid_topology);
        if (record.kind == 1)
            return true;
        if (record.kind != 2)
            return fail(AnalyticOperandLoweringError::unsupported_geometry);
        segment.is_arc = true;
        segment.ccw = record.direction == 1;
        segment.cx = record.center_x_nm;
        segment.cy = record.center_y_nm;
        const BigInt rsx = BigInt(segment.sx) - segment.cx;
        const BigInt rsy = BigInt(segment.sy) - segment.cy;
        const BigInt rex = BigInt(segment.ex) - segment.cx;
        const BigInt rey = BigInt(segment.ey) - segment.cy;
        segment.squared_radius = rsx * rsx + rsy * rsy;
        if (segment.squared_radius == 0 || segment.squared_radius != rex * rex + rey * rey)
            return fail(AnalyticOperandLoweringError::invalid_arc);
        // The traversal direction already selects the branch, so the authored
        // major flag must match the branch it derives.
        const BigInt turn = cross(rsx, rsy, rex, rey);
        const bool derived_major = segment.ccw ? turn < 0 : turn > 0;
        segment.major = record.major_arc;
        if (segment.major != derived_major)
            return fail(AnalyticOperandLoweringError::invalid_arc);
        return true;
    }

    // True when the leftmost point of the arc's full circle lies strictly
    // inside the arc's traversed open angular domain.
    [[nodiscard]] static bool arc_interior_contains_leftmost(const SegmentGeometry& segment)
    {
        BigInt rsx = BigInt(segment.sx) - segment.cx;
        BigInt rsy = BigInt(segment.sy) - segment.cy;
        BigInt rex = BigInt(segment.ex) - segment.cx;
        BigInt rey = BigInt(segment.ey) - segment.cy;
        if (!segment.ccw)
        {
            std::swap(rsx, rex);
            std::swap(rsy, rey);
        }
        const BigInt turn = cross(rsx, rsy, rex, rey);
        const bool start_side = rsy > 0;
        const bool end_side = rey < 0;
        if (turn > 0)
            return start_side && end_side;
        if (turn == 0)
            return start_side;
        const bool complement_interior = rey > 0 && rsy < 0;
        const bool at_start = rsy == 0 && rsx < 0;
        const bool at_end = rey == 0 && rex < 0;
        return !complement_interior && !at_start && !at_end;
    }

    [[nodiscard]] bool lower_authored_ring(const AnalyticRequestRingRecord& ring, bool is_hole,
                                           std::uint64_t coverage_id)
    {
        std::vector<SegmentGeometry> segments(ring.segment_count);
        for (std::uint32_t index = 0; index < ring.segment_count; ++index)
        {
            const AnalyticRequestVertexRecord& start = records_.vertices[ring.vertex_begin + index];
            const AnalyticRequestVertexRecord& end =
                records_.vertices[ring.vertex_begin + (index + 1) % ring.vertex_count];
            SegmentGeometry& segment = segments[index];
            segment.sx = start.x_nm;
            segment.sy = start.y_nm;
            segment.ex = end.x_nm;
            segment.ey = end.y_nm;
            include_span(segment.sx, segment.sy);
            if (!validate_segment(records_.segments[ring.segment_begin + index], segment))
                return false;
            if (segment.is_arc)
                include_span(segment.cx, segment.cy);
        }

        bool ring_ccw = false;
        if (!resolve_ring_orientation(segments, ring_ccw))
            return false;
        const bool material_on_left = is_hole ? !ring_ccw : ring_ccw;

        for (std::uint32_t index = 0; index < ring.segment_count; ++index)
        {
            const AnalyticRequestSegmentRecord& record =
                records_.segments[ring.segment_begin + index];
            const SegmentGeometry& segment = segments[index];
            exact::ExactAtomicCurve curve;
            if (!point(segment.sx, segment.sy, curve.start) ||
                !point(segment.ex, segment.ey, curve.end))
                return false;
            exact::ExactSourceReference source{exact::ExactSourceKind::authored_segment_curve,
                                               exact::ExactSourceRole::authored_line, coverage_id,
                                               record.id, record.curve_id};
            bool agrees_with_carrier =
                segment.ex != segment.sx ? segment.ex > segment.sx : segment.ey > segment.sy;
            if (segment.is_arc)
            {
                curve.kind = exact::ExactAtomicCurveKind::circular_arc;
                curve.counterclockwise = segment.ccw;
                curve.major_arc = segment.major;
                agrees_with_carrier = segment.ccw;
                if (!point(segment.cx, segment.cy, curve.circle.center) ||
                    !square_root_node(segment.squared_radius, curve.circle.radius))
                    return false;
                source.role = exact::ExactSourceRole::authored_circular_arc;
            }
            if (!emit(std::move(curve), agrees_with_carrier, material_on_left, coverage_id, source))
                return false;
        }
        return true;
    }

    // Resolves the authored winding with exact predicates: locate the
    // lexicographically least (x, then y) point of the ring, which is either
    // an authored vertex or the leftmost circle point inside an arc, then
    // read the local travel direction there. At a smooth arc minimum the
    // ring is counterclockwise exactly when the arc travels clockwise-free
    // downward, i.e. the traversal direction itself; at a vertex minimum the
    // exact tangent turn decides, with the vertical collinear case decided
    // by travel direction.
    [[nodiscard]] bool resolve_ring_orientation(const std::vector<SegmentGeometry>& segments,
                                                bool& ring_ccw)
    {
        std::size_t best_vertex = 0;
        for (std::size_t index = 1; index < segments.size(); ++index)
        {
            const SegmentGeometry& candidate = segments[index];
            const SegmentGeometry& best = segments[best_vertex];
            if (candidate.sx < best.sx || (candidate.sx == best.sx && candidate.sy < best.sy))
                best_vertex = index;
        }

        bool best_is_arc = false;
        std::size_t best_arc = 0;
        exact::ConstructionBuilder builder(arena_);
        exact::ConstructionNodeId best_x = 0;
        exact::ConstructionNodeId best_y = 0;
        if (!node(BigInt(segments[best_vertex].sx), best_x) ||
            !node(BigInt(segments[best_vertex].sy), best_y))
            return false;
        for (std::size_t index = 0; index < segments.size(); ++index)
        {
            const SegmentGeometry& segment = segments[index];
            if (!segment.is_arc || !arc_interior_contains_leftmost(segment))
                continue;
            exact::ConstructionNodeId center_x = 0;
            exact::ConstructionNodeId radius = 0;
            exact::ConstructionNodeId candidate_y = 0;
            if (!node(BigInt(segment.cx), center_x) ||
                !square_root_node(segment.squared_radius, radius) ||
                !node(BigInt(segment.cy), candidate_y))
                return false;
            const exact::ConstructionNodeId candidate_x = builder.subtract(center_x, radius);
            std::int8_t order = builder.compare(candidate_x, best_x);
            if (order == 0)
                order = builder.compare(candidate_y, best_y);
            if (!builder.good())
                return fail(AnalyticOperandLoweringError::resource_limit_exceeded);
            if (order < 0)
            {
                best_is_arc = true;
                best_arc = index;
                best_x = candidate_x;
                best_y = candidate_y;
            }
        }

        if (best_is_arc)
        {
            ring_ccw = segments[best_arc].ccw;
            return true;
        }
        return vertex_orientation(segments, best_vertex, ring_ccw);
    }

    [[nodiscard]] bool vertex_orientation(const std::vector<SegmentGeometry>& segments,
                                          std::size_t vertex, bool& ring_ccw)
    {
        const SegmentGeometry& outgoing = segments[vertex];
        const SegmentGeometry& incoming =
            segments[(vertex + segments.size() - 1) % segments.size()];
        BigInt out_x;
        BigInt out_y;
        tangent(outgoing, true, out_x, out_y);
        BigInt in_x;
        BigInt in_y;
        tangent(incoming, false, in_x, in_y);
        const BigInt turn = cross(in_x, in_y, out_x, out_y);
        if (turn != 0)
        {
            ring_ccw = turn > 0;
            return true;
        }
        if (dot(in_x, in_y, out_x, out_y) <= 0 || out_y == 0)
            return fail(AnalyticOperandLoweringError::invalid_topology);
        ring_ccw = out_y < 0;
        return true;
    }

    // Exact (unnormalized) travel tangent of a segment at its start
    // (at_start) or end vertex; arcs use the perpendicular of the radius
    // vector oriented by travel direction.
    static void tangent(const SegmentGeometry& segment, bool at_start, BigInt& x, BigInt& y)
    {
        if (!segment.is_arc)
        {
            x = BigInt(segment.ex) - segment.sx;
            y = BigInt(segment.ey) - segment.sy;
            return;
        }
        const BigInt radial_x = BigInt(at_start ? segment.sx : segment.ex) - segment.cx;
        const BigInt radial_y = BigInt(at_start ? segment.sy : segment.ey) - segment.cy;
        if (segment.ccw)
        {
            x = -radial_y;
            y = radial_x;
        }
        else
        {
            x = radial_y;
            y = -radial_x;
        }
    }

    exact::ConstructionArena& arena_;
    const AnalyticRequestPacketRecords& records_;
    AnalyticLoweredGeometry out_;
    AnalyticOperandLoweringError error_ = AnalyticOperandLoweringError::none;
    std::uint64_t next_occurrence_ = 1;
    bool has_bounds_ = false;
    BigInt min_x_;
    BigInt max_x_;
    BigInt min_y_;
    BigInt max_y_;
};

} // namespace

AnalyticLoweredGeometryResult
lower_analytic_job_geometry(exact::ConstructionArena& arena,
                            const AnalyticRequestPacketRecords& records, std::uint32_t job_index)
{
    return JobLowerer(arena, records).lower(job_index);
}

} // namespace geometer

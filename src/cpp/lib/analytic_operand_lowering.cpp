#include "geometer/analytic_operand_lowering.h"

#include "geometer/exact_construction.h"

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
        include_span_big(BigInt(x), BigInt(y));
    }

    void include_span_big(const BigInt& x, const BigInt& y)
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
        const BigInt limit(kMaximumSpanNm);
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
            const BigInt radius(disk.radius_nm);
            include_span_big(BigInt(disk.center_x_nm) - radius, BigInt(disk.center_y_nm) - radius);
            include_span_big(BigInt(disk.center_x_nm) + radius, BigInt(disk.center_y_nm) + radius);
            return lower_full_circle(disk.center_x_nm, disk.center_y_nm, disk.radius_nm, true,
                                     operand.operand_id,
                                     {exact::ExactSourceKind::compact_feature_role,
                                      exact::ExactSourceRole::primitive_outer_circle,
                                      operand.operand_id, disk.feature_id, 0});
        }
        case 3:
        {
            const AnalyticRequestAnnulusRecord& annulus = records_.annuli[operand.geometry_index];
            const BigInt outer(annulus.outer_radius_nm);
            include_span_big(BigInt(annulus.center_x_nm) - outer,
                             BigInt(annulus.center_y_nm) - outer);
            include_span_big(BigInt(annulus.center_x_nm) + outer,
                             BigInt(annulus.center_y_nm) + outer);
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
        default:
            // Capsules and swept paths are lowered by a later slice.
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

    [[nodiscard]] bool validate_segment(const AnalyticRequestSegmentRecord& record,
                                        SegmentGeometry& segment)
    {
        if (segment.sx == segment.ex && segment.sy == segment.ey)
            return fail(AnalyticOperandLoweringError::invalid_topology);
        if (record.kind != 2)
            return true;
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

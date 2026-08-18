#pragma once

// Internal swept-path validation, tangent preparation, and primitive setup phases.
SweptLowerer(const AnalyticRequestPacketRecords& records,
             const AnalyticRequestOperandRecord& operand, std::int64_t origin_x_nm,
             std::int64_t origin_y_nm, AnalyticSolverLimits limits)
    : records_(records), operand_(operand), origin_x_nm_(origin_x_nm), origin_y_nm_(origin_y_nm),
      limits_(limits)
{
}

SweptPathLoweringResult lower()
{
    if (!analytic_solver_limits_within_hard_ceilings(limits_) || operand_.geometry_kind != 5 ||
        operand_.geometry_index >= records_.swept_paths.size())
        return failure(AnalyticFilteredLoweringError::resource_limit_exceeded);
    swept_ = &records_.swept_paths[operand_.geometry_index];
    if (swept_->path_ring >= records_.rings.size())
        return failure(AnalyticFilteredLoweringError::invalid_topology);
    ring_ = &records_.rings[swept_->path_ring];
    if (ring_->segment_count == 0 || ring_->vertex_count != ring_->segment_count + 1)
        return failure(AnalyticFilteredLoweringError::invalid_topology);
    try
    {
        if (!read_segments())
            return failure(error_);
        if (!validate_centerline())
            return failure(error_);
        if (!release_centerline())
            return failure(error_);
        if (!prepare_tangent_points())
            return failure(error_);
        if (!build_piece_geometry())
            return failure(error_);
        if (!assign_tokens())
            return failure(error_);
        if (!run_local_union())
            return failure(error_);
    }
    catch (const std::bad_alloc&)
    {
        telemetry_.required_working_memory_bytes = limits_.working_memory_bytes + 1;
        return failure(AnalyticFilteredLoweringError::resource_limit_exceeded);
    }
    telemetry_.emitted_curves = output_.size();
    telemetry_.retained_geometry_bytes = output_capacity_bytes_;
    return {AnalyticFilteredLoweringError::none, std::move(output_), std::move(output_tangencies_),
            telemetry_};
}

private:
SweptPathLoweringResult failure(AnalyticFilteredLoweringError error)
{
    return {error, {}, {}, telemetry_};
}

bool fail(AnalyticFilteredLoweringError error)
{
    error_ = error;
    return false;
}

bool fail_memory(std::uint64_t required)
{
    telemetry_.required_working_memory_bytes =
        std::max(telemetry_.required_working_memory_bytes, required);
    return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
}

bool charge(std::uint64_t units = 1)
{
    if (telemetry_.work_units > limits_.predicate_calls ||
        units > limits_.predicate_calls - telemetry_.work_units)
        return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
    telemetry_.work_units += units;
    return true;
}

bool local_point(std::int64_t x, std::int64_t y, AnalyticIntegerPointNm& result) const
{
    if (x < origin_x_nm_ || y < origin_y_nm_)
        return false;
    const std::uint64_t dx =
        static_cast<std::uint64_t>(x) - static_cast<std::uint64_t>(origin_x_nm_);
    const std::uint64_t dy =
        static_cast<std::uint64_t>(y) - static_cast<std::uint64_t>(origin_y_nm_);
    if (dx > static_cast<std::uint64_t>(kMaximumSpanNm) ||
        dy > static_cast<std::uint64_t>(kMaximumSpanNm))
        return false;
    result = {static_cast<std::int64_t>(dx), static_cast<std::int64_t>(dy)};
    return true;
}

bool exact_integer_root(WideInteger squared, double approximation, std::uint64_t& root)
{
    const std::uint64_t center = static_cast<std::uint64_t>(approximation);
    const std::uint64_t first = center > 4 ? center - 4 : 0;
    for (std::uint64_t value = first; value <= center + 4; ++value)
    {
        if (!charge())
            return false;
        ++telemetry_.fixed_width_predicates;
        if (value <= static_cast<std::uint64_t>(kMaximumSpanNm) &&
            wide_compare(
                wide_multiply(static_cast<std::int64_t>(value), static_cast<std::int64_t>(value)),
                squared) == 0)
        {
            root = value;
            return true;
        }
    }
    return true;
}

bool read_segments()
{
    if (!charge(ring_->vertex_count + ring_->segment_count))
        return false;
    const std::uint64_t segment_bytes = ring_->segment_count * kSegmentBytes;
    if (!retain(segment_bytes))
        return false;
    segments_.resize(ring_->segment_count);
    for (std::uint32_t index = 0; index < ring_->segment_count; ++index)
    {
        const auto& start_record = records_.vertices[ring_->vertex_begin + index];
        const auto& end_record = records_.vertices[ring_->vertex_begin + index + 1];
        const auto& source = records_.segments[ring_->segment_begin + index];
        Segment& segment = segments_[index];
        if (!local_point(start_record.x_nm, start_record.y_nm, segment.start) ||
            !local_point(end_record.x_nm, end_record.y_nm, segment.end))
            return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
        if (segment.start.x == segment.end.x && segment.start.y == segment.end.y)
            return fail(AnalyticFilteredLoweringError::invalid_topology);
        if (source.kind == 1)
            continue;
        if (source.kind != 2 ||
            !local_point(source.center_x_nm, source.center_y_nm, segment.center))
            return fail(AnalyticFilteredLoweringError::invalid_arc);
        segment.arc = true;
        segment.counterclockwise = source.direction == 1;
        segment.major = source.major_arc;
        segment.radius_squared = squared_distance(segment.start, segment.center);
        if (!charge(2))
            return false;
        telemetry_.fixed_width_predicates += 2;
        if (wide_sign(segment.radius_squared) == 0 ||
            wide_compare(segment.radius_squared, squared_distance(segment.end, segment.center)) !=
                0)
            return fail(AnalyticFilteredLoweringError::invalid_arc);
        const WideInteger width_squared =
            wide_multiply(static_cast<std::int64_t>(swept_->width_nm),
                          static_cast<std::int64_t>(swept_->width_nm));
        const WideInteger four_radius =
            wide_add(wide_add(segment.radius_squared, segment.radius_squared),
                     wide_add(segment.radius_squared, segment.radius_squared));
        if (wide_compare(four_radius, width_squared) <= 0)
            return fail(AnalyticFilteredLoweringError::invalid_arc);
        if (!charge())
            return false;
        ++telemetry_.fixed_width_predicates;
        const int turn = wide_sign(
            cross_vectors(segment.start.x - segment.center.x, segment.start.y - segment.center.y,
                          segment.end.x - segment.center.x, segment.end.y - segment.center.y));
        if (turn == 0)
        {
            const int radial_dot = wide_sign(
                dot_vectors(segment.start.x - segment.center.x, segment.start.y - segment.center.y,
                            segment.end.x - segment.center.x, segment.end.y - segment.center.y));
            if (radial_dot >= 0 || segment.major)
                return fail(AnalyticFilteredLoweringError::invalid_arc);
            continue;
        }
        const bool derived_major = segment.counterclockwise ? turn < 0 : turn > 0;
        if (segment.major != derived_major)
            return fail(AnalyticFilteredLoweringError::invalid_arc);
    }
    for (std::uint32_t index = 0; index + 1 < segments_.size(); ++index)
    {
        std::int64_t ax = 0;
        std::int64_t ay = 0;
        std::int64_t bx = 0;
        std::int64_t by = 0;
        tangent(segments_[index], false, ax, ay);
        tangent(segments_[index + 1], true, bx, by);
        if (!charge(2))
            return false;
        telemetry_.fixed_width_predicates += 2;
        if (wide_sign(cross_vectors(ax, ay, bx, by)) == 0 &&
            wide_sign(dot_vectors(ax, ay, bx, by)) < 0)
            return fail(AnalyticFilteredLoweringError::invalid_topology);
    }
    return true;
}

bool legal_adjacent_same_carrier(std::uint32_t first, std::uint32_t second) const noexcept
{
    if (second != first + 1U || second >= segments_.size())
        return false;
    const Segment& left = segments_[first];
    const Segment& right = segments_[second];
    if (left.end.x != right.start.x || left.end.y != right.start.y || left.arc != right.arc)
        return false;
    std::int64_t left_x = 0;
    std::int64_t left_y = 0;
    std::int64_t right_x = 0;
    std::int64_t right_y = 0;
    tangent(left, false, left_x, left_y);
    tangent(right, true, right_x, right_y);
    if (wide_sign(cross_vectors(left_x, left_y, right_x, right_y)) != 0 ||
        wide_sign(dot_vectors(left_x, left_y, right_x, right_y)) <= 0)
        return false;
    if (!left.arc)
        return wide_sign(cross_vectors(left.end.x - left.start.x, left.end.y - left.start.y,
                                       right.end.x - right.start.x, right.end.y - right.start.y)) ==
               0;
    if (left.counterclockwise != right.counterclockwise || left.center.x != right.center.x ||
        left.center.y != right.center.y ||
        wide_compare(left.radius_squared, right.radius_squared) != 0 ||
        (left.start.x == right.end.x && left.start.y == right.end.y))
        return false;
    return !point_in_open_arc(left, right.end) && !point_in_open_arc(right, left.start);
}

static bool point_in_open_arc(const Segment& arc, AnalyticIntegerPointNm candidate) noexcept
{
    if (!arc.arc || (candidate.x == arc.start.x && candidate.y == arc.start.y) ||
        (candidate.x == arc.end.x && candidate.y == arc.end.y))
        return false;
    const std::int64_t start_x = arc.start.x - arc.center.x;
    const std::int64_t start_y = arc.start.y - arc.center.y;
    const std::int64_t value_x = candidate.x - arc.center.x;
    const std::int64_t value_y = candidate.y - arc.center.y;
    const std::int64_t end_x = arc.end.x - arc.center.x;
    const std::int64_t end_y = arc.end.y - arc.center.y;
    if (wide_compare(arc.radius_squared, wide_add(wide_multiply(value_x, value_x),
                                                  wide_multiply(value_y, value_y))) != 0)
        return false;
    const int from_start = wide_sign(cross_vectors(start_x, start_y, value_x, value_y));
    const int to_end = wide_sign(cross_vectors(value_x, value_y, end_x, end_y));
    if (arc.counterclockwise)
        return arc.major ? from_start > 0 || to_end > 0 : from_start > 0 && to_end > 0;
    return arc.major ? from_start < 0 || to_end < 0 : from_start < 0 && to_end < 0;
}

static void tangent(const Segment& segment, bool at_start, std::int64_t& x,
                    std::int64_t& y) noexcept
{
    if (!segment.arc)
    {
        x = segment.end.x - segment.start.x;
        y = segment.end.y - segment.start.y;
        return;
    }
    const auto endpoint = at_start ? segment.start : segment.end;
    const std::int64_t rx = endpoint.x - segment.center.x;
    const std::int64_t ry = endpoint.y - segment.center.y;
    x = segment.counterclockwise ? -ry : ry;
    y = segment.counterclockwise ? rx : -rx;
}

TokenDescriptor exact_line_descriptor(AnalyticIntegerPointNm start,
                                      AnalyticIntegerPointNm end) const
{
    return exact_line_descriptor(start, end.x - start.x, end.y - start.y);
}

TokenDescriptor exact_line_descriptor(AnalyticIntegerPointNm point_value, std::int64_t dx,
                                      std::int64_t dy) const
{
    const LineFamilyKey family = canonical_direction(dx, dy);
    const WideInteger offset = wide_subtract(wide_multiply(family.dx, point_value.y),
                                             wide_multiply(family.dy, point_value.x));
    TokenDescriptor result;
    result.line = {family, wide_add(offset, offset), 0};
    return result;
}

TokenDescriptor circle_descriptor(AnalyticIntegerPointNm center,
                                  WideInteger radius_squared_times_four) const
{
    TokenDescriptor result;
    result.kind = TokenKeyKind::circle;
    result.circle = {{center.x, center.y}, radius_squared_times_four, 0, {}};
    return result;
}

TokenDescriptor offset_circle_descriptor(AnalyticIntegerPointNm center, WideInteger radius_squared,
                                         std::int64_t width, bool outer,
                                         std::uint64_t integer_radius) const
{
    TokenDescriptor result;
    result.kind = TokenKeyKind::circle;
    result.circle.family = {center.x, center.y};
    if (integer_radius != 0)
    {
        const std::int64_t doubled =
            static_cast<std::int64_t>(integer_radius) * 2 + (outer ? width : -width);
        result.circle.rational_part = wide_multiply(doubled, doubled);
        return result;
    }
    const WideInteger width_squared = wide_multiply(width, width);
    result.circle.rational_part =
        wide_add(wide_add(radius_squared, radius_squared),
                 wide_add(wide_add(radius_squared, radius_squared), width_squared));
    result.circle.radical_coefficient = outer ? width * 4 : -width * 4;
    result.circle.radicand = radius_squared;
    return result;
}

bool append_centerline_curve(const Segment& segment, std::uint32_t coverage)
{
    AnalyticAtomicCurveNm curve;
    curve.curve_index = static_cast<std::uint32_t>(centerline_.curves.size() + 1);
    curve.start = public_point(point(segment.start));
    curve.end = public_point(point(segment.end));
    curve.has_integer_certificate = true;
    curve.integer_start = segment.start;
    curve.integer_end = segment.end;
    TokenDescriptor descriptor = exact_line_descriptor(segment.start, segment.end);
    if (segment.arc)
    {
        curve.kind = AnalyticAtomicCurveKind::circular_arc;
        curve.counterclockwise = segment.counterclockwise;
        curve.major_arc = segment.major;
        curve.circle.center = public_point(point(segment.center));
        ++telemetry_.square_root_calls;
        const Point radial = subtract(point(segment.start), point(segment.center));
        Interval radius = square_root(dot(radial, radial));
        std::uint64_t integer_radius = 0;
        if (!exact_integer_root(segment.radius_squared, radius.upper, integer_radius))
            return false;
        if (integer_radius != 0)
        {
            radius = exact(static_cast<double>(integer_radius));
            curve.has_integer_radius_certificate = true;
            curve.integer_radius = integer_radius;
        }
        curve.integer_center = segment.center;
        curve.circle.radius = public_interval(radius);
        descriptor = circle_descriptor(
            segment.center, wide_add(wide_add(segment.radius_squared, segment.radius_squared),
                                     wide_add(segment.radius_squared, segment.radius_squared)));
    }
    centerline_.curves.push_back(curve);
    centerline_descriptors_.push_back(descriptor);
    centerline_.bounds.push_back(bounds_for(curve));
    centerline_.occurrences.push_back(
        {curve.curve_index,
         coverage,
         true,
         true,
         {AnalyticFilteredSourceKind::authored_segment_curve,
          segment.arc ? AnalyticFilteredSourceRole::authored_circular_arc
                      : AnalyticFilteredSourceRole::authored_line,
          coverage, coverage, coverage}});
    return true;
}

AnalyticCurveBoundsNm bounds_for(const AnalyticAtomicCurveNm& curve) const
{
    AnalyticCurveBoundsNm result;
    result.curve_index = curve.curve_index;
    result.min_x = std::min(curve.start.x.lower, curve.end.x.lower);
    result.min_y = std::min(curve.start.y.lower, curve.end.y.lower);
    result.max_x = std::max(curve.start.x.upper, curve.end.x.upper);
    result.max_y = std::max(curve.start.y.upper, curve.end.y.upper);
    if (curve.kind == AnalyticAtomicCurveKind::circular_arc)
    {
        result.min_x =
            std::min(result.min_x, curve.circle.center.x.lower - curve.circle.radius.upper);
        result.min_y =
            std::min(result.min_y, curve.circle.center.y.lower - curve.circle.radius.upper);
        result.max_x =
            std::max(result.max_x, curve.circle.center.x.upper + curve.circle.radius.upper);
        result.max_y =
            std::max(result.max_y, curve.circle.center.y.upper + curve.circle.radius.upper);
    }
    return result;
}

bool validate_centerline()
{
    centerline_.origin_x_nm = origin_x_nm_;
    centerline_.origin_y_nm = origin_y_nm_;
    centerline_bytes_ = segments_.size() * (kPieceCurveBytes + kPieceCoverageBytes);
    if (!retain(centerline_bytes_))
        return false;
    centerline_.curves.reserve(segments_.size());
    centerline_.bounds.reserve(segments_.size());
    centerline_.occurrences.reserve(segments_.size());
    centerline_descriptors_.reserve(segments_.size());
    for (std::uint32_t index = 0; index < segments_.size(); ++index)
        if (!append_centerline_curve(segments_[index], index + 1))
            return false;
    if (!assign_tokens(centerline_, centerline_descriptors_, {}))
        return false;

    AnalyticSolverLimits phase_limits = remaining_limits();
    const AnalyticBroadPhaseResult broad = analytic_execution_detail::build_curve_candidates(
        centerline_.bounds, phase_limits,
        analytic_execution_detail::TopologyPolicy::resolution_50nm);
    add_phase(broad.telemetry.work_units, broad.telemetry.peak_working_memory_bytes,
              broad.telemetry.required_working_memory_bytes);
    if (broad.error != AnalyticBroadPhaseError::none)
    {
        return fail(broad.error == AnalyticBroadPhaseError::invalid_argument
                        ? AnalyticFilteredLoweringError::invalid_topology
                        : AnalyticFilteredLoweringError::resource_limit_exceeded);
    }
    if (!retain(broad.telemetry.retained_pair_bytes))
        return false;
    centerline_pair_bytes_ = broad.telemetry.retained_pair_bytes;
    phase_limits = remaining_limits();
    const AnalyticNarrowPhaseResult narrow = analytic_execution_detail::intersect_curve_candidates(
        centerline_.curves, broad.pairs, phase_limits,
        analytic_execution_detail::TopologyPolicy::resolution_50nm);
    add_phase(narrow.telemetry.predicate_calls, narrow.telemetry.peak_working_memory_bytes,
              narrow.telemetry.required_working_memory_bytes);
    if (narrow.error != AnalyticNarrowPhaseError::none)
        return fail(narrow.error == AnalyticNarrowPhaseError::invalid_argument
                        ? AnalyticFilteredLoweringError::invalid_topology
                        : AnalyticFilteredLoweringError::resource_limit_exceeded);
    if (!charge(narrow.intersections.size()))
        return false;
    for (const AnalyticPairIntersection& intersection : narrow.intersections)
    {
        if (intersection.relation == AnalyticPairRelation::disjoint)
            continue;
        const std::uint32_t first = intersection.pair.first - 1;
        const std::uint32_t second = intersection.pair.second - 1;
        if (second == first + 1U && intersection.relation == AnalyticPairRelation::point &&
            intersection.point_count == 1 &&
            point_contains(intersection.points[0], segments_[first].end))
            continue;
        if (intersection.relation == AnalyticPairRelation::coincident &&
            legal_adjacent_same_carrier(first, second))
            continue;
        return fail(AnalyticFilteredLoweringError::invalid_topology);
    }
    retained_piece_bytes_ -= centerline_pair_bytes_;
    centerline_pair_bytes_ = 0;
    return true;
}

bool release_centerline()
{
    if (centerline_pair_bytes_ != 0 || centerline_bytes_ > retained_piece_bytes_)
        return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
    retained_piece_bytes_ -= centerline_bytes_;
    centerline_bytes_ = 0;
    AnalyticFilteredGeometry{}.curves.swap(centerline_.curves);
    AnalyticFilteredGeometry{}.bounds.swap(centerline_.bounds);
    AnalyticFilteredGeometry{}.occurrences.swap(centerline_.occurrences);
    std::vector<TokenDescriptor>().swap(centerline_descriptors_);
    return true;
}

static bool point_contains(const AnalyticFilteredPointNm& value,
                           AnalyticIntegerPointNm point_value) noexcept
{
    const double x = static_cast<double>(point_value.x);
    const double y = static_cast<double>(point_value.y);
    return value.x.lower <= x && x <= value.x.upper && value.y.lower <= y && y <= value.y.upper;
}

AnalyticSolverLimits remaining_limits() const noexcept
{
    AnalyticSolverLimits result = limits_;
    result.predicate_calls -= std::min(result.predicate_calls, telemetry_.work_units);
    result.working_memory_bytes -= std::min(result.working_memory_bytes, retained_piece_bytes_);
    return result;
}

void add_phase(std::uint64_t work, std::uint64_t peak, std::uint64_t required)
{
    telemetry_.work_units += work;
    telemetry_.peak_working_memory_bytes =
        std::max(telemetry_.peak_working_memory_bytes, retained_piece_bytes_ + peak);
    if (required != 0)
        telemetry_.required_working_memory_bytes =
            std::max(telemetry_.required_working_memory_bytes, retained_piece_bytes_ + required);
}

bool retain(std::uint64_t bytes)
{
    if (bytes > limits_.working_memory_bytes -
                    std::min(limits_.working_memory_bytes, retained_piece_bytes_))
        return fail_memory(retained_piece_bytes_ + bytes);
    retained_piece_bytes_ += bytes;
    telemetry_.peak_working_memory_bytes =
        std::max(telemetry_.peak_working_memory_bytes, retained_piece_bytes_);
    return true;
}

bool selection_retained_bytes(const AnalyticFilteredBooleanSelectionResult& selection,
                              std::uint64_t& retained) const noexcept
{
    using namespace analytic_selection_detail;
    bool valid = true;
    const auto& arrangement = selection.arrangement;
    retained = checked_multiply(arrangement.vertices.size(), kAnalyticArrangementVertexLogicalBytes,
                                valid);
    retained = checked_add(
        retained,
        checked_multiply(arrangement.edges.size(), kAnalyticArrangementEdgeLogicalBytes, valid),
        valid);
    retained = checked_add(retained,
                           checked_multiply(arrangement.half_edges.size(),
                                            kAnalyticArrangementHalfEdgeLogicalBytes, valid),
                           valid);
    retained = checked_add(
        retained,
        checked_multiply(arrangement.outgoing_half_edges.size(), kIndexLogicalBytes, valid), valid);
    retained = checked_add(retained,
                           checked_multiply(arrangement.collapsed_spans.size(),
                                            kAnalyticArrangementCollapsedSpanLogicalBytes, valid),
                           valid);
    retained = checked_add(retained,
                           checked_multiply(arrangement.memberships.size(),
                                            kAnalyticOverlayMembershipLogicalBytes, valid),
                           valid);
    retained = checked_add(
        retained,
        checked_multiply(arrangement.cycles.size(), kAnalyticArrangementCycleLogicalBytes, valid),
        valid);
    retained = checked_add(
        retained, checked_multiply(arrangement.cycle_half_edges.size(), kIndexLogicalBytes, valid),
        valid);
    retained = checked_add(
        retained, checked_multiply(selection.occurrences.size(), kOccurrenceLogicalBytes, valid),
        valid);
    retained = checked_add(
        retained, checked_multiply(selection.half_edge_faces.size(), kIndexLogicalBytes, valid),
        valid);
    retained = checked_add(
        retained, checked_multiply(selection.faces.size(), kFaceLogicalBytes, valid), valid);
    retained = checked_add(
        retained,
        checked_multiply(selection.face_boundary_cycles.size(), kIndexLogicalBytes, valid), valid);
    retained = checked_add(
        retained,
        checked_multiply(selection.coverage_state_nodes.size(), kCoverageNodeLogicalBytes, valid),
        valid);
    retained = checked_add(
        retained,
        checked_multiply(selection.outcome_evidence.size(), kOutcomeEvidenceLogicalBytes, valid),
        valid);
    return valid;
}

bool prepare_tangent_points()
{
    const std::uint64_t bytes =
        segments_.size() * kSegmentTangentBytes + segments_.size() * 4U * kVertexRayBytes;
    if (!retain(bytes))
        return false;
    tangent_points_.resize(segments_.size());
    disk_rays_.reserve(segments_.size() * 4U);
    const Interval half_width = exact(static_cast<double>(swept_->width_nm) * 0.5);
    for (std::uint32_t index = 0; index < segments_.size(); ++index)
    {
        if (!charge(4))
            return false;
        const Segment& segment = segments_[index];
        SegmentTangentPoints& points = tangent_points_[index];
        std::array<std::pair<std::int64_t, std::int64_t>, 4> rays{};
        if (!segment.arc)
        {
            const std::int64_t dx = segment.end.x - segment.start.x;
            const std::int64_t dy = segment.end.y - segment.start.y;
            ++telemetry_.square_root_calls;
            const Interval length = square_root(add(square(exact(static_cast<double>(dx))),
                                                    square(exact(static_cast<double>(dy)))));
            const Interval factor = divide(half_width, length);
            Point offset{multiply(exact(static_cast<double>(-dy)), factor),
                         multiply(exact(static_cast<double>(dx)), factor)};
            const WideInteger length_squared =
                wide_add(wide_multiply(dx, dx), wide_multiply(dy, dy));
            std::uint64_t integer_length = 0;
            if (!exact_integer_root(length_squared, length.upper, integer_length))
                return false;
            const auto exact_offset = [&](std::int64_t direction, std::int64_t& value)
            {
                if (integer_length == 0 ||
                    integer_length >
                        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max() / 2))
                    return false;
                const std::int64_t denominator = static_cast<std::int64_t>(integer_length * 2);
                const std::int64_t width = static_cast<std::int64_t>(swept_->width_nm);
                if (direction != 0 &&
                    magnitude(direction) > static_cast<std::uint64_t>(
                                               std::numeric_limits<std::int64_t>::max() / width))
                    return false;
                const std::int64_t numerator = direction * width;
                if (numerator % denominator != 0)
                    return false;
                value = numerator / denominator;
                return true;
            };
            std::int64_t offset_x = 0;
            std::int64_t offset_y = 0;
            if (exact_offset(-dy, offset_x) && exact_offset(dx, offset_y))
                offset = {exact(static_cast<double>(offset_x)),
                          exact(static_cast<double>(offset_y))};
            const Point start = point(segment.start);
            const Point end = point(segment.end);
            points.points = {subtract(start, offset), add(start, offset), subtract(end, offset),
                             add(end, offset)};
            rays = {{{dy, -dx}, {-dy, dx}, {dy, -dx}, {-dy, dx}}};
        }
        else
        {
            const Point center = point(segment.center);
            const Point start = point(segment.start);
            const Point end = point(segment.end);
            const Point start_radial = subtract(start, center);
            const Point end_radial = subtract(end, center);
            ++telemetry_.square_root_calls;
            const Interval radius = square_root(dot(start_radial, start_radial));
            const Interval inner = subtract(radius, half_width);
            const Interval outer = add(radius, half_width);
            const auto radial_point = [&](Point endpoint, Point radial, Interval target)
            {
                return add(center, {multiply(radial.x, divide(target, radius)),
                                    multiply(radial.y, divide(target, radius))});
            };
            Point start_inner = radial_point(start, start_radial, inner);
            Point start_outer = radial_point(start, start_radial, outer);
            Point end_inner = radial_point(end, end_radial, inner);
            Point end_outer = radial_point(end, end_radial, outer);
            std::uint64_t integer_radius = 0;
            if (!exact_integer_root(segment.radius_squared, radius.upper, integer_radius))
                return false;
            if (!charge(8))
                return false;
            telemetry_.fixed_width_predicates += 8;
            AnalyticIntegerPointNm exact_point;
            if (exact_arc_offset_endpoint(segment, segment.start, integer_radius, false,
                                          exact_point))
                start_inner = point(exact_point);
            if (exact_arc_offset_endpoint(segment, segment.start, integer_radius, true,
                                          exact_point))
                start_outer = point(exact_point);
            if (exact_arc_offset_endpoint(segment, segment.end, integer_radius, false, exact_point))
                end_inner = point(exact_point);
            if (exact_arc_offset_endpoint(segment, segment.end, integer_radius, true, exact_point))
                end_outer = point(exact_point);
            const bool left_is_inner = segment.counterclockwise;
            points.points = {left_is_inner ? start_outer : start_inner,
                             left_is_inner ? start_inner : start_outer,
                             left_is_inner ? end_outer : end_inner,
                             left_is_inner ? end_inner : end_outer};
            const std::int64_t start_rx = segment.start.x - segment.center.x;
            const std::int64_t start_ry = segment.start.y - segment.center.y;
            const std::int64_t end_rx = segment.end.x - segment.center.x;
            const std::int64_t end_ry = segment.end.y - segment.center.y;
            const std::int64_t right_sign = left_is_inner ? 1 : -1;
            rays = {{{right_sign * start_rx, right_sign * start_ry},
                     {-right_sign * start_rx, -right_sign * start_ry},
                     {right_sign * end_rx, right_sign * end_ry},
                     {-right_sign * end_rx, -right_sign * end_ry}}};
        }
        for (std::uint32_t slot = 0; slot < 4; ++slot)
        {
            const std::uint32_t vertex = index + (slot >= 2 ? 1U : 0U);
            disk_rays_.push_back({vertex, index * 4U + slot, rays[slot].first, rays[slot].second,
                                  points.points[slot], 0});
        }
    }
    const auto upper_half = [](const VertexRay& value)
    { return value.dy > 0 || (value.dy == 0 && value.dx >= 0); };
    const auto ray_less = [&](const VertexRay& left, const VertexRay& right)
    {
        if (left.vertex != right.vertex)
            return left.vertex < right.vertex;
        const bool left_upper = upper_half(left);
        const bool right_upper = upper_half(right);
        if (left_upper != right_upper)
            return left_upper > right_upper;
        const int turn = wide_sign(cross_vectors(left.dx, left.dy, right.dx, right.dy));
        if (turn != 0)
            return turn > 0;
        return left.slot < right.slot;
    };
    const std::uint64_t levels =
        disk_rays_.size() < 2 ? 0
                              : static_cast<std::uint64_t>(std::ceil(std::log2(disk_rays_.size())));
    if (!charge(disk_rays_.size() * (levels + 2)))
        return false;
    std::sort(disk_rays_.begin(), disk_rays_.end(), ray_less);
    std::size_t write = 0;
    std::uint32_t keyed_vertex = std::numeric_limits<std::uint32_t>::max();
    std::uint32_t ray_ordinal = 0;
    for (std::size_t begin = 0; begin < disk_rays_.size();)
    {
        std::size_t end = begin + 1;
        while (end < disk_rays_.size() && disk_rays_[end].vertex == disk_rays_[begin].vertex &&
               wide_sign(cross_vectors(disk_rays_[begin].dx, disk_rays_[begin].dy,
                                       disk_rays_[end].dx, disk_rays_[end].dy)) == 0 &&
               wide_sign(dot_vectors(disk_rays_[begin].dx, disk_rays_[begin].dy, disk_rays_[end].dx,
                                     disk_rays_[end].dy)) > 0)
            ++end;
        if (disk_rays_[begin].vertex != keyed_vertex)
        {
            keyed_vertex = disk_rays_[begin].vertex;
            ray_ordinal = 0;
        }
        ++ray_ordinal;
        if (ray_ordinal > 4U)
            return fail(AnalyticFilteredLoweringError::invalid_topology);
        const std::uint64_t key =
            (std::uint64_t{disk_rays_[begin].vertex + 1U} << 32U) | ray_ordinal;
        for (std::size_t cursor = begin; cursor < end; ++cursor)
        {
            const std::uint32_t segment = disk_rays_[cursor].slot / 4U;
            const std::uint32_t slot = disk_rays_[cursor].slot % 4U;
            tangent_points_[segment].points[slot] = disk_rays_[begin].endpoint;
            tangent_points_[segment].keys[slot] = key;
        }
        disk_rays_[write] = disk_rays_[begin];
        disk_rays_[write].key = key;
        ++write;
        begin = end;
    }
    disk_rays_.resize(write);
    return true;
}

bool append_piece(AnalyticAtomicCurveNm curve, bool agrees, bool material_on_left,
                  std::uint64_t coverage, AnalyticFilteredSourceReference final_source,
                  TokenDescriptor descriptor, bool construction,
                  std::uint64_t start_tangent_key = 0, std::uint64_t end_tangent_key = 0)
{
    if (pieces_.curves.size() >= limits_.boundary_occurrences)
        return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
    if (curve.kind == AnalyticAtomicCurveKind::line)
        certify_integer_line(curve);
    curve.curve_index = static_cast<std::uint32_t>(pieces_.curves.size() + 1);
    pieces_.bounds.push_back(bounds_for(curve));
    pieces_.curves.push_back(std::move(curve));
    pieces_.occurrences.push_back(
        {pieces_.curves.size(),
         coverage,
         agrees,
         material_on_left,
         {AnalyticFilteredSourceKind::compact_feature_role,
          AnalyticFilteredSourceRole::primitive_outer_circle, coverage, coverage, 0}});
    piece_sources_.push_back(
        {final_source, descriptor, construction, start_tangent_key, end_tangent_key});
    piece_descriptors_.push_back(std::move(descriptor));
    return true;
}

static void certify_integer_line(AnalyticAtomicCurveNm& curve) noexcept
{
    const auto integer_coordinate =
        [](const AnalyticCoordinateIntervalNm& value, std::int64_t& output)
    {
        if (value.lower != value.upper || !std::isfinite(value.lower) ||
            value.lower < static_cast<double>(std::numeric_limits<std::int64_t>::min()) ||
            value.lower > static_cast<double>(std::numeric_limits<std::int64_t>::max()))
            return false;
        const double rounded = std::nearbyint(value.lower);
        if (rounded != value.lower)
            return false;
        output = static_cast<std::int64_t>(rounded);
        return true;
    };
    AnalyticIntegerPointNm start;
    AnalyticIntegerPointNm end;
    if (integer_coordinate(curve.start.x, start.x) && integer_coordinate(curve.start.y, start.y) &&
        integer_coordinate(curve.end.x, end.x) && integer_coordinate(curve.end.y, end.y) &&
        (start.x != end.x || start.y != end.y))
    {
        curve.has_integer_certificate = true;
        curve.integer_start = start;
        curve.integer_end = end;
    }
}

static bool singleton_integer_point(const AnalyticFilteredPointNm& value,
                                    AnalyticIntegerPointNm& output) noexcept
{
    const auto coordinate = [](const AnalyticCoordinateIntervalNm& input, std::int64_t& result)
    {
        if (input.lower != input.upper || !std::isfinite(input.lower) ||
            std::nearbyint(input.lower) != input.lower ||
            input.lower < static_cast<double>(std::numeric_limits<std::int64_t>::min()) ||
            input.lower > static_cast<double>(std::numeric_limits<std::int64_t>::max()))
            return false;
        result = static_cast<std::int64_t>(input.lower);
        return true;
    };
    return coordinate(value.x, output.x) && coordinate(value.y, output.y);
}

AnalyticAtomicCurveNm circle_half(Point start, Point end, Point center, Interval radius) const
{
    AnalyticAtomicCurveNm curve;
    curve.kind = AnalyticAtomicCurveKind::circular_arc;
    curve.start = public_point(start);
    curve.end = public_point(end);
    curve.circle.center = public_point(center);
    curve.circle.radius = public_interval(radius);
    curve.counterclockwise = true;
    curve.has_arc_sweep_certificate = true;
    return curve;
}

bool add_vertex_disk(std::uint32_t vertex_index, AnalyticIntegerPointNm center,
                     std::uint64_t coverage)
{
    const Interval half_width = exact(static_cast<double>(swept_->width_nm) * 0.5);
    const Point center_point = point(center);
    const std::uint64_t secondary =
        vertex_index == 0 ? std::uint64_t{1} << 32U
        : vertex_index == segments_.size()
            ? std::uint64_t(segments_.size() + 1) << 32U
            : (std::uint64_t(vertex_index) << 32U) | (vertex_index + 1U);
    const auto role = vertex_index == 0 ? AnalyticFilteredSourceRole::swept_start_cap
                      : vertex_index == segments_.size()
                          ? AnalyticFilteredSourceRole::swept_end_cap
                          : AnalyticFilteredSourceRole::swept_round_join;
    const auto source =
        AnalyticFilteredSourceReference{AnalyticFilteredSourceKind::compact_feature_role, role,
                                        operand_.operand_id, swept_->feature_id, secondary};
    const std::int64_t width = static_cast<std::int64_t>(swept_->width_nm);
    const TokenDescriptor descriptor = circle_descriptor(center, wide_multiply(width, width));
    const auto first = std::lower_bound(disk_rays_.begin(), disk_rays_.end(), vertex_index,
                                        [](const VertexRay& ray, std::uint32_t vertex)
                                        { return ray.vertex < vertex; });
    const auto last = std::upper_bound(first, disk_rays_.end(), vertex_index,
                                       [](std::uint32_t vertex, const VertexRay& ray)
                                       { return vertex < ray.vertex; });
    const std::size_t count = static_cast<std::size_t>(last - first);
    if (count < 2)
        return fail(AnalyticFilteredLoweringError::invalid_topology);
    for (std::size_t local = 0; local < count; ++local)
    {
        const VertexRay& start_ray = first[local];
        const VertexRay& end_ray = first[(local + 1U) % count];
        AnalyticAtomicCurveNm curve =
            circle_half(start_ray.endpoint, end_ray.endpoint, center_point, half_width);
        curve.major_arc =
            wide_sign(cross_vectors(start_ray.dx, start_ray.dy, end_ray.dx, end_ray.dy)) < 0;
        if ((swept_->width_nm & 1U) == 0)
        {
            AnalyticIntegerPointNm integer_start;
            AnalyticIntegerPointNm integer_end;
            if (singleton_integer_point(curve.start, integer_start) &&
                singleton_integer_point(curve.end, integer_end))
            {
                curve.has_integer_certificate = true;
                curve.integer_start = integer_start;
                curve.integer_end = integer_end;
                curve.integer_center = center;
                curve.has_integer_radius_certificate = true;
                curve.integer_radius = static_cast<std::uint64_t>(width / 2);
            }
        }
        if (!append_piece(std::move(curve), true, true, coverage, source, descriptor, false,
                          start_ray.key, end_ray.key))
            return false;
    }
    return true;
}

TokenDescriptor offset_line_descriptor(AnalyticIntegerPointNm start, std::int64_t dx,
                                       std::int64_t dy, bool original_left)
{
    const LineFamilyKey family = canonical_direction(dx, dy);
    const bool forward_agrees = dx != 0 ? dx > 0 : dy > 0;
    const bool canonical_left = original_left == forward_agrees;
    const std::int64_t sign = canonical_left ? 1 : -1;
    const WideInteger base =
        wide_subtract(wide_multiply(family.dx, start.y), wide_multiply(family.dy, start.x));
    TokenDescriptor descriptor;
    descriptor.line.family = family;
    descriptor.line.rational_part_times_two = wide_add(base, base);
    const WideInteger primitive_squared =
        wide_add(wide_multiply(family.dx, family.dx), wide_multiply(family.dy, family.dy));
    std::uint64_t primitive_length = 0;
    ++telemetry_.square_root_calls;
    if (!exact_integer_root(
            primitive_squared,
            std::hypot(static_cast<double>(family.dx), static_cast<double>(family.dy)),
            primitive_length))
        return descriptor;
    if (primitive_length != 0)
    {
        const WideInteger shift = wide_multiply(static_cast<std::int64_t>(swept_->width_nm),
                                                static_cast<std::int64_t>(primitive_length));
        descriptor.line.rational_part_times_two =
            sign > 0 ? wide_add(descriptor.line.rational_part_times_two, shift)
                     : wide_subtract(descriptor.line.rational_part_times_two, shift);
    }
    else
        descriptor.line.radical_coefficient = sign * static_cast<std::int64_t>(swept_->width_nm);
    return descriptor;
}

#pragma once

// Internal swept-path strip, token, and union-classification phases.
bool add_line_strip(std::uint32_t index, const Segment& segment, std::uint64_t coverage)
{
    const std::int64_t dx = segment.end.x - segment.start.x;
    const std::int64_t dy = segment.end.y - segment.start.y;
    const SegmentTangentPoints& tangencies = tangent_points_[index];
    const Point& start_right = tangencies.points[0];
    const Point& start_left = tangencies.points[1];
    const Point& end_right = tangencies.points[2];
    const Point& end_left = tangencies.points[3];
    const bool forward = dx != 0 ? dx > 0 : dy > 0;
    const bool chord_agrees = dy != 0 ? dy < 0 : dx > 0;
    const std::uint64_t secondary = std::uint64_t(index + 1) << 32U;
    const auto source = [&](AnalyticFilteredSourceRole role)
    {
        return AnalyticFilteredSourceReference{AnalyticFilteredSourceKind::compact_feature_role,
                                               role, operand_.operand_id, swept_->feature_id,
                                               secondary};
    };
    const auto line = [&](Point first, Point second)
    {
        AnalyticAtomicCurveNm curve;
        curve.start = public_point(first);
        curve.end = public_point(second);
        return curve;
    };
    const std::uint32_t right_index = static_cast<std::uint32_t>(pieces_.curves.size());
    if (!append_piece(line(start_right, end_right), forward, true, coverage,
                      source(AnalyticFilteredSourceRole::swept_right_offset_line),
                      offset_line_descriptor(segment.start, dx, dy, false), false,
                      tangencies.keys[0], tangencies.keys[2]) ||
        !append_piece(line(start_right, start_left), chord_agrees, false, coverage, {},
                      exact_line_descriptor(segment.start, -dy, dx), true, tangencies.keys[0],
                      tangencies.keys[1]) ||
        !append_piece(line(end_right, end_left), chord_agrees, true, coverage, {},
                      exact_line_descriptor(segment.end, -dy, dx), true, tangencies.keys[2],
                      tangencies.keys[3]) ||
        !append_piece(line(start_left, end_left), forward, false, coverage,
                      source(AnalyticFilteredSourceRole::swept_left_offset_line),
                      offset_line_descriptor(segment.start, dx, dy, true), false,
                      tangencies.keys[1], tangencies.keys[3]))
        return false;
    if (dy == 0)
        mirrors_.push_back({right_index, right_index + 3U, segment.start.y});
    return true;
}

bool exact_arc_offset_endpoint(const Segment& segment, AnalyticIntegerPointNm endpoint,
                               std::uint64_t integer_radius, bool outer,
                               AnalyticIntegerPointNm& result) const noexcept
{
    if (integer_radius == 0 || (swept_->width_nm & 1U) != 0 ||
        swept_->width_nm / 2U >
            static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
        return false;
    const std::uint64_t half = swept_->width_nm / 2U;
    const auto shifted = [&](std::int64_t coordinate, std::int64_t center, std::int64_t& output)
    {
        const std::int64_t radial = coordinate - center;
        const std::uint64_t divisor = std::gcd(magnitude(radial), integer_radius);
        const std::uint64_t denominator = integer_radius / divisor;
        if (denominator == 0 || half % denominator != 0)
            return false;
        const std::uint64_t scale = half / denominator;
        const std::uint64_t reduced = magnitude(radial) / divisor;
        if (reduced != 0 &&
            scale > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) / reduced)
            return false;
        std::int64_t delta = static_cast<std::int64_t>(reduced * scale);
        if (radial < 0)
            delta = -delta;
        if (!outer)
            delta = -delta;
        if ((delta > 0 && coordinate > std::numeric_limits<std::int64_t>::max() - delta) ||
            (delta < 0 && coordinate < std::numeric_limits<std::int64_t>::min() - delta))
            return false;
        output = coordinate + delta;
        return true;
    };
    return shifted(endpoint.x, segment.center.x, result.x) &&
           shifted(endpoint.y, segment.center.y, result.y);
}

bool add_arc_strip(std::uint32_t index, const Segment& segment, std::uint64_t coverage)
{
    const Point center = point(segment.center);
    const Point start = point(segment.start);
    const Point end = point(segment.end);
    const Point radial_start = subtract(start, center);
    const Point radial_end = subtract(end, center);
    ++telemetry_.square_root_calls;
    const Interval radius = square_root(dot(radial_start, radial_start));
    std::uint64_t integer_radius = 0;
    if (!exact_integer_root(segment.radius_squared, radius.upper, integer_radius))
        return false;
    if (!charge(8))
        return false;
    telemetry_.fixed_width_predicates += 8;
    const Interval half_width = exact(static_cast<double>(swept_->width_nm) * 0.5);
    const Interval inner_radius = subtract(radius, half_width);
    const Interval outer_radius = add(radius, half_width);
    if (!valid(inner_radius) || inner_radius.lower <= 0.0 || !valid(outer_radius))
        return fail(AnalyticFilteredLoweringError::invalid_arc);
    const SegmentTangentPoints& tangencies = tangent_points_[index];
    const Point& start_inner =
        segment.counterclockwise ? tangencies.points[1] : tangencies.points[0];
    const Point& start_outer =
        segment.counterclockwise ? tangencies.points[0] : tangencies.points[1];
    const Point& end_inner = segment.counterclockwise ? tangencies.points[3] : tangencies.points[2];
    const Point& end_outer = segment.counterclockwise ? tangencies.points[2] : tangencies.points[3];
    const std::uint64_t start_inner_key =
        segment.counterclockwise ? tangencies.keys[1] : tangencies.keys[0];
    const std::uint64_t start_outer_key =
        segment.counterclockwise ? tangencies.keys[0] : tangencies.keys[1];
    const std::uint64_t end_inner_key =
        segment.counterclockwise ? tangencies.keys[3] : tangencies.keys[2];
    const std::uint64_t end_outer_key =
        segment.counterclockwise ? tangencies.keys[2] : tangencies.keys[3];
    const std::uint64_t secondary = std::uint64_t(index + 1) << 32U;
    const std::int64_t width = static_cast<std::int64_t>(swept_->width_nm);
    const auto source = [&](AnalyticFilteredSourceRole role)
    {
        return AnalyticFilteredSourceReference{AnalyticFilteredSourceKind::compact_feature_role,
                                               role, operand_.operand_id, swept_->feature_id,
                                               secondary};
    };
    const auto arc = [&](Point first, Point second, Interval offset_radius, bool outer)
    {
        AnalyticAtomicCurveNm curve;
        curve.kind = AnalyticAtomicCurveKind::circular_arc;
        curve.start = public_point(first);
        curve.end = public_point(second);
        curve.circle.center = public_point(center);
        curve.circle.radius = public_interval(offset_radius);
        curve.counterclockwise = segment.counterclockwise;
        curve.major_arc = segment.major;
        curve.has_arc_sweep_certificate = true;
        AnalyticIntegerPointNm integer_start;
        AnalyticIntegerPointNm integer_end;
        if (exact_arc_offset_endpoint(segment, segment.start, integer_radius, outer,
                                      integer_start) &&
            exact_arc_offset_endpoint(segment, segment.end, integer_radius, outer, integer_end))
        {
            curve.has_integer_certificate = true;
            curve.integer_start = integer_start;
            curve.integer_end = integer_end;
            curve.start = public_point(point(integer_start));
            curve.end = public_point(point(integer_end));
            curve.integer_center = segment.center;
            curve.has_integer_radius_certificate = true;
            const std::uint64_t half = swept_->width_nm / 2U;
            curve.integer_radius = outer ? integer_radius + half : integer_radius - half;
            curve.circle.radius = public_interval(exact(static_cast<double>(curve.integer_radius)));
        }
        return curve;
    };
    const auto chord = [&](Point first, Point second)
    {
        AnalyticAtomicCurveNm curve;
        curve.start = public_point(first);
        curve.end = public_point(second);
        return curve;
    };
    const auto inner_role = segment.counterclockwise
                                ? AnalyticFilteredSourceRole::swept_left_offset_arc
                                : AnalyticFilteredSourceRole::swept_right_offset_arc;
    const auto outer_role = segment.counterclockwise
                                ? AnalyticFilteredSourceRole::swept_right_offset_arc
                                : AnalyticFilteredSourceRole::swept_left_offset_arc;
    const TokenDescriptor inner_descriptor = offset_circle_descriptor(
        segment.center, segment.radius_squared, width, false, integer_radius);
    const TokenDescriptor outer_descriptor = offset_circle_descriptor(
        segment.center, segment.radius_squared, width, true, integer_radius);
    const std::int64_t start_rx = segment.start.x - segment.center.x;
    const std::int64_t start_ry = segment.start.y - segment.center.y;
    const std::int64_t end_rx = segment.end.x - segment.center.x;
    const std::int64_t end_ry = segment.end.y - segment.center.y;
    const bool start_agrees = start_rx != 0 ? start_rx > 0 : start_ry > 0;
    const bool end_agrees = end_rx != 0 ? end_rx > 0 : end_ry > 0;
    if (!append_piece(arc(start_inner, end_inner, inner_radius, false), segment.counterclockwise,
                      !segment.counterclockwise, coverage, source(inner_role), inner_descriptor,
                      false, start_inner_key, end_inner_key) ||
        !append_piece(chord(start_inner, start_outer), start_agrees, segment.counterclockwise,
                      coverage, {}, exact_line_descriptor(segment.start, segment.center), true,
                      start_inner_key, start_outer_key) ||
        !append_piece(chord(end_inner, end_outer), end_agrees, !segment.counterclockwise, coverage,
                      {}, exact_line_descriptor(segment.end, segment.center), true, end_inner_key,
                      end_outer_key) ||
        !append_piece(arc(start_outer, end_outer, outer_radius, true), segment.counterclockwise,
                      segment.counterclockwise, coverage, source(outer_role), outer_descriptor,
                      false, start_outer_key, end_outer_key))
        return false;
    return true;
}

bool build_piece_geometry()
{
    pieces_.origin_x_nm = origin_x_nm_;
    pieces_.origin_y_nm = origin_y_nm_;
    const std::uint64_t curve_count = disk_rays_.size() + segments_.size() * 4;
    if (curve_count > limits_.boundary_occurrences)
        return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
    const std::uint64_t reserved = curve_count * (kPieceCurveBytes + kPieceCoverageBytes) +
                                   segments_.size() * kMirrorPairBytes + kSyntheticOperandBytes +
                                   kSyntheticJobBytes + kSyntheticStageBytes;
    if (!retain(reserved))
        return false;
    if (!charge(segments_.size() * 2 + 1))
        return false;
    pieces_.curves.reserve(static_cast<std::size_t>(curve_count));
    pieces_.bounds.reserve(static_cast<std::size_t>(curve_count));
    pieces_.occurrences.reserve(static_cast<std::size_t>(curve_count));
    piece_sources_.reserve(static_cast<std::size_t>(curve_count));
    piece_descriptors_.reserve(static_cast<std::size_t>(curve_count));
    mirrors_.reserve(segments_.size());
    std::uint64_t coverage = 0;
    for (std::uint32_t vertex = 0; vertex <= segments_.size(); ++vertex)
    {
        ++coverage;
        const auto center =
            vertex == segments_.size() ? segments_.back().end : segments_[vertex].start;
        if (!add_vertex_disk(vertex, center, coverage))
            return false;
    }
    for (std::uint32_t index = 0; index < segments_.size(); ++index)
    {
        ++coverage;
        if (!(segments_[index].arc ? add_arc_strip(index, segments_[index], coverage)
                                   : add_line_strip(index, segments_[index], coverage)))
            return false;
    }
    return true;
}

bool assign_tokens()
{
    return assign_tokens(pieces_, piece_descriptors_, mirrors_);
}

bool assign_tokens(AnalyticFilteredGeometry& geometry,
                   const std::vector<TokenDescriptor>& descriptors,
                   const std::vector<MirrorPair>& mirrors)
{
    if (geometry.curves.size() != descriptors.size())
        return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
    const std::uint64_t order_bytes = descriptors.size() * kIndexBytes;
    if (order_bytes > limits_.working_memory_bytes -
                          std::min(limits_.working_memory_bytes, retained_piece_bytes_))
        return fail_memory(retained_piece_bytes_ + order_bytes);
    telemetry_.peak_working_memory_bytes =
        std::max(telemetry_.peak_working_memory_bytes, retained_piece_bytes_ + order_bytes);
    std::vector<std::uint32_t> order(descriptors.size());
    std::iota(order.begin(), order.end(), 0U);
    const std::uint64_t levels =
        descriptors.size() < 2
            ? 0
            : static_cast<std::uint64_t>(std::ceil(std::log2(descriptors.size())));
    if (!charge(descriptors.size() * (levels * 2 + 5)))
        return false;
    std::sort(order.begin(), order.end(),
              [&](std::uint32_t left, std::uint32_t right)
              {
                  if (descriptor_family_less(descriptors[left], descriptors[right]))
                      return true;
                  if (descriptor_family_less(descriptors[right], descriptors[left]))
                      return false;
                  if (descriptors[left].kind == TokenKeyKind::circle)
                  {
                      const auto& left_radius = geometry.curves[left].circle.radius;
                      const auto& right_radius = geometry.curves[right].circle.radius;
                      if (left_radius.lower != right_radius.lower)
                          return left_radius.lower < right_radius.lower;
                      if (left_radius.upper != right_radius.upper)
                          return left_radius.upper < right_radius.upper;
                  }
                  return left < right;
              });
    std::uint64_t token = 0;
    std::optional<std::uint32_t> previous;
    for (std::uint32_t index : order)
    {
        if (!previous || !descriptor_family_equal(descriptors[*previous], descriptors[index]))
            ++token;
        geometry.curves[index].construction_family_id = token;
        previous = index;
    }
    for (std::size_t begin = 0; begin < order.size();)
    {
        std::size_t end = begin + 1;
        while (end < order.size() &&
               descriptor_family_equal(descriptors[order[begin]], descriptors[order[end]]))
            ++end;
        if (descriptors[order[begin]].kind == TokenKeyKind::circle)
        {
            std::size_t component_begin = begin;
            double component_upper = geometry.curves[order[begin]].circle.radius.upper;
            bool component_symbolic = descriptors[order[begin]].circle.radical_coefficient != 0;
            bool component_distinct = false;
            const auto finish_component = [&](std::size_t component_end)
            {
                return component_end - component_begin < 2 || !component_symbolic ||
                       !component_distinct;
            };
            for (std::size_t cursor = begin + 1; cursor < end; ++cursor)
            {
                const auto& radius = geometry.curves[order[cursor]].circle.radius;
                if (radius.lower > component_upper)
                {
                    if (!finish_component(cursor))
                        return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
                    component_begin = cursor;
                    component_upper = radius.upper;
                    component_symbolic = descriptors[order[cursor]].circle.radical_coefficient != 0;
                    component_distinct = false;
                }
                else
                {
                    component_upper = std::max(component_upper, radius.upper);
                    component_symbolic = component_symbolic ||
                                         descriptors[order[cursor]].circle.radical_coefficient != 0;
                    component_distinct =
                        component_distinct ||
                        !descriptor_carrier_equal(descriptors[order[component_begin]],
                                                  descriptors[order[cursor]]);
                }
            }
            if (!finish_component(end))
                return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
        }
        begin = end;
    }
    std::sort(order.begin(), order.end(),
              [&](std::uint32_t left, std::uint32_t right)
              {
                  if (descriptor_carrier_less(descriptors[left], descriptors[right]))
                      return true;
                  if (descriptor_carrier_less(descriptors[right], descriptors[left]))
                      return false;
                  return left < right;
              });
    token = 0;
    previous.reset();
    for (std::uint32_t index : order)
    {
        if (!previous || !descriptor_carrier_equal(descriptors[*previous], descriptors[index]))
            ++token;
        AnalyticAtomicCurveNm& curve = geometry.curves[index];
        curve.construction_carrier_id = token;
        if (descriptors[index].kind == TokenKeyKind::line)
        {
            curve.has_construction_line_direction = true;
            curve.construction_line_dx = descriptors[index].line.family.dx;
            curve.construction_line_dy = descriptors[index].line.family.dy;
            if (curve.construction_line_dx == 0)
            {
                const std::uint64_t column = analytic_vertical_x_column_token(token);
                if (column == 0)
                    return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
                curve.start.construction_x_column_id = column;
                curve.end.construction_x_column_id = column;
            }
        }
        previous = index;
    }
    for (const MirrorPair& mirror : mirrors)
    {
        if (mirror.first >= geometry.curves.size() || mirror.second >= geometry.curves.size())
            return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
        AnalyticAtomicCurveNm& first = geometry.curves[mirror.first];
        AnalyticAtomicCurveNm& second = geometry.curves[mirror.second];
        const std::uint64_t identity = analytic_horizontal_mirror_construction_id(
            first.construction_carrier_id, second.construction_carrier_id);
        if (identity == 0)
            return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
        first.construction_horizontal_mirror_id = identity;
        second.construction_horizontal_mirror_id = identity;
        first.construction_horizontal_mirror_axis_y = mirror.axis_y;
        second.construction_horizontal_mirror_axis_y = mirror.axis_y;
    }
    return true;
}

static bool coverage_contains(const AnalyticFilteredBooleanSelectionResult& selection,
                              std::uint32_t root, std::uint32_t operand,
                              std::uint32_t operand_count) noexcept
{
    std::uint32_t leaf_capacity = 1;
    while (leaf_capacity < operand_count)
        leaf_capacity <<= 1U;
    std::uint32_t begin = 0;
    std::uint32_t width = leaf_capacity;
    while (width > 1)
    {
        if (root >= selection.coverage_state_nodes.size() || root == 1)
            return false;
        const std::uint32_t half = width / 2;
        if (operand < begin + half)
            root = root == 0 ? 0 : selection.coverage_state_nodes[root].left;
        else
        {
            begin += half;
            root = root == 0 ? 0 : selection.coverage_state_nodes[root].right;
        }
        width = half;
    }
    return root == 1;
}

PieceContainment strictly_inside_piece(Point candidate, std::uint64_t coverage) noexcept
{
    const std::uint64_t vertex_count = segments_.size() + 1;
    const Interval four = exact(4.0);
    const Interval width_squared = square(exact(static_cast<double>(swept_->width_nm)));
    if (coverage >= 1 && coverage <= vertex_count)
    {
        const std::size_t vertex = static_cast<std::size_t>(coverage - 1);
        const AnalyticIntegerPointNm center =
            vertex == segments_.size() ? segments_.back().end : segments_[vertex].start;
        const Point radial = subtract(candidate, point(center));
        const Interval distance = multiply(four, dot(radial, radial));
        if (distance.upper < width_squared.lower)
            return PieceContainment::proven_inside;
        if (distance.lower >= width_squared.upper)
            return PieceContainment::proven_outside;
        return PieceContainment::uncertain;
    }
    if (coverage <= vertex_count || coverage > vertex_count + segments_.size())
        return PieceContainment::uncertain;
    const Segment& segment = segments_[static_cast<std::size_t>(coverage - vertex_count - 1)];
    if (segment.arc)
    {
        const Point center = point(segment.center);
        const Point radial = subtract(candidate, center);
        const Point start_radial = subtract(point(segment.start), center);
        const Point end_radial = subtract(point(segment.end), center);
        ++telemetry_.square_root_calls;
        const Interval radius = square_root(dot(start_radial, start_radial));
        const Interval half_width = exact(static_cast<double>(swept_->width_nm) * 0.5);
        const Interval inner_squared = square(subtract(radius, half_width));
        const Interval outer_squared = square(add(radius, half_width));
        const Interval candidate_squared = dot(radial, radial);
        if (candidate_squared.upper <= inner_squared.lower ||
            candidate_squared.lower >= outer_squared.upper)
            return PieceContainment::proven_outside;
        const bool radial_inside = candidate_squared.lower > inner_squared.upper &&
                                   candidate_squared.upper < outer_squared.lower;
        const Interval from_start = cross(start_radial, radial);
        const Interval to_end = cross(radial, end_radial);
        const bool start_positive = from_start.lower > 0.0;
        const bool start_negative = from_start.upper < 0.0;
        const bool end_positive = to_end.lower > 0.0;
        const bool end_negative = to_end.upper < 0.0;
        bool angular_inside = false;
        bool angular_outside = false;
        if (segment.counterclockwise)
        {
            angular_inside =
                segment.major ? start_positive || end_positive : start_positive && end_positive;
            angular_outside = segment.major ? from_start.upper <= 0.0 && to_end.upper <= 0.0
                                            : from_start.upper <= 0.0 || to_end.upper <= 0.0;
        }
        else
        {
            angular_inside =
                segment.major ? start_negative || end_negative : start_negative && end_negative;
            angular_outside = segment.major ? from_start.lower >= 0.0 && to_end.lower >= 0.0
                                            : from_start.lower >= 0.0 || to_end.lower >= 0.0;
        }
        if (angular_outside)
            return PieceContainment::proven_outside;
        if (radial_inside && angular_inside)
            return PieceContainment::proven_inside;
        return PieceContainment::uncertain;
    }
    const Point start = point(segment.start);
    const Point direction = subtract(point(segment.end), start);
    const Point relative = subtract(candidate, start);
    const Interval length_squared = dot(direction, direction);
    const Interval projection = dot(relative, direction);
    if (projection.upper <= 0.0 || projection.lower >= length_squared.upper)
        return PieceContainment::proven_outside;
    const bool projection_inside =
        projection.lower > 0.0 && projection.upper < length_squared.lower;
    const Interval distance_numerator = square(cross(direction, relative));
    const Interval distance = multiply(four, distance_numerator);
    const Interval threshold = multiply(width_squared, length_squared);
    if (distance.lower >= threshold.upper)
        return PieceContainment::proven_outside;
    if (projection_inside && distance.upper < threshold.lower)
        return PieceContainment::proven_inside;
    return PieceContainment::uncertain;
}

bool classify_union_span(const AnalyticFilteredOverlayResult& overlay, std::uint32_t span_index,
                         bool& material_left, bool& material_right, std::uint32_t& chosen)
{
    const AnalyticAtomicSpanNm& span = overlay.spans[span_index];
    material_left = false;
    material_right = false;
    chosen = std::numeric_limits<std::uint32_t>::max();
    bool construction_covers_boundary = false;
    if (!charge(span.membership_count))
        return false;
    for (std::uint32_t local = 0; local < span.membership_count; ++local)
    {
        const AnalyticSpanMembership& membership =
            overlay.memberships[span.membership_begin + local];
        material_left = material_left || membership.material_on_span_left;
        material_right = material_right || !membership.material_on_span_left;
    }
    Point representative = scale(add(analytic_selection_detail::point(span.start),
                                     analytic_selection_detail::point(span.end)),
                                 exact(0.5));
    if (span.kind == AnalyticAtomicCurveKind::circular_arc)
    {
        if (span.carrier_curve_index == 0 || span.carrier_curve_index > pieces_.curves.size())
            return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
        const AnalyticAtomicCurveNm& carrier = pieces_.curves[span.carrier_curve_index - 1];
        const Point center = analytic_selection_detail::point(carrier.circle.center);
        const Point start_radial = subtract(analytic_selection_detail::point(span.start), center);
        const Point end_radial = subtract(analytic_selection_detail::point(span.end), center);
        const Point bisector = add(start_radial, end_radial);
        ++telemetry_.square_root_calls;
        const Interval bisector_length = square_root(dot(bisector, bisector));
        const Interval radius{carrier.circle.radius.lower, carrier.circle.radius.upper};
        if (bisector_length.lower > 0.0)
            representative = add(center, scale(bisector, divide(radius, bisector_length)));
        else if (bisector_length.upper == 0.0 &&
                 span.x_monotone_branch != AnalyticXMonotoneBranch::none)
        {
            const Interval signed_radius = span.x_monotone_branch == AnalyticXMonotoneBranch::upper
                                               ? radius
                                               : multiply(radius, exact(-1.0));
            representative = add(center, {exact(0.0), signed_radius});
        }
        else
            return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
    }
    const std::uint64_t piece_count = segments_.size() * 2 + 1;
    if (!charge(piece_count * (span.membership_count + 1U)))
        return false;
    for (std::uint64_t coverage = 1; coverage <= piece_count; ++coverage)
    {
        bool boundary_coverage = false;
        for (std::uint32_t local = 0; local < span.membership_count; ++local)
        {
            const AnalyticSpanMembership& membership =
                overlay.memberships[span.membership_begin + local];
            if (membership.curve_index != 0 &&
                membership.curve_index <= pieces_.occurrences.size() &&
                pieces_.occurrences[membership.curve_index - 1U].coverage_id == coverage)
            {
                boundary_coverage = true;
                break;
            }
        }
        if (boundary_coverage)
            continue;
        const PieceContainment containment = strictly_inside_piece(representative, coverage);
        if (containment == PieceContainment::proven_inside)
        {
            material_left = true;
            material_right = true;
            break;
        }
        if (containment == PieceContainment::uncertain)
            return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
    }
    if (material_left == material_right)
        return true;
    for (std::uint32_t local = 0; local < span.membership_count; ++local)
    {
        const AnalyticSpanMembership& membership =
            overlay.memberships[span.membership_begin + local];
        if (membership.material_on_span_left != material_left || membership.curve_index == 0 ||
            membership.curve_index > piece_sources_.size())
            continue;
        const std::uint32_t curve = membership.curve_index - 1;
        if (piece_sources_[curve].construction)
            construction_covers_boundary = true;
        else
            chosen = std::min(chosen, curve);
    }
    if (construction_covers_boundary)
        return fail(AnalyticFilteredLoweringError::invalid_topology);
    if (chosen == std::numeric_limits<std::uint32_t>::max())
        return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
    return true;
}

bool project_union_spans(const AnalyticFilteredOverlayResult& overlay)
{
    const std::uint64_t rebuild_bytes =
        overlay.spans.size() * (kPieceCurveBytes + kPieceCoverageBytes);
    if (rebuild_bytes > limits_.working_memory_bytes -
                            std::min(limits_.working_memory_bytes, retained_piece_bytes_))
        return fail_memory(retained_piece_bytes_ + rebuild_bytes);
    telemetry_.peak_working_memory_bytes =
        std::max(telemetry_.peak_working_memory_bytes, retained_piece_bytes_ + rebuild_bytes);
    if (!charge(overlay.spans.size()))
        return false;
    AnalyticFilteredGeometry rebuilt;
    rebuilt.origin_x_nm = pieces_.origin_x_nm;
    rebuilt.origin_y_nm = pieces_.origin_y_nm;
    std::vector<PieceSource> rebuilt_sources;
    std::vector<TokenDescriptor> rebuilt_descriptors;
    rebuilt.curves.reserve(overlay.spans.size());
    rebuilt.bounds.reserve(overlay.spans.size());
    rebuilt.occurrences.reserve(overlay.spans.size());
    rebuilt_sources.reserve(overlay.spans.size());
    rebuilt_descriptors.reserve(overlay.spans.size());
    for (std::uint32_t span_index = 0; span_index < overlay.spans.size(); ++span_index)
    {
        bool material_left = false;
        bool material_right = false;
        std::uint32_t chosen = std::numeric_limits<std::uint32_t>::max();
        if (!classify_union_span(overlay, span_index, material_left, material_right, chosen))
            return false;
        if (material_left == material_right)
            continue;
        const AnalyticAtomicSpanNm& span = overlay.spans[span_index];
        if (span.carrier_curve_index == 0 || span.carrier_curve_index > pieces_.curves.size())
            return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
        AnalyticAtomicCurveNm curve = pieces_.curves[span.carrier_curve_index - 1];
        const bool had_integer_radius = curve.has_integer_radius_certificate;
        const AnalyticIntegerPointNm integer_center = curve.integer_center;
        const std::uint64_t integer_radius = curve.integer_radius;
        curve.start = span.start;
        curve.end = span.end;
        curve.start.construction_x_column_id = 0;
        curve.end.construction_x_column_id = 0;
        curve.major_arc = false;
        curve.has_integer_certificate = false;
        curve.has_integer_radius_certificate = false;
        curve.has_endpoint_authoritative_arc_certificate = false;
        curve.has_endpoint_authoritative_x_monotone_certificate = false;
        curve.construction_start_tangent_id = 0;
        curve.construction_end_tangent_id = 0;
        if (curve.kind == AnalyticAtomicCurveKind::line)
            certify_integer_line(curve);
        else
        {
            curve.counterclockwise = true;
            AnalyticIntegerPointNm integer_start;
            AnalyticIntegerPointNm integer_end;
            if (had_integer_radius && integer_radius != 0 &&
                singleton_integer_point(curve.start, integer_start) &&
                singleton_integer_point(curve.end, integer_end))
            {
                curve.has_integer_certificate = true;
                curve.integer_start = integer_start;
                curve.integer_end = integer_end;
                curve.integer_center = integer_center;
                curve.has_integer_radius_certificate = true;
                curve.integer_radius = integer_radius;
            }
        }
        rebuilt.curves.push_back(std::move(curve));
        rebuilt.curves.back().curve_index = static_cast<std::uint32_t>(rebuilt.curves.size());
        rebuilt.bounds.push_back(bounds_for(rebuilt.curves.back()));
        rebuilt.occurrences.push_back(
            {rebuilt.curves.size(),
             1,
             true,
             material_left,
             {AnalyticFilteredSourceKind::compact_feature_role,
              AnalyticFilteredSourceRole::primitive_outer_circle, 1, 1, 0}});
        PieceSource rebuilt_source = piece_sources_[chosen];
        const AnalyticAtomicCurveNm& source_curve = pieces_.curves[chosen];
        const auto key_at = [&](const AnalyticFilteredPointNm& value)
        {
            if (same_filtered_point(value, source_curve.start))
                return piece_sources_[chosen].start_tangent_key;
            if (same_filtered_point(value, source_curve.end))
                return piece_sources_[chosen].end_tangent_key;
            return std::uint64_t{0};
        };
        rebuilt_source.start_tangent_key = key_at(span.start);
        rebuilt_source.end_tangent_key = key_at(span.end);
        rebuilt_sources.push_back(std::move(rebuilt_source));
        rebuilt_descriptors.push_back(piece_sources_[chosen].descriptor);
    }
    const std::uint64_t original_bytes =
        pieces_.curves.size() * (kPieceCurveBytes + kPieceCoverageBytes);
    pieces_ = std::move(rebuilt);
    piece_sources_ = std::move(rebuilt_sources);
    piece_descriptors_ = std::move(rebuilt_descriptors);
    retained_piece_bytes_ -= original_bytes;
    retained_piece_bytes_ += rebuild_bytes;
    return true;
}

static bool same_filtered_point(const AnalyticFilteredPointNm& left,
                                const AnalyticFilteredPointNm& right) noexcept
{
    return left.x.lower == right.x.lower && left.x.upper == right.x.upper &&
           left.y.lower == right.y.lower && left.y.upper == right.y.upper;
}

static bool filtered_points_overlap(const AnalyticFilteredPointNm& left,
                                    const AnalyticFilteredPointNm& right) noexcept
{
    return left.x.lower <= right.x.upper && right.x.lower <= left.x.upper &&
           left.y.lower <= right.y.upper && right.y.lower <= left.y.upper;
}

bool source_names_tangent_vertex(const PieceSource& source, AnalyticAtomicCurveKind kind,
                                 std::uint32_t vertex) const noexcept
{
    if (vertex > segments_.size())
        return false;
    switch (source.source.role)
    {
    case AnalyticFilteredSourceRole::swept_start_cap:
        return kind == AnalyticAtomicCurveKind::circular_arc && vertex == 0;
    case AnalyticFilteredSourceRole::swept_end_cap:
        return kind == AnalyticAtomicCurveKind::circular_arc && vertex == segments_.size();
    case AnalyticFilteredSourceRole::swept_round_join:
        return kind == AnalyticAtomicCurveKind::circular_arc && vertex != 0 &&
               vertex < segments_.size() && (source.source.secondary_id >> 32U) == vertex;
    case AnalyticFilteredSourceRole::swept_left_offset_line:
    case AnalyticFilteredSourceRole::swept_right_offset_line:
        if (kind != AnalyticAtomicCurveKind::line)
            return false;
        break;
    case AnalyticFilteredSourceRole::swept_left_offset_arc:
    case AnalyticFilteredSourceRole::swept_right_offset_arc:
        if (kind != AnalyticAtomicCurveKind::circular_arc)
            return false;
        break;
    default:
        return source.construction && kind == AnalyticAtomicCurveKind::line;
    }
    const std::uint64_t ordinal = source.source.secondary_id >> 32U;
    if (ordinal == 0)
        return false;
    const std::uint32_t segment = static_cast<std::uint32_t>(ordinal - 1U);
    return segment < segments_.size() && (vertex == segment || vertex == segment + 1U);
}

bool trusted_collapsed_tangency(const AnalyticPairIntersection& intersection) const noexcept
{
    if ((intersection.relation != AnalyticPairRelation::point &&
         intersection.relation != AnalyticPairRelation::two_points) ||
        intersection.point_count == 0 || intersection.point_count > 2 ||
        intersection.pair.first == 0 || intersection.pair.second == 0)
        return false;
    const std::uint32_t first = intersection.pair.first - 1;
    const std::uint32_t second = intersection.pair.second - 1;
    if (first >= pieces_.curves.size() || second >= pieces_.curves.size())
        return false;
    const AnalyticAtomicCurveNm& first_curve = pieces_.curves[first];
    const AnalyticAtomicCurveNm& second_curve = pieces_.curves[second];
    if ((first_curve.kind != AnalyticAtomicCurveKind::line &&
         first_curve.kind != AnalyticAtomicCurveKind::circular_arc) ||
        (second_curve.kind != AnalyticAtomicCurveKind::line &&
         second_curve.kind != AnalyticAtomicCurveKind::circular_arc) ||
        first_curve.construction_carrier_id == 0 || second_curve.construction_carrier_id == 0)
        return false;
    const TokenKeyKind first_descriptor_kind = piece_descriptors_[first].kind;
    const TokenKeyKind second_descriptor_kind = piece_descriptors_[second].kind;
    if ((first_curve.kind == AnalyticAtomicCurveKind::line) !=
            (first_descriptor_kind == TokenKeyKind::line) ||
        (second_curve.kind == AnalyticAtomicCurveKind::line) !=
            (second_descriptor_kind == TokenKeyKind::line))
        return false;
    const PieceSource& first_source = piece_sources_[first];
    const PieceSource& second_source = piece_sources_[second];
    if (first_curve.kind == AnalyticAtomicCurveKind::line &&
        second_curve.kind == AnalyticAtomicCurveKind::line &&
        first_source.construction == second_source.construction)
        return false;
    const std::array<std::pair<std::uint64_t, const AnalyticFilteredPointNm*>, 2> first_ends = {
        {{first_source.start_tangent_key, &first_curve.start},
         {first_source.end_tangent_key, &first_curve.end}}};
    const std::array<std::pair<std::uint64_t, const AnalyticFilteredPointNm*>, 2> second_ends = {
        {{second_source.start_tangent_key, &second_curve.start},
         {second_source.end_tangent_key, &second_curve.end}}};
    std::array<bool, 2> point_bound{};
    std::uint32_t bound_relations = 0;
    for (const auto& first_end : first_ends)
        for (const auto& second_end : second_ends)
        {
            if (first_end.first == 0 || first_end.first != second_end.first ||
                !same_filtered_point(*first_end.second, *second_end.second))
                continue;
            const std::uint32_t vertex = static_cast<std::uint32_t>((first_end.first >> 32U) - 1U);
            const std::uint64_t ray = first_end.first & 0xffffffffULL;
            if (ray == 0 || ray > 4U ||
                !source_names_tangent_vertex(first_source, first_curve.kind, vertex) ||
                !source_names_tangent_vertex(second_source, second_curve.kind, vertex))
                continue;
            // This is an exact lowering-issued construction relation: both finite-domain
            // endpoints were derived from the same authored vertex and exact incident normal
            // ray. The carrier/source checks above bind that private key to each curve.
            ++bound_relations;
            for (std::uint32_t point_index = 0; point_index < intersection.point_count;
                 ++point_index)
                if (filtered_points_overlap(intersection.points[point_index], *first_end.second))
                    point_bound[point_index] = true;
        }
    if (bound_relations != intersection.point_count)
        return false;
    for (std::uint32_t index = 0; index < intersection.point_count; ++index)
        if (!point_bound[index])
            return false;
    return true;
}

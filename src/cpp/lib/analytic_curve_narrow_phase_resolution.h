#pragma once

// Internal resolution certification and authoritative-endpoint intersection helpers.
ResolutionPairStatus compare_point_distance(Point left, Point right) noexcept
{
    constexpr double resolution_squared =
        static_cast<double>(kAnalyticTopologyResolutionNm * kAnalyticTopologyResolutionNm);
    const Point delta = subtract(left, right);
    const Interval distance_squared = add(square(delta.x), square(delta.y));
    if (distance_squared.upper <= resolution_squared)
        return ResolutionPairStatus::within;
    if (distance_squared.lower > resolution_squared)
        return ResolutionPairStatus::outside;
    return ResolutionPairStatus::uncertain;
}

std::uint8_t resolution_endpoints(Point candidate, const AnalyticAtomicCurveNm& curve,
                                  Point (&endpoints)[2]) noexcept
{
    std::uint8_t count = 0;
    for (const Point endpoint : {point(curve.start), point(curve.end)})
        if (interval_within_resolution(candidate, endpoint))
            endpoints[count++] = endpoint;
    return count;
}

ResolutionPairStatus certify_resolution_pair(Point candidate, const AnalyticAtomicCurveNm& left,
                                             const AnalyticAtomicCurveNm& right,
                                             DomainResult left_domain, DomainResult right_domain,
                                             AnalyticNarrowPhaseTelemetry& telemetry,
                                             const AnalyticSolverLimits& limits) noexcept
{
    const bool left_resolution = left_domain == DomainResult::inside_resolution ||
                                 left_domain == DomainResult::ambiguous_resolution;
    const bool right_resolution = right_domain == DomainResult::inside_resolution ||
                                  right_domain == DomainResult::ambiguous_resolution;
    if (!left_resolution && !right_resolution)
        return ResolutionPairStatus::within;

    Point left_endpoints[2]{};
    Point right_endpoints[2]{};
    if ((left_resolution && !charge_predicate(telemetry, limits, true)) ||
        (right_resolution && !charge_predicate(telemetry, limits, true)))
        return ResolutionPairStatus::uncertain;
    const std::uint8_t left_count =
        left_resolution ? resolution_endpoints(candidate, left, left_endpoints) : 0;
    const std::uint8_t right_count =
        right_resolution ? resolution_endpoints(candidate, right, right_endpoints) : 0;
    if ((left_resolution && left_count == 0) || (right_resolution && right_count == 0))
        return ResolutionPairStatus::uncertain;

    bool saw_uncertain = false;
    if (left_resolution && right_resolution)
    {
        for (std::uint8_t left_index = 0; left_index < left_count; ++left_index)
            for (std::uint8_t right_index = 0; right_index < right_count; ++right_index)
            {
                if (!charge_predicate(telemetry, limits, true))
                    return ResolutionPairStatus::uncertain;
                const ResolutionPairStatus status = compare_point_distance(
                    left_endpoints[left_index], right_endpoints[right_index]);
                if (status == ResolutionPairStatus::within)
                    return status;
                saw_uncertain = saw_uncertain || status == ResolutionPairStatus::uncertain;
            }
    }
    else
    {
        const Point* endpoints = left_resolution ? left_endpoints : right_endpoints;
        const std::uint8_t count = left_resolution ? left_count : right_count;
        const AnalyticAtomicCurveNm& other = left_resolution ? right : left;
        for (std::uint8_t index = 0; index < count; ++index)
        {
            if (!charge_predicate(telemetry, limits, true))
                return ResolutionPairStatus::uncertain;
            const AnalyticFilteredPointCurveStatus status =
                classify_point_on_valid_curve(other, endpoints[index]);
            if (status == AnalyticFilteredPointCurveStatus::certified_on_domain)
                return ResolutionPairStatus::within;
            saw_uncertain = saw_uncertain ||
                            status == AnalyticFilteredPointCurveStatus::uncertain ||
                            status == AnalyticFilteredPointCurveStatus::invalid_argument;
        }
    }
    return saw_uncertain ? ResolutionPairStatus::uncertain : ResolutionPairStatus::outside;
}

bool retain_point(PairWork& work, Point candidate, const AnalyticAtomicCurveNm& left,
                  const AnalyticAtomicCurveNm& right, AnalyticNarrowPhaseTelemetry& telemetry,
                  const AnalyticSolverLimits& limits,
                  analytic_execution_detail::TopologyPolicy policy) noexcept
{
    if (!valid_interval(candidate.x) || !valid_interval(candidate.y) ||
        !point_interval_fits_resolution(candidate))
    {
        work.uncertain = true;
        return false;
    }
    const DomainResult left_domain = curve_domain(candidate, left, telemetry, limits, policy);
    const DomainResult right_domain = curve_domain(candidate, right, telemetry, limits, policy);
    if (left_domain == DomainResult::uncertain || right_domain == DomainResult::uncertain)
    {
        work.uncertain = true;
        return false;
    }
    if (left_domain == DomainResult::outside || right_domain == DomainResult::outside)
        return true;
    const ResolutionPairStatus pair_status = certify_resolution_pair(
        candidate, left, right, left_domain, right_domain, telemetry, limits);
    if (pair_status == ResolutionPairStatus::uncertain)
    {
        work.uncertain = true;
        return false;
    }
    if (pair_status == ResolutionPairStatus::outside)
    {
        if (left_domain == DomainResult::ambiguous_resolution ||
            right_domain == DomainResult::ambiguous_resolution)
        {
            work.uncertain = true;
            return false;
        }
        return true;
    }
    if (left_domain == DomainResult::inside_resolution ||
        left_domain == DomainResult::ambiguous_resolution ||
        right_domain == DomainResult::inside_resolution ||
        right_domain == DomainResult::ambiguous_resolution)
        work.value.resolution_collapsed = true;
    if (work.value.point_count == work.value.points.size())
    {
        work.uncertain = true;
        return false;
    }
    work.value.points[work.value.point_count++] = public_point(candidate);
    return true;
}

void finish_relation(PairWork& work) noexcept
{
    if (work.value.point_count == 2)
    {
        const AnalyticFilteredPointNm& first = work.value.points[0];
        const AnalyticFilteredPointNm& second = work.value.points[1];
        const bool already_ordered =
            first.x.upper < second.x.lower ||
            (!(second.x.upper < first.x.lower) && first.y.upper < second.y.lower);
        const bool reverse_ordered =
            second.x.upper < first.x.lower ||
            (!(first.x.upper < second.x.lower) && second.y.upper < first.y.lower);
        if (reverse_ordered)
            std::swap(work.value.points[0], work.value.points[1]);
        else if (!already_ordered)
        {
            work.uncertain = true;
            return;
        }
    }
    work.value.relation = work.value.point_count == 0   ? AnalyticPairRelation::disjoint
                          : work.value.point_count == 1 ? AnalyticPairRelation::point
                                                        : AnalyticPairRelation::two_points;
}

std::uint8_t shared_authoritative_endpoints(const AnalyticAtomicCurveNm& left,
                                            const AnalyticAtomicCurveNm& right,
                                            AnalyticIntegerPointNm (&output)[2]) noexcept
{
    const bool left_has_exact_endpoints =
        left.has_integer_certificate || left.has_endpoint_authoritative_arc_certificate;
    const bool right_has_exact_endpoints =
        right.has_integer_certificate || right.has_endpoint_authoritative_arc_certificate;
    if (!left_has_exact_endpoints || !right_has_exact_endpoints)
        return 0;
    std::uint8_t count = 0;
    const AnalyticIntegerPointNm left_points[] = {left.integer_start, left.integer_end};
    const AnalyticIntegerPointNm right_points[] = {right.integer_start, right.integer_end};
    for (const AnalyticIntegerPointNm& a : left_points)
        for (const AnalyticIntegerPointNm& b : right_points)
            if (same_point(a, b))
            {
                if (count == 0 || !same_point(output[0], a))
                    output[count++] = a;
                break;
            }
    return count;
}

std::uint64_t tangent_token_at(const AnalyticAtomicCurveNm& curve,
                               AnalyticIntegerPointNm endpoint) noexcept
{
    if (same_point(curve.integer_start, endpoint))
        return curve.construction_start_tangent_id;
    if (same_point(curve.integer_end, endpoint))
        return curve.construction_end_tangent_id;
    return 0;
}

bool append_authoritative_point(PairWork& work, Point candidate) noexcept
{
    if (work.value.point_count == work.value.points.size())
        return false;
    work.value.points[work.value.point_count++] = public_point(candidate);
    return true;
}

bool append_strict_second_root(PairWork& work, Point candidate, const AnalyticAtomicCurveNm& left,
                               const AnalyticAtomicCurveNm& right,
                               AnalyticNarrowPhaseTelemetry& telemetry,
                               const AnalyticSolverLimits& limits) noexcept
{
    if (!valid_interval(candidate.x) || !valid_interval(candidate.y))
        return false;
    const DomainResult left_domain = curve_domain(
        candidate, left, telemetry, limits, analytic_execution_detail::kStrictPublishedGeometry);
    const DomainResult right_domain = curve_domain(
        candidate, right, telemetry, limits, analytic_execution_detail::kStrictPublishedGeometry);
    if (left_domain == DomainResult::outside || right_domain == DomainResult::outside)
        return true;
    // A broad algebraic root enclosure is harmless when a complete finite-
    // domain predicate has already excluded it. Width is relevant only for a
    // root that may survive on both finite curves.
    if (!point_interval_fits_resolution(candidate))
        return false;
    if (left_domain == DomainResult::uncertain || right_domain == DomainResult::uncertain ||
        left_domain == DomainResult::inside_resolution ||
        right_domain == DomainResult::inside_resolution ||
        left_domain == DomainResult::ambiguous_resolution ||
        right_domain == DomainResult::ambiguous_resolution)
        return false;
    return left_domain == DomainResult::inside && right_domain == DomainResult::inside &&
           append_authoritative_point(work, candidate);
}

PairWork intersect_endpoint_authoritative(const AnalyticAtomicCurveNm& left,
                                          const AnalyticAtomicCurveNm& right,
                                          AnalyticNarrowPhaseTelemetry& telemetry,
                                          const AnalyticSolverLimits& limits,
                                          analytic_execution_detail::TopologyPolicy policy) noexcept
{
    PairWork work;
    AnalyticIntegerPointNm shared[2]{};
    const std::uint8_t shared_count = shared_authoritative_endpoints(left, right, shared);
    if (shared_count == 0)
    {
        work.uncertain = true;
        return work;
    }
    for (std::uint8_t index = 0; index < shared_count; ++index)
        if (!append_authoritative_point(work, point(shared[index])))
        {
            work.uncertain = true;
            return work;
        }
    if (shared_count == 2)
    {
        finish_relation(work);
        return work;
    }
    if (!charge_predicate(telemetry, limits))
    {
        work.uncertain = true;
        return work;
    }

    const Point anchor = point(shared[0]);
    const std::uint64_t left_tangent = tangent_token_at(left, shared[0]);
    const std::uint64_t right_tangent = tangent_token_at(right, shared[0]);
    const bool certified_tangent =
        left_tangent != 0 && left_tangent == right_tangent &&
        (left.kind != right.kind || (left.kind == AnalyticAtomicCurveKind::circular_arc &&
                                     analytic_is_circle_endpoint_tangent_token(left_tangent)));
    Point direction;
    Point center;
    if (left.kind == AnalyticAtomicCurveKind::line || right.kind == AnalyticAtomicCurveKind::line)
    {
        const AnalyticAtomicCurveNm& line =
            left.kind == AnalyticAtomicCurveKind::line ? left : right;
        const AnalyticAtomicCurveNm& arc =
            left.kind == AnalyticAtomicCurveKind::circular_arc ? left : right;
        direction = subtract(point(line.end), point(line.start));
        center = point(arc.circle.center);
    }
    else
    {
        const Point first_center = point(left.circle.center);
        const Point second_center = point(right.circle.center);
        direction = perpendicular(subtract(second_center, first_center));
        center = first_center;
    }
    const Interval denominator = dot(direction, direction);
    if (denominator.lower <= 0.0)
    {
        work.uncertain = true;
        return work;
    }
    const Interval parameter =
        divide(multiply(exact(-2.0), dot(direction, subtract(anchor, center))), denominator);
    if (!valid(parameter))
    {
        work.uncertain = true;
        return work;
    }
    const Point second = add(anchor, scale(direction, parameter));
    const Point delta = subtract(second, anchor);
    const Interval separation_squared = dot(delta, delta);
    if (!valid(separation_squared))
    {
        work.uncertain = true;
        return work;
    }
    if (separation_squared.lower == 0.0 && separation_squared.upper == 0.0)
    {
        finish_relation(work);
        return work;
    }
    constexpr double kResolutionSquared =
        static_cast<double>(kAnalyticTopologyResolutionNm * kAnalyticTopologyResolutionNm);
    if (certified_tangent && separation_squared.upper <= kResolutionSquared)
    {
        finish_relation(work);
        return work;
    }
    if (!append_strict_second_root(work, second, left, right, telemetry, limits))
        work.uncertain = true;
    finish_relation(work);
    return work;
}

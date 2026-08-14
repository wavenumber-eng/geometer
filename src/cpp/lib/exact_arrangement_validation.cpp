#include "exact_arrangement_internal.h"

namespace geometer::exact::arrangement_detail
{
namespace
{

ExactPredicateResult contains(ConstructionArena& arena, const ExactAtomicCurve& curve,
                              const ExactPoint& point)
{
    if (curve.kind == ExactAtomicCurveKind::line)
        return point_on_closed_exact_segment(arena, {curve.start, curve.end}, point);
    return point_on_closed_exact_arc(
        arena, {curve.circle, curve.start, curve.end, curve.counterclockwise, curve.major_arc},
        point);
}

ExactPredicateResult is_endpoint(ConstructionArena& arena, const ExactAtomicCurve& curve,
                                 const ExactPoint& point)
{
    const ExactPredicateResult start = exact_points_equal(arena, curve.start, point);
    if (start.error != Error::none || !start.value || *start.value)
        return start;
    return exact_points_equal(arena, curve.end, point);
}

ExactPredicateResult same_endpoints(ConstructionArena& arena, const ExactAtomicCurve& left,
                                    const ExactAtomicCurve& right, bool& reversed)
{
    const ExactPredicateResult first_first = exact_points_equal(arena, left.start, right.start);
    if (first_first.error != Error::none || !first_first.value)
        return first_first;
    const ExactPredicateResult second_second = exact_points_equal(arena, left.end, right.end);
    if (second_second.error != Error::none || !second_second.value)
        return second_second;
    if (*first_first.value && *second_second.value)
    {
        reversed = false;
        return {Error::none, true};
    }
    const ExactPredicateResult first_second = exact_points_equal(arena, left.start, right.end);
    if (first_second.error != Error::none || !first_second.value)
        return first_second;
    const ExactPredicateResult second_first = exact_points_equal(arena, left.end, right.start);
    if (second_first.error != Error::none || !second_first.value)
        return second_first;
    reversed = *first_second.value && *second_first.value;
    return {Error::none, reversed};
}

Error validate_same_carrier(ConstructionArena& arena, const ExactAtomicCurve& left,
                            const ExactAtomicCurve& right)
{
    bool reversed = false;
    const ExactPredicateResult endpoints = same_endpoints(arena, left, right, reversed);
    if (endpoints.error != Error::none || !endpoints.value)
        return endpoints.error == Error::none ? Error::invalid_argument : endpoints.error;
    if (*endpoints.value)
    {
        if (left.kind == ExactAtomicCurveKind::line)
            return Error::none;
        const bool same_direction = reversed ? left.counterclockwise != right.counterclockwise
                                             : left.counterclockwise == right.counterclockwise;
        if (same_direction && left.major_arc == right.major_arc)
            return Error::none;
    }

    for (const ExactPoint point : {left.start, left.end})
    {
        const ExactPredicateResult in_right = contains(arena, right, point);
        const ExactPredicateResult right_endpoint = is_endpoint(arena, right, point);
        if (in_right.error != Error::none || right_endpoint.error != Error::none ||
            !in_right.value || !right_endpoint.value)
            return in_right.error != Error::none ? in_right.error : right_endpoint.error;
        if (*in_right.value && !*right_endpoint.value)
            return Error::invalid_argument;
    }
    for (const ExactPoint point : {right.start, right.end})
    {
        const ExactPredicateResult in_left = contains(arena, left, point);
        const ExactPredicateResult left_endpoint = is_endpoint(arena, left, point);
        if (in_left.error != Error::none || left_endpoint.error != Error::none || !in_left.value ||
            !left_endpoint.value)
            return in_left.error != Error::none ? in_left.error : left_endpoint.error;
        if (*in_left.value && !*left_endpoint.value)
            return Error::invalid_argument;
    }
    return Error::none;
}

Error validate_finite_intersections(ConstructionArena& arena, const ExactAtomicCurve& left,
                                    const ExactAtomicCurve& right,
                                    const ExactIntersectionResult& intersections)
{
    if (intersections.error != Error::none)
        return intersections.error;
    for (std::uint32_t index = 0; index < intersections.point_count; ++index)
    {
        const ExactPoint& point = intersections.points[index];
        const ExactPredicateResult in_left = contains(arena, left, point);
        const ExactPredicateResult in_right = contains(arena, right, point);
        if (in_left.error != Error::none || in_right.error != Error::none || !in_left.value ||
            !in_right.value)
            return in_left.error != Error::none ? in_left.error : in_right.error;
        if (!*in_left.value || !*in_right.value)
            continue;
        const ExactPredicateResult left_endpoint = is_endpoint(arena, left, point);
        const ExactPredicateResult right_endpoint = is_endpoint(arena, right, point);
        if (left_endpoint.error != Error::none || right_endpoint.error != Error::none ||
            !left_endpoint.value || !right_endpoint.value)
            return left_endpoint.error != Error::none ? left_endpoint.error : right_endpoint.error;
        if (!*left_endpoint.value || !*right_endpoint.value)
            return Error::invalid_argument;
    }
    return Error::none;
}

Error validate_pair(ConstructionArena& arena, const ExactAtomicCurve& left,
                    const ExactAtomicCurve& right)
{
    if (left.kind == ExactAtomicCurveKind::line && right.kind == ExactAtomicCurveKind::line)
    {
        const ExactPredicateResult same =
            same_exact_line_carrier(arena, {left.start, left.end}, {right.start, right.end});
        if (same.error != Error::none || !same.value)
            return same.error == Error::none ? Error::invalid_argument : same.error;
        if (*same.value)
            return validate_same_carrier(arena, left, right);
        return validate_finite_intersections(
            arena, left, right,
            intersect_exact_lines(arena, {left.start, left.end}, {right.start, right.end}));
    }
    if (left.kind == ExactAtomicCurveKind::circular_arc &&
        right.kind == ExactAtomicCurveKind::circular_arc)
    {
        const ExactPredicateResult same =
            same_exact_circle_carrier(arena, left.circle, right.circle);
        if (same.error != Error::none || !same.value)
            return same.error == Error::none ? Error::invalid_argument : same.error;
        if (*same.value)
            return validate_same_carrier(arena, left, right);
        return validate_finite_intersections(
            arena, left, right, intersect_exact_circles(arena, left.circle, right.circle));
    }
    const ExactAtomicCurve& line = left.kind == ExactAtomicCurveKind::line ? left : right;
    const ExactAtomicCurve& arc = left.kind == ExactAtomicCurveKind::circular_arc ? left : right;
    return validate_finite_intersections(
        arena, left, right, intersect_exact_line_circle(arena, {line.start, line.end}, arc.circle));
}

} // namespace

Error validate_atomic_pairs(ConstructionArena& arena, const std::vector<ExactAtomicCurve>& curves)
{
    for (std::size_t left = 0; left < curves.size(); ++left)
        for (std::size_t right = left + 1; right < curves.size(); ++right)
            if (const Error error = validate_pair(arena, curves[left], curves[right]);
                error != Error::none)
                return error;
    return Error::none;
}

} // namespace geometer::exact::arrangement_detail

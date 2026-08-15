#include "geometer/analytic_curve_narrow_phase.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <new>
#include <utility>

#if defined(_MSC_VER) && defined(_M_X64)
#include <intrin.h>
#endif

namespace geometer
{

namespace
{

constexpr std::int64_t kLocalCoordinateSpanNm = 1'000'000'000'000;
constexpr std::uint64_t kPairLogicalBytes = 256;

struct Interval
{
    double lower = 0.0;
    double upper = 0.0;
};

struct Point
{
    Interval x;
    Interval y;
};

enum class DomainResult : std::uint8_t
{
    outside,
    inside,
    inside_resolution,
    uncertain,
};

struct PairWork
{
    AnalyticPairIntersection value;
    bool uncertain = false;
};

#if defined(_MSC_VER) && defined(_M_X64)
struct SignedWide
{
    std::uint64_t low = 0;
    std::int64_t high = 0;
};

SignedWide wide_multiply(std::int64_t left, std::int64_t right) noexcept
{
    std::int64_t high = 0;
    const std::int64_t low = _mul128(left, right, &high);
    return {static_cast<std::uint64_t>(low), high};
}

SignedWide wide_add(SignedWide left, SignedWide right) noexcept
{
    const std::uint64_t low = left.low + right.low;
    const std::uint64_t carry = low < left.low ? 1U : 0U;
    return {low, static_cast<std::int64_t>(static_cast<std::uint64_t>(left.high) +
                                           static_cast<std::uint64_t>(right.high) + carry)};
}

SignedWide wide_subtract(SignedWide left, SignedWide right) noexcept
{
    const std::uint64_t low = left.low - right.low;
    const std::uint64_t borrow = left.low < right.low ? 1U : 0U;
    return {low, static_cast<std::int64_t>(static_cast<std::uint64_t>(left.high) -
                                           static_cast<std::uint64_t>(right.high) - borrow)};
}

int wide_sign(SignedWide value) noexcept
{
    if (value.high < 0)
        return -1;
    if (value.high > 0 || value.low != 0)
        return 1;
    return 0;
}

int wide_compare(SignedWide left, SignedWide right) noexcept
{
    return wide_sign(wide_subtract(left, right));
}
#else
using SignedWide = __int128;

SignedWide wide_multiply(std::int64_t left, std::int64_t right) noexcept
{
    return static_cast<SignedWide>(left) * static_cast<SignedWide>(right);
}

SignedWide wide_add(SignedWide left, SignedWide right) noexcept
{
    return left + right;
}

SignedWide wide_subtract(SignedWide left, SignedWide right) noexcept
{
    return left - right;
}

int wide_sign(SignedWide value) noexcept
{
    return value < 0 ? -1 : value > 0 ? 1 : 0;
}

int wide_compare(SignedWide left, SignedWide right) noexcept
{
    return wide_sign(left - right);
}
#endif

double downward(double value) noexcept
{
    return std::nextafter(value, -std::numeric_limits<double>::infinity());
}

double upward(double value) noexcept
{
    return std::nextafter(value, std::numeric_limits<double>::infinity());
}

Interval exact(double value) noexcept
{
    return {value, value};
}

bool singleton(Interval value) noexcept
{
    return value.lower == value.upper;
}

Interval add(Interval left, Interval right) noexcept
{
    if (singleton(left) && singleton(right))
    {
        const double sum = left.lower + right.lower;
        const double right_virtual = sum - left.lower;
        const double residual =
            (left.lower - (sum - right_virtual)) + (right.lower - right_virtual);
        if (residual == 0.0)
            return exact(sum);
        return {downward(sum + residual), upward(sum + residual)};
    }
    return {downward(left.lower + right.lower), upward(left.upper + right.upper)};
}

Interval negate(Interval value) noexcept
{
    return {-value.upper, -value.lower};
}

Interval subtract(Interval left, Interval right) noexcept
{
    return add(left, negate(right));
}

Interval multiply_singletons(double left, double right) noexcept
{
    const double product = left * right;
    const double residual = std::fma(left, right, -product);
    if (residual == 0.0)
        return exact(product);
    return {downward(product + residual), upward(product + residual)};
}

Interval multiply(Interval left, Interval right) noexcept
{
    if (singleton(left) && singleton(right))
        return multiply_singletons(left.lower, right.lower);
    const double products[] = {left.lower * right.lower, left.lower * right.upper,
                               left.upper * right.lower, left.upper * right.upper};
    return {downward(*std::min_element(std::begin(products), std::end(products))),
            upward(*std::max_element(std::begin(products), std::end(products)))};
}

Interval divide(Interval numerator, Interval denominator) noexcept
{
    if (denominator.lower <= 0.0 && denominator.upper >= 0.0)
        return {-std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()};
    if (singleton(numerator) && singleton(denominator))
    {
        const double quotient = numerator.lower / denominator.lower;
        if (std::fma(quotient, denominator.lower, -numerator.lower) == 0.0)
            return exact(quotient);
        return {downward(quotient), upward(quotient)};
    }
    const double quotients[] = {
        numerator.lower / denominator.lower, numerator.lower / denominator.upper,
        numerator.upper / denominator.lower, numerator.upper / denominator.upper};
    return {downward(*std::min_element(std::begin(quotients), std::end(quotients))),
            upward(*std::max_element(std::begin(quotients), std::end(quotients)))};
}

Interval square(Interval value) noexcept
{
    if (value.lower >= 0.0)
        return multiply(value, value);
    if (value.upper <= 0.0)
        return multiply(negate(value), negate(value));
    const double maximum = std::max(value.lower * value.lower, value.upper * value.upper);
    return {0.0, upward(maximum)};
}

Interval square_root(Interval value) noexcept
{
    const double lower_target = std::max(0.0, value.lower);
    const double upper_target = std::max(0.0, value.upper);
    double lower = std::sqrt(lower_target);
    double upper = std::sqrt(upper_target);
    for (int step = 0; step < 8 && std::fma(lower, lower, -lower_target) > 0.0; ++step)
        lower = downward(lower);
    for (int step = 0; step < 8 && std::fma(upper, upper, -upper_target) < 0.0; ++step)
        upper = upward(upper);
    if (std::fma(lower, lower, -lower_target) > 0.0 || std::fma(upper, upper, -upper_target) < 0.0)
        return {0.0, std::numeric_limits<double>::infinity()};
    return {lower, upper};
}

Interval dot(Point left, Point right) noexcept
{
    return add(multiply(left.x, right.x), multiply(left.y, right.y));
}

Interval cross(Point left, Point right) noexcept
{
    return subtract(multiply(left.x, right.y), multiply(left.y, right.x));
}

Point add(Point left, Point right) noexcept
{
    return {add(left.x, right.x), add(left.y, right.y)};
}

Point subtract(Point left, Point right) noexcept
{
    return {subtract(left.x, right.x), subtract(left.y, right.y)};
}

Point scale(Point point, Interval scalar) noexcept
{
    return {multiply(point.x, scalar), multiply(point.y, scalar)};
}

Point perpendicular(Point point) noexcept
{
    return {negate(point.y), point.x};
}

Point point(AnalyticIntegerPointNm value) noexcept
{
    return {{static_cast<double>(value.x), static_cast<double>(value.x)},
            {static_cast<double>(value.y), static_cast<double>(value.y)}};
}

AnalyticFilteredPointNm public_point(Point value) noexcept
{
    return {{value.x.lower, value.x.upper}, {value.y.lower, value.y.upper}};
}

bool valid_interval(Interval value) noexcept
{
    return std::isfinite(value.lower) && std::isfinite(value.upper) && value.lower <= value.upper;
}

std::int64_t difference(std::int64_t left, std::int64_t right) noexcept
{
    return left - right;
}

SignedWide exact_cross(AnalyticIntegerPointNm first, AnalyticIntegerPointNm second,
                       AnalyticIntegerPointNm third) noexcept
{
    const std::int64_t first_x = difference(second.x, first.x);
    const std::int64_t first_y = difference(second.y, first.y);
    const std::int64_t second_x = difference(third.x, first.x);
    const std::int64_t second_y = difference(third.y, first.y);
    return wide_subtract(wide_multiply(first_x, second_y), wide_multiply(first_y, second_x));
}

SignedWide exact_squared_distance(AnalyticIntegerPointNm left,
                                  AnalyticIntegerPointNm right) noexcept
{
    const std::int64_t dx = difference(left.x, right.x);
    const std::int64_t dy = difference(left.y, right.y);
    return wide_add(wide_multiply(dx, dx), wide_multiply(dy, dy));
}

SignedWide exact_dot_from(AnalyticIntegerPointNm origin, AnalyticIntegerPointNm left,
                          AnalyticIntegerPointNm right) noexcept
{
    const std::int64_t left_x = difference(left.x, origin.x);
    const std::int64_t left_y = difference(left.y, origin.y);
    const std::int64_t right_x = difference(right.x, origin.x);
    const std::int64_t right_y = difference(right.y, origin.y);
    return wide_add(wide_multiply(left_x, right_x), wide_multiply(left_y, right_y));
}

SignedWide exact_squared_radius(std::uint64_t radius) noexcept
{
    return wide_multiply(static_cast<std::int64_t>(radius), static_cast<std::int64_t>(radius));
}

bool same_point(AnalyticIntegerPointNm left, AnalyticIntegerPointNm right) noexcept
{
    return left.x == right.x && left.y == right.y;
}

bool coordinate_in_span(std::int64_t value) noexcept
{
    return value >= -kLocalCoordinateSpanNm && value <= kLocalCoordinateSpanNm;
}

bool valid_curve(const AnalyticAtomicCurveNm& curve) noexcept
{
    if (curve.curve_index == 0 || !coordinate_in_span(curve.start.x) ||
        !coordinate_in_span(curve.start.y) || !coordinate_in_span(curve.end.x) ||
        !coordinate_in_span(curve.end.y) || same_point(curve.start, curve.end))
        return false;
    if (curve.kind == AnalyticAtomicCurveKind::line)
        return true;
    if (curve.kind != AnalyticAtomicCurveKind::circular_arc || curve.circle.radius == 0 ||
        curve.circle.radius > static_cast<std::uint64_t>(kLocalCoordinateSpanNm) ||
        !coordinate_in_span(curve.circle.center.x) || !coordinate_in_span(curve.circle.center.y))
        return false;
    const SignedWide radius_squared = exact_squared_radius(curve.circle.radius);
    if (wide_compare(exact_squared_distance(curve.start, curve.circle.center), radius_squared) !=
            0 ||
        wide_compare(exact_squared_distance(curve.end, curve.circle.center), radius_squared) != 0)
        return false;
    int orientation = wide_sign(exact_cross(curve.circle.center, curve.start, curve.end));
    if (!curve.counterclockwise)
        orientation = -orientation;
    if (orientation == 0)
        return !curve.major_arc &&
               wide_sign(exact_dot_from(curve.circle.center, curve.start, curve.end)) < 0;
    return curve.major_arc ? orientation < 0 : orientation > 0;
}

bool charge_predicate(AnalyticNarrowPhaseTelemetry& telemetry, const AnalyticSolverLimits& limits,
                      bool domain = false) noexcept
{
    if (telemetry.predicate_calls == limits.predicate_calls)
        return false;
    ++telemetry.predicate_calls;
    if (domain)
        ++telemetry.domain_predicates;
    else
        ++telemetry.carrier_predicates;
    return true;
}

bool interval_within_resolution(Point candidate, AnalyticIntegerPointNm endpoint) noexcept
{
    const double x = static_cast<double>(endpoint.x);
    const double y = static_cast<double>(endpoint.y);
    const double dx = std::max(std::fabs(candidate.x.lower - x), std::fabs(candidate.x.upper - x));
    const double dy = std::max(std::fabs(candidate.y.lower - y), std::fabs(candidate.y.upper - y));
    const Interval x_squared =
        singleton(candidate.x) ? square(exact(dx)) : square({0.0, upward(dx)});
    const Interval y_squared =
        singleton(candidate.y) ? square(exact(dy)) : square({0.0, upward(dy)});
    const Interval radial_squared = add(x_squared, y_squared);
    constexpr double resolution_squared =
        static_cast<double>(kAnalyticTopologyResolutionNm * kAnalyticTopologyResolutionNm);
    return radial_squared.upper <= resolution_squared;
}

bool interval_is_exact_point(Point candidate, AnalyticIntegerPointNm endpoint) noexcept
{
    return singleton(candidate.x) && singleton(candidate.y) &&
           candidate.x.lower == static_cast<double>(endpoint.x) &&
           candidate.y.lower == static_cast<double>(endpoint.y);
}

bool point_interval_fits_resolution(Point candidate) noexcept
{
    const double half_width_x = upward((candidate.x.upper - candidate.x.lower) * 0.5);
    const double half_width_y = upward((candidate.y.upper - candidate.y.lower) * 0.5);
    const Interval radius_squared = add(square({0.0, half_width_x}), square({0.0, half_width_y}));
    constexpr double resolution_squared =
        static_cast<double>(kAnalyticTopologyResolutionNm * kAnalyticTopologyResolutionNm);
    return radius_squared.upper <= resolution_squared;
}

Interval measured_square_root(Interval value, AnalyticNarrowPhaseTelemetry& telemetry) noexcept
{
    ++telemetry.square_root_calls;
    return square_root(value);
}

bool circle_boundary_within_resolution(Point candidate, const AnalyticIntegerCircleNm& circle,
                                       AnalyticNarrowPhaseTelemetry& telemetry) noexcept
{
    const Point radial = subtract(candidate, point(circle.center));
    const Interval distance = measured_square_root(dot(radial, radial), telemetry);
    const double radius = static_cast<double>(circle.radius);
    const double maximum_gap =
        std::max(std::fabs(distance.lower - radius), std::fabs(distance.upper - radius));
    return maximum_gap <= static_cast<double>(kAnalyticTopologyResolutionNm);
}

DomainResult line_domain(Point candidate, const AnalyticAtomicCurveNm& line,
                         AnalyticNarrowPhaseTelemetry& telemetry,
                         const AnalyticSolverLimits& limits) noexcept
{
    if (!charge_predicate(telemetry, limits, true))
        return DomainResult::uncertain;
    if (interval_is_exact_point(candidate, line.start) ||
        interval_is_exact_point(candidate, line.end))
        return DomainResult::inside;
    if (interval_within_resolution(candidate, line.start) ||
        interval_within_resolution(candidate, line.end))
        return DomainResult::inside_resolution;
    const Point start = point(line.start);
    const Point direction = subtract(point(line.end), start);
    const Interval parameter =
        divide(dot(subtract(candidate, start), direction), dot(direction, direction));
    if (parameter.upper < 0.0 || parameter.lower > 1.0)
        return DomainResult::outside;
    if (parameter.lower >= 0.0 && parameter.upper <= 1.0)
        return DomainResult::inside;
    return DomainResult::uncertain;
}

DomainResult arc_domain(Point candidate, const AnalyticAtomicCurveNm& arc,
                        AnalyticNarrowPhaseTelemetry& telemetry,
                        const AnalyticSolverLimits& limits) noexcept
{
    if (!charge_predicate(telemetry, limits, true))
        return DomainResult::uncertain;
    if (interval_is_exact_point(candidate, arc.start) ||
        interval_is_exact_point(candidate, arc.end))
        return DomainResult::inside;
    if (interval_within_resolution(candidate, arc.start) ||
        interval_within_resolution(candidate, arc.end))
        return DomainResult::inside_resolution;

    const Point center = point(arc.circle.center);
    const Point start = subtract(point(arc.start), center);
    const Point end = subtract(point(arc.end), center);
    const Point radial = subtract(candidate, center);
    Interval from_start = cross(start, radial);
    Interval to_end = cross(radial, end);
    if (!arc.counterclockwise)
    {
        from_start = negate(from_start);
        to_end = negate(to_end);
    }
    if (!arc.major_arc)
    {
        if (from_start.upper < 0.0 || to_end.upper < 0.0)
            return DomainResult::outside;
        if (from_start.lower > 0.0 && to_end.lower > 0.0)
            return DomainResult::inside;
    }
    else
    {
        if (from_start.lower > 0.0 || to_end.lower > 0.0)
            return DomainResult::inside;
        if (from_start.upper < 0.0 && to_end.upper < 0.0)
            return DomainResult::outside;
    }
    return DomainResult::uncertain;
}

DomainResult curve_domain(Point candidate, const AnalyticAtomicCurveNm& curve,
                          AnalyticNarrowPhaseTelemetry& telemetry,
                          const AnalyticSolverLimits& limits) noexcept
{
    return curve.kind == AnalyticAtomicCurveKind::line
               ? line_domain(candidate, curve, telemetry, limits)
               : arc_domain(candidate, curve, telemetry, limits);
}

bool retain_point(PairWork& work, Point candidate, const AnalyticAtomicCurveNm& left,
                  const AnalyticAtomicCurveNm& right, AnalyticNarrowPhaseTelemetry& telemetry,
                  const AnalyticSolverLimits& limits) noexcept
{
    if (!valid_interval(candidate.x) || !valid_interval(candidate.y) ||
        !point_interval_fits_resolution(candidate))
    {
        work.uncertain = true;
        return false;
    }
    const DomainResult left_domain = curve_domain(candidate, left, telemetry, limits);
    const DomainResult right_domain = curve_domain(candidate, right, telemetry, limits);
    if (left_domain == DomainResult::uncertain || right_domain == DomainResult::uncertain)
    {
        work.uncertain = true;
        return false;
    }
    if (left_domain == DomainResult::outside || right_domain == DomainResult::outside)
        return true;
    if (left_domain == DomainResult::inside_resolution ||
        right_domain == DomainResult::inside_resolution)
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

AnalyticIntegerPointNm endpoint_at_projection(const AnalyticAtomicCurveNm& curve,
                                              std::int64_t projection, bool use_x) noexcept
{
    const std::int64_t first = use_x ? curve.start.x : curve.start.y;
    return first == projection ? curve.start : curve.end;
}

PairWork intersect_lines(const AnalyticAtomicCurveNm& left, const AnalyticAtomicCurveNm& right,
                         AnalyticNarrowPhaseTelemetry& telemetry,
                         const AnalyticSolverLimits& limits) noexcept
{
    PairWork work;
    if (!charge_predicate(telemetry, limits))
    {
        work.uncertain = true;
        return work;
    }
    const int denominator_sign = wide_sign(exact_cross(
        {0, 0}, {difference(left.end.x, left.start.x), difference(left.end.y, left.start.y)},
        {difference(right.end.x, right.start.x), difference(right.end.y, right.start.y)}));
    if (denominator_sign == 0)
    {
        if (wide_sign(exact_cross(left.start, left.end, right.start)) != 0)
            return work;
        const std::int64_t dx = difference(left.end.x, left.start.x);
        const std::int64_t dy = difference(left.end.y, left.start.y);
        const bool use_x = std::llabs(dx) >= std::llabs(dy);
        const std::int64_t left_first = use_x ? left.start.x : left.start.y;
        const std::int64_t left_second = use_x ? left.end.x : left.end.y;
        const std::int64_t right_first = use_x ? right.start.x : right.start.y;
        const std::int64_t right_second = use_x ? right.end.x : right.end.y;
        const std::int64_t overlap_start =
            std::max(std::min(left_first, left_second), std::min(right_first, right_second));
        const std::int64_t overlap_end =
            std::min(std::max(left_first, left_second), std::max(right_first, right_second));
        if (overlap_end < overlap_start)
            return work;
        if (overlap_end == overlap_start)
        {
            const AnalyticIntegerPointNm intersection =
                (overlap_start == left_first || overlap_start == left_second)
                    ? endpoint_at_projection(left, overlap_start, use_x)
                    : endpoint_at_projection(right, overlap_start, use_x);
            work.value.points[0] = public_point(point(intersection));
            work.value.point_count = 1;
            work.value.relation = AnalyticPairRelation::point;
            return work;
        }
        work.value.relation = AnalyticPairRelation::coincident;
        return work;
    }

    const Point first = point(left.start);
    const Point left_direction = subtract(point(left.end), first);
    const Point second = point(right.start);
    const Point right_direction = subtract(point(right.end), second);
    const Point offset = subtract(second, first);
    const Interval denominator = cross(left_direction, right_direction);
    if (denominator.lower <= 0.0 && denominator.upper >= 0.0)
    {
        work.uncertain = true;
        return work;
    }
    const Interval left_parameter = divide(cross(offset, right_direction), denominator);
    const Point intersection = add(first, scale(left_direction, left_parameter));
    retain_point(work, intersection, left, right, telemetry, limits);
    finish_relation(work);
    return work;
}

PairWork intersect_line_circle(const AnalyticAtomicCurveNm& line, const AnalyticAtomicCurveNm& arc,
                               AnalyticNarrowPhaseTelemetry& telemetry,
                               const AnalyticSolverLimits& limits) noexcept
{
    PairWork work;
    if (!charge_predicate(telemetry, limits))
    {
        work.uncertain = true;
        return work;
    }
    const Point start = point(line.start);
    const Point direction = subtract(point(line.end), start);
    const Point center = point(arc.circle.center);
    const Point center_offset = subtract(center, start);
    const Interval length_squared = dot(direction, direction);
    const Interval along = divide(dot(center_offset, direction), length_squared);
    const Point base = add(start, scale(direction, along));
    const Interval distance_squared = dot(subtract(base, center), subtract(base, center));
    const Interval radius_squared = square(exact(static_cast<double>(arc.circle.radius)));
    const Interval height_squared = subtract(radius_squared, distance_squared);
    if (height_squared.upper < 0.0)
        return work;

    if (height_squared.lower <= 0.0)
    {
        if (!circle_boundary_within_resolution(base, arc.circle, telemetry))
        {
            work.uncertain = true;
            return work;
        }
        work.value.resolution_collapsed = true;
        retain_point(work, base, line, arc, telemetry, limits);
        finish_relation(work);
        return work;
    }

    const Interval scale_value =
        measured_square_root(divide(height_squared, length_squared), telemetry);
    const Point displacement = scale(direction, scale_value);
    const double maximum_height = measured_square_root(height_squared, telemetry).upper;
    if (2.0 * maximum_height <= static_cast<double>(kAnalyticTopologyResolutionNm))
    {
        if (!circle_boundary_within_resolution(base, arc.circle, telemetry))
        {
            work.uncertain = true;
            return work;
        }
        work.value.resolution_collapsed = true;
        retain_point(work, base, line, arc, telemetry, limits);
    }
    else
    {
        retain_point(work, subtract(base, displacement), line, arc, telemetry, limits);
        if (!work.uncertain)
            retain_point(work, add(base, displacement), line, arc, telemetry, limits);
    }
    finish_relation(work);
    return work;
}

PairWork intersect_circles(const AnalyticAtomicCurveNm& left, const AnalyticAtomicCurveNm& right,
                           AnalyticNarrowPhaseTelemetry& telemetry,
                           const AnalyticSolverLimits& limits) noexcept
{
    PairWork work;
    if (!charge_predicate(telemetry, limits))
    {
        work.uncertain = true;
        return work;
    }
    const SignedWide distance_squared_exact =
        exact_squared_distance(left.circle.center, right.circle.center);
    if (wide_sign(distance_squared_exact) == 0)
    {
        work.value.relation = left.circle.radius == right.circle.radius
                                  ? AnalyticPairRelation::coincident
                                  : AnalyticPairRelation::disjoint;
        return work;
    }
    const std::int64_t radius_sum = static_cast<std::int64_t>(left.circle.radius) +
                                    static_cast<std::int64_t>(right.circle.radius);
    const std::int64_t radius_difference = static_cast<std::int64_t>(left.circle.radius) -
                                           static_cast<std::int64_t>(right.circle.radius);
    const SignedWide sum_squared = wide_multiply(radius_sum, radius_sum);
    const SignedWide difference_squared = wide_multiply(radius_difference, radius_difference);
    if (wide_compare(distance_squared_exact, sum_squared) > 0 ||
        wide_compare(distance_squared_exact, difference_squared) < 0)
        return work;
    const bool exact_tangent = wide_compare(distance_squared_exact, sum_squared) == 0 ||
                               wide_compare(distance_squared_exact, difference_squared) == 0;

    const Point first_center = point(left.circle.center);
    const Point center_delta = subtract(point(right.circle.center), first_center);
    const Interval distance_squared = dot(center_delta, center_delta);
    const Interval left_radius_squared = square(exact(static_cast<double>(left.circle.radius)));
    const Interval right_radius_squared = square(exact(static_cast<double>(right.circle.radius)));
    const Interval numerator =
        add(subtract(left_radius_squared, right_radius_squared), distance_squared);
    const Interval along = divide(numerator, multiply(exact(2.0), distance_squared));
    const Point base = add(first_center, scale(center_delta, along));
    const Interval height_squared =
        subtract(left_radius_squared, multiply(square(along), distance_squared));
    if (exact_tangent || height_squared.lower <= 0.0)
    {
        if (!circle_boundary_within_resolution(base, left.circle, telemetry) ||
            !circle_boundary_within_resolution(base, right.circle, telemetry))
        {
            work.uncertain = true;
            return work;
        }
        work.value.resolution_collapsed = true;
        retain_point(work, base, left, right, telemetry, limits);
        finish_relation(work);
        return work;
    }

    const Interval scale_value =
        measured_square_root(divide(height_squared, distance_squared), telemetry);
    const Point displacement = scale(perpendicular(center_delta), scale_value);
    const double maximum_height = measured_square_root(height_squared, telemetry).upper;
    if (2.0 * maximum_height <= static_cast<double>(kAnalyticTopologyResolutionNm))
    {
        if (!circle_boundary_within_resolution(base, left.circle, telemetry) ||
            !circle_boundary_within_resolution(base, right.circle, telemetry))
        {
            work.uncertain = true;
            return work;
        }
        work.value.resolution_collapsed = true;
        retain_point(work, base, left, right, telemetry, limits);
    }
    else
    {
        retain_point(work, subtract(base, displacement), left, right, telemetry, limits);
        if (!work.uncertain)
            retain_point(work, add(base, displacement), left, right, telemetry, limits);
    }
    finish_relation(work);
    return work;
}

const AnalyticAtomicCurveNm* find_curve(const std::vector<AnalyticAtomicCurveNm>& curves,
                                        std::uint32_t index) noexcept
{
    const auto found = std::lower_bound(curves.begin(), curves.end(), index,
                                        [](const AnalyticAtomicCurveNm& curve, std::uint32_t value)
                                        { return curve.curve_index < value; });
    return found != curves.end() && found->curve_index == index ? &*found : nullptr;
}

bool checked_memory(std::size_t count, std::uint64_t& bytes) noexcept
{
    if (count > std::numeric_limits<std::uint64_t>::max() / kPairLogicalBytes)
        return false;
    bytes = static_cast<std::uint64_t>(count) * kPairLogicalBytes;
    return true;
}

} // namespace

AnalyticNarrowPhaseResult
intersect_analytic_curve_candidates(const std::vector<AnalyticAtomicCurveNm>& curves,
                                    const std::vector<AnalyticCurvePair>& candidate_pairs,
                                    const AnalyticSolverLimits& limits)
{
    AnalyticNarrowPhaseResult result;
    if (!analytic_solver_limits_within_hard_ceilings(limits))
    {
        result.error = AnalyticNarrowPhaseError::invalid_argument;
        return result;
    }
    if (candidate_pairs.size() > limits.examined_curve_pairs)
    {
        result.error = AnalyticNarrowPhaseError::resource_limit_exceeded;
        return result;
    }
    for (std::size_t index = 0; index < curves.size(); ++index)
    {
        if (!valid_curve(curves[index]) ||
            (index != 0 && curves[index - 1].curve_index >= curves[index].curve_index))
        {
            result.error = AnalyticNarrowPhaseError::invalid_argument;
            return result;
        }
    }
    std::uint64_t logical_bytes = 0;
    if (!checked_memory(candidate_pairs.size(), logical_bytes) ||
        logical_bytes > limits.working_memory_bytes)
    {
        result.error = AnalyticNarrowPhaseError::resource_limit_exceeded;
        return result;
    }
    result.telemetry.peak_working_memory_bytes = logical_bytes;
    static_assert(sizeof(AnalyticPairIntersection) <= kPairLogicalBytes,
                  "canonical narrow-phase pair charge must cover the native record");
    try
    {
        result.intersections.reserve(candidate_pairs.size());
    }
    catch (const std::bad_alloc&)
    {
        result.error = AnalyticNarrowPhaseError::resource_limit_exceeded;
        return result;
    }
    if (result.intersections.capacity() >
        logical_bytes / static_cast<std::uint64_t>(sizeof(AnalyticPairIntersection)))
    {
        result.error = AnalyticNarrowPhaseError::resource_limit_exceeded;
        result.intersections.clear();
        return result;
    }

    AnalyticCurvePair previous{};
    for (std::size_t pair_index = 0; pair_index < candidate_pairs.size(); ++pair_index)
    {
        const AnalyticCurvePair pair = candidate_pairs[pair_index];
        if (pair.first == 0 || pair.first >= pair.second ||
            (pair_index != 0 && (previous.first > pair.first ||
                                 (previous.first == pair.first && previous.second >= pair.second))))
        {
            result.error = AnalyticNarrowPhaseError::invalid_argument;
            result.intersections.clear();
            return result;
        }
        previous = pair;
        const AnalyticAtomicCurveNm* left = find_curve(curves, pair.first);
        const AnalyticAtomicCurveNm* right = find_curve(curves, pair.second);
        if (left == nullptr || right == nullptr)
        {
            result.error = AnalyticNarrowPhaseError::invalid_argument;
            result.intersections.clear();
            return result;
        }
        ++result.telemetry.candidate_pairs_consumed;
        PairWork work;
        work.value.pair = pair;
        if (left->kind == AnalyticAtomicCurveKind::line &&
            right->kind == AnalyticAtomicCurveKind::line)
        {
            ++result.telemetry.line_line_pairs;
            work = intersect_lines(*left, *right, result.telemetry, limits);
        }
        else if (left->kind == AnalyticAtomicCurveKind::circular_arc &&
                 right->kind == AnalyticAtomicCurveKind::circular_arc)
        {
            ++result.telemetry.circle_circle_pairs;
            work = intersect_circles(*left, *right, result.telemetry, limits);
        }
        else
        {
            ++result.telemetry.line_circle_pairs;
            const AnalyticAtomicCurveNm& line =
                left->kind == AnalyticAtomicCurveKind::line ? *left : *right;
            const AnalyticAtomicCurveNm& arc =
                left->kind == AnalyticAtomicCurveKind::circular_arc ? *left : *right;
            work = intersect_line_circle(line, arc, result.telemetry, limits);
        }
        work.value.pair = pair;
        if (work.uncertain)
        {
            ++result.telemetry.uncertain_predicates;
            result.error = AnalyticNarrowPhaseError::resource_limit_exceeded;
            result.intersections.clear();
            return result;
        }
        if (work.value.point_count > limits.intersections - result.telemetry.point_intersections)
        {
            result.error = AnalyticNarrowPhaseError::resource_limit_exceeded;
            result.intersections.clear();
            return result;
        }
        result.telemetry.point_intersections += work.value.point_count;
        if (work.value.relation == AnalyticPairRelation::coincident)
            ++result.telemetry.coincident_pairs;
        if (work.value.resolution_collapsed)
            ++result.telemetry.resolution_collapses;
        result.intersections.push_back(work.value);
    }
    return result;
}

} // namespace geometer

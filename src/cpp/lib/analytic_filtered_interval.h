#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

namespace geometer::analytic_detail
{

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

inline double downward(double value) noexcept
{
    return std::nextafter(value, -std::numeric_limits<double>::infinity());
}

inline double upward(double value) noexcept
{
    return std::nextafter(value, std::numeric_limits<double>::infinity());
}

inline Interval exact(double value) noexcept
{
    return {value, value};
}

inline bool singleton(Interval value) noexcept
{
    return value.lower == value.upper;
}

inline bool valid(Interval value) noexcept
{
    return std::isfinite(value.lower) && std::isfinite(value.upper) && value.lower <= value.upper;
}

inline Interval add(Interval left, Interval right) noexcept
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

inline Interval negate(Interval value) noexcept
{
    return {-value.upper, -value.lower};
}

inline Interval subtract(Interval left, Interval right) noexcept
{
    return add(left, negate(right));
}

inline Interval multiply_singletons(double left, double right) noexcept
{
    const double product = left * right;
    const double residual = std::fma(left, right, -product);
    if (residual == 0.0)
        return exact(product);
    return {downward(product + residual), upward(product + residual)};
}

inline Interval multiply(Interval left, Interval right) noexcept
{
    if (singleton(left) && singleton(right))
        return multiply_singletons(left.lower, right.lower);
    const double products[] = {left.lower * right.lower, left.lower * right.upper,
                               left.upper * right.lower, left.upper * right.upper};
    return {downward(*std::min_element(std::begin(products), std::end(products))),
            upward(*std::max_element(std::begin(products), std::end(products)))};
}

inline Interval divide(Interval numerator, Interval denominator) noexcept
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

inline Interval square(Interval value) noexcept
{
    if (value.lower >= 0.0)
        return multiply(value, value);
    if (value.upper <= 0.0)
        return multiply(negate(value), negate(value));
    const double maximum = std::max(value.lower * value.lower, value.upper * value.upper);
    return {0.0, upward(maximum)};
}

inline Interval square_root(Interval value) noexcept
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

inline Interval dot(Point left, Point right) noexcept
{
    return add(multiply(left.x, right.x), multiply(left.y, right.y));
}

inline Interval cross(Point left, Point right) noexcept
{
    return subtract(multiply(left.x, right.y), multiply(left.y, right.x));
}

inline Point add(Point left, Point right) noexcept
{
    return {add(left.x, right.x), add(left.y, right.y)};
}

inline Point subtract(Point left, Point right) noexcept
{
    return {subtract(left.x, right.x), subtract(left.y, right.y)};
}

inline Point scale(Point point, Interval scalar) noexcept
{
    return {multiply(point.x, scalar), multiply(point.y, scalar)};
}

inline Point perpendicular(Point point) noexcept
{
    return {negate(point.y), point.x};
}

} // namespace geometer::analytic_detail

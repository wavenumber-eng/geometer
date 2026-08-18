#pragma once

// Internal scalar, carrier, and coordinate helpers for filtered lowering.

std::uint64_t ordered_key(std::int64_t value) noexcept
{
    return static_cast<std::uint64_t>(value) ^ (std::uint64_t{1} << 63U);
}

std::uint64_t span(std::int64_t minimum, std::int64_t maximum) noexcept
{
    return ordered_key(maximum) - ordered_key(minimum);
}

bool global_expansion_fits(std::int64_t center, std::uint64_t extent) noexcept
{
    const std::uint64_t key = ordered_key(center);
    return key >= extent && key <= std::numeric_limits<std::uint64_t>::max() - extent;
}

std::uint64_t magnitude(std::int64_t value) noexcept
{
    return value < 0 ? std::uint64_t{0} - static_cast<std::uint64_t>(value)
                     : static_cast<std::uint64_t>(value);
}

LineFamilyKey canonical_direction(std::int64_t dx, std::int64_t dy) noexcept
{
    const std::uint64_t divisor = std::gcd(magnitude(dx), magnitude(dy));
    dx /= static_cast<std::int64_t>(divisor);
    dy /= static_cast<std::int64_t>(divisor);
    if (dx < 0 || (dx == 0 && dy < 0))
    {
        dx = -dx;
        dy = -dy;
    }
    return {dx, dy};
}

std::uint64_t hash_word(std::uint64_t state, std::uint64_t value) noexcept
{
    state ^= value + 0x9e3779b97f4a7c15ULL + (state << 6U) + (state >> 2U);
    state ^= state >> 30U;
    state *= 0xbf58476d1ce4e5b9ULL;
    state ^= state >> 27U;
    state *= 0x94d049bb133111ebULL;
    return state ^ (state >> 31U);
}

std::uint64_t hash_wide(std::uint64_t state, WideInteger value) noexcept
{
    return hash_word(hash_word(state, wide_low_bits(value)), wide_high_bits(value));
}

bool same_line_family(const LineFamilyKey& left, const LineFamilyKey& right) noexcept
{
    return left.dx == right.dx && left.dy == right.dy;
}

bool same_circle_family(const CircleFamilyKey& left, const CircleFamilyKey& right) noexcept
{
    return left.x == right.x && left.y == right.y;
}

bool same_family(const TokenDescriptor& left, const TokenDescriptor& right) noexcept
{
    if (left.kind != right.kind)
        return false;
    return left.kind == TokenKeyKind::line
               ? same_line_family(left.line.family, right.line.family)
               : same_circle_family(left.circle.family, right.circle.family);
}

bool same_carrier(const TokenDescriptor& left, const TokenDescriptor& right) noexcept
{
    if (!same_family(left, right))
        return false;
    if (left.kind == TokenKeyKind::line)
        return wide_compare(left.line.rational_part_times_two,
                            right.line.rational_part_times_two) == 0 &&
               left.line.radical_coefficient == right.line.radical_coefficient;
    return wide_compare(left.circle.rational_part, right.circle.rational_part) == 0 &&
           left.circle.radical_coefficient == right.circle.radical_coefficient &&
           wide_compare(left.circle.radicand, right.circle.radicand) == 0;
}

std::uint64_t token_hash(const TokenDescriptor& value, bool carrier) noexcept
{
    std::uint64_t state = hash_word(0x243f6a8885a308d3ULL, static_cast<std::uint8_t>(value.kind));
    if (value.kind == TokenKeyKind::line)
    {
        state = hash_word(state, static_cast<std::uint64_t>(value.line.family.dx));
        state = hash_word(state, static_cast<std::uint64_t>(value.line.family.dy));
        if (carrier)
        {
            state = hash_wide(state, value.line.rational_part_times_two);
            state = hash_word(state, static_cast<std::uint64_t>(value.line.radical_coefficient));
        }
        return state;
    }
    state = hash_word(state, static_cast<std::uint64_t>(value.circle.family.x));
    state = hash_word(state, static_cast<std::uint64_t>(value.circle.family.y));
    if (!carrier)
        return state;
    state = hash_wide(state, value.circle.rational_part);
    state = hash_word(state, static_cast<std::uint64_t>(value.circle.radical_coefficient));
    return hash_wide(state, value.circle.radicand);
}

std::size_t token_table_capacity(std::size_t count) noexcept
{
    std::size_t capacity = 1;
    while (capacity < count * 2)
        capacity *= 2;
    return capacity;
}

WideInteger squared_distance(AnalyticIntegerPointNm left, AnalyticIntegerPointNm right) noexcept
{
    const std::int64_t dx = left.x - right.x;
    const std::int64_t dy = left.y - right.y;
    return wide_add(wide_multiply(dx, dx), wide_multiply(dy, dy));
}

WideInteger cross_from(AnalyticIntegerPointNm origin, AnalyticIntegerPointNm left,
                       AnalyticIntegerPointNm right) noexcept
{
    const std::int64_t ax = left.x - origin.x;
    const std::int64_t ay = left.y - origin.y;
    const std::int64_t bx = right.x - origin.x;
    const std::int64_t by = right.y - origin.y;
    return wide_subtract(wide_multiply(ax, by), wide_multiply(ay, bx));
}

WideInteger dot_vectors(std::int64_t ax, std::int64_t ay, std::int64_t bx, std::int64_t by) noexcept
{
    return wide_add(wide_multiply(ax, bx), wide_multiply(ay, by));
}

AnalyticCoordinateIntervalNm public_interval(Interval value) noexcept
{
    return {value.lower, value.upper};
}

AnalyticFilteredPointNm public_point(Point value) noexcept
{
    return {public_interval(value.x), public_interval(value.y)};
}

Point point(AnalyticIntegerPointNm value) noexcept
{
    return {exact(static_cast<double>(value.x)), exact(static_cast<double>(value.y))};
}

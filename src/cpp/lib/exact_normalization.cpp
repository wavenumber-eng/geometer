#include "geometer/exact_normalization.h"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>

namespace geometer::exact
{
namespace
{

class StorageReservation
{
  public:
    StorageReservation(Budget& budget, std::uint64_t bytes)
        : budget_(budget), bytes_(bytes), acquired_(budget.acquire_storage(bytes))
    {
    }

    ~StorageReservation()
    {
        if (acquired_)
            budget_.release_storage(bytes_);
    }

    [[nodiscard]] bool acquired() const
    {
        return acquired_;
    }

  private:
    Budget& budget_;
    std::uint64_t bytes_;
    bool acquired_;
};

std::uint64_t checked_add(std::uint64_t left, std::uint64_t right)
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
        throw std::overflow_error("normalization budget estimate overflow");
    return left + right;
}

std::uint64_t checked_multiply(std::uint64_t left, std::uint64_t right)
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left)
        throw std::overflow_error("normalization budget estimate overflow");
    return left * right;
}

std::uint64_t limb_count(const BigInt& value)
{
    if (value == 0)
        return 1;
    const auto& backend = value.backend();
    auto high_limb = backend.limbs()[backend.size() - 1];
    std::uint64_t high_bits = 0;
    while (high_limb != 0)
    {
        ++high_bits;
        high_limb >>= 1;
    }
    const std::uint64_t preceding_bytes = checked_multiply(
        static_cast<std::uint64_t>(backend.size() - 1), sizeof(boost::multiprecision::limb_type));
    const std::uint64_t magnitude_bytes =
        checked_add(preceding_bytes, checked_add(high_bits, 7) / 8);
    return std::max<std::uint64_t>(1, checked_add(magnitude_bytes, 3) / 4);
}

std::uint64_t normalization_width(const CanonicalReal& value)
{
    if (value.kind() == CanonicalRealKind::rational)
        return checked_add(
            checked_add(limb_count(value.numerator()), limb_count(value.denominator())), 4);
    std::uint64_t width = checked_add(limb_count(value.root()->interval_k), 132);
    for (const BigInt& coefficient : value.polynomial()->coefficients())
        width = checked_add(width, limb_count(coefficient));
    return width;
}

BigInt floor_divide(const BigInt& numerator, const BigInt& positive_denominator)
{
    BigInt quotient = numerator / positive_denominator;
    if (numerator < 0 && numerator % positive_denominator != 0)
        --quotient;
    return quotient;
}

BigInt ceil_divide(const BigInt& numerator, const BigInt& positive_denominator)
{
    BigInt quotient = numerator / positive_denominator;
    if (numerator > 0 && numerator % positive_denominator != 0)
        ++quotient;
    return quotient;
}

std::optional<std::int64_t> checked_int64(const BigInt& value)
{
    if (value < std::numeric_limits<std::int64_t>::min() ||
        value > std::numeric_limits<std::int64_t>::max())
        return std::nullopt;
    return value.convert_to<std::int64_t>();
}

IntegerNormalizationResult normalize_rational(const CanonicalReal& value)
{
    const BigInt magnitude = value.numerator() < 0 ? -value.numerator() : value.numerator();
    BigInt rounded = magnitude / value.denominator();
    const BigInt remainder = magnitude % value.denominator();
    if (remainder * 2 >= value.denominator())
        ++rounded;
    if (value.numerator() < 0)
        rounded = -rounded;
    auto result = checked_int64(rounded);
    return result ? IntegerNormalizationResult{Error::none, *result}
                  : IntegerNormalizationResult{Error::invalid_argument, std::nullopt};
}

struct FloorResult
{
    Error error = Error::none;
    std::optional<BigInt> value;
};

FloorResult certified_floor(Budget& budget, const CanonicalReal& value)
{
    const IsolatedRoot& root = *value.root();
    constexpr std::array<std::uint32_t, 5> precisions = {256, 512, 1024, 2048, 4096};
    for (const std::uint32_t precision : precisions)
    {
        if (precision < root.precision)
            continue;
        RootRefinementResult refined =
            refine_real_root(budget, *value.polynomial(), root.ordinal, precision);
        if (refined.error != Error::none || !refined.value)
            return {refined.error == Error::none ? Error::invalid_argument : refined.error,
                    std::nullopt};
        const BigInt scale = BigInt(1) << precision;
        const BigInt lower_floor = floor_divide(refined.value->interval_k, scale);
        const BigInt upper_floor = ceil_divide(refined.value->interval_k + 1, scale) - 1;
        if (lower_floor == upper_floor)
            return {Error::none, lower_floor};
        if (upper_floor != lower_floor + 1)
            continue;
        auto boundary = make_canonical_rational(budget, upper_floor, 1);
        if (boundary.error != Error::none || !boundary.value)
            return {boundary.error, std::nullopt};
        ComparisonResult side = compare_canonical_reals(budget, value, *boundary.value);
        if (side.error != Error::none || !side.ordering || *side.ordering == 0)
            return {side.error == Error::none ? Error::invalid_argument : side.error, std::nullopt};
        return {Error::none, *side.ordering < 0 ? lower_floor : upper_floor};
    }
    return {Error::resource_limit_exceeded, std::nullopt};
}

} // namespace

IntegerNormalizationResult normalize_exact_to_integer_nm(Budget& budget, const CanonicalReal& value)
{
    try
    {
        const std::uint64_t width = normalization_width(value);
        const std::uint64_t storage = checked_add(8192, checked_multiply(width, 512));
        const std::uint64_t work =
            checked_add(1'000'000, checked_multiply(512, checked_multiply(width, width)));
        StorageReservation reservation(budget, storage);
        if (!reservation.acquired() || !budget.consume_work(work))
            return {Error::resource_limit_exceeded, std::nullopt};
        if (value.kind() == CanonicalRealKind::rational)
            return normalize_rational(value);
        FloorResult floor = certified_floor(budget, value);
        if (floor.error != Error::none || !floor.value)
            return {floor.error, std::nullopt};
        auto half_boundary = make_canonical_rational(budget, *floor.value * 2 + 1, 2);
        if (half_boundary.error != Error::none || !half_boundary.value)
            return {half_boundary.error, std::nullopt};
        ComparisonResult half_order = compare_canonical_reals(budget, value, *half_boundary.value);
        if (half_order.error != Error::none || !half_order.ordering || *half_order.ordering == 0)
            return {half_order.error == Error::none ? Error::invalid_argument : half_order.error,
                    std::nullopt};
        const BigInt rounded = *half_order.ordering < 0 ? *floor.value : *floor.value + 1;
        auto result = checked_int64(rounded);
        return result ? IntegerNormalizationResult{Error::none, *result}
                      : IntegerNormalizationResult{Error::invalid_argument, std::nullopt};
    }
    catch (const std::exception&)
    {
        return {Error::resource_limit_exceeded, std::nullopt};
    }
}

} // namespace geometer::exact

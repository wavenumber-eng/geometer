#pragma once

#include <cstdint>

#if defined(_MSC_VER) && defined(_M_X64)
#include <intrin.h>
#endif

namespace geometer::analytic_detail
{

#if defined(_MSC_VER) && defined(_M_X64)
struct WideInteger
{
    std::uint64_t low = 0;
    std::int64_t high = 0;
};

inline WideInteger wide_multiply(std::int64_t left, std::int64_t right) noexcept
{
    std::int64_t high = 0;
    const std::int64_t low = _mul128(left, right, &high);
    return {static_cast<std::uint64_t>(low), high};
}

inline WideInteger wide_add(WideInteger left, WideInteger right) noexcept
{
    const std::uint64_t low = left.low + right.low;
    const std::uint64_t carry = low < left.low ? 1U : 0U;
    return {low, static_cast<std::int64_t>(static_cast<std::uint64_t>(left.high) +
                                           static_cast<std::uint64_t>(right.high) + carry)};
}

inline WideInteger wide_subtract(WideInteger left, WideInteger right) noexcept
{
    const std::uint64_t low = left.low - right.low;
    const std::uint64_t borrow = left.low < right.low ? 1U : 0U;
    return {low, static_cast<std::int64_t>(static_cast<std::uint64_t>(left.high) -
                                           static_cast<std::uint64_t>(right.high) - borrow)};
}

inline int wide_sign(WideInteger value) noexcept
{
    if (value.high < 0)
        return -1;
    if (value.high > 0 || value.low != 0)
        return 1;
    return 0;
}
#else
using WideInteger = __int128;

inline WideInteger wide_multiply(std::int64_t left, std::int64_t right) noexcept
{
    return static_cast<WideInteger>(left) * static_cast<WideInteger>(right);
}

inline WideInteger wide_add(WideInteger left, WideInteger right) noexcept
{
    return left + right;
}

inline WideInteger wide_subtract(WideInteger left, WideInteger right) noexcept
{
    return left - right;
}

inline int wide_sign(WideInteger value) noexcept
{
    return value < 0 ? -1 : value > 0 ? 1 : 0;
}
#endif

inline int wide_compare(WideInteger left, WideInteger right) noexcept
{
    return wide_sign(wide_subtract(left, right));
}

inline WideInteger wide_absolute(WideInteger value) noexcept
{
    return wide_sign(value) < 0 ? wide_subtract(wide_multiply(0, 0), value) : value;
}

} // namespace geometer::analytic_detail

#pragma once

#include <cstdint>
#include <optional>

namespace geometer::analytic_arrangement_detail
{

inline bool merge_certified_cycle_orientation(std::optional<std::int8_t> candidate,
                                              std::optional<std::int8_t>& orientation) noexcept
{
    if (!candidate)
        return true;
    if (*candidate == 0 || (orientation && *orientation != *candidate))
        return false;
    orientation = candidate;
    return true;
}

} // namespace geometer::analytic_arrangement_detail

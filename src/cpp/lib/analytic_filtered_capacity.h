#pragma once

#include <cstdint>

namespace geometer
{
namespace analytic_detail
{

inline constexpr std::uint64_t kOverlayRawEventLogicalBytes = 96;
inline constexpr std::uint64_t kOverlayUniqueEventLogicalBytes = 144;
inline constexpr std::uint64_t kOverlayActionLogicalBytes = 24;

struct AnalyticFilteredArrangementCapacityEnvelope
{
    std::uint64_t curve_count = 0;
    std::uint64_t pair_count = 0;
    std::uint64_t point_intersections = 0;
    std::uint64_t circular_carrier_groups = 0;
    std::uint64_t spans = 0;
    std::uint64_t collapsed_domains = 0;
    std::uint64_t memberships = 0;
};

[[nodiscard]] bool estimate_analytic_filtered_arrangement_possible_memory(
    const AnalyticFilteredArrangementCapacityEnvelope& envelope,
    std::uint64_t& working_memory_bytes) noexcept;

} // namespace analytic_detail
} // namespace geometer

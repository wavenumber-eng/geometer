#pragma once

#include "geometer/analytic_curve_narrow_phase.h"

#include <cstdint>
#include <optional>
#include <tuple>

namespace geometer::analytic_arrangement_detail
{

struct TangentEndpointIdentity
{
    AnalyticAtomicCurveKind kind = AnalyticAtomicCurveKind::line;
    std::uint64_t carrier_id = 0;
    std::uint64_t tangent_id = 0;
    AnalyticFilteredPointNm point;
};

inline bool same_endpoint_enclosure(const AnalyticFilteredPointNm& left,
                                    const AnalyticFilteredPointNm& right) noexcept
{
    return left.x.lower == right.x.lower && left.x.upper == right.x.upper &&
           left.y.lower == right.y.lower && left.y.upper == right.y.upper &&
           left.construction_x_column_id == right.construction_x_column_id;
}

inline bool tangent_token_names_endpoint(const TangentEndpointIdentity& value) noexcept
{
    return value.tangent_id != 0 && (analytic_endpoint_tangent_matches(value.tangent_id, value.kind,
                                                                       value.carrier_id, false) ||
                                     analytic_endpoint_tangent_matches(value.tangent_id, value.kind,
                                                                       value.carrier_id, true));
}

inline bool shares_exact_tangent_contact(const TangentEndpointIdentity& first,
                                         const TangentEndpointIdentity& second,
                                         double tangent_dot_lower) noexcept
{
    constexpr std::uint64_t kEndpointRoleBits =
        (std::uint64_t{1} << 34U) | (std::uint64_t{1} << 35U);
    const bool same_line_circle_carriers =
        analytic_is_endpoint_tangent_token(first.tangent_id) &&
        analytic_is_endpoint_tangent_token(second.tangent_id) &&
        (first.tangent_id & ~kEndpointRoleBits) == (second.tangent_id & ~kEndpointRoleBits);
    return tangent_dot_lower > 0.0 && first.tangent_id != 0 &&
           (first.tangent_id == second.tangent_id || same_line_circle_carriers) &&
           tangent_token_names_endpoint(first) && tangent_token_names_endpoint(second) &&
           same_endpoint_enclosure(first.point, second.point);
}

inline bool shares_canonical_tangent_class(const TangentEndpointIdentity& first,
                                           const TangentEndpointIdentity& second,
                                           double tangent_dot_lower, double first_angle,
                                           double second_angle) noexcept
{
    return first_angle == second_angle &&
           shares_exact_tangent_contact(first, second, tangent_dot_lower);
}

inline std::optional<std::int8_t> compare_canonical_tangent_class(
    const TangentEndpointIdentity& first, const TangentEndpointIdentity& second,
    double tangent_dot_lower, double first_angle, double second_angle, std::int8_t first_curvature,
    std::int8_t second_curvature, double first_radius_key, double second_radius_key,
    std::uint32_t first_source, std::uint32_t second_source, std::uint32_t first_half_edge,
    std::uint32_t second_half_edge) noexcept
{
    if (!shares_canonical_tangent_class(first, second, tangent_dot_lower, first_angle,
                                        second_angle))
        return std::nullopt;
    const auto first_key =
        std::tie(first_angle, first_curvature, first_radius_key, first_source, first_half_edge);
    const auto second_key = std::tie(second_angle, second_curvature, second_radius_key,
                                     second_source, second_half_edge);
    return first_key < second_key ? -1 : (second_key < first_key ? 1 : 0);
}

} // namespace geometer::analytic_arrangement_detail

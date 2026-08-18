#pragma once

#include "analytic_wide_integer.h"
#include "geometer/analytic_curve_narrow_phase.h"
#include "geometer/analytic_filtered_lowering.h"

#include <cstdint>
#include <tuple>
#include <vector>

namespace geometer::analytic_lowering_detail
{

using analytic_detail::WideInteger;

struct LineFamilyKey
{
    std::int64_t dx = 0;
    std::int64_t dy = 0;

    bool operator<(const LineFamilyKey& other) const noexcept
    {
        return std::tie(dx, dy) < std::tie(other.dx, other.dy);
    }
};

struct LineCarrierKey
{
    LineFamilyKey family;
    // Exact carrier offset is rational_part_times_two / 2 plus
    // radical_coefficient / 2 * sqrt(dx^2 + dy^2).
    WideInteger rational_part_times_two{};
    std::int64_t radical_coefficient = 0;
};

struct CircleFamilyKey
{
    std::int64_t x = 0;
    std::int64_t y = 0;
};

struct CircleCarrierKey
{
    CircleFamilyKey family;
    // Exact (2 * radius)^2. For ordinary rational radii only rational_part is
    // populated. Offset circles may additionally carry
    // radical_coefficient * sqrt(radicand).
    WideInteger rational_part{};
    std::int64_t radical_coefficient = 0;
    WideInteger radicand{};
};

enum class TokenKeyKind : std::uint8_t
{
    line,
    circle,
};

struct TokenDescriptor
{
    TokenKeyKind kind = TokenKeyKind::line;
    LineCarrierKey line;
    CircleCarrierKey circle;
};

struct EmittedCurve
{
    AnalyticAtomicCurveNm curve;
    bool agrees_with_carrier = false;
    bool material_on_left = false;
    AnalyticFilteredSourceReference source;
    TokenDescriptor descriptor;
};

struct EmittedEndpointTangency
{
    std::uint32_t first_curve = 0;
    std::uint32_t second_curve = 0;
    bool first_start = false;
    bool second_start = false;
    std::uint32_t construction_identity = 0;
};

struct SweptPathLoweringResult
{
    AnalyticFilteredLoweringError error = AnalyticFilteredLoweringError::none;
    std::vector<EmittedCurve> curves;
    std::vector<EmittedEndpointTangency> endpoint_tangencies;
    AnalyticFilteredLoweringTelemetry telemetry;
};

[[nodiscard]] SweptPathLoweringResult
lower_filtered_swept_path(const AnalyticRequestPacketRecords& records,
                          const AnalyticRequestOperandRecord& operand, std::int64_t origin_x_nm,
                          std::int64_t origin_y_nm, const AnalyticSolverLimits& limits);

} // namespace geometer::analytic_lowering_detail

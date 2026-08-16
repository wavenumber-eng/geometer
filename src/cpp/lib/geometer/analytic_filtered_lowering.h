#pragma once

#include "geometer/analytic_curve_narrow_phase.h"
#include "geometer/analytic_request_packet.h"
#include "geometer/analytic_solver_limits.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace geometer
{

enum class AnalyticFilteredLoweringError : std::uint8_t
{
    none = 0,
    invalid_topology = 1,
    invalid_arc = 2,
    unsupported_geometry = 3,
    resource_limit_exceeded = 4,
};

enum class AnalyticFilteredSourceKind : std::uint16_t
{
    authored_segment_curve = 1,
    compact_feature_role = 2,
    subtractive_operand_effect = 3,
};

enum class AnalyticFilteredSourceRole : std::uint16_t
{
    none = 0,
    authored_line = 1,
    authored_circular_arc = 2,
    primitive_outer_circle = 16,
    primitive_inner_circle = 17,
    capsule_left_line = 32,
    capsule_end_cap = 33,
    capsule_right_line = 34,
    capsule_start_cap = 35,
    swept_left_offset_line = 48,
    swept_left_offset_arc = 49,
    swept_right_offset_line = 50,
    swept_right_offset_arc = 51,
    swept_round_join = 52,
    swept_start_cap = 53,
    swept_end_cap = 54,
};

struct AnalyticFilteredSourceReference
{
    AnalyticFilteredSourceKind kind = AnalyticFilteredSourceKind::authored_segment_curve;
    AnalyticFilteredSourceRole role = AnalyticFilteredSourceRole::none;
    std::uint64_t operand_id = 0;
    std::uint64_t primary_id = 0;
    std::uint64_t secondary_id = 0;
};

struct AnalyticFilteredOccurrence
{
    std::uint64_t occurrence_id = 0;
    std::uint64_t coverage_id = 0;
    bool agrees_with_carrier = false;
    bool material_on_left = false;
    AnalyticFilteredSourceReference source;
};

struct AnalyticFilteredLoweringTelemetry
{
    std::uint64_t input_operands = 0;
    std::uint64_t input_segments = 0;
    std::uint64_t emitted_curves = 0;
    std::uint64_t fixed_width_predicates = 0;
    std::uint64_t square_root_calls = 0;
    std::uint64_t peak_working_memory_bytes = 0;
    std::uint64_t algebraic_fallback_calls = 0;
};

struct AnalyticFilteredGeometry
{
    std::int64_t origin_x_nm = 0;
    std::int64_t origin_y_nm = 0;
    std::vector<AnalyticAtomicCurveNm> curves;
    std::vector<AnalyticCurveBoundsNm> bounds;
    std::vector<AnalyticFilteredOccurrence> occurrences;
};

struct AnalyticFilteredLoweringResult
{
    AnalyticFilteredLoweringError error = AnalyticFilteredLoweringError::none;
    std::optional<AnalyticFilteredGeometry> value;
    AnalyticFilteredLoweringTelemetry telemetry;
};

// Lowers one job from an already packet-validated request directly into the
// bounded binary64 curve domain used by the production broad and narrow
// phases. Coordinates are translated to a deterministic job-local integer-nm
// origin before conversion. Proof tokens are minted only here from exact
// integer construction facts; they are not request fields.
//
// A0 authored regions, disks, annuli, and capsules are supported. Swept paths
// remain fail-closed until their piece union is implemented by the filtered
// indexed arrangement; this path never invokes the algebraic construction
// arena or performs an implicit curve-pair scan.
[[nodiscard]] AnalyticFilteredLoweringResult
lower_analytic_job_to_filtered_curves(const AnalyticRequestPacketRecords& records,
                                      std::uint32_t job_index,
                                      const AnalyticSolverLimits& limits = {});

} // namespace geometer

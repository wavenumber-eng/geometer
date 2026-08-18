#pragma once

#include "geometer/analytic_curve_narrow_phase.h"
#include "geometer/analytic_filtered_lowering.h"
#include "geometer/analytic_solver_limits.h"

#include <cstdint>
#include <vector>

namespace geometer
{

inline constexpr std::uint64_t kAnalyticOverlayCurveGroupLogicalBytes = 96;
inline constexpr std::uint64_t kAnalyticOverlaySpanLogicalBytes = 112;

enum class AnalyticXMonotoneBranch : std::uint8_t
{
    none = 0,
    lower = 1,
    upper = 2,
};

struct AnalyticAtomicSpanNm
{
    std::uint32_t span_index = 0;
    std::uint32_t carrier_curve_index = 0;
    AnalyticAtomicCurveKind kind = AnalyticAtomicCurveKind::line;
    AnalyticFilteredPointNm start;
    AnalyticFilteredPointNm end;
    bool major_arc = false;
    std::uint32_t membership_begin = 0;
    std::uint32_t membership_count = 0;
    AnalyticXMonotoneBranch x_monotone_branch = AnalyticXMonotoneBranch::none;
};

struct AnalyticSpanMembership
{
    std::uint32_t curve_index = 0;
    bool agrees_with_span = false;
    bool material_on_span_left = false;
};

enum class AnalyticFilteredOverlayError : std::uint8_t
{
    none = 0,
    invalid_argument = 1,
    resource_limit_exceeded = 2,
};

struct AnalyticFilteredOverlayTelemetry
{
    std::uint64_t input_curves = 0;
    std::uint64_t input_pair_results = 0;
    std::uint64_t input_point_intersections = 0;
    std::uint64_t carrier_groups = 0;
    std::uint64_t raw_events = 0;
    std::uint64_t unique_events = 0;
    std::uint64_t resolution_merges = 0;
    std::uint64_t collapsed_domains = 0;
    std::uint64_t sort_work_units = 0;
    std::uint64_t active_set_updates = 0;
    std::uint64_t membership_visits = 0;
    std::uint64_t narrow_phase_predicate_calls = 0;
    std::uint64_t narrow_phase_peak_working_memory_bytes = 0;
    std::uint64_t predicate_calls = 0;
    std::uint64_t emitted_spans = 0;
    std::uint64_t emitted_memberships = 0;
    std::uint64_t peak_working_memory_bytes = 0;
    std::uint64_t required_working_memory_bytes = 0;
    std::uint64_t algebraic_fallback_calls = 0;
    bool unresolved_predicate_failure = false;
};

struct AnalyticFilteredOverlayResult
{
    AnalyticFilteredOverlayError error = AnalyticFilteredOverlayError::none;
    std::vector<AnalyticAtomicSpanNm> spans;
    std::vector<AnalyticSpanMembership> memberships;
    AnalyticFilteredOverlayTelemetry telemetry;
};

// Runs the narrow phase for only the supplied broad-phase candidates and
// converts its trusted results directly into canonical atomic carrier
// intervals. Keeping the result boundary internal prevents a caller from
// injecting unverified split points. Equal lowering-issued carrier ids are
// grouped directly; no curve-pair search is performed. Endpoint/intersection
// events are sorted once per job, resolution-equivalent events merge only when
// a 50 nm enclosure is certified, and an indexed active set makes same-domain
// membership output proportional to the memberships actually emitted. The
// combined minimum narrow-result and overlay-table memory/work is preflighted
// before the narrow phase allocates or evaluates a candidate. Every published
// span names an active member whose finite domain covers that span, rather than
// merely naming an arbitrary representative of the shared carrier.
[[nodiscard]] AnalyticFilteredOverlayResult
build_analytic_filtered_overlay(const AnalyticFilteredGeometry& geometry,
                                const std::vector<AnalyticCurvePair>& candidate_pairs,
                                const AnalyticSolverLimits& limits = {});

} // namespace geometer

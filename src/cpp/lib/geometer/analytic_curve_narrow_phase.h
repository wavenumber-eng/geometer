#pragma once

#include "geometer/analytic_curve_broad_phase.h"
#include "geometer/analytic_solver_limits.h"

#include <array>
#include <cstdint>
#include <vector>

namespace geometer
{

// Canonical target-independent storage charged for each retained candidate
// result. Integrated stages use the same charge to reject work that cannot fit
// before the narrow phase allocates its result table.
inline constexpr std::uint64_t kAnalyticNarrowPhasePairLogicalBytes = 256;

struct AnalyticCoordinateIntervalNm
{
    double lower = 0.0;
    double upper = 0.0;
};

struct AnalyticFilteredPointNm
{
    AnalyticCoordinateIntervalNm x;
    AnalyticCoordinateIntervalNm y;
    // Nonzero only when trusted lowering/narrow construction proves one x
    // column. Upstream 50 nm event reconciliation may carry that identity onto
    // the merged vertex hull; the token never spends another repair or stores
    // an algebraic expression/coordinate.
    std::uint64_t construction_x_column_id = 0;
};

// Fixed-width proof-token namespaces used only inside the trusted filtered
// pipeline. A vertical carrier token proves every point on that line has one
// exact x coordinate; a pair token proves the two roots produced by one
// carrier pair share an exact x coordinate. An endpoint-arc partition token
// correlates overlapping event enclosures around one normalized endpoint and
// one cardinal side so the face sweep handles them in one atomic column; it
// does not prove coordinate equality and may never merge vertices. No token
// stores an algebraic expression or coordinate.
inline constexpr std::uint64_t kAnalyticVerticalXColumnTag = std::uint64_t{1} << 63U;
inline constexpr std::uint64_t kAnalyticEndpointArcLeftColumnTag = std::uint64_t{3} << 62U;
inline constexpr std::uint64_t kAnalyticEndpointArcRightColumnTag = std::uint64_t{7} << 61U;

[[nodiscard]] inline constexpr std::uint64_t
analytic_vertical_x_column_token(std::uint64_t carrier_id) noexcept
{
    return carrier_id != 0 && carrier_id < (std::uint64_t{1} << 31U)
               ? kAnalyticVerticalXColumnTag | carrier_id
               : 0;
}

[[nodiscard]] inline constexpr std::uint64_t
analytic_pair_x_column_token(std::uint64_t first_carrier_id,
                             std::uint64_t second_carrier_id) noexcept
{
    const std::uint64_t first =
        first_carrier_id < second_carrier_id ? first_carrier_id : second_carrier_id;
    const std::uint64_t second =
        first_carrier_id < second_carrier_id ? second_carrier_id : first_carrier_id;
    return first != 0 && first < (std::uint64_t{1} << 31U) && second < (std::uint64_t{1} << 32U)
               ? (first << 32U) | second
               : 0;
}

[[nodiscard]] inline constexpr std::uint64_t
analytic_endpoint_arc_partition_column_token(std::uint64_t carrier_id, bool right) noexcept
{
    if (carrier_id == 0 || carrier_id >= (std::uint64_t{1} << 61U))
        return 0;
    return (right ? kAnalyticEndpointArcRightColumnTag : kAnalyticEndpointArcLeftColumnTag) |
           carrier_id;
}

[[nodiscard]] inline constexpr bool
analytic_is_endpoint_arc_partition_column_token(std::uint64_t token) noexcept
{
    const std::uint64_t tag = token >> 61U;
    return tag == 6 || tag == 7;
}

[[nodiscard]] inline constexpr bool
analytic_endpoint_arc_partition_column_is_right(std::uint64_t token) noexcept
{
    return (token >> 61U) == 7;
}

struct AnalyticFilteredCircleNm
{
    AnalyticFilteredPointNm center;
    AnalyticCoordinateIntervalNm radius;
};

struct AnalyticIntegerPointNm
{
    std::int64_t x = 0;
    std::int64_t y = 0;
};

enum class AnalyticAtomicCurveKind : std::uint8_t
{
    line = 0,
    circular_arc = 1,
};

struct AnalyticAtomicCurveNm
{
    std::uint32_t curve_index = 0;
    AnalyticAtomicCurveKind kind = AnalyticAtomicCurveKind::line;
    AnalyticFilteredPointNm start;
    AnalyticFilteredPointNm end;
    AnalyticFilteredCircleNm circle;
    bool counterclockwise = true;
    bool major_arc = false;
    bool has_integer_certificate = false;
    AnalyticIntegerPointNm integer_start;
    AnalyticIntegerPointNm integer_end;
    AnalyticIntegerPointNm integer_center;
    bool has_integer_radius_certificate = false;
    std::uint64_t integer_radius = 0;
    // Normalization-replay-only construction fact. Integer endpoints/radius,
    // direction, major flag, and the filtered center are bound to the exact
    // endpoint-authoritative center branch reconstructed by the trusted
    // normalizer. This allows a known endpoint root to be factored without
    // storing its generally irrational center algebraically. Request inputs
    // cannot mint this certificate.
    bool has_endpoint_authoritative_arc_certificate = false;
    // A separately verified refinement proves that the reconstructed finite
    // arc remains on one named x-monotone half. Without it, overlay performs
    // its ordinary internal cardinal partition.
    bool has_endpoint_authoritative_x_monotone_certificate = false;
    bool endpoint_authoritative_upper_branch = false;
    // Nonzero construction ids are job-local proof tokens emitted by the
    // trusted lowering stage. Equal carrier ids mean the same infinite line
    // or circle; equal family ids mean parallel lines or concentric circles.
    std::uint64_t construction_carrier_id = 0;
    std::uint64_t construction_family_id = 0;
    // Certifies the authored minor/major and direction flags when correlated
    // filtered endpoint expressions make an exact zero cross product appear
    // as a non-singleton interval (notably arbitrary-angle offset caps).
    bool has_arc_sweep_certificate = false;
    // Lowering-issued primitive direction for line carriers. The pair is
    // canonical (first nonzero component positive) and proves vertical/east
    // sweep behavior even when correlated offset endpoints are non-singleton
    // filtered values. It is not accepted from request packets.
    bool has_construction_line_direction = false;
    std::int64_t construction_line_dx = 0;
    std::int64_t construction_line_dy = 0;
};

// Target-independent logical charge for one retained atomic curve record.
// The value deliberately covers native ABI padding as well as wasm32 layout.
inline constexpr std::uint64_t kAnalyticAtomicCurveLogicalBytes = 272;

enum class AnalyticPairRelation : std::uint8_t
{
    disjoint = 0,
    point = 1,
    two_points = 2,
    coincident = 3,
};

struct AnalyticPairIntersection
{
    AnalyticCurvePair pair;
    AnalyticPairRelation relation = AnalyticPairRelation::disjoint;
    std::uint8_t point_count = 0;
    bool resolution_collapsed = false;
    std::array<AnalyticFilteredPointNm, 2> points{};
};

enum class AnalyticNarrowPhaseError : std::uint8_t
{
    none = 0,
    invalid_argument = 1,
    resource_limit_exceeded = 2,
};

struct AnalyticNarrowPhaseTelemetry
{
    std::uint64_t curve_table_entries = 0;
    std::uint64_t curve_references_resolved = 0;
    std::uint64_t candidate_pairs_consumed = 0;
    std::uint64_t line_line_pairs = 0;
    std::uint64_t line_circle_pairs = 0;
    std::uint64_t circle_circle_pairs = 0;
    std::uint64_t carrier_predicates = 0;
    std::uint64_t domain_predicates = 0;
    std::uint64_t square_root_calls = 0;
    std::uint64_t uncertain_predicates = 0;
    std::uint64_t tangent_contacts = 0;
    std::uint64_t resolution_collapses = 0;
    std::uint64_t point_intersections = 0;
    std::uint64_t coincident_pairs = 0;
    std::uint64_t predicate_calls = 0;
    std::uint64_t peak_working_memory_bytes = 0;
    std::uint64_t algebraic_fallback_calls = 0;
};

struct AnalyticNarrowPhaseResult
{
    AnalyticNarrowPhaseError error = AnalyticNarrowPhaseError::none;
    std::vector<AnalyticPairIntersection> intersections;
    AnalyticNarrowPhaseTelemetry telemetry;
};

enum class AnalyticFilteredPointCurveStatus : std::uint8_t
{
    certified_on_domain = 0,
    outside_domain = 1,
    uncertain = 2,
    invalid_argument = 3,
};

[[nodiscard]] bool analytic_filtered_curve_is_valid(const AnalyticAtomicCurveNm& curve) noexcept;

// Revalidates a filtered point at a stage boundary. Certification requires the
// complete point enclosure to remain within the fixed topology-resolution
// distance of the carrier and its finite line/arc domain. This is deliberately
// independent of construction provenance so a malformed intermediate result
// cannot publish an off-carrier split.
[[nodiscard]] AnalyticFilteredPointCurveStatus
classify_analytic_filtered_point_on_curve(const AnalyticAtomicCurveNm& curve,
                                          const AnalyticFilteredPointNm& candidate) noexcept;

// Intersects only the supplied broad-phase candidates. Input coordinates and
// radii are outward bounds on one authored curve; each bound must itself fit
// the fixed 50 nm displacement envelope. Optional integer certificates bind
// singleton coordinates and enable exact fixed-width signs. Trusted lowering
// may also attach job-local construction proof tokens for correlated
// non-integral carriers and arc sweeps; neither certificate stores algebraic
// expressions.
// Curves use dense canonical indices 1..N, and pairs are in strictly increasing
// index order; this makes pair resolution direct O(1) work instead of a hidden
// per-pair search. No implicit all-curves cross product is performed.
// Coincident carriers are identified here; their finite overlap spans are
// deliberately owned by the subsequent same-domain overlay stage.
[[nodiscard]] AnalyticNarrowPhaseResult
intersect_analytic_curve_candidates(const std::vector<AnalyticAtomicCurveNm>& curves,
                                    const std::vector<AnalyticCurvePair>& candidate_pairs,
                                    const AnalyticSolverLimits& limits = {});

} // namespace geometer

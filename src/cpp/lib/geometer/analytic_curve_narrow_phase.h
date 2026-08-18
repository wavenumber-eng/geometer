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
// carrier pair share an exact x coordinate. A symmetric-root token proves one
// named left/right line-circle root is shared by a lowering-certified pair of
// horizontal mirror lines around the circle's exact integer y coordinate. An
// endpoint-arc partition token
// correlates overlapping event enclosures around one normalized endpoint and
// one cardinal side so the face sweep handles them in one atomic column; it
// does not prove coordinate equality and may never merge vertices. No token
// stores an algebraic expression or coordinate.
inline constexpr std::uint64_t kAnalyticVerticalXColumnTag = std::uint64_t{1} << 63U;
inline constexpr std::uint64_t kAnalyticSymmetricRootXColumnTag = std::uint64_t{5} << 61U;
inline constexpr std::uint64_t kAnalyticEndpointArcLeftColumnTag = std::uint64_t{3} << 62U;
inline constexpr std::uint64_t kAnalyticEndpointArcRightColumnTag = std::uint64_t{7} << 61U;
inline constexpr std::uint64_t kAnalyticEndpointTangentTag = std::uint64_t{0xA1} << 56U;
inline constexpr std::uint64_t kAnalyticCircleEndpointTangentTag = std::uint64_t{0xC4} << 56U;
inline constexpr std::uint64_t kAnalyticIntegerLineIntersectionTag = std::uint64_t{0xB2} << 48U;
inline constexpr std::uint64_t kAnalyticConstructionCarrierCount = std::uint64_t{1} << 17U;

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
analytic_integer_line_intersection_token(std::uint64_t first_carrier_id,
                                         std::uint64_t second_carrier_id) noexcept
{
    const std::uint64_t first =
        first_carrier_id < second_carrier_id ? first_carrier_id : second_carrier_id;
    const std::uint64_t second =
        first_carrier_id < second_carrier_id ? second_carrier_id : first_carrier_id;
    if (first == 0 || first == second || first > kAnalyticConstructionCarrierCount ||
        second > kAnalyticConstructionCarrierCount)
        return 0;
    return kAnalyticIntegerLineIntersectionTag | ((first - 1U) << 17U) | (second - 1U);
}

[[nodiscard]] inline constexpr bool
analytic_is_integer_line_intersection_token(std::uint64_t token) noexcept
{
    return (token >> 48U) == 0xB2U;
}

[[nodiscard]] inline constexpr bool
analytic_integer_line_intersection_contains_carrier(std::uint64_t token,
                                                    std::uint64_t carrier_id) noexcept
{
    if (!analytic_is_integer_line_intersection_token(token) || carrier_id == 0 ||
        carrier_id > kAnalyticConstructionCarrierCount)
        return false;
    const std::uint64_t payload = token & ((std::uint64_t{1} << 34U) - 1U);
    return ((payload >> 17U) + 1U) == carrier_id ||
           ((payload & ((std::uint64_t{1} << 17U) - 1U)) + 1U) == carrier_id;
}

// A horizontal-mirror construction id is internal-only. It binds the two
// distinct exact line carriers emitted by one horizontal capsule construction;
// duplicate constructions reuse the same carrier pair and therefore the same
// id. Carrier ids are bounded by the governed boundary-occurrence ceiling.
[[nodiscard]] inline constexpr std::uint64_t
analytic_horizontal_mirror_construction_id(std::uint64_t first_carrier_id,
                                           std::uint64_t second_carrier_id) noexcept
{
    const std::uint64_t first =
        first_carrier_id < second_carrier_id ? first_carrier_id : second_carrier_id;
    const std::uint64_t second =
        first_carrier_id < second_carrier_id ? second_carrier_id : first_carrier_id;
    if (first == 0 || first == second || first > kAnalyticConstructionCarrierCount ||
        second > kAnalyticConstructionCarrierCount)
        return 0;
    return ((first - 1U) << 17U) | (second - 1U) | (std::uint64_t{1} << 34U);
}

[[nodiscard]] inline constexpr bool
analytic_horizontal_mirror_contains_carrier(std::uint64_t construction_id,
                                            std::uint64_t carrier_id) noexcept
{
    if ((construction_id >> 34U) != 1U || carrier_id == 0 ||
        carrier_id > kAnalyticConstructionCarrierCount)
        return false;
    const std::uint64_t payload = construction_id & ((std::uint64_t{1} << 34U) - 1U);
    return ((payload >> 17U) + 1U) == carrier_id ||
           ((payload & ((std::uint64_t{1} << 17U) - 1U)) + 1U) == carrier_id;
}

// Internal-only identity for one lowering-proven tangent line/arc endpoint.
// The payload binds both exact carrier identities and the named endpoint on
// each carrier. It never promotes proximity to tangency.
[[nodiscard]] inline constexpr std::uint64_t
analytic_endpoint_tangent_token(std::uint64_t line_carrier_id, bool line_start,
                                std::uint64_t arc_carrier_id, bool arc_start) noexcept
{
    if (line_carrier_id == 0 || line_carrier_id > kAnalyticConstructionCarrierCount ||
        arc_carrier_id == 0 || arc_carrier_id > kAnalyticConstructionCarrierCount)
        return 0;
    return kAnalyticEndpointTangentTag | ((line_start ? std::uint64_t{1} : 0) << 34U) |
           ((arc_start ? std::uint64_t{1} : 0) << 35U) | ((line_carrier_id - 1U) << 17U) |
           (arc_carrier_id - 1U);
}

[[nodiscard]] inline constexpr bool analytic_is_endpoint_tangent_token(std::uint64_t token) noexcept
{
    return (token >> 56U) == 0xA1U;
}

[[nodiscard]] inline constexpr bool
analytic_endpoint_tangent_line_starts(std::uint64_t token) noexcept
{
    return analytic_is_endpoint_tangent_token(token) && ((token >> 34U) & 1U) != 0;
}

[[nodiscard]] inline constexpr bool
analytic_endpoint_tangent_arc_starts(std::uint64_t token) noexcept
{
    return analytic_is_endpoint_tangent_token(token) && ((token >> 35U) & 1U) != 0;
}

// Internal-only identity for a lowering-proven tangent endpoint on two
// distinct circle carriers. The construction identity is minted from the
// authored swept-path vertex and its exact incident tangent ray; it is not a
// proximity or carrier-coincidence rule.
[[nodiscard]] inline constexpr std::uint64_t
analytic_circle_endpoint_tangent_token(std::uint64_t first_carrier_id, bool first_start,
                                       std::uint64_t second_carrier_id, bool second_start,
                                       std::uint64_t construction_identity) noexcept
{
    if (first_carrier_id == 0 || second_carrier_id == 0 || first_carrier_id == second_carrier_id ||
        first_carrier_id > kAnalyticConstructionCarrierCount ||
        second_carrier_id > kAnalyticConstructionCarrierCount || construction_identity == 0 ||
        construction_identity >= (std::uint64_t{1} << 20U))
        return 0;
    if (second_carrier_id < first_carrier_id)
    {
        const std::uint64_t carrier = first_carrier_id;
        first_carrier_id = second_carrier_id;
        second_carrier_id = carrier;
        const bool start = first_start;
        first_start = second_start;
        second_start = start;
    }
    return kAnalyticCircleEndpointTangentTag | (construction_identity << 36U) |
           ((first_start ? std::uint64_t{1} : 0) << 34U) |
           ((second_start ? std::uint64_t{1} : 0) << 35U) | ((first_carrier_id - 1U) << 17U) |
           (second_carrier_id - 1U);
}

[[nodiscard]] inline constexpr bool
analytic_is_circle_endpoint_tangent_token(std::uint64_t token) noexcept
{
    return (token >> 56U) == 0xC4U;
}

[[nodiscard]] inline constexpr std::uint64_t
analytic_circle_endpoint_tangent_identity(std::uint64_t token) noexcept
{
    return analytic_is_circle_endpoint_tangent_token(token)
               ? (token >> 36U) & ((std::uint64_t{1} << 20U) - 1U)
               : 0;
}

[[nodiscard]] inline constexpr std::uint64_t analytic_symmetric_line_circle_root_x_column_token(
    std::uint64_t mirror_construction_id, std::uint64_t circle_carrier_id, bool right_root) noexcept
{
    if ((mirror_construction_id >> 34U) != 1U || circle_carrier_id == 0 ||
        circle_carrier_id > kAnalyticConstructionCarrierCount)
        return 0;
    const std::uint64_t mirror_payload = mirror_construction_id & ((std::uint64_t{1} << 34U) - 1U);
    const std::uint64_t payload =
        (mirror_payload << 18U) | ((circle_carrier_id - 1U) << 1U) | (right_root ? 1U : 0U);
    return kAnalyticSymmetricRootXColumnTag | payload;
}

[[nodiscard]] inline constexpr bool
analytic_is_symmetric_line_circle_root_x_column_token(std::uint64_t token) noexcept
{
    return (token >> 61U) == 5U;
}

[[nodiscard]] inline constexpr std::uint64_t
analytic_endpoint_arc_partition_column_token(std::uint64_t group_id, bool right) noexcept
{
    if (group_id == 0 || group_id >= (std::uint64_t{1} << 61U))
        return 0;
    return (right ? kAnalyticEndpointArcRightColumnTag : kAnalyticEndpointArcLeftColumnTag) |
           group_id;
}

[[nodiscard]] inline constexpr bool
analytic_is_endpoint_arc_partition_column_token(std::uint64_t token) noexcept
{
    const std::uint64_t tag = token >> 61U;
    return (tag == 6 || tag == 7) && (token & ((std::uint64_t{1} << 61U) - 1U)) != 0;
}

[[nodiscard]] inline constexpr bool
analytic_endpoint_arc_partition_column_is_right(std::uint64_t token) noexcept
{
    return (token >> 61U) == 7;
}

[[nodiscard]] inline constexpr std::uint64_t
analytic_endpoint_arc_partition_column_group(std::uint64_t token) noexcept
{
    return analytic_is_endpoint_arc_partition_column_token(token)
               ? token & ((std::uint64_t{1} << 61U) - 1U)
               : 0;
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

[[nodiscard]] inline constexpr bool analytic_endpoint_tangent_matches(std::uint64_t token,
                                                                      AnalyticAtomicCurveKind kind,
                                                                      std::uint64_t carrier_id,
                                                                      bool endpoint_start) noexcept
{
    if (!analytic_is_endpoint_tangent_token(token) || carrier_id == 0 ||
        carrier_id > kAnalyticConstructionCarrierCount)
    {
        if (!analytic_is_circle_endpoint_tangent_token(token) ||
            kind != AnalyticAtomicCurveKind::circular_arc || carrier_id == 0 ||
            carrier_id > kAnalyticConstructionCarrierCount)
            return false;
        const std::uint64_t first = ((token >> 17U) & ((std::uint64_t{1} << 17U) - 1U)) + 1U;
        const std::uint64_t second = (token & ((std::uint64_t{1} << 17U) - 1U)) + 1U;
        if (carrier_id == first)
            return (((token >> 34U) & 1U) != 0) == endpoint_start;
        return carrier_id == second && (((token >> 35U) & 1U) != 0) == endpoint_start;
    }
    if (kind == AnalyticAtomicCurveKind::line)
        return ((token >> 17U) & ((std::uint64_t{1} << 17U) - 1U)) + 1U == carrier_id &&
               (((token >> 34U) & 1U) != 0) == endpoint_start;
    if (kind == AnalyticAtomicCurveKind::circular_arc)
        return (token & ((std::uint64_t{1} << 17U) - 1U)) + 1U == carrier_id &&
               (((token >> 35U) & 1U) != 0) == endpoint_start;
    return false;
}

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
    // Lowering-only certificate for the two exact horizontal line carriers of
    // one capsule. Both lines carry the same nonzero id and exact integer
    // mirror-axis y. Request records cannot mint this identity.
    std::uint64_t construction_horizontal_mirror_id = 0;
    std::int64_t construction_horizontal_mirror_axis_y = 0;
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
    // Trusted lowering-only endpoint identities. Equal nonzero values on one
    // line and one arc prove that their named construction endpoints are the
    // same exact tangent contact before normalization.
    std::uint64_t construction_start_tangent_id = 0;
    std::uint64_t construction_end_tangent_id = 0;
};

// Target-independent logical charge for one retained atomic curve record.
// The value deliberately covers native ABI padding as well as wasm32 layout.
inline constexpr std::uint64_t kAnalyticAtomicCurveLogicalBytes = 304;

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
    std::uint64_t required_working_memory_bytes = 0;
    std::uint64_t algebraic_fallback_calls = 0;
    bool unresolved_predicate_failure = false;
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

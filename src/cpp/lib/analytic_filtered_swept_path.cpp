#include "analytic_filtered_lowering_internal.h"

#include "analytic_filtered_boolean_selection_support.h"
#include "analytic_filtered_execution_policy.h"
#include "analytic_filtered_interval.h"
#include "analytic_filtered_outcome_tracker.h"
#include "analytic_wide_integer.h"
#include "geometer/analytic_curve_broad_phase.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

namespace geometer::analytic_lowering_detail
{
namespace
{

using namespace analytic_detail;

constexpr std::uint64_t kPieceCurveBytes = 768;
constexpr std::uint64_t kPieceCoverageBytes = 64;
constexpr std::uint64_t kSyntheticOperandBytes = 32;
constexpr std::uint64_t kSyntheticJobBytes = 32;
constexpr std::uint64_t kSyntheticStageBytes = 32;
constexpr std::uint64_t kMirrorPairBytes = 24;
constexpr std::uint64_t kSegmentTangentBytes = 192;
constexpr std::uint64_t kVertexRayBytes = 96;
constexpr std::uint64_t kOutputEndpointBytes = 64;
constexpr std::uint64_t kEmittedTangencyBytes = 24;
constexpr std::uint64_t kOutputCurveBytes = 512;
constexpr std::uint64_t kSegmentBytes = 96;
constexpr std::uint64_t kIndexBytes = 8;
constexpr std::int64_t kMaximumSpanNm = 1'000'000'000'000;

struct Segment
{
    AnalyticIntegerPointNm start;
    AnalyticIntegerPointNm end;
    AnalyticIntegerPointNm center;
    WideInteger radius_squared{};
    bool arc = false;
    bool counterclockwise = true;
    bool major = false;
};

struct PieceSource
{
    AnalyticFilteredSourceReference source;
    TokenDescriptor descriptor;
    bool construction = false;
    std::uint64_t start_tangent_key = 0;
    std::uint64_t end_tangent_key = 0;
};

struct MirrorPair
{
    std::uint32_t first = 0;
    std::uint32_t second = 0;
    std::int64_t axis_y = 0;
};

struct SegmentTangentPoints
{
    std::array<Point, 4> points;
    std::array<std::uint64_t, 4> keys{};
};

struct VertexRay
{
    std::uint32_t vertex = 0;
    std::uint32_t slot = 0;
    std::int64_t dx = 0;
    std::int64_t dy = 0;
    Point endpoint;
    std::uint64_t key = 0;
};

struct OutputEndpoint
{
    AnalyticFilteredPointNm point;
    std::uint32_t curve = 0;
    std::uint64_t key = 0;
    bool start = false;
};

enum class PieceContainment : std::uint8_t
{
    proven_outside,
    proven_inside,
    uncertain,
};

static_assert(sizeof(Segment) <= kSegmentBytes);
static_assert(sizeof(MirrorPair) <= kMirrorPairBytes);
static_assert(sizeof(SegmentTangentPoints) <= kSegmentTangentBytes);
static_assert(sizeof(VertexRay) <= kVertexRayBytes);
static_assert(sizeof(OutputEndpoint) <= kOutputEndpointBytes);
static_assert(sizeof(EmittedEndpointTangency) <= kEmittedTangencyBytes);
static_assert(sizeof(AnalyticRequestJobRecord) <= kSyntheticJobBytes);
static_assert(sizeof(AnalyticRequestStageRecord) <= kSyntheticStageBytes);
static_assert(sizeof(AnalyticRequestOperandRecord) <= kSyntheticOperandBytes);
static_assert(sizeof(EmittedCurve) <= kOutputCurveBytes);

std::uint64_t magnitude(std::int64_t value) noexcept
{
    return value < 0 ? std::uint64_t{0} - static_cast<std::uint64_t>(value)
                     : static_cast<std::uint64_t>(value);
}

LineFamilyKey canonical_direction(std::int64_t dx, std::int64_t dy) noexcept
{
    const std::uint64_t divisor = std::gcd(magnitude(dx), magnitude(dy));
    dx /= static_cast<std::int64_t>(divisor);
    dy /= static_cast<std::int64_t>(divisor);
    if (dx < 0 || (dx == 0 && dy < 0))
    {
        dx = -dx;
        dy = -dy;
    }
    return {dx, dy};
}

WideInteger squared_distance(AnalyticIntegerPointNm left, AnalyticIntegerPointNm right) noexcept
{
    const std::int64_t dx = left.x - right.x;
    const std::int64_t dy = left.y - right.y;
    return wide_add(wide_multiply(dx, dx), wide_multiply(dy, dy));
}

WideInteger cross_vectors(std::int64_t ax, std::int64_t ay, std::int64_t bx,
                          std::int64_t by) noexcept
{
    return wide_subtract(wide_multiply(ax, by), wide_multiply(ay, bx));
}

WideInteger dot_vectors(std::int64_t ax, std::int64_t ay, std::int64_t bx, std::int64_t by) noexcept
{
    return wide_add(wide_multiply(ax, bx), wide_multiply(ay, by));
}

AnalyticCoordinateIntervalNm public_interval(Interval value) noexcept
{
    return {value.lower, value.upper};
}

AnalyticFilteredPointNm public_point(Point value) noexcept
{
    return {public_interval(value.x), public_interval(value.y)};
}

Point point(AnalyticIntegerPointNm value) noexcept
{
    return {exact(static_cast<double>(value.x)), exact(static_cast<double>(value.y))};
}

bool wide_less(WideInteger left, WideInteger right) noexcept
{
    return wide_compare(left, right) < 0;
}

bool line_family_less(const LineFamilyKey& left, const LineFamilyKey& right) noexcept
{
    return std::tie(left.dx, left.dy) < std::tie(right.dx, right.dy);
}

bool circle_family_less(const CircleFamilyKey& left, const CircleFamilyKey& right) noexcept
{
    return std::tie(left.x, left.y) < std::tie(right.x, right.y);
}

bool descriptor_family_less(const TokenDescriptor& left, const TokenDescriptor& right) noexcept
{
    if (left.kind != right.kind)
        return left.kind < right.kind;
    return left.kind == TokenKeyKind::line
               ? line_family_less(left.line.family, right.line.family)
               : circle_family_less(left.circle.family, right.circle.family);
}

bool descriptor_family_equal(const TokenDescriptor& left, const TokenDescriptor& right) noexcept
{
    return !descriptor_family_less(left, right) && !descriptor_family_less(right, left);
}

bool descriptor_carrier_less(const TokenDescriptor& left, const TokenDescriptor& right) noexcept
{
    if (descriptor_family_less(left, right))
        return true;
    if (descriptor_family_less(right, left))
        return false;
    if (left.kind == TokenKeyKind::line)
    {
        if (wide_less(left.line.rational_part_times_two, right.line.rational_part_times_two))
            return true;
        if (wide_less(right.line.rational_part_times_two, left.line.rational_part_times_two))
            return false;
        return left.line.radical_coefficient < right.line.radical_coefficient;
    }
    if (wide_less(left.circle.rational_part, right.circle.rational_part))
        return true;
    if (wide_less(right.circle.rational_part, left.circle.rational_part))
        return false;
    if (left.circle.radical_coefficient != right.circle.radical_coefficient)
        return left.circle.radical_coefficient < right.circle.radical_coefficient;
    return wide_less(left.circle.radicand, right.circle.radicand);
}

bool descriptor_carrier_equal(const TokenDescriptor& left, const TokenDescriptor& right) noexcept
{
    return !descriptor_carrier_less(left, right) && !descriptor_carrier_less(right, left);
}

class SweptLowerer
{
  public:
#include "analytic_filtered_swept_path_setup.h"

#include "analytic_filtered_swept_path_geometry.h"

    bool run_local_union()
    {
        if (!charge(pieces_.curves.size()))
            return false;
        for (std::size_t index = 0; index < pieces_.curves.size(); ++index)
            if (!analytic_filtered_curve_is_valid(pieces_.curves[index]))
                return fail(AnalyticFilteredLoweringError::invalid_topology);
        AnalyticSolverLimits phase_limits = remaining_limits();
        constexpr auto kLocalUnionPolicy = analytic_execution_detail::TopologyPolicy::
            resolution_50nm_preserve_integer_construction_endpoints;
        AnalyticBroadPhaseResult initial_broad = analytic_execution_detail::build_curve_candidates(
            pieces_.bounds, phase_limits, kLocalUnionPolicy);
        add_phase(initial_broad.telemetry.work_units,
                  initial_broad.telemetry.peak_working_memory_bytes,
                  initial_broad.telemetry.required_working_memory_bytes);
        if (initial_broad.error != AnalyticBroadPhaseError::none)
            return fail(initial_broad.error == AnalyticBroadPhaseError::invalid_argument
                            ? AnalyticFilteredLoweringError::invalid_topology
                            : AnalyticFilteredLoweringError::resource_limit_exceeded);
        if (!retain(initial_broad.telemetry.retained_pair_bytes))
            return false;
        const std::uint64_t initial_pair_bytes = initial_broad.telemetry.retained_pair_bytes;
        phase_limits = remaining_limits();
        AnalyticNarrowPhaseResult local_narrow =
            analytic_execution_detail::intersect_curve_candidates(
                pieces_.curves, initial_broad.pairs, phase_limits, kLocalUnionPolicy);
        add_phase(local_narrow.telemetry.predicate_calls,
                  local_narrow.telemetry.peak_working_memory_bytes,
                  local_narrow.telemetry.required_working_memory_bytes);
        if (local_narrow.error != AnalyticNarrowPhaseError::none)
            return fail(local_narrow.error == AnalyticNarrowPhaseError::invalid_argument
                            ? AnalyticFilteredLoweringError::invalid_topology
                            : AnalyticFilteredLoweringError::resource_limit_exceeded);
        const std::uint64_t disk_coverage_count = segments_.size() + 1;
        if (local_narrow.intersections.size() > std::numeric_limits<std::uint64_t>::max() / 16U ||
            !charge(local_narrow.intersections.size() * 16U))
            return false;
        for (const AnalyticPairIntersection& intersection : local_narrow.intersections)
        {
            const std::uint32_t first = intersection.pair.first;
            const std::uint32_t second = intersection.pair.second;
            if (first == 0 || second == 0 || first > pieces_.occurrences.size() ||
                second > pieces_.occurrences.size())
                return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
            const std::uint64_t first_coverage = pieces_.occurrences[first - 1].coverage_id;
            const std::uint64_t second_coverage = pieces_.occurrences[second - 1].coverage_id;
            if (intersection.resolution_collapsed && !trusted_collapsed_tangency(intersection))
                return fail(AnalyticFilteredLoweringError::invalid_topology);
            if (first_coverage != second_coverage && first_coverage <= disk_coverage_count &&
                second_coverage <= disk_coverage_count)
            {
                const auto disk_center = [&](std::uint64_t coverage)
                {
                    const std::size_t vertex = static_cast<std::size_t>(coverage - 1);
                    return vertex == segments_.size() ? segments_.back().end
                                                      : segments_[vertex].start;
                };
                const WideInteger center_distance_squared =
                    squared_distance(disk_center(first_coverage), disk_center(second_coverage));
                const std::uint64_t coverage_distance = first_coverage > second_coverage
                                                            ? first_coverage - second_coverage
                                                            : second_coverage - first_coverage;
                if (wide_sign(center_distance_squared) == 0 || coverage_distance != 1)
                    return fail(AnalyticFilteredLoweringError::invalid_topology);
            }
        }
        local_narrow = {};
        phase_limits = remaining_limits();
        AnalyticFilteredOverlayResult initial_overlay = analytic_execution_detail::build_overlay(
            pieces_, initial_broad.pairs, phase_limits, kLocalUnionPolicy);
        add_phase(initial_overlay.telemetry.predicate_calls,
                  initial_overlay.telemetry.peak_working_memory_bytes,
                  initial_overlay.telemetry.required_working_memory_bytes);
        if (initial_overlay.error != AnalyticFilteredOverlayError::none)
            return fail(initial_overlay.error == AnalyticFilteredOverlayError::invalid_argument
                            ? AnalyticFilteredLoweringError::invalid_topology
                            : AnalyticFilteredLoweringError::resource_limit_exceeded);
        const std::uint64_t overlay_bytes =
            initial_overlay.spans.size() * kAnalyticOverlaySpanLogicalBytes +
            initial_overlay.memberships.size() * kAnalyticOverlayMembershipLogicalBytes;
        if (!retain(overlay_bytes) || !project_union_spans(initial_overlay))
            return false;
        retained_piece_bytes_ -= overlay_bytes;
        retained_piece_bytes_ -= initial_pair_bytes;
        initial_overlay = {};
        initial_broad = {};

        AnalyticRequestPacketRecords local_records;
        constexpr std::uint32_t piece_count = 1;
        if (!charge(piece_count))
            return false;
        local_records.jobs = {{1, 0, 1}};
        local_records.stages = {{1, 1, 0, piece_count}};
        local_records.operands = {{1, 2, 0}};

        phase_limits = remaining_limits();
        const AnalyticBroadPhaseResult broad = analytic_execution_detail::build_curve_candidates(
            pieces_.bounds, phase_limits, kLocalUnionPolicy);
        add_phase(broad.telemetry.work_units, broad.telemetry.peak_working_memory_bytes,
                  broad.telemetry.required_working_memory_bytes);
        if (broad.error != AnalyticBroadPhaseError::none)
            return fail(broad.error == AnalyticBroadPhaseError::invalid_argument
                            ? AnalyticFilteredLoweringError::invalid_topology
                            : AnalyticFilteredLoweringError::resource_limit_exceeded);
        if (!retain(broad.telemetry.retained_pair_bytes))
            return false;
        phase_limits = remaining_limits();
        AnalyticFilteredBooleanSelectionResult selection =
            analytic_execution_detail::build_boolean_selection(
                local_records, 0, pieces_, broad.pairs, phase_limits, kLocalUnionPolicy);
        add_phase(selection.telemetry.predicate_calls,
                  selection.telemetry.peak_working_memory_bytes,
                  selection.telemetry.required_working_memory_bytes);
        if (selection.error != AnalyticFilteredBooleanSelectionError::none)
            return fail(selection.error == AnalyticFilteredBooleanSelectionError::invalid_argument
                            ? AnalyticFilteredLoweringError::invalid_topology
                            : AnalyticFilteredLoweringError::resource_limit_exceeded);

        const auto& arrangement = selection.arrangement;
        const std::uint64_t edge_scratch_bytes = arrangement.edges.size() * kIndexBytes * 2;
        const std::uint64_t projected_output_bytes = arrangement.edges.size() * kOutputCurveBytes;
        const std::uint64_t tangent_scratch_bytes =
            arrangement.edges.size() * (kOutputEndpointBytes * 2U + kEmittedTangencyBytes);
        std::uint64_t selection_bytes = 0;
        bool valid_memory = selection_retained_bytes(selection, selection_bytes);
        std::uint64_t post_selection_bytes = analytic_selection_detail::checked_add(
            retained_piece_bytes_, selection_bytes, valid_memory);
        post_selection_bytes = analytic_selection_detail::checked_add(
            post_selection_bytes, edge_scratch_bytes, valid_memory);
        post_selection_bytes = analytic_selection_detail::checked_add(
            post_selection_bytes, projected_output_bytes, valid_memory);
        post_selection_bytes = analytic_selection_detail::checked_add(
            post_selection_bytes, tangent_scratch_bytes, valid_memory);
        if (!valid_memory)
            return fail_memory(std::numeric_limits<std::uint64_t>::max());
        if (post_selection_bytes > limits_.working_memory_bytes)
            return fail_memory(post_selection_bytes);
        telemetry_.peak_working_memory_bytes =
            std::max(telemetry_.peak_working_memory_bytes, post_selection_bytes);

        std::uint64_t coverage_depth = 0;
        for (std::uint64_t capacity = 1; capacity < piece_count; capacity <<= 1U)
            ++coverage_depth;
        bool valid_work = true;
        std::uint64_t post_selection_work = analytic_selection_detail::checked_add(
            arrangement.half_edges.size(), arrangement.edges.size(), valid_work);
        post_selection_work = analytic_selection_detail::checked_add(
            post_selection_work,
            analytic_selection_detail::checked_multiply(arrangement.memberships.size(),
                                                        coverage_depth + 1, valid_work),
            valid_work);
        post_selection_work = analytic_selection_detail::checked_add(
            post_selection_work, arrangement.edges.size() * 6U, valid_work);
        if (!valid_work)
            return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
        if (!charge(post_selection_work))
            return false;
        std::vector<std::uint32_t> forward(arrangement.edges.size(),
                                           std::numeric_limits<std::uint32_t>::max());
        std::vector<std::uint32_t> reverse(arrangement.edges.size(),
                                           std::numeric_limits<std::uint32_t>::max());
        std::vector<OutputEndpoint> tangent_endpoints;
        tangent_endpoints.reserve(arrangement.edges.size() * 2U);
        for (std::uint32_t half = 0; half < arrangement.half_edges.size(); ++half)
        {
            const auto& value = arrangement.half_edges[half];
            (value.forward ? forward : reverse)[value.edge] = half;
        }
        output_.reserve(arrangement.edges.size());
        output_tangencies_.reserve(arrangement.edges.size());
        output_capacity_bytes_ =
            arrangement.edges.size() * (kOutputCurveBytes + kEmittedTangencyBytes);
        for (std::uint32_t edge_index = 0; edge_index < arrangement.edges.size(); ++edge_index)
        {
            const std::uint32_t f = forward[edge_index];
            const std::uint32_t r = reverse[edge_index];
            if (f >= selection.half_edge_faces.size() || r >= selection.half_edge_faces.size())
                return fail(AnalyticFilteredLoweringError::invalid_topology);
            const std::uint32_t left_face = selection.half_edge_faces[f];
            const std::uint32_t right_face = selection.half_edge_faces[r];
            if (left_face >= selection.faces.size() || right_face >= selection.faces.size())
                return fail(AnalyticFilteredLoweringError::invalid_topology);
            const bool left_material = selection.faces[left_face].material;
            const bool right_material = selection.faces[right_face].material;
            if (left_material == right_material)
                continue;
            const std::uint32_t material_face = left_material ? left_face : right_face;
            const auto& edge = arrangement.edges[edge_index];
            std::uint32_t chosen = std::numeric_limits<std::uint32_t>::max();
            bool construction_covers = false;
            for (std::uint32_t offset = 0; offset < edge.membership_count; ++offset)
            {
                const auto& membership = arrangement.memberships[edge.membership_begin + offset];
                if (membership.curve_index == 0 ||
                    membership.curve_index > pieces_.occurrences.size())
                    return fail(AnalyticFilteredLoweringError::invalid_topology);
                const std::uint32_t curve_index = membership.curve_index - 1;
                const std::uint64_t coverage = pieces_.occurrences[curve_index].coverage_id;
                if (coverage == 0 || coverage > piece_count ||
                    !coverage_contains(selection,
                                       selection.faces[material_face].coverage_state_root,
                                       static_cast<std::uint32_t>(coverage - 1), piece_count))
                    continue;
                if (piece_sources_[curve_index].construction)
                    construction_covers = true;
                else
                    chosen = std::min(chosen, curve_index);
            }
            if (construction_covers)
                return fail(AnalyticFilteredLoweringError::invalid_topology);
            if (chosen == std::numeric_limits<std::uint32_t>::max())
                return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
            AnalyticAtomicCurveNm curve;
            curve.kind = edge.kind;
            curve.start = edge.carrier_start;
            curve.end = edge.carrier_end;
            curve.start.construction_x_column_id = 0;
            curve.end.construction_x_column_id = 0;
            curve.circle = edge.circle;
            curve.counterclockwise = true;
            curve.major_arc = edge.major_arc;
            curve.has_arc_sweep_certificate = edge.kind == AnalyticAtomicCurveKind::circular_arc;
            curve.has_construction_line_direction = edge.has_construction_line_direction;
            curve.construction_line_dx = edge.construction_line_dx;
            curve.construction_line_dy = edge.construction_line_dy;
            if (curve.kind == AnalyticAtomicCurveKind::line)
                certify_integer_line(curve);
            output_.push_back({curve, true, left_material, piece_sources_[chosen].source,
                               piece_sources_[chosen].descriptor});
            const PieceSource& selected_source = piece_sources_[chosen];
            const AnalyticAtomicCurveNm& selected_curve = pieces_.curves[chosen];
            const auto key_at = [&](const AnalyticFilteredPointNm& value)
            {
                if (same_filtered_point(value, selected_curve.start))
                    return selected_source.start_tangent_key;
                if (same_filtered_point(value, selected_curve.end))
                    return selected_source.end_tangent_key;
                return std::uint64_t{0};
            };
            const std::uint32_t output_index = static_cast<std::uint32_t>(output_.size() - 1U);
            const std::uint64_t start_key = key_at(curve.start);
            const std::uint64_t end_key = key_at(curve.end);
            if (start_key != 0)
                tangent_endpoints.push_back({curve.start, output_index, start_key, true});
            if (end_key != 0)
                tangent_endpoints.push_back({curve.end, output_index, end_key, false});
        }
        const std::uint64_t tangent_levels =
            tangent_endpoints.size() < 2
                ? 0
                : static_cast<std::uint64_t>(std::ceil(std::log2(tangent_endpoints.size())));
        if (!charge(tangent_endpoints.size() * (tangent_levels + 3U)))
            return false;
        std::sort(
            tangent_endpoints.begin(), tangent_endpoints.end(),
            [](const OutputEndpoint& left, const OutputEndpoint& right)
            {
                return std::tie(left.key, left.point.x.lower, left.point.x.upper,
                                left.point.y.lower, left.point.y.upper, left.curve, left.start) <
                       std::tie(right.key, right.point.x.lower, right.point.x.upper,
                                right.point.y.lower, right.point.y.upper, right.curve, right.start);
            });
        const auto source_names_vertex =
            [&](const AnalyticFilteredSourceReference& source, std::uint32_t vertex)
        {
            switch (source.role)
            {
            case AnalyticFilteredSourceRole::swept_start_cap:
                return vertex == 0;
            case AnalyticFilteredSourceRole::swept_end_cap:
                return vertex == segments_.size();
            case AnalyticFilteredSourceRole::swept_round_join:
                return vertex != 0 && vertex < segments_.size() &&
                       (source.secondary_id >> 32U) == vertex;
            case AnalyticFilteredSourceRole::swept_left_offset_arc:
            case AnalyticFilteredSourceRole::swept_right_offset_arc:
            {
                const std::uint32_t segment =
                    static_cast<std::uint32_t>((source.secondary_id >> 32U) - 1U);
                return segment < segments_.size() && (vertex == segment || vertex == segment + 1U);
            }
            default:
                return false;
            }
        };
        for (std::size_t begin = 0; begin < tangent_endpoints.size();)
        {
            std::size_t end = begin + 1;
            while (end < tangent_endpoints.size() &&
                   tangent_endpoints[end].key == tangent_endpoints[begin].key)
                ++end;
            if (end - begin == 1)
            {
                begin = end;
                continue;
            }
            if (end - begin != 2 || !same_filtered_point(tangent_endpoints[begin].point,
                                                         tangent_endpoints[begin + 1U].point))
                return fail(AnalyticFilteredLoweringError::invalid_topology);
            const OutputEndpoint& first_endpoint = tangent_endpoints[begin];
            const OutputEndpoint& second_endpoint = tangent_endpoints[begin + 1U];
            const auto first_kind = output_[first_endpoint.curve].curve.kind;
            const auto second_kind = output_[second_endpoint.curve].curve.kind;
            const std::uint32_t vertex =
                static_cast<std::uint32_t>((first_endpoint.key >> 32U) - 1U);
            const auto endpoint_valid =
                [&](const OutputEndpoint& endpoint, AnalyticAtomicCurveKind kind)
            {
                const auto& source = output_[endpoint.curve].source;
                if (kind == AnalyticAtomicCurveKind::circular_arc)
                    return source_names_vertex(source, vertex);
                const bool line_role =
                    source.role == AnalyticFilteredSourceRole::swept_left_offset_line ||
                    source.role == AnalyticFilteredSourceRole::swept_right_offset_line;
                const std::uint32_t segment =
                    static_cast<std::uint32_t>((source.secondary_id >> 32U) - 1U);
                return line_role && segment < segments_.size() &&
                       (vertex == segment || vertex == segment + 1U);
            };
            if (!endpoint_valid(first_endpoint, first_kind) ||
                !endpoint_valid(second_endpoint, second_kind))
                return fail(AnalyticFilteredLoweringError::invalid_topology);
            const EmittedCurve& first_output = output_[first_endpoint.curve];
            const EmittedCurve& second_output = output_[second_endpoint.curve];
            if (first_kind == second_kind &&
                descriptor_carrier_equal(first_output.descriptor, second_output.descriptor))
            {
                // A legal authored split of one carrier retains both segment sources. Its common
                // finite-domain seam needs no replay token because the carrier has only one root.
                begin = end;
                continue;
            }
            if (first_kind == AnalyticAtomicCurveKind::line &&
                second_kind == AnalyticAtomicCurveKind::line)
                return fail(AnalyticFilteredLoweringError::invalid_topology);
            const std::uint64_t ray = first_endpoint.key & 0xffffffffULL;
            const std::uint64_t construction_identity =
                (std::uint64_t{vertex + 1U} << 2U) | (ray - 1U);
            if (ray == 0 || ray > 4U || construction_identity >= (std::uint64_t{1} << 20U))
                return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
            output_tangencies_.push_back({first_endpoint.curve, second_endpoint.curve,
                                          first_endpoint.start, second_endpoint.start,
                                          static_cast<std::uint32_t>(construction_identity)});
            begin = end;
        }
        return true;
    }

    const AnalyticRequestPacketRecords& records_;
    const AnalyticRequestOperandRecord& operand_;
    std::int64_t origin_x_nm_ = 0;
    std::int64_t origin_y_nm_ = 0;
    AnalyticSolverLimits limits_;
    const AnalyticRequestSweptPathRecord* swept_ = nullptr;
    const AnalyticRequestRingRecord* ring_ = nullptr;
    AnalyticFilteredLoweringError error_ = AnalyticFilteredLoweringError::none;
    AnalyticFilteredLoweringTelemetry telemetry_;
    std::vector<Segment> segments_;
    AnalyticFilteredGeometry centerline_;
    std::vector<TokenDescriptor> centerline_descriptors_;
    std::vector<SegmentTangentPoints> tangent_points_;
    std::vector<VertexRay> disk_rays_;
    AnalyticFilteredGeometry pieces_;
    std::vector<PieceSource> piece_sources_;
    std::vector<TokenDescriptor> piece_descriptors_;
    std::vector<MirrorPair> mirrors_;
    std::vector<EmittedCurve> output_;
    std::vector<EmittedEndpointTangency> output_tangencies_;
    std::uint64_t retained_piece_bytes_ = 0;
    std::uint64_t centerline_bytes_ = 0;
    std::uint64_t centerline_pair_bytes_ = 0;
    std::uint64_t output_capacity_bytes_ = 0;
};

} // namespace

SweptPathLoweringResult lower_filtered_swept_path(const AnalyticRequestPacketRecords& records,
                                                  const AnalyticRequestOperandRecord& operand,
                                                  std::int64_t origin_x_nm,
                                                  std::int64_t origin_y_nm,
                                                  const AnalyticSolverLimits& limits)
{
    return SweptLowerer(records, operand, origin_x_nm, origin_y_nm, limits).lower();
}

} // namespace geometer::analytic_lowering_detail

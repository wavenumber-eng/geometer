#include "geometer/analytic_filtered_normalization.h"

#include "analytic_endpoint_arc_reconstruction.h"
#include "analytic_filtered_interval.h"
#include "analytic_filtered_normalization_replay.h"
#include "analytic_wide_integer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <tuple>
#include <utility>

namespace geometer
{
namespace
{
using analytic_detail::add;
using analytic_detail::complete_distance_squared;
using analytic_detail::cross;
using analytic_detail::divide;
using analytic_detail::dot;
using analytic_detail::exact;
using analytic_detail::Interval;
using analytic_detail::multiply;
using analytic_detail::negate;
using analytic_detail::perpendicular;
using analytic_detail::Point;
using analytic_detail::scale;
using analytic_detail::square;
using analytic_detail::square_root;
using analytic_detail::subtract;
using analytic_detail::valid;
using analytic_detail::WideInteger;

constexpr std::uint64_t kVertexLogicalBytes = 24;
constexpr std::uint64_t kFragmentLogicalBytes = 32;
constexpr std::uint64_t kRingLogicalBytes = 24;
constexpr std::uint64_t kRegionLogicalBytes = 8;
constexpr std::uint64_t kReplayCurveLogicalBytes = kAnalyticAtomicCurveLogicalBytes;
constexpr std::uint64_t kNormalizationVertexPhaseBytes = 128;
constexpr std::uint64_t kNormalizationBoundaryPhaseBytes = 384;
constexpr std::uint64_t kNormalizationRingPhaseBytes = 64;
constexpr std::uint64_t kNormalizationRegionPhaseBytes = 32;
constexpr std::uint64_t kNormalizationFixedPhaseBytes = 4096;
constexpr std::uint64_t kNormalizationPossibleFragmentBytes = 4096;
constexpr std::uint64_t kNormalizationPossibleFragmentBaseWork = 128;
constexpr std::uint64_t kNormalizationPossibleFragmentLevelWork = 32;
constexpr double kResolutionSquared = 2500.0;

Point point(const AnalyticFilteredPointNm& value) noexcept
{
    return {{value.x.lower, value.x.upper}, {value.y.lower, value.y.upper}};
}

AnalyticFilteredPointNm filtered(Point value) noexcept
{
    return {{value.x.lower, value.x.upper}, {value.y.lower, value.y.upper}, 0};
}

Point exact_point(std::int64_t x, std::int64_t y) noexcept
{
    return {exact(static_cast<double>(x)), exact(static_cast<double>(y))};
}

bool checked_add_i64(std::int64_t left, std::int64_t right, std::int64_t& output) noexcept
{
    if ((right > 0 && left > std::numeric_limits<std::int64_t>::max() - right) ||
        (right < 0 && left < std::numeric_limits<std::int64_t>::min() - right))
        return false;
    output = left + right;
    return true;
}

bool checked_add_u64(std::uint64_t left, std::uint64_t right, std::uint64_t& output) noexcept
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
        return false;
    output = left + right;
    return true;
}

bool checked_multiply_u64(std::uint64_t left, std::uint64_t right, std::uint64_t& output) noexcept
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left)
        return false;
    output = left * right;
    return true;
}

std::uint64_t sort_units(std::uint64_t count) noexcept
{
    if (count < 2)
        return 0;
    std::uint64_t levels = 0;
    for (std::uint64_t value = count - 1; value != 0; value >>= 1U)
        ++levels;
    std::uint64_t output = 0;
    return checked_multiply_u64(count, levels, output) ? output
                                                       : std::numeric_limits<std::uint64_t>::max();
}

int expansion_sign(double leading, double residual) noexcept
{
    if (leading == 0.0)
        return residual < 0.0 ? -1 : residual > 0.0 ? 1 : 0;
    if (residual == 0.0 || std::signbit(leading) == std::signbit(residual))
        return leading < 0.0 ? -1 : 1;
    const double a = std::fabs(leading);
    const double b = std::fabs(residual);
    if (a == b)
        return 0;
    return a > b ? (leading < 0.0 ? -1 : 1) : (residual < 0.0 ? -1 : 1);
}

bool round_interval_midpoint(Interval value, std::int64_t origin, std::int64_t& local,
                             std::int64_t& global) noexcept
{
    if (!valid(value) || std::fabs(value.lower) > 1.0e12 || std::fabs(value.upper) > 1.0e12)
        return false;
    const double sum = value.lower + value.upper;
    const double virtual_upper = sum - value.lower;
    const double residual = (value.lower - (sum - virtual_upper)) + (value.upper - virtual_upper);
    const double half_floor = std::floor(sum * 0.5);
    if (!std::isfinite(half_floor) || half_floor < -1.0e12 - 1.0 || half_floor > 1.0e12 + 1.0)
        return false;
    const std::int64_t lower_integer = static_cast<std::int64_t>(half_floor);
    const double boundary = static_cast<double>(lower_integer) * 2.0 + 1.0;
    const int order = expansion_sign(sum - boundary, residual);
    std::int64_t lower_global = 0;
    if (!checked_add_i64(origin, lower_integer, lower_global))
        return false;
    const bool upper = order > 0 || (order == 0 && lower_global >= 0);
    local = lower_integer + (upper ? 1 : 0);
    return checked_add_i64(origin, local, global);
}

enum class Decision : std::uint8_t
{
    safe,
    exceeded,
    uncertain,
};

Decision distance_within(Point left, Point right) noexcept
{
    const Interval distance =
        add(square(subtract(left.x, right.x)), square(subtract(left.y, right.y)));
    if (distance.upper <= kResolutionSquared)
        return Decision::safe;
    if (distance.lower > kResolutionSquared)
        return Decision::exceeded;
    return Decision::uncertain;
}

struct Arc
{
    Point center;
    Interval radius;
    Point start;
    Point end;
    bool counterclockwise = true;
    bool major_arc = false;
};

enum class Domain : std::uint8_t
{
    inside,
    outside,
    uncertain,
};

bool same_exact_point(Point left, Point right) noexcept
{
    return left.x.lower == left.x.upper && left.y.lower == left.y.upper &&
           right.x.lower == right.x.upper && right.y.lower == right.y.upper &&
           left.x.lower == right.x.lower && left.y.lower == right.y.lower;
}

Domain arc_domain(const Arc& arc, Point candidate) noexcept
{
    if (same_exact_point(candidate, arc.start) || same_exact_point(candidate, arc.end))
        return Domain::inside;
    const Point start = subtract(arc.start, arc.center);
    const Point end = subtract(arc.end, arc.center);
    const Point radial = subtract(candidate, arc.center);
    Interval from_start = cross(start, radial);
    Interval to_end = cross(radial, end);
    if (!arc.counterclockwise)
    {
        from_start = negate(from_start);
        to_end = negate(to_end);
    }
    if (!arc.major_arc)
    {
        if (from_start.lower >= 0.0 && to_end.lower >= 0.0)
            return Domain::inside;
        if (from_start.upper < 0.0 || to_end.upper < 0.0)
            return Domain::outside;
    }
    else
    {
        if (from_start.lower > 0.0 || to_end.lower > 0.0)
            return Domain::inside;
        if (from_start.upper <= 0.0 && to_end.upper <= 0.0)
            return Domain::outside;
    }
    return Domain::uncertain;
}

Decision distance_to_arc_within(Point candidate, const Arc& target) noexcept
{
    const Domain radial_domain = arc_domain(target, candidate);
    const Decision start = distance_within(candidate, target.start);
    const Decision end = distance_within(candidate, target.end);
    if (radial_domain != Domain::inside && (start == Decision::safe || end == Decision::safe))
        return Decision::safe;
    if (radial_domain == Domain::outside)
        return start == Decision::exceeded && end == Decision::exceeded ? Decision::exceeded
                                                                        : Decision::uncertain;
    if (radial_domain == Domain::uncertain)
        return Decision::uncertain;

    const Point delta = subtract(candidate, target.center);
    const Interval squared = add(square(delta.x), square(delta.y));
    const double lower_radius = std::max(0.0, target.radius.upper - 50.0);
    const double upper_radius = target.radius.lower + 50.0;
    const Interval lower_squared = square(exact(lower_radius));
    const Interval upper_squared = square(exact(upper_radius));
    if (squared.lower >= lower_squared.upper && squared.upper <= upper_squared.lower)
        return Decision::safe;
    const Interval root = square_root(squared);
    const double lower_gap =
        root.lower > target.radius.upper
            ? root.lower - target.radius.upper
            : (root.upper < target.radius.lower ? target.radius.lower - root.upper : 0.0);
    return lower_gap > 50.0 ? Decision::exceeded : Decision::uncertain;
}

bool append_circle_line_candidates(const Arc& source, Point first, Point second,
                                   std::vector<Point>& output, bool& uncertain) noexcept
{
    const Point direction = subtract(second, first);
    const Interval length_squared = dot(direction, direction);
    if (length_squared.upper == 0.0)
        return true;
    if (length_squared.lower <= 0.0)
    {
        uncertain = true;
        return false;
    }
    const Point offset = subtract(first, source.center);
    const Interval parameter = divide(negate(dot(offset, direction)), length_squared);
    const Point base = add(first, scale(direction, parameter));
    const Interval carrier_distance_squared =
        divide(square(cross(offset, direction)), length_squared);
    const Interval height_squared = subtract(square(source.radius), carrier_distance_squared);
    if (height_squared.upper < 0.0)
        return true;
    if (height_squared.lower < 0.0)
    {
        uncertain = true;
        return false;
    }
    const Interval factor = square_root(divide(height_squared, length_squared));
    if (!valid(factor))
    {
        uncertain = true;
        return false;
    }
    const Point displacement = scale(perpendicular(direction), factor);
    for (const Point candidate : {add(base, displacement), subtract(base, displacement)})
    {
        const Domain domain = arc_domain(source, candidate);
        if (domain == Domain::inside)
            output.push_back(candidate);
        else if (domain == Domain::uncertain)
        {
            uncertain = true;
            return false;
        }
    }
    return true;
}

Decision directed_arc_within(const Arc& source, const Arc& target,
                             std::uint64_t& candidate_count) noexcept
{
    std::vector<Point> candidates;
    candidates.reserve(16);
    candidates.push_back(source.start);
    candidates.push_back(source.end);
    const std::pair<Point, Point> lines[]{{source.center, target.center},
                                          {source.center, target.start},
                                          {source.center, target.end},
                                          {target.center, target.start},
                                          {target.center, target.end}};
    bool uncertain = false;
    for (const auto& line : lines)
        if (!append_circle_line_candidates(source, line.first, line.second, candidates, uncertain))
            return Decision::uncertain;
    const Point chord = subtract(target.end, target.start);
    const Point midpoint = scale(add(target.start, target.end), exact(0.5));
    if (!append_circle_line_candidates(source, midpoint, add(midpoint, perpendicular(chord)),
                                       candidates, uncertain))
        return Decision::uncertain;
    candidate_count += candidates.size();
    bool saw_uncertain = false;
    for (const Point candidate : candidates)
    {
        const Decision value = distance_to_arc_within(candidate, target);
        if (value == Decision::exceeded)
            return value;
        saw_uncertain = saw_uncertain || value == Decision::uncertain;
    }
    return saw_uncertain ? Decision::uncertain : Decision::safe;
}

Decision arcs_within(const Arc& left, const Arc& right, std::uint64_t& candidate_count) noexcept
{
    const Decision forward = directed_arc_within(left, right, candidate_count);
    if (forward != Decision::safe)
        return forward;
    return directed_arc_within(right, left, candidate_count);
}

std::int64_t floor_div_51(std::int64_t value) noexcept
{
    std::int64_t quotient = value / 51;
    if (value < 0 && value % 51 != 0)
        --quotient;
    return quotient;
}

struct CellVertex
{
    std::int64_t cell_x = 0;
    std::int64_t cell_y = 0;
    std::uint32_t vertex = 0;
};

class Builder
{
  public:
    Builder(const AnalyticRequestPacketRecords& records, std::uint32_t job_index,
            const AnalyticFilteredGeometry& geometry, const std::vector<AnalyticCurvePair>& pairs,
            const AnalyticSolverLimits& limits)
        : records_(records), job_index_(job_index), geometry_(geometry), pairs_(pairs),
          limits_(limits)
    {
    }

    AnalyticFilteredNormalizationResult build()
    {
        if (!analytic_solver_limits_within_hard_ceilings(limits_))
        {
            result_.error = AnalyticFilteredNormalizationError::resource_limit_exceeded;
            return failure();
        }
        if (!reserve_before_outcomes())
            return failure();
        AnalyticSolverLimits execution_limits = limits_;
        execution_limits.predicate_calls -= reserved_work_;
        execution_limits.working_memory_bytes -= reserved_memory_;
        result_.outcomes = build_analytic_filtered_outcomes(records_, job_index_, geometry_, pairs_,
                                                            execution_limits);
        const auto& upstream = result_.outcomes.telemetry;
        result_.telemetry.outcomes_work_units = upstream.predicate_calls;
        result_.telemetry.outcomes_peak_working_memory_bytes = upstream.peak_working_memory_bytes;
        result_.telemetry.predicate_calls = upstream.predicate_calls;
        result_.telemetry.peak_working_memory_bytes = upstream.peak_working_memory_bytes;
        result_.telemetry.algebraic_fallback_calls = upstream.algebraic_fallback_calls;
        result_.telemetry.reserved_normalization_work_units = reserved_work_;
        result_.telemetry.reserved_normalization_memory_bytes = reserved_memory_;
        if (result_.outcomes.error != AnalyticFilteredOutcomesError::none)
        {
            result_.error =
                result_.outcomes.error == AnalyticFilteredOutcomesError::invalid_argument
                    ? AnalyticFilteredNormalizationError::invalid_argument
                    : AnalyticFilteredNormalizationError::resource_limit_exceeded;
            return std::move(result_);
        }
        try
        {
            if (!preflight() || !normalize_vertices() || !normalize_fragments() ||
                !publish_topology() || !validate_vertex_separation() || !strict_replay())
                return failure();
        }
        catch (const std::bad_alloc&)
        {
            result_.error = AnalyticFilteredNormalizationError::resource_limit_exceeded;
            return failure();
        }
        result_.telemetry.normalized_vertices = result_.vertices.size();
        result_.telemetry.normalized_fragments = result_.fragments.size();
        result_.telemetry.normalized_rings = result_.rings.size();
        result_.telemetry.normalized_regions = result_.regions.size();
        result_.telemetry.normalization_work_units = work_;
        result_.telemetry.predicate_calls = result_.outcomes.telemetry.predicate_calls + work_;
        return std::move(result_);
    }

  private:
    bool reserve_before_outcomes()
    {
        std::uint64_t fragments = 0;
        std::uint64_t pair_term = 0;
        if (!checked_multiply_u64(geometry_.curves.size(), 4, fragments) ||
            !checked_multiply_u64(pairs_.size(), 4, pair_term) ||
            !checked_add_u64(fragments, pair_term, fragments))
        {
            result_.error = AnalyticFilteredNormalizationError::resource_limit_exceeded;
            return false;
        }
        if (!checked_multiply_u64(fragments, kNormalizationPossibleFragmentBytes,
                                  reserved_memory_) ||
            !checked_add_u64(reserved_memory_, kNormalizationFixedPhaseBytes, reserved_memory_))
        {
            result_.error = AnalyticFilteredNormalizationError::resource_limit_exceeded;
            return false;
        }
        std::uint64_t depth = 1;
        for (std::uint64_t value = fragments > 1 ? fragments - 1 : 0; value != 0; value >>= 1U)
            ++depth;
        std::uint64_t level_work = 0;
        std::uint64_t work_per_fragment = 0;
        if (!checked_multiply_u64(depth, kNormalizationPossibleFragmentLevelWork, level_work) ||
            !checked_add_u64(kNormalizationPossibleFragmentBaseWork, level_work,
                             work_per_fragment) ||
            !checked_multiply_u64(fragments, work_per_fragment, reserved_work_) ||
            !checked_add_u64(reserved_work_, kNormalizationFixedPhaseBytes, reserved_work_))
        {
            result_.error = AnalyticFilteredNormalizationError::resource_limit_exceeded;
            return false;
        }
        result_.telemetry.reserved_normalization_work_units = reserved_work_;
        result_.telemetry.reserved_normalization_memory_bytes = reserved_memory_;
        if (reserved_work_ > limits_.predicate_calls ||
            reserved_memory_ > limits_.working_memory_bytes)
        {
            result_.error = AnalyticFilteredNormalizationError::resource_limit_exceeded;
            return false;
        }
        return true;
    }

    bool charge(std::uint64_t units)
    {
        const std::uint64_t upstream = result_.outcomes.telemetry.predicate_calls;
        if (upstream > limits_.predicate_calls || work_ > limits_.predicate_calls - upstream ||
            units > limits_.predicate_calls - upstream - work_)
        {
            result_.error = AnalyticFilteredNormalizationError::resource_limit_exceeded;
            return false;
        }
        work_ += units;
        return true;
    }

    AnalyticFilteredNormalizationResult failure()
    {
        if (result_.error == AnalyticFilteredNormalizationError::none)
            result_.error = AnalyticFilteredNormalizationError::invalid_argument;
        const auto upstream_work = result_.outcomes.telemetry.predicate_calls;
        const auto upstream_memory = result_.outcomes.telemetry.peak_working_memory_bytes;
        const auto peak_memory = result_.telemetry.peak_working_memory_bytes;
        const auto fallback = result_.outcomes.telemetry.algebraic_fallback_calls;
        const auto arc_candidates = result_.telemetry.arc_critical_candidates;
        const auto replay_pairs = result_.telemetry.strict_replay_candidate_pairs;
        result_.outcomes = {};
        result_.vertices.clear();
        result_.fragments.clear();
        result_.ring_fragments.clear();
        result_.rings.clear();
        result_.regions.clear();
        result_.old_vertex_to_normalized.clear();
        result_.old_boundary_to_normalized.clear();
        result_.old_ring_to_normalized.clear();
        result_.old_region_to_normalized.clear();
        result_.telemetry = {};
        result_.telemetry.outcomes_work_units = upstream_work;
        result_.telemetry.outcomes_peak_working_memory_bytes = upstream_memory;
        result_.telemetry.normalization_work_units = work_;
        result_.telemetry.arc_critical_candidates = arc_candidates;
        result_.telemetry.strict_replay_candidate_pairs = replay_pairs;
        result_.telemetry.reserved_normalization_work_units = reserved_work_;
        result_.telemetry.reserved_normalization_memory_bytes = reserved_memory_;
        result_.telemetry.predicate_calls = upstream_work + work_;
        result_.telemetry.peak_working_memory_bytes = std::max(upstream_memory, peak_memory);
        result_.telemetry.algebraic_fallback_calls = fallback;
        return std::move(result_);
    }

    bool preflight()
    {
        const auto& lineage = result_.outcomes.lineage;
        const auto& regions = lineage.regions;
        const auto& arrangement = regions.selection.arrangement;
        if (lineage.boundaries.size() != regions.ring_half_edges.size() ||
            lineage.region_lineage.size() != regions.regions.size())
            return false;
        bool valid_count = true;
        std::uint64_t bytes = 0;
        std::uint64_t term = 0;
        const std::uint64_t vertices = arrangement.vertices.size();
        const std::uint64_t boundaries = lineage.boundaries.size();
        const std::uint64_t rings = regions.rings.size();
        const std::uint64_t region_count = regions.regions.size();
        valid_count = checked_multiply_u64(vertices, kNormalizationVertexPhaseBytes, term) &&
                      checked_add_u64(bytes, term, bytes);
        valid_count = valid_count &&
                      checked_multiply_u64(boundaries, kNormalizationBoundaryPhaseBytes, term) &&
                      checked_add_u64(bytes, term, bytes);
        valid_count = valid_count &&
                      checked_multiply_u64(rings, kNormalizationRingPhaseBytes, term) &&
                      checked_add_u64(bytes, term, bytes);
        valid_count = valid_count &&
                      checked_multiply_u64(region_count, kNormalizationRegionPhaseBytes, term) &&
                      checked_add_u64(bytes, term, bytes) &&
                      checked_add_u64(bytes, kNormalizationFixedPhaseBytes, bytes);
        if (!valid_count || bytes > limits_.working_memory_bytes)
        {
            result_.error = AnalyticFilteredNormalizationError::resource_limit_exceeded;
            return false;
        }
        if (!charge(vertices * 3 + boundaries * 12 + rings * 3 + region_count * 2))
            return false;
        if (bytes > reserved_memory_ ||
            !checked_add_u64(result_.outcomes.telemetry.peak_working_memory_bytes, bytes,
                             base_phase_bytes_) ||
            base_phase_bytes_ > limits_.working_memory_bytes)
        {
            result_.error = AnalyticFilteredNormalizationError::resource_limit_exceeded;
            return false;
        }
        result_.telemetry.peak_working_memory_bytes =
            std::max(result_.outcomes.telemetry.peak_working_memory_bytes, base_phase_bytes_);
        result_.old_vertex_to_normalized.assign(arrangement.vertices.size(),
                                                kNoAnalyticNormalizedIndex);
        result_.old_boundary_to_normalized.assign(lineage.boundaries.size(),
                                                  kNoAnalyticNormalizedIndex);
        result_.old_ring_to_normalized.assign(regions.rings.size(), kNoAnalyticNormalizedIndex);
        result_.old_region_to_normalized.assign(regions.regions.size(), kNoAnalyticNormalizedIndex);
        local_vertices_.reserve(arrangement.vertices.size());
        result_.vertices.reserve(arrangement.vertices.size());
        result_.fragments.reserve(lineage.boundaries.size());
        replay_curves_.reserve(lineage.boundaries.size());
        replay_bounds_.reserve(lineage.boundaries.size());
        result_.ring_fragments.reserve(regions.ring_half_edges.size());
        result_.rings.reserve(regions.rings.size());
        result_.regions.reserve(regions.regions.size());
        return true;
    }

    bool mark_used_vertices(std::vector<std::uint8_t>& used) const
    {
        const auto& lineage = result_.outcomes.lineage;
        const auto& arrangement = lineage.regions.selection.arrangement;
        for (const auto& boundary : lineage.boundaries)
        {
            if (boundary.half_edge >= arrangement.half_edges.size())
                return false;
            const auto& half = arrangement.half_edges[boundary.half_edge];
            if (half.twin >= arrangement.half_edges.size())
                return false;
            used[half.origin_vertex] = 1;
            used[arrangement.half_edges[half.twin].origin_vertex] = 1;
        }
        return true;
    }

    bool resolve_column_x(const std::vector<std::uint8_t>& used,
                          std::vector<std::int64_t>& grouped_x,
                          std::vector<std::uint8_t>& has_grouped_x)
    {
        const auto& regions = result_.outcomes.lineage.regions;
        const auto& vertices = regions.selection.arrangement.vertices;
        std::vector<std::pair<std::uint64_t, std::uint32_t>> columns;
        columns.reserve(vertices.size());
        for (std::uint32_t vertex = 0; vertex < vertices.size(); ++vertex)
            if (used[vertex] && vertices[vertex].point.construction_x_column_id != 0)
                columns.push_back({vertices[vertex].point.construction_x_column_id, vertex});
        if (!charge(sort_units(columns.size())))
            return false;
        std::sort(columns.begin(), columns.end());
        for (std::size_t begin = 0; begin < columns.size();)
        {
            std::size_t end = begin + 1;
            Interval intersection{vertices[columns[begin].second].point.x.lower,
                                  vertices[columns[begin].second].point.x.upper};
            while (end < columns.size() && columns[end].first == columns[begin].first)
            {
                const auto& x = vertices[columns[end].second].point.x;
                intersection.lower = std::max(intersection.lower, x.lower);
                intersection.upper = std::min(intersection.upper, x.upper);
                ++end;
            }
            std::int64_t local = 0;
            std::int64_t global = 0;
            if (intersection.lower > intersection.upper ||
                !round_interval_midpoint(intersection, regions.selection.origin_x_nm, local,
                                         global))
                return false;
            for (std::size_t at = begin; at < end; ++at)
            {
                grouped_x[columns[at].second] = local;
                has_grouped_x[columns[at].second] = 1;
            }
            begin = end;
        }
        return true;
    }

    bool publish_vertex(std::uint32_t vertex, bool has_grouped_x, std::int64_t grouped_x)
    {
        const auto& selection = result_.outcomes.lineage.regions.selection;
        const auto& source = selection.arrangement.vertices[vertex].point;
        std::int64_t local_x = 0;
        std::int64_t global_x = 0;
        if (has_grouped_x)
        {
            local_x = grouped_x;
            if (!checked_add_i64(selection.origin_x_nm, local_x, global_x))
                return false;
        }
        else if (!round_interval_midpoint({source.x.lower, source.x.upper}, selection.origin_x_nm,
                                          local_x, global_x))
            return false;
        std::int64_t local_y = 0;
        std::int64_t global_y = 0;
        if (!round_interval_midpoint({source.y.lower, source.y.upper}, selection.origin_y_nm,
                                     local_y, global_y))
            return false;
        const Decision displacement = distance_within(point(source), exact_point(local_x, local_y));
        if (displacement != Decision::safe)
        {
            result_.error = displacement == Decision::exceeded
                                ? AnalyticFilteredNormalizationError::normalization_error_exceeded
                                : AnalyticFilteredNormalizationError::resource_limit_exceeded;
            return false;
        }
        const std::uint32_t normalized = static_cast<std::uint32_t>(result_.vertices.size());
        result_.old_vertex_to_normalized[vertex] = normalized;
        result_.vertices.push_back({global_x, global_y, vertex});
        local_vertices_.push_back({local_x, local_y});
        return true;
    }

    bool normalized_vertices_are_unique()
    {
        std::vector<std::tuple<std::int64_t, std::int64_t, std::uint32_t>> unique;
        unique.reserve(result_.vertices.size());
        for (std::uint32_t index = 0; index < result_.vertices.size(); ++index)
            unique.emplace_back(result_.vertices[index].x_nm, result_.vertices[index].y_nm, index);
        if (!charge(sort_units(unique.size())))
            return false;
        std::sort(unique.begin(), unique.end());
        const bool duplicate =
            std::adjacent_find(unique.begin(), unique.end(),
                               [](const auto& left, const auto& right)
                               {
                                   return std::get<0>(left) == std::get<0>(right) &&
                                          std::get<1>(left) == std::get<1>(right);
                               }) != unique.end();
        if (duplicate)
            result_.error = AnalyticFilteredNormalizationError::normalization_topology_collapse;
        return !duplicate;
    }

    bool normalize_vertices()
    {
        const auto& vertices = result_.outcomes.lineage.regions.selection.arrangement.vertices;
        std::vector<std::uint8_t> used(vertices.size());
        if (!mark_used_vertices(used))
            return false;
        std::vector<std::int64_t> grouped_x(vertices.size());
        std::vector<std::uint8_t> has_grouped_x(vertices.size());
        if (!resolve_column_x(used, grouped_x, has_grouped_x))
            return false;
        for (std::uint32_t vertex = 0; vertex < vertices.size(); ++vertex)
            if (used[vertex] &&
                !publish_vertex(vertex, has_grouped_x[vertex] != 0, grouped_x[vertex]))
                return false;
        return normalized_vertices_are_unique();
    }

    bool certify_line_fragment(const AnalyticArrangementEdgeNm& edge,
                               const AnalyticArrangementHalfEdge& half,
                               const AnalyticIntegerPointNm& local_start,
                               const AnalyticIntegerPointNm& local_end)
    {
        const Point source_start =
            half.forward ? point(edge.carrier_start) : point(edge.carrier_end);
        const Point source_end = half.forward ? point(edge.carrier_end) : point(edge.carrier_start);
        const Decision first =
            distance_within(source_start, exact_point(local_start.x, local_start.y));
        const Decision second = distance_within(source_end, exact_point(local_end.x, local_end.y));
        if (first == Decision::safe && second == Decision::safe)
            return true;
        result_.error = first == Decision::exceeded || second == Decision::exceeded
                            ? AnalyticFilteredNormalizationError::normalization_error_exceeded
                            : AnalyticFilteredNormalizationError::resource_limit_exceeded;
        return false;
    }

    bool certify_arc_fragment(const AnalyticArrangementEdgeNm& edge,
                              const AnalyticArrangementHalfEdge& half,
                              const AnalyticIntegerPointNm& local_start,
                              const AnalyticIntegerPointNm& local_end,
                              AnalyticNormalizedFragmentNm& fragment, AnalyticAtomicCurveNm& replay)
    {
        const std::uint64_t candidates_before = result_.telemetry.arc_critical_candidates;
        std::int64_t radius_local = 0;
        std::int64_t radius_global = 0;
        if (!round_interval_midpoint({edge.circle.radius.lower, edge.circle.radius.upper}, 0,
                                     radius_local, radius_global) ||
            radius_local <= 0)
            return false;
        const double maximum_radius_gap =
            std::max(std::fabs(static_cast<double>(radius_local) - edge.circle.radius.lower),
                     std::fabs(static_cast<double>(radius_local) - edge.circle.radius.upper));
        if (maximum_radius_gap > 50.0)
        {
            result_.error = AnalyticFilteredNormalizationError::normalization_error_exceeded;
            return false;
        }
        Point replay_center;
        if (!analytic_detail::reconstruct_endpoint_authoritative_arc_center(
                local_start.x, local_start.y, local_end.x, local_end.y,
                static_cast<std::uint64_t>(radius_local), fragment.counterclockwise,
                fragment.major_arc, replay_center))
        {
            result_.error = AnalyticFilteredNormalizationError::normalization_topology_collapse;
            return false;
        }
        replay.circle.center = filtered(replay_center);
        replay.circle.radius = {static_cast<double>(radius_local),
                                static_cast<double>(radius_local)};
        replay.has_integer_certificate = false;
        replay.has_integer_radius_certificate = true;
        replay.integer_radius = static_cast<std::uint64_t>(radius_local);
        replay.has_endpoint_authoritative_arc_certificate = true;
        replay.endpoint_authoritative_upper_branch =
            edge.x_monotone_branch == AnalyticXMonotoneBranch::upper;
        replay.has_endpoint_authoritative_x_monotone_certificate =
            analytic_detail::endpoint_authoritative_arc_is_x_monotone(
                local_start.x, local_start.y, local_end.x, local_end.y,
                static_cast<std::uint64_t>(radius_local), fragment.counterclockwise,
                fragment.major_arc, replay_center, replay.endpoint_authoritative_upper_branch);
        replay.has_arc_sweep_certificate = true;
        fragment.radius_nm = static_cast<std::uint64_t>(radius_local);
        const Arc source{point(edge.circle.center),
                         {edge.circle.radius.lower, edge.circle.radius.upper},
                         half.forward ? point(edge.carrier_start) : point(edge.carrier_end),
                         half.forward ? point(edge.carrier_end) : point(edge.carrier_start),
                         fragment.counterclockwise,
                         fragment.major_arc};
        const Arc target{replay_center,
                         {replay.circle.radius.lower, replay.circle.radius.upper},
                         point(replay.start),
                         point(replay.end),
                         fragment.counterclockwise,
                         fragment.major_arc};
        const Decision certification =
            arcs_within(source, target, result_.telemetry.arc_critical_candidates);
        const std::uint64_t candidate_work =
            result_.telemetry.arc_critical_candidates - candidates_before;
        if (!charge(candidate_work * 4 + 16))
            return false;
        if (certification == Decision::safe)
            return true;
        result_.error = certification == Decision::exceeded
                            ? AnalyticFilteredNormalizationError::normalization_error_exceeded
                            : AnalyticFilteredNormalizationError::resource_limit_exceeded;
        return false;
    }

    bool normalize_fragment(std::uint32_t boundary)
    {
        const auto& lineage = result_.outcomes.lineage;
        const auto& arrangement = lineage.regions.selection.arrangement;
        const auto& record = lineage.boundaries[boundary];
        if (record.half_edge >= arrangement.half_edges.size())
            return false;
        const auto& half = arrangement.half_edges[record.half_edge];
        if (half.twin >= arrangement.half_edges.size() || half.edge >= arrangement.edges.size())
            return false;
        const auto& twin = arrangement.half_edges[half.twin];
        const auto& edge = arrangement.edges[half.edge];
        if (half.origin_vertex >= result_.old_vertex_to_normalized.size() ||
            twin.origin_vertex >= result_.old_vertex_to_normalized.size())
            return false;
        const std::uint32_t start = result_.old_vertex_to_normalized[half.origin_vertex];
        const std::uint32_t end = result_.old_vertex_to_normalized[twin.origin_vertex];
        if (start == kNoAnalyticNormalizedIndex || end == kNoAnalyticNormalizedIndex ||
            start == end)
        {
            result_.error = AnalyticFilteredNormalizationError::normalization_topology_collapse;
            return false;
        }
        AnalyticNormalizedFragmentNm fragment;
        fragment.start_vertex = start;
        fragment.end_vertex = end;
        fragment.kind = edge.kind;
        fragment.counterclockwise = half.forward ? edge.counterclockwise : !edge.counterclockwise;
        fragment.major_arc = edge.major_arc;
        fragment.old_boundary = boundary;
        AnalyticAtomicCurveNm replay;
        replay.curve_index = boundary + 1;
        replay.kind = edge.kind;
        const auto& local_start = local_vertices_[start];
        const auto& local_end = local_vertices_[end];
        replay.start = filtered(exact_point(local_start.x, local_start.y));
        replay.end = filtered(exact_point(local_end.x, local_end.y));
        replay.counterclockwise = fragment.counterclockwise;
        replay.major_arc = fragment.major_arc;
        replay.has_integer_certificate = true;
        replay.integer_start = local_start;
        replay.integer_end = local_end;
        const bool certified =
            edge.kind == AnalyticAtomicCurveKind::line
                ? certify_line_fragment(edge, half, local_start, local_end)
                : certify_arc_fragment(edge, half, local_start, local_end, fragment, replay);
        if (!certified)
            return false;
        result_.old_boundary_to_normalized[boundary] =
            static_cast<std::uint32_t>(result_.fragments.size());
        result_.fragments.push_back(fragment);
        replay_curves_.push_back(replay);
        replay_bounds_.push_back(bounds(replay));
        return true;
    }

    bool normalize_fragments()
    {
        const auto count = result_.outcomes.lineage.boundaries.size();
        for (std::uint32_t boundary = 0; boundary < count; ++boundary)
            if (!normalize_fragment(boundary))
                return false;
        return true;
    }

    AnalyticCurveBoundsNm bounds(const AnalyticAtomicCurveNm& curve) const noexcept
    {
        AnalyticCurveBoundsNm output;
        output.curve_index = curve.curve_index;
        output.min_x = std::min(curve.start.x.lower, curve.end.x.lower);
        output.min_y = std::min(curve.start.y.lower, curve.end.y.lower);
        output.max_x = std::max(curve.start.x.upper, curve.end.x.upper);
        output.max_y = std::max(curve.start.y.upper, curve.end.y.upper);
        if (curve.kind == AnalyticAtomicCurveKind::circular_arc)
        {
            const Arc arc{
                point(curve.circle.center), {curve.circle.radius.lower, curve.circle.radius.upper},
                point(curve.start),         point(curve.end),
                curve.counterclockwise,     curve.major_arc};
            const Point cardinals[]{{add(arc.center.x, arc.radius), arc.center.y},
                                    {subtract(arc.center.x, arc.radius), arc.center.y},
                                    {arc.center.x, add(arc.center.y, arc.radius)},
                                    {arc.center.x, subtract(arc.center.y, arc.radius)}};
            for (const Point cardinal : cardinals)
                if (arc_domain(arc, cardinal) != Domain::outside)
                {
                    output.min_x = std::min(output.min_x, cardinal.x.lower);
                    output.max_x = std::max(output.max_x, cardinal.x.upper);
                    output.min_y = std::min(output.min_y, cardinal.y.lower);
                    output.max_y = std::max(output.max_y, cardinal.y.upper);
                }
        }
        return output;
    }

    bool publish_topology()
    {
        const auto& regions = result_.outcomes.lineage.regions;
        for (std::uint32_t old_ring = 0; old_ring < regions.rings.size(); ++old_ring)
        {
            const auto& source = regions.rings[old_ring];
            if (source.half_edge_begin > regions.ring_half_edges.size() ||
                source.half_edge_count > regions.ring_half_edges.size() - source.half_edge_begin)
                return false;
            AnalyticNormalizedRing ring;
            ring.fragment_begin = static_cast<std::uint32_t>(result_.ring_fragments.size());
            ring.fragment_count = source.half_edge_count;
            ring.parent_ring = source.parent_ring;
            ring.depth = source.depth;
            ring.counterclockwise = source.counterclockwise;
            ring.old_ring = old_ring;
            for (std::uint32_t offset = 0; offset < source.half_edge_count; ++offset)
            {
                const std::uint32_t boundary = source.half_edge_begin + offset;
                if (boundary >= result_.old_boundary_to_normalized.size())
                    return false;
                result_.ring_fragments.push_back(result_.old_boundary_to_normalized[boundary]);
            }
            result_.old_ring_to_normalized[old_ring] =
                static_cast<std::uint32_t>(result_.rings.size());
            result_.rings.push_back(ring);
        }
        for (std::uint32_t old_region = 0; old_region < regions.regions.size(); ++old_region)
        {
            const auto& source = regions.regions[old_region];
            if (source.outer_ring >= result_.old_ring_to_normalized.size())
                return false;
            result_.old_region_to_normalized[old_region] =
                static_cast<std::uint32_t>(result_.regions.size());
            result_.regions.push_back(
                {result_.old_ring_to_normalized[source.outer_ring], old_region});
        }
        return true;
    }

    bool validate_vertex_separation()
    {
        std::vector<CellVertex> cells;
        cells.reserve(local_vertices_.size());
        for (std::uint32_t vertex = 0; vertex < local_vertices_.size(); ++vertex)
            cells.push_back({floor_div_51(local_vertices_[vertex].x),
                             floor_div_51(local_vertices_[vertex].y), vertex});
        if (!charge(sort_units(cells.size())))
            return false;
        std::sort(cells.begin(), cells.end(),
                  [](const CellVertex& left, const CellVertex& right)
                  {
                      return std::tie(left.cell_x, left.cell_y, left.vertex) <
                             std::tie(right.cell_x, right.cell_y, right.vertex);
                  });
        std::uint64_t search_levels = 1;
        for (std::uint64_t value = cells.size() > 1 ? cells.size() - 1 : 0; value != 0;
             value >>= 1U)
            ++search_levels;
        std::uint64_t search_work = 0;
        if (!checked_multiply_u64(cells.size(), 9, search_work) ||
            !checked_multiply_u64(search_work, search_levels, search_work) || !charge(search_work))
            return false;
        const auto cell_less =
            [](const CellVertex& value, const std::pair<std::int64_t, std::int64_t>& key)
        { return std::tie(value.cell_x, value.cell_y) < std::tie(key.first, key.second); };
        for (const CellVertex& current : cells)
            for (std::int64_t dx = -1; dx <= 1; ++dx)
                for (std::int64_t dy = -1; dy <= 1; ++dy)
                {
                    const std::pair<std::int64_t, std::int64_t> key{current.cell_x + dx,
                                                                    current.cell_y + dy};
                    auto at = std::lower_bound(cells.begin(), cells.end(), key, cell_less);
                    while (at != cells.end() && at->cell_x == key.first && at->cell_y == key.second)
                    {
                        if (at->vertex < current.vertex)
                        {
                            if (!charge(1))
                                return false;
                            const auto& left = local_vertices_[at->vertex];
                            const auto& right = local_vertices_[current.vertex];
                            const WideInteger dx2 =
                                analytic_detail::wide_multiply(left.x - right.x, left.x - right.x);
                            const WideInteger dy2 =
                                analytic_detail::wide_multiply(left.y - right.y, left.y - right.y);
                            const WideInteger d2 = analytic_detail::wide_add(dx2, dy2);
                            if (analytic_detail::wide_compare(
                                    d2, analytic_detail::wide_multiply(50, 50)) <= 0)
                            {
                                result_.error = AnalyticFilteredNormalizationError::
                                    normalization_topology_collapse;
                                return false;
                            }
                        }
                        ++at;
                    }
                }
        return true;
    }

    bool strict_replay()
    {
        const std::uint64_t upstream = result_.outcomes.telemetry.predicate_calls;
        if (upstream > limits_.predicate_calls || work_ > limits_.predicate_calls - upstream ||
            base_phase_bytes_ > limits_.working_memory_bytes)
            return false;
        AnalyticSolverLimits replay_limits = limits_;
        replay_limits.predicate_calls = limits_.predicate_calls - upstream - work_;
        replay_limits.working_memory_bytes = limits_.working_memory_bytes - base_phase_bytes_;
        const auto& regions = result_.outcomes.lineage.regions;
        const auto replay = analytic_normalization_detail::validate_normalized_replay(
            regions.selection.origin_x_nm, regions.selection.origin_y_nm, replay_curves_,
            replay_bounds_, regions, replay_limits);
        result_.telemetry.strict_replay_candidate_pairs = replay.candidate_pairs;
        if (!charge(replay.work_units))
            return false;
        std::uint64_t peak = 0;
        if (!checked_add_u64(base_phase_bytes_, replay.peak_working_memory_bytes, peak) ||
            peak > limits_.working_memory_bytes)
        {
            result_.error = AnalyticFilteredNormalizationError::resource_limit_exceeded;
            return false;
        }
        result_.telemetry.peak_working_memory_bytes =
            std::max(result_.telemetry.peak_working_memory_bytes, peak);
        if (replay.error == analytic_normalization_detail::ReplayError::none)
            return true;
        result_.error =
            replay.error == analytic_normalization_detail::ReplayError::resource_limit_exceeded
                ? AnalyticFilteredNormalizationError::resource_limit_exceeded
                : (replay.error == analytic_normalization_detail::ReplayError::invalid_argument
                       ? AnalyticFilteredNormalizationError::invalid_argument
                       : AnalyticFilteredNormalizationError::normalization_topology_collapse);
        return false;
    }

    const AnalyticRequestPacketRecords& records_;
    std::uint32_t job_index_ = 0;
    const AnalyticFilteredGeometry& geometry_;
    const std::vector<AnalyticCurvePair>& pairs_;
    const AnalyticSolverLimits& limits_;
    AnalyticFilteredNormalizationResult result_;
    std::vector<AnalyticIntegerPointNm> local_vertices_;
    std::vector<AnalyticAtomicCurveNm> replay_curves_;
    std::vector<AnalyticCurveBoundsNm> replay_bounds_;
    std::uint64_t work_ = 0;
    std::uint64_t reserved_work_ = 0;
    std::uint64_t reserved_memory_ = 0;
    std::uint64_t base_phase_bytes_ = 0;
};

static_assert(sizeof(AnalyticNormalizedVertexNm) <= kVertexLogicalBytes);
static_assert(sizeof(AnalyticNormalizedFragmentNm) <= kFragmentLogicalBytes);
static_assert(sizeof(AnalyticNormalizedRing) <= kRingLogicalBytes);
static_assert(sizeof(AnalyticNormalizedRegion) <= kRegionLogicalBytes);
static_assert(sizeof(AnalyticAtomicCurveNm) <= kReplayCurveLogicalBytes);
} // namespace

AnalyticFilteredNormalizationResult build_analytic_filtered_normalization(
    const AnalyticRequestPacketRecords& records, std::uint32_t job_index,
    const AnalyticFilteredGeometry& geometry, const std::vector<AnalyticCurvePair>& candidate_pairs,
    const AnalyticSolverLimits& limits)
{
    return Builder(records, job_index, geometry, candidate_pairs, limits).build();
}

} // namespace geometer

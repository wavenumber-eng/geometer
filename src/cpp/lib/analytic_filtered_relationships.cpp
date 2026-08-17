#include "analytic_filtered_relationships.h"

#include "analytic_endpoint_arc_reconstruction.h"
#include "analytic_filtered_execution_policy.h"
#include "analytic_filtered_interval.h"
#include "analytic_wide_integer.h"
#include "geometer/analytic_curve_broad_phase.h"
#include "geometer/analytic_curve_narrow_phase.h"
#include "geometer/analytic_filtered_boolean_selection.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <numeric>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

namespace geometer::analytic_relationship_detail
{
namespace
{

constexpr std::uint64_t kRelationshipRowBytes = 32;
constexpr std::uint64_t kIndexBytes = 4;
constexpr std::uint64_t kCandidatePairBytes = 8;
constexpr std::uint64_t kQueryPlanBytes = 16;
constexpr std::uint64_t kQueryKeyBytes = 8;
constexpr std::uint64_t kCacheEntryBytes = 40;
constexpr std::uint64_t kRelationEventBytes = 16;
constexpr std::uint64_t kGeometryCurveBytes =
    kAnalyticAtomicCurveLogicalBytes + 40 + 64 + kIndexBytes;
constexpr std::uint64_t kGeometryOperandBytes = 16;
constexpr std::uint64_t kGeometryFixedBytes = 40;
constexpr std::uint64_t kLineCarrierScratchBytes = 40;
constexpr std::uint64_t kArcCarrierScratchBytes = 80;

bool checked_add(std::uint64_t left, std::uint64_t right, std::uint64_t& output) noexcept
{
    if (left > std::numeric_limits<std::uint64_t>::max() - right)
        return false;
    output = left + right;
    return true;
}

bool checked_multiply(std::uint64_t left, std::uint64_t right, std::uint64_t& output) noexcept
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left)
        return false;
    output = left * right;
    return true;
}

std::uint64_t ceil_log2(std::uint64_t count) noexcept
{
    std::uint64_t result = 0;
    for (std::uint64_t value = count > 1 ? count - 1 : 0; value != 0; value >>= 1U)
        ++result;
    return result;
}

std::uint64_t sort_units(std::uint64_t count) noexcept
{
    std::uint64_t output = 0;
    return checked_multiply(count, ceil_log2(count) + 1, output)
               ? output
               : std::numeric_limits<std::uint64_t>::max();
}

struct PairValue
{
    std::uint8_t dimension = 0;
    bool equality = false;
    bool left_contains_right = false;
    bool right_contains_left = false;
};

using RegionPair = std::pair<std::uint32_t, std::uint32_t>;

struct RelationEvent
{
    RegionPair pair;
    std::uint8_t dimension = 0;
    bool equality = false;
    bool left_contains_right = false;
    bool right_contains_left = false;
};

static_assert(sizeof(RelationEvent) <= kRelationEventBytes);

struct CachedPair
{
    std::vector<AnalyticRelationshipPairRecord> pairs;
    std::uint8_t aggregate = 0;
};

struct QueryPlan
{
    std::uint32_t left_job = 0;
    std::uint32_t right_job = 0;
    std::uint32_t cache_index = std::numeric_limits<std::uint32_t>::max();
    bool failed = false;
};

struct CacheEntry
{
    std::uint32_t first_job = 0;
    std::uint32_t second_job = 0;
    CachedPair value;
};

static_assert(sizeof(QueryPlan) <= kQueryPlanBytes);
static_assert(sizeof(CacheEntry) <= kCacheEntryBytes);

struct ArcCarrierKey
{
    bool exact_center = false;
    double center_x = 0.0;
    double center_y = 0.0;
    std::int64_t first_x = 0;
    std::int64_t first_y = 0;
    std::int64_t second_x = 0;
    std::int64_t second_y = 0;
    std::uint64_t radius = 0;
    bool center_on_positive_side = false;

    auto key() const noexcept
    {
        return std::tie(exact_center, center_x, center_y, first_x, first_y, second_x, second_y,
                        radius, center_on_positive_side);
    }

    bool operator<(const ArcCarrierKey& other) const noexcept
    {
        return key() < other.key();
    }
};

struct PairBuild
{
    EvaluationError error = EvaluationError::none;
    CachedPair value;
    std::uint64_t work = 0;
    std::uint64_t candidates = 0;
    std::uint64_t peak_memory = 0;
    std::uint64_t required_memory = 0;
    std::uint64_t algebraic_fallback_calls = 0;
    bool unresolved_predicate_failure = false;
};

AnalyticFilteredPointNm exact_point(std::int64_t x, std::int64_t y) noexcept
{
    const double dx = static_cast<double>(x);
    const double dy = static_cast<double>(y);
    return {{dx, dx}, {dy, dy}, 0};
}

struct GeometryBuild
{
    EvaluationError error = EvaluationError::none;
    AnalyticRequestPacketRecords request;
    AnalyticFilteredGeometry geometry;
    std::vector<std::uint32_t> curve_operands;
    std::uint32_t left_operand_count = 0;
    std::uint32_t left_curve_count = 0;
    std::uint64_t work = 0;
    std::uint64_t memory = 0;
    std::uint64_t peak_memory = 0;
    std::uint64_t next_carrier = 1;
};

bool checked_difference(std::int64_t value, std::int64_t origin, std::int64_t& output) noexcept
{
    if ((origin > 0 && value < std::numeric_limits<std::int64_t>::min() + origin) ||
        (origin < 0 && value > std::numeric_limits<std::int64_t>::max() + origin))
        return false;
    output = value - origin;
    return true;
}

std::vector<std::uint32_t> ring_region_owners(const AnalyticResultPacketRecords& records)
{
    const std::uint32_t none = std::numeric_limits<std::uint32_t>::max();
    std::vector<std::uint32_t> owners(records.rings.size(), none);
    for (std::uint32_t region = 0; region < records.regions.size(); ++region)
    {
        const std::uint32_t outer = records.regions[region].outer_ring;
        if (outer >= owners.size() || owners[outer] != none)
            return {};
        owners[outer] = region;
    }
    std::vector<std::uint32_t> order(records.rings.size());
    for (std::uint32_t index = 0; index < order.size(); ++index)
        order[index] = index;
    std::sort(order.begin(), order.end(),
              [&](std::uint32_t left, std::uint32_t right)
              {
                  return std::tie(records.rings[left].depth, left) <
                         std::tie(records.rings[right].depth, right);
              });
    for (const std::uint32_t ring : order)
    {
        if (owners[ring] != none)
            continue;
        const std::uint32_t parent = records.rings[ring].parent_ring;
        if (parent >= owners.size() || owners[parent] == none)
            return {};
        owners[ring] = owners[parent];
    }
    return owners;
}

bool append_fragment(const AnalyticResultPacketRecords& records, std::uint32_t fragment_index,
                     std::uint64_t operand_id, std::uint32_t operand_index, std::int64_t origin_x,
                     std::int64_t origin_y, GeometryBuild& output)
{
    if (fragment_index >= records.fragments.size())
        return false;
    const auto& fragment = records.fragments[fragment_index];
    if (fragment.start_vertex >= records.vertices.size() ||
        fragment.end_vertex >= records.vertices.size())
        return false;
    const auto& global_start = records.vertices[fragment.start_vertex];
    const auto& global_end = records.vertices[fragment.end_vertex];
    std::int64_t start_x = 0;
    std::int64_t start_y = 0;
    std::int64_t end_x = 0;
    std::int64_t end_y = 0;
    if (!checked_difference(global_start.x_nm, origin_x, start_x) ||
        !checked_difference(global_start.y_nm, origin_y, start_y) ||
        !checked_difference(global_end.x_nm, origin_x, end_x) ||
        !checked_difference(global_end.y_nm, origin_y, end_y))
        return false;
    if (start_x == end_x && start_y == end_y)
        return false;

    AnalyticAtomicCurveNm curve;
    curve.curve_index = static_cast<std::uint32_t>(output.geometry.curves.size() + 1);
    curve.start = exact_point(start_x, start_y);
    curve.end = exact_point(end_x, end_y);
    curve.has_integer_certificate = true;
    curve.integer_start = {start_x, start_y};
    curve.integer_end = {end_x, end_y};

    AnalyticCurveBoundsNm bounds;
    bounds.curve_index = curve.curve_index;
    if (fragment.kind == 1 && fragment.direction == 0 && fragment.radius_nm == 0)
    {
        const std::int64_t dx = end_x - start_x;
        const std::int64_t dy = end_y - start_y;
        const bool agrees = dx > 0 || (dx == 0 && dy > 0);
        const std::int64_t divisor = std::gcd(std::abs(dx), std::abs(dy));
        curve.construction_carrier_id = output.next_carrier++;
        curve.construction_family_id = curve.construction_carrier_id;
        curve.kind = AnalyticAtomicCurveKind::line;
        curve.has_construction_line_direction = true;
        curve.construction_line_dx = agrees ? dx : -dx;
        curve.construction_line_dy = agrees ? dy : -dy;
        bounds.min_x = static_cast<double>(std::min(start_x, end_x));
        bounds.min_y = static_cast<double>(std::min(start_y, end_y));
        bounds.max_x = static_cast<double>(std::max(start_x, end_x));
        bounds.max_y = static_cast<double>(std::max(start_y, end_y));
        AnalyticFilteredOccurrence occurrence;
        occurrence.occurrence_id = curve.curve_index;
        occurrence.coverage_id = operand_id;
        occurrence.agrees_with_carrier = agrees;
        occurrence.material_on_left = true;
        occurrence.source.operand_id = operand_id;
        occurrence.source.primary_id = fragment.id;
        output.geometry.curves.push_back(curve);
        output.geometry.bounds.push_back(bounds);
        output.geometry.occurrences.push_back(occurrence);
        output.curve_operands.push_back(operand_index);
        return true;
    }
    if (fragment.kind != 2 || (fragment.direction != 1 && fragment.direction != 2) ||
        fragment.radius_nm == 0)
        return false;

    curve.kind = AnalyticAtomicCurveKind::circular_arc;
    curve.counterclockwise = fragment.direction == 1;
    curve.major_arc = fragment.major_arc;
    analytic_detail::Point center;
    if (!analytic_detail::reconstruct_endpoint_authoritative_arc_center(
            start_x, start_y, end_x, end_y, fragment.radius_nm, curve.counterclockwise,
            curve.major_arc, center))
        return false;
    curve.circle.center = {{center.x.lower, center.x.upper}, {center.y.lower, center.y.upper}, 0};
    curve.circle.radius = {static_cast<double>(fragment.radius_nm),
                           static_cast<double>(fragment.radius_nm)};
    curve.has_integer_certificate = false;
    curve.has_integer_radius_certificate = true;
    curve.integer_radius = fragment.radius_nm;
    curve.has_endpoint_authoritative_arc_certificate = true;
    curve.has_arc_sweep_certificate = true;
    const bool upper = analytic_detail::endpoint_authoritative_arc_is_x_monotone(
        start_x, start_y, end_x, end_y, fragment.radius_nm, curve.counterclockwise, curve.major_arc,
        center, true);
    const bool lower = analytic_detail::endpoint_authoritative_arc_is_x_monotone(
        start_x, start_y, end_x, end_y, fragment.radius_nm, curve.counterclockwise, curve.major_arc,
        center, false);
    if (upper != lower)
    {
        curve.has_endpoint_authoritative_x_monotone_certificate = true;
        curve.endpoint_authoritative_upper_branch = upper;
    }
    curve.construction_carrier_id = output.next_carrier++;
    curve.construction_family_id = curve.construction_carrier_id;
    const double radius = static_cast<double>(fragment.radius_nm);
    bounds.min_x = center.x.lower - radius;
    bounds.min_y = center.y.lower - radius;
    bounds.max_x = center.x.upper + radius;
    bounds.max_y = center.y.upper + radius;
    AnalyticFilteredOccurrence occurrence;
    occurrence.occurrence_id = curve.curve_index;
    occurrence.coverage_id = operand_id;
    occurrence.agrees_with_carrier = curve.counterclockwise;
    occurrence.material_on_left = true;
    occurrence.source.operand_id = operand_id;
    occurrence.source.primary_id = fragment.id;
    output.geometry.curves.push_back(curve);
    output.geometry.bounds.push_back(bounds);
    output.geometry.occurrences.push_back(occurrence);
    output.curve_operands.push_back(operand_index);
    return true;
}

struct EndpointColumn
{
    std::int64_t x = 0;
    std::int64_t y = 0;
    std::uint32_t curve = 0;
    bool start = true;
    bool right = false;

    auto key() const noexcept
    {
        return std::tie(right, x, y, curve, start);
    }

    auto group_key() const noexcept
    {
        return std::tie(right, x, y);
    }
};

struct ArcCarrierEntry
{
    ArcCarrierKey carrier;
    std::uint32_t curve = 0;

    auto key() const noexcept
    {
        return std::tie(carrier.exact_center, carrier.center_x, carrier.center_y, carrier.first_x,
                        carrier.first_y, carrier.second_x, carrier.second_y, carrier.radius,
                        carrier.center_on_positive_side, curve);
    }
};

struct LineCarrierEntry
{
    std::int64_t direction_x = 0;
    std::int64_t direction_y = 0;
    std::uint64_t constant_high = 0;
    std::uint64_t constant_low = 0;
    std::uint32_t curve = 0;

    auto carrier_key() const noexcept
    {
        return std::tie(direction_x, direction_y, constant_high, constant_low);
    }

    auto key() const noexcept
    {
        return std::tie(direction_x, direction_y, constant_high, constant_low, curve);
    }
};

static_assert(sizeof(LineCarrierEntry) <= kLineCarrierScratchBytes);
static_assert(sizeof(ArcCarrierEntry) <= kArcCarrierScratchBytes);
static_assert(sizeof(EndpointColumn) * 2 <= kArcCarrierScratchBytes);

bool assign_line_carriers(GeometryBuild& output)
{
    std::vector<LineCarrierEntry> entries;
    const std::size_t line_count = static_cast<std::size_t>(std::count_if(
        output.geometry.curves.begin(), output.geometry.curves.end(),
        [](const auto& curve) { return curve.kind == AnalyticAtomicCurveKind::line; }));
    entries.reserve(line_count);
    for (std::uint32_t curve = 0; curve < output.geometry.curves.size(); ++curve)
    {
        const auto& value = output.geometry.curves[curve];
        if (value.kind != AnalyticAtomicCurveKind::line || !value.has_construction_line_direction)
            continue;
        const std::int64_t divisor =
            std::gcd(std::abs(value.construction_line_dx), std::abs(value.construction_line_dy));
        if (divisor == 0)
            return false;
        const std::int64_t direction_x = value.construction_line_dx / divisor;
        const std::int64_t direction_y = value.construction_line_dy / divisor;
        const analytic_detail::WideInteger constant = analytic_detail::wide_subtract(
            analytic_detail::wide_multiply(direction_y, value.integer_start.x),
            analytic_detail::wide_multiply(direction_x, value.integer_start.y));
        entries.push_back({direction_x, direction_y, analytic_detail::wide_high_bits(constant),
                           analytic_detail::wide_low_bits(constant), curve});
    }
    std::sort(entries.begin(), entries.end(),
              [](const auto& left, const auto& right) { return left.key() < right.key(); });
    std::uint64_t carrier = 0;
    for (std::size_t index = 0; index < entries.size(); ++index)
    {
        if (index == 0 || entries[index - 1].carrier_key() != entries[index].carrier_key())
            ++carrier;
        auto& curve = output.geometry.curves[entries[index].curve];
        curve.construction_carrier_id = carrier;
        curve.construction_family_id = carrier;
        if (entries[index].direction_x == 0)
        {
            const std::uint64_t column = analytic_vertical_x_column_token(carrier);
            curve.start.construction_x_column_id = column;
            curve.end.construction_x_column_id = column;
        }
    }
    output.next_carrier = carrier + 1;
    output.work += entries.size() * 3 + sort_units(entries.size());
    return carrier < (std::uint64_t{1} << 31U);
}

bool assign_arc_carriers(GeometryBuild& output)
{
    std::vector<ArcCarrierEntry> entries;
    const std::size_t arc_count = static_cast<std::size_t>(std::count_if(
        output.geometry.curves.begin(), output.geometry.curves.end(),
        [](const auto& curve) { return curve.kind == AnalyticAtomicCurveKind::circular_arc; }));
    entries.reserve(arc_count);
    for (std::uint32_t curve = 0; curve < output.geometry.curves.size(); ++curve)
    {
        const auto& value = output.geometry.curves[curve];
        if (value.kind == AnalyticAtomicCurveKind::circular_arc)
        {
            const bool canonical_direction =
                std::tie(value.integer_start.x, value.integer_start.y) <
                std::tie(value.integer_end.x, value.integer_end.y);
            const auto& first = canonical_direction ? value.integer_start : value.integer_end;
            const auto& second = canonical_direction ? value.integer_end : value.integer_start;
            const bool exact_center = value.circle.center.x.lower == value.circle.center.x.upper &&
                                      value.circle.center.y.lower == value.circle.center.y.upper;
            entries.push_back(
                {{exact_center, exact_center ? value.circle.center.x.lower : 0.0,
                  exact_center ? value.circle.center.y.lower : 0.0, exact_center ? 0 : first.x,
                  exact_center ? 0 : first.y, exact_center ? 0 : second.x,
                  exact_center ? 0 : second.y, value.integer_radius,
                  !exact_center &&
                      (value.counterclockwise != value.major_arc) == canonical_direction},
                 curve});
        }
    }
    std::sort(entries.begin(), entries.end(),
              [](const auto& left, const auto& right) { return left.key() < right.key(); });
    std::uint64_t carrier = output.next_carrier - 1;
    for (std::size_t index = 0; index < entries.size(); ++index)
    {
        if (index == 0 || entries[index - 1].carrier.key() != entries[index].carrier.key())
            ++carrier;
        auto& curve = output.geometry.curves[entries[index].curve];
        curve.construction_carrier_id = carrier;
        curve.construction_family_id = carrier;
    }
    output.next_carrier = carrier + 1;
    output.work += entries.size() * 3 + sort_units(entries.size());
    return carrier < (std::uint64_t{1} << 31U);
}

bool assign_endpoint_columns(GeometryBuild& output)
{
    std::vector<EndpointColumn> columns;
    const std::size_t arc_count = static_cast<std::size_t>(std::count_if(
        output.geometry.curves.begin(), output.geometry.curves.end(),
        [](const auto& curve) { return curve.kind == AnalyticAtomicCurveKind::circular_arc; }));
    if (arc_count > std::numeric_limits<std::size_t>::max() / 2)
        return false;
    columns.reserve(arc_count * 2);
    for (std::uint32_t curve_index = 0; curve_index < output.geometry.curves.size(); ++curve_index)
    {
        const auto& curve = output.geometry.curves[curve_index];
        if (!curve.has_endpoint_authoritative_arc_certificate)
            continue;
        const analytic_detail::Interval center_x{curve.circle.center.x.lower,
                                                 curve.circle.center.x.upper};
        const analytic_detail::Interval radius{curve.circle.radius.lower,
                                               curve.circle.radius.upper};
        const analytic_detail::Interval left = analytic_detail::subtract(center_x, radius);
        const analytic_detail::Interval right = analytic_detail::add(center_x, radius);
        const auto collect = [&](const AnalyticFilteredPointNm& endpoint,
                                 const AnalyticIntegerPointNm& integer, bool start)
        {
            const AnalyticFilteredPointNm left_seam = {
                {left.lower, left.upper}, curve.circle.center.y, 0};
            const AnalyticFilteredPointNm right_seam = {
                {right.lower, right.upper}, curve.circle.center.y, 0};
            constexpr double resolution_squared =
                static_cast<double>(kAnalyticTopologyResolutionNm * kAnalyticTopologyResolutionNm);
            const auto point = [](const AnalyticFilteredPointNm& value)
            {
                return analytic_detail::Point{{value.x.lower, value.x.upper},
                                              {value.y.lower, value.y.upper}};
            };
            const bool near_left =
                analytic_detail::complete_distance_squared(point(left_seam), point(endpoint))
                    .upper <= resolution_squared;
            const bool near_right =
                analytic_detail::complete_distance_squared(point(right_seam), point(endpoint))
                    .upper <= resolution_squared;
            if (near_left == near_right)
                return;
            const auto& seam = near_right ? right_seam : left_seam;
            const bool same_seam =
                endpoint.x.lower == seam.x.lower && endpoint.x.upper == seam.x.upper &&
                endpoint.y.lower == seam.y.lower && endpoint.y.upper == seam.y.upper;
            if (!same_seam)
                columns.push_back({integer.x, integer.y, curve_index, start, near_right});
        };
        collect(curve.start, curve.integer_start, true);
        collect(curve.end, curve.integer_end, false);
    }
    std::sort(columns.begin(), columns.end(),
              [](const EndpointColumn& left, const EndpointColumn& right)
              { return left.key() < right.key(); });
    std::uint64_t group = 0;
    for (std::size_t index = 0; index < columns.size(); ++index)
    {
        if (index == 0 || columns[index - 1].group_key() != columns[index].group_key())
            ++group;
        auto& entry = columns[index];
        auto& curve = output.geometry.curves[entry.curve];
        (entry.start ? curve.start : curve.end).construction_x_column_id =
            analytic_endpoint_arc_partition_column_token(group, entry.right);
    }
    output.work += output.geometry.curves.size() * 4 + sort_units(columns.size());
    return group < (std::uint64_t{1} << 61U);
}

GeometryBuild build_geometry(const AnalyticResultPacketRecords& records,
                             const std::vector<std::uint32_t>& region_ring_offsets,
                             const std::vector<std::uint32_t>& region_rings,
                             const AnalyticJobResultRecord& left_job,
                             const AnalyticJobResultRecord& right_job, bool self)
{
    GeometryBuild output;
    const std::uint64_t region_count = static_cast<std::uint64_t>(left_job.result_region_count) +
                                       (self ? 0 : right_job.result_region_count);
    if (region_count == 0)
        return output;
    if (region_count > std::numeric_limits<std::uint32_t>::max())
    {
        output.error = EvaluationError::resource_limit_exceeded;
        return output;
    }
    output.left_operand_count = left_job.result_region_count;

    const auto fragment_count = [&](const AnalyticJobResultRecord& job)
    {
        std::uint64_t count = 0;
        const std::uint32_t end = job.result_region_begin + job.result_region_count;
        for (std::uint32_t region = job.result_region_begin; region < end; ++region)
            for (std::uint32_t at = region_ring_offsets[region];
                 at < region_ring_offsets[region + 1]; ++at)
            {
                ++output.work;
                count += records.rings[region_rings[at]].fragment_reference_count;
            }
        return count;
    };
    const std::uint64_t curve_count =
        fragment_count(left_job) + (self ? 0 : fragment_count(right_job));
    if (curve_count > std::numeric_limits<std::uint32_t>::max())
    {
        output.error = EvaluationError::resource_limit_exceeded;
        return output;
    }
    output.geometry.curves.reserve(static_cast<std::size_t>(curve_count));
    output.geometry.bounds.reserve(static_cast<std::size_t>(curve_count));
    output.geometry.occurrences.reserve(static_cast<std::size_t>(curve_count));
    output.curve_operands.reserve(static_cast<std::size_t>(curve_count));
    output.request.jobs.reserve(1);
    output.request.stages.reserve(1);
    output.request.operands.reserve(static_cast<std::size_t>(region_count));

    bool have_point = false;
    std::int64_t minimum_x = 0;
    std::int64_t maximum_x = 0;
    std::int64_t minimum_y = 0;
    std::int64_t maximum_y = 0;
    auto visit_region_vertices = [&](const AnalyticJobResultRecord& job)
    {
        const std::uint32_t region_end = job.result_region_begin + job.result_region_count;
        for (std::uint32_t region = job.result_region_begin; region < region_end; ++region)
        {
            for (std::uint32_t at = region_ring_offsets[region];
                 at < region_ring_offsets[region + 1]; ++at)
            {
                const auto& value = records.rings[region_rings[at]];
                for (std::uint32_t offset = 0; offset < value.fragment_reference_count; ++offset)
                {
                    ++output.work;
                    const std::uint32_t fragment =
                        records.fragment_references[value.fragment_reference_begin + offset];
                    for (const std::uint32_t vertex : {records.fragments[fragment].start_vertex,
                                                       records.fragments[fragment].end_vertex})
                    {
                        const auto& point = records.vertices[vertex];
                        if (!have_point)
                        {
                            minimum_x = maximum_x = point.x_nm;
                            minimum_y = maximum_y = point.y_nm;
                            have_point = true;
                        }
                        else
                        {
                            minimum_x = std::min(minimum_x, point.x_nm);
                            maximum_x = std::max(maximum_x, point.x_nm);
                            minimum_y = std::min(minimum_y, point.y_nm);
                            maximum_y = std::max(maximum_y, point.y_nm);
                        }
                    }
                }
            }
        }
    };
    visit_region_vertices(left_job);
    if (!self)
        visit_region_vertices(right_job);
    if (!have_point)
        return output;
    const auto span = [](std::int64_t minimum, std::int64_t maximum)
    { return static_cast<std::uint64_t>(maximum) - static_cast<std::uint64_t>(minimum); };
    if (span(minimum_x, maximum_x) > 2'000'000'000'000ULL ||
        span(minimum_y, maximum_y) > 2'000'000'000'000ULL)
    {
        output.error = EvaluationError::solver_failed;
        return output;
    }
    const auto midpoint = [](std::int64_t minimum, std::int64_t maximum)
    {
        const std::int64_t halves = minimum / 2 + maximum / 2;
        return halves + (minimum % 2 + maximum % 2) / 2;
    };
    output.geometry.origin_x_nm = midpoint(minimum_x, maximum_x);
    output.geometry.origin_y_nm = midpoint(minimum_y, maximum_y);

    output.request.jobs.push_back({1, 0, 1});
    output.request.stages.push_back({1, 1, 0, static_cast<std::uint32_t>(region_count)});
    auto append_job = [&](const AnalyticJobResultRecord& job, std::uint32_t operand_base)
    {
        const std::uint32_t region_end = job.result_region_begin + job.result_region_count;
        for (std::uint32_t region = job.result_region_begin; region < region_end; ++region)
            output.request.operands.push_back({records.regions[region].id, 2, 0});
        for (std::uint32_t owner = job.result_region_begin; owner < region_end; ++owner)
        {
            for (std::uint32_t at = region_ring_offsets[owner]; at < region_ring_offsets[owner + 1];
                 ++at)
            {
                const std::uint64_t operand_id = records.regions[owner].id;
                const std::uint32_t operand_index = operand_base + owner - job.result_region_begin;
                const auto& value = records.rings[region_rings[at]];
                for (std::uint32_t offset = 0; offset < value.fragment_reference_count; ++offset)
                    if (!append_fragment(
                            records,
                            records.fragment_references[value.fragment_reference_begin + offset],
                            operand_id, operand_index, output.geometry.origin_x_nm,
                            output.geometry.origin_y_nm, output))
                        return false;
            }
        }
        return true;
    };
    if (!append_job(left_job, 0))
    {
        output.error = EvaluationError::solver_failed;
        return output;
    }
    output.left_curve_count = static_cast<std::uint32_t>(output.geometry.curves.size());
    if (!self && !append_job(right_job, left_job.result_region_count))
    {
        output.error = EvaluationError::solver_failed;
        return output;
    }
    if (!assign_line_carriers(output) || !assign_arc_carriers(output) ||
        !assign_endpoint_columns(output))
    {
        output.error = EvaluationError::resource_limit_exceeded;
        return output;
    }
    output.work += output.geometry.curves.size() * 5 + region_count * 3;
    std::uint64_t curve_bytes = 0;
    std::uint64_t operand_bytes = 0;
    std::uint64_t line_count = 0;
    std::uint64_t arc_count = 0;
    for (const auto& curve : output.geometry.curves)
        (curve.kind == AnalyticAtomicCurveKind::line ? line_count : arc_count) += 1;
    std::uint64_t line_scratch_bytes = 0;
    std::uint64_t arc_scratch_bytes = 0;
    if (!checked_multiply(output.geometry.curves.size(), kGeometryCurveBytes, curve_bytes) ||
        !checked_multiply(region_count, kGeometryOperandBytes, operand_bytes) ||
        !checked_add(kGeometryFixedBytes, curve_bytes, output.memory) ||
        !checked_add(output.memory, operand_bytes, output.memory) ||
        !checked_multiply(line_count, kLineCarrierScratchBytes, line_scratch_bytes) ||
        !checked_multiply(arc_count, kArcCarrierScratchBytes, arc_scratch_bytes) ||
        !checked_add(output.memory, std::max(line_scratch_bytes, arc_scratch_bytes),
                     output.peak_memory))
        output.error = EvaluationError::resource_limit_exceeded;
    return output;
}

bool collect_coverage(const AnalyticFilteredBooleanSelectionResult& selection, std::uint32_t root,
                      std::uint32_t begin, std::uint32_t capacity,
                      std::vector<std::uint32_t>& output, std::uint64_t& work,
                      std::uint64_t work_limit, bool& resource_failure) noexcept
{
    if (work >= work_limit)
    {
        resource_failure = true;
        return false;
    }
    ++work;
    if (root == 0)
        return true;
    if (capacity == 1)
    {
        if (root != 1 || begin >= selection.telemetry.input_operands)
            return false;
        output.push_back(begin);
        return true;
    }
    if (root == 1 || root >= selection.coverage_state_nodes.size())
        return false;
    const std::uint32_t half = capacity / 2;
    return collect_coverage(selection, selection.coverage_state_nodes[root].left, begin, half,
                            output, work, work_limit, resource_failure) &&
           collect_coverage(selection, selection.coverage_state_nodes[root].right, begin + half,
                            half, output, work, work_limit, resource_failure);
}

PairBuild evaluate_pair(const AnalyticResultPacketRecords& records,
                        const std::vector<std::uint32_t>& region_ring_offsets,
                        const std::vector<std::uint32_t>& region_rings,
                        const AnalyticJobResultRecord& left_job,
                        const AnalyticJobResultRecord& right_job, bool self,
                        const AnalyticSolverLimits& supplied_limits, std::uint64_t work_limit,
                        std::uint64_t memory_limit, std::uint64_t retained_memory)
{
    PairBuild output;
    std::uint64_t expected_curves = 0;
    std::uint64_t expected_lines = 0;
    std::uint64_t expected_arcs = 0;
    std::uint64_t preflight_work = 0;
    bool invalid_fragment = false;
    const auto admit_preflight_work = [&](std::uint64_t amount)
    { return checked_add(preflight_work, amount, preflight_work) && preflight_work <= work_limit; };
    const auto count_job_fragments = [&](const AnalyticJobResultRecord& job)
    {
        const std::uint32_t end = job.result_region_begin + job.result_region_count;
        if (!admit_preflight_work(job.result_region_count))
            return false;
        for (std::uint32_t region = job.result_region_begin; region < end; ++region)
        {
            const std::uint64_t ring_count =
                region_ring_offsets[region + 1] - region_ring_offsets[region];
            if (!admit_preflight_work(ring_count))
                return false;
            for (std::uint32_t at = region_ring_offsets[region];
                 at < region_ring_offsets[region + 1]; ++at)
            {
                const auto& ring = records.rings[region_rings[at]];
                const std::uint64_t count = ring.fragment_reference_count;
                if (!admit_preflight_work(count) ||
                    !checked_add(expected_curves, count, expected_curves))
                    return false;
                for (std::uint32_t offset = 0; offset < ring.fragment_reference_count; ++offset)
                {
                    const std::uint32_t fragment =
                        records.fragment_references[ring.fragment_reference_begin + offset];
                    if (fragment >= records.fragments.size())
                    {
                        invalid_fragment = true;
                        return false;
                    }
                    if (records.fragments[fragment].kind == 1)
                        ++expected_lines;
                    else if (records.fragments[fragment].kind == 2)
                        ++expected_arcs;
                }
            }
        }
        return true;
    };
    if (!count_job_fragments(left_job) || (!self && !count_job_fragments(right_job)) ||
        expected_curves > std::numeric_limits<std::uint32_t>::max())
    {
        output.error = invalid_fragment ? EvaluationError::solver_failed
                                        : EvaluationError::resource_limit_exceeded;
        return output;
    }
    const std::uint64_t expected_regions =
        static_cast<std::uint64_t>(left_job.result_region_count) +
        (self ? 0 : right_job.result_region_count);
    std::uint64_t expected_retained = 0;
    std::uint64_t expected_peak = 0;
    std::uint64_t expected_term = 0;
    std::uint64_t line_scratch = 0;
    std::uint64_t arc_scratch = 0;
    std::uint64_t carrier_sort_work = 0;
    std::uint64_t carrier_term = 0;
    if (!checked_multiply(expected_curves, kGeometryCurveBytes, expected_retained) ||
        !checked_multiply(expected_regions, kGeometryOperandBytes, expected_term) ||
        !checked_add(expected_retained, expected_term, expected_retained) ||
        !checked_add(expected_retained, kGeometryFixedBytes, expected_retained) ||
        !checked_multiply(expected_lines, kLineCarrierScratchBytes, line_scratch) ||
        !checked_multiply(expected_arcs, kArcCarrierScratchBytes, arc_scratch) ||
        !checked_add(expected_retained, std::max(line_scratch, arc_scratch), expected_peak) ||
        !checked_multiply(expected_lines, 3, carrier_sort_work) ||
        !checked_add(carrier_sort_work, sort_units(expected_lines), carrier_sort_work) ||
        !checked_multiply(expected_arcs, 3, carrier_term) ||
        !checked_add(carrier_sort_work, carrier_term, carrier_sort_work) ||
        !checked_add(carrier_sort_work, sort_units(expected_arcs), carrier_sort_work) ||
        !checked_multiply(expected_curves, 4, carrier_term) ||
        !checked_add(carrier_sort_work, carrier_term, carrier_sort_work) ||
        !checked_multiply(expected_arcs, 2, carrier_term) ||
        !checked_add(carrier_sort_work, sort_units(carrier_term), carrier_sort_work) ||
        !checked_add(preflight_work, preflight_work, carrier_term) ||
        !checked_add(carrier_term, carrier_sort_work, carrier_term) ||
        !checked_multiply(expected_curves, 5, carrier_sort_work) ||
        !checked_add(carrier_term, carrier_sort_work, carrier_term) ||
        !checked_multiply(expected_regions, 3, carrier_sort_work) ||
        !checked_add(carrier_term, carrier_sort_work, carrier_term) || carrier_term > work_limit ||
        retained_memory > memory_limit || expected_peak > memory_limit - retained_memory)
    {
        output.error = EvaluationError::resource_limit_exceeded;
        output.required_memory = retained_memory + expected_peak;
        return output;
    }
    GeometryBuild built =
        build_geometry(records, region_ring_offsets, region_rings, left_job, right_job, self);
    if (!checked_add(preflight_work, built.work, output.work))
    {
        output.error = EvaluationError::resource_limit_exceeded;
        return output;
    }
    output.peak_memory = built.peak_memory;
    if (built.error != EvaluationError::none)
    {
        output.error = built.error;
        return output;
    }
    if (built.geometry.curves.empty())
        return output;
    if (output.work > work_limit || built.memory > memory_limit ||
        retained_memory > memory_limit - built.memory)
    {
        output.error = EvaluationError::resource_limit_exceeded;
        output.required_memory = retained_memory + built.memory;
        return output;
    }

    AnalyticSolverLimits limits = supplied_limits;
    limits.predicate_calls = std::min(limits.predicate_calls, work_limit - output.work);
    limits.working_memory_bytes =
        std::min(limits.working_memory_bytes, memory_limit - retained_memory - built.memory);
    AnalyticBroadPhaseResult broad =
        self ? analytic_execution_detail::build_curve_candidates(
                   built.geometry.bounds, limits,
                   analytic_execution_detail::kStrictPublishedGeometry)
             : analytic_execution_detail::build_bipartite_curve_candidates(
                   built.geometry.bounds, built.left_curve_count, limits,
                   analytic_execution_detail::kStrictPublishedGeometry);
    if (!checked_add(output.work, broad.telemetry.work_units, output.work) ||
        !checked_add(output.algebraic_fallback_calls, broad.telemetry.algebraic_fallback_calls,
                     output.algebraic_fallback_calls))
    {
        output.error = EvaluationError::resource_limit_exceeded;
        return output;
    }
    output.candidates = broad.telemetry.candidate_pairs;
    output.peak_memory =
        std::max(output.peak_memory, built.memory + broad.telemetry.peak_working_memory_bytes);
    if (broad.error != AnalyticBroadPhaseError::none)
    {
        output.error = broad.error == AnalyticBroadPhaseError::resource_limit_exceeded
                           ? EvaluationError::resource_limit_exceeded
                           : EvaluationError::solver_failed;
        output.required_memory =
            retained_memory + built.memory + broad.telemetry.required_working_memory_bytes;
        return output;
    }

    std::vector<AnalyticCurvePair> candidates = std::move(broad.pairs);
    output.candidates = candidates.size();
    if (candidates.size() > limits.examined_curve_pairs)
    {
        output.error = EvaluationError::resource_limit_exceeded;
        return output;
    }
    std::uint64_t minimum_candidate_bytes = 0;
    if (!checked_multiply(candidates.size(), kCandidatePairBytes, minimum_candidate_bytes) ||
        broad.telemetry.retained_pair_bytes < minimum_candidate_bytes)
    {
        output.error = EvaluationError::solver_failed;
        return output;
    }
    const std::uint64_t candidate_bytes = broad.telemetry.retained_pair_bytes;
    output.peak_memory = std::max(output.peak_memory, built.memory + candidate_bytes);

    if (output.work > work_limit)
    {
        output.error = EvaluationError::resource_limit_exceeded;
        return output;
    }
    limits.predicate_calls = std::min(supplied_limits.predicate_calls, work_limit - output.work);
    if (built.memory > memory_limit - retained_memory ||
        candidate_bytes > memory_limit - retained_memory - built.memory)
    {
        output.error = EvaluationError::resource_limit_exceeded;
        output.required_memory = retained_memory + built.memory + candidate_bytes;
        return output;
    }
    limits.working_memory_bytes =
        std::min(supplied_limits.working_memory_bytes,
                 memory_limit - retained_memory - built.memory - candidate_bytes);
    const AnalyticFilteredBooleanSelectionResult selection =
        analytic_execution_detail::build_boolean_selection(
            built.request, 0, built.geometry, candidates, limits,
            analytic_execution_detail::kStrictPublishedGeometry);
    if (!checked_add(output.work, selection.telemetry.predicate_calls, output.work) ||
        !checked_add(output.algebraic_fallback_calls, selection.telemetry.algebraic_fallback_calls,
                     output.algebraic_fallback_calls))
    {
        output.error = EvaluationError::resource_limit_exceeded;
        return output;
    }
    output.peak_memory =
        std::max(output.peak_memory,
                 built.memory + candidate_bytes + selection.telemetry.peak_working_memory_bytes);
    if (selection.error != AnalyticFilteredBooleanSelectionError::none)
    {
        output.unresolved_predicate_failure = selection.telemetry.unresolved_predicate_failure;
        output.error = selection.error == AnalyticFilteredBooleanSelectionError::invalid_argument ||
                               selection.telemetry.unresolved_predicate_failure
                           ? EvaluationError::solver_failed
                           : EvaluationError::resource_limit_exceeded;
        output.required_memory = retained_memory + built.memory + candidate_bytes +
                                 selection.telemetry.required_working_memory_bytes;
        return output;
    }
    if (output.work > work_limit)
    {
        output.error = EvaluationError::resource_limit_exceeded;
        return output;
    }

    const std::uint32_t operand_count = static_cast<std::uint32_t>(built.request.operands.size());
    const auto charge = [&](std::uint64_t units)
    {
        std::uint64_t next = 0;
        if (!checked_add(output.work, units, next) || next > work_limit)
            return false;
        output.work = next;
        return true;
    };
    const std::uint64_t base_live =
        built.memory + candidate_bytes + selection.telemetry.peak_working_memory_bytes;
    std::uint64_t fixed_classification_bytes = 0;
    std::uint64_t term = 0;
    if (!checked_multiply(operand_count, 14, fixed_classification_bytes) ||
        !checked_multiply(selection.arrangement.edges.size() + 1, kIndexBytes, term) ||
        !checked_add(fixed_classification_bytes, term, fixed_classification_bytes) ||
        !checked_multiply(selection.arrangement.memberships.size(), kIndexBytes, term) ||
        !checked_add(fixed_classification_bytes, term, fixed_classification_bytes) ||
        retained_memory > memory_limit || base_live > memory_limit - retained_memory ||
        fixed_classification_bytes > memory_limit - retained_memory - base_live)
    {
        output.error = EvaluationError::resource_limit_exceeded;
        output.required_memory = retained_memory + base_live + fixed_classification_bytes;
        return output;
    }
    output.peak_memory = std::max(output.peak_memory, base_live + fixed_classification_bytes);

    std::vector<bool> saw_face(operand_count);
    std::vector<bool> fully_covered(operand_count, true);
    std::vector<std::uint32_t> unique_cover(operand_count,
                                            std::numeric_limits<std::uint32_t>::max());
    std::vector<std::uint32_t> scratch;
    scratch.reserve(operand_count);
    std::vector<std::uint32_t> incidence_marks(operand_count);
    std::vector<std::uint32_t> edge_offsets(selection.arrangement.edges.size() + 1);
    std::vector<std::uint32_t> edge_owners;
    edge_owners.reserve(selection.arrangement.memberships.size());
    for (std::uint32_t edge = 0; edge < selection.arrangement.edges.size(); ++edge)
    {
        edge_offsets[edge] = static_cast<std::uint32_t>(edge_owners.size());
        const auto& value = selection.arrangement.edges[edge];
        if (!charge(value.membership_count))
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        for (std::uint32_t offset = 0; offset < value.membership_count; ++offset)
        {
            const auto& membership =
                selection.arrangement.memberships[value.membership_begin + offset];
            if (membership.curve_index != 0 &&
                membership.curve_index <= built.curve_operands.size())
                edge_owners.push_back(built.curve_operands[membership.curve_index - 1]);
        }
        auto begin = edge_owners.begin() + edge_offsets[edge];
        if (!charge(sort_units(edge_owners.size() - edge_offsets[edge])))
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        std::sort(begin, edge_owners.end());
        edge_owners.erase(std::unique(begin, edge_owners.end()), edge_owners.end());
    }
    edge_offsets.back() = static_cast<std::uint32_t>(edge_owners.size());

    std::uint32_t incidence_generation = 0;
    bool vertex_collection_resource_failure = false;
    const auto charge_vertex_collection = [&](std::uint64_t units)
    {
        if (charge(units))
            return true;
        vertex_collection_resource_failure = true;
        return false;
    };
    const auto collect_vertex_operands = [&](const auto& vertex)
    {
        if (!charge_vertex_collection(vertex.outgoing_count))
            return false;
        if (incidence_generation == std::numeric_limits<std::uint32_t>::max())
        {
            if (!charge_vertex_collection(incidence_marks.size()))
                return false;
            std::fill(incidence_marks.begin(), incidence_marks.end(), 0);
            incidence_generation = 1;
        }
        else
            ++incidence_generation;
        scratch.clear();
        for (std::uint32_t offset = 0; offset < vertex.outgoing_count; ++offset)
        {
            const std::uint32_t half_edge =
                selection.arrangement.outgoing_half_edges[vertex.outgoing_begin + offset];
            if (half_edge >= selection.arrangement.half_edges.size())
                return false;
            const std::uint32_t edge = selection.arrangement.half_edges[half_edge].edge;
            if (edge >= selection.arrangement.edges.size())
                return false;
            const std::uint32_t begin = edge_offsets[edge];
            const std::uint32_t end = edge_offsets[edge + 1];
            if (!charge_vertex_collection(end - begin))
                return false;
            for (std::uint32_t at = begin; at < end; ++at)
            {
                const std::uint32_t operand = edge_owners[at];
                if (incidence_marks[operand] == incidence_generation)
                    continue;
                incidence_marks[operand] = incidence_generation;
                scratch.push_back(operand);
            }
        }
        if (!charge_vertex_collection(sort_units(scratch.size())))
            return false;
        std::sort(scratch.begin(), scratch.end());
        return true;
    };

    const auto incidence_count =
        [&](const std::vector<std::uint32_t>& operands, std::uint64_t& count)
    {
        if (self)
            return checked_multiply(operands.size(), operands.size() - (operands.empty() ? 0 : 1),
                                    count);
        const auto split =
            std::lower_bound(operands.begin(), operands.end(), built.left_operand_count);
        return checked_multiply(static_cast<std::uint64_t>(split - operands.begin()),
                                static_cast<std::uint64_t>(operands.end() - split), count);
    };
    const auto append_count = [&](std::uint64_t& total, std::uint64_t amount)
    { return checked_add(total, amount, total); };

    std::uint64_t event_capacity = self ? operand_count : 0;
    std::uint32_t coverage_capacity = 1;
    while (coverage_capacity < std::max<std::uint32_t>(1, operand_count))
        coverage_capacity <<= 1U;
    for (const auto& face : selection.faces)
    {
        if (face.unbounded)
            continue;
        scratch.clear();
        bool coverage_resource_failure = false;
        if (!collect_coverage(selection, face.coverage_state_root, 0, coverage_capacity, scratch,
                              output.work, work_limit, coverage_resource_failure))
        {
            output.error = coverage_resource_failure ? EvaluationError::resource_limit_exceeded
                                                     : EvaluationError::solver_failed;
            return output;
        }
        if (!charge(sort_units(scratch.size()) + scratch.size() * 2))
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        std::sort(scratch.begin(), scratch.end());
        scratch.erase(std::unique(scratch.begin(), scratch.end()), scratch.end());
        for (const std::uint32_t operand : scratch)
        {
            saw_face[operand] = true;
            std::optional<std::uint32_t> other;
            for (const std::uint32_t candidate : scratch)
            {
                const bool opposite = self ? candidate != operand
                                           : (operand < built.left_operand_count) !=
                                                 (candidate < built.left_operand_count);
                if (!opposite)
                    continue;
                if (other)
                {
                    output.error = EvaluationError::solver_failed;
                    return output;
                }
                other = candidate;
            }
            if (!other)
                fully_covered[operand] = false;
            else if (unique_cover[operand] == std::numeric_limits<std::uint32_t>::max())
                unique_cover[operand] = *other;
            else if (unique_cover[operand] != *other)
                fully_covered[operand] = false;
        }
        if (!self)
        {
            std::uint64_t count = 0;
            if (!incidence_count(scratch, count) || !append_count(event_capacity, count))
            {
                output.error = EvaluationError::resource_limit_exceeded;
                return output;
            }
        }
    }
    for (std::uint32_t operand = 0; operand < operand_count; ++operand)
        if (!self && saw_face[operand] && fully_covered[operand] &&
            unique_cover[operand] != std::numeric_limits<std::uint32_t>::max() &&
            !append_count(event_capacity, 1))
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
    for (std::uint32_t edge = 0; edge < selection.arrangement.edges.size(); ++edge)
    {
        const std::uint64_t owner_count = edge_offsets[edge + 1] - edge_offsets[edge];
        if (!charge(owner_count + 1))
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        scratch.assign(edge_owners.begin() + edge_offsets[edge],
                       edge_owners.begin() + edge_offsets[edge + 1]);
        std::uint64_t count = 0;
        if (!incidence_count(scratch, count) || !append_count(event_capacity, count))
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
    }
    for (const auto& vertex : selection.arrangement.vertices)
    {
        if (!collect_vertex_operands(vertex))
        {
            output.error = vertex_collection_resource_failure
                               ? EvaluationError::resource_limit_exceeded
                               : EvaluationError::solver_failed;
            return output;
        }
        std::uint64_t count = 0;
        if (!charge(1) || !incidence_count(scratch, count) || !append_count(event_capacity, count))
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
    }

    std::uint64_t event_bytes = 0;
    std::uint64_t maximum_output_bytes = 0;
    if (!checked_multiply(event_capacity, kRelationEventBytes, event_bytes) ||
        !checked_multiply(event_capacity, kRelationshipRowBytes, maximum_output_bytes) ||
        !checked_add(fixed_classification_bytes, event_bytes, term) ||
        !checked_add(term, maximum_output_bytes, term) ||
        term > memory_limit - retained_memory - base_live)
    {
        output.error = EvaluationError::resource_limit_exceeded;
        output.required_memory = retained_memory + base_live + fixed_classification_bytes +
                                 event_bytes + maximum_output_bytes;
        return output;
    }
    output.peak_memory = std::max(output.peak_memory, base_live + term);
    std::vector<RelationEvent> events;
    events.reserve(static_cast<std::size_t>(event_capacity));
    const auto add_incidence =
        [&](const std::vector<std::uint32_t>& operands, std::uint8_t dimension)
    {
        std::uint64_t count = 0;
        if (!incidence_count(operands, count) || !charge(count))
            return false;
        if (self)
        {
            for (const std::uint32_t left : operands)
                for (const std::uint32_t right : operands)
                    if (left != right)
                        events.push_back({{left, right}, dimension, false, false, false});
        }
        else
        {
            for (const std::uint32_t left : operands)
                if (left < built.left_operand_count)
                    for (const std::uint32_t right : operands)
                        if (right >= built.left_operand_count)
                            events.push_back({{left, right}, dimension, false, false, false});
        }
        return true;
    };
    if (self)
    {
        if (!charge(operand_count))
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        for (std::uint32_t operand = 0; operand < operand_count; ++operand)
            events.push_back({{operand, operand}, 3, true, true, true});
    }
    else
    {
        std::uint64_t containment_events = 0;
        if (!charge(operand_count))
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        for (std::uint32_t operand = 0; operand < operand_count; ++operand)
            containment_events +=
                saw_face[operand] && fully_covered[operand] &&
                unique_cover[operand] != std::numeric_limits<std::uint32_t>::max();
        if (!charge(static_cast<std::uint64_t>(operand_count) + containment_events))
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        for (std::uint32_t operand = 0; operand < operand_count; ++operand)
        {
            if (!saw_face[operand] || !fully_covered[operand] ||
                unique_cover[operand] == std::numeric_limits<std::uint32_t>::max())
                continue;
            const std::uint32_t other = unique_cover[operand];
            const RegionPair pair = operand < built.left_operand_count ? RegionPair{operand, other}
                                                                       : RegionPair{other, operand};
            events.push_back({pair, 0, false, operand >= built.left_operand_count,
                              operand < built.left_operand_count});
        }
        for (const auto& face : selection.faces)
        {
            if (face.unbounded)
                continue;
            scratch.clear();
            bool coverage_resource_failure = false;
            if (!collect_coverage(selection, face.coverage_state_root, 0, coverage_capacity,
                                  scratch, output.work, work_limit, coverage_resource_failure))
            {
                output.error = coverage_resource_failure ? EvaluationError::resource_limit_exceeded
                                                         : EvaluationError::solver_failed;
                return output;
            }
            if (!charge(sort_units(scratch.size()) + scratch.size()))
            {
                output.error = EvaluationError::resource_limit_exceeded;
                return output;
            }
            std::sort(scratch.begin(), scratch.end());
            scratch.erase(std::unique(scratch.begin(), scratch.end()), scratch.end());
            if (!add_incidence(scratch, 3))
            {
                output.error = EvaluationError::resource_limit_exceeded;
                return output;
            }
        }
    }
    for (std::uint32_t edge = 0; edge < selection.arrangement.edges.size(); ++edge)
    {
        const std::uint64_t owner_count = edge_offsets[edge + 1] - edge_offsets[edge];
        if (!charge(owner_count))
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        scratch.assign(edge_owners.begin() + edge_offsets[edge],
                       edge_owners.begin() + edge_offsets[edge + 1]);
        if (!add_incidence(scratch, 2))
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
    }
    for (const auto& vertex : selection.arrangement.vertices)
    {
        if (!collect_vertex_operands(vertex))
        {
            output.error = vertex_collection_resource_failure
                               ? EvaluationError::resource_limit_exceeded
                               : EvaluationError::solver_failed;
            return output;
        }
        if (!add_incidence(scratch, 1))
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
    }
    if (events.size() > event_capacity || !charge(sort_units(events.size()) + events.size()))
    {
        output.error = EvaluationError::resource_limit_exceeded;
        return output;
    }
    std::sort(events.begin(), events.end(),
              [](const auto& left, const auto& right) { return left.pair < right.pair; });
    std::size_t output_pair_count = 0;
    for (std::size_t begin = 0; begin < events.size();)
    {
        std::size_t end = begin + 1;
        std::uint8_t dimension = events[begin].dimension;
        while (end < events.size() && events[end].pair == events[begin].pair)
        {
            dimension = std::max(dimension, events[end].dimension);
            ++end;
        }
        output_pair_count += dimension != 0;
        begin = end;
    }
    output.value.pairs.reserve(output_pair_count);
    std::uint64_t output_build_work = 0;
    if (!checked_multiply(output_pair_count, 4, output_build_work) ||
        !checked_add(output_build_work, events.size(), output_build_work) ||
        !charge(output_build_work))
    {
        output.error = EvaluationError::resource_limit_exceeded;
        return output;
    }
    for (std::size_t begin = 0; begin < events.size();)
    {
        std::size_t end = begin + 1;
        PairValue value;
        while (end < events.size() && events[end].pair == events[begin].pair)
            ++end;
        for (std::size_t index = begin; index < end; ++index)
        {
            value.dimension = std::max(value.dimension, events[index].dimension);
            value.equality = value.equality || events[index].equality;
            value.left_contains_right =
                value.left_contains_right || events[index].left_contains_right;
            value.right_contains_left =
                value.right_contains_left || events[index].right_contains_left;
        }
        value.equality = value.equality || (value.left_contains_right && value.right_contains_left);
        const RegionPair pair = events[begin].pair;
        begin = end;
        if (value.dimension == 0)
            continue;
        const std::uint32_t left_region =
            pair.first < built.left_operand_count
                ? left_job.result_region_begin + pair.first
                : right_job.result_region_begin + pair.first - built.left_operand_count;
        const std::uint32_t right_region =
            self ? left_job.result_region_begin + pair.second
                 : right_job.result_region_begin + pair.second - built.left_operand_count;
        output.value.pairs.push_back(
            {records.regions[left_region].id, records.regions[right_region].id, value.dimension,
             value.equality, value.left_contains_right, value.right_contains_left});
        output.value.aggregate = std::max(output.value.aggregate, value.dimension);
    }
    if (output.value.pairs.size() != output_pair_count ||
        !charge(sort_units(output.value.pairs.size())))
    {
        output.error = EvaluationError::resource_limit_exceeded;
        return output;
    }
    std::sort(output.value.pairs.begin(), output.value.pairs.end(),
              [](const auto& left, const auto& right)
              {
                  return std::tie(left.left_result_region_id, left.right_result_region_id,
                                  left.dimension, left.equality, left.left_contains_right,
                                  left.right_contains_left) <
                         std::tie(right.left_result_region_id, right.right_result_region_id,
                                  right.dimension, right.equality, right.left_contains_right,
                                  right.right_contains_left);
              });
    return output;
}

} // namespace

EvaluationResult evaluate(const AnalyticRequestPacketRecords& request,
                          const AnalyticResultPacketRecords& published,
                          const AnalyticSolverLimits& per_pair_limits, std::uint64_t work_limit,
                          std::uint64_t working_memory_limit, std::uint64_t retained_memory_bytes,
                          std::uint64_t maximum_packet_bytes)
{
    EvaluationResult output;
    if (request.relationship_queries.empty())
        return output;
    try
    {
        if (published.job_results.size() != request.jobs.size() ||
            !published.relationship_results.empty() || !published.relationship_pairs.empty())
        {
            output.error = EvaluationError::invalid_argument;
            return output;
        }
        std::uint64_t job_search_work = 0;
        std::uint64_t job_search_term = 0;
        if (!checked_multiply(ceil_log2(published.job_results.size()) + 1, 4, job_search_term) ||
            !checked_multiply(request.relationship_queries.size(), job_search_term,
                              job_search_work) ||
            job_search_work > work_limit)
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        output.telemetry.work_units = job_search_work;
        const auto find_job_index = [&](std::uint64_t id) -> std::optional<std::uint32_t>
        {
            const auto found =
                std::lower_bound(published.job_results.begin(), published.job_results.end(), id,
                                 [](const AnalyticJobResultRecord& job, std::uint64_t value)
                                 { return job.job_id < value; });
            if (found == published.job_results.end() || found->job_id != id)
                return std::nullopt;
            return static_cast<std::uint32_t>(found - published.job_results.begin());
        };
        std::uint64_t successful_queries = 0;
        for (const auto& query : request.relationship_queries)
        {
            const auto left = find_job_index(query.left_job_id);
            const auto right = find_job_index(query.right_job_id);
            if (!left || !right)
            {
                output.error = EvaluationError::invalid_argument;
                return output;
            }
            successful_queries += published.job_results[*left].status == 0 &&
                                  published.job_results[*right].status == 0;
        }
        std::uint64_t index_entries = 0;
        std::uint64_t index_bytes = 0;
        std::uint64_t preadmitted_bytes = 0;
        std::uint64_t preadmitted_term = 0;
        if (!checked_add(published.rings.size() * 2, published.regions.size() * 2 + 2,
                         index_entries) ||
            !checked_multiply(index_entries, kIndexBytes, index_bytes) ||
            !checked_add(preadmitted_bytes, index_bytes, preadmitted_bytes) ||
            !checked_multiply(request.relationship_queries.size(), kQueryPlanBytes,
                              preadmitted_term) ||
            !checked_add(preadmitted_bytes, preadmitted_term, preadmitted_bytes) ||
            !checked_multiply(successful_queries, kQueryKeyBytes, preadmitted_term) ||
            !checked_add(preadmitted_bytes, preadmitted_term, preadmitted_bytes) ||
            !checked_multiply(successful_queries, kCacheEntryBytes, preadmitted_term) ||
            !checked_add(preadmitted_bytes, preadmitted_term, preadmitted_bytes) ||
            !checked_multiply(request.relationship_queries.size(), kRelationshipRowBytes,
                              preadmitted_term) ||
            !checked_add(preadmitted_bytes, preadmitted_term, preadmitted_bytes) ||
            retained_memory_bytes > working_memory_limit ||
            preadmitted_bytes > working_memory_limit - retained_memory_bytes)
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        std::uint64_t ring_index_work = sort_units(published.rings.size());
        std::uint64_t ring_index_term = 0;
        if (!checked_multiply(published.rings.size(), 4, ring_index_term) ||
            !checked_add(ring_index_work, ring_index_term, ring_index_work) ||
            !checked_multiply(published.regions.size(), 2, ring_index_term) ||
            !checked_add(ring_index_work, ring_index_term, ring_index_work) ||
            !checked_add(output.telemetry.work_units, ring_index_work,
                         output.telemetry.work_units) ||
            output.telemetry.work_units > work_limit)
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        output.telemetry.peak_working_memory_bytes = retained_memory_bytes + index_bytes;
        const std::vector<std::uint32_t> ring_owners = ring_region_owners(published);
        if (ring_owners.size() != published.rings.size())
        {
            output.error = EvaluationError::invalid_argument;
            return output;
        }
        std::vector<std::uint32_t> region_ring_offsets(published.regions.size() + 1);
        for (const std::uint32_t owner : ring_owners)
        {
            if (owner >= published.regions.size())
            {
                output.error = EvaluationError::invalid_argument;
                return output;
            }
            ++region_ring_offsets[owner + 1];
        }
        for (std::size_t index = 1; index < region_ring_offsets.size(); ++index)
            region_ring_offsets[index] += region_ring_offsets[index - 1];
        std::vector<std::uint32_t> region_rings(published.rings.size());
        std::vector<std::uint32_t> ring_cursors = region_ring_offsets;
        for (std::uint32_t ring = 0; ring < ring_owners.size(); ++ring)
            region_rings[ring_cursors[ring_owners[ring]]++] = ring;
        const std::uint64_t maximum_relationship_rows =
            maximum_packet_bytes / kRelationshipRowBytes;
        if (request.relationship_queries.size() > maximum_relationship_rows)
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }

        std::vector<QueryPlan> plans;
        plans.reserve(request.relationship_queries.size());
        std::vector<std::pair<std::uint32_t, std::uint32_t>> unique_keys;
        unique_keys.reserve(static_cast<std::size_t>(successful_queries));
        for (const auto& query : request.relationship_queries)
        {
            const auto left = find_job_index(query.left_job_id);
            const auto right = find_job_index(query.right_job_id);
            if (!left || !right)
            {
                output.error = EvaluationError::invalid_argument;
                return output;
            }
            const auto& left_job = published.job_results[*left];
            const auto& right_job = published.job_results[*right];
            if (left_job.status != 0 || right_job.status != 0)
            {
                plans.push_back({*left, *right, std::numeric_limits<std::uint32_t>::max(), true});
                continue;
            }
            const auto key = std::pair{std::min(*left, *right), std::max(*left, *right)};
            plans.push_back({*left, *right, 0, false});
            unique_keys.push_back(key);
        }
        const std::uint64_t key_sort_work = sort_units(unique_keys.size()) + unique_keys.size() * 2;
        if (!checked_add(output.telemetry.work_units, key_sort_work, output.telemetry.work_units) ||
            output.telemetry.work_units > work_limit)
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        std::sort(unique_keys.begin(), unique_keys.end());
        unique_keys.erase(std::unique(unique_keys.begin(), unique_keys.end()), unique_keys.end());
        std::vector<CacheEntry> cache;
        cache.reserve(unique_keys.size());
        for (const auto key : unique_keys)
            cache.push_back({key.first, key.second, {}});
        std::uint64_t cache_lookup_work = 0;
        if (!checked_multiply(successful_queries, ceil_log2(unique_keys.size()) + 1,
                              cache_lookup_work) ||
            !checked_add(output.telemetry.work_units, cache_lookup_work,
                         output.telemetry.work_units) ||
            output.telemetry.work_units > work_limit)
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        for (QueryPlan& plan : plans)
            if (!plan.failed)
            {
                const auto key = std::pair{std::min(plan.left_job, plan.right_job),
                                           std::max(plan.left_job, plan.right_job)};
                plan.cache_index = static_cast<std::uint32_t>(
                    std::lower_bound(unique_keys.begin(), unique_keys.end(), key) -
                    unique_keys.begin());
            }

        std::uint64_t fixed_bytes = index_bytes;
        std::uint64_t term = 0;
        if (!checked_multiply(plans.size(), kQueryPlanBytes, term) ||
            !checked_add(fixed_bytes, term, fixed_bytes) ||
            !checked_multiply(successful_queries, kQueryKeyBytes, term) ||
            !checked_add(fixed_bytes, term, fixed_bytes) ||
            !checked_multiply(cache.size(), kCacheEntryBytes, term) ||
            !checked_add(fixed_bytes, term, fixed_bytes) ||
            !checked_multiply(request.relationship_queries.size(), kRelationshipRowBytes, term) ||
            !checked_add(fixed_bytes, term, fixed_bytes) ||
            retained_memory_bytes > working_memory_limit ||
            fixed_bytes > working_memory_limit - retained_memory_bytes)
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        output.telemetry.peak_working_memory_bytes = std::max(
            output.telemetry.peak_working_memory_bytes, retained_memory_bytes + fixed_bytes);

        std::uint64_t cached_pair_rows = 0;
        for (CacheEntry& entry : cache)
        {
            PairBuild built = evaluate_pair(
                published, region_ring_offsets, region_rings,
                published.job_results[entry.first_job], published.job_results[entry.second_job],
                entry.first_job == entry.second_job, per_pair_limits,
                work_limit - std::min(work_limit, output.telemetry.work_units),
                working_memory_limit,
                retained_memory_bytes + fixed_bytes + cached_pair_rows * kRelationshipRowBytes);
            if (!checked_add(output.telemetry.work_units, built.work,
                             output.telemetry.work_units) ||
                !checked_add(output.telemetry.candidate_pairs, built.candidates,
                             output.telemetry.candidate_pairs) ||
                !checked_add(output.telemetry.algebraic_fallback_calls,
                             built.algebraic_fallback_calls,
                             output.telemetry.algebraic_fallback_calls))
            {
                output.error = EvaluationError::resource_limit_exceeded;
                return output;
            }
            output.telemetry.peak_working_memory_bytes =
                std::max(output.telemetry.peak_working_memory_bytes,
                         retained_memory_bytes + fixed_bytes +
                             cached_pair_rows * kRelationshipRowBytes + built.peak_memory);
            output.telemetry.unresolved_predicate_failures +=
                built.unresolved_predicate_failure ? 1U : 0U;
            output.telemetry.required_working_memory_bytes =
                std::max(output.telemetry.required_working_memory_bytes, built.required_memory);
            if (built.error != EvaluationError::none)
            {
                output.error = built.error;
                return output;
            }
            entry.value = std::move(built.value);
            if (!checked_add(cached_pair_rows, entry.value.pairs.size(), cached_pair_rows) ||
                cached_pair_rows > std::numeric_limits<std::uint32_t>::max())
            {
                output.error = EvaluationError::resource_limit_exceeded;
                return output;
            }
        }

        std::uint64_t published_pair_rows = 0;
        for (const QueryPlan& plan : plans)
            if (!plan.failed &&
                !checked_add(published_pair_rows, cache[plan.cache_index].value.pairs.size(),
                             published_pair_rows))
            {
                output.error = EvaluationError::resource_limit_exceeded;
                return output;
            }
        std::uint64_t total_rows = 0;
        if (!checked_add(plans.size(), published_pair_rows, total_rows) ||
            total_rows > maximum_relationship_rows ||
            published_pair_rows > std::numeric_limits<std::uint32_t>::max())
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        std::uint64_t live_rows = 0;
        if (!checked_add(cached_pair_rows, published_pair_rows, live_rows) ||
            !checked_multiply(live_rows, kRelationshipRowBytes, term) ||
            fixed_bytes > working_memory_limit - retained_memory_bytes ||
            term > working_memory_limit - retained_memory_bytes - fixed_bytes)
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        output.telemetry.peak_working_memory_bytes = std::max(
            output.telemetry.peak_working_memory_bytes, retained_memory_bytes + fixed_bytes + term);
        std::uint64_t publication_work = 0;
        for (const QueryPlan& plan : plans)
        {
            std::uint64_t query_work = 1;
            if (!plan.failed)
            {
                const CacheEntry& entry = cache[plan.cache_index];
                const bool forward = plan.left_job == entry.first_job;
                std::uint64_t pair_work = 0;
                if (!checked_multiply(entry.value.pairs.size(),
                                      forward ? 1 : ceil_log2(entry.value.pairs.size()) + 2,
                                      pair_work) ||
                    !checked_add(query_work, pair_work, query_work))
                {
                    output.error = EvaluationError::resource_limit_exceeded;
                    return output;
                }
            }
            if (!checked_add(publication_work, query_work, publication_work))
            {
                output.error = EvaluationError::resource_limit_exceeded;
                return output;
            }
        }
        if (!checked_add(output.telemetry.work_units, publication_work,
                         output.telemetry.work_units) ||
            output.telemetry.work_units > work_limit)
        {
            output.error = EvaluationError::resource_limit_exceeded;
            return output;
        }
        output.results.reserve(plans.size());
        output.pairs.reserve(static_cast<std::size_t>(published_pair_rows));
        for (std::size_t query_index = 0; query_index < plans.size(); ++query_index)
        {
            const QueryPlan& plan = plans[query_index];
            const auto& query = request.relationship_queries[query_index];
            if (plan.failed)
            {
                output.results.push_back({query.query_id, 1, 0, 0, 0});
                continue;
            }
            const CacheEntry& entry = cache[plan.cache_index];
            const bool forward = plan.left_job == entry.first_job;
            const std::uint32_t begin =
                entry.value.pairs.empty() ? 0U : static_cast<std::uint32_t>(output.pairs.size());
            output.results.push_back({query.query_id, 0, entry.value.aggregate, begin,
                                      static_cast<std::uint32_t>(entry.value.pairs.size())});
            for (const auto& pair : entry.value.pairs)
                output.pairs.push_back(forward ? pair
                                               : AnalyticRelationshipPairRecord{
                                                     pair.right_result_region_id,
                                                     pair.left_result_region_id, pair.dimension,
                                                     pair.equality, pair.right_contains_left,
                                                     pair.left_contains_right});
            if (!forward)
                std::sort(
                    output.pairs.begin() + begin, output.pairs.end(),
                    [](const auto& left, const auto& right)
                    {
                        return std::tie(left.left_result_region_id, left.right_result_region_id,
                                        left.dimension, left.equality, left.left_contains_right,
                                        left.right_contains_left) <
                               std::tie(right.left_result_region_id, right.right_result_region_id,
                                        right.dimension, right.equality, right.left_contains_right,
                                        right.right_contains_left);
                    });
        }
        std::uint64_t row_count = 0;
        std::uint64_t row_bytes = 0;
        if (!checked_add(output.results.size(), output.pairs.size(), row_count) ||
            !checked_multiply(row_count, kRelationshipRowBytes, row_bytes))
        {
            output.error = EvaluationError::resource_limit_exceeded;
            output.results.clear();
            output.pairs.clear();
            return output;
        }
        if (output.telemetry.work_units > work_limit)
        {
            output.error = EvaluationError::resource_limit_exceeded;
            output.results.clear();
            output.pairs.clear();
        }
    }
    catch (const std::bad_alloc&)
    {
        output.error = EvaluationError::resource_limit_exceeded;
        output.results.clear();
        output.pairs.clear();
    }
    return output;
}

} // namespace geometer::analytic_relationship_detail

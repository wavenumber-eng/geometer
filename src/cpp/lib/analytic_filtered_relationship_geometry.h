#pragma once

// Internal relationship geometry materialization and coverage helpers.

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

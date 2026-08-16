#include "geometer/analytic_filtered_lowering.h"

#include "analytic_filtered_interval.h"
#include "analytic_wide_integer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numeric>
#include <tuple>
#include <utility>
#include <vector>

namespace geometer
{
namespace
{

using namespace analytic_detail;

constexpr std::uint64_t kMaximumSpanNm = 1'000'000'000'000;
constexpr std::uint64_t kLogicalBytesPerCurve = 768;

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
    // radical_coefficient / 2 * sqrt(dx^2 + dy^2). The radical coefficient is
    // zero for authored lines and constructed lines with an integral primitive
    // direction length.
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
    WideInteger radius_squared_times_four{};
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

struct TokenSlot
{
    std::uint32_t descriptor_index = 0;
    std::uint64_t token = 0;
    bool occupied = false;
};

static_assert(sizeof(AnalyticAtomicCurveNm) + sizeof(AnalyticCurveBoundsNm) +
                      sizeof(AnalyticFilteredOccurrence) + sizeof(TokenDescriptor) +
                      8 * sizeof(TokenSlot) <=
                  kLogicalBytesPerCurve,
              "filtered lowering storage exceeds its canonical per-curve charge");

struct SegmentGeometry
{
    bool is_arc = false;
    bool counterclockwise = true;
    bool major_arc = false;
    AnalyticIntegerPointNm start;
    AnalyticIntegerPointNm end;
    AnalyticIntegerPointNm center;
    WideInteger radius_squared{};
};

std::uint64_t ordered_key(std::int64_t value) noexcept
{
    return static_cast<std::uint64_t>(value) ^ (std::uint64_t{1} << 63U);
}

std::uint64_t span(std::int64_t minimum, std::int64_t maximum) noexcept
{
    return ordered_key(maximum) - ordered_key(minimum);
}

bool global_expansion_fits(std::int64_t center, std::uint64_t extent) noexcept
{
    const std::uint64_t key = ordered_key(center);
    return key >= extent && key <= std::numeric_limits<std::uint64_t>::max() - extent;
}

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

std::uint64_t hash_word(std::uint64_t state, std::uint64_t value) noexcept
{
    state ^= value + 0x9e3779b97f4a7c15ULL + (state << 6U) + (state >> 2U);
    state ^= state >> 30U;
    state *= 0xbf58476d1ce4e5b9ULL;
    state ^= state >> 27U;
    state *= 0x94d049bb133111ebULL;
    return state ^ (state >> 31U);
}

std::uint64_t hash_wide(std::uint64_t state, WideInteger value) noexcept
{
    return hash_word(hash_word(state, wide_low_bits(value)), wide_high_bits(value));
}

bool same_line_family(const LineFamilyKey& left, const LineFamilyKey& right) noexcept
{
    return left.dx == right.dx && left.dy == right.dy;
}

bool same_circle_family(const CircleFamilyKey& left, const CircleFamilyKey& right) noexcept
{
    return left.x == right.x && left.y == right.y;
}

bool same_family(const TokenDescriptor& left, const TokenDescriptor& right) noexcept
{
    if (left.kind != right.kind)
        return false;
    return left.kind == TokenKeyKind::line
               ? same_line_family(left.line.family, right.line.family)
               : same_circle_family(left.circle.family, right.circle.family);
}

bool same_carrier(const TokenDescriptor& left, const TokenDescriptor& right) noexcept
{
    if (!same_family(left, right))
        return false;
    if (left.kind == TokenKeyKind::line)
        return wide_compare(left.line.rational_part_times_two,
                            right.line.rational_part_times_two) == 0 &&
               left.line.radical_coefficient == right.line.radical_coefficient;
    return wide_compare(left.circle.radius_squared_times_four,
                        right.circle.radius_squared_times_four) == 0;
}

std::uint64_t token_hash(const TokenDescriptor& value, bool carrier) noexcept
{
    std::uint64_t state = hash_word(0x243f6a8885a308d3ULL, static_cast<std::uint8_t>(value.kind));
    if (value.kind == TokenKeyKind::line)
    {
        state = hash_word(state, static_cast<std::uint64_t>(value.line.family.dx));
        state = hash_word(state, static_cast<std::uint64_t>(value.line.family.dy));
        if (carrier)
        {
            state = hash_wide(state, value.line.rational_part_times_two);
            state = hash_word(state, static_cast<std::uint64_t>(value.line.radical_coefficient));
        }
        return state;
    }
    state = hash_word(state, static_cast<std::uint64_t>(value.circle.family.x));
    state = hash_word(state, static_cast<std::uint64_t>(value.circle.family.y));
    return carrier ? hash_wide(state, value.circle.radius_squared_times_four) : state;
}

std::size_t token_table_capacity(std::size_t count) noexcept
{
    std::size_t capacity = 1;
    while (capacity < count * 2)
        capacity *= 2;
    return capacity;
}

WideInteger squared_distance(AnalyticIntegerPointNm left, AnalyticIntegerPointNm right) noexcept
{
    const std::int64_t dx = left.x - right.x;
    const std::int64_t dy = left.y - right.y;
    return wide_add(wide_multiply(dx, dx), wide_multiply(dy, dy));
}

WideInteger cross_from(AnalyticIntegerPointNm origin, AnalyticIntegerPointNm left,
                       AnalyticIntegerPointNm right) noexcept
{
    const std::int64_t ax = left.x - origin.x;
    const std::int64_t ay = left.y - origin.y;
    const std::int64_t bx = right.x - origin.x;
    const std::int64_t by = right.y - origin.y;
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

class FilteredJobLowerer
{
  public:
    FilteredJobLowerer(const AnalyticRequestPacketRecords& records, AnalyticSolverLimits limits)
        : records_(records), limits_(limits)
    {
    }

    AnalyticFilteredLoweringResult lower(std::uint32_t job_index)
    {
        if (!analytic_solver_limits_within_hard_ceilings(limits_) ||
            job_index >= records_.jobs.size())
            return result(AnalyticFilteredLoweringError::resource_limit_exceeded);
        job_ = &records_.jobs[job_index];
        if (!preflight())
            return result(error_);
        if (projected_curves_ == 0)
            return {AnalyticFilteredLoweringError::none, std::move(out_), telemetry_};
        if (!choose_origin_and_validate_span())
            return result(error_);
        try
        {
            out_.curves.reserve(projected_curves_);
            out_.bounds.reserve(projected_curves_);
            out_.occurrences.reserve(projected_curves_);
            descriptors_.reserve(projected_curves_);
            if (!lower_operands())
                return result(error_);
            if (!assign_construction_tokens())
                return result(error_);
        }
        catch (const std::bad_alloc&)
        {
            return result(AnalyticFilteredLoweringError::resource_limit_exceeded);
        }
        return {AnalyticFilteredLoweringError::none, std::move(out_), telemetry_};
    }

  private:
    AnalyticFilteredLoweringResult result(AnalyticFilteredLoweringError error)
    {
        return {error, std::nullopt, telemetry_};
    }

    bool fail(AnalyticFilteredLoweringError error)
    {
        error_ = error;
        return false;
    }

    bool charge_work(std::uint64_t units = 1)
    {
        if (units > limits_.predicate_calls - telemetry_.work_units)
            return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
        telemetry_.work_units += units;
        return true;
    }

    bool charge_predicate()
    {
        if (!charge_work())
            return false;
        ++telemetry_.fixed_width_predicates;
        return true;
    }

    bool charge_stage_visit()
    {
        if (!charge_work())
            return false;
        ++telemetry_.stage_records_visited;
        return true;
    }

    bool charge_operand_visit()
    {
        if (!charge_work())
            return false;
        ++telemetry_.operand_records_visited;
        return true;
    }

    bool add_projected(std::uint64_t count)
    {
        if (count > limits_.boundary_occurrences - projected_curves_)
            return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
        projected_curves_ += count;
        return true;
    }

    bool preflight()
    {
        if (job_->stage_count != 0)
            operand_begin_ = records_.stages[job_->stage_begin].operand_begin;
        for (std::uint32_t stage_offset = 0; stage_offset < job_->stage_count; ++stage_offset)
        {
            if (!charge_stage_visit())
                return false;
            const AnalyticRequestStageRecord& stage =
                records_.stages[job_->stage_begin + stage_offset];
            operand_end_ = stage.operand_begin + stage.operand_count;
        }
        for (std::uint32_t operand_index = operand_begin_; operand_index < operand_end_;
             ++operand_index)
        {
            if (!charge_operand_visit())
                return false;
            const AnalyticRequestOperandRecord& operand = records_.operands[operand_index];
            ++telemetry_.input_operands;
            switch (operand.geometry_kind)
            {
            case 1:
            {
                const AnalyticRequestPlanarRegionRecord& region =
                    records_.planar_regions[operand.geometry_index];
                if (!preflight_ring(records_.rings[region.outer_ring]))
                    return false;
                for (std::uint32_t index = 0; index < region.hole_reference_count; ++index)
                    if (!preflight_ring(
                            records_.rings[records_.ring_references[region.hole_reference_begin +
                                                                    index]]))
                        return false;
                break;
            }
            case 2:
                if (!add_projected(2))
                    return false;
                break;
            case 3:
                if (!add_projected(4))
                    return false;
                break;
            case 4:
                if (!add_projected(4))
                    return false;
                break;
            case 5:
                return fail(AnalyticFilteredLoweringError::unsupported_geometry);
            default:
                return fail(AnalyticFilteredLoweringError::unsupported_geometry);
            }
            if (!scan_operand(operand))
                return false;
        }
        if (projected_curves_ > std::numeric_limits<std::uint64_t>::max() / kLogicalBytesPerCurve)
            return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
        const std::uint64_t bytes = projected_curves_ * kLogicalBytesPerCurve;
        if (bytes > limits_.working_memory_bytes)
            return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
        telemetry_.peak_working_memory_bytes = bytes;
        return true;
    }

    bool preflight_ring(const AnalyticRequestRingRecord& ring)
    {
        if (!charge_work())
            return false;
        telemetry_.input_segments += ring.segment_count;
        return add_projected(ring.segment_count);
    }

    void include_global(std::int64_t x, std::int64_t y)
    {
        if (!has_global_bounds_)
        {
            min_global_x_ = max_global_x_ = x;
            min_global_y_ = max_global_y_ = y;
            has_global_bounds_ = true;
            return;
        }
        min_global_x_ = std::min(min_global_x_, x);
        max_global_x_ = std::max(max_global_x_, x);
        min_global_y_ = std::min(min_global_y_, y);
        max_global_y_ = std::max(max_global_y_, y);
    }

    bool scan_ring(const AnalyticRequestRingRecord& ring)
    {
        if (!charge_work(ring.vertex_count))
            return false;
        for (std::uint32_t index = 0; index < ring.vertex_count; ++index)
        {
            const AnalyticRequestVertexRecord& vertex =
                records_.vertices[ring.vertex_begin + index];
            include_global(vertex.x_nm, vertex.y_nm);
            const AnalyticRequestSegmentRecord& segment =
                records_.segments[ring.segment_begin + index];
            if (segment.kind == 2)
                include_global(segment.center_x_nm, segment.center_y_nm);
        }
        return true;
    }

    bool scan_operand(const AnalyticRequestOperandRecord& operand)
    {
        switch (operand.geometry_kind)
        {
        case 1:
        {
            const AnalyticRequestPlanarRegionRecord& region =
                records_.planar_regions[operand.geometry_index];
            if (!scan_ring(records_.rings[region.outer_ring]))
                return false;
            for (std::uint32_t index = 0; index < region.hole_reference_count; ++index)
                if (!scan_ring(
                        records_
                            .rings[records_.ring_references[region.hole_reference_begin + index]]))
                    return false;
            break;
        }
        case 2:
        {
            const auto& disk = records_.disks[operand.geometry_index];
            include_global(disk.center_x_nm, disk.center_y_nm);
            break;
        }
        case 3:
        {
            const auto& annulus = records_.annuli[operand.geometry_index];
            include_global(annulus.center_x_nm, annulus.center_y_nm);
            break;
        }
        case 4:
        {
            const auto& capsule = records_.capsules[operand.geometry_index];
            include_global(capsule.start_x_nm, capsule.start_y_nm);
            include_global(capsule.end_x_nm, capsule.end_y_nm);
            break;
        }
        default:
            break;
        }
        return true;
    }

    bool local_coordinate(std::int64_t value, std::int64_t origin, std::int64_t& output) const
    {
        if (value < origin)
            return false;
        const std::uint64_t delta = ordered_key(value) - ordered_key(origin);
        if (delta > kMaximumSpanNm)
            return false;
        output = static_cast<std::int64_t>(delta);
        return true;
    }

    bool local_point(std::int64_t x, std::int64_t y, AnalyticIntegerPointNm& output) const
    {
        return local_coordinate(x, out_.origin_x_nm, output.x) &&
               local_coordinate(y, out_.origin_y_nm, output.y);
    }

    void include_extent_doubled(std::int64_t x, std::int64_t y)
    {
        if (!has_extent_)
        {
            min_extent_x_ = max_extent_x_ = x;
            min_extent_y_ = max_extent_y_ = y;
            has_extent_ = true;
            return;
        }
        min_extent_x_ = std::min(min_extent_x_, x);
        max_extent_x_ = std::max(max_extent_x_, x);
        min_extent_y_ = std::min(min_extent_y_, y);
        max_extent_y_ = std::max(max_extent_y_, y);
    }

    bool include_core_extent(std::int64_t x, std::int64_t y)
    {
        AnalyticIntegerPointNm local;
        if (!local_point(x, y, local))
            return false;
        include_extent_doubled(local.x * 2, local.y * 2);
        return true;
    }

    bool validate_ring_extent(const AnalyticRequestRingRecord& ring)
    {
        if (!charge_work(ring.vertex_count))
            return false;
        for (std::uint32_t index = 0; index < ring.vertex_count; ++index)
        {
            const auto& vertex = records_.vertices[ring.vertex_begin + index];
            if (!include_core_extent(vertex.x_nm, vertex.y_nm))
                return false;
            const auto& segment = records_.segments[ring.segment_begin + index];
            if (segment.kind == 2 && !include_core_extent(segment.center_x_nm, segment.center_y_nm))
                return false;
        }
        return true;
    }

    bool validate_operand_extent(const AnalyticRequestOperandRecord& operand)
    {
        if (operand.geometry_kind == 1)
        {
            const auto& region = records_.planar_regions[operand.geometry_index];
            if (!validate_ring_extent(records_.rings[region.outer_ring]))
                return false;
            for (std::uint32_t index = 0; index < region.hole_reference_count; ++index)
                if (!validate_ring_extent(
                        records_
                            .rings[records_.ring_references[region.hole_reference_begin + index]]))
                    return false;
            return true;
        }
        std::int64_t x = 0;
        std::int64_t y = 0;
        std::uint64_t diameter = 0;
        if (operand.geometry_kind == 2)
        {
            const auto& value = records_.disks[operand.geometry_index];
            if (!global_expansion_fits(value.center_x_nm, value.radius_nm) ||
                !global_expansion_fits(value.center_y_nm, value.radius_nm))
                return false;
            if (!local_coordinate(value.center_x_nm, out_.origin_x_nm, x) ||
                !local_coordinate(value.center_y_nm, out_.origin_y_nm, y))
                return false;
            diameter = value.radius_nm * 2;
            include_extent_doubled(x * 2 - static_cast<std::int64_t>(diameter),
                                   y * 2 - static_cast<std::int64_t>(diameter));
            include_extent_doubled(x * 2 + static_cast<std::int64_t>(diameter),
                                   y * 2 + static_cast<std::int64_t>(diameter));
            return true;
        }
        if (operand.geometry_kind == 3)
        {
            const auto& value = records_.annuli[operand.geometry_index];
            if (!global_expansion_fits(value.center_x_nm, value.outer_radius_nm) ||
                !global_expansion_fits(value.center_y_nm, value.outer_radius_nm))
                return false;
            if (!local_coordinate(value.center_x_nm, out_.origin_x_nm, x) ||
                !local_coordinate(value.center_y_nm, out_.origin_y_nm, y))
                return false;
            diameter = value.outer_radius_nm * 2;
            include_extent_doubled(x * 2 - static_cast<std::int64_t>(diameter),
                                   y * 2 - static_cast<std::int64_t>(diameter));
            include_extent_doubled(x * 2 + static_cast<std::int64_t>(diameter),
                                   y * 2 + static_cast<std::int64_t>(diameter));
            return true;
        }
        const auto& value = records_.capsules[operand.geometry_index];
        const std::uint64_t half_width_ceiling = value.width_nm / 2 + value.width_nm % 2;
        if (!global_expansion_fits(value.start_x_nm, half_width_ceiling) ||
            !global_expansion_fits(value.start_y_nm, half_width_ceiling) ||
            !global_expansion_fits(value.end_x_nm, half_width_ceiling) ||
            !global_expansion_fits(value.end_y_nm, half_width_ceiling))
            return false;
        AnalyticIntegerPointNm start;
        AnalyticIntegerPointNm end;
        if (!local_point(value.start_x_nm, value.start_y_nm, start) ||
            !local_point(value.end_x_nm, value.end_y_nm, end))
            return false;
        const std::int64_t width = static_cast<std::int64_t>(value.width_nm);
        include_extent_doubled(start.x * 2 - width, start.y * 2 - width);
        include_extent_doubled(start.x * 2 + width, start.y * 2 + width);
        include_extent_doubled(end.x * 2 - width, end.y * 2 - width);
        include_extent_doubled(end.x * 2 + width, end.y * 2 + width);
        return true;
    }

    bool choose_origin_and_validate_span()
    {
        if (!has_global_bounds_ || span(min_global_x_, max_global_x_) > kMaximumSpanNm ||
            span(min_global_y_, max_global_y_) > kMaximumSpanNm)
            return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
        out_.origin_x_nm = min_global_x_;
        out_.origin_y_nm = min_global_y_;
        bool valid_extents = true;
        if (!for_each_operand(
                [&](const AnalyticRequestOperandRecord& operand)
                {
                    valid_extents = validate_operand_extent(operand);
                    return valid_extents;
                }))
            return error_ == AnalyticFilteredLoweringError::none
                       ? fail(AnalyticFilteredLoweringError::resource_limit_exceeded)
                       : false;
        if (!valid_extents || !has_extent_ || max_extent_x_ - min_extent_x_ > 2'000'000'000'000LL ||
            max_extent_y_ - min_extent_y_ > 2'000'000'000'000LL)
            return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
        return true;
    }

    template <typename Function> bool for_each_operand(Function&& function)
    {
        for (std::uint32_t operand_index = operand_begin_; operand_index < operand_end_;
             ++operand_index)
        {
            if (!charge_operand_visit() || !function(records_.operands[operand_index]))
                return false;
        }
        return true;
    }

    bool charge_token_probe()
    {
        if (!charge_work())
            return false;
        ++telemetry_.token_table_probes;
        ++telemetry_.fixed_width_predicates;
        return true;
    }

    bool intern_token(std::vector<TokenSlot>& table, std::uint32_t descriptor_index, bool carrier,
                      std::uint64_t& next_token, std::uint64_t& token)
    {
        const std::size_t mask = table.size() - 1;
        std::size_t slot =
            static_cast<std::size_t>(token_hash(descriptors_[descriptor_index], carrier)) & mask;
        for (std::size_t probe = 0; probe < table.size(); ++probe)
        {
            if (!charge_token_probe())
                return false;
            TokenSlot& candidate = table[slot];
            if (!candidate.occupied)
            {
                candidate = {descriptor_index, next_token++, true};
                token = candidate.token;
                return true;
            }
            const TokenDescriptor& existing = descriptors_[candidate.descriptor_index];
            const bool equal = carrier ? same_carrier(existing, descriptors_[descriptor_index])
                                       : same_family(existing, descriptors_[descriptor_index]);
            if (equal)
            {
                token = candidate.token;
                return true;
            }
            slot = (slot + 1) & mask;
        }
        return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
    }

    bool assign_construction_tokens()
    {
        if (descriptors_.size() != out_.curves.size())
            return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
        const std::size_t capacity = token_table_capacity(descriptors_.size());
        std::vector<TokenSlot> family_table(capacity);
        std::vector<TokenSlot> carrier_table(capacity);
        std::uint64_t next_family = 1;
        std::uint64_t next_carrier = 1;
        for (std::uint32_t index = 0; index < descriptors_.size(); ++index)
        {
            if (descriptors_[index].kind == TokenKeyKind::line)
            {
                out_.curves[index].has_construction_line_direction = true;
                out_.curves[index].construction_line_dx = descriptors_[index].line.family.dx;
                out_.curves[index].construction_line_dy = descriptors_[index].line.family.dy;
            }
            if (!intern_token(family_table, index, false, next_family,
                              out_.curves[index].construction_family_id) ||
                !intern_token(carrier_table, index, true, next_carrier,
                              out_.curves[index].construction_carrier_id))
                return false;
            if (out_.curves[index].kind == AnalyticAtomicCurveKind::line &&
                out_.curves[index].has_construction_line_direction &&
                out_.curves[index].construction_line_dx == 0)
            {
                const std::uint64_t column =
                    analytic_vertical_x_column_token(out_.curves[index].construction_carrier_id);
                if (column == 0)
                    return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
                out_.curves[index].start.construction_x_column_id = column;
                out_.curves[index].end.construction_x_column_id = column;
            }
        }
        return true;
    }

    TokenDescriptor exact_line_descriptor(const AnalyticAtomicCurveNm& curve) const
    {
        const std::int64_t dx = curve.integer_end.x - curve.integer_start.x;
        const std::int64_t dy = curve.integer_end.y - curve.integer_start.y;
        const LineFamilyKey family = canonical_direction(dx, dy);
        const WideInteger offset = wide_subtract(wide_multiply(family.dx, curve.integer_start.y),
                                                 wide_multiply(family.dy, curve.integer_start.x));
        TokenDescriptor descriptor;
        descriptor.line = {family, wide_add(offset, offset), 0};
        return descriptor;
    }

    TokenDescriptor circle_descriptor(AnalyticIntegerPointNm center,
                                      WideInteger radius_squared_times_four) const
    {
        TokenDescriptor descriptor;
        descriptor.kind = TokenKeyKind::circle;
        descriptor.circle = {{center.x, center.y}, radius_squared_times_four};
        return descriptor;
    }

    bool cardinal_may_be_on_arc(const AnalyticAtomicCurveNm& curve, Point radial)
    {
        if (!charge_predicate())
            return true;
        const Point center{{curve.circle.center.x.lower, curve.circle.center.x.upper},
                           {curve.circle.center.y.lower, curve.circle.center.y.upper}};
        const Point start = subtract({{curve.start.x.lower, curve.start.x.upper},
                                      {curve.start.y.lower, curve.start.y.upper}},
                                     center);
        const Point end = subtract(
            {{curve.end.x.lower, curve.end.x.upper}, {curve.end.y.lower, curve.end.y.upper}},
            center);
        Interval from_start = cross(start, radial);
        Interval to_end = cross(radial, end);
        if (!curve.counterclockwise)
        {
            from_start = negate(from_start);
            to_end = negate(to_end);
        }
        if (!curve.major_arc)
            return from_start.upper >= 0.0 && to_end.upper >= 0.0;
        return from_start.upper >= 0.0 || to_end.upper >= 0.0;
    }

    bool bounds_for(const AnalyticAtomicCurveNm& curve, AnalyticCurveBoundsNm& bounds)
    {
        bounds.curve_index = curve.curve_index;
        bounds.min_x = std::min(curve.start.x.lower, curve.end.x.lower);
        bounds.min_y = std::min(curve.start.y.lower, curve.end.y.lower);
        bounds.max_x = std::max(curve.start.x.upper, curve.end.x.upper);
        bounds.max_y = std::max(curve.start.y.upper, curve.end.y.upper);
        if (curve.kind == AnalyticAtomicCurveKind::line)
            return true;
        const Interval center_x{curve.circle.center.x.lower, curve.circle.center.x.upper};
        const Interval center_y{curve.circle.center.y.lower, curve.circle.center.y.upper};
        const Interval radius{curve.circle.radius.lower, curve.circle.radius.upper};
        const auto include = [&](Interval x, Interval y)
        {
            bounds.min_x = std::min(bounds.min_x, x.lower);
            bounds.min_y = std::min(bounds.min_y, y.lower);
            bounds.max_x = std::max(bounds.max_x, x.upper);
            bounds.max_y = std::max(bounds.max_y, y.upper);
        };
        if (cardinal_may_be_on_arc(curve, {exact(1.0), exact(0.0)}))
            include(add(center_x, radius), center_y);
        if (cardinal_may_be_on_arc(curve, {exact(-1.0), exact(0.0)}))
            include(subtract(center_x, radius), center_y);
        if (cardinal_may_be_on_arc(curve, {exact(0.0), exact(1.0)}))
            include(center_x, add(center_y, radius));
        if (cardinal_may_be_on_arc(curve, {exact(0.0), exact(-1.0)}))
            include(center_x, subtract(center_y, radius));
        return error_ == AnalyticFilteredLoweringError::none;
    }

    bool emit(AnalyticAtomicCurveNm curve, bool agrees_with_carrier, bool material_on_left,
              std::uint64_t coverage_id, AnalyticFilteredSourceReference source,
              TokenDescriptor descriptor)
    {
        if (out_.curves.size() >= projected_curves_)
            return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
        curve.curve_index = static_cast<std::uint32_t>(out_.curves.size() + 1);
        AnalyticCurveBoundsNm bounds;
        if (!bounds_for(curve, bounds))
            return false;
        if (!has_output_bounds_)
        {
            min_output_x_ = bounds.min_x;
            min_output_y_ = bounds.min_y;
            max_output_x_ = bounds.max_x;
            max_output_y_ = bounds.max_y;
            has_output_bounds_ = true;
        }
        else
        {
            min_output_x_ = std::min(min_output_x_, bounds.min_x);
            min_output_y_ = std::min(min_output_y_, bounds.min_y);
            max_output_x_ = std::max(max_output_x_, bounds.max_x);
            max_output_y_ = std::max(max_output_y_, bounds.max_y);
        }
        if (!output_bounds_within_span())
            return false;
        out_.bounds.push_back(bounds);
        out_.curves.push_back(std::move(curve));
        descriptors_.push_back(std::move(descriptor));
        out_.occurrences.push_back({static_cast<std::uint64_t>(out_.curves.size()), coverage_id,
                                    agrees_with_carrier, material_on_left, source});
        ++telemetry_.emitted_curves;
        return true;
    }

    bool output_bounds_within_span()
    {
        if (!has_output_bounds_ || !std::isfinite(min_output_x_) || !std::isfinite(min_output_y_) ||
            !std::isfinite(max_output_x_) || !std::isfinite(max_output_y_) ||
            max_output_x_ - min_output_x_ > kMaximumSpanNm ||
            max_output_y_ - min_output_y_ > kMaximumSpanNm)
            return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
        return true;
    }

    Interval radius_interval(AnalyticIntegerPointNm endpoint, AnalyticIntegerPointNm center)
    {
        ++telemetry_.square_root_calls;
        const Point radial = subtract(point(endpoint), point(center));
        return square_root(dot(radial, radial));
    }

    bool integer_square_root(WideInteger squared, double approximation,
                             std::uint64_t maximum_candidate, std::uint64_t& root)
    {
        const std::uint64_t center = static_cast<std::uint64_t>(approximation);
        const std::uint64_t first = center > 4 ? center - 4 : 0;
        for (std::uint64_t candidate = first; candidate <= center + 4; ++candidate)
        {
            if (!charge_predicate())
                return false;
            if (candidate <= maximum_candidate &&
                wide_compare(wide_multiply(static_cast<std::int64_t>(candidate),
                                           static_cast<std::int64_t>(candidate)),
                             squared) == 0)
            {
                root = candidate;
                return true;
            }
        }
        return false;
    }

    bool integer_radius(WideInteger squared, double approximation, std::uint64_t& radius)
    {
        return integer_square_root(squared, approximation, kMaximumSpanNm, radius);
    }

    bool validate_segment(const AnalyticRequestRingRecord& ring, std::uint32_t index,
                          SegmentGeometry& segment)
    {
        const auto& start = records_.vertices[ring.vertex_begin + index];
        const auto& end = records_.vertices[ring.vertex_begin + (index + 1) % ring.vertex_count];
        const auto& record = records_.segments[ring.segment_begin + index];
        if (!local_point(start.x_nm, start.y_nm, segment.start) ||
            !local_point(end.x_nm, end.y_nm, segment.end))
            return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
        if (segment.start.x == segment.end.x && segment.start.y == segment.end.y)
            return fail(AnalyticFilteredLoweringError::invalid_topology);
        if (record.kind != 2)
            return true;
        segment.is_arc = true;
        segment.counterclockwise = record.direction == 1;
        segment.major_arc = record.major_arc;
        if (!local_point(record.center_x_nm, record.center_y_nm, segment.center))
            return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
        segment.radius_squared = squared_distance(segment.start, segment.center);
        if (!charge_predicate())
            return false;
        if (wide_sign(segment.radius_squared) == 0 ||
            wide_compare(segment.radius_squared, squared_distance(segment.end, segment.center)) !=
                0)
            return fail(AnalyticFilteredLoweringError::invalid_arc);
        if (!charge_predicate())
            return false;
        const int turn = wide_sign(cross_from(segment.center, segment.start, segment.end));
        const bool derived_major = segment.counterclockwise ? turn < 0 : turn > 0;
        if (segment.major_arc != derived_major)
            return fail(AnalyticFilteredLoweringError::invalid_arc);
        return true;
    }

    bool arc_contains_leftmost(const SegmentGeometry& segment)
    {
        std::int64_t sx = segment.start.x - segment.center.x;
        std::int64_t sy = segment.start.y - segment.center.y;
        std::int64_t ex = segment.end.x - segment.center.x;
        std::int64_t ey = segment.end.y - segment.center.y;
        if (!segment.counterclockwise)
        {
            std::swap(sx, ex);
            std::swap(sy, ey);
        }
        if (!charge_predicate())
            return false;
        const int turn = wide_sign(wide_subtract(wide_multiply(sx, ey), wide_multiply(sy, ex)));
        if (turn > 0)
            return sy > 0 && ey < 0;
        if (turn == 0)
            return sy > 0;
        const bool complement_interior = ey > 0 && sy < 0;
        const bool at_start = sy == 0 && sx < 0;
        const bool at_end = ey == 0 && ex < 0;
        return !complement_interior && !at_start && !at_end;
    }

    void tangent(const SegmentGeometry& segment, bool at_start, std::int64_t& x,
                 std::int64_t& y) const
    {
        if (!segment.is_arc)
        {
            x = segment.end.x - segment.start.x;
            y = segment.end.y - segment.start.y;
            return;
        }
        const AnalyticIntegerPointNm endpoint = at_start ? segment.start : segment.end;
        const std::int64_t radial_x = endpoint.x - segment.center.x;
        const std::int64_t radial_y = endpoint.y - segment.center.y;
        if (segment.counterclockwise)
        {
            x = -radial_y;
            y = radial_x;
        }
        else
        {
            x = radial_y;
            y = -radial_x;
        }
    }

    bool vertex_orientation(const AnalyticRequestRingRecord& ring, std::uint32_t vertex,
                            bool& counterclockwise)
    {
        SegmentGeometry outgoing;
        SegmentGeometry incoming;
        if (!validate_segment(ring, vertex, outgoing) ||
            !validate_segment(ring, (vertex + ring.segment_count - 1) % ring.segment_count,
                              incoming))
            return false;
        std::int64_t out_x = 0;
        std::int64_t out_y = 0;
        std::int64_t in_x = 0;
        std::int64_t in_y = 0;
        tangent(outgoing, true, out_x, out_y);
        tangent(incoming, false, in_x, in_y);
        if (!charge_predicate())
            return false;
        const int turn =
            wide_sign(wide_subtract(wide_multiply(in_x, out_y), wide_multiply(in_y, out_x)));
        if (turn != 0)
        {
            counterclockwise = turn > 0;
            return true;
        }
        if (!charge_predicate())
            return false;
        if (wide_sign(dot_vectors(in_x, in_y, out_x, out_y)) <= 0 || out_y == 0)
            return fail(AnalyticFilteredLoweringError::invalid_topology);
        counterclockwise = out_y < 0;
        return true;
    }

    bool resolve_ring_orientation(const AnalyticRequestRingRecord& ring, bool& counterclockwise)
    {
        if (!charge_work(ring.vertex_count))
            return false;
        std::uint32_t best_vertex = 0;
        for (std::uint32_t index = 1; index < ring.vertex_count; ++index)
        {
            const auto& candidate = records_.vertices[ring.vertex_begin + index];
            const auto& best = records_.vertices[ring.vertex_begin + best_vertex];
            if (std::tie(candidate.x_nm, candidate.y_nm) < std::tie(best.x_nm, best.y_nm))
                best_vertex = index;
        }
        const auto& best_record = records_.vertices[ring.vertex_begin + best_vertex];
        AnalyticIntegerPointNm best_point;
        if (!local_point(best_record.x_nm, best_record.y_nm, best_point))
            return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
        Interval best_x = exact(static_cast<double>(best_point.x));
        std::int64_t best_y = best_point.y;
        bool best_is_arc = false;
        bool best_arc_ccw = false;
        AnalyticIntegerPointNm best_center{};
        WideInteger best_squared{};

        if (!charge_work(ring.segment_count))
            return false;
        for (std::uint32_t index = 0; index < ring.segment_count; ++index)
        {
            SegmentGeometry segment;
            if (!validate_segment(ring, index, segment))
                return false;
            if (!segment.is_arc || !arc_contains_leftmost(segment))
            {
                if (error_ != AnalyticFilteredLoweringError::none)
                    return false;
                continue;
            }
            const Interval candidate_x = subtract(exact(static_cast<double>(segment.center.x)),
                                                  radius_interval(segment.start, segment.center));
            bool choose = candidate_x.upper < best_x.lower;
            if (!choose && !(best_x.upper < candidate_x.lower))
            {
                bool same_x = false;
                if (!best_is_arc)
                {
                    const std::int64_t delta = segment.center.x - best_point.x;
                    if (delta >= 0 && charge_predicate())
                        same_x =
                            wide_compare(segment.radius_squared, wide_multiply(delta, delta)) == 0;
                }
                else if (segment.center.x == best_center.x && charge_predicate())
                    same_x = wide_compare(segment.radius_squared, best_squared) == 0;
                if (error_ != AnalyticFilteredLoweringError::none)
                    return false;
                if (!same_x)
                    return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
                if (segment.center.y < best_y)
                    choose = true;
                else if (segment.center.y == best_y && best_is_arc &&
                         segment.counterclockwise != best_arc_ccw)
                    return fail(AnalyticFilteredLoweringError::invalid_topology);
            }
            if (choose)
            {
                best_is_arc = true;
                best_arc_ccw = segment.counterclockwise;
                best_center = segment.center;
                best_squared = segment.radius_squared;
                best_x = candidate_x;
                best_y = segment.center.y;
            }
        }
        if (best_is_arc)
        {
            counterclockwise = best_arc_ccw;
            return true;
        }
        return vertex_orientation(ring, best_vertex, counterclockwise);
    }

    bool lower_authored_ring(const AnalyticRequestRingRecord& ring, bool hole,
                             std::uint64_t coverage_id)
    {
        bool ring_ccw = false;
        if (!resolve_ring_orientation(ring, ring_ccw))
            return false;
        const bool material_on_left = hole ? !ring_ccw : ring_ccw;
        if (!charge_work(ring.segment_count))
            return false;
        for (std::uint32_t index = 0; index < ring.segment_count; ++index)
        {
            SegmentGeometry segment;
            if (!validate_segment(ring, index, segment))
                return false;
            const auto& record = records_.segments[ring.segment_begin + index];
            AnalyticAtomicCurveNm curve;
            curve.start = public_point(point(segment.start));
            curve.end = public_point(point(segment.end));
            curve.has_integer_certificate = true;
            curve.integer_start = segment.start;
            curve.integer_end = segment.end;
            bool agrees = segment.end.x != segment.start.x ? segment.end.x > segment.start.x
                                                           : segment.end.y > segment.start.y;
            AnalyticFilteredSourceRole role = AnalyticFilteredSourceRole::authored_line;
            TokenDescriptor descriptor;
            if (segment.is_arc)
            {
                curve.kind = AnalyticAtomicCurveKind::circular_arc;
                curve.counterclockwise = segment.counterclockwise;
                curve.major_arc = segment.major_arc;
                curve.circle.center = public_point(point(segment.center));
                Interval radius = radius_interval(segment.start, segment.center);
                curve.integer_center = segment.center;
                std::uint64_t exact_radius = 0;
                if (!integer_radius(segment.radius_squared, radius.upper, exact_radius) &&
                    error_ != AnalyticFilteredLoweringError::none)
                    return false;
                if (exact_radius != 0)
                {
                    curve.has_integer_radius_certificate = true;
                    curve.integer_radius = exact_radius;
                    radius = exact(static_cast<double>(exact_radius));
                }
                if (!valid(radius) || radius.lower <= 0.0 || radius.upper > kMaximumSpanNm)
                    return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
                const std::uint64_t radius_ceiling =
                    static_cast<std::uint64_t>(std::ceil(radius.upper));
                if (!global_expansion_fits(record.center_x_nm, radius_ceiling) ||
                    !global_expansion_fits(record.center_y_nm, radius_ceiling))
                    return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
                curve.circle.radius = public_interval(radius);
                descriptor = circle_descriptor(
                    segment.center,
                    wide_add(wide_add(segment.radius_squared, segment.radius_squared),
                             wide_add(segment.radius_squared, segment.radius_squared)));
                agrees = segment.counterclockwise;
                role = AnalyticFilteredSourceRole::authored_circular_arc;
            }
            else
                descriptor = exact_line_descriptor(curve);
            if (!emit(std::move(curve), agrees, material_on_left, coverage_id,
                      {AnalyticFilteredSourceKind::authored_segment_curve, role, coverage_id,
                       record.id, record.curve_id},
                      std::move(descriptor)))
                return false;
        }
        return true;
    }

    bool lower_full_circle(std::int64_t global_x, std::int64_t global_y, std::uint64_t radius,
                           bool material_inside, std::uint64_t coverage_id,
                           AnalyticFilteredSourceReference source)
    {
        AnalyticIntegerPointNm center;
        if (!local_point(global_x, global_y, center))
            return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
        const std::int64_t signed_radius = static_cast<std::int64_t>(radius);
        const AnalyticIntegerPointNm left{center.x - signed_radius, center.y};
        const AnalyticIntegerPointNm right{center.x + signed_radius, center.y};
        const WideInteger squared = wide_multiply(signed_radius, signed_radius);
        const WideInteger squared_times_four =
            wide_add(wide_add(squared, squared), wide_add(squared, squared));
        const auto make_half = [&](AnalyticIntegerPointNm start, AnalyticIntegerPointNm end)
        {
            AnalyticAtomicCurveNm curve;
            curve.kind = AnalyticAtomicCurveKind::circular_arc;
            curve.start = public_point(point(start));
            curve.end = public_point(point(end));
            curve.circle.center = public_point(point(center));
            curve.circle.radius = {static_cast<double>(radius), static_cast<double>(radius)};
            curve.counterclockwise = true;
            curve.has_integer_certificate = true;
            curve.integer_start = start;
            curve.integer_end = end;
            curve.integer_center = center;
            curve.has_integer_radius_certificate = true;
            curve.integer_radius = radius;
            return curve;
        };
        const TokenDescriptor descriptor = circle_descriptor(center, squared_times_four);
        return emit(make_half(left, right), true, material_inside, coverage_id, source,
                    descriptor) &&
               emit(make_half(right, left), true, material_inside, coverage_id, source, descriptor);
    }

    bool lower_capsule(const AnalyticRequestOperandRecord& operand)
    {
        const auto& capsule = records_.capsules[operand.geometry_index];
        AnalyticIntegerPointNm start;
        AnalyticIntegerPointNm end;
        if (!local_point(capsule.start_x_nm, capsule.start_y_nm, start) ||
            !local_point(capsule.end_x_nm, capsule.end_y_nm, end))
            return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
        if (start.x == end.x && start.y == end.y)
            return fail(AnalyticFilteredLoweringError::invalid_topology);
        const std::int64_t dx = end.x - start.x;
        const std::int64_t dy = end.y - start.y;
        ++telemetry_.square_root_calls;
        const Interval length = square_root(
            add(square(exact(static_cast<double>(dx))), square(exact(static_cast<double>(dy)))));
        if (!valid(length) || length.lower <= 0.0)
            return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
        const Interval half_width = exact(static_cast<double>(capsule.width_nm) * 0.5);
        const Interval factor = divide(half_width, length);
        const Point offset{multiply(exact(static_cast<double>(-dy)), factor),
                           multiply(exact(static_cast<double>(dx)), factor)};
        const Point start_point = point(start);
        const Point end_point = point(end);
        const Point start_left = add(start_point, offset);
        const Point start_right = subtract(start_point, offset);
        const Point end_left = add(end_point, offset);
        const Point end_right = subtract(end_point, offset);
        const LineFamilyKey line_family = canonical_direction(dx, dy);
        const WideInteger width_squared =
            wide_multiply(static_cast<std::int64_t>(capsule.width_nm),
                          static_cast<std::int64_t>(capsule.width_nm));
        const bool forward_agrees = dx != 0 ? dx > 0 : dy > 0;
        const WideInteger base = wide_subtract(wide_multiply(line_family.dx, start.y),
                                               wide_multiply(line_family.dy, start.x));
        const WideInteger base_times_two = wide_add(base, base);
        const WideInteger primitive_squared =
            wide_add(wide_multiply(line_family.dx, line_family.dx),
                     wide_multiply(line_family.dy, line_family.dy));
        std::uint64_t primitive_length = 0;
        const double primitive_approximation =
            std::hypot(static_cast<double>(line_family.dx), static_cast<double>(line_family.dy));
        const bool integral_primitive_length = integer_square_root(
            primitive_squared, primitive_approximation,
            static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()), primitive_length);
        if (error_ != AnalyticFilteredLoweringError::none)
            return false;
        const auto line_descriptor = [&](bool original_left)
        {
            const bool canonical_left = original_left == forward_agrees;
            const std::int64_t sign = canonical_left ? 1 : -1;
            TokenDescriptor descriptor;
            descriptor.line.family = line_family;
            if (integral_primitive_length)
            {
                const WideInteger shift =
                    wide_multiply(static_cast<std::int64_t>(capsule.width_nm),
                                  static_cast<std::int64_t>(primitive_length));
                descriptor.line.rational_part_times_two =
                    sign > 0 ? wide_add(base_times_two, shift)
                             : wide_subtract(base_times_two, shift);
            }
            else
            {
                descriptor.line.rational_part_times_two = base_times_two;
                descriptor.line.radical_coefficient =
                    sign * static_cast<std::int64_t>(capsule.width_nm);
            }
            return descriptor;
        };
        const auto source = [&](AnalyticFilteredSourceRole role)
        {
            return AnalyticFilteredSourceReference{AnalyticFilteredSourceKind::compact_feature_role,
                                                   role, operand.operand_id, capsule.feature_id, 0};
        };
        const auto line = [&](Point first, Point second)
        {
            AnalyticAtomicCurveNm curve;
            curve.start = public_point(first);
            curve.end = public_point(second);
            return curve;
        };
        const auto cap = [&](Point first, Point second, AnalyticIntegerPointNm center)
        {
            AnalyticAtomicCurveNm curve;
            curve.kind = AnalyticAtomicCurveKind::circular_arc;
            curve.start = public_point(first);
            curve.end = public_point(second);
            curve.circle.center = public_point(point(center));
            curve.circle.radius = public_interval(half_width);
            curve.counterclockwise = true;
            curve.has_arc_sweep_certificate = true;
            return curve;
        };
        return emit(line(start_right, end_right), forward_agrees, true, operand.operand_id,
                    source(AnalyticFilteredSourceRole::capsule_right_line),
                    line_descriptor(false)) &&
               emit(cap(end_right, end_left, end), true, true, operand.operand_id,
                    source(AnalyticFilteredSourceRole::capsule_end_cap),
                    circle_descriptor(end, width_squared)) &&
               emit(line(end_left, start_left), !forward_agrees, true, operand.operand_id,
                    source(AnalyticFilteredSourceRole::capsule_left_line), line_descriptor(true)) &&
               emit(cap(start_left, start_right, start), true, true, operand.operand_id,
                    source(AnalyticFilteredSourceRole::capsule_start_cap),
                    circle_descriptor(start, width_squared));
    }

    bool lower_operand(const AnalyticRequestOperandRecord& operand)
    {
        if (operand.geometry_kind == 1)
        {
            const auto& region = records_.planar_regions[operand.geometry_index];
            if (!lower_authored_ring(records_.rings[region.outer_ring], false, operand.operand_id))
                return false;
            for (std::uint32_t index = 0; index < region.hole_reference_count; ++index)
                if (!lower_authored_ring(
                        records_
                            .rings[records_.ring_references[region.hole_reference_begin + index]],
                        true, operand.operand_id))
                    return false;
            return true;
        }
        if (operand.geometry_kind == 2)
        {
            const auto& disk = records_.disks[operand.geometry_index];
            return lower_full_circle(disk.center_x_nm, disk.center_y_nm, disk.radius_nm, true,
                                     operand.operand_id,
                                     {AnalyticFilteredSourceKind::compact_feature_role,
                                      AnalyticFilteredSourceRole::primitive_outer_circle,
                                      operand.operand_id, disk.feature_id, 0});
        }
        if (operand.geometry_kind == 3)
        {
            const auto& annulus = records_.annuli[operand.geometry_index];
            return lower_full_circle(annulus.center_x_nm, annulus.center_y_nm,
                                     annulus.outer_radius_nm, true, operand.operand_id,
                                     {AnalyticFilteredSourceKind::compact_feature_role,
                                      AnalyticFilteredSourceRole::primitive_outer_circle,
                                      operand.operand_id, annulus.feature_id, 0}) &&
                   lower_full_circle(annulus.center_x_nm, annulus.center_y_nm,
                                     annulus.inner_radius_nm, false, operand.operand_id,
                                     {AnalyticFilteredSourceKind::compact_feature_role,
                                      AnalyticFilteredSourceRole::primitive_inner_circle,
                                      operand.operand_id, annulus.feature_id, 0});
        }
        if (operand.geometry_kind == 4)
            return lower_capsule(operand);
        return fail(AnalyticFilteredLoweringError::unsupported_geometry);
    }

    bool lower_operands()
    {
        return for_each_operand([&](const AnalyticRequestOperandRecord& operand)
                                { return lower_operand(operand); }) &&
               out_.curves.size() == projected_curves_;
    }

    const AnalyticRequestPacketRecords& records_;
    AnalyticSolverLimits limits_;
    const AnalyticRequestJobRecord* job_ = nullptr;
    AnalyticFilteredGeometry out_;
    AnalyticFilteredLoweringTelemetry telemetry_;
    AnalyticFilteredLoweringError error_ = AnalyticFilteredLoweringError::none;
    std::uint64_t projected_curves_ = 0;
    std::uint32_t operand_begin_ = 0;
    std::uint32_t operand_end_ = 0;
    bool has_global_bounds_ = false;
    std::int64_t min_global_x_ = 0;
    std::int64_t max_global_x_ = 0;
    std::int64_t min_global_y_ = 0;
    std::int64_t max_global_y_ = 0;
    bool has_extent_ = false;
    std::int64_t min_extent_x_ = 0;
    std::int64_t max_extent_x_ = 0;
    std::int64_t min_extent_y_ = 0;
    std::int64_t max_extent_y_ = 0;
    bool has_output_bounds_ = false;
    double min_output_x_ = 0.0;
    double max_output_x_ = 0.0;
    double min_output_y_ = 0.0;
    double max_output_y_ = 0.0;
    std::vector<TokenDescriptor> descriptors_;
};

} // namespace

AnalyticFilteredLoweringResult
lower_analytic_job_to_filtered_curves(const AnalyticRequestPacketRecords& records,
                                      std::uint32_t job_index, const AnalyticSolverLimits& limits)
{
    return FilteredJobLowerer(records, limits).lower(job_index);
}

} // namespace geometer

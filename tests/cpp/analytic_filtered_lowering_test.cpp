#include "geometer/analytic_filtered_lowering.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{

using namespace geometer;

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct SegmentSpec
{
    std::uint64_t id = 0;
    std::uint64_t curve_id = 0;
    std::uint8_t kind = 1;
    std::uint8_t direction = 0;
    bool major_arc = false;
    std::int64_t center_x_nm = 0;
    std::int64_t center_y_nm = 0;
};

std::uint32_t add_ring(AnalyticRequestPacketRecords& records, std::uint64_t ring_id,
                       const std::vector<std::pair<std::int64_t, std::int64_t>>& vertices,
                       const std::vector<SegmentSpec>& segments, std::uint32_t flags = 0)
{
    const std::uint32_t ring_index = static_cast<std::uint32_t>(records.rings.size());
    const std::uint32_t vertex_begin = static_cast<std::uint32_t>(records.vertices.size());
    const std::uint32_t segment_begin = static_cast<std::uint32_t>(records.segments.size());
    std::uint64_t vertex_id = ring_id * 100;
    for (const auto& [x, y] : vertices)
        records.vertices.push_back({vertex_id++, x, y});
    for (const SegmentSpec& segment : segments)
        records.segments.push_back({segment.id, segment.curve_id, segment.kind, segment.direction,
                                    segment.major_arc, segment.center_x_nm, segment.center_y_nm});
    records.rings.push_back({ring_id, vertex_begin, static_cast<std::uint32_t>(vertices.size()),
                             segment_begin, static_cast<std::uint32_t>(segments.size()), flags});
    return ring_index;
}

AnalyticRequestPacketRecords
region_records(const std::vector<std::pair<std::int64_t, std::int64_t>>& vertices,
               const std::vector<SegmentSpec>& segments)
{
    AnalyticRequestPacketRecords records;
    records.jobs = {{10, 0, 1}};
    records.stages = {{100, 1, 0, 1}};
    records.operands = {{1000, 1, 0}};
    records.planar_regions = {{500, 0, 0, 0}};
    add_ring(records, 600, vertices, segments);
    return records;
}

AnalyticFilteredGeometry lower(const AnalyticRequestPacketRecords& records,
                               const std::string& label)
{
    const AnalyticFilteredLoweringResult result = lower_analytic_job_to_filtered_curves(records, 0);
    require(result.error == AnalyticFilteredLoweringError::none && result.value.has_value(),
            label + " failed with error " + std::to_string(static_cast<int>(result.error)));
    require(result.telemetry.algebraic_fallback_calls == 0,
            label + " invoked the algebraic fallback");
    return *result.value;
}

void require_narrow_accepts(const AnalyticFilteredGeometry& geometry, const std::string& label)
{
    const AnalyticNarrowPhaseResult result =
        intersect_analytic_curve_candidates(geometry.curves, {});
    require(result.error == AnalyticNarrowPhaseError::none,
            label + " produced curves rejected by the narrow phase");
}

void test_authored_winding_and_large_origin()
{
    constexpr std::int64_t base = std::numeric_limits<std::int64_t>::max() - 1000;
    const std::vector<SegmentSpec> segments = {{800, 900}, {801, 901}, {802, 902}, {803, 903}};
    const AnalyticFilteredGeometry ccw =
        lower(region_records(
                  {{base, base}, {base + 100, base}, {base + 100, base + 100}, {base, base + 100}},
                  segments),
              "large-origin CCW square");
    require(ccw.origin_x_nm == base && ccw.origin_y_nm == base && ccw.curves.size() == 4,
            "local origin or square curve count drifted");
    for (std::size_t index = 0; index < ccw.curves.size(); ++index)
    {
        require(ccw.curves[index].curve_index == index + 1 &&
                    ccw.occurrences[index].material_on_left,
                "CCW square occurrence metadata drifted");
        require(ccw.curves[index].has_integer_certificate &&
                    ccw.curves[index].construction_carrier_id != 0 &&
                    ccw.curves[index].construction_family_id != 0,
                "authored line proof tokens were not emitted");
    }
    require_narrow_accepts(ccw, "large-origin CCW square");

    const AnalyticFilteredGeometry clockwise = lower(
        region_records({{0, 0}, {0, 100}, {100, 100}, {100, 0}}, segments), "clockwise square");
    for (const AnalyticFilteredOccurrence& occurrence : clockwise.occurrences)
        require(!occurrence.material_on_left,
                "clockwise authored winding did not preserve the interior side");
}

void test_irrational_authored_arc()
{
    const AnalyticRequestPacketRecords records =
        region_records({{1, 1}, {-1, 1}}, {{800, 900, 2, 1, false, 0, 0}, {801, 901}});
    const AnalyticFilteredGeometry geometry = lower(records, "sqrt(2) authored arc");
    require(geometry.curves.size() == 2 &&
                geometry.curves[0].kind == AnalyticAtomicCurveKind::circular_arc,
            "authored arc did not lower first");
    const AnalyticAtomicCurveNm& arc = geometry.curves[0];
    require(arc.has_integer_certificate && !arc.has_integer_radius_certificate &&
                arc.circle.radius.lower <= std::sqrt(2.0) &&
                arc.circle.radius.upper >= std::sqrt(2.0),
            "irrational authored radius was not outward enclosed");
    require(arc.construction_carrier_id != 0 && arc.construction_family_id != 0,
            "irrational authored arc did not receive exact construction tokens");
    require(geometry.occurrences[0].material_on_left &&
                geometry.occurrences[0].source.role ==
                    AnalyticFilteredSourceRole::authored_circular_arc,
            "authored arc winding or source metadata drifted");
    require_narrow_accepts(geometry, "sqrt(2) authored arc");
}

void test_disks_annuli_and_tokens()
{
    AnalyticRequestPacketRecords records;
    records.jobs = {{10, 0, 1}};
    records.stages = {{100, 1, 0, 2}};
    records.operands = {{1000, 2, 0}, {1001, 3, 0}};
    records.disks = {{7000, 1000, -2000, 100}};
    records.annuli = {{7001, 1000, -2000, 40, 100}};
    const AnalyticFilteredGeometry geometry = lower(records, "disk and annulus");
    require(geometry.curves.size() == 6 && geometry.bounds.size() == 6 &&
                geometry.occurrences.size() == 6,
            "compact circle curve counts drifted");
    require(geometry.curves[0].construction_carrier_id ==
                    geometry.curves[1].construction_carrier_id &&
                geometry.curves[0].construction_carrier_id ==
                    geometry.curves[2].construction_carrier_id &&
                geometry.curves[0].construction_family_id ==
                    geometry.curves[4].construction_family_id &&
                geometry.curves[0].construction_carrier_id !=
                    geometry.curves[4].construction_carrier_id,
            "equal circle carriers or concentric families were not interned deterministically");
    require(geometry.occurrences[4].material_on_left == false &&
                geometry.occurrences[4].source.role ==
                    AnalyticFilteredSourceRole::primitive_inner_circle,
            "annulus inner boundary metadata drifted");
    require_narrow_accepts(geometry, "disk and annulus");
}

void test_arbitrary_capsule()
{
    AnalyticRequestPacketRecords records;
    records.jobs = {{10, 0, 1}};
    records.stages = {{100, 1, 0, 1}};
    records.operands = {{1000, 4, 0}};
    records.capsules = {{8000, 0, 0, 3, 4, 21}};
    const AnalyticFilteredGeometry geometry = lower(records, "3-4-5 odd-width capsule");
    require(geometry.curves.size() == 4 &&
                geometry.curves[0].kind == AnalyticAtomicCurveKind::line &&
                geometry.curves[1].kind == AnalyticAtomicCurveKind::circular_arc &&
                geometry.curves[2].kind == AnalyticAtomicCurveKind::line &&
                geometry.curves[3].kind == AnalyticAtomicCurveKind::circular_arc,
            "capsule role order drifted");
    require(geometry.curves[0].construction_family_id ==
                    geometry.curves[2].construction_family_id &&
                geometry.curves[0].construction_carrier_id !=
                    geometry.curves[2].construction_carrier_id,
            "capsule offset-line family tokens drifted");
    require(geometry.curves[1].has_arc_sweep_certificate &&
                geometry.curves[3].has_arc_sweep_certificate &&
                geometry.curves[1].circle.radius.lower == 10.5 &&
                geometry.curves[1].circle.radius.upper == 10.5,
            "capsule cap sweep/radius certificates drifted");
    require(geometry.curves[0].start.x.lower <= 8.4 && geometry.curves[0].start.x.upper >= 8.4 &&
                geometry.curves[0].start.y.lower <= -6.3 &&
                geometry.curves[0].start.y.upper >= -6.3,
            "arbitrary capsule offset was not outward enclosed");
    for (const AnalyticFilteredOccurrence& occurrence : geometry.occurrences)
        require(occurrence.material_on_left, "capsule boundary must traverse with material left");
    require_narrow_accepts(geometry, "3-4-5 odd-width capsule");
}

AnalyticRequestPacketRecords duplicate_capsule_records()
{
    AnalyticRequestPacketRecords records;
    records.jobs = {{10, 0, 1}};
    records.stages = {{100, 1, 0, 2}};
    records.operands = {{1000, 4, 0}, {1001, 4, 1}};
    records.capsules = {{8000, 0, 0, 100, 0, 20}, {8001, 0, 0, 100, 0, 20}};
    return records;
}

void test_capsule_carrier_proofs()
{
    const AnalyticFilteredGeometry duplicates =
        lower(duplicate_capsule_records(), "duplicate capsules");
    require(duplicates.curves[0].construction_family_id ==
                    duplicates.curves[4].construction_family_id &&
                duplicates.curves[0].construction_carrier_id ==
                    duplicates.curves[4].construction_carrier_id &&
                duplicates.curves[2].construction_carrier_id ==
                    duplicates.curves[6].construction_carrier_id,
            "duplicate capsule offset carriers did not reuse proof tokens");
    const AnalyticNarrowPhaseResult duplicate_intersections =
        intersect_analytic_curve_candidates(duplicates.curves, {{1, 5}, {3, 7}});
    require(
        duplicate_intersections.error == AnalyticNarrowPhaseError::none &&
            duplicate_intersections.intersections.size() == 2 &&
            duplicate_intersections.intersections[0].relation == AnalyticPairRelation::coincident &&
            duplicate_intersections.intersections[1].relation == AnalyticPairRelation::coincident,
        "duplicate capsule lines were not preserved for same-domain overlay");

    AnalyticRequestPacketRecords irrational = duplicate_capsule_records();
    irrational.capsules = {{8000, -7, 11, 16, 28, 21}, {8001, -7, 11, 16, 28, 21}};
    const AnalyticFilteredGeometry irrational_duplicates =
        lower(irrational, "duplicate irrational-direction capsules");
    require(irrational_duplicates.curves[0].construction_carrier_id ==
                    irrational_duplicates.curves[4].construction_carrier_id &&
                irrational_duplicates.curves[2].construction_carrier_id ==
                    irrational_duplicates.curves[6].construction_carrier_id,
            "duplicate irrational capsule carriers did not reuse exact proof tokens");
    const AnalyticNarrowPhaseResult irrational_intersections =
        intersect_analytic_curve_candidates(irrational_duplicates.curves, {{1, 5}, {3, 7}});
    require(irrational_intersections.error == AnalyticNarrowPhaseError::none &&
                irrational_intersections.intersections.size() == 2 &&
                irrational_intersections.intersections[0].relation ==
                    AnalyticPairRelation::coincident &&
                irrational_intersections.intersections[1].relation ==
                    AnalyticPairRelation::coincident,
            "irrational duplicate capsule lines were not preserved for same-domain overlay");

    AnalyticRequestPacketRecords authored;
    authored.jobs = {{10, 0, 1}};
    authored.stages = {{100, 1, 0, 2}};
    authored.operands = {{1000, 1, 0}, {1001, 4, 0}};
    authored.planar_regions = {{500, 0, 0, 0}};
    add_ring(authored, 600, {{0, -10}, {100, -10}, {100, -20}, {0, -20}},
             {{800, 900}, {801, 901}, {802, 902}, {803, 903}});
    authored.capsules = {{8000, 0, 0, 100, 0, 20}};
    const AnalyticFilteredGeometry mixed = lower(authored, "authored/capsule coincidence");
    require(mixed.curves[0].construction_family_id == mixed.curves[4].construction_family_id &&
                mixed.curves[0].construction_carrier_id == mixed.curves[4].construction_carrier_id,
            "authored and constructed equal line carriers did not reuse proof tokens");
    const AnalyticNarrowPhaseResult mixed_intersection =
        intersect_analytic_curve_candidates(mixed.curves, {{1, 5}});
    require(mixed_intersection.error == AnalyticNarrowPhaseError::none &&
                mixed_intersection.intersections[0].relation == AnalyticPairRelation::coincident,
            "authored/capsule coincidence was not forwarded to overlay");
}

AnalyticRequestPacketRecords sparse_short_arc_records(std::uint32_t count)
{
    AnalyticRequestPacketRecords records;
    records.jobs = {{10, 0, 1}};
    records.stages = {{100, 1, 0, count}};
    for (std::uint32_t index = 0; index < count; ++index)
    {
        const std::int64_t center_x = static_cast<std::int64_t>(index) * 1'100'000;
        records.operands.push_back({1000 + index, 1, index});
        records.planar_regions.push_back({5000 + index, index, 0, 0});
        add_ring(records, 6000 + index,
                 {{center_x + 5'000'000, 0}, {center_x + 4'000'000, 3'000'000}},
                 {{8000 + index * 2, 9000 + index * 2, 2, 1, false, center_x, 0},
                  {8001 + index * 2, 9001 + index * 2}});
    }
    return records;
}

void test_arc_tight_bounds_and_sparse_scaling()
{
    const AnalyticFilteredLoweringResult small_result =
        lower_analytic_job_to_filtered_curves(sparse_short_arc_records(24), 0);
    const AnalyticFilteredLoweringResult large_result =
        lower_analytic_job_to_filtered_curves(sparse_short_arc_records(48), 0);
    require(small_result.error == AnalyticFilteredLoweringError::none &&
                small_result.value.has_value() &&
                large_result.error == AnalyticFilteredLoweringError::none &&
                large_result.value.has_value(),
            "sparse short-arc lowering failed");
    require(large_result.telemetry.work_units <= small_result.telemetry.work_units * 3 &&
                large_result.telemetry.token_table_probes <=
                    small_result.telemetry.token_table_probes * 3,
            "lowering/token-table work did not remain near-linear at 2x input");
    const AnalyticFilteredGeometry& small = *small_result.value;
    const AnalyticFilteredGeometry& large = *large_result.value;
    const AnalyticBroadPhaseResult small_pairs = build_analytic_curve_candidates(small.bounds);
    const AnalyticBroadPhaseResult large_pairs = build_analytic_curve_candidates(large.bounds);
    require(small_pairs.error == AnalyticBroadPhaseError::none &&
                large_pairs.error == AnalyticBroadPhaseError::none &&
                small_pairs.pairs.size() == 24 && large_pairs.pairs.size() == 48,
            "short large-radius arcs retained full-circle quadratic broad-phase bounds");
    require(small.bounds[0].min_x == 4'000'000.0 && small.bounds[0].max_x == 5'000'000.0 &&
                small.bounds[0].min_y == 0.0 && small.bounds[0].max_y == 3'000'000.0,
            "CCW minor-arc cardinal bounds drifted");

    const AnalyticRequestPacketRecords major_records = region_records(
        {{5'000'000, 0}, {4'000'000, 3'000'000}}, {{800, 900, 2, 2, true, 0, 0}, {801, 901}});
    const AnalyticFilteredGeometry major = lower(major_records, "CW major arc");
    require(major.bounds[0].min_x <= -5'000'000.0 && major.bounds[0].min_y <= -5'000'000.0 &&
                major.bounds[0].max_y >= 5'000'000.0,
            "major-arc cardinal extrema were omitted");
}

void test_empty_jobs_radius_domain_and_global_expansion()
{
    AnalyticRequestPacketRecords empty;
    empty.jobs = {{10, 0, 4}};
    empty.stages = {{100, 1, 0, 0}, {101, 2, 0, 0}, {102, 1, 0, 0}, {103, 2, 0, 0}};
    AnalyticFilteredLoweringResult result = lower_analytic_job_to_filtered_curves(empty, 0);
    require(result.error == AnalyticFilteredLoweringError::none && result.value.has_value() &&
                result.value->curves.empty() && result.value->origin_x_nm == 0 &&
                result.value->origin_y_nm == 0 && result.telemetry.stage_records_visited == 4,
            "zero-operand stages must lower as a deterministic empty no-op");
    AnalyticSolverLimits limits = kAnalyticSolverHardLimits;
    limits.predicate_calls = 3;
    result = lower_analytic_job_to_filtered_curves(empty, 0, limits);
    require(result.error == AnalyticFilteredLoweringError::resource_limit_exceeded &&
                result.telemetry.stage_records_visited == 3 && result.telemetry.work_units == 3,
            "empty-stage traversal did not stop at the effective work limit");

    const AnalyticRequestPacketRecords excessive_radius =
        region_records({{0, 999'999'999'999}, {999'999'999'999, 0}},
                       {{800, 900, 2, 1, false, 1'000'000'000'000, 1'000'000'000'000}, {801, 901}});
    result = lower_analytic_job_to_filtered_curves(excessive_radius, 0);
    require(result.error == AnalyticFilteredLoweringError::resource_limit_exceeded &&
                !result.value.has_value(),
            "an authored radius outside the narrow domain lowered successfully");

    const AnalyticRequestPacketRecords maximum_radius =
        region_records({{1'000'000'000'000, 0}, {0, 1'000'000'000'000}},
                       {{800, 900, 2, 1, false, 0, 0}, {801, 901}});
    const AnalyticFilteredGeometry maximum = lower(maximum_radius, "maximum exact radius");
    require(maximum.curves[0].has_integer_radius_certificate &&
                maximum.curves[0].circle.radius.lower == 1'000'000'000'000.0 &&
                maximum.curves[0].circle.radius.upper == 1'000'000'000'000.0,
            "the exact maximum radius was not canonicalized to a singleton");
    require_narrow_accepts(maximum, "maximum exact radius");

    AnalyticRequestPacketRecords edge;
    edge.jobs = {{10, 0, 1}};
    edge.stages = {{100, 1, 0, 1}};
    edge.operands = {{1000, 2, 0}};
    edge.disks = {{7000, std::numeric_limits<std::int64_t>::max(), 0, 1}};
    result = lower_analytic_job_to_filtered_curves(edge, 0);
    require(result.error == AnalyticFilteredLoweringError::resource_limit_exceeded,
            "positive global coordinate expansion overflow was accepted");
    edge.disks[0].center_x_nm = std::numeric_limits<std::int64_t>::min();
    result = lower_analytic_job_to_filtered_curves(edge, 0);
    require(result.error == AnalyticFilteredLoweringError::resource_limit_exceeded,
            "negative global coordinate expansion overflow was accepted");

    const AnalyticRequestPacketRecords maximum_minor_arc =
        region_records({{0, 0}, {1'000'000'000'000, 1'000'000'000'000}},
                       {{800, 900, 2, 2, false, 1'000'000'000'000, 0}, {801, 901}});
    const AnalyticFilteredGeometry minor = lower(maximum_minor_arc, "maximum-span minor arc");
    require(minor.bounds[0].max_x - minor.bounds[0].min_x <= 1'000'000'000'000.0 &&
                minor.bounds[0].max_y - minor.bounds[0].min_y <= 1'000'000'000'000.0,
            "sweep-tight minor arc did not remain inside the maximum span");

    const AnalyticRequestPacketRecords excessive_major_arc =
        region_records({{0, 0}, {1'000'000'000'000, 1'000'000'000'000}},
                       {{800, 900, 2, 1, true, 1'000'000'000'000, 0}, {801, 901}});
    result = lower_analytic_job_to_filtered_curves(excessive_major_arc, 0);
    require(result.error == AnalyticFilteredLoweringError::resource_limit_exceeded &&
                !result.value.has_value(),
            "major-arc cardinal extrema bypassed the maximum job span");
}

void test_fail_closed_limits_and_swept_path()
{
    AnalyticRequestPacketRecords disk;
    disk.jobs = {{10, 0, 1}};
    disk.stages = {{100, 1, 0, 1}};
    disk.operands = {{1000, 2, 0}};
    disk.disks = {{7000, 0, 0, 100}};

    AnalyticSolverLimits limits = kAnalyticSolverHardLimits;
    limits.boundary_occurrences = 1;
    AnalyticFilteredLoweringResult result = lower_analytic_job_to_filtered_curves(disk, 0, limits);
    require(result.error == AnalyticFilteredLoweringError::resource_limit_exceeded &&
                !result.value.has_value(),
            "curve count limit was not enforced before allocation");

    limits = kAnalyticSolverHardLimits;
    limits.working_memory_bytes = 2 * 768 - 1;
    result = lower_analytic_job_to_filtered_curves(disk, 0, limits);
    require(result.error == AnalyticFilteredLoweringError::resource_limit_exceeded &&
                result.telemetry.peak_working_memory_bytes == 0,
            "one-byte-short lowering memory limit did not fail closed");
    limits.working_memory_bytes = 2 * 768;
    result = lower_analytic_job_to_filtered_curves(disk, 0, limits);
    require(result.error == AnalyticFilteredLoweringError::none &&
                result.telemetry.peak_working_memory_bytes == 2 * 768,
            "exact lowering memory budget did not succeed deterministically");

    const std::uint64_t disk_work_units = result.telemetry.work_units;
    require(result.telemetry.token_table_probes != 0 && disk_work_units != 0,
            "disk lowering did not meter token-table work");
    limits = kAnalyticSolverHardLimits;
    limits.predicate_calls = disk_work_units - 1;
    result = lower_analytic_job_to_filtered_curves(disk, 0, limits);
    require(result.error == AnalyticFilteredLoweringError::resource_limit_exceeded &&
                result.telemetry.token_table_probes != 0 &&
                result.telemetry.work_units == limits.predicate_calls,
            "one-unit-short lowering work limit did not stop deterministically");

    constexpr std::uint32_t large_ring_size = 131'072;
    constexpr std::int64_t large_ring_side = large_ring_size / 4;
    AnalyticRequestPacketRecords large_ring;
    large_ring.jobs = {{10, 0, 1}};
    large_ring.stages = {{100, 1, 0, 1}};
    large_ring.operands = {{1000, 1, 0}};
    large_ring.planar_regions = {{500, 0, 0, 0}};
    large_ring.rings = {{600, 0, large_ring_size, 0, large_ring_size, 0}};
    large_ring.vertices.reserve(large_ring_size);
    large_ring.segments.reserve(large_ring_size);
    for (std::uint32_t index = 0; index < large_ring_size; ++index)
    {
        std::int64_t x = 0;
        std::int64_t y = 0;
        if (index < large_ring_side)
            x = index;
        else if (index < large_ring_side * 2)
        {
            x = large_ring_side;
            y = index - large_ring_side;
        }
        else if (index < large_ring_side * 3)
        {
            x = large_ring_side * 3 - index;
            y = large_ring_side;
        }
        else
            y = large_ring_size - index;
        large_ring.vertices.push_back({10'000 + index, x, y});
        large_ring.segments.push_back({20'000 + index, 30'000 + index, 1, 0, false, 0, 0});
    }
    require(validate_analytic_request_packet_records(large_ring) ==
                AnalyticRequestPacketError::none,
            "large authored-ring work fixture is not packet-valid");
    limits = kAnalyticSolverHardLimits;
    limits.predicate_calls = 3;
    result = lower_analytic_job_to_filtered_curves(large_ring, 0, limits);
    require(result.error == AnalyticFilteredLoweringError::resource_limit_exceeded &&
                !result.value.has_value() && result.telemetry.work_units == 3 &&
                result.telemetry.input_segments == large_ring_size &&
                result.telemetry.peak_working_memory_bytes == 0,
            "large authored ring was traversed or allocated past its short work budget");

    AnalyticRequestPacketRecords swept;
    swept.jobs = {{10, 0, 1}};
    swept.stages = {{100, 1, 0, 1}};
    swept.operands = {{1000, 5, 0}};
    swept.rings = {{600, 0, 2, 0, 1, 1}};
    swept.vertices = {{1, 0, 0}, {2, 100, 0}};
    swept.segments = {{3, 4, 1, 0, false, 0, 0}};
    swept.swept_paths = {{8000, 0, 20}};
    result = lower_analytic_job_to_filtered_curves(swept, 0);
    require(result.error == AnalyticFilteredLoweringError::unsupported_geometry &&
                !result.value.has_value() && result.telemetry.algebraic_fallback_calls == 0,
            "swept paths must fail closed without entering the exact arena");
}

std::string lowering_parity_vector()
{
    AnalyticRequestPacketRecords records;
    records.jobs = {{10, 0, 1}};
    records.stages = {{100, 1, 0, 3}};
    records.operands = {{1000, 1, 0}, {1001, 4, 0}, {1002, 4, 1}};
    records.planar_regions = {{500, 0, 0, 0}};
    add_ring(records, 600, {{1, 1}, {-1, 1}}, {{800, 900, 2, 1, false, 0, 0}, {801, 901}});
    records.capsules = {{8000, -7, 11, 16, 28, 21}, {8001, -7, 11, 16, 28, 21}};
    const AnalyticFilteredLoweringResult result = lower_analytic_job_to_filtered_curves(records, 0);
    require(result.error == AnalyticFilteredLoweringError::none && result.value.has_value(),
            "lowering parity fixture failed");
    const AnalyticFilteredGeometry& geometry = *result.value;
    require(geometry.curves[2].construction_carrier_id ==
                    geometry.curves[6].construction_carrier_id &&
                geometry.curves[4].construction_carrier_id ==
                    geometry.curves[8].construction_carrier_id,
            "parity fixture duplicate capsule carrier proofs drifted");
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    auto append_u64 = [&output](std::uint64_t value) { output << std::setw(16) << value; };
    auto append_double = [&append_u64](double value)
    {
        std::uint64_t bits = 0;
        static_assert(sizeof(bits) == sizeof(value));
        std::memcpy(&bits, &value, sizeof(bits));
        append_u64(bits);
    };
    append_u64(static_cast<std::uint64_t>(geometry.origin_x_nm));
    append_u64(static_cast<std::uint64_t>(geometry.origin_y_nm));
    append_u64(geometry.curves.size());
    for (std::size_t index = 0; index < geometry.curves.size(); ++index)
    {
        const AnalyticAtomicCurveNm& curve = geometry.curves[index];
        const AnalyticCurveBoundsNm& bounds = geometry.bounds[index];
        const AnalyticFilteredOccurrence& occurrence = geometry.occurrences[index];
        append_u64(curve.curve_index);
        append_u64(static_cast<std::uint8_t>(curve.kind));
        append_double(curve.start.x.lower);
        append_double(curve.start.x.upper);
        append_double(curve.start.y.lower);
        append_double(curve.start.y.upper);
        append_double(curve.end.x.lower);
        append_double(curve.end.x.upper);
        append_double(curve.end.y.lower);
        append_double(curve.end.y.upper);
        append_double(curve.circle.center.x.lower);
        append_double(curve.circle.center.x.upper);
        append_double(curve.circle.center.y.lower);
        append_double(curve.circle.center.y.upper);
        append_double(curve.circle.radius.lower);
        append_double(curve.circle.radius.upper);
        append_u64(curve.counterclockwise ? 1U : 0U);
        append_u64(curve.major_arc ? 1U : 0U);
        append_u64(curve.construction_carrier_id);
        append_u64(curve.construction_family_id);
        append_u64(curve.has_arc_sweep_certificate ? 1U : 0U);
        append_u64(curve.has_integer_certificate ? 1U : 0U);
        append_u64(static_cast<std::uint64_t>(curve.integer_start.x));
        append_u64(static_cast<std::uint64_t>(curve.integer_start.y));
        append_u64(static_cast<std::uint64_t>(curve.integer_end.x));
        append_u64(static_cast<std::uint64_t>(curve.integer_end.y));
        append_u64(static_cast<std::uint64_t>(curve.integer_center.x));
        append_u64(static_cast<std::uint64_t>(curve.integer_center.y));
        append_u64(curve.has_integer_radius_certificate ? 1U : 0U);
        append_u64(curve.integer_radius);
        append_double(bounds.min_x);
        append_double(bounds.min_y);
        append_double(bounds.max_x);
        append_double(bounds.max_y);
        append_u64(occurrence.occurrence_id);
        append_u64(occurrence.coverage_id);
        append_u64(occurrence.agrees_with_carrier ? 1U : 0U);
        append_u64(occurrence.material_on_left ? 1U : 0U);
        append_u64(static_cast<std::uint16_t>(occurrence.source.kind));
        append_u64(static_cast<std::uint16_t>(occurrence.source.role));
        append_u64(occurrence.source.operand_id);
        append_u64(occurrence.source.primary_id);
        append_u64(occurrence.source.secondary_id);
    }
    append_u64(result.telemetry.input_operands);
    append_u64(result.telemetry.input_segments);
    append_u64(result.telemetry.emitted_curves);
    append_u64(result.telemetry.stage_records_visited);
    append_u64(result.telemetry.operand_records_visited);
    append_u64(result.telemetry.fixed_width_predicates);
    append_u64(result.telemetry.token_table_probes);
    append_u64(result.telemetry.work_units);
    append_u64(result.telemetry.square_root_calls);
    append_u64(result.telemetry.peak_working_memory_bytes);
    append_u64(result.telemetry.algebraic_fallback_calls);
    return output.str();
}

} // namespace

int main()
{
    test_authored_winding_and_large_origin();
    test_irrational_authored_arc();
    test_disks_annuli_and_tokens();
    test_arbitrary_capsule();
    test_capsule_carrier_proofs();
    test_arc_tight_bounds_and_sparse_scaling();
    test_empty_jobs_radius_domain_and_global_expansion();
    test_fail_closed_limits_and_swept_path();
    std::cout << "ANALYTIC_FILTERED_LOWERING_VECTOR=" << lowering_parity_vector() << '\n';
    return 0;
}

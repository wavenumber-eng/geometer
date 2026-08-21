#include "geometer/analytic_curve_broad_phase.h"
#include "geometer/analytic_filtered_lowering.h"
#include "geometer/analytic_filtered_normalization.h"
#include "geometer/analytic_filtered_packet.h"

#include <algorithm>
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
    std::uint64_t radius_nm = 0;
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
                                    segment.major_arc, segment.center_x_nm, segment.center_y_nm,
                                    segment.radius_nm});
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
        if (ccw.curves[index].construction_line_dx == 0)
        {
            const std::uint64_t column =
                analytic_vertical_x_column_token(ccw.curves[index].construction_carrier_id);
            require(column != 0 && ccw.curves[index].start.construction_x_column_id == column &&
                        ccw.curves[index].end.construction_x_column_id == column,
                    "authored vertical line did not receive a shared x-column token");
        }
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

void test_endpoint_radius_authored_minor_and_major_arcs()
{
    const AnalyticRequestPacketRecords records =
        region_records({{0, 0}, {4000, 0}},
                       {{800, 900, 3, 1, false, 0, 0, 3000}, {801, 901, 3, 1, true, 0, 0, 3000}});
    const AnalyticFilteredGeometry geometry =
        lower(records, "endpoint/radius irrational-center circle");
    require(geometry.curves.size() == 2,
            "endpoint/radius circle did not preserve two authored arcs");
    const AnalyticAtomicCurveNm& minor = geometry.curves[0];
    const AnalyticAtomicCurveNm& major = geometry.curves[1];
    require(!minor.has_integer_certificate && !major.has_integer_certificate &&
                minor.has_integer_radius_certificate && major.has_integer_radius_certificate &&
                minor.has_endpoint_authoritative_arc_certificate &&
                major.has_endpoint_authoritative_arc_certificate &&
                minor.has_arc_sweep_certificate && major.has_arc_sweep_certificate &&
                minor.integer_radius == 3000 && major.integer_radius == 3000 && !minor.major_arc &&
                major.major_arc,
            "endpoint/radius proof certificates or branch selection drifted");
    const double local_center_y = 6000.0 + std::sqrt(5'000'000.0);
    require(minor.circle.center.x.lower <= 8000.0 && minor.circle.center.x.upper >= 8000.0 &&
                minor.circle.center.y.lower <= local_center_y &&
                minor.circle.center.y.upper >= local_center_y &&
                minor.construction_carrier_id == major.construction_carrier_id &&
                minor.construction_family_id == major.construction_family_id,
            "endpoint/radius irrational center or shared carrier identity drifted");
    require_narrow_accepts(geometry, "endpoint/radius irrational-center circle");

    AnalyticRequestPacketRecords chord_too_long = records;
    chord_too_long.vertices[1].x_nm = 6001;
    require(lower_analytic_job_to_filtered_curves(chord_too_long, 0).error ==
                AnalyticFilteredLoweringError::invalid_arc,
            "endpoint/radius chord longer than the diameter was accepted");
    AnalyticRequestPacketRecords major_semicircle = records;
    major_semicircle.vertices[1].x_nm = 6000;
    require(lower_analytic_job_to_filtered_curves(major_semicircle, 0).error ==
                AnalyticFilteredLoweringError::invalid_arc,
            "endpoint/radius major semicircle ambiguity was accepted");
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
    require(duplicates.curves[0].construction_horizontal_mirror_id != 0 &&
                duplicates.curves[0].construction_horizontal_mirror_id ==
                    duplicates.curves[2].construction_horizontal_mirror_id &&
                duplicates.curves[0].construction_horizontal_mirror_id ==
                    duplicates.curves[4].construction_horizontal_mirror_id &&
                duplicates.curves[0].construction_horizontal_mirror_id ==
                    duplicates.curves[6].construction_horizontal_mirror_id &&
                duplicates.curves[0].construction_horizontal_mirror_axis_y == 0 &&
                duplicates.curves[2].construction_horizontal_mirror_axis_y == 0,
            "duplicate horizontal capsules did not reuse one certified mirror construction");
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
    require(irrational_duplicates.curves[0].construction_horizontal_mirror_id == 0 &&
                irrational_duplicates.curves[2].construction_horizontal_mirror_id == 0,
            "a nonhorizontal capsule minted a horizontal mirror certificate");
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
    limits.working_memory_bytes = 2 * 800 - 1;
    result = lower_analytic_job_to_filtered_curves(disk, 0, limits);
    require(result.error == AnalyticFilteredLoweringError::resource_limit_exceeded &&
                result.telemetry.peak_working_memory_bytes == 0,
            "one-byte-short lowering memory limit did not fail closed");
    limits.working_memory_bytes = 2 * 800;
    result = lower_analytic_job_to_filtered_curves(disk, 0, limits);
    require(result.error == AnalyticFilteredLoweringError::none &&
                result.telemetry.peak_working_memory_bytes == 2 * 800,
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
    swept.vertices = {{1, 0, 0}, {2, 1'000, 0}};
    swept.segments = {{3, 4, 1, 0, false, 0, 0}};
    swept.swept_paths = {{8000, 0, 200}};
    result = lower_analytic_job_to_filtered_curves(swept, 0);
    require(result.error == AnalyticFilteredLoweringError::none && result.value.has_value() &&
                result.value->curves.size() == 6 && result.telemetry.algebraic_fallback_calls == 0,
            "single-line swept path did not lower to its capsule boundary error=" +
                std::to_string(static_cast<int>(result.error)) +
                " curves=" + std::to_string(result.value ? result.value->curves.size() : 0));

    swept.rings = {{600, 0, 3, 0, 2, 1}};
    swept.vertices = {{1, 0, 0}, {2, 3'000'000, 0}, {3, 5'000'000, 2'000'000}};
    swept.segments = {{3, 4, 1, 0, false, 0, 0}, {5, 6, 2, 1, false, 3'000'000, 2'000'000}};
    swept.swept_paths = {{8000, 0, 1'200'000}};
    result = lower_analytic_job_to_filtered_curves(swept, 0);
    require(result.error == AnalyticFilteredLoweringError::none && result.value.has_value() &&
                result.telemetry.algebraic_fallback_calls == 0,
            "MATZ line/quarter-arc swept path failed filtered lowering error=" +
                std::to_string(static_cast<int>(result.error)));
    const auto broad = build_analytic_curve_candidates(result.value->bounds);
    require(broad.error == AnalyticBroadPhaseError::none,
            "MATZ line/quarter-arc broad phase failed");
    const auto packet = build_analytic_filtered_job_records(swept, 0, *result.value, broad.pairs);
    require(packet.error == AnalyticFilteredPacketError::none && packet.records.has_value(),
            "MATZ line/quarter-arc production packet failed");
}

AnalyticRequestPacketRecords swept_lines(std::initializer_list<AnalyticIntegerPointNm> points,
                                         std::uint64_t width = 200, std::uint64_t feature = 8000)
{
    AnalyticRequestPacketRecords records;
    records.jobs = {{10, 0, 1}};
    records.stages = {{100, 1, 0, 1}};
    records.operands = {{1000, 5, 0}};
    records.rings = {{600, 0, static_cast<std::uint32_t>(points.size()), 0,
                      static_cast<std::uint32_t>(points.size() - 1), 1}};
    std::uint64_t id = 1;
    for (const auto point : points)
        records.vertices.push_back({id++, point.x, point.y});
    for (std::uint32_t index = 0; index + 1 < points.size(); ++index)
        records.segments.push_back({id++, id + 100, 1, 0, false, 0, 0});
    records.swept_paths = {{feature, 0, width}};
    return records;
}

AnalyticFilteredNormalizationResult normalize_lowered(const AnalyticRequestPacketRecords& records)
{
    const auto lowered = lower_analytic_job_to_filtered_curves(records, 0);
    require(lowered.error == AnalyticFilteredLoweringError::none && lowered.value.has_value(),
            "normalization fixture failed swept/primitive lowering");
    const auto broad = build_analytic_curve_candidates(lowered.value->bounds);
    require(broad.error == AnalyticBroadPhaseError::none,
            "normalization fixture failed broad phase");
    return build_analytic_filtered_normalization(records, 0, *lowered.value, broad.pairs);
}

void require_swept_production_success(const AnalyticRequestPacketRecords& records,
                                      const std::string& label)
{
    const auto lowered = lower_analytic_job_to_filtered_curves(records, 0);
    require(lowered.error == AnalyticFilteredLoweringError::none && lowered.value,
            label + " lowering failed error=" + std::to_string(static_cast<int>(lowered.error)));
    const auto broad = build_analytic_curve_candidates(lowered.value->bounds);
    require(broad.error == AnalyticBroadPhaseError::none, label + " broad phase failed");
    const auto normalized =
        build_analytic_filtered_normalization(records, 0, *lowered.value, broad.pairs);
    require(normalized.error == AnalyticFilteredNormalizationError::none,
            label + " normalization failed error=" +
                std::to_string(static_cast<int>(normalized.error)) + " strict_pairs=" +
                std::to_string(normalized.telemetry.strict_replay_candidate_pairs) +
                " fragments=" + std::to_string(normalized.fragments.size()));
    const auto packet =
        build_analytic_filtered_job_records(records, 0, *lowered.value, broad.pairs);
    require(packet.error == AnalyticFilteredPacketError::none && packet.records &&
                packet.records->job_results.size() == 1 &&
                packet.records->job_results[0].status == 0,
            label + " production packet failed error=" +
                std::to_string(static_cast<int>(packet.error)) + " status=" +
                std::to_string(packet.records ? packet.records->job_results[0].status : 99) +
                " diagnostic=" +
                std::to_string(packet.records && !packet.records->diagnostics.empty()
                                   ? packet.records->diagnostics[0].code
                                   : 0));
}

void test_swept_path_staged_contracts()
{
    const auto swept = swept_lines({{0, 0}, {1000, 0}});
    AnalyticRequestPacketRecords capsule;
    capsule.jobs = {{10, 0, 1}};
    capsule.stages = {{100, 1, 0, 1}};
    capsule.operands = {{1000, 4, 0}};
    capsule.capsules = {{8000, 0, 0, 1000, 0, 200}};
    auto swept_normalized = normalize_lowered(swept);
    auto capsule_normalized = normalize_lowered(capsule);
    require(swept_normalized.error == AnalyticFilteredNormalizationError::none &&
                capsule_normalized.error == AnalyticFilteredNormalizationError::none,
            "single-line swept/capsule differential did not normalize swept=" +
                std::to_string(static_cast<int>(swept_normalized.error)) +
                " capsule=" + std::to_string(static_cast<int>(capsule_normalized.error)));
    const auto vertex_coordinates = [](const AnalyticFilteredNormalizationResult& value)
    {
        std::vector<std::pair<std::int64_t, std::int64_t>> result;
        for (const auto& vertex : value.vertices)
            result.emplace_back(vertex.x_nm, vertex.y_nm);
        std::sort(result.begin(), result.end());
        return result;
    };
    require(vertex_coordinates(swept_normalized) == vertex_coordinates(capsule_normalized) &&
                swept_normalized.fragments.size() == capsule_normalized.fragments.size() &&
                swept_normalized.rings.size() == capsule_normalized.rings.size() &&
                swept_normalized.regions.size() == capsule_normalized.regions.size(),
            "single-line swept boundary differs from the production capsule boundary");
    const auto fragment_signature = [](const AnalyticFilteredNormalizationResult& value)
    {
        std::vector<std::tuple<std::int64_t, std::int64_t, std::int64_t, std::int64_t,
                               AnalyticAtomicCurveKind, std::uint64_t, bool, bool>>
            result;
        for (const auto& fragment : value.fragments)
        {
            const auto& start_vertex = value.vertices[fragment.start_vertex];
            const auto& end_vertex = value.vertices[fragment.end_vertex];
            result.emplace_back(start_vertex.x_nm, start_vertex.y_nm, end_vertex.x_nm,
                                end_vertex.y_nm, fragment.kind, fragment.radius_nm,
                                fragment.counterclockwise, fragment.major_arc);
        }
        std::sort(result.begin(), result.end());
        return result;
    };
    require(fragment_signature(swept_normalized) == fragment_signature(capsule_normalized),
            "single-line swept/capsule directed fragment topology differs");
    for (std::size_t index = 0; index < swept_normalized.rings.size(); ++index)
    {
        const auto& left = swept_normalized.rings[index];
        const auto& right = capsule_normalized.rings[index];
        require(left.fragment_count == right.fragment_count && left.depth == right.depth &&
                    left.counterclockwise == right.counterclockwise,
                "single-line swept/capsule ring topology differs");
    }

    const auto l_path = swept_lines({{0, 0}, {1000, 0}, {1000, 1000}}, 200, 8123);
    auto lowered = lower_analytic_job_to_filtered_curves(l_path, 0);
    require(lowered.error == AnalyticFilteredLoweringError::none && lowered.value &&
                lowered.telemetry.algebraic_fallback_calls == 0,
            "L swept path did not union with zero algebraic fallback");
    bool start = false;
    bool end = false;
    bool join = false;
    bool first_segment = false;
    bool second_segment = false;
    for (const auto& occurrence : lowered.value->occurrences)
    {
        const auto& source = occurrence.source;
        require(source.kind == AnalyticFilteredSourceKind::compact_feature_role &&
                    source.operand_id == 1000 && source.primary_id == 8123,
                "swept output source did not bind compact feature/operand identity");
        start = start || (source.role == AnalyticFilteredSourceRole::swept_start_cap &&
                          source.secondary_id == (std::uint64_t{1} << 32U));
        end = end || (source.role == AnalyticFilteredSourceRole::swept_end_cap &&
                      source.secondary_id == (std::uint64_t{3} << 32U));
        join = join || (source.role == AnalyticFilteredSourceRole::swept_round_join &&
                        source.secondary_id == ((std::uint64_t{1} << 32U) | 2U));
        const bool offset = source.role == AnalyticFilteredSourceRole::swept_left_offset_line ||
                            source.role == AnalyticFilteredSourceRole::swept_right_offset_line;
        first_segment =
            first_segment || (offset && source.secondary_id == (std::uint64_t{1} << 32U));
        second_segment =
            second_segment || (offset && source.secondary_id == (std::uint64_t{2} << 32U));
    }
    require(start && end && join && first_segment && second_segment,
            "swept output did not publish the reviewed cap/join/segment source tuples");
    require_swept_production_success(
        swept_lines({{0, 525'000}, {24'999, 549'999}, {24'999, 1'325'001}}, 254'000),
        "short joined RT capsule path");
    AnalyticFilteredGeometry forged_source = *lowered.value;
    forged_source.occurrences[0].source.secondary_id = std::uint64_t{99} << 32U;
    const auto l_broad = build_analytic_curve_candidates(forged_source.bounds);
    require(build_analytic_filtered_job_records(l_path, 0, forged_source, l_broad.pairs).error ==
                AnalyticFilteredPacketError::invalid_argument,
            "packet binding accepted a swept source ordinal outside its path");

    auto mixed_path = swept_lines({{0, 0}, {1000, 0}, {2000, 1000}}, 200);
    mixed_path.segments[1].kind = 2;
    mixed_path.segments[1].direction = 1;
    mixed_path.segments[1].center_x_nm = 1000;
    mixed_path.segments[1].center_y_nm = 1000;
    const auto mixed_lowered = lower_analytic_job_to_filtered_curves(mixed_path, 0);
    require(mixed_lowered.error == AnalyticFilteredLoweringError::none && mixed_lowered.value,
            "mixed line/arc swept source-binding fixture failed");
    AnalyticFilteredGeometry forged_kind = *mixed_lowered.value;
    auto forged_arc = std::find_if(
        forged_kind.occurrences.begin(), forged_kind.occurrences.end(),
        [](const auto& occurrence)
        {
            return occurrence.source.role == AnalyticFilteredSourceRole::swept_left_offset_arc ||
                   occurrence.source.role == AnalyticFilteredSourceRole::swept_right_offset_arc;
        });
    require(forged_arc != forged_kind.occurrences.end(),
            "mixed swept fixture emitted no arc offset source");
    forged_arc->source.secondary_id = std::uint64_t{1} << 32U;
    const auto mixed_broad = build_analytic_curve_candidates(forged_kind.bounds);
    require(
        build_analytic_filtered_job_records(mixed_path, 0, forged_kind, mixed_broad.pairs).error ==
            AnalyticFilteredPacketError::invalid_argument,
        "packet binding accepted an arc role naming a line segment ordinal");

    for (const auto [gap, expected] : {std::pair<std::int64_t, AnalyticFilteredLoweringError>{
                                           251, AnalyticFilteredLoweringError::none},
                                       {250, AnalyticFilteredLoweringError::invalid_topology},
                                       {249, AnalyticFilteredLoweringError::invalid_topology}})
    {
        const auto u_path = swept_lines({{0, 0}, {1000, 0}, {1000, gap}, {0, gap}}, 200);
        const auto u_result = lower_analytic_job_to_filtered_curves(u_path, 0);
        require(u_result.error == expected && u_result.telemetry.algebraic_fallback_calls == 0,
                "U swept path drifted at the 49/50/51 nm fail-closed boundary gap=" +
                    std::to_string(gap) +
                    " error=" + std::to_string(static_cast<int>(u_result.error)));
    }
    const auto separated_u = lower_analytic_job_to_filtered_curves(
        swept_lines({{0, 0}, {1000, 0}, {1000, 1000}, {0, 1000}}, 200), 0);
    require(separated_u.error == AnalyticFilteredLoweringError::none && separated_u.value,
            "separated U swept path did not preserve its gap error=" +
                std::to_string(static_cast<int>(separated_u.error)));
    for (const auto [separation, expected] :
         {std::pair<std::int64_t, AnalyticFilteredLoweringError>{
              199, AnalyticFilteredLoweringError::invalid_topology},
          {200, AnalyticFilteredLoweringError::invalid_topology},
          {251, AnalyticFilteredLoweringError::none}})
    {
        for (const auto& path : {
                 swept_lines({{0, 0}, {1000, 0}, {1000, separation}, {0, separation}}, 200),
                 swept_lines({{0, 0}, {0, 1000}, {-separation, 1000}, {-separation, 0}}, 200),
             })
        {
            const auto result = lower_analytic_job_to_filtered_curves(path, 0);
            require(result.error == expected &&
                        (expected != AnalyticFilteredLoweringError::none || result.value) &&
                        result.telemetry.algebraic_fallback_calls == 0,
                    "U swept overlap/contact/gap transform drifted at boundary delta=" +
                        std::to_string(separation - 200) +
                        " error=" + std::to_string(static_cast<int>(result.error)));
        }
    }

    for (const auto& invalid : {
             swept_lines({{0, 0}, {1000, 0}, {0, 0}}),
             swept_lines({{0, 0}, {1000, 1000}, {0, 1000}, {1000, 0}}),
         })
        require(lower_analytic_job_to_filtered_curves(invalid, 0).error ==
                    AnalyticFilteredLoweringError::invalid_topology,
                "invalid swept centerline topology was accepted");
    auto narrow_arc = swept_lines({{0, 0}, {100, 0}}, 200);
    narrow_arc.segments[0].kind = 2;
    narrow_arc.segments[0].direction = 1;
    narrow_arc.segments[0].center_x_nm = 50;
    narrow_arc.segments[0].center_y_nm = 0;
    require(lower_analytic_job_to_filtered_curves(narrow_arc, 0).error ==
                AnalyticFilteredLoweringError::invalid_arc,
            "swept arc radius at or below half width was accepted");
    auto wrong_major = mixed_path;
    wrong_major.segments[1].major_arc = true;
    require(lower_analytic_job_to_filtered_curves(wrong_major, 0).error ==
                AnalyticFilteredLoweringError::invalid_arc,
            "swept arc accepted a noncanonical direction/major combination");
    auto unequal_radius = mixed_path;
    unequal_radius.vertices[2].x_nm = 2100;
    require(lower_analytic_job_to_filtered_curves(unequal_radius, 0).error ==
                AnalyticFilteredLoweringError::invalid_arc,
            "swept arc accepted unequal endpoint radii");
    auto clockwise_minor = swept_lines({{0, 1000}, {1000, 0}}, 200);
    clockwise_minor.segments[0].kind = 2;
    clockwise_minor.segments[0].direction = 2;
    clockwise_minor.segments[0].center_x_nm = 0;
    clockwise_minor.segments[0].center_y_nm = 0;
    require(lower_analytic_job_to_filtered_curves(clockwise_minor, 0).error ==
                AnalyticFilteredLoweringError::none,
            "canonical clockwise minor swept arc failed");
    require_swept_production_success(clockwise_minor,
                                     "canonical clockwise minor production swept arc");
    auto counterclockwise_major = clockwise_minor;
    counterclockwise_major.segments[0].direction = 1;
    counterclockwise_major.segments[0].major_arc = true;
    require(lower_analytic_job_to_filtered_curves(counterclockwise_major, 0).error ==
                AnalyticFilteredLoweringError::none,
            "canonical counterclockwise major swept arc failed");
    require_swept_production_success(counterclockwise_major,
                                     "canonical counterclockwise major production swept arc");

    require_swept_production_success(swept_lines({{0, 0}, {1'000'000, 0}, {2'000'000, 0}}, 150'001),
                                     "split same-carrier straight swept path");

    const auto arc_path =
        [](std::initializer_list<AnalyticIntegerPointNm> points, std::initializer_list<bool> major)
    {
        auto records = swept_lines(points, 150'000);
        std::size_t index = 0;
        for (const bool is_major : major)
        {
            records.segments[index].kind = 2;
            records.segments[index].direction = 1;
            records.segments[index].major_arc = is_major;
            records.segments[index].center_x_nm = 0;
            records.segments[index].center_y_nm = 0;
            ++index;
        }
        return records;
    };
    require_swept_production_success(
        arc_path({{1'000'000, 0}, {0, 1'000'000}, {-1'000'000, 0}}, {false, false}),
        "split same-circle CCW arc swept path");
    require_swept_production_success(arc_path({{1'000'000, 0}, {-1'000'000, 0}}, {false}),
                                     "CCW semicircle swept path");
    auto clockwise_semicircle = arc_path({{1'000'000, 0}, {-1'000'000, 0}}, {false});
    clockwise_semicircle.segments[0].direction = 2;
    require_swept_production_success(clockwise_semicircle, "clockwise semicircle swept path");
    auto clockwise_major = arc_path({{1'000'000, 0}, {0, 1'000'000}}, {true});
    clockwise_major.segments[0].direction = 2;
    require_swept_production_success(clockwise_major, "clockwise major swept path");
    require_swept_production_success(
        arc_path({{1'000'000, 0}, {0, -1'000'000}, {600'000, -800'000}}, {true, false}),
        "legal major-to-minor same-circle swept path");
    require_swept_production_success(
        arc_path({{1'000'000, 0}, {600'000, 800'000}, {600'000, -800'000}}, {false, true}),
        "legal minor-to-major same-circle swept path");
    const auto overlapping_same_circle =
        arc_path({{1'000'000, 0}, {0, -1'000'000}, {0, 1'000'000}}, {true, false});
    require(lower_analytic_job_to_filtered_curves(overlapping_same_circle, 0).error ==
                AnalyticFilteredLoweringError::invalid_topology,
            "overlapping adjacent same-circle arc domains were accepted");

    auto line_arc_kink = swept_lines({{0, 0}, {1'000'000, 0}, {3'000'000, 0}}, 150'001);
    line_arc_kink.segments[1].kind = 2;
    line_arc_kink.segments[1].direction = 2;
    line_arc_kink.segments[1].center_x_nm = 2'000'000;
    line_arc_kink.segments[1].center_y_nm = -1'000'000;
    require_swept_production_success(line_arc_kink, "line-to-arc kink swept path");
    auto arc_arc_tangent =
        arc_path({{1'000'000, 0}, {0, 1'000'000}, {-2'000'000, -1'000'000}}, {false, false});
    arc_arc_tangent.segments[1].center_y_nm = -1'000'000;
    require_swept_production_success(arc_arc_tangent,
                                     "distinct-carrier tangent arc-to-arc swept path");
    auto arc_arc_kink =
        arc_path({{1'000'000, 0}, {0, 1'000'000}, {1'000'000, 2'000'000}}, {false, false});
    arc_arc_kink.segments[1].direction = 2;
    arc_arc_kink.segments[1].center_x_nm = 1'000'000;
    arc_arc_kink.segments[1].center_y_nm = 1'000'000;
    require_swept_production_success(arc_arc_kink, "arc-to-arc kink swept path");

    auto symbolic_concentric = swept_lines({{1000, 1}, {-1, 1000}, {-1, 1100}, {1100, 1}}, 200);
    symbolic_concentric.segments[0] = {10, 110, 2, 1, false, 0, 0};
    symbolic_concentric.segments[2] = {12, 112, 2, 2, false, 0, 0};
    const auto symbolic_result = lower_analytic_job_to_filtered_curves(symbolic_concentric, 0);
    require(symbolic_result.error != AnalyticFilteredLoweringError::none &&
                !symbolic_result.value && symbolic_result.telemetry.algebraic_fallback_calls == 0,
            "overlapping concentric symbolic offset radii did not fail closed error=" +
                std::to_string(static_cast<int>(symbolic_result.error)));

    const auto horizontal = lower_analytic_job_to_filtered_curves(swept, 0);
    require(horizontal.error == AnalyticFilteredLoweringError::none && horizontal.value,
            "horizontal singleton-certificate fixture failed");
    bool has_certified_line = false;
    for (const auto& curve : horizontal.value->curves)
        has_certified_line = has_certified_line || (curve.kind == AnalyticAtomicCurveKind::line &&
                                                    curve.has_integer_certificate);
    require(has_certified_line,
            "mathematically singleton integer swept endpoints were not certified");

    const auto diagonal = swept_lines({{0, 0}, {1000, 1000}}, 201);
    const auto diagonal_result = lower_analytic_job_to_filtered_curves(diagonal, 0);
    require(diagonal_result.error == AnalyticFilteredLoweringError::none && diagonal_result.value &&
                diagonal_result.telemetry.algebraic_fallback_calls == 0 &&
                std::none_of(diagonal_result.value->curves.begin(),
                             diagonal_result.value->curves.end(),
                             [](const auto& curve) { return curve.has_integer_certificate; }),
            "non-singleton swept construction was rounded/promoted to an integer certificate");

    require_swept_production_success(swept_lines({{0, 0}, {2'000'000, 2'000'000}}, 150'000),
                                     "150000 nm diagonal swept path");
    require_swept_production_success(
        swept_lines({{0, 0}, {2'000'000, 0}, {4'000'000, 2'000'000}}, 150'001),
        "150001 nm horizontal-to-45 swept path");
    require_swept_production_success(
        swept_lines({{0, 0}, {2'000'000, 2'000'000}, {4'000'000, 2'000'000}}, 550'000),
        "550000 nm 45-to-horizontal swept path");

    const std::uint64_t exact_work = horizontal.telemetry.work_units;
    AnalyticSolverLimits limits = kAnalyticSolverHardLimits;
    limits.predicate_calls = exact_work;
    auto limited = lower_analytic_job_to_filtered_curves(swept, 0, limits);
    require(limited.error == AnalyticFilteredLoweringError::none,
            "exact swept work budget was rejected");
    --limits.predicate_calls;
    limited = lower_analytic_job_to_filtered_curves(swept, 0, limits);
    require(limited.error == AnalyticFilteredLoweringError::resource_limit_exceeded,
            "one-unit-short swept work budget was accepted");
    std::uint64_t memory_low = 0;
    std::uint64_t memory_high = 1'000'000;
    while (memory_low < memory_high)
    {
        const std::uint64_t middle = memory_low + (memory_high - memory_low) / 2;
        limits = kAnalyticSolverHardLimits;
        limits.working_memory_bytes = middle;
        limited = lower_analytic_job_to_filtered_curves(swept, 0, limits);
        if (limited.error == AnalyticFilteredLoweringError::resource_limit_exceeded)
            memory_low = middle + 1;
        else
            memory_high = middle;
    }
    limits = kAnalyticSolverHardLimits;
    limits.working_memory_bytes = memory_low;
    limited = lower_analytic_job_to_filtered_curves(swept, 0, limits);
    require(limited.error == AnalyticFilteredLoweringError::none,
            "exact swept memory boundary was rejected");
    --limits.working_memory_bytes;
    limited = lower_analytic_job_to_filtered_curves(swept, 0, limits);
    require(limited.error == AnalyticFilteredLoweringError::resource_limit_exceeded,
            "one-byte-short swept memory budget was accepted");
    require(limited.telemetry.required_working_memory_bytes == memory_low,
            "one-byte-short swept memory telemetry did not report the exact admitted boundary");

    auto mixed_memory = swept;
    mixed_memory.stages[0].operand_count = 4;
    mixed_memory.operands = {{997, 4, 0}, {998, 4, 1}, {999, 4, 2}, {1000, 5, 0}};
    mixed_memory.capsules = {{7997, -3000, 0, -2000, 0, 100},
                             {7998, 2000, 0, 3000, 0, 100},
                             {7999, 4000, 0, 5000, 0, 100}};
    const auto mixed_unlimited = lower_analytic_job_to_filtered_curves(mixed_memory, 0);
    require(mixed_unlimited.error == AnalyticFilteredLoweringError::none && mixed_unlimited.value,
            "mixed primitive/swept allocation fixture failed");
    memory_low = 0;
    memory_high = 1'000'000;
    while (memory_low < memory_high)
    {
        const std::uint64_t middle = memory_low + (memory_high - memory_low) / 2;
        limits = kAnalyticSolverHardLimits;
        limits.working_memory_bytes = middle;
        limited = lower_analytic_job_to_filtered_curves(mixed_memory, 0, limits);
        if (limited.error == AnalyticFilteredLoweringError::resource_limit_exceeded)
            memory_low = middle + 1;
        else
            memory_high = middle;
    }
    limits = kAnalyticSolverHardLimits;
    limits.working_memory_bytes = memory_low - 1;
    limited = lower_analytic_job_to_filtered_curves(mixed_memory, 0, limits);
    require(limited.error == AnalyticFilteredLoweringError::resource_limit_exceeded &&
                limited.telemetry.required_working_memory_bytes == memory_low,
            "mixed primitive/swept parent-child overlap was not exactly pre-admitted");

    memory_low = 0;
    memory_high = 2'000'000;
    while (memory_low < memory_high)
    {
        const std::uint64_t middle = memory_low + (memory_high - memory_low) / 2;
        limits = kAnalyticSolverHardLimits;
        limits.working_memory_bytes = middle;
        limited = lower_analytic_job_to_filtered_curves(l_path, 0, limits);
        if (limited.error == AnalyticFilteredLoweringError::resource_limit_exceeded)
            memory_low = middle + 1;
        else
            memory_high = middle;
    }
    limits = kAnalyticSolverHardLimits;
    limits.working_memory_bytes = memory_low - 1;
    limited = lower_analytic_job_to_filtered_curves(l_path, 0, limits);
    require(limited.error == AnalyticFilteredLoweringError::resource_limit_exceeded &&
                limited.telemetry.required_working_memory_bytes == memory_low,
            "high-discard swept child capacity did not report its exact one-byte-short boundary");
}

void test_matz_primitive_family_production()
{
    AnalyticRequestPacketRecords records;
    records.jobs = {{3, 0, 2}};
    records.stages = {{301, 1, 0, 4}, {302, 2, 4, 2}};
    records.operands = {{3001, 2, 0}, {3002, 3, 0}, {3003, 4, 0},
                        {3004, 5, 0}, {3005, 2, 1}, {3006, 4, 1}};
    records.disks = {{3101, 0, 0, 1'500'000}, {3501, 0, 0, 500'000}};
    records.annuli = {{3201, 5'000'000, 0, 900'000, 1'800'000}};
    records.capsules = {{3301, 8'000'000, -1'000'000, 12'000'000, 1'000'000, 1'800'000},
                        {3601, 9'500'000, -500'000, 10'500'000, 500'000, 600'000}};
    records.swept_paths = {{3401, 0, 1'200'000}};
    records.rings = {{3402, 0, 3, 0, 2, 1}};
    records.vertices = {
        {34'001, 14'000'000, 0}, {34'002, 17'000'000, 0}, {34'003, 19'000'000, 2'000'000}};
    records.segments = {{34'101, 34'201, 1, 0, false, 0, 0},
                        {34'102, 34'202, 2, 1, false, 17'000'000, 2'000'000}};
    require(validate_analytic_request_packet_records(records) == AnalyticRequestPacketError::none,
            "MATZ primitive-family request is not packet-valid");
    AnalyticRequestPacketRecords without_swept = records;
    without_swept.stages = {{301, 1, 0, 3}, {302, 2, 3, 2}};
    without_swept.operands.erase(without_swept.operands.begin() + 3);
    without_swept.swept_paths.clear();
    without_swept.rings.clear();
    without_swept.vertices.clear();
    without_swept.segments.clear();
    require(validate_analytic_request_packet_records(without_swept) ==
                AnalyticRequestPacketError::none,
            "MATZ primitive-family control is not packet-valid");
    const auto control_lowered = lower_analytic_job_to_filtered_curves(without_swept, 0);
    require(control_lowered.error == AnalyticFilteredLoweringError::none &&
                control_lowered.value.has_value(),
            "MATZ primitive-family control lowering failed");
    const auto control_broad = build_analytic_curve_candidates(control_lowered.value->bounds);
    const auto control_normalization = build_analytic_filtered_normalization(
        without_swept, 0, *control_lowered.value, control_broad.pairs);
    require(control_normalization.error == AnalyticFilteredNormalizationError::none,
            "MATZ primitive-family no-swept control normalization failed error=" +
                std::to_string(static_cast<int>(control_normalization.error)) + " strict_pairs=" +
                std::to_string(control_normalization.telemetry.strict_replay_candidate_pairs) +
                " fragments=" + std::to_string(control_normalization.fragments.size()) +
                " required=" +
                std::to_string(control_normalization.telemetry.required_working_memory_bytes) +
                " work=" + std::to_string(control_normalization.telemetry.predicate_calls));
    const auto lowered = lower_analytic_job_to_filtered_curves(records, 0);
    require(lowered.error == AnalyticFilteredLoweringError::none && lowered.value.has_value() &&
                lowered.telemetry.algebraic_fallback_calls == 0,
            "MATZ primitive-family production lowering failed");
    const auto broad = build_analytic_curve_candidates(lowered.value->bounds);
    require(broad.error == AnalyticBroadPhaseError::none,
            "MATZ primitive-family production broad phase failed");
    const auto packet =
        build_analytic_filtered_job_records(records, 0, *lowered.value, broad.pairs);
    require(packet.error == AnalyticFilteredPacketError::none && packet.records.has_value() &&
                packet.records->job_results.size() == 1 &&
                packet.records->job_results[0].status == 0 &&
                packet.telemetry.algebraic_fallback_calls == 0,
            "MATZ primitive-family production packet failed");
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
        append_u64(curve.start.construction_x_column_id);
        append_double(curve.end.x.lower);
        append_double(curve.end.x.upper);
        append_double(curve.end.y.lower);
        append_double(curve.end.y.upper);
        append_u64(curve.end.construction_x_column_id);
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
        append_u64(curve.has_construction_line_direction ? 1U : 0U);
        append_u64(static_cast<std::uint64_t>(curve.construction_line_dx));
        append_u64(static_cast<std::uint64_t>(curve.construction_line_dy));
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
    test_endpoint_radius_authored_minor_and_major_arcs();
    test_disks_annuli_and_tokens();
    test_arbitrary_capsule();
    test_capsule_carrier_proofs();
    test_arc_tight_bounds_and_sparse_scaling();
    test_empty_jobs_radius_domain_and_global_expansion();
    test_fail_closed_limits_and_swept_path();
    test_swept_path_staged_contracts();
    test_matz_primitive_family_production();
    std::cout << "ANALYTIC_FILTERED_LOWERING_VECTOR=" << lowering_parity_vector() << '\n';
    return 0;
}

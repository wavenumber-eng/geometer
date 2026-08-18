#include "geometer/analytic_curve_broad_phase.h"
#include "geometer/analytic_filtered_normalization.h"

#include "analytic_endpoint_arc_reconstruction.h"
#include "analytic_filtered_normalization_arc_certificate.h"
#include "analytic_filtered_normalization_replay.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
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

void test_near_coincident_arc_certificate_guards()
{
    using analytic_detail::exact;
    using analytic_detail::Point;
    using analytic_normalization_detail::ArcConstruction;
    using analytic_normalization_detail::certifies_near_coincident_arc;
    const auto point = [](double x, double y) -> Point { return {exact(x), exact(y)}; };

    const ArcConstruction source{point(0.0, 0.0),
                                 exact(500'000.0),
                                 point(-500'000.0, 0.0),
                                 point(0.0, -500'000.0),
                                 true,
                                 false};
    require(certifies_near_coincident_arc(source, source),
            "exact matching arc construction was not certified");
    ArcConstruction target = source;
    target.center = point(0.25, -0.25);
    target.radius = exact(500'000.5);
    target.start = point(-499'999.5, 0.0);
    target.end = point(0.0, -499'999.5);
    require(certifies_near_coincident_arc(source, target),
            "near-coincident capsule arc was not certified");

    ArcConstruction invalid = target;
    invalid.center = point(17.0, 0.0);
    require(!certifies_near_coincident_arc(source, invalid),
            "displaced arc center was incorrectly certified");
    invalid = target;
    invalid.radius = exact(500'017.0);
    require(!certifies_near_coincident_arc(source, invalid),
            "wrong arc radius was incorrectly certified");
    invalid = target;
    invalid.counterclockwise = false;
    require(!certifies_near_coincident_arc(source, invalid),
            "wrong arc direction was incorrectly certified");
    invalid = target;
    invalid.major_arc = true;
    require(!certifies_near_coincident_arc(source, invalid),
            "wrong arc branch was incorrectly certified");
    invalid = target;
    invalid.start = point(-499'983.0, 0.0);
    require(!certifies_near_coincident_arc(source, invalid),
            "displaced arc endpoint was incorrectly certified");
}

AnalyticFilteredPointNm exact_point(double x, double y)
{
    return {{x, x}, {y, y}};
}

AnalyticAtomicCurveNm endpoint_authoritative_arc(std::uint32_t index, std::int64_t start_x,
                                                 std::int64_t start_y, std::int64_t end_x,
                                                 std::int64_t end_y, std::uint64_t radius,
                                                 bool counterclockwise, bool upper_branch,
                                                 bool x_monotone = true)
{
    AnalyticAtomicCurveNm curve;
    curve.curve_index = index;
    curve.kind = AnalyticAtomicCurveKind::circular_arc;
    curve.start = exact_point(static_cast<double>(start_x), static_cast<double>(start_y));
    curve.end = exact_point(static_cast<double>(end_x), static_cast<double>(end_y));
    curve.integer_start = {start_x, start_y};
    curve.integer_end = {end_x, end_y};
    curve.circle.radius = {static_cast<double>(radius), static_cast<double>(radius)};
    curve.counterclockwise = counterclockwise;
    curve.has_integer_radius_certificate = true;
    curve.integer_radius = radius;
    curve.has_arc_sweep_certificate = true;
    curve.has_endpoint_authoritative_arc_certificate = true;
    curve.has_endpoint_authoritative_x_monotone_certificate = x_monotone;
    curve.endpoint_authoritative_upper_branch = upper_branch;
    analytic_detail::Point center;
    require(analytic_detail::reconstruct_endpoint_authoritative_arc_center(
                start_x, start_y, end_x, end_y, radius, counterclockwise, false, center),
            "replay test center reconstruction failed");
    curve.circle.center = {{center.x.lower, center.x.upper}, {center.y.lower, center.y.upper}};
    return curve;
}

AnalyticCurveBoundsNm circle_bounds(const AnalyticAtomicCurveNm& curve)
{
    return {curve.curve_index, curve.circle.center.x.lower - curve.circle.radius.upper,
            curve.circle.center.y.lower - curve.circle.radius.upper,
            curve.circle.center.x.upper + curve.circle.radius.upper,
            curve.circle.center.y.upper + curve.circle.radius.upper};
}

void reverse_arc(AnalyticAtomicCurveNm& curve)
{
    std::swap(curve.start, curve.end);
    std::swap(curve.integer_start, curve.integer_end);
    std::swap(curve.construction_start_tangent_id, curve.construction_end_tangent_id);
    curve.counterclockwise = !curve.counterclockwise;
}

AnalyticRequestPacketRecords records_for(std::uint64_t operand)
{
    AnalyticRequestPacketRecords records;
    records.jobs.push_back({1, 0, 1});
    records.stages.push_back({1, 1, 0, 1});
    records.operands.push_back({operand, 2, 0});
    return records;
}

AnalyticRequestPacketRecords records_for_two(std::uint64_t first, std::uint64_t second)
{
    AnalyticRequestPacketRecords records;
    records.jobs.push_back({1, 0, 1});
    records.stages.push_back({1, 1, 0, 2});
    records.operands.push_back({first, 2, 0});
    records.operands.push_back({second, 2, 0});
    return records;
}

AnalyticRequestPacketRecords nested_records()
{
    AnalyticRequestPacketRecords records;
    records.jobs.push_back({1, 0, 3});
    records.stages.push_back({1, 1, 0, 1});
    records.stages.push_back({2, 2, 1, 1});
    records.stages.push_back({3, 1, 2, 1});
    records.operands.push_back({1, 2, 0});
    records.operands.push_back({2, 2, 0});
    records.operands.push_back({3, 2, 0});
    return records;
}

AnalyticRequestPacketRecords difference_records()
{
    AnalyticRequestPacketRecords records;
    records.jobs.push_back({1, 0, 2});
    records.stages.push_back({1, 1, 0, 1});
    records.stages.push_back({2, 2, 1, 1});
    records.operands.push_back({1, 2, 0});
    records.operands.push_back({2, 2, 0});
    return records;
}

AnalyticRequestPacketRecords union_records(std::uint32_t count)
{
    AnalyticRequestPacketRecords records;
    records.jobs.push_back({1, 0, 1});
    records.stages.push_back({1, 1, 0, count});
    for (std::uint32_t index = 0; index < count; ++index)
        records.operands.push_back({index + 1, 2, 0});
    return records;
}

void append_line(AnalyticFilteredGeometry& geometry, std::uint64_t operand, double x1, double y1,
                 double x2, double y2)
{
    const std::uint32_t index = static_cast<std::uint32_t>(geometry.curves.size() + 1);
    AnalyticAtomicCurveNm curve;
    curve.curve_index = index;
    curve.start = exact_point(x1, y1);
    curve.end = exact_point(x2, y2);
    curve.has_integer_certificate = true;
    curve.integer_start = {static_cast<std::int64_t>(x1), static_cast<std::int64_t>(y1)};
    curve.integer_end = {static_cast<std::int64_t>(x2), static_cast<std::int64_t>(y2)};
    curve.construction_carrier_id = 1000 + index;
    curve.construction_family_id = 2000 + index;
    curve.has_construction_line_direction = true;
    const std::int64_t dx = static_cast<std::int64_t>(x2 - x1);
    const std::int64_t dy = static_cast<std::int64_t>(y2 - y1);
    const bool canonical = dx > 0 || (dx == 0 && dy > 0);
    curve.construction_line_dx = canonical ? dx : -dx;
    curve.construction_line_dy = canonical ? dy : -dy;
    geometry.curves.push_back(curve);
    geometry.bounds.push_back(
        {index, std::min(x1, x2), std::min(y1, y2), std::max(x1, x2), std::max(y1, y2)});
    AnalyticFilteredOccurrence occurrence;
    occurrence.occurrence_id = index;
    occurrence.coverage_id = operand;
    occurrence.agrees_with_carrier = canonical;
    occurrence.material_on_left = true;
    occurrence.source.kind = AnalyticFilteredSourceKind::authored_segment_curve;
    occurrence.source.role = AnalyticFilteredSourceRole::authored_line;
    occurrence.source.operand_id = operand;
    occurrence.source.primary_id = 10000 + index;
    occurrence.source.secondary_id = 20000 + index;
    geometry.occurrences.push_back(occurrence);
}

void append_box(AnalyticFilteredGeometry& geometry, std::uint64_t operand, double min_x,
                double min_y, double max_x, double max_y)
{
    append_line(geometry, operand, min_x, min_y, max_x, min_y);
    append_line(geometry, operand, max_x, min_y, max_x, max_y);
    append_line(geometry, operand, max_x, max_y, min_x, max_y);
    append_line(geometry, operand, min_x, max_y, min_x, min_y);
}

void append_arc(AnalyticFilteredGeometry& geometry, std::uint64_t operand, double start_x,
                double start_y, double end_x, double end_y, double center_x, double center_y,
                double radius, bool counterclockwise = true, bool major_arc = false)
{
    const std::uint32_t index = static_cast<std::uint32_t>(geometry.curves.size() + 1);
    AnalyticAtomicCurveNm curve;
    curve.curve_index = index;
    curve.kind = AnalyticAtomicCurveKind::circular_arc;
    curve.start = exact_point(start_x, start_y);
    curve.end = exact_point(end_x, end_y);
    curve.circle.center = exact_point(center_x, center_y);
    curve.circle.radius = {radius, radius};
    curve.counterclockwise = counterclockwise;
    curve.major_arc = major_arc;
    curve.has_integer_certificate = true;
    curve.integer_start = {static_cast<std::int64_t>(start_x), static_cast<std::int64_t>(start_y)};
    curve.integer_end = {static_cast<std::int64_t>(end_x), static_cast<std::int64_t>(end_y)};
    curve.integer_center = {static_cast<std::int64_t>(center_x),
                            static_cast<std::int64_t>(center_y)};
    curve.has_integer_radius_certificate = true;
    curve.integer_radius = static_cast<std::uint64_t>(radius);
    curve.construction_carrier_id = 1000 + operand;
    curve.construction_family_id = 2000 + operand;
    curve.has_arc_sweep_certificate = true;
    geometry.curves.push_back(curve);
    geometry.bounds.push_back(
        {index, center_x - radius, center_y - radius, center_x + radius, center_y + radius});
    AnalyticFilteredOccurrence occurrence;
    occurrence.occurrence_id = index;
    occurrence.coverage_id = operand;
    occurrence.agrees_with_carrier = counterclockwise;
    occurrence.material_on_left = true;
    occurrence.source.kind = AnalyticFilteredSourceKind::compact_feature_role;
    occurrence.source.role = AnalyticFilteredSourceRole::primitive_outer_circle;
    occurrence.source.operand_id = operand;
    occurrence.source.primary_id = 1;
    occurrence.source.secondary_id = 0;
    geometry.occurrences.push_back(occurrence);
}

void append_irrational_arc(AnalyticFilteredGeometry& geometry, std::uint64_t operand,
                           bool counterclockwise)
{
    const double radius = std::sqrt(818.0);
    append_arc(geometry, operand, 23, 17, -23, -17, 0, 0, radius, counterclockwise);
    auto& curve = geometry.curves.back();
    curve.has_integer_certificate = false;
    curve.has_integer_radius_certificate = false;
    curve.circle.radius = {std::nextafter(radius, -std::numeric_limits<double>::infinity()),
                           std::nextafter(radius, std::numeric_limits<double>::infinity())};
    geometry.bounds.back() = {curve.curve_index, -radius, -radius, radius, radius};
}

void append_disk(AnalyticFilteredGeometry& geometry, std::uint64_t operand, double center_x,
                 double center_y, double radius)
{
    append_arc(geometry, operand, center_x + radius, center_y, center_x - radius, center_y,
               center_x, center_y, radius);
    append_arc(geometry, operand, center_x - radius, center_y, center_x + radius, center_y,
               center_x, center_y, radius);
}

AnalyticFilteredNormalizationResult normalize(const AnalyticFilteredGeometry& geometry,
                                              std::uint64_t operand = 1,
                                              const AnalyticSolverLimits& limits = {})
{
    const auto broad = build_analytic_curve_candidates(geometry.bounds, limits);
    require(broad.error == AnalyticBroadPhaseError::none, "normalization broad phase failed");
    return build_analytic_filtered_normalization(records_for(operand), 0, geometry, broad.pairs,
                                                 limits);
}

void test_integer_box()
{
    AnalyticFilteredGeometry geometry;
    append_box(geometry, 1, 0, 0, 1000, 1000);
    const auto result = normalize(geometry);
    require(result.error == AnalyticFilteredNormalizationError::none,
            "integer box normalization failed: " +
                std::to_string(static_cast<std::uint32_t>(result.error)) + "/" +
                std::to_string(result.telemetry.predicate_calls));
    require(result.vertices.size() == 4 && result.fragments.size() == 4 &&
                result.rings.size() == 1 && result.regions.size() == 1,
            "integer box normalization topology drifted");
    require(result.telemetry.algebraic_fallback_calls == 0,
            "integer box normalization called the algebraic solver");
}

void test_integer_disk()
{
    AnalyticFilteredGeometry geometry;
    append_disk(geometry, 1, 0, 0, 1000);
    const auto result = normalize(geometry);
    require(result.error == AnalyticFilteredNormalizationError::none,
            "integer two-half disk normalization failed: " +
                std::to_string(static_cast<std::uint32_t>(result.error)));
    require(result.vertices.size() == 2 && result.fragments.size() == 2 &&
                result.fragments[0].radius_nm == 1000 && result.fragments[1].radius_nm == 1000 &&
                result.rings.size() == 1 && result.regions.size() == 1,
            "integer disk normalization topology or radius drifted");
}

void test_rotated_semicircles_and_major_arc()
{
    AnalyticFilteredGeometry rotated;
    append_arc(rotated, 1, 300, 400, -300, -400, 0, 0, 500);
    append_arc(rotated, 1, -300, -400, 300, 400, 0, 0, 500);
    const auto rotated_result = normalize(rotated);
    require(rotated_result.error == AnalyticFilteredNormalizationError::none &&
                rotated_result.fragments.size() == 4 &&
                rotated_result.fragments[0].radius_nm == 500,
            "rotated semicircle normalization failed: " +
                std::to_string(static_cast<std::uint32_t>(rotated_result.error)) + "/" +
                std::to_string(rotated_result.fragments.size()) + "/" +
                std::to_string(rotated_result.telemetry.arc_critical_candidates) + "/" +
                std::to_string(rotated_result.telemetry.strict_replay_candidate_pairs) + "/" +
                std::to_string(rotated_result.telemetry.outcomes_work_units) + "/" +
                std::to_string(rotated_result.telemetry.normalization_work_units));

    AnalyticFilteredGeometry major;
    append_arc(major, 1, 1000, 0, 0, 1000, 1000, 1000, 1000, true, true);
    append_line(major, 1, 0, 1000, 1000, 0);
    const auto major_result = normalize(major);
    require(major_result.error == AnalyticFilteredNormalizationError::none &&
                major_result.fragments.size() >= 3 && major_result.rings.size() == 1,
            "major-arc normalization failed: " +
                std::to_string(static_cast<std::uint32_t>(major_result.error)));
}

void test_irrational_disk_crossings()
{
    AnalyticFilteredGeometry geometry;
    append_disk(geometry, 1, 0, 0, 1000);
    append_disk(geometry, 2, 1201, 0, 1000);
    const auto broad = build_analytic_curve_candidates(geometry.bounds);
    require(broad.error == AnalyticBroadPhaseError::none, "overlapping-disk broad phase failed");
    const auto result =
        build_analytic_filtered_normalization(records_for_two(1, 2), 0, geometry, broad.pairs);
    require(result.error == AnalyticFilteredNormalizationError::none,
            "irrational disk-crossing normalization failed: " +
                std::to_string(static_cast<std::uint32_t>(result.error)) + "/" +
                std::to_string(result.telemetry.arc_critical_candidates) + "/" +
                std::to_string(result.telemetry.strict_replay_candidate_pairs) + "/" +
                std::to_string(result.telemetry.outcomes_work_units) + "/" +
                std::to_string(result.telemetry.normalization_work_units) + "/" +
                std::to_string(result.telemetry.reserved_normalization_work_units));
    require(result.regions.size() == 1 && result.rings.size() == 1 && result.vertices.size() == 4 &&
                result.fragments.size() == 4 && result.telemetry.algebraic_fallback_calls == 0,
            "irrational disk union topology drifted or used algebraic fallback");
}

void test_successful_empty_difference_skips_geometry_replay()
{
    AnalyticFilteredGeometry geometry;
    append_disk(geometry, 1, 0, 0, 1'000);
    append_disk(geometry, 2, 0, 0, 1'200);
    const auto broad = build_analytic_curve_candidates(geometry.bounds);
    require(broad.error == AnalyticBroadPhaseError::none, "empty-difference broad phase failed");
    const auto result =
        build_analytic_filtered_normalization(difference_records(), 0, geometry, broad.pairs);
    require(result.error == AnalyticFilteredNormalizationError::none && result.vertices.empty() &&
                result.fragments.empty() && result.rings.empty() && result.regions.empty() &&
                result.outcomes.lineage.boundaries.empty() &&
                result.outcomes.lineage.regions.rings.empty() &&
                result.outcomes.lineage.regions.regions.empty() &&
                result.outcomes.result_references.empty(),
            "successful empty difference was rejected by strict replay");
    require(result.outcomes.events.size() == 2 &&
                result.outcomes.events[0].kind ==
                    AnalyticOperandOutcomeKind::completely_removed_later &&
                result.outcomes.events[1].kind ==
                    AnalyticOperandOutcomeKind::subtraction_effect_survives,
            "successful empty difference lost its operand outcomes");
}

void test_collapsed_only_empty_output_fails_closed()
{
    AnalyticFilteredGeometry geometry;
    append_line(geometry, 1, 0, 0, 30, 40);
    const auto broad = build_analytic_curve_candidates(geometry.bounds);
    require(broad.error == AnalyticBroadPhaseError::none,
            "collapsed-only empty-output broad phase failed");
    const auto result =
        build_analytic_filtered_normalization(records_for(1), 0, geometry, broad.pairs);
    require(result.error == AnalyticFilteredNormalizationError::normalization_topology_collapse &&
                result.vertices.empty() && result.fragments.empty() && result.rings.empty() &&
                result.regions.empty(),
            "collapsed-only topology was mistaken for an exact successful empty result");
}

void test_irrational_radius_arc_and_clockwise_hole()
{
    AnalyticFilteredGeometry half_disk;
    append_irrational_arc(half_disk, 1, true);
    append_line(half_disk, 1, -23, -17, 23, 17);
    const auto half_result = normalize(half_disk);
    require(half_result.error == AnalyticFilteredNormalizationError::none &&
                half_result.fragments.size() >= 2 &&
                half_result.telemetry.algebraic_fallback_calls == 0,
            "irrational-radius authored arc normalization failed: " +
                std::to_string(static_cast<std::uint32_t>(half_result.error)));

    AnalyticFilteredGeometry annulus;
    append_disk(annulus, 1, 0, 0, 2000);
    append_disk(annulus, 2, 0, 0, 1000);
    const auto broad = build_analytic_curve_candidates(annulus.bounds);
    require(broad.error == AnalyticBroadPhaseError::none, "annulus broad phase failed");
    const auto result =
        build_analytic_filtered_normalization(difference_records(), 0, annulus, broad.pairs);
    require(result.error == AnalyticFilteredNormalizationError::none && result.rings.size() == 2 &&
                result.regions.size() == 1 && !result.rings[1].counterclockwise,
            "clockwise hole strict replay failed: " +
                std::to_string(static_cast<std::uint32_t>(result.error)));
}

void verify_maps_and_tagged_references(const AnalyticFilteredNormalizationResult& result)
{
    for (std::uint32_t normalized = 0; normalized < result.vertices.size(); ++normalized)
    {
        const std::uint32_t old = result.vertices[normalized].arrangement_vertex;
        require(old < result.old_vertex_to_normalized.size() &&
                    result.old_vertex_to_normalized[old] == normalized,
                "vertex normalization map is not reciprocal");
    }
    for (std::uint32_t normalized = 0; normalized < result.fragments.size(); ++normalized)
    {
        const std::uint32_t old = result.fragments[normalized].old_boundary;
        require(old < result.old_boundary_to_normalized.size() &&
                    result.old_boundary_to_normalized[old] == normalized,
                "boundary normalization map is not reciprocal");
    }
    for (std::uint32_t normalized = 0; normalized < result.rings.size(); ++normalized)
    {
        const std::uint32_t old = result.rings[normalized].old_ring;
        require(old < result.old_ring_to_normalized.size() &&
                    result.old_ring_to_normalized[old] == normalized,
                "ring normalization map is not reciprocal");
    }
    for (std::uint32_t normalized = 0; normalized < result.regions.size(); ++normalized)
    {
        const std::uint32_t old = result.regions[normalized].old_region;
        require(old < result.old_region_to_normalized.size() &&
                    result.old_region_to_normalized[old] == normalized,
                "region normalization map is not reciprocal");
    }
    for (const auto& event : result.outcomes.events)
        for (std::uint32_t offset = 0; offset < event.result_references.count; ++offset)
        {
            const auto& reference =
                result.outcomes.result_references[event.result_references.begin + offset];
            const auto& map = reference.kind == AnalyticFilteredResultReferenceKind::ring
                                  ? result.old_ring_to_normalized
                                  : result.old_region_to_normalized;
            require(reference.local_index < map.size() &&
                        map[reference.local_index] != kNoAnalyticNormalizedIndex,
                    "tagged outcome reference does not close through normalization maps");
        }
}

void test_maps_and_unused_vertex_sentinel()
{
    AnalyticFilteredGeometry geometry;
    append_box(geometry, 1, 0, 0, 1000, 1000);
    append_line(geometry, 1, 2000, 0, 2030, 0);
    const auto result = normalize(geometry);
    require(result.error == AnalyticFilteredNormalizationError::none,
            "normalization map fixture failed");
    verify_maps_and_tagged_references(result);
    require(std::find(result.old_vertex_to_normalized.begin(),
                      result.old_vertex_to_normalized.end(),
                      kNoAnalyticNormalizedIndex) != result.old_vertex_to_normalized.end(),
            "unused collapsed vertex did not retain normalization sentinel");
}

void test_original_hard_limits_are_enforced()
{
    AnalyticFilteredGeometry geometry;
    append_box(geometry, 1, 0, 0, 1000, 1000);
    const auto broad = build_analytic_curve_candidates(geometry.bounds);
    require(broad.error == AnalyticBroadPhaseError::none, "hard-limit broad fixture failed");
    AnalyticSolverLimits limits;
    ++limits.predicate_calls;
    auto result =
        build_analytic_filtered_normalization(records_for(1), 0, geometry, broad.pairs, limits);
    require(result.error == AnalyticFilteredNormalizationError::resource_limit_exceeded &&
                result.telemetry.outcomes_work_units == 0,
            "over-hard predicate limit reached outcomes");
    limits = {};
    ++limits.working_memory_bytes;
    result =
        build_analytic_filtered_normalization(records_for(1), 0, geometry, broad.pairs, limits);
    require(result.error == AnalyticFilteredNormalizationError::resource_limit_exceeded &&
                result.telemetry.outcomes_work_units == 0,
            "over-hard memory limit reached outcomes");
}

void test_vertex_collision_fails_closed()
{
    AnalyticFilteredGeometry geometry;
    append_box(geometry, 1, 0, 0, 1000, 1000);
    geometry.curves[0].start.x = {0.49, 0.51};
    geometry.curves[3].end.x = {0.49, 0.51};
    const auto result = normalize(geometry);
    require(result.error != AnalyticFilteredNormalizationError::none && result.vertices.empty(),
            "uncertain normalization exposed partial output");
}

void test_global_half_ties()
{
    AnalyticFilteredGeometry geometry;
    geometry.origin_x_nm = 10;
    geometry.origin_y_nm = 20;
    append_box(geometry, 1, -0.5, -0.5, 999.5, 999.5);
    for (auto& curve : geometry.curves)
        curve.has_integer_certificate = false;
    const auto result = normalize(geometry);
    require(result.error == AnalyticFilteredNormalizationError::none,
            "global half-grid tie normalization failed");
    bool lower_left = false;
    bool upper_right = false;
    for (const auto& vertex : result.vertices)
    {
        lower_left = lower_left || (vertex.x_nm == 10 && vertex.y_nm == 20);
        upper_right = upper_right || (vertex.x_nm == 1010 && vertex.y_nm == 1020);
    }
    require(lower_left && upper_right,
            "half-grid ties were rounded locally instead of in global coordinates");
}

void test_nested_hole_and_island_replay()
{
    AnalyticFilteredGeometry geometry;
    append_box(geometry, 1, 0, 0, 4000, 4000);
    append_box(geometry, 2, 500, 500, 3500, 3500);
    append_box(geometry, 3, 1000, 1000, 3000, 3000);
    const auto broad = build_analytic_curve_candidates(geometry.bounds);
    require(broad.error == AnalyticBroadPhaseError::none, "nested replay broad phase failed");
    const auto result =
        build_analytic_filtered_normalization(nested_records(), 0, geometry, broad.pairs);
    require(result.error == AnalyticFilteredNormalizationError::none,
            "nested normalization replay failed: " +
                std::to_string(static_cast<std::uint32_t>(result.error)));
    require(result.rings.size() == 3 && result.regions.size() == 2,
            "nested normalization changed ring or region counts");
    require(result.rings[0].depth == 0 && result.rings[1].depth == 1 && result.rings[2].depth == 2,
            "nested normalization changed ring hierarchy");
}

void test_early_normalization_reservation()
{
    AnalyticFilteredGeometry geometry;
    append_box(geometry, 1, 0, 0, 1000, 1000);
    const auto baseline = normalize(geometry);
    require(baseline.error == AnalyticFilteredNormalizationError::none,
            "normalization reservation baseline failed");
    AnalyticSolverLimits limits;
    limits.predicate_calls = baseline.telemetry.reserved_normalization_work_units - 1;
    const auto short_work = normalize(geometry, 1, limits);
    require(short_work.error == AnalyticFilteredNormalizationError::resource_limit_exceeded &&
                short_work.telemetry.outcomes_work_units == 0 && short_work.vertices.empty(),
            "one-short normalization reservation reached outcomes");
    limits = {};
    limits.working_memory_bytes = baseline.telemetry.reserved_normalization_memory_bytes - 1;
    const auto short_memory = normalize(geometry, 1, limits);
    require(short_memory.error == AnalyticFilteredNormalizationError::resource_limit_exceeded &&
                short_memory.telemetry.outcomes_work_units == 0 && short_memory.vertices.empty(),
            "one-byte-short normalization reservation reached outcomes");
}

AnalyticFilteredNormalizationResult build_disjoint(std::uint32_t count,
                                                   const AnalyticSolverLimits& limits = {})
{
    AnalyticFilteredGeometry geometry;
    for (std::uint32_t index = 0; index < count; ++index)
    {
        const double x = static_cast<double>(index) * 2000.0;
        append_box(geometry, index + 1, x, 0, x + 1000, 1000);
    }
    const auto broad = build_analytic_curve_candidates(geometry.bounds, limits);
    if (broad.error != AnalyticBroadPhaseError::none)
        return {AnalyticFilteredNormalizationError::resource_limit_exceeded};
    return build_analytic_filtered_normalization(union_records(count), 0, geometry, broad.pairs,
                                                 limits);
}

void test_exact_limits_and_sparse_scaling()
{
    const auto small = build_disjoint(8);
    const auto large = build_disjoint(16);
    require(small.error == AnalyticFilteredNormalizationError::none &&
                large.error == AnalyticFilteredNormalizationError::none,
            "normalization scaling fixtures failed");
    require(large.telemetry.predicate_calls < small.telemetry.predicate_calls * 3 &&
                large.telemetry.peak_working_memory_bytes <
                    small.telemetry.peak_working_memory_bytes * 3,
            "normalization exceeded 3x work/memory at 2x sparse input");

    AnalyticSolverLimits limits;
    std::uint64_t low = 0;
    std::uint64_t high = limits.predicate_calls;
    while (low < high)
    {
        const std::uint64_t middle = low + (high - low) / 2;
        auto probe = limits;
        probe.predicate_calls = middle;
        if (build_disjoint(4, probe).error == AnalyticFilteredNormalizationError::none)
            high = middle;
        else
            low = middle + 1;
    }
    auto exact_work = limits;
    exact_work.predicate_calls = low;
    require(build_disjoint(4, exact_work).error == AnalyticFilteredNormalizationError::none,
            "exact normalization work boundary failed");
    --exact_work.predicate_calls;
    const auto short_work = build_disjoint(4, exact_work);
    require(short_work.error == AnalyticFilteredNormalizationError::resource_limit_exceeded &&
                short_work.vertices.empty() && short_work.outcomes.events.empty(),
            "one-unit-short normalization work leaked publication");

    low = 0;
    high = limits.working_memory_bytes;
    while (low < high)
    {
        const std::uint64_t middle = low + (high - low) / 2;
        auto probe = limits;
        probe.working_memory_bytes = middle;
        if (build_disjoint(4, probe).error == AnalyticFilteredNormalizationError::none)
            high = middle;
        else
            low = middle + 1;
    }
    auto exact_memory = limits;
    exact_memory.working_memory_bytes = low;
    require(build_disjoint(4, exact_memory).error == AnalyticFilteredNormalizationError::none,
            "exact normalization memory boundary failed");
    --exact_memory.working_memory_bytes;
    const auto short_memory = build_disjoint(4, exact_memory);
    require(short_memory.error == AnalyticFilteredNormalizationError::resource_limit_exceeded &&
                short_memory.vertices.empty() && short_memory.outcomes.events.empty() &&
                short_memory.telemetry.required_working_memory_bytes >
                    exact_memory.working_memory_bytes,
            "one-byte-short normalization memory leaked publication");
}

void test_large_sparse_admission()
{
    const auto result = build_disjoint(513);
    require(result.error == AnalyticFilteredNormalizationError::none &&
                result.regions.size() == 513,
            "representative sparse PCB artwork was rejected by normalization admission: " +
                std::to_string(static_cast<std::uint32_t>(result.error)) + "/" +
                std::to_string(result.telemetry.reserved_normalization_work_units));
}

void test_direct_replay_narrow_memory_boundary()
{
    AnalyticFilteredGeometry geometry;
    append_line(geometry, 1, -1000, 0, 1000, 0);
    append_line(geometry, 1, 0, -1000, 0, 1000);
    AnalyticFilteredRegionsResult empty_regions;
    AnalyticSolverLimits limits;
    std::uint64_t low = 0;
    std::uint64_t high = limits.working_memory_bytes;
    while (low < high)
    {
        const std::uint64_t middle = low + (high - low) / 2;
        auto probe = limits;
        probe.working_memory_bytes = middle;
        const auto replay = analytic_normalization_detail::validate_normalized_replay(
            0, 0, geometry.curves, geometry.bounds, empty_regions, probe);
        if (replay.error == analytic_normalization_detail::ReplayError::resource_limit_exceeded)
            low = middle + 1;
        else
            high = middle;
    }
    auto exact = limits;
    exact.working_memory_bytes = low;
    const auto admitted = analytic_normalization_detail::validate_normalized_replay(
        0, 0, geometry.curves, geometry.bounds, empty_regions, exact);
    require(admitted.error == analytic_normalization_detail::ReplayError::topology_collapse &&
                admitted.peak_working_memory_bytes == low,
            "direct replay exact narrow memory boundary failed");
    --exact.working_memory_bytes;
    const auto short_result = analytic_normalization_detail::validate_normalized_replay(
        0, 0, geometry.curves, geometry.bounds, empty_regions, exact);
    require(short_result.error ==
                analytic_normalization_detail::ReplayError::resource_limit_exceeded,
            "direct replay one-byte-short narrow memory was admitted");
}

void test_strict_replay_rejects_nearby_residual_root()
{
    const std::vector<AnalyticAtomicCurveNm> curves = {
        endpoint_authoritative_arc(1, 0, 0, -34, 68, 85, false, false),
        endpoint_authoritative_arc(2, 0, 0, -77, 49, 85, true, true),
    };
    std::vector<AnalyticCurveBoundsNm> bounds;
    for (const auto& curve : curves)
        bounds.push_back({curve.curve_index,
                          curve.circle.center.x.lower - curve.circle.radius.upper,
                          curve.circle.center.y.lower - curve.circle.radius.upper,
                          curve.circle.center.x.upper + curve.circle.radius.upper,
                          curve.circle.center.y.upper + curve.circle.radius.upper});
    const auto replay = analytic_normalization_detail::validate_normalized_replay(
        0, 0, curves, bounds, AnalyticFilteredRegionsResult{}, {});
    require(replay.error == analytic_normalization_detail::ReplayError::topology_collapse,
            "strict replay accepted a distinct residual root within 50 nm");
}

void test_replay_composite_arc_carrier_identity()
{
    const auto run = [](bool reverse_first, bool reverse_second, bool swap_order, bool same_source,
                        bool same_descriptor)
    {
        std::vector<AnalyticAtomicCurveNm> curves;
        if (same_descriptor)
        {
            curves = {
                endpoint_authoritative_arc(1, 11'290'300, 15'978'500, 11'299'882, 16'015'493,
                                           76'200, false, false, false),
                endpoint_authoritative_arc(2, 11'299'882, 16'015'493, 11'442'700, 15'978'500,
                                           76'200, false, false, false),
            };
        }
        else
        {
            curves = {
                endpoint_authoritative_arc(1, 11'442'700, 15'978'500, 11'304'131, 15'934'721,
                                           76'200, false, false, false),
                endpoint_authoritative_arc(2, 11'304'131, 15'934'721, 11'290'300, 15'978'500,
                                           76'200, false, false, false),
            };
        }
        if (reverse_first)
            reverse_arc(curves[0]);
        if (reverse_second)
            reverse_arc(curves[1]);
        if (swap_order)
            std::swap(curves[0], curves[1]);
        for (std::uint32_t index = 0; index < curves.size(); ++index)
        {
            curves[index].curve_index = index + 1;
            curves[index].construction_carrier_id = same_source ? 4'073 : 4'073 + index;
        }
        return curves;
    };

    for (const bool reverse_first : {false, true})
        for (const bool reverse_second : {false, true})
            for (const bool swap_order : {false, true})
            {
                const auto transported = run(reverse_first, reverse_second, swap_order, true, true);
                require(
                    analytic_normalization_detail::normalized_replay_arc_carrier_identity_matches(
                        transported[0], transported[1]),
                    "same-source byte-identical arc carrier was not transported");
            }

    const auto distinct_source = run(false, false, false, false, true);
    require(!analytic_normalization_detail::normalized_replay_arc_carrier_identity_matches(
                distinct_source[0], distinct_source[1]),
            "distinct source carriers were merged from identical filtered enclosures");
    const auto distinct_descriptor = run(false, false, false, true, false);
    require(!analytic_normalization_detail::normalized_replay_arc_carrier_identity_matches(
                distinct_descriptor[0], distinct_descriptor[1]),
            "same-source arcs with distinct normalized descriptors were falsely merged");
    std::vector<AnalyticCurveBoundsNm> distinct_bounds;
    for (const auto& curve : distinct_descriptor)
        distinct_bounds.push_back(circle_bounds(curve));
    const auto distinct_descriptor_replay =
        analytic_normalization_detail::validate_normalized_replay(
            0, 0, distinct_descriptor, distinct_bounds, AnalyticFilteredRegionsResult{}, {});
    require(distinct_descriptor_replay.error ==
                analytic_normalization_detail::ReplayError::topology_collapse,
            "distinct normalized siblings did not survive strict finite-domain replay");

    std::vector<AnalyticAtomicCurveNm> mixed = {
        endpoint_authoritative_arc(1, -100, 0, 100, 0, 100, false, false, false),
    };
    AnalyticAtomicCurveNm line;
    line.curve_index = 2;
    line.start = exact_point(-100.0, 0.0);
    line.end = exact_point(100.0, 0.0);
    line.has_integer_certificate = true;
    line.integer_start = {-100, 0};
    line.integer_end = {100, 0};
    mixed[0].construction_carrier_id = 77;
    line.construction_carrier_id = 77;
    mixed.push_back(line);
    const std::vector<AnalyticCurveBoundsNm> mixed_bounds = {circle_bounds(mixed[0]),
                                                             {2, -100.0, 0.0, 100.0, 0.0}};
    const auto mixed_result = analytic_normalization_detail::validate_normalized_replay(
        0, 0, mixed, mixed_bounds, AnalyticFilteredRegionsResult{}, {});
    require(mixed_result.error == analytic_normalization_detail::ReplayError::invalid_argument,
            "mixed-kind source carrier group was admitted");
}

void test_strict_replay_discards_broad_circle_residual_outside_domains()
{
    const auto make = []
    {
        std::array<AnalyticAtomicCurveNm, 2> curves = {
            endpoint_authoritative_arc(1, 11'442'700, 15'978'500, 11'304'131, 15'934'721, 76'200,
                                       false, false, false),
            endpoint_authoritative_arc(2, 11'304'131, 15'934'721, 11'290'300, 15'978'500, 76'200,
                                       false, false, false),
        };
        curves[0].construction_carrier_id = 10;
        curves[1].construction_carrier_id = 11;
        return curves;
    };
    for (const bool reverse_first : {false, true})
        for (const bool reverse_second : {false, true})
            for (const bool swap_order : {false, true})
            {
                auto curves = make();
                if (reverse_first)
                    reverse_arc(curves[0]);
                if (reverse_second)
                    reverse_arc(curves[1]);
                if (swap_order)
                    std::swap(curves[0], curves[1]);
                curves[0].curve_index = 1;
                curves[1].curve_index = 2;
                const std::vector<AnalyticAtomicCurveNm> input = {curves[0], curves[1]};
                const auto narrow = intersect_analytic_curve_candidates(input, {{1, 2}});
                require(narrow.error == AnalyticNarrowPhaseError::none &&
                            narrow.intersections.size() == 1 &&
                            narrow.intersections[0].relation == AnalyticPairRelation::point &&
                            narrow.intersections[0].point_count == 1,
                        "strict replay retained a broad residual root outside both finite arcs");
            }
}

void test_vertex_tangent_classes_are_source_order_invariant()
{
    const auto run_permutations = [](const std::array<AnalyticRequestCapsuleRecord, 3>& capsules)
    {
        std::array<std::uint32_t, 3> order = {0, 1, 2};
        do
        {
            AnalyticRequestPacketRecords records;
            records.jobs = {{1, 0, 1}};
            records.stages = {{100, 1, 0, 3}};
            for (std::uint32_t index = 0; index < order.size(); ++index)
            {
                AnalyticRequestCapsuleRecord capsule = capsules[order[index]];
                capsule.feature_id = capsules[index].feature_id;
                records.capsules.push_back(capsule);
                records.operands.push_back({2001 + index * 2, 4, index});
            }
            require(validate_analytic_request_packet_records(records) ==
                        AnalyticRequestPacketError::none,
                    "vertex tangent-class permutation is not packet-valid");
            const auto lowered = lower_analytic_job_to_filtered_curves(records, 0);
            require(lowered.error == AnalyticFilteredLoweringError::none && lowered.value,
                    "vertex tangent-class permutation did not lower");
            const auto broad = build_analytic_curve_candidates(lowered.value->bounds);
            require(broad.error == AnalyticBroadPhaseError::none,
                    "vertex tangent-class permutation broad phase failed");
            const auto normalized =
                build_analytic_filtered_normalization(records, 0, *lowered.value, broad.pairs);
            require(normalized.error == AnalyticFilteredNormalizationError::none &&
                        normalized.telemetry.algebraic_fallback_calls == 0,
                    "vertex tangent-class permutation failed normalization: " +
                        std::to_string(static_cast<unsigned>(normalized.error)) +
                        " order=" + std::to_string(order[0]) + std::to_string(order[1]) +
                        std::to_string(order[2]));
        } while (std::next_permutation(order.begin(), order.end()));
    };
    run_permutations({
        AnalyticRequestCapsuleRecord{1001, 0, 415801, 1450327, 415801, 254000},
        AnalyticRequestCapsuleRecord{1003, 1605082, 9718, 1614800, 0, 254000},
        AnalyticRequestCapsuleRecord{1009, 1614800, 0, 1624518, 9718, 254000},
    });
    run_permutations({
        AnalyticRequestCapsuleRecord{1001, 0, 406083, 1450327, 406083, 254000},
        AnalyticRequestCapsuleRecord{1003, 1450327, 406083, 1624518, 231892, 254000},
        AnalyticRequestCapsuleRecord{1009, 1624518, 0, 1624518, 231892, 254000},
    });
    run_permutations({
        AnalyticRequestCapsuleRecord{1001, 0, 9718, 9718, 0, 254000},
        AnalyticRequestCapsuleRecord{1003, 9718, 0, 19436, 9718, 254000},
        AnalyticRequestCapsuleRecord{1009, 19436, 9718, 19436, 241610, 254000},
    });
    run_permutations({
        AnalyticRequestCapsuleRecord{1001, 0, 0, 300000, 0, 254000},
        AnalyticRequestCapsuleRecord{1003, 1000000, 0, 1490281, 490281, 254000},
        AnalyticRequestCapsuleRecord{1009, 1490281, 490281, 1665279, 490281, 254000},
    });
}

void test_strict_replay_discards_residual_root_strictly_outside_finite_domain()
{
    AnalyticAtomicCurveNm arc =
        endpoint_authoritative_arc(1, -80085, -89803, 99521, -89803, 127000, true, false);
    arc.construction_carrier_id = 10;

    AnalyticAtomicCurveNm line;
    line.curve_index = 2;
    line.start = exact_point(99521.0, -89803.0);
    line.end = exact_point(109239.0, -80085.0);
    line.has_integer_certificate = true;
    line.integer_start = {99521, -89803};
    line.integer_end = {109239, -80085};
    line.construction_carrier_id = 4;
    line.has_construction_line_direction = true;
    line.construction_line_dx = 1;
    line.construction_line_dy = 1;

    const std::vector<AnalyticAtomicCurveNm> curves = {arc, line};
    const auto outside = intersect_analytic_curve_candidates(curves, {{1, 2}});
    require(outside.error == AnalyticNarrowPhaseError::none && outside.intersections.size() == 1 &&
                outside.intersections[0].relation == AnalyticPairRelation::point &&
                outside.intersections[0].point_count == 1,
            "endpoint-authoritative replay retained a residual carrier root wholly before the "
            "finite line");

    line.start = exact_point(99519.0, -89805.0);
    line.end = exact_point(99521.0, -89803.0);
    line.integer_start = {99519, -89805};
    line.integer_end = {99521, -89803};
    const std::vector<AnalyticAtomicCurveNm> entering_curves = {arc, line};
    const auto entering_narrow = intersect_analytic_curve_candidates(entering_curves, {{1, 2}});
    require(entering_narrow.error == AnalyticNarrowPhaseError::none &&
                entering_narrow.intersections.size() == 1 &&
                entering_narrow.intersections[0].relation == AnalyticPairRelation::two_points &&
                entering_narrow.intersections[0].point_count == 2,
            "endpoint-authoritative replay discarded a residual root inside the finite line");
    const std::vector<AnalyticCurveBoundsNm> entering_bounds = {
        {1, arc.circle.center.x.lower - arc.circle.radius.upper,
         arc.circle.center.y.lower - arc.circle.radius.upper,
         arc.circle.center.x.upper + arc.circle.radius.upper,
         arc.circle.center.y.upper + arc.circle.radius.upper},
        {2, 99519.0, -89805.0, 99521.0, -89803.0}};
    const auto entering = analytic_normalization_detail::validate_normalized_replay(
        0, 0, entering_curves, entering_bounds, AnalyticFilteredRegionsResult{}, {});
    require(entering.error == analytic_normalization_detail::ReplayError::topology_collapse,
            "strict replay discarded a residual root that enters the finite line domain");
}

void test_strict_replay_retains_near_cardinal_partition()
{
    std::vector<AnalyticAtomicCurveNm> curves = {
        endpoint_authoritative_arc(1, 399, -40, 399, 40, 401, true, false, false),
    };
    AnalyticAtomicCurveNm line;
    line.curve_index = 2;
    line.start = exact_point(399.0, 40.0);
    line.end = exact_point(399.0, -40.0);
    line.has_integer_certificate = true;
    line.integer_start = {399, 40};
    line.integer_end = {399, -40};
    curves.push_back(line);

    const std::vector<AnalyticCurveBoundsNm> bounds = {
        {1, -401.0, -401.0, 401.0, 401.0},
        {2, 399.0, -40.0, 399.0, 40.0},
    };
    AnalyticFilteredRegionsResult expected;
    expected.rings.push_back({0, 2, kNoAnalyticFilteredRing, 0, true});
    expected.ring_half_edges = {0, 1};
    expected.regions.push_back({0, 0});
    const auto replay = analytic_normalization_detail::validate_normalized_replay(
        0, 0, curves, bounds, expected, {});
    require(replay.error == analytic_normalization_detail::ReplayError::none,
            "strict replay failed to retain a distinct near-cardinal partition");
}

void append_parity_result(std::ostringstream& output,
                          const AnalyticFilteredNormalizationResult& result)
{
    const auto append = [&output](std::uint64_t value) { output << std::setw(16) << value; };
    append(static_cast<std::uint8_t>(result.error));
    append(result.vertices.size());
    for (const auto& value : result.vertices)
    {
        append(static_cast<std::uint64_t>(value.x_nm));
        append(static_cast<std::uint64_t>(value.y_nm));
        append(value.arrangement_vertex);
    }
    append(result.fragments.size());
    for (const auto& value : result.fragments)
    {
        append(value.start_vertex);
        append(value.end_vertex);
        append(static_cast<std::uint8_t>(value.kind));
        append(value.radius_nm);
        append(value.counterclockwise);
        append(value.major_arc);
        append(value.old_boundary);
    }
    append(result.ring_fragments.size());
    for (const auto value : result.ring_fragments)
        append(value);
    append(result.rings.size());
    for (const auto& value : result.rings)
    {
        append(value.fragment_begin);
        append(value.fragment_count);
        append(value.parent_ring);
        append(value.depth);
        append(value.counterclockwise);
        append(value.old_ring);
    }
    append(result.regions.size());
    for (const auto& value : result.regions)
    {
        append(value.outer_ring);
        append(value.old_region);
    }
    const auto append_map = [&append](const std::vector<std::uint32_t>& map)
    {
        append(map.size());
        for (const std::uint32_t value : map)
            append(value);
    };
    append_map(result.old_vertex_to_normalized);
    append_map(result.old_boundary_to_normalized);
    append_map(result.old_ring_to_normalized);
    append_map(result.old_region_to_normalized);
    const auto& telemetry = result.telemetry;
    append(telemetry.outcomes_work_units);
    append(telemetry.outcomes_peak_working_memory_bytes);
    append(telemetry.normalized_vertices);
    append(telemetry.normalized_fragments);
    append(telemetry.normalized_rings);
    append(telemetry.normalized_regions);
    append(telemetry.arc_critical_candidates);
    append(telemetry.strict_replay_candidate_pairs);
    append(telemetry.reserved_normalization_work_units);
    append(telemetry.reserved_normalization_memory_bytes);
    append(telemetry.normalization_work_units);
    append(telemetry.predicate_calls);
    append(telemetry.peak_working_memory_bytes);
    append(telemetry.algebraic_fallback_calls);
}

std::string parity_vector()
{
    AnalyticFilteredGeometry disks;
    append_disk(disks, 1, 0, 0, 1000);
    append_disk(disks, 2, 1201, 0, 1000);
    const auto disk_broad = build_analytic_curve_candidates(disks.bounds);
    require(disk_broad.error == AnalyticBroadPhaseError::none, "parity disk broad phase failed");
    const auto disk_result =
        build_analytic_filtered_normalization(records_for_two(1, 2), 0, disks, disk_broad.pairs);
    require(disk_result.error == AnalyticFilteredNormalizationError::none,
            "parity disk normalization failed");

    AnalyticFilteredGeometry nested;
    append_box(nested, 1, 0, 0, 4000, 4000);
    append_box(nested, 2, 500, 500, 3500, 3500);
    append_box(nested, 3, 1000, 1000, 3000, 3000);
    const auto nested_broad = build_analytic_curve_candidates(nested.bounds);
    require(nested_broad.error == AnalyticBroadPhaseError::none,
            "parity nested broad phase failed");
    const auto nested_result =
        build_analytic_filtered_normalization(nested_records(), 0, nested, nested_broad.pairs);
    require(nested_result.error == AnalyticFilteredNormalizationError::none,
            "parity nested normalization failed");

    std::ostringstream output;
    output << std::hex << std::setfill('0');
    append_parity_result(output, disk_result);
    append_parity_result(output, nested_result);
    return output.str();
}
} // namespace

int main(int argc, char** argv)
{
    test_near_coincident_arc_certificate_guards();
    test_integer_box();
    test_integer_disk();
    test_rotated_semicircles_and_major_arc();
    test_irrational_disk_crossings();
    test_successful_empty_difference_skips_geometry_replay();
    test_collapsed_only_empty_output_fails_closed();
    test_irrational_radius_arc_and_clockwise_hole();
    test_maps_and_unused_vertex_sentinel();
    test_original_hard_limits_are_enforced();
    test_vertex_collision_fails_closed();
    test_global_half_ties();
    test_nested_hole_and_island_replay();
    test_early_normalization_reservation();
    test_exact_limits_and_sparse_scaling();
    test_large_sparse_admission();
    test_direct_replay_narrow_memory_boundary();
    test_strict_replay_rejects_nearby_residual_root();
    test_replay_composite_arc_carrier_identity();
    test_strict_replay_discards_broad_circle_residual_outside_domains();
    test_vertex_tangent_classes_are_source_order_invariant();
    test_strict_replay_discards_residual_root_strictly_outside_finite_domain();
    test_strict_replay_retains_near_cardinal_partition();
    if (argc == 2 && std::string(argv[1]) == "--emit-parity")
        std::cout << "ANALYTIC_FILTERED_NORMALIZATION_VECTOR=" << parity_vector() << '\n';
    std::cout << "analytic filtered normalization tests passed\n";
    return 0;
}

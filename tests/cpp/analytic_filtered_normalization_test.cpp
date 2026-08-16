#include "geometer/analytic_curve_broad_phase.h"
#include "geometer/analytic_filtered_normalization.h"

#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
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

AnalyticFilteredPointNm exact_point(double x, double y)
{
    return {{x, x}, {y, y}};
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
    occurrence.agrees_with_carrier = true;
    occurrence.material_on_left = true;
    occurrence.source.kind = AnalyticFilteredSourceKind::compact_feature_role;
    occurrence.source.role = AnalyticFilteredSourceRole::primitive_outer_circle;
    occurrence.source.operand_id = operand;
    occurrence.source.primary_id = 1;
    occurrence.source.secondary_id = 0;
    geometry.occurrences.push_back(occurrence);
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
                std::to_string(rotated_result.fragments.size()));

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
    append_disk(geometry, 2, 1200, 0, 1000);
    const auto broad = build_analytic_curve_candidates(geometry.bounds);
    require(broad.error == AnalyticBroadPhaseError::none, "overlapping-disk broad phase failed");
    const auto result =
        build_analytic_filtered_normalization(records_for_two(1, 2), 0, geometry, broad.pairs);
    require(result.error == AnalyticFilteredNormalizationError::none,
            "irrational disk-crossing normalization failed: " +
                std::to_string(static_cast<std::uint32_t>(result.error)) + "/" +
                std::to_string(result.telemetry.arc_critical_candidates) + "/" +
                std::to_string(result.telemetry.strict_replay_candidate_pairs));
    require(result.regions.size() == 1 && result.rings.size() == 1 && result.vertices.size() == 4 &&
                result.fragments.size() == 4 && result.telemetry.algebraic_fallback_calls == 0,
            "irrational disk union topology drifted or used algebraic fallback");
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
                short_memory.vertices.empty() && short_memory.outcomes.events.empty(),
            "one-byte-short normalization memory leaked publication");
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
    append_disk(disks, 2, 1200, 0, 1000);
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
    test_integer_box();
    test_integer_disk();
    test_rotated_semicircles_and_major_arc();
    test_irrational_disk_crossings();
    test_vertex_collision_fails_closed();
    test_global_half_ties();
    test_nested_hole_and_island_replay();
    test_early_normalization_reservation();
    test_exact_limits_and_sparse_scaling();
    if (argc == 2 && std::string(argv[1]) == "--emit-parity")
        std::cout << "ANALYTIC_FILTERED_NORMALIZATION_VECTOR=" << parity_vector() << '\n';
    std::cout << "analytic filtered normalization tests passed\n";
    return 0;
}

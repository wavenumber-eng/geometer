#include "geometer/analytic_curve_broad_phase.h"
#include "geometer/analytic_filtered_boolean_selection.h"
#include "geometer/analytic_filtered_lowering.h"

#include "analytic_filtered_boolean_selection_support.h"
#include "analytic_filtered_execution_policy.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
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

AnalyticFilteredPointNm exact_point(double x, double y)
{
    return {{x, x}, {y, y}};
}

bool same_point(const AnalyticFilteredPointNm& left, const AnalyticFilteredPointNm& right)
{
    return left.x.lower == right.x.lower && left.x.upper == right.x.upper &&
           left.y.lower == right.y.lower && left.y.upper == right.y.upper &&
           left.construction_x_column_id == right.construction_x_column_id;
}

struct StageSpec
{
    std::uint8_t operation = 1;
    std::vector<std::uint64_t> operands;
};

AnalyticRequestPacketRecords records_for(const std::vector<StageSpec>& specs)
{
    AnalyticRequestPacketRecords records;
    records.jobs.push_back({1, 0, static_cast<std::uint32_t>(specs.size())});
    for (std::uint32_t stage = 0; stage < specs.size(); ++stage)
    {
        const StageSpec& spec = specs[stage];
        const std::uint32_t begin = static_cast<std::uint32_t>(records.operands.size());
        for (const std::uint64_t operand : spec.operands)
            records.operands.push_back({operand, 2, 0});
        records.stages.push_back(
            {stage + 1, spec.operation, begin, static_cast<std::uint32_t>(spec.operands.size())});
    }
    return records;
}

void append_line(AnalyticFilteredGeometry& geometry, std::uint64_t operand, double x1, double y1,
                 double x2, double y2, bool material_on_left)
{
    const std::uint32_t index = static_cast<std::uint32_t>(geometry.curves.size() + 1);
    const double dx = x2 - x1;
    const double dy = y2 - y1;
    const bool agrees = dx > 0.0 || (dx == 0.0 && dy > 0.0);
    AnalyticAtomicCurveNm curve;
    curve.curve_index = index;
    curve.start = exact_point(x1, y1);
    curve.end = exact_point(x2, y2);
    curve.construction_carrier_id = 1000 + index;
    curve.construction_family_id = 2000 + index;
    curve.has_construction_line_direction = true;
    curve.construction_line_dx = static_cast<std::int64_t>(agrees ? dx : -dx);
    curve.construction_line_dy = static_cast<std::int64_t>(agrees ? dy : -dy);
    if (curve.construction_line_dx == 0)
    {
        const std::uint64_t column =
            analytic_vertical_x_column_token(curve.construction_carrier_id);
        curve.start.construction_x_column_id = column;
        curve.end.construction_x_column_id = column;
    }
    geometry.curves.push_back(curve);
    geometry.bounds.push_back(
        {index, std::min(x1, x2), std::min(y1, y2), std::max(x1, x2), std::max(y1, y2)});
    AnalyticFilteredOccurrence occurrence;
    occurrence.occurrence_id = index;
    occurrence.coverage_id = operand;
    occurrence.agrees_with_carrier = agrees;
    occurrence.material_on_left = material_on_left;
    occurrence.source.operand_id = operand;
    occurrence.source.primary_id = 10000 + index;
    geometry.occurrences.push_back(occurrence);
}

void append_axis_rectangle(AnalyticFilteredGeometry& geometry, std::uint64_t operand,
                           double minimum_x, double minimum_y, double maximum_x, double maximum_y,
                           bool material_inside = true)
{
    append_line(geometry, operand, minimum_x, minimum_y, maximum_x, minimum_y, material_inside);
    append_line(geometry, operand, maximum_x, minimum_y, maximum_x, maximum_y, material_inside);
    append_line(geometry, operand, maximum_x, maximum_y, minimum_x, maximum_y, material_inside);
    append_line(geometry, operand, minimum_x, maximum_y, minimum_x, minimum_y, material_inside);
}

void append_rectangle(AnalyticFilteredGeometry& geometry, std::uint64_t operand, double minimum,
                      double maximum, bool material_inside = true)
{
    append_axis_rectangle(geometry, operand, minimum, minimum, maximum, maximum, material_inside);
}

void append_polygon(AnalyticFilteredGeometry& geometry, std::uint64_t operand,
                    const std::vector<std::array<double, 2>>& vertices)
{
    require(vertices.size() >= 3, "polygon fixture requires at least three vertices");
    for (std::size_t index = 0; index < vertices.size(); ++index)
    {
        const auto& start = vertices[index];
        const auto& end = vertices[(index + 1) % vertices.size()];
        append_line(geometry, operand, start[0], start[1], end[0], end[1], true);
    }
}

AnalyticFilteredBooleanSelectionResult select(const AnalyticRequestPacketRecords& records,
                                              const AnalyticFilteredGeometry& geometry,
                                              const AnalyticSolverLimits& limits = {})
{
    const AnalyticBroadPhaseResult broad = build_analytic_curve_candidates(geometry.bounds, limits);
    require(broad.error == AnalyticBroadPhaseError::none, "broad phase failed");
    return build_analytic_filtered_boolean_selection(records, 0, geometry, broad.pairs, limits);
}

AnalyticFilteredBooleanSelectionResult strict_select(const AnalyticRequestPacketRecords& records,
                                                     const AnalyticFilteredGeometry& geometry,
                                                     const AnalyticSolverLimits& limits = {})
{
    const AnalyticBroadPhaseResult broad = analytic_execution_detail::build_curve_candidates(
        geometry.bounds, limits, analytic_execution_detail::kStrictPublishedGeometry);
    require(broad.error == AnalyticBroadPhaseError::none, "strict selection broad phase failed");
    return analytic_execution_detail::build_boolean_selection(
        records, 0, geometry, broad.pairs, limits,
        analytic_execution_detail::kStrictPublishedGeometry);
}

bool contains_operand(const AnalyticFilteredBooleanSelectionResult& result,
                      const AnalyticFilteredSelectedFace& face, std::uint32_t operand)
{
    std::uint32_t capacity = 1;
    while (capacity < std::max<std::uint64_t>(1, result.telemetry.input_operands))
        capacity <<= 1U;
    std::uint32_t root = face.coverage_state_root;
    std::uint32_t begin = 0;
    while (capacity > 1)
    {
        require(root < result.coverage_state_nodes.size() && root != 1,
                "malformed coverage state root");
        const std::uint32_t half = capacity / 2;
        if (operand < begin + half)
            root = root == 0 ? 0 : result.coverage_state_nodes[root].left;
        else
        {
            begin += half;
            root = root == 0 ? 0 : result.coverage_state_nodes[root].right;
        }
        capacity = half;
    }
    return root == 1;
}

void test_single_square()
{
    AnalyticFilteredGeometry geometry;
    append_rectangle(geometry, 1, 0, 1000);
    const AnalyticFilteredBooleanSelectionResult result = select(records_for({{1, {1}}}), geometry);
    require(result.error == AnalyticFilteredBooleanSelectionError::none,
            "single-square selection failed");
    require(result.faces.size() == 2 && result.face_boundary_cycles.size() == 2,
            "single-square face topology drifted");
    require(result.faces[0].unbounded && !result.faces[0].material && !result.faces[1].unbounded &&
                result.faces[1].material,
            "single-square material classification drifted");
    require(contains_operand(result, result.faces[1], 0),
            "single-square persistent coverage state drifted");
}

void test_nested_components()
{
    AnalyticFilteredGeometry geometry;
    append_rectangle(geometry, 1, 0, 1000);
    append_rectangle(geometry, 2, 250, 750);
    const AnalyticFilteredBooleanSelectionResult result =
        select(records_for({{1, {1, 2}}}), geometry);
    require(result.error == AnalyticFilteredBooleanSelectionError::none,
            "nested-component selection failed");
    require(result.faces.size() == 3, "nested components did not create three faces");
    const auto owner = std::find_if(result.faces.begin(), result.faces.end(),
                                    [](const AnalyticFilteredSelectedFace& face)
                                    { return face.boundary_cycle_count == 2; });
    require(owner != result.faces.end() && owner->material,
            "nested exterior cycle was not owned by its containing face");
    require(result.telemetry.coverage_state_nodes <= 8,
            "nested persistent coverage state grew unexpectedly");
}

void test_annular_material_side()
{
    AnalyticFilteredGeometry geometry;
    append_rectangle(geometry, 1, 0, 1000, true);
    append_rectangle(geometry, 1, 250, 750, false);
    const AnalyticFilteredBooleanSelectionResult result = select(records_for({{1, {1}}}), geometry);
    require(result.error == AnalyticFilteredBooleanSelectionError::none,
            "annular material-side selection failed");
    require(result.faces.size() == 3 && result.telemetry.material_faces == 1,
            "annulus must select only its middle face");
    const auto material =
        std::find_if(result.faces.begin(), result.faces.end(),
                     [](const AnalyticFilteredSelectedFace& face) { return face.material; });
    require(material != result.faces.end() && material->boundary_cycle_count == 2 &&
                contains_operand(result, *material, 0),
            "annular face ownership or coverage drifted");
}

void test_add_subtract_add()
{
    AnalyticFilteredGeometry geometry;
    append_rectangle(geometry, 1, 0, 1200);
    append_rectangle(geometry, 2, 200, 1000);
    append_rectangle(geometry, 3, 400, 800);
    const AnalyticFilteredBooleanSelectionResult result =
        select(records_for({{1, {1}}, {2, {2}}, {1, {3}}}), geometry);
    require(result.error == AnalyticFilteredBooleanSelectionError::none,
            "add-subtract-add selection failed");
    require(result.faces.size() == 4 && result.telemetry.material_faces == 2,
            "add-subtract-add material faces drifted");
    bool found_restored = false;
    for (const AnalyticFilteredSelectedFace& face : result.faces)
    {
        if (contains_operand(result, face, 2))
        {
            require(face.material && contains_operand(result, face, 0) &&
                        contains_operand(result, face, 1),
                    "restored face lost its ordered-stage coverage state");
            found_restored = true;
        }
    }
    require(found_restored, "add-subtract-add restored face was not found");
}

void test_overlapping_rectangles()
{
    AnalyticFilteredGeometry geometry;
    append_rectangle(geometry, 1, 0, 1000);
    append_line(geometry, 2, 500, 250, 1500, 250, true);
    append_line(geometry, 2, 1500, 250, 1500, 750, true);
    append_line(geometry, 2, 1500, 750, 500, 750, true);
    append_line(geometry, 2, 500, 750, 500, 250, true);
    const AnalyticFilteredBooleanSelectionResult result =
        select(records_for({{1, {1, 2}}}), geometry);
    require(result.error == AnalyticFilteredBooleanSelectionError::none,
            "overlapping-rectangle selection failed");
    require(result.faces.size() == 4 && result.telemetry.material_faces == 3 &&
                result.telemetry.coverage_state_nodes <= 12,
            "overlapping-rectangle face coverage drifted");
}

void test_lowered_disk()
{
    AnalyticRequestPacketRecords records;
    records.jobs.push_back({1, 0, 1});
    records.stages.push_back({1, 1, 0, 1});
    records.operands.push_back({1, 2, 0});
    records.disks.push_back({10, 0, 0, 100});
    const AnalyticFilteredLoweringResult lowered =
        lower_analytic_job_to_filtered_curves(records, 0);
    require(lowered.error == AnalyticFilteredLoweringError::none && lowered.value,
            "disk lowering failed");
    const AnalyticFilteredBooleanSelectionResult result = select(records, *lowered.value);
    require(result.error == AnalyticFilteredBooleanSelectionError::none &&
                result.faces.size() == 2 && result.telemetry.material_faces == 1,
            "lowered disk face selection failed");
    require(std::all_of(result.arrangement.edges.begin(), result.arrangement.edges.end(),
                        [](const AnalyticArrangementEdgeNm& edge)
                        { return edge.x_monotone_branch != AnalyticXMonotoneBranch::none; }),
            "disk arrangement lost x-monotone branch certificates");
}

void test_large_origin_is_retained()
{
    AnalyticRequestPacketRecords records;
    records.jobs = {{1, 0, 1}};
    records.stages = {{1, 1, 0, 1}};
    records.operands = {{1, 2, 0}};
    records.disks = {{10, 700000000000LL, -600000000000LL, 1000}};
    const AnalyticFilteredLoweringResult lowered =
        lower_analytic_job_to_filtered_curves(records, 0);
    require(lowered.error == AnalyticFilteredLoweringError::none && lowered.value,
            "large-origin disk lowering failed");
    const AnalyticFilteredBooleanSelectionResult result = select(records, *lowered.value);
    require(result.error == AnalyticFilteredBooleanSelectionError::none &&
                result.origin_x_nm == lowered.value->origin_x_nm &&
                result.origin_y_nm == lowered.value->origin_y_nm && result.origin_x_nm != 0 &&
                result.origin_y_nm < 0,
            "face selection dropped its job-local coordinate origin");
}

AnalyticRequestPacketRecords seam_adjacent_disk_rectangle_records()
{
    AnalyticRequestPacketRecords records;
    records.jobs = {{1, 0, 1}};
    records.stages = {{1, 1, 0, 2}};
    records.operands = {{1, 2, 0}, {2, 1, 0}};
    records.disks = {{10, 0, 0, 1000}};
    records.planar_regions = {{20, 0, 0, 0}};
    records.rings = {{30, 0, 4, 0, 4, 0}};
    records.vertices = {{40, 999, -500}, {41, 1500, -500}, {42, 1500, 500}, {43, 999, 500}};
    for (std::uint32_t edge = 0; edge < 4; ++edge)
        records.segments.push_back({50 + edge, 60 + edge, 1, 0, false, 0, 0});
    return records;
}

AnalyticFilteredBooleanSelectionResult test_seam_adjacent_intersections_remain_distinct()
{
    const AnalyticRequestPacketRecords records = seam_adjacent_disk_rectangle_records();
    const AnalyticFilteredLoweringResult lowered =
        lower_analytic_job_to_filtered_curves(records, 0);
    require(lowered.error == AnalyticFilteredLoweringError::none && lowered.value,
            "seam-adjacent disk/rectangle lowering failed");
    const AnalyticBroadPhaseResult broad = build_analytic_curve_candidates(lowered.value->bounds);
    require(broad.error == AnalyticBroadPhaseError::none,
            "seam-adjacent disk/rectangle broad phase failed");
    const AnalyticFilteredArrangementResult arrangement =
        build_analytic_filtered_arrangement(*lowered.value, broad.pairs);
    const AnalyticFilteredBooleanSelectionResult result =
        build_analytic_filtered_boolean_selection(records, 0, *lowered.value, broad.pairs);
    require(arrangement.error == AnalyticFilteredArrangementError::none &&
                result.error == AnalyticFilteredBooleanSelectionError::none,
            "seam-adjacent disk/rectangle solve failed: arrangement=" +
                std::to_string(static_cast<int>(arrangement.error)) +
                " selection=" + std::to_string(static_cast<int>(result.error)) +
                " overlay=" + std::to_string(static_cast<int>(result.arrangement.error)) +
                " vertices=" + std::to_string(arrangement.vertices.size()) +
                " edges=" + std::to_string(arrangement.edges.size()) +
                " cycles=" + std::to_string(arrangement.cycles.size()) +
                " columns=" + std::to_string(result.telemetry.event_columns) +
                " unions=" + std::to_string(result.telemetry.face_gap_unions) +
                " transitions=" + std::to_string(result.telemetry.transition_records));
    require(result.arrangement.vertices.size() == arrangement.vertices.size(),
            "face selection changed the arrangement vertex count");
    for (std::size_t index = 0; index < arrangement.vertices.size(); ++index)
        require(
            same_point(result.arrangement.vertices[index].point, arrangement.vertices[index].point),
            "face selection changed an arrangement coordinate");

    std::vector<double> crossing_y;
    for (const AnalyticArrangementVertexNm& vertex : result.arrangement.vertices)
    {
        const double global_x = result.origin_x_nm + vertex.point.x.lower;
        const double global_y = result.origin_y_nm + vertex.point.y.lower;
        if (global_x > 998.0 && global_x < 1000.0 && std::fabs(global_y) < 500.0)
            crossing_y.push_back(global_y);
    }
    std::sort(crossing_y.begin(), crossing_y.end());
    require(crossing_y.size() == 2 && crossing_y[1] - crossing_y[0] > 50.0,
            "opposite seam-adjacent roots were collapsed through the cardinal point");
    return result;
}

AnalyticFilteredBooleanSelectionResult
select_disks(const std::vector<AnalyticRequestDiskRecord>& disks, std::uint8_t operation = 1)
{
    AnalyticRequestPacketRecords records;
    records.jobs.push_back({1, 0, 1});
    records.stages.push_back({1, operation, 0, static_cast<std::uint32_t>(disks.size())});
    records.disks = disks;
    for (std::uint32_t index = 0; index < disks.size(); ++index)
        records.operands.push_back({static_cast<std::uint64_t>(index) + 1, 2, index});
    const AnalyticFilteredLoweringResult lowered =
        lower_analytic_job_to_filtered_curves(records, 0);
    require(lowered.error == AnalyticFilteredLoweringError::none && lowered.value,
            "disk-set lowering failed");
    return select(records, *lowered.value);
}

std::vector<AnalyticRequestDiskRecord> disjoint_disks(std::uint32_t count)
{
    std::vector<AnalyticRequestDiskRecord> disks;
    disks.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index)
        disks.push_back({static_cast<std::uint64_t>(index) + 10,
                         static_cast<std::int64_t>(index) * 5000, 0, 1000});
    return disks;
}

void test_disjoint_disk_admission_remains_sparse()
{
    const AnalyticFilteredBooleanSelectionResult small = select_disks(disjoint_disks(200));
    const AnalyticFilteredBooleanSelectionResult large = select_disks(disjoint_disks(400));
    require(small.error == AnalyticFilteredBooleanSelectionError::none &&
                large.error == AnalyticFilteredBooleanSelectionError::none &&
                large.faces.size() == 401 && large.telemetry.material_faces == 400,
            "ordinary disjoint disks were rejected by repeated-carrier admission");
    require(large.telemetry.admission_work_units <= small.telemetry.admission_work_units * 3 &&
                large.telemetry.predicate_calls <= small.telemetry.predicate_calls * 3 &&
                large.telemetry.peak_working_memory_bytes <=
                    small.telemetry.peak_working_memory_bytes * 3,
            "disjoint repeated-carrier disks regressed toward quadratic scaling");
}

void test_irrational_overlapping_disks()
{
    const AnalyticFilteredBooleanSelectionResult result =
        select_disks({{10, 0, 0, 1000}, {11, 1000, 0, 1000}});
    require(result.error == AnalyticFilteredBooleanSelectionError::none,
            "irrational overlapping disks failed face selection: error=" +
                std::to_string(static_cast<int>(result.error)) +
                " faces=" + std::to_string(result.faces.size()) +
                " columns=" + std::to_string(result.telemetry.event_columns) +
                " correlated=" + std::to_string(result.telemetry.resolution_event_columns));
    require(result.faces.size() == 4 && result.telemetry.material_faces == 3 &&
                result.telemetry.algebraic_fallback_calls == 0,
            "irrational overlapping disks lost their three bounded faces");
}

void test_tangent_disks_do_not_join_faces()
{
    const AnalyticFilteredBooleanSelectionResult result =
        select_disks({{10, 0, 0, 1000}, {11, 2000, 0, 1000}});
    require(result.error == AnalyticFilteredBooleanSelectionError::none,
            "externally tangent disks failed face selection");
    require(result.faces.size() == 3 && result.telemetry.material_faces == 2,
            "point tangency incorrectly joined an open face gap");
}

void test_coincident_and_difference_only_disks()
{
    const AnalyticFilteredBooleanSelectionResult coincident =
        select_disks({{10, 0, 0, 1000}, {11, 0, 0, 1000}});
    require(coincident.error == AnalyticFilteredBooleanSelectionError::none &&
                coincident.faces.size() == 2 && coincident.telemetry.material_faces == 1 &&
                contains_operand(coincident, coincident.faces[1], 0) &&
                contains_operand(coincident, coincident.faces[1], 1),
            "coincident union lost same-stage coverage multiplicity");

    const AnalyticFilteredBooleanSelectionResult difference = select_disks({{10, 0, 0, 1000}}, 2);
    require(difference.error == AnalyticFilteredBooleanSelectionError::none &&
                difference.faces.size() == 2 && difference.telemetry.material_faces == 0,
            "difference from empty incorrectly created material");
}

void test_annulus_and_irrational_capsule()
{
    AnalyticRequestPacketRecords annulus_records;
    annulus_records.jobs = {{1, 0, 1}};
    annulus_records.stages = {{1, 1, 0, 1}};
    annulus_records.operands = {{1, 3, 0}};
    annulus_records.annuli = {{10, 0, 0, 400, 1000}};
    const AnalyticFilteredLoweringResult annulus_lowered =
        lower_analytic_job_to_filtered_curves(annulus_records, 0);
    require(annulus_lowered.error == AnalyticFilteredLoweringError::none && annulus_lowered.value,
            "annulus lowering failed");
    const AnalyticFilteredBooleanSelectionResult annulus =
        select(annulus_records, *annulus_lowered.value);
    require(annulus.error == AnalyticFilteredBooleanSelectionError::none &&
                annulus.faces.size() == 3 && annulus.telemetry.material_faces == 1,
            "annulus face selection lost its empty center");

    AnalyticRequestPacketRecords capsule_records;
    capsule_records.jobs = {{1, 0, 1}};
    capsule_records.stages = {{1, 1, 0, 1}};
    capsule_records.operands = {{1, 4, 0}};
    capsule_records.capsules = {{10, -7000, 11000, 16000, 28000, 21001}};
    const AnalyticFilteredLoweringResult capsule_lowered =
        lower_analytic_job_to_filtered_curves(capsule_records, 0);
    require(capsule_lowered.error == AnalyticFilteredLoweringError::none && capsule_lowered.value,
            "irrational-direction capsule lowering failed");
    const AnalyticFilteredBooleanSelectionResult capsule =
        select(capsule_records, *capsule_lowered.value);
    require(capsule.error == AnalyticFilteredBooleanSelectionError::none &&
                capsule.faces.size() == 2 && capsule.telemetry.material_faces == 1,
            "irrational-direction capsule face selection failed: error=" +
                std::to_string(static_cast<int>(capsule.error)) +
                " columns=" + std::to_string(capsule.telemetry.event_columns) +
                " unions=" + std::to_string(capsule.telemetry.face_gap_unions));

    capsule_records.capsules = {{11, 0, 0, 0, 10000, 1001}};
    const AnalyticFilteredLoweringResult vertical_lowered =
        lower_analytic_job_to_filtered_curves(capsule_records, 0);
    require(vertical_lowered.error == AnalyticFilteredLoweringError::none && vertical_lowered.value,
            "vertical odd-width capsule lowering failed");
    const AnalyticBroadPhaseResult vertical_broad =
        build_analytic_curve_candidates(vertical_lowered.value->bounds);
    require(vertical_broad.error == AnalyticBroadPhaseError::none,
            "vertical odd-width capsule broad phase failed");
    const AnalyticFilteredOverlayResult vertical_overlay =
        build_analytic_filtered_overlay(*vertical_lowered.value, vertical_broad.pairs);
    require(vertical_overlay.error == AnalyticFilteredOverlayError::none,
            "vertical odd-width capsule overlay failed: error=" +
                std::to_string(static_cast<int>(vertical_overlay.error)) +
                " unique=" + std::to_string(vertical_overlay.telemetry.unique_events) +
                " spans=" + std::to_string(vertical_overlay.telemetry.emitted_spans));
    const AnalyticFilteredArrangementResult vertical_arrangement =
        build_analytic_filtered_arrangement(*vertical_lowered.value, vertical_broad.pairs);
    require(vertical_arrangement.error == AnalyticFilteredArrangementError::none,
            "vertical odd-width capsule arrangement failed: error=" +
                std::to_string(static_cast<int>(vertical_arrangement.error)) + " endpoints=" +
                std::to_string(vertical_arrangement.telemetry.endpoint_records) + " merged=" +
                std::to_string(vertical_arrangement.telemetry.merged_endpoint_records) +
                " vertices=" + std::to_string(vertical_arrangement.telemetry.emitted_vertices) +
                " edges=" + std::to_string(vertical_arrangement.telemetry.emitted_edges));
    const AnalyticFilteredBooleanSelectionResult vertical =
        select(capsule_records, *vertical_lowered.value);
    require(vertical.error == AnalyticFilteredBooleanSelectionError::none &&
                vertical.faces.size() == 2 && vertical.telemetry.material_faces == 1,
            "vertical odd-width capsule lost its correlated x columns: error=" +
                std::to_string(static_cast<int>(vertical.error)) +
                " columns=" + std::to_string(vertical.telemetry.event_columns) +
                " correlated=" + std::to_string(vertical.telemetry.resolution_event_columns) +
                " stages=" + std::to_string(vertical.telemetry.input_stages));
}

void test_symmetric_capsule_disk_intersections_share_bounded_columns()
{
    AnalyticRequestPacketRecords records;
    records.jobs = {{7, 0, 1}};
    records.stages = {{701, 1, 0, 3}};
    records.operands = {{7001, 2, 0}, {7002, 2, 1}, {7003, 4, 0}};
    records.disks = {{7101, 5'000'000, 5'000'000, 1'000'000},
                     {7102, 5'000'000, 5'000'000, 1'000'000}};
    records.capsules = {{7201, -1'000'000, 5'000'000, 21'000'000, 5'000'000, 1'000'000}};
    const AnalyticFilteredLoweringResult lowered =
        lower_analytic_job_to_filtered_curves(records, 0);
    require(lowered.error == AnalyticFilteredLoweringError::none && lowered.value,
            "symmetric capsule/disk fixture did not lower");
    const AnalyticFilteredBooleanSelectionResult result = select(records, *lowered.value);
    require(result.error == AnalyticFilteredBooleanSelectionError::none &&
                result.telemetry.resolution_event_columns >= 2 &&
                !result.telemetry.unresolved_predicate_failure,
            "symmetric capsule/disk roots were not admitted as bounded event columns");
}

void test_unrelated_overlapping_x_enclosures_share_a_safe_event_column()
{
    const auto solve =
        [](double short_x_lower, double short_x_upper, bool reflect_x, bool reflect_y)
    {
        std::vector<std::array<double, 2>> vertices{
            {0, 0},
            {1000, 0},
            {1000, 1000},
            {500, 1000},
            {short_x_lower, 800},
            {short_x_upper, 788},
            {0, 788},
        };
        for (auto& vertex : vertices)
        {
            if (reflect_x)
                vertex[0] = 1000 - vertex[0];
            if (reflect_y)
                vertex[1] = 1000 - vertex[1];
        }
        if (reflect_x != reflect_y)
            std::reverse(vertices.begin(), vertices.end());
        AnalyticFilteredGeometry geometry;
        append_polygon(geometry, 1, vertices);
        return select(records_for({{1, {1}}}), geometry);
    };

    // The 12.37 nm span collapses to a non-singleton vertex whose x enclosure
    // touches the exact x=500 vertex 200 nm away in y. Both vertices retain two
    // incident edges, matching the isolated real-board failure mechanism.
    const AnalyticFilteredBooleanSelectionResult result = solve(500, 503, false, false);
    require(
        result.error == AnalyticFilteredBooleanSelectionError::none &&
            result.telemetry.material_faces == 1 &&
            result.arrangement.collapsed_spans.size() == 1 && result.telemetry.event_columns == 3 &&
            result.telemetry.resolution_event_columns >= 1 &&
            !result.telemetry.unresolved_predicate_failure,
        "y-separated overlapping x enclosures did not share a safe event column: error=" +
            std::to_string(static_cast<int>(result.error)) +
            " collapsed=" + std::to_string(result.arrangement.collapsed_spans.size()) +
            " faces=" + std::to_string(result.faces.size()) +
            " material=" + std::to_string(result.telemetry.material_faces) +
            " columns=" + std::to_string(result.telemetry.event_columns) +
            " unresolved=" + std::to_string(result.telemetry.unresolved_predicate_failure ? 1 : 0));

    // Reflecting x maps the enclosure to [497,500], so it sorts before the
    // exact x=500 event while preserving the geometric configuration.
    // Telemetry must still identify the column as resolution-dependent.
    const AnalyticFilteredBooleanSelectionResult non_singleton_first = solve(500, 503, true, false);
    require(non_singleton_first.error == AnalyticFilteredBooleanSelectionError::none &&
                non_singleton_first.arrangement.collapsed_spans.size() == 1 &&
                non_singleton_first.telemetry.event_columns == 3 &&
                non_singleton_first.telemetry.resolution_event_columns ==
                    result.telemetry.resolution_event_columns,
            "event-column telemetry depended on singleton/non-singleton sort order: error=" +
                std::to_string(static_cast<int>(non_singleton_first.error)) + " columns=" +
                std::to_string(non_singleton_first.telemetry.event_columns) + " correlated=" +
                std::to_string(non_singleton_first.telemetry.resolution_event_columns));

    const AnalyticFilteredBooleanSelectionResult empty_intersection = solve(501, 504, false, false);
    require(empty_intersection.error == AnalyticFilteredBooleanSelectionError::none &&
                empty_intersection.telemetry.event_columns == 4,
            "empty x-enclosure intersection was incorrectly folded into an event column: error=" +
                std::to_string(static_cast<int>(empty_intersection.error)) + " columns=" +
                std::to_string(empty_intersection.telemetry.event_columns) + " correlated=" +
                std::to_string(empty_intersection.telemetry.resolution_event_columns));

    for (const bool reflect_x : {false, true})
        for (const bool reflect_y : {false, true})
        {
            const AnalyticFilteredBooleanSelectionResult transformed =
                solve(500, 503, reflect_x, reflect_y);
            require(transformed.error == AnalyticFilteredBooleanSelectionError::none &&
                        transformed.arrangement.collapsed_spans.size() == 1 &&
                        transformed.telemetry.event_columns == 3 &&
                        !transformed.telemetry.unresolved_predicate_failure,
                    "reflected safe event column changed classification");
        }
}

void test_event_columns_require_one_common_x_intersection()
{
    using analytic_selection_detail::event_column_x_requires_resolution;
    using analytic_selection_detail::event_column_y_is_strictly_ordered;
    using analytic_selection_detail::extend_event_column_x_intersection;
    using analytic_selection_detail::Interval;
    const std::array<Interval, 3> spans{{{0, 2}, {1, 3}, {2.5, 4}}};
    std::array<std::uint32_t, 3> order{0, 1, 2};
    do
    {
        std::array<Interval, 3> sorted{spans[order[0]], spans[order[1]], spans[order[2]]};
        std::sort(
            sorted.begin(), sorted.end(), [](Interval left, Interval right)
            { return std::tie(left.lower, left.upper) < std::tie(right.lower, right.upper); });
        double common_lower = sorted[0].lower;
        double common_upper = sorted[0].upper;
        require(extend_event_column_x_intersection(sorted[1], common_lower, common_upper) &&
                    common_lower == 1 && common_upper == 2 &&
                    !extend_event_column_x_intersection(sorted[2], common_lower, common_upper),
                "transitive x-overlap chain changed common intersection under permutation");
    } while (std::next_permutation(order.begin(), order.end()));

    double boundary_lower = 500;
    double boundary_upper = 500;
    require(
        extend_event_column_x_intersection(Interval{500, 503}, boundary_lower, boundary_upper) &&
            boundary_lower == 500 && boundary_upper == 500,
        "boundary-touching x enclosures did not retain their common point");
    double empty_lower = 500;
    double empty_upper = 500;
    require(!extend_event_column_x_intersection(Interval{501, 504}, empty_lower, empty_upper),
            "empty x-enclosure intersection was accepted");
    require(event_column_y_is_strictly_ordered(Interval{200, 212}, Interval{800, 800}) &&
                !event_column_y_is_strictly_ordered(Interval{200, 240}, Interval{240, 280}),
            "event-column y ordering did not fail closed at enclosure contact");

    double reflected_lower = -503;
    double reflected_upper = -500;
    require(extend_event_column_x_intersection(Interval{-500, -500}, reflected_lower,
                                               reflected_upper) &&
                reflected_lower == -500 && reflected_upper == -500,
            "reflected boundary-touching x enclosures changed classification");
    require(event_column_x_requires_resolution(Interval{499, 501}) &&
                !event_column_x_requires_resolution(Interval{500, 500}),
            "resolution-dependent event-column telemetry depended on interval sort order");
}

void test_endpoint_partition_column_local_arc_authority()
{
    using analytic_selection_detail::endpoint_partition_arc_is_column_local;
    AnalyticArrangementEdgeNm edge;
    edge.kind = AnalyticAtomicCurveKind::circular_arc;
    edge.endpoint_authoritative_arc = true;
    edge.construction_carrier_id = 7;
    edge.circle.center = {{0, 0}, {0.1, 0.1}};
    edge.circle.radius = {100, 100};
    const std::uint64_t token = analytic_endpoint_arc_partition_column_token(1, true);
    AnalyticFilteredPointNm seam = {{100, 100}, {0.1, 0.1}, token};
    AnalyticFilteredPointNm endpoint = exact_point(100, 0);
    edge.carrier_start = endpoint;
    edge.carrier_end = seam;
    require(endpoint_partition_arc_is_column_local(edge, seam, endpoint) &&
                endpoint_partition_arc_is_column_local(edge, endpoint, seam),
            "authenticated endpoint/cardinal arc was not classified as column-local");

    AnalyticArrangementEdgeNm invalid = edge;
    invalid.endpoint_authoritative_arc = false;
    require(!endpoint_partition_arc_is_column_local(invalid, seam, endpoint),
            "unauthenticated arc was classified as column-local");
    invalid = edge;
    invalid.construction_carrier_id = 0;
    require(!endpoint_partition_arc_is_column_local(invalid, seam, endpoint),
            "carrierless arc was classified as column-local");
    AnalyticFilteredPointNm forged = seam;
    forged.construction_x_column_id = analytic_vertical_x_column_token(7);
    require(!endpoint_partition_arc_is_column_local(edge, forged, endpoint),
            "non-partition column identity authorized a local arc");
    AnalyticFilteredPointNm wrong_seam = seam;
    wrong_seam.x = {99, 99};
    require(!endpoint_partition_arc_is_column_local(edge, wrong_seam, endpoint),
            "non-cardinal endpoint authorized a local arc");
    AnalyticFilteredPointNm conflicting_endpoint = endpoint;
    conflicting_endpoint.construction_x_column_id =
        analytic_endpoint_arc_partition_column_token(2, true);
    require(!endpoint_partition_arc_is_column_local(edge, seam, conflicting_endpoint),
            "conflicting endpoint partition identities authorized a local arc");
    AnalyticFilteredPointNm unnamed_endpoint = endpoint;
    unnamed_endpoint.y = {-0.1, -0.1};
    require(!endpoint_partition_arc_is_column_local(edge, seam, unnamed_endpoint),
            "an unnamed integer endpoint authorized a local arc");
    AnalyticFilteredPointNm overlapping_y = endpoint;
    overlapping_y.y = {0.1, 0.1};
    require(!endpoint_partition_arc_is_column_local(edge, seam, overlapping_y),
            "non-strict endpoint y ordering authorized a local arc");
}

AnalyticFilteredBooleanSelectionResult nested_rectangles(std::uint32_t count)
{
    AnalyticFilteredGeometry geometry;
    std::vector<std::uint64_t> operands;
    operands.reserve(count);
    const double maximum = static_cast<double>(count * 3000 + 1000);
    for (std::uint32_t index = 0; index < count; ++index)
    {
        operands.push_back(static_cast<std::uint64_t>(index) + 1);
        append_rectangle(geometry, static_cast<std::uint64_t>(index) + 1,
                         static_cast<double>(index * 1000), maximum - index * 1000);
    }
    return select(records_for({{1, operands}}), geometry);
}

void test_nested_scaling_is_not_face_by_operand()
{
    const AnalyticFilteredBooleanSelectionResult small = nested_rectangles(48);
    const AnalyticFilteredBooleanSelectionResult large = nested_rectangles(96);
    require(small.error == AnalyticFilteredBooleanSelectionError::none &&
                large.error == AnalyticFilteredBooleanSelectionError::none,
            "nested scaling fixture failed");
    require(small.faces.size() == 49 && large.faces.size() == 97 &&
                large.telemetry.predicate_calls <= small.telemetry.predicate_calls * 3,
            "nested face selection exceeded near-N-log-N scaling");
    require(large.coverage_state_nodes.size() < 96U * 96U / 2U,
            "nested coverage states regressed toward copied face-by-operand storage");
}

void test_sibling_island_ownership()
{
    AnalyticFilteredGeometry geometry;
    std::vector<std::uint64_t> operands;
    for (std::uint32_t index = 0; index < 24; ++index)
    {
        operands.push_back(index + 1);
        const double offset = static_cast<double>(index * 3000);
        append_rectangle(geometry, index + 1, offset, offset + 1000);
    }
    const AnalyticFilteredBooleanSelectionResult result =
        select(records_for({{1, operands}}), geometry);
    require(result.error == AnalyticFilteredBooleanSelectionError::none &&
                result.faces.size() == 25 && result.faces[0].boundary_cycle_count == 24 &&
                result.telemetry.material_faces == 24,
            "sibling islands were not assigned to the unbounded face canonically");
}

double selected_line_area(const AnalyticFilteredBooleanSelectionResult& result)
{
    double twice_area = 0.0;
    for (const AnalyticFilteredSelectedFace& face : result.faces)
    {
        if (!face.material)
            continue;
        require(face.boundary_cycle_begin <= result.face_boundary_cycles.size() &&
                    face.boundary_cycle_count <=
                        result.face_boundary_cycles.size() - face.boundary_cycle_begin,
                "material face has an invalid boundary range");
        for (std::uint32_t boundary = 0; boundary < face.boundary_cycle_count; ++boundary)
        {
            const std::uint32_t cycle_index =
                result.face_boundary_cycles[face.boundary_cycle_begin + boundary];
            require(cycle_index < result.arrangement.cycles.size(),
                    "material face has an invalid boundary cycle");
            const AnalyticArrangementCycle& cycle = result.arrangement.cycles[cycle_index];
            require(cycle.half_edge_begin <= result.arrangement.cycle_half_edges.size() &&
                        cycle.half_edge_count <=
                            result.arrangement.cycle_half_edges.size() - cycle.half_edge_begin,
                    "material boundary has an invalid half-edge range");
            for (std::uint32_t offset = 0; offset < cycle.half_edge_count; ++offset)
            {
                const std::uint32_t half_edge =
                    result.arrangement.cycle_half_edges[cycle.half_edge_begin + offset];
                require(half_edge < result.arrangement.half_edges.size(),
                        "material boundary has an invalid half-edge");
                const AnalyticArrangementHalfEdge& current =
                    result.arrangement.half_edges[half_edge];
                require(current.next < result.arrangement.half_edges.size() &&
                            current.origin_vertex < result.arrangement.vertices.size(),
                        "material boundary has invalid topology");
                const AnalyticArrangementHalfEdge& next =
                    result.arrangement.half_edges[current.next];
                require(next.origin_vertex < result.arrangement.vertices.size(),
                        "material boundary has an invalid next vertex");
                const AnalyticFilteredPointNm& a =
                    result.arrangement.vertices[current.origin_vertex].point;
                const AnalyticFilteredPointNm& b =
                    result.arrangement.vertices[next.origin_vertex].point;
                require(a.x.lower == a.x.upper && a.y.lower == a.y.upper &&
                            b.x.lower == b.x.upper && b.y.lower == b.y.upper,
                        "line-area oracle requires exact singleton vertices");
                twice_area += a.x.lower * b.y.lower - b.x.lower * a.y.lower;
            }
        }
    }
    return twice_area / 2.0;
}

void test_ordered_stage_area_oracle()
{
    constexpr double cell = 1000.0;
    struct Rectangle
    {
        std::int32_t minimum_x;
        std::int32_t minimum_y;
        std::int32_t maximum_x;
        std::int32_t maximum_y;
    };
    constexpr std::array<Rectangle, 4> rectangles = {Rectangle{0, 0, 4, 4}, Rectangle{2, 1, 7, 5},
                                                     Rectangle{1, 2, 6, 7}, Rectangle{3, -2, 8, 3}};
    for (std::uint32_t union_mask = 0; union_mask < (1U << rectangles.size()); ++union_mask)
    {
        AnalyticRequestPacketRecords records;
        records.jobs = {{1, 0, static_cast<std::uint32_t>(rectangles.size())}};
        for (std::uint32_t index = 0; index < rectangles.size(); ++index)
        {
            const std::uint64_t operand_id = index + 1;
            const std::uint32_t vertex_begin = static_cast<std::uint32_t>(records.vertices.size());
            const std::uint32_t segment_begin = static_cast<std::uint32_t>(records.segments.size());
            const Rectangle& rectangle = rectangles[index];
            records.stages.push_back(
                {index + 1, (union_mask & (1U << index)) != 0 ? std::uint8_t{1} : std::uint8_t{2},
                 index, 1});
            records.operands.push_back({operand_id, 1, index});
            records.planar_regions.push_back({100 + index, index, 0, 0});
            records.vertices.push_back({1000 + index * 4,
                                        static_cast<std::int64_t>(rectangle.minimum_x * cell),
                                        static_cast<std::int64_t>(rectangle.minimum_y * cell)});
            records.vertices.push_back({1001 + index * 4,
                                        static_cast<std::int64_t>(rectangle.maximum_x * cell),
                                        static_cast<std::int64_t>(rectangle.minimum_y * cell)});
            records.vertices.push_back({1002 + index * 4,
                                        static_cast<std::int64_t>(rectangle.maximum_x * cell),
                                        static_cast<std::int64_t>(rectangle.maximum_y * cell)});
            records.vertices.push_back({1003 + index * 4,
                                        static_cast<std::int64_t>(rectangle.minimum_x * cell),
                                        static_cast<std::int64_t>(rectangle.maximum_y * cell)});
            for (std::uint32_t edge = 0; edge < 4; ++edge)
                records.segments.push_back(
                    {2000 + index * 4 + edge, 3000 + index * 4 + edge, 1, 0, false, 0, 0});
            records.rings.push_back({500 + index, vertex_begin, 4, segment_begin, 4, 0});
        }
        const AnalyticFilteredLoweringResult lowered =
            lower_analytic_job_to_filtered_curves(records, 0);
        require(lowered.error == AnalyticFilteredLoweringError::none && lowered.value,
                "ordered-stage area-oracle lowering failed for mask " + std::to_string(union_mask));
        const AnalyticFilteredBooleanSelectionResult result = select(records, *lowered.value);
        require(result.error == AnalyticFilteredBooleanSelectionError::none,
                "ordered-stage area-oracle solve failed for mask " + std::to_string(union_mask) +
                    " error=" + std::to_string(static_cast<int>(result.error)) +
                    " columns=" + std::to_string(result.telemetry.event_columns) +
                    " faces=" + std::to_string(result.faces.size()) + " arrangement_error=" +
                    std::to_string(static_cast<int>(result.arrangement.error)) +
                    " arrangement_work=" +
                    std::to_string(result.telemetry.arrangement_predicate_calls));

        std::uint32_t occupied_cells = 0;
        for (std::int32_t y = -2; y < 7; ++y)
            for (std::int32_t x = 0; x < 8; ++x)
            {
                bool material = false;
                for (std::uint32_t index = 0; index < rectangles.size(); ++index)
                {
                    const Rectangle& rectangle = rectangles[index];
                    const bool covered = x >= rectangle.minimum_x && x < rectangle.maximum_x &&
                                         y >= rectangle.minimum_y && y < rectangle.maximum_y;
                    if (covered)
                        material = (union_mask & (1U << index)) != 0;
                }
                occupied_cells += material ? 1U : 0U;
            }
        const double expected_area = static_cast<double>(occupied_cells) * cell * cell;
        require(std::abs(selected_line_area(result) - expected_area) < 0.5,
                "ordered-stage area oracle disagreed for mask " + std::to_string(union_mask));
    }
}

void test_exact_resource_boundaries()
{
    AnalyticFilteredGeometry geometry;
    append_rectangle(geometry, 1, 0, 1000);
    const AnalyticRequestPacketRecords records = records_for({{1, {1}}});
    const AnalyticBroadPhaseResult broad = build_analytic_curve_candidates(geometry.bounds);
    require(broad.error == AnalyticBroadPhaseError::none, "resource fixture broad phase failed");
    const AnalyticFilteredBooleanSelectionResult baseline =
        build_analytic_filtered_boolean_selection(records, 0, geometry, broad.pairs);
    require(baseline.error == AnalyticFilteredBooleanSelectionError::none,
            "resource fixture baseline failed");

    AnalyticSolverLimits exact_work;
    exact_work.predicate_calls = baseline.telemetry.predicate_calls;
    const AnalyticFilteredBooleanSelectionResult work_success =
        build_analytic_filtered_boolean_selection(records, 0, geometry, broad.pairs, exact_work);
    require(work_success.error == AnalyticFilteredBooleanSelectionError::none,
            "exact face-selection work budget did not succeed");
    --exact_work.predicate_calls;
    const AnalyticFilteredBooleanSelectionResult work_failure =
        build_analytic_filtered_boolean_selection(records, 0, geometry, broad.pairs, exact_work);
    require(work_failure.error == AnalyticFilteredBooleanSelectionError::resource_limit_exceeded &&
                work_failure.faces.empty() && work_failure.arrangement.edges.empty(),
            "one-unit-short face-selection work budget did not fail closed");

    AnalyticSolverLimits exact_memory;
    exact_memory.working_memory_bytes = baseline.telemetry.peak_working_memory_bytes;
    const AnalyticFilteredBooleanSelectionResult memory_success =
        build_analytic_filtered_boolean_selection(records, 0, geometry, broad.pairs, exact_memory);
    require(memory_success.error == AnalyticFilteredBooleanSelectionError::none,
            "exact face-selection memory budget did not succeed");
    --exact_memory.working_memory_bytes;
    const AnalyticFilteredBooleanSelectionResult memory_failure =
        build_analytic_filtered_boolean_selection(records, 0, geometry, broad.pairs, exact_memory);
    require(memory_failure.error ==
                    AnalyticFilteredBooleanSelectionError::resource_limit_exceeded &&
                memory_failure.faces.empty() && memory_failure.arrangement.edges.empty() &&
                memory_failure.telemetry.required_working_memory_bytes >
                    exact_memory.working_memory_bytes,
            "one-byte-short face-selection memory budget did not fail closed");
}

AnalyticFilteredBooleanSelectionResult disjoint_rectangles(std::uint32_t count)
{
    AnalyticFilteredGeometry geometry;
    std::vector<std::uint64_t> operands;
    operands.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index)
    {
        const std::uint64_t operand = static_cast<std::uint64_t>(index) + 1;
        const double minimum = static_cast<double>(index) * 2000.0;
        operands.push_back(operand);
        append_rectangle(geometry, operand, minimum, minimum + 1000.0);
    }
    return select(records_for({{1, operands}}), geometry);
}

void test_many_memberships_are_linear_and_fixed_capacity()
{
    constexpr std::uint32_t count = 257;
    const AnalyticFilteredBooleanSelectionResult baseline = disjoint_rectangles(count);
    require(baseline.error == AnalyticFilteredBooleanSelectionError::none &&
                baseline.telemetry.input_operands == count &&
                baseline.telemetry.transition_records == count * 4,
            "non-power-of-two membership fixture failed");

    AnalyticSolverLimits exact_memory;
    exact_memory.working_memory_bytes = baseline.telemetry.peak_working_memory_bytes;
    AnalyticFilteredGeometry geometry;
    std::vector<std::uint64_t> operands;
    operands.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index)
    {
        const std::uint64_t operand = static_cast<std::uint64_t>(index) + 1;
        const double minimum = static_cast<double>(index) * 2000.0;
        operands.push_back(operand);
        append_rectangle(geometry, operand, minimum, minimum + 1000.0);
    }
    const AnalyticRequestPacketRecords records = records_for({{1, operands}});
    const AnalyticBroadPhaseResult broad = build_analytic_curve_candidates(geometry.bounds);
    require(broad.error == AnalyticBroadPhaseError::none, "many-membership broad phase failed");
    AnalyticSolverLimits exact_work;
    exact_work.predicate_calls = baseline.telemetry.predicate_calls;
    const AnalyticFilteredBooleanSelectionResult work_success =
        build_analytic_filtered_boolean_selection(records, 0, geometry, broad.pairs, exact_work);
    require(work_success.error == AnalyticFilteredBooleanSelectionError::none &&
                work_success.telemetry.predicate_calls == baseline.telemetry.predicate_calls,
            "exact many-membership work budget failed or was nondeterministic");
    --exact_work.predicate_calls;
    const AnalyticFilteredBooleanSelectionResult work_failure =
        build_analytic_filtered_boolean_selection(records, 0, geometry, broad.pairs, exact_work);
    require(work_failure.error == AnalyticFilteredBooleanSelectionError::resource_limit_exceeded,
            "one-unit-short many-membership work budget succeeded");
    const AnalyticFilteredBooleanSelectionResult memory_success =
        build_analytic_filtered_boolean_selection(records, 0, geometry, broad.pairs, exact_memory);
    require(memory_success.error == AnalyticFilteredBooleanSelectionError::none,
            "exact non-power-of-two selection memory failed");
    --exact_memory.working_memory_bytes;
    const AnalyticFilteredBooleanSelectionResult memory_failure =
        build_analytic_filtered_boolean_selection(records, 0, geometry, broad.pairs, exact_memory);
    require(memory_failure.error == AnalyticFilteredBooleanSelectionError::resource_limit_exceeded,
            "one-byte-short non-power-of-two selection memory succeeded");

    std::uint64_t lower = 0;
    std::uint64_t upper = baseline.telemetry.peak_working_memory_bytes;
    while (lower < upper)
    {
        const std::uint64_t middle = lower + (upper - lower) / 2;
        AnalyticSolverLimits probe_limits;
        probe_limits.working_memory_bytes = middle;
        const AnalyticFilteredBooleanSelectionResult probe =
            build_analytic_filtered_boolean_selection(records, 0, geometry, broad.pairs,
                                                      probe_limits);
        if (probe.telemetry.arrangement_predicate_calls == 0)
            lower = middle + 1;
        else
            upper = middle;
    }
    require(lower != 0, "persistent-coverage admission threshold was not found");
    AnalyticSolverLimits admission_exact;
    admission_exact.working_memory_bytes = lower;
    const AnalyticFilteredBooleanSelectionResult exact_admission =
        build_analytic_filtered_boolean_selection(records, 0, geometry, broad.pairs,
                                                  admission_exact);
    --admission_exact.working_memory_bytes;
    const AnalyticFilteredBooleanSelectionResult short_admission =
        build_analytic_filtered_boolean_selection(records, 0, geometry, broad.pairs,
                                                  admission_exact);
    require(exact_admission.telemetry.arrangement_predicate_calls != 0 &&
                short_admission.error ==
                    AnalyticFilteredBooleanSelectionError::resource_limit_exceeded &&
                short_admission.telemetry.arrangement_predicate_calls == 0 &&
                short_admission.telemetry.arrangement_peak_working_memory_bytes == 0,
            "persistent-coverage admission did not reject at its exact one-byte boundary");

    const AnalyticFilteredBooleanSelectionResult small = disjoint_rectangles(64);
    const AnalyticFilteredBooleanSelectionResult large = disjoint_rectangles(128);
    require(small.error == AnalyticFilteredBooleanSelectionError::none &&
                large.error == AnalyticFilteredBooleanSelectionError::none &&
                large.telemetry.predicate_calls < small.telemetry.predicate_calls * 3,
            "membership/operand scaling regressed toward quadratic work");
}

void test_split_heavy_coverage_is_reserved_before_arrangement()
{
    constexpr std::uint32_t teeth = 64;
    AnalyticFilteredGeometry geometry;
    std::vector<std::uint64_t> operands;
    operands.reserve(teeth + 1);
    operands.push_back(1);
    append_axis_rectangle(geometry, 1, 0, -100, teeth * 1000.0 + 1000, 100);
    for (std::uint32_t index = 0; index < teeth; ++index)
    {
        const std::uint64_t operand = static_cast<std::uint64_t>(index) + 2;
        const double minimum_x = 500.0 + index * 1000.0;
        operands.push_back(operand);
        append_axis_rectangle(geometry, operand, minimum_x, -1000, minimum_x + 100, 1000);
    }
    const AnalyticRequestPacketRecords records = records_for({{1, operands}});
    const AnalyticBroadPhaseResult broad = build_analytic_curve_candidates(geometry.bounds);
    require(broad.error == AnalyticBroadPhaseError::none, "split-heavy comb broad phase failed");
    const AnalyticFilteredBooleanSelectionResult baseline =
        build_analytic_filtered_boolean_selection(records, 0, geometry, broad.pairs);
    require(baseline.error == AnalyticFilteredBooleanSelectionError::none &&
                baseline.telemetry.transition_records > geometry.curves.size() * 2,
            "split-heavy comb did not produce the expected sparse transition expansion");
    std::uint64_t lower = 0;
    std::uint64_t upper = AnalyticSolverLimits{}.working_memory_bytes;
    while (lower < upper)
    {
        const std::uint64_t middle = lower + (upper - lower) / 2;
        AnalyticSolverLimits probe_limits;
        probe_limits.working_memory_bytes = middle;
        const AnalyticFilteredBooleanSelectionResult probe =
            build_analytic_filtered_boolean_selection(records, 0, geometry, broad.pairs,
                                                      probe_limits);
        if (probe.error == AnalyticFilteredBooleanSelectionError::none)
            upper = middle;
        else
            lower = middle + 1;
    }
    AnalyticSolverLimits exact;
    exact.working_memory_bytes = lower;
    const AnalyticFilteredBooleanSelectionResult success =
        build_analytic_filtered_boolean_selection(records, 0, geometry, broad.pairs, exact);
    --exact.working_memory_bytes;
    const AnalyticFilteredBooleanSelectionResult failure =
        build_analytic_filtered_boolean_selection(records, 0, geometry, broad.pairs, exact);
    require(success.error == AnalyticFilteredBooleanSelectionError::none &&
                failure.error == AnalyticFilteredBooleanSelectionError::resource_limit_exceeded &&
                failure.telemetry.arrangement_predicate_calls == 0 &&
                failure.telemetry.arrangement_peak_working_memory_bytes == 0,
            "split-heavy coverage admission did not stop at its exact one-byte boundary");
}

void test_malformed_filtered_point_is_invalid()
{
    AnalyticFilteredGeometry geometry;
    append_rectangle(geometry, 1, 0, 1000);
    geometry.curves[0].start.x.lower = std::numeric_limits<double>::quiet_NaN();
    const AnalyticFilteredBooleanSelectionResult result =
        build_analytic_filtered_boolean_selection(records_for({{1, {1}}}), 0, geometry, {});
    require(result.error == AnalyticFilteredBooleanSelectionError::invalid_argument &&
                result.telemetry.arrangement_predicate_calls == 0,
            "malformed filtered point was misclassified as resource exhaustion");
}

void test_noncanonical_candidate_pairs_are_invalid()
{
    AnalyticFilteredGeometry geometry;
    append_line(geometry, 1, 0, 0, 1000, 0, true);
    append_line(geometry, 1, 0, 100, 1000, 100, false);
    append_line(geometry, 1, 0, 200, 1000, 200, false);
    const AnalyticRequestPacketRecords records = records_for({{1, {1}}});
    const std::array<std::vector<AnalyticCurvePair>, 2> malformed = {
        std::vector<AnalyticCurvePair>{{1, 2}, {1, 2}},
        std::vector<AnalyticCurvePair>{{1, 3}, {1, 2}},
    };
    for (const auto& pairs : malformed)
    {
        for (const std::uint64_t memory :
             {AnalyticSolverLimits{}.working_memory_bytes, std::uint64_t{9}})
        {
            AnalyticSolverLimits limits;
            limits.working_memory_bytes = memory;
            const AnalyticFilteredBooleanSelectionResult result =
                build_analytic_filtered_boolean_selection(records, 0, geometry, pairs, limits);
            require(result.error == AnalyticFilteredBooleanSelectionError::invalid_argument &&
                        result.telemetry.arrangement_predicate_calls == 0 &&
                        result.telemetry.arrangement_peak_working_memory_bytes == 0,
                    "noncanonical candidate pairs produced a budget-dependent outcome");
        }
    }
}

void test_zero_operand_stage_scans_are_metered()
{
    constexpr std::uint32_t count = 257;
    AnalyticRequestPacketRecords records;
    records.jobs = {{1, 0, count}};
    for (std::uint32_t index = 0; index < count; ++index)
        records.stages.push_back({static_cast<std::uint64_t>(index) + 1, 1, 0, 0});
    const AnalyticFilteredBooleanSelectionResult baseline = select(records, {});
    require(baseline.error == AnalyticFilteredBooleanSelectionError::none &&
                baseline.telemetry.admission_work_units >= count * 2 &&
                baseline.telemetry.predicate_calls >= count * 4,
            "repeated zero-operand stage traversals were not metered");
    AnalyticSolverLimits one_short;
    one_short.predicate_calls = baseline.telemetry.predicate_calls - 1;
    const AnalyticFilteredBooleanSelectionResult failure =
        build_analytic_filtered_boolean_selection(records, 0, {}, {}, one_short);
    require(failure.error == AnalyticFilteredBooleanSelectionError::resource_limit_exceeded &&
                failure.arrangement.edges.empty() && failure.faces.empty(),
            "one-unit-short zero-operand stage job did not fail closed");
}

void test_collapsed_topology_is_reserved_before_arrangement()
{
    constexpr std::uint32_t count = 257;
    AnalyticFilteredGeometry geometry;
    std::vector<std::uint64_t> operands;
    operands.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index)
    {
        const std::uint64_t operand = static_cast<std::uint64_t>(index) + 1;
        const double x = static_cast<double>(index) * 1000.0;
        operands.push_back(operand);
        append_line(geometry, operand, x, 0, x + 20, 20, true);
    }
    const AnalyticRequestPacketRecords records = records_for({{1, operands}});
    const AnalyticBroadPhaseResult broad = build_analytic_curve_candidates(geometry.bounds);
    require(broad.error == AnalyticBroadPhaseError::none && broad.pairs.empty(),
            "collapsed-topology broad phase failed");
    const AnalyticFilteredBooleanSelectionResult baseline =
        build_analytic_filtered_boolean_selection(records, 0, geometry, broad.pairs);
    AnalyticFilteredArrangementMinimumRequirements minimum;
    require(estimate_analytic_filtered_arrangement_minimum_requirements(geometry, 0, minimum),
            "collapsed-topology minimum estimate failed");
    require(baseline.error == AnalyticFilteredBooleanSelectionError::none &&
                baseline.arrangement.collapsed_spans.size() == count,
            "collapsed-topology baseline failed");
    AnalyticSolverLimits one_short;
    one_short.predicate_calls = baseline.telemetry.predicate_calls - 1;
    const AnalyticFilteredBooleanSelectionResult failure =
        build_analytic_filtered_boolean_selection(records, 0, geometry, broad.pairs, one_short);
    require(failure.error == AnalyticFilteredBooleanSelectionError::resource_limit_exceeded &&
                failure.telemetry.arrangement_predicate_calls == 0 &&
                failure.telemetry.arrangement_peak_working_memory_bytes == 0,
            "collapsed topology exhausted work only after arrangement execution: baseline=" +
                std::to_string(baseline.telemetry.predicate_calls) +
                " admission=" + std::to_string(baseline.telemetry.admission_work_units) +
                " arrangement=" + std::to_string(baseline.telemetry.arrangement_predicate_calls) +
                " minimum=" + std::to_string(minimum.predicate_calls) + " failure-arrangement=" +
                std::to_string(failure.telemetry.arrangement_predicate_calls) +
                " failure-total=" + std::to_string(failure.telemetry.predicate_calls) +
                " failure-admission=" + std::to_string(failure.telemetry.admission_work_units));
}

void test_strict_selection_policy_and_admission_boundaries()
{
    AnalyticFilteredGeometry geometry;
    append_axis_rectangle(geometry, 1, -1000, 0, 0, 1000);
    append_axis_rectangle(geometry, 2, 1, -1000, 1001, 0);
    const AnalyticRequestPacketRecords records = records_for({{1, {1, 2}}});
    const AnalyticFilteredBooleanSelectionResult baseline = strict_select(records, geometry);
    require(baseline.error == AnalyticFilteredBooleanSelectionError::none &&
                baseline.arrangement.vertices.size() == 8 && baseline.telemetry.material_faces == 2,
            "strict selection policy did not preserve disjoint 1 nm geometry");

    AnalyticSolverLimits exact_work = kAnalyticSolverHardLimits;
    exact_work.predicate_calls = baseline.telemetry.predicate_calls;
    require(strict_select(records, geometry, exact_work).error ==
                AnalyticFilteredBooleanSelectionError::none,
            "strict selection rejected its exact measured work boundary");
    --exact_work.predicate_calls;
    require(strict_select(records, geometry, exact_work).error ==
                AnalyticFilteredBooleanSelectionError::resource_limit_exceeded,
            "strict selection accepted a one-unit-short work boundary");

    AnalyticSolverLimits exact_memory = kAnalyticSolverHardLimits;
    exact_memory.working_memory_bytes = baseline.telemetry.peak_working_memory_bytes;
    require(strict_select(records, geometry, exact_memory).error ==
                AnalyticFilteredBooleanSelectionError::none,
            "strict selection rejected its exact measured memory boundary");
    --exact_memory.working_memory_bytes;
    require(strict_select(records, geometry, exact_memory).error ==
                AnalyticFilteredBooleanSelectionError::resource_limit_exceeded,
            "strict selection accepted a one-byte-short memory boundary");

    AnalyticFilteredGeometry thin;
    append_axis_rectangle(thin, 3, 0, 0, 1, 1000);
    AnalyticFilteredArrangementMinimumRequirements default_minimum;
    AnalyticFilteredArrangementMinimumRequirements strict_minimum;
    require(estimate_analytic_filtered_arrangement_minimum_requirements(thin, 0, default_minimum) &&
                analytic_execution_detail::estimate_arrangement_minimum_requirements(
                    thin, 0, strict_minimum, analytic_execution_detail::kStrictPublishedGeometry) &&
                strict_minimum.guaranteed_spans > default_minimum.guaranteed_spans &&
                strict_minimum.possible_collapsed_domains <
                    default_minimum.possible_collapsed_domains,
            "strict selection admission reused the default collapse estimate");
}

void test_empty_job()
{
    const AnalyticRequestPacketRecords records = records_for({{1, {}}});
    const AnalyticFilteredBooleanSelectionResult result = select(records, {});
    require(result.error == AnalyticFilteredBooleanSelectionError::none &&
                result.faces.size() == 1 && result.faces[0].unbounded && !result.faces[0].material,
            "empty job was not a successful unbounded no-op");
}

std::string parity_vector()
{
    AnalyticFilteredGeometry square_geometry;
    append_rectangle(square_geometry, 1, 0, 1000);
    const AnalyticFilteredBooleanSelectionResult square_result =
        select(records_for({{1, {1}}}), square_geometry);
    const AnalyticFilteredBooleanSelectionResult disk_result =
        select_disks({{10, 0, 0, 1000}, {11, 1000, 0, 1000}});
    const AnalyticFilteredBooleanSelectionResult nested_result = nested_rectangles(12);
    AnalyticRequestPacketRecords capsule_records;
    capsule_records.jobs = {{1, 0, 1}};
    capsule_records.stages = {{1, 1, 0, 1}};
    capsule_records.operands = {{1, 4, 0}};
    capsule_records.capsules = {{10, 0, 0, 0, 10000, 1001}};
    const AnalyticFilteredLoweringResult capsule_lowered =
        lower_analytic_job_to_filtered_curves(capsule_records, 0);
    require(capsule_lowered.error == AnalyticFilteredLoweringError::none && capsule_lowered.value,
            "boolean-selection parity capsule lowering failed");
    const AnalyticFilteredBooleanSelectionResult capsule_result =
        select(capsule_records, *capsule_lowered.value);
    require(square_result.error == AnalyticFilteredBooleanSelectionError::none &&
                disk_result.error == AnalyticFilteredBooleanSelectionError::none &&
                nested_result.error == AnalyticFilteredBooleanSelectionError::none &&
                capsule_result.error == AnalyticFilteredBooleanSelectionError::none,
            "boolean-selection parity fixture failed");
    const AnalyticFilteredBooleanSelectionResult seam_result =
        test_seam_adjacent_intersections_remain_distinct();

    std::ostringstream output;
    output << std::hex << std::setfill('0');
    const auto append_u64 = [&output](std::uint64_t value) { output << std::setw(16) << value; };
    const auto append_double = [&append_u64](double value)
    {
        std::uint64_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        append_u64(bits);
    };
    const auto append_point = [&](const AnalyticFilteredPointNm& point)
    {
        append_double(point.x.lower);
        append_double(point.x.upper);
        append_double(point.y.lower);
        append_double(point.y.upper);
        append_u64(point.construction_x_column_id);
    };
    const auto append_result = [&](const AnalyticFilteredBooleanSelectionResult& result)
    {
        append_u64(static_cast<std::uint8_t>(result.error));
        append_u64(static_cast<std::uint64_t>(result.origin_x_nm));
        append_u64(static_cast<std::uint64_t>(result.origin_y_nm));
        append_u64(result.arrangement.vertices.size());
        for (const AnalyticArrangementVertexNm& vertex : result.arrangement.vertices)
            append_point(vertex.point);
        append_u64(result.arrangement.edges.size());
        for (const AnalyticArrangementEdgeNm& edge : result.arrangement.edges)
        {
            append_u64(edge.start_vertex);
            append_u64(edge.end_vertex);
            append_u64(static_cast<std::uint8_t>(edge.kind));
            append_u64(static_cast<std::uint8_t>(edge.x_monotone_branch));
            append_u64(edge.has_construction_line_direction ? 1 : 0);
            append_u64(static_cast<std::uint64_t>(edge.construction_line_dx));
            append_u64(static_cast<std::uint64_t>(edge.construction_line_dy));
        }
        append_u64(result.arrangement.cycles.size());
        for (const AnalyticArrangementCycle& cycle : result.arrangement.cycles)
        {
            append_u64(cycle.half_edge_begin);
            append_u64(cycle.half_edge_count);
            append_u64(cycle.component);
            append_u64(cycle.counterclockwise ? 1 : 0);
        }
        append_u64(result.half_edge_faces.size());
        for (const std::uint32_t face : result.half_edge_faces)
            append_u64(face);
        append_u64(result.faces.size());
        for (const AnalyticFilteredSelectedFace& face : result.faces)
        {
            append_u64(face.boundary_cycle_begin);
            append_u64(face.boundary_cycle_count);
            append_u64(face.coverage_state_root);
            append_u64(face.positive_stage_begin);
            append_u64(face.active_removal_stage);
            append_u64(face.unbounded ? 1 : 0);
            append_u64(face.material ? 1 : 0);
        }
        append_u64(result.face_boundary_cycles.size());
        for (const std::uint32_t cycle : result.face_boundary_cycles)
            append_u64(cycle);
        append_u64(result.coverage_state_nodes.size());
        for (const AnalyticFilteredCoverageStateNode& node : result.coverage_state_nodes)
        {
            append_u64(node.left);
            append_u64(node.right);
        }
        const AnalyticFilteredBooleanSelectionTelemetry& telemetry = result.telemetry;
        append_u64(telemetry.admission_work_units);
        append_u64(telemetry.input_stages);
        append_u64(telemetry.input_operands);
        append_u64(telemetry.input_cycles);
        append_u64(telemetry.event_columns);
        append_u64(telemetry.resolution_event_columns);
        append_u64(telemetry.sweep_status_node_visits);
        append_u64(telemetry.sweep_status_update_work_units);
        append_u64(telemetry.disjoint_set_node_visits);
        append_u64(telemetry.face_gap_unions);
        append_u64(telemetry.emitted_faces);
        append_u64(telemetry.transition_records);
        append_u64(telemetry.dual_adjacency_visits);
        append_u64(telemetry.non_tree_edge_validations);
        append_u64(telemetry.coverage_state_nodes);
        append_u64(telemetry.coverage_state_table_probes);
        append_u64(telemetry.coverage_state_update_work_units);
        append_u64(telemetry.stage_state_update_work_units);
        append_u64(telemetry.material_faces);
        append_u64(telemetry.sort_work_units);
        append_u64(telemetry.arrangement_predicate_calls);
        append_u64(telemetry.arrangement_peak_working_memory_bytes);
        append_u64(telemetry.predicate_calls);
        append_u64(telemetry.peak_working_memory_bytes);
        append_u64(telemetry.algebraic_fallback_calls);
    };
    append_result(square_result);
    append_result(disk_result);
    append_result(nested_result);
    append_result(capsule_result);
    append_result(seam_result);
    return output.str();
}

} // namespace

int main()
{
    test_single_square();
    test_nested_components();
    test_annular_material_side();
    test_add_subtract_add();
    test_overlapping_rectangles();
    test_lowered_disk();
    test_disjoint_disk_admission_remains_sparse();
    test_large_origin_is_retained();
    test_seam_adjacent_intersections_remain_distinct();
    test_irrational_overlapping_disks();
    test_tangent_disks_do_not_join_faces();
    test_coincident_and_difference_only_disks();
    test_annulus_and_irrational_capsule();
    test_symmetric_capsule_disk_intersections_share_bounded_columns();
    test_unrelated_overlapping_x_enclosures_share_a_safe_event_column();
    test_event_columns_require_one_common_x_intersection();
    test_endpoint_partition_column_local_arc_authority();
    test_nested_scaling_is_not_face_by_operand();
    test_sibling_island_ownership();
    test_ordered_stage_area_oracle();
    test_exact_resource_boundaries();
    test_many_memberships_are_linear_and_fixed_capacity();
    test_split_heavy_coverage_is_reserved_before_arrangement();
    test_malformed_filtered_point_is_invalid();
    test_noncanonical_candidate_pairs_are_invalid();
    test_zero_operand_stage_scans_are_metered();
    test_collapsed_topology_is_reserved_before_arrangement();
    test_strict_selection_policy_and_admission_boundaries();
    test_empty_job();
    std::cout << "ANALYTIC_FILTERED_BOOLEAN_SELECTION_VECTOR=" << parity_vector() << '\n';
    return 0;
}

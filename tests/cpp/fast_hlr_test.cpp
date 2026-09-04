#include "fast_hlr_occt.h"
#include "fast_hlr_reconstruct.h"
#include "fast_mesh_shadow_outline.h"
#include "geometer/fast_hlr.h"

#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

bool near(double actual, double expected, double tolerance = 1.0e-6)
{
    return std::fabs(actual - expected) <= tolerance;
}

geometer::ProjectionViewSpec top_view()
{
    return {"top", {0.0, 0.0, 1.0}, {0.0, 1.0, 0.0}};
}

geometer::FastHlrPreparedMesh prepare(const geometer::FastHlrIndexedMesh& mesh,
                                      const geometer::FastHlrOptions& options = {})
{
    geometer::FastHlrPreparedMesh prepared;
    geometer::Status status;
    const int code = geometer::prepare_fast_hlr_mesh(mesh, options, &prepared, &status);
    require(code == 0, "fast HLR preparation should succeed: " + status.message);
    return prepared;
}

void square_builds_shared_adjacency()
{
    geometer::FastHlrIndexedMesh mesh;
    mesh.vertices = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 1.0, 0.0}};
    mesh.triangles = {{{0, 1, 2}, 10}, {{0, 2, 3}, 10}};

    const geometer::FastHlrPreparedMesh prepared = prepare(mesh);
    require(prepared.triangles.size() == 2, "square should keep two triangles");
    require(prepared.edges.size() == 5, "square should have four boundaries and one shared edge");
    std::size_t boundary_count = 0;
    std::size_t shared_count = 0;
    for (const geometer::FastHlrPreparedEdge& edge : prepared.edges)
    {
        boundary_count += edge.incident_count == 1 ? 1 : 0;
        shared_count += edge.incident_count == 2 ? 1 : 0;
    }
    require(boundary_count == 4, "square should have four boundary edges");
    require(shared_count == 1, "square should have one shared edge");
}

void indexed_mesh_welds_duplicate_seams_by_distance()
{
    geometer::FastHlrIndexedMesh duplicated;
    duplicated.vertices = {{0.099, 0.099, 0.0}, {1.0, 0.0, 0.0},     {0.999, 0.999, 0.0},
                           {0.101, 0.101, 0.0}, {1.001, 1.001, 0.0}, {0.0, 1.0, 0.0}};
    duplicated.triangles = {{{0, 1, 2}, 1}, {{3, 4, 5}, 1}};
    geometer::FastHlrOptions options;
    options.weld_tolerance = 0.01;

    const geometer::FastHlrPreparedMesh prepared = prepare(duplicated, options);
    require(prepared.vertices.size() == 4,
            "duplicate seam vertices in adjacent grid cells should weld");
    require(prepared.edges.size() == 5, "welded square should have one shared edge");

    geometer::FastHlrIndexedMesh separated;
    separated.vertices = {{0.01, 0.01, 0.0}, {1.0, 0.0, 0.0}, {0.01, 1.0, 0.0},
                          {0.09, 0.09, 0.0}, {1.0, 0.2, 0.0}, {0.09, 0.9, 0.0}};
    separated.triangles = {{{0, 1, 2}, 1}, {{3, 4, 5}, 2}};
    options.weld_tolerance = 0.1;
    const geometer::FastHlrPreparedMesh distinct = prepare(separated, options);
    require(distinct.vertices.size() == 6,
            "vertices farther than radial tolerance must not weld within one grid cell");

    geometer::FastHlrIndexedMesh translated;
    constexpr double offset = 1.0e13;
    translated.vertices = {{offset, 0.0, 0.0},         {offset + 10.0, 0.0, 0.0},
                           {offset + 10.0, 10.0, 0.0}, {offset, 0.0, 0.0},
                           {offset + 10.0, 10.0, 0.0}, {offset, 10.0, 0.0}};
    translated.triangles = {{{0, 1, 2}, 1}, {{3, 4, 5}, 1}};
    const geometer::FastHlrPreparedMesh translated_prepared = prepare(translated);
    require(translated_prepared.vertices.size() == 4 && translated_prepared.edges.size() == 5,
            "welding should be invariant under a large common translation");
}

void one_shot_matches_reusable_preparation()
{
    geometer::FastHlrIndexedMesh mesh;
    mesh.vertices = {{0.0, 0.0, 0.0}, {2.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
    mesh.triangles = {{{0, 1, 2}, 3}};
    const geometer::FastHlrPreparedMesh prepared = prepare(mesh);
    geometer::ProjectedModeGeometry reusable;
    geometer::ProjectedModeGeometry one_shot;
    geometer::FastHlrStatistics reusable_stats;
    geometer::FastHlrStatistics one_shot_stats;
    geometer::Status status;
    require(geometer::project_fast_hlr_detail(prepared, top_view(), {}, &reusable, nullptr,
                                              &reusable_stats, &status) == 0,
            "reusable projection should succeed: " + status.message);
    require(geometer::project_fast_hlr_detail(mesh, top_view(), {}, &one_shot, nullptr,
                                              &one_shot_stats, &status) == 0,
            "one-shot projection should succeed: " + status.message);
    require(one_shot.segments.size() == reusable.segments.size(),
            "one-shot and reusable projection should emit the same segment count");
    require(one_shot_stats.visible_segments == reusable_stats.visible_segments &&
                one_shot_stats.candidate_edges == reusable_stats.candidate_edges,
            "one-shot and reusable projection should report equivalent statistics");
    for (std::size_t index = 0; index < reusable.segments.size(); ++index)
    {
        const auto& left = reusable.segments[index];
        const auto& right = one_shot.segments[index];
        require(near(left.x1, right.x1) && near(left.y1, right.y1) && near(left.x2, right.x2) &&
                    near(left.y2, right.y2),
                "one-shot and reusable segments should match");
    }
}

void invalid_indices_and_limits_are_rejected()
{
    geometer::FastHlrIndexedMesh invalid;
    invalid.vertices = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
    invalid.triangles = {{{0, 1, 9}, 0}};
    geometer::FastHlrPreparedMesh prepared;
    geometer::Status status;
    require(geometer::prepare_fast_hlr_mesh(invalid, {}, &prepared, &status) == 5,
            "out-of-range triangle indices should be rejected");

    geometer::FastHlrOptions limited;
    limited.limits.max_vertices = 2;
    require(geometer::prepare_fast_hlr_mesh(invalid, limited, &prepared, &status) == 3,
            "vertex limits should be enforced before triangle validation");

    geometer::FastHlrIndexedMesh square;
    square.vertices = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 1.0, 0.0}};
    square.triangles = {{{0, 1, 2}, 1}, {{0, 2, 3}, 1}};
    limited = {};
    limited.limits.max_edges = 4;
    require(geometer::prepare_fast_hlr_mesh(square, limited, &prepared, &status) == 3,
            "edge limits should fail during adjacency construction");
}

void malformed_prepared_data_and_invalid_options_are_rejected()
{
    geometer::FastHlrIndexedMesh mesh;
    mesh.vertices = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
    mesh.triangles = {{{0, 1, 2}, 1}};
    geometer::FastHlrPreparedMesh prepared = prepare(mesh);
    prepared.triangles[0].vertices[2] = 99;

    geometer::ProjectedModeGeometry visible;
    geometer::Status status;
    require(geometer::project_fast_hlr_detail(prepared, top_view(), {}, &visible, nullptr, nullptr,
                                              &status) == 5,
            "malformed prepared triangle indices should be rejected");

    geometer::FastHlrOptions invalid;
    invalid.projected_tolerance = -1.0;
    require(geometer::prepare_fast_hlr_mesh(mesh, invalid, &prepared, &status) == 4,
            "negative projected tolerance should be rejected");

    prepared = prepare(mesh);
    geometer::FastHlrOptions bounded;
    bounded.limits.max_grid_references = 0;
    require(geometer::project_fast_hlr_detail(prepared, top_view(), bounded, &visible, nullptr,
                                              nullptr, &status) == 6,
            "spatial-index references should obey their resource limit");

    prepared = prepare(mesh);
    prepared.triangles[0].normal = {0.0, 0.0, 2.0};
    require(geometer::project_fast_hlr_detail(prepared, top_view(), {}, &visible, nullptr, nullptr,
                                              &status) == 5,
            "non-unit prepared normals should be rejected");

    prepared = prepare(mesh);
    prepared.triangles[0].normal = {0.0, 0.0, -1.0};
    require(geometer::project_fast_hlr_detail(prepared, top_view(), {}, &visible, nullptr, nullptr,
                                              &status) == 5,
            "prepared normals inconsistent with triangle geometry should be rejected");

    prepared = prepare(mesh);
    prepared.vertices[2] = {2.0, 0.0, 0.0};
    require(geometer::project_fast_hlr_detail(prepared, top_view(), {}, &visible, nullptr, nullptr,
                                              &status) == 5,
            "degenerate prepared triangle geometry should be rejected");

    prepared = prepare(mesh);
    prepared.edges.pop_back();
    require(geometer::project_fast_hlr_detail(prepared, top_view(), {}, &visible, nullptr, nullptr,
                                              &status) == 5,
            "prepared adjacency missing a triangle side should be rejected");
}

void smooth_internal_edge_can_be_a_silhouette()
{
    geometer::FastHlrIndexedMesh mesh;
    mesh.vertices = {{0.0, -1.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 0.0, 1.0}, {-1.0, 0.0, 1.0}};
    mesh.triangles = {{{0, 1, 2}, 7}, {{0, 1, 3}, 7}};
    const geometer::FastHlrPreparedMesh prepared = prepare(mesh);

    geometer::FastHlrOptions options;
    options.include_boundaries = false;
    options.include_creases = false;
    options.include_silhouettes = true;
    geometer::ProjectedModeGeometry visible;
    geometer::FastHlrStatistics statistics;
    geometer::Status status;
    const int code = geometer::project_fast_hlr_detail(prepared, top_view(), options, &visible,
                                                       nullptr, &statistics, &status);
    require(code == 0, "silhouette projection should succeed: " + status.message);
    require(statistics.silhouette_edges == 1, "shared front/back edge should be a silhouette");
    require(visible.segments.size() == 1, "silhouette-only projection should emit one segment");
    require(near(visible.segments[0].x1, 0.0) && near(visible.segments[0].x2, 0.0),
            "silhouette should project onto x=0");
}

void grazing_face_edge_is_a_silhouette()
{
    geometer::FastHlrIndexedMesh mesh;
    mesh.vertices = {{0.0, -1.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};
    mesh.triangles = {{{0, 1, 2}, 1}, {{1, 0, 3}, 2}};
    const geometer::FastHlrPreparedMesh prepared = prepare(mesh);

    geometer::FastHlrOptions options;
    options.include_boundaries = false;
    options.include_creases = false;
    options.include_silhouettes = true;
    geometer::ProjectedModeGeometry visible;
    geometer::FastHlrStatistics statistics;
    geometer::Status status;
    require(geometer::project_fast_hlr_detail(prepared, top_view(), options, &visible, nullptr,
                                              &statistics, &status) == 0,
            "grazing silhouette projection should succeed: " + status.message);
    require(statistics.silhouette_edges == 1,
            "front-to-grazing shared edge should be a silhouette");
}

void oblique_view_uses_the_expected_orthonormal_basis()
{
    geometer::FastHlrIndexedMesh mesh;
    mesh.vertices = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
    mesh.triangles = {{{0, 1, 2}, 1}};
    const geometer::FastHlrPreparedMesh prepared = prepare(mesh);
    geometer::ProjectionViewSpec view = {"oblique", {1.0, 0.0, 1.0}, {0.0, 1.0, 0.0}};
    geometer::ProjectedModeGeometry visible;
    geometer::Status status;
    require(geometer::project_fast_hlr_detail(prepared, view, {}, &visible, nullptr, nullptr,
                                              &status) == 0,
            "oblique projection should succeed: " + status.message);
    require(visible.segments.size() == 3, "one triangle should emit three boundary segments");
    require(near(visible.segments[0].x1, 0.0) && near(visible.segments[0].y1, 0.0) &&
                near(visible.segments[0].x2, std::sqrt(0.5)) && near(visible.segments[0].y2, 0.0),
            "oblique basis should map +X onto +sqrt(1/2) projected X");
}

void collinear_boundary_fragments_are_joined_without_crossing_corners()
{
    geometer::FastHlrIndexedMesh mesh;
    mesh.vertices = {
        {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {2.0, 0.0, 0.0}, {2.0, 1.0, 0.0}, {0.0, 1.0, 0.0}};
    mesh.triangles = {{{0, 1, 4}, 10}, {{1, 3, 4}, 10}, {{1, 2, 3}, 10}};
    const geometer::FastHlrPreparedMesh prepared = prepare(mesh);

    geometer::ProjectedModeGeometry visible;
    geometer::FastHlrStatistics statistics;
    geometer::Status status;
    require(geometer::project_fast_hlr_detail(prepared, top_view(), {}, &visible, nullptr,
                                              &statistics, &status) == 0,
            "collinear reconstruction should succeed: " + status.message);
    require(statistics.raw_visible_segments == 5,
            "subdivided rectangle should begin with five visible boundary fragments");
    require(statistics.collinear_joins == 1,
            "the two bottom fragments should become one exact collinear segment");
    require(visible.segments.size() == 4,
            "collinear reconstruction must retain all four rectangle corners");

    geometer::FastHlrOptions compact_limit;
    compact_limit.limits.max_output_segments = 4;
    require(geometer::project_fast_hlr_detail(prepared, top_view(), compact_limit, &visible,
                                              nullptr, nullptr, &status) == 0,
            "the final output limit should apply after safe reconstruction");
    compact_limit.limits.max_fragments = 4;
    require(geometer::project_fast_hlr_detail(prepared, top_view(), compact_limit, &visible,
                                              nullptr, nullptr, &status) == 6,
            "the separate raw-fragment work limit should stop reconstruction input growth");
    compact_limit.limits.max_fragments = 5;
    compact_limit.limits.max_output_segments = 3;
    require(geometer::project_fast_hlr_detail(prepared, top_view(), compact_limit, &visible,
                                              nullptr, nullptr, &status) == 7,
            "the compact output should retain its own final segment limit");
}

void collinear_reconstruction_handles_long_reversed_chains_and_branches()
{
    using geometer::fast_hlr_internal::FragmentProvenance;
    using geometer::fast_hlr_internal::ProjectedFragment;
    const FragmentProvenance provenance = {1, 10, geometer::kFastHlrUnspecifiedSourceFace, 0};
    const std::vector<ProjectedFragment> chain = {
        {{0.0, 0.0, 1.0, 0.0}, 0, 1, provenance},
        {{2.0, 0.0, 1.0, 0.0}, 2, 1, provenance},
        {{2.0, 0.0, 3.0, 0.0}, 2, 3, provenance},
    };
    geometer::fast_hlr_internal::ReconstructionStatistics statistics;
    std::vector<geometer::ProjectedSegment> result =
        geometer::fast_hlr_internal::reconstruct_collinear_fragments(chain, &statistics);
    require(result.size() == 1 && statistics.joins == 2,
            "a three-piece collinear chain should join despite a reversed middle segment");
    require(near(std::min(result[0].x1, result[0].x2), 0.0) &&
                near(std::max(result[0].x1, result[0].x2), 3.0),
            "the reconstructed chain should retain both outer endpoints");

    std::vector<ProjectedFragment> branched = chain;
    branched.push_back({{1.0, 0.0, 1.0, 1.0}, 1, 4, provenance});
    result = geometer::fast_hlr_internal::reconstruct_collinear_fragments(branched, &statistics);
    require(result.size() == 3,
            "a branch must lock its vertex while an independent unbranched tail may still join");
}

void projected_contact_without_shared_topology_is_not_joined()
{
    geometer::FastHlrIndexedMesh mesh;
    mesh.vertices = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0},
                     {1.0, 0.0, 0.0}, {2.0, 0.0, 0.0}, {2.0, 1.0, 0.0}};
    mesh.triangles = {{{0, 1, 2}, 10}, {{3, 4, 5}, 10}};
    const geometer::FastHlrPreparedMesh prepared = prepare(mesh);

    geometer::ProjectedModeGeometry visible;
    geometer::FastHlrStatistics statistics;
    geometer::Status status;
    require(geometer::project_fast_hlr_detail(prepared, top_view(), {}, &visible, nullptr,
                                              &statistics, &status) == 0,
            "disconnected projection should succeed: " + status.message);
    require(statistics.raw_visible_segments == 6 && visible.segments.size() == 6,
            "coincident projected endpoints without a shared vertex must not be joined");
}

void ambiguous_source_face_provenance_disables_cross_edge_joining()
{
    geometer::FastHlrIndexedMesh mesh;
    mesh.vertices = {
        {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0},  {2.0, 0.0, 0.0},  {0.0, 1.0, 0.0},
        {1.0, 1.0, 0.0}, {0.0, -1.0, 0.0}, {1.0, -1.0, 0.0},
    };
    mesh.triangles = {
        {{0, 1, 3}, 10},
        {{0, 1, 5}, geometer::kFastHlrUnspecifiedSourceFace},
        {{1, 2, 4}, 10},
        {{1, 2, 6}, geometer::kFastHlrUnspecifiedSourceFace},
    };
    const geometer::FastHlrPreparedMesh prepared = prepare(mesh);
    geometer::FastHlrOptions options;
    options.include_boundaries = false;
    options.include_creases = false;
    options.include_silhouettes = true;

    geometer::ProjectedModeGeometry visible;
    geometer::Status status;
    require(geometer::project_fast_hlr_detail(prepared, top_view(), options, &visible, nullptr,
                                              nullptr, &status) == 0,
            "ambiguous-provenance projection should succeed: " + status.message);
    require(visible.segments.size() == 2,
            "a known/unknown incident-face pair must disable cross-edge reconstruction");
}

void hidden_split_cannot_be_bridged_by_visible_reconstruction()
{
    geometer::FastHlrIndexedMesh mesh;
    mesh.vertices = {
        {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0},   {2.0, 0.0, 0.0},   {2.0, 1.0, 0.0},
        {0.0, 1.0, 0.0}, {0.75, -1.0, 1.0}, {1.25, -1.0, 1.0}, {1.0, 1.0, 1.0},
    };
    mesh.triangles = {{{0, 1, 4}, 10}, {{1, 3, 4}, 10}, {{1, 2, 3}, 10}, {{5, 6, 7}, 20}};
    const geometer::FastHlrPreparedMesh prepared = prepare(mesh);
    geometer::FastHlrOptions options;
    options.include_creases = false;
    options.include_silhouettes = false;
    options.include_hidden = true;

    geometer::ProjectedModeGeometry visible;
    geometer::ProjectedModeGeometry hidden;
    geometer::Status status;
    require(geometer::project_fast_hlr_detail(prepared, top_view(), options, &visible, &hidden,
                                              nullptr, &status) == 0,
            "hidden-gap reconstruction should succeed: " + status.message);
    std::size_t visible_bottom = 0;
    std::size_t hidden_bottom = 0;
    for (const geometer::ProjectedSegment& segment : visible.segments)
    {
        visible_bottom += near(segment.y1, 0.0) && near(segment.y2, 0.0) ? 1 : 0;
    }
    for (const geometer::ProjectedSegment& segment : hidden.segments)
    {
        hidden_bottom += near(segment.y1, 0.0) && near(segment.y2, 0.0) ? 1 : 0;
    }
    require(visible_bottom == 2,
            "visible fragments on either side of an occluder must remain disconnected");
    require(hidden_bottom == 1,
            "contiguous hidden fragments may join independently across their shared vertex");
}

geometer::FastHlrIndexedMesh adjacent_coplanar_rectangles(double second_depth = 0.0000005,
                                                          bool reverse_second = false)
{
    geometer::FastHlrIndexedMesh mesh;
    mesh.vertices = {
        {0.0, 0.0, 0.0},          {1.0, 0.0, 0.0},          {1.0, 1.0, 0.0},
        {0.0, 1.0, 0.0},          {1.0, 0.0, second_depth}, {2.0, 0.0, second_depth},
        {2.0, 1.0, second_depth}, {1.0, 1.0, second_depth},
    };
    mesh.triangles = {{{0, 1, 2}, 10}, {{0, 2, 3}, 10}};
    if (reverse_second)
    {
        mesh.triangles.push_back({{4, 6, 5}, 20});
        mesh.triangles.push_back({{4, 7, 6}, 20});
    }
    else
    {
        mesh.triangles.push_back({{4, 5, 6}, 20});
        mesh.triangles.push_back({{4, 6, 7}, 20});
    }
    return mesh;
}

geometer::FastHlrIndexedMesh coincident_coplanar_rectangles()
{
    geometer::FastHlrIndexedMesh mesh;
    mesh.vertices = {
        {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 1.0, 0.0},
        {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 1.0, 0.0},
    };
    mesh.triangles = {
        {{0, 1, 2}, 10},
        {{0, 2, 3}, 10},
        {{4, 5, 6}, 20},
        {{4, 6, 7}, 20},
    };
    return mesh;
}

geometer::FastHlrIndexedMesh adjacent_sloped_rectangles(double rise)
{
    geometer::FastHlrIndexedMesh mesh = adjacent_coplanar_rectangles();
    mesh.vertices[5].z = rise;
    mesh.vertices[6].z = rise;
    return mesh;
}

void coplanar_continuation_suppression_is_opt_in_and_conservative()
{
    const geometer::FastHlrPreparedMesh prepared = prepare(adjacent_coplanar_rectangles());
    geometer::ProjectedModeGeometry visible;
    geometer::FastHlrStatistics statistics;
    geometer::Status status;
    require(geometer::project_fast_hlr_detail(prepared, top_view(), {}, &visible, nullptr,
                                              &statistics, &status) == 0,
            "unsuppressed coplanar projection should succeed: " + status.message);
    require(visible.segments.size() == 7 && statistics.coplanar_seam_intervals == 0,
            "the optional seam filter must remain disabled by default: segments=" +
                std::to_string(visible.segments.size()) +
                " seams=" + std::to_string(statistics.coplanar_seam_intervals));

    geometer::FastHlrOptions options;
    options.suppress_coplanar_seams = true;
    require(geometer::project_fast_hlr_detail(prepared, top_view(), options, &visible, nullptr,
                                              &statistics, &status) == 0,
            "coplanar seam suppression should succeed: " + status.message);
    require(visible.segments.size() == 6 && statistics.coplanar_seam_intervals == 2,
            "two adjacent coplanar faces should remove both coincident seam candidates: segments=" +
                std::to_string(visible.segments.size()) +
                " seams=" + std::to_string(statistics.coplanar_seam_intervals));

    const geometer::FastHlrPreparedMesh stepped = prepare(adjacent_coplanar_rectangles(0.01));
    require(geometer::project_fast_hlr_detail(stepped, top_view(), options, &visible, nullptr,
                                              &statistics, &status) == 0,
            "stepped-face projection should succeed: " + status.message);
    require(statistics.coplanar_seam_intervals == 0,
            "faces outside the seam depth tolerance must retain their step edge");

    const geometer::FastHlrPreparedMesh opposed = prepare(adjacent_coplanar_rectangles(0.0, true));
    require(geometer::project_fast_hlr_detail(opposed, top_view(), options, &visible, nullptr,
                                              &statistics, &status) == 0,
            "opposed-normal projection should succeed: " + status.message);
    require(statistics.coplanar_seam_intervals == 0,
            "opposed normals must not establish a coplanar continuation");

    const geometer::FastHlrPreparedMesh coincident = prepare(coincident_coplanar_rectangles());
    require(geometer::project_fast_hlr_detail(coincident, top_view(), options, &visible, nullptr,
                                              &statistics, &status) == 0,
            "coincident-face projection should succeed: " + status.message);
    require(statistics.coplanar_seam_intervals == 0,
            "coincident faces that fill the same side must retain their external boundaries");

    const geometer::FastHlrPreparedMesh sloped = prepare(adjacent_sloped_rectangles(0.1));
    require(geometer::project_fast_hlr_detail(sloped, top_view(), options, &visible, nullptr,
                                              &statistics, &status) == 0,
            "sloped continuation projection should succeed: " + status.message);
    require(statistics.coplanar_seam_intervals == 0,
            "a continuation outside the seam angle tolerance must retain its ridge");
    options.coplanar_seam_angle_rad = 0.2;
    require(geometer::project_fast_hlr_detail(sloped, top_view(), options, &visible, nullptr,
                                              &statistics, &status) == 0,
            "relaxed seam-angle projection should succeed: " + status.message);
    require(statistics.coplanar_seam_intervals == 2,
            "the configured seam angle should admit a shallow continuation when requested");

    geometer::FastHlrIndexedMesh straddling;
    straddling.vertices = {{0.0, -1.0, 0.0},  {0.0, 1.0, 0.0}, {-1.0, 0.0, 0.0},
                           {-1.0, -2.0, 0.0}, {1.0, 0.0, 0.0}, {-1.0, 2.0, 0.0}};
    straddling.triangles = {{{0, 1, 2}, 10}, {{3, 4, 5}, 20}};
    const geometer::FastHlrPreparedMesh straddling_prepared = prepare(straddling);
    options.coplanar_seam_angle_rad = 0.2;
    require(geometer::project_fast_hlr_detail(straddling_prepared, top_view(), options, &visible,
                                              nullptr, &statistics, &status) == 0,
            "straddling-support projection should succeed: " + status.message);
    bool retained_candidate = false;
    for (const geometer::ProjectedSegment& segment : visible.segments)
    {
        retained_candidate =
            retained_candidate || (near(segment.x1, 0.0) && near(segment.x2, 0.0) &&
                                   near(std::min(segment.y1, segment.y2), -1.0) &&
                                   near(std::max(segment.y1, segment.y2), 1.0));
    }
    require(retained_candidate,
            "a support triangle that straddles both sides must not erase the candidate boundary");
}

void partial_coplanar_suppression_stays_separate_from_hidden_intervals()
{
    geometer::FastHlrIndexedMesh mesh;
    mesh.vertices = {
        {0.0, 0.0, 0.0},  {4.0, 0.0, 0.0},  {0.0, 2.0, 0.0},  {1.0, 0.0, 0.0},
        {2.0, 0.0, 0.0},  {3.0, 0.0, 0.0},  {3.0, -1.0, 0.0}, {1.0, -1.0, 0.0},
        {3.0, -1.0, 1.0}, {4.0, -1.0, 1.0}, {3.5, 1.0, 1.0},
    };
    mesh.triangles = {
        {{0, 1, 2}, 10}, {{3, 7, 4}, 20}, {{4, 7, 6}, 20}, {{4, 6, 5}, 20}, {{8, 9, 10}, 30},
    };
    const geometer::FastHlrPreparedMesh prepared = prepare(mesh);
    geometer::FastHlrOptions options;
    options.include_creases = false;
    options.include_silhouettes = false;
    options.include_hidden = true;
    options.suppress_coplanar_seams = true;

    geometer::ProjectedModeGeometry visible;
    geometer::ProjectedModeGeometry hidden;
    geometer::Status status;
    require(geometer::project_fast_hlr_detail(prepared, top_view(), options, &visible, &hidden,
                                              nullptr, &status) == 0,
            "partial seam projection should succeed: " + status.message);
    std::vector<std::array<double, 2>> visible_ranges;
    std::vector<std::array<double, 2>> hidden_ranges;
    for (const geometer::ProjectedSegment& segment : visible.segments)
    {
        if (near(segment.y1, 0.0) && near(segment.y2, 0.0))
        {
            visible_ranges.push_back(
                {std::min(segment.x1, segment.x2), std::max(segment.x1, segment.x2)});
        }
    }
    for (const geometer::ProjectedSegment& segment : hidden.segments)
    {
        if (near(segment.y1, 0.0) && near(segment.y2, 0.0))
        {
            hidden_ranges.push_back(
                {std::min(segment.x1, segment.x2), std::max(segment.x1, segment.x2)});
        }
    }
    std::sort(visible_ranges.begin(), visible_ranges.end());
    std::sort(hidden_ranges.begin(), hidden_ranges.end());
    require(visible_ranges.size() == 3 && near(visible_ranges[0][0], 0.0) &&
                near(visible_ranges[0][1], 1.0) && near(visible_ranges[1][0], 3.0) &&
                near(visible_ranges[1][1], 3.25) && near(visible_ranges[2][0], 3.75) &&
                near(visible_ranges[2][1], 4.0),
            "merged middle seam coverage must remove only its supported candidate interval");
    require(hidden_ranges.size() == 1 && near(hidden_ranges[0][0], 3.25) &&
                near(hidden_ranges[0][1], 3.75),
            "a true hidden interval outside the seam must remain independently reportable");
}

void occluder_splits_a_visible_edge()
{
    geometer::FastHlrIndexedMesh mesh;
    mesh.vertices = {
        {-2.0, 0.0, 0.0},  {2.0, 0.0, 0.0},  {0.0, -2.0, 0.0},
        {-0.5, -1.0, 1.0}, {0.5, -1.0, 1.0}, {0.0, 1.0, 1.0},
    };
    mesh.triangles = {{{0, 1, 2}, 1}, {{3, 4, 5}, 2}};
    const geometer::FastHlrPreparedMesh prepared = prepare(mesh);

    geometer::FastHlrOptions options;
    options.include_creases = false;
    options.include_silhouettes = false;
    options.include_hidden = true;
    geometer::ProjectedModeGeometry visible;
    geometer::ProjectedModeGeometry hidden;
    geometer::FastHlrStatistics statistics;
    geometer::Status status;
    const int code = geometer::project_fast_hlr_detail(prepared, top_view(), options, &visible,
                                                       &hidden, &statistics, &status);
    require(code == 0, "occlusion projection should succeed: " + status.message);

    std::size_t split_visible = 0;
    std::size_t split_hidden = 0;
    for (const geometer::ProjectedSegment& segment : visible.segments)
    {
        if (near(segment.y1, 0.0) && near(segment.y2, 0.0) &&
            (near(segment.x1, -2.0) || near(segment.x2, 2.0)))
        {
            ++split_visible;
        }
    }
    for (const geometer::ProjectedSegment& segment : hidden.segments)
    {
        if (near(segment.y1, 0.0) && near(segment.y2, 0.0))
        {
            ++split_hidden;
        }
    }
    require(split_visible == 2, "occluder should leave two visible pieces of the background edge");
    require(split_hidden == 1, "occluder should classify the middle interval as hidden");
    require(statistics.candidate_triangle_pairs > 0, "occlusion should exercise triangle queries");
}

void reversed_view_uses_the_same_near_direction_convention()
{
    geometer::FastHlrIndexedMesh mesh;
    mesh.vertices = {
        {-2.0, 0.0, 0.0},   {2.0, 0.0, 0.0},   {0.0, -2.0, 0.0},
        {-0.5, -1.0, -1.0}, {0.5, -1.0, -1.0}, {0.0, 1.0, -1.0},
    };
    mesh.triangles = {{{0, 1, 2}, 1}, {{3, 4, 5}, 2}};
    const geometer::FastHlrPreparedMesh prepared = prepare(mesh);

    geometer::FastHlrOptions options;
    options.include_creases = false;
    options.include_silhouettes = false;
    options.include_hidden = true;
    geometer::ProjectedModeGeometry visible;
    geometer::ProjectedModeGeometry hidden;
    geometer::Status status;
    geometer::ProjectionViewSpec bottom = {"bottom", {0.0, 0.0, -1.0}, {0.0, 1.0, 0.0}};
    const int code = geometer::project_fast_hlr_detail(prepared, bottom, options, &visible, &hidden,
                                                       nullptr, &status);
    require(code == 0, "reversed projection should succeed: " + status.message);
    std::size_t horizontal_hidden = 0;
    for (const geometer::ProjectedSegment& segment : hidden.segments)
    {
        horizontal_hidden += near(segment.y1, 0.0) && near(segment.y2, 0.0) ? 1 : 0;
    }
    require(horizontal_hidden == 1, "negative-Z view should treat lower Z as nearer");
}

void sloped_occluder_splits_at_the_depth_crossing()
{
    geometer::FastHlrIndexedMesh mesh;
    mesh.vertices = {
        {-2.0, 0.0, 0.0},   {2.0, 0.0, 0.0},  {0.0, -2.0, 0.0},
        {-1.0, -1.0, -1.0}, {1.0, -1.0, 1.0}, {0.0, 1.0, 0.0},
    };
    mesh.triangles = {{{0, 1, 2}, 1}, {{3, 4, 5}, 2}};
    const geometer::FastHlrPreparedMesh prepared = prepare(mesh);
    geometer::FastHlrOptions options;
    options.include_creases = false;
    options.include_silhouettes = false;
    options.include_hidden = true;
    options.depth_tolerance = 0.0;
    geometer::ProjectedModeGeometry visible;
    geometer::ProjectedModeGeometry hidden;
    geometer::Status status;
    require(geometer::project_fast_hlr_detail(prepared, top_view(), options, &visible, &hidden,
                                              nullptr, &status) == 0,
            "sloped occluder projection should succeed: " + status.message);
    bool found_crossing = false;
    for (const geometer::ProjectedSegment& segment : hidden.segments)
    {
        if (near(segment.y1, 0.0) && near(segment.y2, 0.0) &&
            near(std::min(segment.x1, segment.x2), 0.0) &&
            near(std::max(segment.x1, segment.x2), 0.5))
        {
            found_crossing = true;
        }
    }
    require(found_crossing, "hidden interval should begin where the sloped depth crosses z=0");
}

void disconnected_coincident_shapes_keep_separate_topology()
{
    const TopoDS_Shape first = BRepPrimAPI_MakeBox(1.0, 1.0, 1.0).Shape();
    const TopoDS_Shape second = BRepPrimAPI_MakeBox(1.0, 1.0, 1.0).Shape();
    TopoDS_Compound compound;
    BRep_Builder builder;
    builder.MakeCompound(compound);
    builder.Add(compound, first);
    builder.Add(compound, second);
    BRepMesh_IncrementalMesh mesher(compound, 0.05, false, 0.5, true);

    geometer::FastHlrPreparedMesh one;
    geometer::FastHlrPreparedMesh both;
    geometer::Status status;
    require(geometer::prepare_fast_hlr_shape(first, {}, &one, &status) == 0,
            "single box preparation should succeed: " + status.message);
    require(geometer::prepare_fast_hlr_shape(compound, {}, &both, &status) == 0,
            "coincident compound preparation should succeed: " + status.message);
    require(both.vertices.size() == one.vertices.size() * 2,
            "disconnected coincident boxes must not share topological vertices");
    require(both.edges.size() == one.edges.size() * 2,
            "disconnected coincident boxes must retain independent edges");
}

geometer::ProjectedModeGeometry fast_outline(const geometer::FastHlrIndexedMesh& mesh)
{
    const geometer::FastHlrPreparedMesh prepared = prepare(mesh);
    geometer::ProjectedModeGeometry outline;
    geometer::Status status;
    const int code = geometer::fast_mesh_shadow_outline_geometry(prepared, top_view(), {}, 1000,
                                                                 &outline, nullptr, &status);
    require(code == 0, "fast mesh-shadow should succeed: " + status.message);
    return outline;
}

void fast_mesh_shadow_preserves_a_planar_hole()
{
    geometer::FastHlrIndexedMesh ring;
    ring.vertices = {
        {0.0, 0.0, 0.0}, {4.0, 0.0, 0.0}, {4.0, 4.0, 0.0}, {0.0, 4.0, 0.0},
        {1.0, 1.0, 0.0}, {3.0, 1.0, 0.0}, {3.0, 3.0, 0.0}, {1.0, 3.0, 0.0},
    };
    ring.triangles = {
        {{0, 1, 5}, 1}, {{0, 5, 4}, 1}, {{1, 2, 6}, 1}, {{1, 6, 5}, 1},
        {{2, 3, 7}, 1}, {{2, 7, 6}, 1}, {{3, 0, 4}, 1}, {{3, 4, 7}, 1},
    };
    const geometer::ProjectedModeGeometry outline = fast_outline(ring);
    require(outline.segments.size() == 8,
            "planar ring shadow should contain four outer and four hole segments");
}

void fast_mesh_shadow_unions_overlapping_components()
{
    geometer::FastHlrIndexedMesh mesh;
    mesh.vertices = {
        {0.0, 0.0, 0.0}, {2.0, 0.0, 0.0}, {2.0, 2.0, 0.0}, {0.0, 2.0, 0.0},
        {1.0, 0.0, 1.0}, {3.0, 0.0, 1.0}, {3.0, 2.0, 1.0}, {1.0, 2.0, 1.0},
    };
    mesh.triangles = {
        {{0, 1, 2}, 1},
        {{0, 2, 3}, 1},
        {{4, 5, 6}, 2},
        {{4, 6, 7}, 2},
    };
    const geometer::ProjectedModeGeometry outline = fast_outline(mesh);
    require(outline.segments.size() == 4,
            "overlapping rectangular components should union to one rectangle");
    double minimum_x = std::numeric_limits<double>::infinity();
    double maximum_x = -std::numeric_limits<double>::infinity();
    for (const geometer::ProjectedSegment& segment : outline.segments)
    {
        minimum_x = std::min(minimum_x, std::min(segment.x1, segment.x2));
        maximum_x = std::max(maximum_x, std::max(segment.x1, segment.x2));
    }
    require(near(minimum_x, 0.0) && near(maximum_x, 3.0),
            "overlapping component union should retain the complete shadow extent");
}

void fast_mesh_shadow_falls_back_for_same_direction_shared_edges()
{
    geometer::FastHlrIndexedMesh mesh;
    mesh.vertices = {{0.0, 0.0, 0.0}, {2.0, 0.0, 0.0}, {0.0, 2.0, 0.0}};
    mesh.triangles = {{{0, 1, 2}, 1}, {{0, 1, 2}, 1}};
    const geometer::ProjectedModeGeometry outline = fast_outline(mesh);
    require(outline.segments.size() == 3,
            "coincident same-face triangles should fall back to a triangle union");
}

void fast_mesh_shadow_enforces_output_and_coordinate_limits()
{
    geometer::FastHlrIndexedMesh mesh;
    mesh.vertices = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
    mesh.triangles = {{{0, 1, 2}, 1}};
    const geometer::FastHlrPreparedMesh prepared = prepare(mesh);
    geometer::FastHlrOptions limited;
    limited.limits.max_output_segments = 2;
    geometer::ProjectedModeGeometry outline;
    geometer::Status status;
    require(geometer::fast_mesh_shadow_outline_geometry(prepared, top_view(), limited, 1000,
                                                        &outline, nullptr, &status) == 7,
            "fast mesh-shadow should enforce its output segment limit");

    limited = {};
    limited.limits.max_candidate_pairs = 5;
    require(geometer::fast_mesh_shadow_outline_geometry(prepared, top_view(), limited, 1000,
                                                        &outline, nullptr, &status) == 6,
            "fast mesh-shadow should bound Clipper candidate-pair work before union");

    mesh.vertices[1].x = 1.0e20;
    geometer::FastHlrOptions coarse_weld;
    coarse_weld.weld_tolerance = 100.0;
    const geometer::FastHlrPreparedMesh huge = prepare(mesh, coarse_weld);
    require(geometer::fast_mesh_shadow_outline_geometry(huge, top_view(), {}, 1000, &outline,
                                                        nullptr, &status) == 5,
            "fast mesh-shadow should reject coordinates outside the Clipper grid");

    geometer::FastHlrPreparedMesh malformed = prepared;
    malformed.triangles[0].vertices[2] = 99;
    require(geometer::fast_mesh_shadow_outline_geometry(malformed, top_view(), {}, 1000, &outline,
                                                        nullptr, &status) == 5,
            "fast mesh-shadow should reject malformed prepared triangle indices");
}

void fast_mesh_shadow_candidate_budget_ignores_disjoint_bounds()
{
    geometer::FastHlrIndexedMesh mesh;
    constexpr std::size_t triangle_count = 20;
    for (std::size_t index = 0; index < triangle_count; ++index)
    {
        const double x = static_cast<double>(index) * 3.0;
        const std::uint32_t first = static_cast<std::uint32_t>(mesh.vertices.size());
        mesh.vertices.push_back({x, 0.0, 0.0});
        mesh.vertices.push_back({x + 1.0, 0.0, 0.0});
        mesh.vertices.push_back({x, 1.0, 0.0});
        mesh.triangles.push_back(
            {{first, first + 1, first + 2}, static_cast<std::uint32_t>(index + 1)});
    }

    const geometer::FastHlrPreparedMesh prepared = prepare(mesh);
    geometer::FastHlrOptions limited;
    limited.limits.max_candidate_pairs = triangle_count * 6;
    geometer::ProjectedModeGeometry outline;
    geometer::Status status;
    require(geometer::fast_mesh_shadow_outline_geometry(prepared, top_view(), limited, 1000,
                                                        &outline, nullptr, &status) == 0,
            "disjoint shadows should charge only overlapping segment bounds: " + status.message);
    require(outline.segments.size() == triangle_count * 3,
            "disjoint triangle shadows should retain every outline segment");
}

} // namespace

int main()
{
    try
    {
        square_builds_shared_adjacency();
        indexed_mesh_welds_duplicate_seams_by_distance();
        one_shot_matches_reusable_preparation();
        invalid_indices_and_limits_are_rejected();
        malformed_prepared_data_and_invalid_options_are_rejected();
        smooth_internal_edge_can_be_a_silhouette();
        grazing_face_edge_is_a_silhouette();
        oblique_view_uses_the_expected_orthonormal_basis();
        collinear_boundary_fragments_are_joined_without_crossing_corners();
        collinear_reconstruction_handles_long_reversed_chains_and_branches();
        projected_contact_without_shared_topology_is_not_joined();
        ambiguous_source_face_provenance_disables_cross_edge_joining();
        hidden_split_cannot_be_bridged_by_visible_reconstruction();
        coplanar_continuation_suppression_is_opt_in_and_conservative();
        partial_coplanar_suppression_stays_separate_from_hidden_intervals();
        occluder_splits_a_visible_edge();
        reversed_view_uses_the_same_near_direction_convention();
        sloped_occluder_splits_at_the_depth_crossing();
        disconnected_coincident_shapes_keep_separate_topology();
        fast_mesh_shadow_preserves_a_planar_hole();
        fast_mesh_shadow_unions_overlapping_components();
        fast_mesh_shadow_falls_back_for_same_direction_shared_edges();
        fast_mesh_shadow_enforces_output_and_coordinate_limits();
        fast_mesh_shadow_candidate_budget_ignores_disjoint_bounds();
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << "\n";
        return 1;
    }
    return 0;
}

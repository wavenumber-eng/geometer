#include "fast_hlr_occt.h"
#include "fast_mesh_shadow_outline.h"
#include "geometer/fast_hlr.h"

#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>

#include <cmath>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

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

    mesh.vertices[1].x = 1.0e20;
    const geometer::FastHlrPreparedMesh huge = prepare(mesh);
    require(geometer::fast_mesh_shadow_outline_geometry(huge, top_view(), {}, 1000, &outline,
                                                        nullptr, &status) == 5,
            "fast mesh-shadow should reject coordinates outside the Clipper grid");

    geometer::FastHlrPreparedMesh malformed = prepared;
    malformed.triangles[0].vertices[2] = 99;
    require(geometer::fast_mesh_shadow_outline_geometry(malformed, top_view(), {}, 1000, &outline,
                                                        nullptr, &status) == 5,
            "fast mesh-shadow should reject malformed prepared triangle indices");
}

} // namespace

int main()
{
    try
    {
        square_builds_shared_adjacency();
        invalid_indices_and_limits_are_rejected();
        malformed_prepared_data_and_invalid_options_are_rejected();
        smooth_internal_edge_can_be_a_silhouette();
        grazing_face_edge_is_a_silhouette();
        oblique_view_uses_the_expected_orthonormal_basis();
        occluder_splits_a_visible_edge();
        reversed_view_uses_the_same_near_direction_convention();
        sloped_occluder_splits_at_the_depth_crossing();
        disconnected_coincident_shapes_keep_separate_topology();
        fast_mesh_shadow_preserves_a_planar_hole();
        fast_mesh_shadow_unions_overlapping_components();
        fast_mesh_shadow_falls_back_for_same_direction_shared_edges();
        fast_mesh_shadow_enforces_output_and_coordinate_limits();
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << "\n";
        return 1;
    }
    return 0;
}

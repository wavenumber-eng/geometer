#include "illustration_clipping.h"

#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace
{
using namespace geometer;
using namespace geometer::contracts;

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

MeshIllustrationMesh crossing_triangle()
{
    MeshIllustrationMesh mesh;
    mesh.id = "crossing";
    mesh.positions = {-1, 0, 0, 1, 0, 0, 1, 1, 0};
    mesh.normals = std::vector<double>{0, 0, 1, 0, 0, 1, 0, 0, 1};
    mesh.indices = std::vector<std::uint32_t>{0, 1, 2};
    MeshIllustrationMaterial first;
    first.color = {1, 0, 0};
    MeshIllustrationMaterial second;
    second.color = {0, 1, 0};
    mesh.materials = {first, second};
    mesh.triangle_material_indices = std::vector<std::uint32_t>{1};
    return mesh;
}

IllustrationClipping clipping(double x_distance = 0)
{
    IllustrationClipping result;
    HalfSpacePlane plane;
    plane.normal = {1, 0, 0};
    plane.distance_mm = x_distance;
    plane.tolerance_mm = 0;
    result.planes.push_back(std::move(plane));
    return result;
}

void split_preserves_attributes_and_is_deterministic()
{
    PreparedIllustrationFragment first;
    Status status;
    require(prepare_illustration_fragment({crossing_triangle()}, clipping(), {}, "raw", &first,
                                          &status) == 0,
            "crossing triangle should clip successfully");
    require(!first.empty && first.collection.meshes.size() == 1,
            "retained fragment should be nonempty");
    require(first.metadata.input_triangles == 1 && first.metadata.output_triangles == 2,
            "one crossing triangle should split into two triangles");
    const auto& mesh = first.collection.meshes.front();
    require(mesh.indices && mesh.indices->size() == 6 && mesh.positions.size() == 18,
            "split output should be compact indexed triangles");
    require(mesh.normals && mesh.normals->size() == mesh.positions.size(),
            "split vertices should retain interpolated normals");
    require(mesh.triangle_material_indices &&
                *mesh.triangle_material_indices == std::vector<std::uint32_t>({1, 1}),
            "split triangles should retain their source material identity");
    for (std::size_t index = 0; index < mesh.positions.size(); index += 3)
    {
        require(mesh.positions[index] >= -1e-12, "all output vertices must satisfy the plane");
        const double length = std::sqrt((*mesh.normals)[index] * (*mesh.normals)[index] +
                                        (*mesh.normals)[index + 1] * (*mesh.normals)[index + 1] +
                                        (*mesh.normals)[index + 2] * (*mesh.normals)[index + 2]);
        require(std::abs(length - 1) < 1e-12, "interpolated normals should be normalized");
    }
    require(first.bounds_mm && (*first.bounds_mm)[0] == 0 && (*first.bounds_mm)[3] == 1,
            "bounds must be computed after clipping");
    require(first.metadata.clipping && first.metadata.raw_attachment_sha256 == "raw",
            "normalized clipping and raw attachment identity should be reported");

    PreparedIllustrationFragment second;
    require(prepare_illustration_fragment({crossing_triangle()}, clipping(), {}, "raw", &second,
                                          &status) == 0,
            "repeated clipping should succeed");
    require(first.metadata.fragment_sha256 == second.metadata.fragment_sha256 &&
                first.metadata.linework_geometry_sha256 == second.metadata.linework_geometry_sha256,
            "identical clipping must produce deterministic identities");
}

void transforms_precede_planes_and_empty_is_success()
{
    auto mesh = crossing_triangle();
    mesh.matrix = IllustrationMatrix4x4{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 2, 0, 0, 1};
    PreparedIllustrationFragment translated;
    Status status;
    require(prepare_illustration_fragment({mesh}, clipping(2), {}, {}, &translated, &status) == 0,
            "mesh transform should be applied before clipping");
    require(!translated.empty && translated.bounds_mm && (*translated.bounds_mm)[0] == 2,
            "the transformed fragment should be clipped in world space");

    auto planes = clipping(10);
    PreparedIllustrationFragment empty;
    require(prepare_illustration_fragment({crossing_triangle()}, planes, {}, {}, &empty, &status) ==
                0,
            "fully clipped geometry should be a successful result");
    require(empty.empty && !empty.bounds_mm && empty.collection.meshes.empty() &&
                empty.metadata.output_triangles == 0,
            "empty output must be unmistakable and must not fabricate bounds");
}

void affine_baking_is_column_major_and_preserves_normal_geometry()
{
    auto mesh = crossing_triangle();
    mesh.matrix = IllustrationMatrix4x4{2, 0, 0.2, 0, 0.3, 1, 0, 0, 0, 0.1, 0.7, 0, 3, -2, 4, 1};
    PreparedIllustrationFragment result;
    Status status;
    require(prepare_illustration_fragment({mesh}, {}, {}, {}, &result, &status) == 0,
            "nontrivial affine baking should succeed");
    const auto& baked = result.collection.meshes.front();
    const std::vector<double> expected = {1, -2, 3.8, 5, -2, 4.2, 5.3, -1, 4.2};
    require(baked.positions.size() == expected.size(), "affine baking should retain one triangle");
    for (std::size_t index = 0; index < expected.size(); ++index)
        require(std::abs(baked.positions[index] - expected[index]) < 1e-12,
                "affine baking must use the illustration column-major convention");
    require(!baked.matrix, "the baked fragment must not retain a second transform");
    require(baked.normals && baked.normals->size() == baked.positions.size(),
            "affine baking should preserve transformed normals");
    const std::array<double, 3> edge_x = {4, 0, 0.4};
    const std::array<double, 3> edge_y = {4.3, 1, 0.4};
    const std::array<double, 3> normal = {(*baked.normals)[0], (*baked.normals)[1],
                                          (*baked.normals)[2]};
    const auto dot = [](const std::array<double, 3>& first, const std::array<double, 3>& second)
    { return first[0] * second[0] + first[1] * second[1] + first[2] * second[2]; };
    const double x_error = std::abs(dot(normal, edge_x));
    const double y_error = std::abs(dot(normal, edge_y));
    if (x_error >= 1e-10 || y_error >= 1e-10)
        throw std::runtime_error("normals must use the inverse transpose of the baked affine "
                                 "transform (errors " +
                                 std::to_string(x_error) + ", " + std::to_string(y_error) + ")");
}

void multiple_planes_and_limits_are_governed()
{
    auto planes = clipping();
    HalfSpacePlane y_plane;
    y_plane.normal = {0, 1, 0};
    y_plane.distance_mm = 0.75;
    planes.planes.push_back(y_plane);
    PreparedIllustrationFragment result;
    Status status;
    require(
        prepare_illustration_fragment({crossing_triangle()}, planes, {}, {}, &result, &status) == 0,
        "ordered multiple planes should clip successfully");
    for (std::size_t index = 0; index < result.collection.meshes.front().positions.size();
         index += 3)
        require(result.collection.meshes.front().positions[index + 1] >= 0.75 - 1e-12,
                "every retained point must satisfy every plane");

    auto bounded = clipping();
    bounded.limits.emplace();
    bounded.limits->max_output_triangles = 1;
    require(prepare_illustration_fragment({crossing_triangle()}, bounded, {}, {}, &result,
                                          &status) == 102,
            "output growth beyond the requested limit should fail closed");

    bounded = clipping();
    bounded.limits.emplace();
    bounded.limits->max_generated_vertices = 6;
    require(prepare_illustration_fragment({crossing_triangle()}, bounded, {}, {}, &result,
                                          &status) == 102,
            "intermediate polygons and final fan vertices must both consume the vertex budget");
}

void extreme_finite_plane_normalizes_without_overflow()
{
    auto extreme = clipping();
    const double maximum = std::numeric_limits<double>::max();
    extreme.planes[0].normal = {maximum, maximum, maximum};
    extreme.planes[0].distance_mm = maximum;
    PreparedIllustrationFragment result;
    Status status;
    require(prepare_illustration_fragment({crossing_triangle()}, extreme, {}, {}, &result,
                                          &status) == 0,
            "a finite extreme plane must normalize without overflowing its magnitude");
    require(result.metadata.clipping && result.metadata.clipping->planes.size() == 1,
            "normalized clipping metadata should be present");
    const auto& plane = result.metadata.clipping->planes.front();
    const double expected = 1 / std::sqrt(3.0);
    require(std::abs(plane.normal[0] - expected) < 1e-12 &&
                std::abs(plane.distance_mm - expected) < 1e-12,
            "extreme finite normal and distance must normalize in bounded stages");
}

void malformed_hidden_material_and_cancelling_normals_fail_safely()
{
    auto malformed = crossing_triangle();
    (*malformed.triangle_material_indices)[0] = 2;
    PreparedIllustrationFragment result;
    Status status;
    require(prepare_illustration_fragment({malformed}, clipping(10), {}, {}, &result, &status) ==
                22,
            "fully clipped triangles must not hide malformed material indices");

    auto cancelling = crossing_triangle();
    cancelling.normals = std::vector<double>{1, 0, 0, -1, 1e-15, 0, -1, 1e-15, 0};
    require(prepare_illustration_fragment({cancelling}, clipping(), {}, {}, &result, &status) == 0,
            "near-cancelling boundary normals should clip successfully");
    bool found_zero = false;
    for (std::size_t index = 0; index < result.collection.meshes.front().normals->size();
         index += 3)
    {
        const auto& normals = *result.collection.meshes.front().normals;
        found_zero = found_zero ||
                     (normals[index] == 0 && normals[index + 1] == 0 && normals[index + 2] == 0);
    }
    require(found_zero,
            "normal interpolation that cancels within geometry epsilon must become canonical zero");
}
} // namespace

int main()
{
    try
    {
        split_preserves_attributes_and_is_deterministic();
        transforms_precede_planes_and_empty_is_success();
        affine_baking_is_column_major_and_preserves_normal_geometry();
        multiple_planes_and_limits_are_governed();
        extreme_finite_plane_normalizes_without_overflow();
        malformed_hidden_material_and_cancelling_normals_fail_safely();
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

#include "analytic_illustration_lowering.h"
#include "model_illustration_transform.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace
{
using namespace geometer;
using namespace geometer::contracts;

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

MeshIllustrationMaterial material()
{
    MeshIllustrationMaterial value;
    value.color = {0.4, 0.6, 0.8};
    return value;
}

IllustrationProfileRingA0 rectangle(double x0, double y0, double x1, double y1)
{
    IllustrationProfileRingA0 ring;
    ring.points_mm = {{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}};
    ring.segments.resize(4, IllustrationProfileLineA0{});
    return ring;
}

AnalyticIllustrationSourceA0 source_with(AnalyticPrimitiveA0 primitive)
{
    AnalyticIllustrationSourceA0 source;
    AnalyticDefinitionA0 definition;
    definition.id = "definition";
    definition.primitives.push_back(std::move(primitive));
    source.scene.definitions.push_back(std::move(definition));
    source.scene.occurrences.push_back({"occurrence", "definition", std::nullopt});
    return source;
}

void sphere_has_outward_winding_and_exact_limit()
{
    AnalyticSphereA0 sphere;
    sphere.id = "sphere";
    sphere.center_mm = {2, 3, 4};
    sphere.radius_mm = 1;
    sphere.material = material();
    auto source = source_with(sphere);
    source.lowering.emplace();
    source.lowering->linear_deflection_mm = 1;
    source.lowering->angular_deflection_rad = 1;
    source.lowering->limits.emplace();
    source.lowering->limits->max_definition_triangles = 120;
    MeshCollectionA0 collection;
    model_illustration_detail::AnalyticLoweringStats stats;
    Status status;
    require(model_illustration_detail::lower_analytic_scene(source, std::nullopt, &collection,
                                                            &stats, &status) == 0,
            "sphere at its exact triangle limit should lower");
    require(stats.triangles == 120 && collection.meshes.size() == 1,
            "sphere preflight triangle count should be exact");
    const auto& mesh = collection.meshes.front();
    for (std::size_t index = 0; index < mesh.indices->size(); index += 3)
    {
        const auto point = [&](std::uint32_t vertex, unsigned axis)
        { return mesh.positions[3 * vertex + axis]; };
        const auto a = (*mesh.indices)[index], b = (*mesh.indices)[index + 1],
                   c = (*mesh.indices)[index + 2];
        const double abx = point(b, 0) - point(a, 0), aby = point(b, 1) - point(a, 1),
                     abz = point(b, 2) - point(a, 2);
        const double acx = point(c, 0) - point(a, 0), acy = point(c, 1) - point(a, 1),
                     acz = point(c, 2) - point(a, 2);
        const double nx = aby * acz - abz * acy, ny = abz * acx - abx * acz,
                     nz = abx * acy - aby * acx;
        const double cx = (point(a, 0) + point(b, 0) + point(c, 0)) / 3 - 2;
        const double cy = (point(a, 1) + point(b, 1) + point(c, 1)) / 3 - 3;
        const double cz = (point(a, 2) + point(b, 2) + point(c, 2)) / 3 - 4;
        require(nx * cx + ny * cy + nz * cz > 0, "sphere triangle must face outward");
    }
    source.lowering->limits->max_definition_triangles = 119;
    require(model_illustration_detail::lower_analytic_scene(source, std::nullopt, &collection,
                                                            &stats, &status) == 102,
            "sphere one triangle over its limit should fail before allocation");
}

void invalid_profile_topology_is_rejected()
{
    AnalyticExtrusionA0 extrusion;
    extrusion.id = "body";
    extrusion.z_min_mm = 0;
    extrusion.z_max_mm = 1;
    extrusion.material = material();
    extrusion.regions = {{rectangle(0, 0, 4, 4), std::nullopt},
                         {rectangle(2, 2, 6, 6), std::nullopt}};
    MeshCollectionA0 collection;
    model_illustration_detail::AnalyticLoweringStats stats;
    Status status;
    require(model_illustration_detail::lower_analytic_scene(source_with(extrusion), std::nullopt,
                                                            &collection, &stats, &status) == 1,
            "overlapping regions must be rejected");

    extrusion.regions.resize(1);
    extrusion.regions[0].holes = std::vector<IllustrationProfileRingA0>{rectangle(0, 1, 2, 3)};
    require(model_illustration_detail::lower_analytic_scene(source_with(extrusion), std::nullopt,
                                                            &collection, &stats, &status) == 1,
            "a hole touching its outer ring must be rejected");
}

void affine_conditioning_is_scale_invariant()
{
    for (const double scale : {1e-200, 1e200})
    {
        std::vector<double> authored = {scale, 0, 0, 0, 0, scale, 0, 0, 0, 0, scale, 0, 0, 0, 0, 1};
        model_illustration_detail::Matrix4 matrix;
        std::string error;
        require(model_illustration_detail::validate_affine_matrix(authored, &matrix, &error),
                "uniform extreme scale should validate");
        MeshIllustrationMesh mesh;
        mesh.positions = {1, 0, 0, 0, 1, 0, 0, 0, 1};
        mesh.normals = std::vector<double>{0, 0, 1, 0, 0, 1, 0, 0, 1};
        mesh.indices = std::vector<std::uint32_t>{0, 1, 2};
        require(model_illustration_detail::apply_affine_matrix(&mesh, matrix, &error),
                "uniform extreme scale should transform finite geometry and normals");
    }
    std::vector<double> singular = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1e-13, 0, 0, 0, 0, 1};
    model_illustration_detail::Matrix4 matrix;
    std::string error;
    require(!model_illustration_detail::validate_affine_matrix(singular, &matrix, &error),
            "singular-value ratio above the contract limit should fail");
    std::vector<double> correlated = {1, 0, 0, 0, 1, 1e-13, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    require(!model_illustration_detail::validate_affine_matrix(correlated, &matrix, &error),
            "correlated near-singular columns should fail without forming normal equations");
}
} // namespace

int main()
{
    try
    {
        sphere_has_outward_winding_and_exact_limit();
        invalid_profile_topology_is_rejected();
        affine_conditioning_is_scale_invariant();
        std::cout << "model illustration native tests passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

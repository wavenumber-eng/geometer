#include "geometer/mesh_illustration.h"
#include "mesh_illustration_internal.h"

#include <iostream>
#include <stdexcept>

// Link-time tripwire: geometry-only calls must work without the SVG writer.
namespace geometer::illustration_detail
{
std::string render_svg(const Scene&, const Style&, const Commands&,
                       const contracts::MeshIllustrationSvgOptions&, const char*)
{
    throw std::runtime_error("SVG writer tripwire");
}
} // namespace geometer::illustration_detail

int main()
{
    using namespace geometer;
    try
    {
        contracts::MeshIllustrationGeometryInputA0 input;
        contracts::MeshIllustrationMesh mesh;
        mesh.id = "square";
        mesh.positions = {0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0};
        mesh.indices = std::vector<std::uint32_t>{0, 1, 2, 0, 2, 3};
        mesh.materials.push_back({{.2, .7, .62}, .6, {}});
        input.meshes.push_back(mesh);
        input.view.direction = {0, 0, 1};
        input.view.up = {0, 1, 0};
        contracts::MeshIllustrationGeometryA0 result;
        Status status;
        if (illustrate_mesh_geometry(input, &result, &status) || result.surfaces.empty() ||
            result.surfaces.front().layers.front().opacity != .6 || result.stats.triangles != 2)
            throw std::runtime_error("geometry-only square failed: " + status.message);
        std::string json;
        contracts::ContractError error;
        if (!contracts::encode_json(result, &json, &error) ||
            json.find("\"svg\"") != std::string::npos)
            throw std::runtime_error("invalid geometry result: " + error.message);
        contracts::MeshIllustrationInputA0 svg_input;
        svg_input.meshes = input.meshes;
        svg_input.view = input.view;
        contracts::MeshIllustrationResultA0 svg;
        if (!illustrate_mesh(svg_input, &svg, &status) || status.message != "SVG writer tripwire")
            throw std::runtime_error("SVG tripwire did not intercept writer");
        input.prepare.emplace();
        input.prepare->max_triangles = 1;
        if (illustrate_mesh_geometry(input, &result, &status) != 102 || !result.surfaces.empty())
            throw std::runtime_error("geometry resource failure leaked partial data");
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

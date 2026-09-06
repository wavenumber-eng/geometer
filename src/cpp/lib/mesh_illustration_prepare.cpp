#include "mesh_illustration_internal.h"

#include <algorithm>
#include <unordered_map>

namespace geometer::illustration_detail
{
namespace
{
struct Preparation
{
    Scene scene;
    Vec3 right, up;
    bool mirror;
    double tolerance;
    std::size_t suppressed = 0;
    std::unordered_map<std::string, std::size_t> edge_indices;

    void warn(std::string value)
    {
        if (scene.warnings.size() < 255)
            scene.warnings.push_back(std::move(value));
        else
            ++suppressed;
    }

    std::string point_key(Vec3 point) const
    {
        const double scale = 1 / tolerance;
        return integer_text(point[0] * scale) + "," + integer_text(point[1] * scale) + "," +
               integer_text(point[2] * scale);
    }

    void edge(const Triangle& triangle, const std::array<Vec3, 3>& world, unsigned start,
              unsigned end)
    {
        const auto a = point_key(world[start]), b = point_key(world[end]);
        const auto key = a < b ? a + "|" + b : b + "|" + a;
        const auto found = edge_indices.find(key);
        if (found == edge_indices.end())
        {
            edge_indices.emplace(key, scene.edges.size());
            scene.edges.push_back({{triangle.points[start], triangle.points[end]},
                                   (triangle.depths[start] + triangle.depths[end]) / 2,
                                   triangle.front,
                                   {},
                                   triangle.geometric_normal,
                                   {}});
        }
        else
        {
            auto& existing = scene.edges[found->second];
            if (!existing.normal_b)
            {
                existing.front_b = triangle.front;
                existing.normal_b = triangle.geometric_normal;
            }
        }
    }

    void triangle(const contracts::MeshIllustrationMesh& mesh, const Matrix& matrix,
                  std::size_t index)
    {
        std::array<std::size_t, 3> indices{};
        for (unsigned i = 0; i < 3; ++i)
        {
            indices[i] = mesh.indices ? (*mesh.indices)[index * 3 + i] : index * 3 + i;
            if (indices[i] >= mesh.positions.size() / 3)
            {
                warn("Skipped out-of-range triangle " + std::to_string(index) + " in " + mesh.id +
                     ".");
                return;
            }
        }
        std::array<Vec3, 3> world;
        for (unsigned i = 0; i < 3; ++i)
            world[i] = transform_point(matrix, position(mesh.positions, indices[i]));
        auto normal = cross(subtract(world[1], world[0]), subtract(world[2], world[0]));
        if (length(normal) < 1e-12)
        {
            warn("Skipped degenerate triangle " + std::to_string(index) + " in " + mesh.id + ".");
            return;
        }
        for (auto& value : normal)
            value *= determinant_sign(matrix);
        normal = normalize(normal, "Triangle normal");
        Triangle t;
        t.normal = t.geometric_normal = normal;
        if (mesh.normals && !mesh.normals->empty())
        {
            Vec3 average{};
            for (auto vertex : indices)
                average = add(average, position(*mesh.normals, vertex));
            if (length(average) >= 1e-12)
                t.normal = transform_normal(matrix, normalize(average, "Source normal"));
        }
        t.front = dot(normal, scene.direction) > 1e-9;
        t.double_sided = mesh.double_sided.value_or(false);
        for (unsigned i = 0; i < 3; ++i)
        {
            t.points[i] = {(mirror ? -1 : 1) * dot(world[i], right), dot(world[i], up)};
            t.depths[i] = dot(world[i], scene.direction);
            for (double value : {t.points[i][0], t.points[i][1], t.depths[i]})
                if (!std::isfinite(value))
                    throw std::runtime_error("Illustration projection is not finite.");
            scene.bounds.include(t.points[i]);
        }
        t.depth = (t.depths[0] + t.depths[1] + t.depths[2]) / 3;
        if (!std::isfinite(t.depth))
            throw std::runtime_error("Illustration triangle depth overflow.");
        std::size_t material = 0;
        if (mesh.triangle_material_indices && index < mesh.triangle_material_indices->size())
            material = (*mesh.triangle_material_indices)[index];
        if (material >= mesh.materials.size())
            material = 0;
        const auto& source = mesh.materials.at(material);
        t.color = vector3(source.color);
        for (auto& channel : t.color)
            channel = clamp(channel);
        t.opacity = clamp(source.opacity.value_or(1));
        scene.triangles.push_back(t);
        edge(t, world, 0, 1);
        edge(t, world, 1, 2);
        edge(t, world, 2, 0);
    }
};
} // namespace

Scene prepare_scene(const contracts::MeshIllustrationInputA0& input)
{
    Preparation p;
    p.scene.direction = normalize(vector3(input.view.direction), "View direction");
    const auto requested_up = normalize(vector3(input.view.up), "View up");
    p.right = normalize(cross(requested_up, p.scene.direction), "View right");
    p.up = normalize(cross(p.scene.direction, p.right), "Orthogonal view up");
    p.mirror = input.view.mirror_x.value_or(false);
    const auto options = input.prepare.value_or(contracts::MeshIllustrationPrepareOptions{});
    p.tolerance = std::max(1e-9, options.weld_tolerance.value_or(1e-7));
    const auto maximum = options.max_triangles.value_or(750000);
    std::size_t count = 0;
    for (const auto& mesh : input.meshes)
    {
        if (mesh.positions.size() % 3 != 0)
            throw std::runtime_error("Mesh " + mesh.id +
                                     " position length must be divisible by 3.");
        const auto elements = mesh.indices ? mesh.indices->size() : mesh.positions.size() / 3;
        if (elements % 3 != 0)
            throw std::runtime_error("Mesh " + mesh.id +
                                     " index/vertex element count must be divisible by 3.");
        if (elements / 3 > maximum - count)
            throw ResourceLimit("Mesh illustration exceeds the configured triangle limit.");
        count += elements / 3;
        auto matrix = identity;
        if (mesh.matrix)
        {
            if (mesh.matrix->size() != matrix.size())
                throw std::runtime_error("Illustration matrix must contain 16 values.");
            std::copy(mesh.matrix->begin(), mesh.matrix->end(), matrix.begin());
        }
        for (std::size_t index = 0; index < elements / 3; ++index)
            p.triangle(mesh, matrix, index);
    }
    if (!std::isfinite(p.scene.bounds.min_x))
        p.scene.bounds = {-1, -1, 1, 1};
    if (!std::isfinite(p.scene.bounds.max_x - p.scene.bounds.min_x) ||
        !std::isfinite(p.scene.bounds.max_y - p.scene.bounds.min_y))
        throw std::runtime_error("Illustration bounds overflow.");
    for (const auto& edge : p.scene.edges)
        if (!std::isfinite(edge.depth))
            throw std::runtime_error("Illustration edge depth overflow.");
    if (p.suppressed)
        p.scene.warnings.push_back(std::to_string(p.suppressed) +
                                   " additional warnings suppressed.");
    return std::move(p.scene);
}
} // namespace geometer::illustration_detail

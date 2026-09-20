#include "illustration_clipping.h"

#include "geometer/sha256.h"
#include "model_illustration_transform.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <string>
#include <utility>

namespace geometer
{
namespace
{
constexpr double kCanonicalScale = 1.0e12;
constexpr std::uint32_t kDefaultMaxOutputTriangles = 2000000;
constexpr std::uint32_t kDefaultMaxGeneratedVertices = 6000000;
constexpr std::uint32_t kDefaultMaxIntersections = 8000000;
constexpr std::uint32_t kDefaultMaxEdgePlaneTests = 32000000;

struct Vertex
{
    std::array<double, 3> position{};
    std::array<double, 3> normal{};
};

struct WorkBudget
{
    std::uint64_t output_triangles = 0;
    std::uint64_t generated_vertices = 0;
    std::uint64_t intersections = 0;
    std::uint64_t edge_plane_tests = 0;
};

int fail(Status* status, int code, std::string message)
{
    if (status)
        *status = {code, std::move(message)};
    return code;
}

bool canonicalize(double value, double* output)
{
    if (!std::isfinite(value) ||
        std::abs(value) > std::numeric_limits<double>::max() / kCanonicalScale)
        return false;
    const double rounded =
        std::copysign(std::floor(std::abs(value) * kCanonicalScale + 0.5), value) / kCanonicalScale;
    *output = rounded == 0 ? 0 : rounded;
    return std::isfinite(*output);
}

bool canonicalize_triplet(std::array<double, 3>* value)
{
    for (double& component : *value)
        if (!canonicalize(component, &component))
            return false;
    return true;
}

bool normalize_vector(std::array<double, 3>* value)
{
    const double maximum =
        std::max({std::abs((*value)[0]), std::abs((*value)[1]), std::abs((*value)[2])});
    if (!(maximum > 0) || !std::isfinite(maximum))
        return false;
    for (double& component : *value)
        component /= maximum;
    const double length = std::hypot((*value)[0], std::hypot((*value)[1], (*value)[2]));
    if (!(length > 0) || !std::isfinite(length))
        return false;
    for (double& component : *value)
        component /= length;
    return canonicalize_triplet(value);
}

bool normalize_clipping(const std::optional<contracts::IllustrationClipping>& source,
                        std::optional<contracts::NormalizedClipping>* result, std::string* error)
{
    result->reset();
    if (!source)
        return true;
    if (source->planes.empty() || source->planes.size() > 16 || source->cap_policy != "none")
    {
        *error = "Clipping requires one to sixteen planes and cap_policy none.";
        return false;
    }
    contracts::NormalizedClipping normalized;
    normalized.cap_policy = "none";
    normalized.max_output_triangles = source->limits && source->limits->max_output_triangles
                                          ? *source->limits->max_output_triangles
                                          : kDefaultMaxOutputTriangles;
    normalized.max_generated_vertices = source->limits && source->limits->max_generated_vertices
                                            ? *source->limits->max_generated_vertices
                                            : kDefaultMaxGeneratedVertices;
    normalized.max_intersections = source->limits && source->limits->max_intersections
                                       ? *source->limits->max_intersections
                                       : kDefaultMaxIntersections;
    normalized.max_edge_plane_tests = source->limits && source->limits->max_edge_plane_tests
                                          ? *source->limits->max_edge_plane_tests
                                          : kDefaultMaxEdgePlaneTests;
    if (normalized.max_output_triangles == 0 ||
        normalized.max_output_triangles > kDefaultMaxOutputTriangles ||
        normalized.max_generated_vertices == 0 ||
        normalized.max_generated_vertices > kDefaultMaxGeneratedVertices ||
        normalized.max_intersections == 0 ||
        normalized.max_intersections > kDefaultMaxIntersections ||
        normalized.max_edge_plane_tests == 0 ||
        normalized.max_edge_plane_tests > kDefaultMaxEdgePlaneTests)
    {
        *error = "Clipping work limits must be positive and cannot exceed their hard ceilings.";
        return false;
    }
    normalized.planes.reserve(source->planes.size());
    for (const auto& plane : source->planes)
    {
        if (plane.normal.size() != 3 || !std::isfinite(plane.distance_mm) ||
            (plane.tolerance_mm &&
             (!std::isfinite(*plane.tolerance_mm) || *plane.tolerance_mm < 0)))
        {
            *error = "Clipping planes require a finite nonzero normal, finite distance, and "
                     "nonnegative finite tolerance.";
            return false;
        }
        std::array<double, 3> normal{plane.normal[0], plane.normal[1], plane.normal[2]};
        const double maximum =
            std::max({std::abs(normal[0]), std::abs(normal[1]), std::abs(normal[2])});
        if (!(maximum > 0) || !std::isfinite(maximum))
        {
            *error = "Clipping plane normal must be finite and nonzero.";
            return false;
        }
        for (double& component : normal)
            component /= maximum;
        const double scaled_length = std::hypot(normal[0], std::hypot(normal[1], normal[2]));
        for (double& component : normal)
            component /= scaled_length;
        // Divide in two bounded stages. Reconstructing maximum * scaled_length can
        // overflow even though both the source plane and its normalized form are
        // finite (for example, a DBL_MAX normal on all three axes).
        double distance = (plane.distance_mm / maximum) / scaled_length;
        double tolerance = plane.tolerance_mm.value_or(0);
        if (!canonicalize_triplet(&normal) || !canonicalize(distance, &distance) ||
            !canonicalize(tolerance, &tolerance))
        {
            *error = "Clipping plane canonicalization overflowed.";
            return false;
        }
        contracts::NormalizedHalfSpacePlane output;
        output.normal.assign(normal.begin(), normal.end());
        output.distance_mm = distance;
        output.tolerance_mm = tolerance;
        normalized.planes.push_back(std::move(output));
    }
    *result = std::move(normalized);
    return true;
}

double signed_distance(const contracts::NormalizedHalfSpacePlane& plane,
                       const std::array<double, 3>& point)
{
    return plane.normal[0] * point[0] + plane.normal[1] * point[1] + plane.normal[2] * point[2] -
           plane.distance_mm + plane.tolerance_mm;
}

Vertex interpolate(const Vertex& first, const Vertex& second, double amount, bool has_normals,
                   double epsilon_squared)
{
    Vertex result;
    for (std::size_t axis = 0; axis < 3; ++axis)
    {
        result.position[axis] =
            first.position[axis] + amount * (second.position[axis] - first.position[axis]);
        if (has_normals)
            result.normal[axis] =
                first.normal[axis] + amount * (second.normal[axis] - first.normal[axis]);
    }
    canonicalize_triplet(&result.position);
    if (has_normals)
    {
        const double length_squared = result.normal[0] * result.normal[0] +
                                      result.normal[1] * result.normal[1] +
                                      result.normal[2] * result.normal[2];
        if (length_squared <= epsilon_squared || !normalize_vector(&result.normal))
            result.normal = {0, 0, 0};
    }
    return result;
}

bool same_position(const Vertex& first, const Vertex& second, double epsilon_squared)
{
    const double x = first.position[0] - second.position[0];
    const double y = first.position[1] - second.position[1];
    const double z = first.position[2] - second.position[2];
    return x * x + y * y + z * z <= epsilon_squared;
}

void remove_duplicate_vertices(std::vector<Vertex>* polygon, double epsilon_squared)
{
    if (polygon->empty())
        return;
    std::vector<Vertex> compact;
    compact.reserve(polygon->size());
    for (const auto& vertex : *polygon)
        if (compact.empty() || !same_position(compact.back(), vertex, epsilon_squared))
            compact.push_back(vertex);
    if (compact.size() > 1 && same_position(compact.front(), compact.back(), epsilon_squared))
        compact.pop_back();
    *polygon = std::move(compact);
}

bool degenerate(const Vertex& first, const Vertex& second, const Vertex& third,
                double epsilon_squared)
{
    const std::array<double, 3> a{second.position[0] - first.position[0],
                                  second.position[1] - first.position[1],
                                  second.position[2] - first.position[2]};
    const std::array<double, 3> b{third.position[0] - first.position[0],
                                  third.position[1] - first.position[1],
                                  third.position[2] - first.position[2]};
    const std::array<double, 3> cross{a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
                                      a[0] * b[1] - a[1] * b[0]};
    return std::hypot(cross[0], std::hypot(cross[1], cross[2])) <= epsilon_squared;
}

void append_u32(Sha256Builder* hash, std::uint32_t value)
{
    const std::array<std::uint8_t, 4> bytes{
        static_cast<std::uint8_t>(value), static_cast<std::uint8_t>(value >> 8U),
        static_cast<std::uint8_t>(value >> 16U), static_cast<std::uint8_t>(value >> 24U)};
    hash->update(bytes.data(), bytes.size());
}

void append_u64(Sha256Builder* hash, std::uint64_t value)
{
    std::array<std::uint8_t, 8> bytes{};
    for (std::size_t index = 0; index < bytes.size(); ++index)
        bytes[index] = static_cast<std::uint8_t>(value >> (index * 8U));
    hash->update(bytes.data(), bytes.size());
}

void append_double(Sha256Builder* hash, double value)
{
    if (value == 0)
        value = 0;
    std::uint64_t bits = 0;
    static_assert(sizeof(bits) == sizeof(value));
    std::memcpy(&bits, &value, sizeof(bits));
    append_u64(hash, bits);
}

void append_string(Sha256Builder* hash, const std::string& value)
{
    append_u32(hash, static_cast<std::uint32_t>(value.size()));
    hash->update(reinterpret_cast<const std::uint8_t*>(value.data()), value.size());
}

void append_clipping(Sha256Builder* hash,
                     const std::optional<contracts::NormalizedClipping>& clipping)
{
    append_u32(hash, clipping ? 1U : 0U);
    if (!clipping)
        return;
    append_u32(hash, static_cast<std::uint32_t>(clipping->planes.size()));
    for (const auto& plane : clipping->planes)
    {
        for (const double component : plane.normal)
            append_double(hash, component);
        append_double(hash, plane.distance_mm);
        append_double(hash, plane.tolerance_mm);
    }
    append_string(hash, clipping->cap_policy);
    append_u32(hash, clipping->max_output_triangles);
    append_u32(hash, clipping->max_generated_vertices);
    append_u32(hash, clipping->max_intersections);
    append_u32(hash, clipping->max_edge_plane_tests);
}

std::string fragment_digest(const contracts::MeshCollectionA0& collection,
                            const std::optional<contracts::NormalizedClipping>& clipping,
                            bool geometry_only)
{
    Sha256Builder hash;
    const std::string prefix =
        geometry_only ? "geometer.illustration.linework.b0" : "geometer.illustration.fragment.b0";
    append_string(&hash, prefix);
    append_clipping(&hash, clipping);
    append_u32(&hash, static_cast<std::uint32_t>(collection.meshes.size()));
    for (const auto& mesh : collection.meshes)
    {
        append_string(&hash, mesh.id);
        append_u32(&hash, static_cast<std::uint32_t>(mesh.positions.size()));
        for (const double value : mesh.positions)
            append_double(&hash, value);
        append_u32(&hash, mesh.indices ? 1U : 0U);
        if (mesh.indices)
        {
            append_u32(&hash, static_cast<std::uint32_t>(mesh.indices->size()));
            for (const auto value : *mesh.indices)
                append_u32(&hash, value);
        }
        if (geometry_only)
            continue;
        append_u32(&hash, mesh.normals ? 1U : 0U);
        if (mesh.normals)
            for (const double value : *mesh.normals)
                append_double(&hash, value);
        append_u32(&hash, static_cast<std::uint32_t>(mesh.materials.size()));
        for (const auto& material : mesh.materials)
        {
            for (const double value : material.color)
                append_double(&hash, value);
            append_u32(&hash, material.opacity ? 1U : 0U);
            if (material.opacity)
                append_double(&hash, *material.opacity);
            append_u32(&hash, material.name ? 1U : 0U);
            if (material.name)
                append_string(&hash, *material.name);
        }
        append_u32(&hash, mesh.triangle_material_indices ? 1U : 0U);
        if (mesh.triangle_material_indices)
            for (const auto value : *mesh.triangle_material_indices)
                append_u32(&hash, value);
        append_u32(&hash, mesh.double_sided.value_or(false) ? 1U : 0U);
    }
    return hash.hex_digest();
}

bool update_bounds(const contracts::MeshCollectionA0& collection,
                   std::optional<contracts::ModelIllustrationBounds3MmA0>* bounds)
{
    std::array<double, 3> minimum{std::numeric_limits<double>::infinity(),
                                  std::numeric_limits<double>::infinity(),
                                  std::numeric_limits<double>::infinity()};
    std::array<double, 3> maximum{-std::numeric_limits<double>::infinity(),
                                  -std::numeric_limits<double>::infinity(),
                                  -std::numeric_limits<double>::infinity()};
    bool found = false;
    for (const auto& mesh : collection.meshes)
        for (std::size_t index = 0; index + 2 < mesh.positions.size(); index += 3)
        {
            found = true;
            for (std::size_t axis = 0; axis < 3; ++axis)
            {
                minimum[axis] = std::min(minimum[axis], mesh.positions[index + axis]);
                maximum[axis] = std::max(maximum[axis], mesh.positions[index + axis]);
            }
        }
    if (!found)
    {
        bounds->reset();
        return false;
    }
    *bounds = contracts::ModelIllustrationBounds3MmA0{minimum[0], minimum[1], minimum[2],
                                                      maximum[0], maximum[1], maximum[2]};
    return true;
}
} // namespace

int prepare_illustration_fragment(std::vector<contracts::MeshIllustrationMesh> meshes,
                                  const std::optional<contracts::IllustrationClipping>& clipping,
                                  const std::optional<std::vector<double>>& global_transform,
                                  const std::string& raw_attachment_sha256,
                                  PreparedIllustrationFragment* result, Status* status)
{
    if (result)
        *result = {};
    if (status)
        *status = {};
    if (!result)
        return fail(status, 1, "Illustration fragment result pointer is null.");

    std::string error;
    std::optional<contracts::NormalizedClipping> normalized;
    if (!normalize_clipping(clipping, &normalized, &error))
        return fail(status, 20, std::move(error));

    model_illustration_detail::Matrix4 global = model_illustration_detail::identity_matrix();
    if (global_transform &&
        !model_illustration_detail::validate_affine_matrix(*global_transform, &global, &error))
        return fail(status, 21, std::move(error));

    std::uint64_t input_triangles = 0;
    WorkBudget work;
    contracts::MeshCollectionA0 output;
    output.meshes.reserve(meshes.size());
    for (auto& mesh : meshes)
    {
        if (mesh.positions.size() % 3 != 0 || mesh.positions.empty() ||
            (mesh.normals && mesh.normals->size() != mesh.positions.size()) ||
            (mesh.indices && mesh.indices->size() % 3 != 0) || mesh.materials.empty())
            return fail(
                status, 22,
                "Illustration mesh has an invalid position, normal, index, or material layout.");
        if (!model_illustration_detail::apply_affine_matrix(&mesh, global, &error))
            return fail(status, 21, std::move(error));
        for (double& value : mesh.positions)
            if (!canonicalize(value, &value))
                return fail(status, 21, "Illustration position canonicalization overflowed.");
        if (mesh.normals)
            for (std::size_t index = 0; index < mesh.normals->size(); index += 3)
            {
                std::array<double, 3> normal{(*mesh.normals)[index], (*mesh.normals)[index + 1],
                                             (*mesh.normals)[index + 2]};
                if (!normalize_vector(&normal))
                    normal = {0, 0, 0};
                std::copy(normal.begin(), normal.end(), mesh.normals->begin() + index);
            }
        const std::size_t vertex_count = mesh.positions.size() / 3;
        const std::size_t element_count = mesh.indices ? mesh.indices->size() : vertex_count;
        if (element_count % 3 != 0)
            return fail(status, 22, "Illustration mesh triangle layout is invalid.");
        const std::size_t triangle_count = element_count / 3;
        if (mesh.triangle_material_indices)
        {
            if (mesh.triangle_material_indices->size() != triangle_count)
                return fail(status, 22,
                            "Illustration triangle material count does not match its triangles.");
            for (const auto material : *mesh.triangle_material_indices)
                if (material >= mesh.materials.size())
                    return fail(status, 22,
                                "Illustration triangle material index is out of range.");
        }
        if (input_triangles + triangle_count > kDefaultMaxOutputTriangles)
            return fail(status, 102, "Illustration input exceeds the governed triangle ceiling.");
        input_triangles += triangle_count;

        if (!normalized)
        {
            work.output_triangles += triangle_count;
            output.meshes.push_back(std::move(mesh));
            continue;
        }

        double scale = 1;
        for (const double value : mesh.positions)
            scale = std::max(scale, std::abs(value));
        const double epsilon =
            std::max(1.0e-12, 32 * std::numeric_limits<double>::epsilon() * scale);
        const double epsilon_squared = epsilon * epsilon;

        contracts::MeshIllustrationMesh clipped;
        clipped.id = mesh.id;
        clipped.materials = mesh.materials;
        clipped.double_sided = mesh.double_sided;
        clipped.indices.emplace();
        if (mesh.normals)
            clipped.normals.emplace();
        if (mesh.triangle_material_indices)
            clipped.triangle_material_indices.emplace();

        for (std::size_t triangle = 0; triangle < triangle_count; ++triangle)
        {
            std::vector<Vertex> polygon;
            polygon.reserve(3 + (normalized ? normalized->planes.size() : 0));
            for (std::size_t corner = 0; corner < 3; ++corner)
            {
                const std::size_t element = triangle * 3 + corner;
                const std::size_t vertex = mesh.indices ? (*mesh.indices)[element] : element;
                if (vertex >= vertex_count)
                    return fail(status, 22, "Illustration mesh index is out of range.");
                Vertex value;
                std::copy_n(mesh.positions.begin() + vertex * 3, 3, value.position.begin());
                if (mesh.normals)
                    std::copy_n(mesh.normals->begin() + vertex * 3, 3, value.normal.begin());
                polygon.push_back(value);
            }
            if (normalized)
                for (const auto& plane : normalized->planes)
                {
                    if (polygon.empty())
                        break;
                    std::vector<Vertex> next;
                    next.reserve(polygon.size() + 1);
                    for (std::size_t index = 0; index < polygon.size(); ++index)
                    {
                        if (++work.edge_plane_tests > normalized->max_edge_plane_tests)
                            return fail(status, 102, "Clipping edge-plane test limit exceeded.");
                        const Vertex& first = polygon[index];
                        const Vertex& second = polygon[(index + 1) % polygon.size()];
                        const double first_distance = signed_distance(plane, first.position);
                        const double second_distance = signed_distance(plane, second.position);
                        const bool first_inside = first_distance >= 0;
                        const bool second_inside = second_distance >= 0;
                        if (first_inside)
                            next.push_back(first);
                        if (first_inside != second_inside)
                        {
                            if (++work.intersections > normalized->max_intersections)
                                return fail(status, 102, "Clipping intersection limit exceeded.");
                            const double denominator = first_distance - second_distance;
                            const double amount =
                                std::clamp(first_distance / denominator, 0.0, 1.0);
                            next.push_back(interpolate(first, second, amount,
                                                       mesh.normals.has_value(), epsilon_squared));
                        }
                    }
                    remove_duplicate_vertices(&next, epsilon_squared);
                    if (work.generated_vertices > normalized->max_generated_vertices ||
                        next.size() > normalized->max_generated_vertices - work.generated_vertices)
                        return fail(status, 102, "Clipping generated vertex limit exceeded.");
                    work.generated_vertices += next.size();
                    polygon = std::move(next);
                }
            remove_duplicate_vertices(&polygon, epsilon_squared);
            if (polygon.size() < 3)
                continue;
            for (std::size_t fan = 1; fan + 1 < polygon.size(); ++fan)
            {
                const std::array<Vertex, 3> emitted{polygon[0], polygon[fan], polygon[fan + 1]};
                if (degenerate(emitted[0], emitted[1], emitted[2], epsilon_squared))
                    continue;
                if (normalized && ++work.output_triangles > normalized->max_output_triangles)
                    return fail(status, 102, "Clipping output triangle limit exceeded.");
                if (!normalized)
                    ++work.output_triangles;
                if (normalized &&
                    (work.generated_vertices > normalized->max_generated_vertices ||
                     3 > normalized->max_generated_vertices - work.generated_vertices))
                    return fail(status, 102, "Clipping generated vertex limit exceeded.");
                work.generated_vertices += 3;
                for (const auto& vertex : emitted)
                {
                    const auto index = static_cast<std::uint32_t>(clipped.positions.size() / 3);
                    clipped.indices->push_back(index);
                    clipped.positions.insert(clipped.positions.end(), vertex.position.begin(),
                                             vertex.position.end());
                    if (clipped.normals)
                        clipped.normals->insert(clipped.normals->end(), vertex.normal.begin(),
                                                vertex.normal.end());
                }
                if (clipped.triangle_material_indices)
                {
                    const auto material = (*mesh.triangle_material_indices)[triangle];
                    clipped.triangle_material_indices->push_back(material);
                }
            }
        }
        if (!clipped.indices->empty())
            output.meshes.push_back(std::move(clipped));
    }

    result->collection = std::move(output);
    result->empty = !update_bounds(result->collection, &result->bounds_mm);
    result->metadata.clipping = normalized;
    result->metadata.input_triangles = static_cast<std::uint32_t>(input_triangles);
    result->metadata.output_triangles = static_cast<std::uint32_t>(work.output_triangles);
    result->metadata.fragment_sha256 = fragment_digest(result->collection, normalized, false);
    result->metadata.linework_geometry_sha256 =
        fragment_digest(result->collection, normalized, true);
    if (!raw_attachment_sha256.empty())
        result->metadata.raw_attachment_sha256 = raw_attachment_sha256;
    return 0;
}

} // namespace geometer

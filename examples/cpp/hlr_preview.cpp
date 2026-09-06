#include "geometer.h"
#include "preview_depth_buffer.h"

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include <SDL3/SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL3/SDL_opengles2.h>
#else
#include <SDL3/SDL_opengl.h>
#endif

#if defined(_WIN32)
// clang-format off
#include <windows.h>
#include <commdlg.h>
// clang-format on
#endif

#include <rapidjson/document.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

constexpr double kPi = 3.14159265358979323846;
constexpr int kCanvasMargin = 24;

struct Vec3
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct Vec2
{
    double x = 0.0;
    double y = 0.0;
};

struct Color
{
    float r = 0.55f;
    float g = 0.55f;
    float b = 0.55f;
    float a = 1.0f;
};

struct Triangle
{
    Vec3 a;
    Vec3 b;
    Vec3 c;
    Color color;
    double depth = 0.0;
};

struct Bounds3
{
    Vec3 min = {std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(),
                std::numeric_limits<double>::infinity()};
    Vec3 max = {-std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(),
                -std::numeric_limits<double>::infinity()};

    void add(const Vec3& value)
    {
        min.x = std::min(min.x, value.x);
        min.y = std::min(min.y, value.y);
        min.z = std::min(min.z, value.z);
        max.x = std::max(max.x, value.x);
        max.y = std::max(max.y, value.y);
        max.z = std::max(max.z, value.z);
    }

    bool valid() const
    {
        return std::isfinite(min.x) && std::isfinite(min.y) && std::isfinite(min.z) &&
               std::isfinite(max.x) && std::isfinite(max.y) && std::isfinite(max.z);
    }

    Vec3 center() const
    {
        return {(min.x + max.x) * 0.5, (min.y + max.y) * 0.5, (min.z + max.z) * 0.5};
    }
};

struct Bounds2
{
    double min_x = std::numeric_limits<double>::infinity();
    double min_y = std::numeric_limits<double>::infinity();
    double max_x = -std::numeric_limits<double>::infinity();
    double max_y = -std::numeric_limits<double>::infinity();

    void add(double x, double y)
    {
        min_x = std::min(min_x, x);
        min_y = std::min(min_y, y);
        max_x = std::max(max_x, x);
        max_y = std::max(max_y, y);
    }

    bool valid() const
    {
        return std::isfinite(min_x) && std::isfinite(min_y) && std::isfinite(max_x) &&
               std::isfinite(max_y);
    }

    double width() const
    {
        return std::max(max_x - min_x, 1.0e-9);
    }

    double height() const
    {
        return std::max(max_y - min_y, 1.0e-9);
    }
};

struct Mat4
{
    std::array<double, 16> m = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
                                0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};
};

struct MeshPreview
{
    std::vector<Triangle> triangles;
    Bounds3 bounds;
};

struct Lighting
{
    float key_azimuth_deg = -42.0f;
    float key_elevation_deg = 38.0f;
    float key_intensity = 0.88f;
    float fill_intensity = 0.34f;
    float ambient = 0.30f;
    float contrast = 0.74f;
};

struct Camera
{
    Vec3 direction = {1.0, 1.0, 1.0};
    Vec3 up = {0.0, 1.0, 0.0};
    double zoom = 1.0;
};

enum class ProjectionMode
{
    Detail,
    Outline,
    Bbox,
    Both,
};

struct AppState
{
    std::array<char, 1024> step_path = {};
    std::vector<unsigned char> step_bytes;
    MeshPreview preview;
    geometer::HlrProjectionResult projection;
    bool has_projection = false;
    ProjectionMode mode = ProjectionMode::Detail;
    bool show_bbox = false;
    geometer::ProjectionAlgorithm projection_algorithm = geometer::ProjectionAlgorithm::Fast;
    geometer::ProjectionOutlineAlgorithm outline_algorithm =
        geometer::ProjectionOutlineAlgorithm::FastMeshShadow;
    GLuint preview_texture = 0;
    bool preview_texture_dirty = true;
    std::array<double, 15> preview_key = {};
    Camera camera;
    Lighting lighting;
    std::string status = "Ready";
    bool projection_dirty = false;
    std::uint64_t last_camera_change_ms = 0;
};

Vec3 operator+(const Vec3& a, const Vec3& b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

Vec3 operator-(const Vec3& a, const Vec3& b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec3 operator*(const Vec3& value, double scale)
{
    return {value.x * scale, value.y * scale, value.z * scale};
}

double dot(const Vec3& a, const Vec3& b)
{
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

Vec3 cross(const Vec3& a, const Vec3& b)
{
    return {(a.y * b.z) - (a.z * b.y), (a.z * b.x) - (a.x * b.z), (a.x * b.y) - (a.y * b.x)};
}

double length(const Vec3& value)
{
    return std::sqrt(dot(value, value));
}

Vec3 normalize(const Vec3& value, const Vec3& fallback = {0.0, 0.0, 1.0})
{
    const double len = length(value);
    if (!std::isfinite(len) || len <= 1.0e-9)
    {
        return fallback;
    }
    return value * (1.0 / len);
}

Vec3 orthogonalized_up(const Vec3& up, const Vec3& direction)
{
    Vec3 result = up - (direction * dot(up, direction));
    const double len = length(result);
    if (std::isfinite(len) && len > 1.0e-9)
    {
        return result * (1.0 / len);
    }
    const std::array<Vec3, 3> candidates = {{{0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}}};
    Vec3 best = candidates[0];
    double best_dot = std::abs(dot(best, direction));
    for (const Vec3& candidate : candidates)
    {
        const double candidate_dot = std::abs(dot(candidate, direction));
        if (candidate_dot < best_dot)
        {
            best = candidate;
            best_dot = candidate_dot;
        }
    }
    return normalize(best - (direction * dot(best, direction)), {0.0, 1.0, 0.0});
}

Vec3 rotate_axis(const Vec3& value, const Vec3& axis, double radians)
{
    const Vec3 n = normalize(axis);
    const double c = std::cos(radians);
    const double s = std::sin(radians);
    return (value * c) + (cross(n, value) * s) + (n * (dot(n, value) * (1.0 - c)));
}

void normalize_camera(Camera& camera)
{
    camera.direction = normalize(camera.direction);
    camera.up = orthogonalized_up(camera.up, camera.direction);
    camera.zoom = std::max(camera.zoom, 0.05);
}

Mat4 multiply(const Mat4& a, const Mat4& b)
{
    Mat4 result;
    result.m.fill(0.0);
    for (int column = 0; column < 4; ++column)
    {
        for (int row = 0; row < 4; ++row)
        {
            for (int k = 0; k < 4; ++k)
            {
                result.m[static_cast<std::size_t>(column * 4 + row)] +=
                    a.m[static_cast<std::size_t>(k * 4 + row)] *
                    b.m[static_cast<std::size_t>(column * 4 + k)];
            }
        }
    }
    return result;
}

Vec3 transform_point(const Mat4& matrix, const Vec3& point)
{
    return {
        (matrix.m[0] * point.x) + (matrix.m[4] * point.y) + (matrix.m[8] * point.z) + matrix.m[12],
        (matrix.m[1] * point.x) + (matrix.m[5] * point.y) + (matrix.m[9] * point.z) + matrix.m[13],
        (matrix.m[2] * point.x) + (matrix.m[6] * point.y) + (matrix.m[10] * point.z) + matrix.m[14],
    };
}

Mat4 translation_matrix(const Vec3& value)
{
    Mat4 result;
    result.m[12] = value.x;
    result.m[13] = value.y;
    result.m[14] = value.z;
    return result;
}

Mat4 scale_matrix(const Vec3& value)
{
    Mat4 result;
    result.m[0] = value.x;
    result.m[5] = value.y;
    result.m[10] = value.z;
    return result;
}

Mat4 quaternion_matrix(double x, double y, double z, double w)
{
    const double xx = x * x;
    const double yy = y * y;
    const double zz = z * z;
    const double xy = x * y;
    const double xz = x * z;
    const double yz = y * z;
    const double wx = w * x;
    const double wy = w * y;
    const double wz = w * z;

    Mat4 result;
    result.m[0] = 1.0 - (2.0 * (yy + zz));
    result.m[1] = 2.0 * (xy + wz);
    result.m[2] = 2.0 * (xz - wy);
    result.m[4] = 2.0 * (xy - wz);
    result.m[5] = 1.0 - (2.0 * (xx + zz));
    result.m[6] = 2.0 * (yz + wx);
    result.m[8] = 2.0 * (xz + wy);
    result.m[9] = 2.0 * (yz - wx);
    result.m[10] = 1.0 - (2.0 * (xx + yy));
    return result;
}

std::uint32_t read_u32_le(const unsigned char* data)
{
    return (static_cast<std::uint32_t>(data[0])) | (static_cast<std::uint32_t>(data[1]) << 8) |
           (static_cast<std::uint32_t>(data[2]) << 16) |
           (static_cast<std::uint32_t>(data[3]) << 24);
}

std::vector<unsigned char> read_file_bytes(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        throw std::runtime_error("Could not open " + path.string());
    }
    return std::vector<unsigned char>(std::istreambuf_iterator<char>(stream),
                                      std::istreambuf_iterator<char>());
}

const rapidjson::Value* member_ptr(const rapidjson::Value& value, const char* name)
{
    if (!value.IsObject())
    {
        return nullptr;
    }
    const auto it = value.FindMember(name);
    return it == value.MemberEnd() ? nullptr : &it->value;
}

double json_number(const rapidjson::Value& value, double fallback)
{
    return value.IsNumber() ? value.GetDouble() : fallback;
}

Vec3 read_vec3_array(const rapidjson::Value* value, Vec3 fallback)
{
    if (value == nullptr || !value->IsArray() || value->Size() < 3)
    {
        return fallback;
    }
    return {json_number((*value)[0], fallback.x), json_number((*value)[1], fallback.y),
            json_number((*value)[2], fallback.z)};
}

Mat4 read_node_transform(const rapidjson::Value& node)
{
    if (const rapidjson::Value* matrix = member_ptr(node, "matrix");
        matrix != nullptr && matrix->IsArray() && matrix->Size() == 16)
    {
        Mat4 result;
        for (rapidjson::SizeType i = 0; i < 16; ++i)
        {
            result.m[i] = json_number((*matrix)[i], result.m[i]);
        }
        return result;
    }

    const Vec3 translation = read_vec3_array(member_ptr(node, "translation"), {0.0, 0.0, 0.0});
    const Vec3 scale = read_vec3_array(member_ptr(node, "scale"), {1.0, 1.0, 1.0});
    Mat4 rotation;
    if (const rapidjson::Value* q = member_ptr(node, "rotation");
        q != nullptr && q->IsArray() && q->Size() >= 4)
    {
        rotation = quaternion_matrix(json_number((*q)[0], 0.0), json_number((*q)[1], 0.0),
                                     json_number((*q)[2], 0.0), json_number((*q)[3], 1.0));
    }
    return multiply(multiply(translation_matrix(translation), rotation), scale_matrix(scale));
}

Color read_material_color(const rapidjson::Document& document, int material_index)
{
    if (material_index < 0 || !document.HasMember("materials") ||
        !document["materials"].IsArray() ||
        static_cast<rapidjson::SizeType>(material_index) >= document["materials"].Size())
    {
        return {};
    }
    const rapidjson::Value& material =
        document["materials"][static_cast<rapidjson::SizeType>(material_index)];
    const rapidjson::Value* pbr = member_ptr(material, "pbrMetallicRoughness");
    const rapidjson::Value* base = pbr == nullptr ? nullptr : member_ptr(*pbr, "baseColorFactor");
    if (base == nullptr || !base->IsArray() || base->Size() < 3)
    {
        return {};
    }
    return {static_cast<float>(json_number((*base)[0], 0.55)),
            static_cast<float>(json_number((*base)[1], 0.55)),
            static_cast<float>(json_number((*base)[2], 0.55)),
            static_cast<float>(base->Size() >= 4 ? json_number((*base)[3], 1.0) : 1.0)};
}

std::pair<const rapidjson::Value&, const rapidjson::Value&>
accessor_and_view(const rapidjson::Document& document, int accessor_index)
{
    if (accessor_index < 0 || !document.HasMember("accessors") ||
        !document["accessors"].IsArray() ||
        static_cast<rapidjson::SizeType>(accessor_index) >= document["accessors"].Size())
    {
        throw std::runtime_error("GLB accessor index is out of range");
    }
    const rapidjson::Value& accessor =
        document["accessors"][static_cast<rapidjson::SizeType>(accessor_index)];
    const rapidjson::Value* view_index_value = member_ptr(accessor, "bufferView");
    if (view_index_value == nullptr || !view_index_value->IsInt())
    {
        throw std::runtime_error("GLB accessor is missing bufferView");
    }
    const int view_index = view_index_value->GetInt();
    if (view_index < 0 || !document.HasMember("bufferViews") ||
        !document["bufferViews"].IsArray() ||
        static_cast<rapidjson::SizeType>(view_index) >= document["bufferViews"].Size())
    {
        throw std::runtime_error("GLB bufferView index is out of range");
    }
    return {accessor, document["bufferViews"][static_cast<rapidjson::SizeType>(view_index)]};
}

std::size_t accessor_stride(const rapidjson::Value& accessor, const rapidjson::Value& view,
                            std::size_t component_size, std::size_t component_count)
{
    if (const rapidjson::Value* stride = member_ptr(view, "byteStride");
        stride != nullptr && stride->IsUint())
    {
        return stride->GetUint();
    }
    const std::size_t natural = component_size * component_count;
    if (const rapidjson::Value* type = member_ptr(accessor, "type");
        type != nullptr && type->IsString() && std::strcmp(type->GetString(), "VEC3") == 0)
    {
        return natural;
    }
    return natural;
}

std::vector<Vec3> read_vec3_accessor(const rapidjson::Document& document,
                                     const std::vector<unsigned char>& bin, int accessor_index)
{
    const auto [accessor, view] = accessor_and_view(document, accessor_index);
    if (!member_ptr(accessor, "componentType") ||
        member_ptr(accessor, "componentType")->GetInt() != 5126)
    {
        throw std::runtime_error("GLB VEC3 accessor must use FLOAT components");
    }
    const std::size_t count = member_ptr(accessor, "count")->GetUint64();
    const std::size_t accessor_offset = member_ptr(accessor, "byteOffset") != nullptr
                                            ? member_ptr(accessor, "byteOffset")->GetUint64()
                                            : 0;
    const std::size_t view_offset =
        member_ptr(view, "byteOffset") != nullptr ? member_ptr(view, "byteOffset")->GetUint64() : 0;
    const std::size_t stride = accessor_stride(accessor, view, sizeof(float), 3);
    const std::size_t offset = view_offset + accessor_offset;
    if (offset + ((count == 0 ? 0 : count - 1) * stride) + (3 * sizeof(float)) > bin.size())
    {
        throw std::runtime_error("GLB VEC3 accessor exceeds BIN chunk");
    }

    std::vector<Vec3> result;
    result.reserve(count);
    for (std::size_t i = 0; i < count; ++i)
    {
        const unsigned char* ptr = bin.data() + offset + (i * stride);
        float values[3] = {};
        std::memcpy(values, ptr, sizeof(values));
        result.push_back({values[0], values[1], values[2]});
    }
    return result;
}

std::vector<std::uint32_t> read_index_accessor(const rapidjson::Document& document,
                                               const std::vector<unsigned char>& bin,
                                               int accessor_index)
{
    const auto [accessor, view] = accessor_and_view(document, accessor_index);
    const int component_type = member_ptr(accessor, "componentType")->GetInt();
    const std::size_t component_size =
        component_type == 5121 ? 1
                               : (component_type == 5123 ? 2 : (component_type == 5125 ? 4 : 0));
    if (component_size == 0)
    {
        throw std::runtime_error(
            "GLB index accessor must use unsigned byte, short, or int components");
    }
    const std::size_t count = member_ptr(accessor, "count")->GetUint64();
    const std::size_t accessor_offset = member_ptr(accessor, "byteOffset") != nullptr
                                            ? member_ptr(accessor, "byteOffset")->GetUint64()
                                            : 0;
    const std::size_t view_offset =
        member_ptr(view, "byteOffset") != nullptr ? member_ptr(view, "byteOffset")->GetUint64() : 0;
    const std::size_t stride = accessor_stride(accessor, view, component_size, 1);
    const std::size_t offset = view_offset + accessor_offset;
    if (offset + ((count == 0 ? 0 : count - 1) * stride) + component_size > bin.size())
    {
        throw std::runtime_error("GLB index accessor exceeds BIN chunk");
    }

    std::vector<std::uint32_t> result;
    result.reserve(count);
    for (std::size_t i = 0; i < count; ++i)
    {
        const unsigned char* ptr = bin.data() + offset + (i * stride);
        if (component_type == 5121)
        {
            result.push_back(*ptr);
        }
        else if (component_type == 5123)
        {
            result.push_back(static_cast<std::uint32_t>(ptr[0]) |
                             (static_cast<std::uint32_t>(ptr[1]) << 8));
        }
        else
        {
            result.push_back(read_u32_le(ptr));
        }
    }
    return result;
}

void append_mesh_primitive(const rapidjson::Document& document,
                           const std::vector<unsigned char>& bin, const rapidjson::Value& primitive,
                           const Mat4& transform, MeshPreview* preview)
{
    const rapidjson::Value* mode = member_ptr(primitive, "mode");
    if (mode != nullptr && mode->IsInt() && mode->GetInt() != 4)
    {
        return;
    }
    const rapidjson::Value* attributes = member_ptr(primitive, "attributes");
    const rapidjson::Value* position_index =
        attributes == nullptr ? nullptr : member_ptr(*attributes, "POSITION");
    if (position_index == nullptr || !position_index->IsInt())
    {
        return;
    }

    std::vector<Vec3> positions = read_vec3_accessor(document, bin, position_index->GetInt());
    std::vector<std::uint32_t> indices;
    if (const rapidjson::Value* index_value = member_ptr(primitive, "indices");
        index_value != nullptr && index_value->IsInt())
    {
        indices = read_index_accessor(document, bin, index_value->GetInt());
    }
    else
    {
        indices.resize(positions.size());
        for (std::size_t i = 0; i < positions.size(); ++i)
        {
            indices[i] = static_cast<std::uint32_t>(i);
        }
    }

    int material_index = -1;
    if (const rapidjson::Value* material = member_ptr(primitive, "material");
        material != nullptr && material->IsInt())
    {
        material_index = material->GetInt();
    }
    const Color color = read_material_color(document, material_index);

    for (std::size_t i = 0; i + 2 < indices.size(); i += 3)
    {
        if (indices[i] >= positions.size() || indices[i + 1] >= positions.size() ||
            indices[i + 2] >= positions.size())
        {
            continue;
        }
        Triangle triangle;
        triangle.a = transform_point(transform, positions[indices[i]]);
        triangle.b = transform_point(transform, positions[indices[i + 1]]);
        triangle.c = transform_point(transform, positions[indices[i + 2]]);
        triangle.color = color;
        if (length(cross(triangle.b - triangle.a, triangle.c - triangle.a)) <= 1.0e-12)
        {
            continue;
        }
        preview->bounds.add(triangle.a);
        preview->bounds.add(triangle.b);
        preview->bounds.add(triangle.c);
        preview->triangles.push_back(triangle);
    }
}

void append_mesh(const rapidjson::Document& document, const std::vector<unsigned char>& bin,
                 int mesh_index, const Mat4& transform, MeshPreview* preview)
{
    if (mesh_index < 0 || !document.HasMember("meshes") || !document["meshes"].IsArray() ||
        static_cast<rapidjson::SizeType>(mesh_index) >= document["meshes"].Size())
    {
        return;
    }
    const rapidjson::Value& mesh = document["meshes"][static_cast<rapidjson::SizeType>(mesh_index)];
    const rapidjson::Value* primitives = member_ptr(mesh, "primitives");
    if (primitives == nullptr || !primitives->IsArray())
    {
        return;
    }
    for (const rapidjson::Value& primitive : primitives->GetArray())
    {
        append_mesh_primitive(document, bin, primitive, transform, preview);
    }
}

void append_node(const rapidjson::Document& document, const std::vector<unsigned char>& bin,
                 int node_index, const Mat4& parent, MeshPreview* preview)
{
    if (node_index < 0 || !document.HasMember("nodes") || !document["nodes"].IsArray() ||
        static_cast<rapidjson::SizeType>(node_index) >= document["nodes"].Size())
    {
        return;
    }
    const rapidjson::Value& node = document["nodes"][static_cast<rapidjson::SizeType>(node_index)];
    const Mat4 transform = multiply(parent, read_node_transform(node));
    if (const rapidjson::Value* mesh = member_ptr(node, "mesh"); mesh != nullptr && mesh->IsInt())
    {
        append_mesh(document, bin, mesh->GetInt(), transform, preview);
    }
    if (const rapidjson::Value* children = member_ptr(node, "children");
        children != nullptr && children->IsArray())
    {
        for (const rapidjson::Value& child : children->GetArray())
        {
            if (child.IsInt())
            {
                append_node(document, bin, child.GetInt(), transform, preview);
            }
        }
    }
}

MeshPreview parse_glb_preview(const std::vector<unsigned char>& glb)
{
    if (glb.size() < 20 || read_u32_le(glb.data()) != 0x46546C67 ||
        read_u32_le(glb.data() + 4) != 2)
    {
        throw std::runtime_error("Expected GLB v2 data from Geometer");
    }

    std::string json;
    std::vector<unsigned char> bin;
    std::size_t offset = 12;
    while (offset + 8 <= glb.size())
    {
        const std::uint32_t chunk_length = read_u32_le(glb.data() + offset);
        const std::uint32_t chunk_type = read_u32_le(glb.data() + offset + 4);
        offset += 8;
        if (offset + chunk_length > glb.size())
        {
            throw std::runtime_error("GLB chunk exceeds file length");
        }
        if (chunk_type == 0x4E4F534A)
        {
            json.assign(reinterpret_cast<const char*>(glb.data() + offset), chunk_length);
        }
        else if (chunk_type == 0x004E4942)
        {
            bin.assign(glb.begin() + static_cast<std::ptrdiff_t>(offset),
                       glb.begin() + static_cast<std::ptrdiff_t>(offset + chunk_length));
        }
        offset += chunk_length;
    }

    rapidjson::Document document;
    document.Parse(json.data(), json.size());
    if (document.HasParseError() || !document.IsObject())
    {
        throw std::runtime_error("Could not parse GLB JSON chunk");
    }

    MeshPreview preview;
    Mat4 identity;
    bool loaded_from_nodes = false;
    if (document.HasMember("scenes") && document["scenes"].IsArray() && document.HasMember("nodes"))
    {
        int scene_index = 0;
        if (const rapidjson::Value* scene = member_ptr(document, "scene");
            scene != nullptr && scene->IsInt())
        {
            scene_index = scene->GetInt();
        }
        if (scene_index >= 0 &&
            static_cast<rapidjson::SizeType>(scene_index) < document["scenes"].Size())
        {
            const rapidjson::Value* nodes = member_ptr(
                document["scenes"][static_cast<rapidjson::SizeType>(scene_index)], "nodes");
            if (nodes != nullptr && nodes->IsArray())
            {
                for (const rapidjson::Value& node : nodes->GetArray())
                {
                    if (node.IsInt())
                    {
                        append_node(document, bin, node.GetInt(), identity, &preview);
                        loaded_from_nodes = true;
                    }
                }
            }
        }
    }

    if (!loaded_from_nodes && document.HasMember("meshes") && document["meshes"].IsArray())
    {
        for (rapidjson::SizeType i = 0; i < document["meshes"].Size(); ++i)
        {
            append_mesh(document, bin, static_cast<int>(i), identity, &preview);
        }
    }

    if (preview.triangles.empty())
    {
        throw std::runtime_error("GLB did not contain triangle geometry");
    }
    return preview;
}

std::vector<std::pair<double, ImVec2>>
projected_triangle_points(const Triangle& triangle, const Vec3& center, const Vec3& right,
                          const Vec3& up, const Vec3& direction, double scale, const ImVec2& origin,
                          const ImVec2& size)
{
    const std::array<Vec3, 3> points = {triangle.a, triangle.b, triangle.c};
    std::vector<std::pair<double, ImVec2>> result;
    result.reserve(3);
    for (const Vec3& point : points)
    {
        const Vec3 local = point - center;
        const double x = dot(local, right);
        const double y = dot(local, up);
        const double z = dot(local, direction);
        const float sx = static_cast<float>(origin.x + (size.x * 0.5f) + (x * scale));
        const float sy = static_cast<float>(origin.y + (size.y * 0.5f) - (y * scale));
        result.push_back({z, {sx, sy}});
    }
    return result;
}

Bounds2 projected_preview_bounds(const MeshPreview& preview, const Vec3& center, const Vec3& right,
                                 const Vec3& up)
{
    Bounds2 bounds;
    for (const Triangle& triangle : preview.triangles)
    {
        for (const Vec3& point : {triangle.a, triangle.b, triangle.c})
        {
            const Vec3 local = point - center;
            bounds.add(dot(local, right), dot(local, up));
        }
    }
    return bounds;
}

double fit_preview_scale(const MeshPreview& preview, const Vec3& center, const Vec3& right,
                         const Vec3& up, const ImVec2& canvas_size)
{
    const Bounds2 bounds = projected_preview_bounds(preview, center, right, up);
    if (!bounds.valid())
    {
        return 1.0;
    }
    const double usable_width =
        std::max(1.0, static_cast<double>(canvas_size.x) - (2.0 * kCanvasMargin));
    const double usable_height =
        std::max(1.0, static_cast<double>(canvas_size.y) - (2.0 * kCanvasMargin));
    return std::max(1.0e-6,
                    std::min(usable_width / bounds.width(), usable_height / bounds.height()));
}

ImU32 shaded_color(const Triangle& triangle, const Camera& camera, const Lighting& lighting)
{
    Vec3 normal = normalize(cross(triangle.b - triangle.a, triangle.c - triangle.a));
    if (dot(normal, camera.direction) < 0.0)
    {
        normal = normal * -1.0;
    }
    const double az = lighting.key_azimuth_deg * kPi / 180.0;
    const double el = lighting.key_elevation_deg * kPi / 180.0;
    const Vec3 key =
        normalize({std::cos(el) * std::cos(az), std::cos(el) * std::sin(az), std::sin(el)});
    const Vec3 fill = normalize({-key.x, -key.y, std::max(0.25, key.z * 0.35)});
    const double key_term = std::max(0.0, dot(normal, key)) * lighting.key_intensity;
    const double fill_term = std::max(0.0, dot(normal, fill)) * lighting.fill_intensity;
    const double shade = std::max(0.0, std::min(1.35, lighting.ambient + key_term + fill_term));
    const double contrast = std::max(0.0f, std::min(1.0f, lighting.contrast));
    const auto channel = [shade, contrast](float base)
    {
        const double lifted = 0.5 + ((static_cast<double>(base) - 0.5) * (0.65 + contrast));
        return static_cast<int>(std::max(0.0, std::min(1.0, lifted * shade)) * 255.0);
    };
    return IM_COL32(channel(triangle.color.r), channel(triangle.color.g), channel(triangle.color.b),
                    static_cast<int>(std::max(0.0f, std::min(1.0f, triangle.color.a)) * 255.0f));
}

void draw_3d_preview(AppState* app)
{
    ImGui::TextUnformatted("3D preview");
    const ImVec2 size = ImGui::GetContentRegionAvail();
    const ImVec2 canvas_size = {std::max(size.x, 120.0f), std::max(size.y, 120.0f)};
    ImGui::InvisibleButton("3d_canvas", canvas_size, ImGuiButtonFlags_MouseButtonLeft);
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();
    const ImVec2 origin = ImGui::GetItemRectMin();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(origin, {origin.x + canvas_size.x, origin.y + canvas_size.y},
                             IM_COL32(250, 250, 250, 255));
    draw_list->AddRect(origin, {origin.x + canvas_size.x, origin.y + canvas_size.y},
                       IM_COL32(190, 195, 200, 255));

    if (active && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        const ImVec2 delta = ImGui::GetIO().MouseDelta;
        Vec3 right = normalize(cross(app->camera.up, app->camera.direction), {1.0, 0.0, 0.0});
        app->camera.direction = rotate_axis(app->camera.direction, app->camera.up,
                                            -static_cast<double>(delta.x) * 0.008);
        app->camera.direction =
            rotate_axis(app->camera.direction, right, -static_cast<double>(delta.y) * 0.008);
        normalize_camera(app->camera);
        app->projection_dirty = true;
        app->last_camera_change_ms = SDL_GetTicks();
    }
    if (hovered && ImGui::GetIO().MouseWheel != 0.0f)
    {
        app->camera.zoom *= std::pow(1.12, static_cast<double>(ImGui::GetIO().MouseWheel));
        app->camera.zoom = std::max(0.05, std::min(20.0, app->camera.zoom));
    }

    if (app->preview.triangles.empty() || !app->preview.bounds.valid())
    {
        draw_list->AddText({origin.x + 18.0f, origin.y + 18.0f}, IM_COL32(90, 98, 108, 255),
                           "No STEP preview loaded");
        return;
    }

    normalize_camera(app->camera);
    const Vec3 center = app->preview.bounds.center();
    const Vec3 direction = app->camera.direction;
    const Vec3 up = app->camera.up;
    const Vec3 right = normalize(cross(up, direction), {1.0, 0.0, 0.0});

    const double resolution = std::min(1.0, 2048.0 / std::max(canvas_size.x, canvas_size.y));
    const int width = std::max(1, static_cast<int>(std::ceil(canvas_size.x * resolution)));
    const int height = std::max(1, static_cast<int>(std::ceil(canvas_size.y * resolution)));
    const auto& light = app->lighting;
    const std::array<double, 15> key = {direction.x,
                                        direction.y,
                                        direction.z,
                                        up.x,
                                        up.y,
                                        up.z,
                                        app->camera.zoom,
                                        light.key_azimuth_deg,
                                        light.key_elevation_deg,
                                        light.key_intensity,
                                        light.fill_intensity,
                                        light.ambient,
                                        light.contrast,
                                        static_cast<double>(width),
                                        static_cast<double>(height)};
    if (app->preview_texture_dirty || app->preview_key != key)
    {
        const ImVec2 raster_size = {static_cast<float>(width), static_cast<float>(height)};
        const double scale =
            std::max(1.0e-6, fit_preview_scale(app->preview, center, right, up, raster_size) *
                                 app->camera.zoom);
        geometer_preview::DepthBuffer buffer(width, height);
        for (const Triangle& triangle : app->preview.triangles)
        {
            const auto points = projected_triangle_points(triangle, center, right, up, direction,
                                                          scale, {0, 0}, raster_size);
            std::array<geometer_preview::ScreenVertex, 3> vertices;
            for (std::size_t i = 0; i < 3; ++i)
                vertices[i] = {points[i].second.x, points[i].second.y, points[i].first};
            // The preview is opaque, matching STEP solid visibility semantics.
            buffer.triangle(vertices, shaded_color(triangle, app->camera, light) | IM_COL32_A_MASK);
        }
        GLint previous_texture = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_texture);
        if (app->preview_texture == 0)
            glGenTextures(1, &app->preview_texture);
        glBindTexture(GL_TEXTURE_2D, app->preview_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     buffer.pixels().data());
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previous_texture));
        app->preview_key = key;
        app->preview_texture_dirty = false;
    }
    draw_list->PushClipRect(origin, {origin.x + canvas_size.x, origin.y + canvas_size.y}, true);
    draw_list->AddImage(static_cast<ImTextureID>(app->preview_texture), origin,
                        {origin.x + canvas_size.x, origin.y + canvas_size.y});
    draw_list->PopClipRect();
}

std::vector<Vec2> arc_points(const geometer::ProjectedArc& arc, int samples = 48)
{
    std::vector<Vec2> result;
    if (arc.radius <= 0.0)
    {
        return result;
    }
    if (arc.full_circle)
    {
        result.reserve(static_cast<std::size_t>(samples + 1));
        for (int i = 0; i <= samples; ++i)
        {
            const double t = (2.0 * kPi * i) / samples;
            result.push_back({arc.center[0] + (std::cos(t) * arc.radius),
                              arc.center[1] + (std::sin(t) * arc.radius)});
        }
        return result;
    }
    const double start_angle =
        std::atan2(arc.start[1] - arc.center[1], arc.start[0] - arc.center[0]);
    const double extent = arc.ccw ? std::abs(arc.extent_rad) : -std::abs(arc.extent_rad);
    result.reserve(static_cast<std::size_t>(samples + 1));
    for (int i = 0; i <= samples; ++i)
    {
        const double t = start_angle + ((extent * i) / samples);
        result.push_back({arc.center[0] + (std::cos(t) * arc.radius),
                          arc.center[1] + (std::sin(t) * arc.radius)});
    }
    return result;
}

void include_geometry(Bounds2* bounds, const geometer::ProjectedModeGeometry& geometry)
{
    for (const geometer::ProjectedSegment& segment : geometry.segments)
    {
        bounds->add(segment.x1, segment.y1);
        bounds->add(segment.x2, segment.y2);
    }
    for (const geometer::ProjectedArc& arc : geometry.arcs)
    {
        for (const Vec2& point : arc_points(arc))
        {
            bounds->add(point.x, point.y);
        }
    }
}

int edge_count(const geometer::ProjectedModeGeometry& geometry)
{
    return static_cast<int>(geometry.segments.size() + geometry.arcs.size());
}

const geometer::ProjectedModeGeometry& outline_geometry(const geometer::ProjectedViewGeometry& view)
{
    return view.outline;
}

Bounds2 geometry_bounds(const geometer::ProjectedModeGeometry& geometry)
{
    Bounds2 bounds;
    include_geometry(&bounds, geometry);
    return bounds;
}

ImVec2 project_point(const Bounds2& bounds, const ImVec2& origin, const ImVec2& size, double x,
                     double y)
{
    const double scale = std::min((size.x - (2.0 * kCanvasMargin)) / bounds.width(),
                                  (size.y - (2.0 * kCanvasMargin)) / bounds.height());
    const double content_width = bounds.width() * scale;
    const double content_height = bounds.height() * scale;
    const double offset_x = (size.x - content_width) * 0.5;
    const double offset_y = (size.y - content_height) * 0.5;
    const float sx = static_cast<float>(origin.x + offset_x + ((x - bounds.min_x) * scale));
    const float sy =
        static_cast<float>(origin.y + size.y - (offset_y + ((y - bounds.min_y) * scale)));
    return {sx, sy};
}

void draw_mode_geometry(ImDrawList* draw_list, const geometer::ProjectedModeGeometry& geometry,
                        const Bounds2& bounds, const ImVec2& origin, const ImVec2& size,
                        ImU32 color)
{
    const auto project = [&](double x, double y)
    { return project_point(bounds, origin, size, x, y); };

    for (const geometer::ProjectedSegment& segment : geometry.segments)
    {
        draw_list->AddLine(project(segment.x1, segment.y1), project(segment.x2, segment.y2), color,
                           1.35f);
    }
    for (const geometer::ProjectedArc& arc : geometry.arcs)
    {
        const std::vector<Vec2> points = arc_points(arc);
        if (points.size() < 2)
        {
            continue;
        }
        std::vector<ImVec2> projected;
        projected.reserve(points.size());
        for (const Vec2& point : points)
        {
            projected.push_back(project(point.x, point.y));
        }
        draw_list->AddPolyline(projected.data(), static_cast<int>(projected.size()), color, 0,
                               1.35f);
    }
}

void draw_bounds_box(ImDrawList* draw_list, const Bounds2& box, const Bounds2& bounds,
                     const ImVec2& origin, const ImVec2& size, ImU32 color)
{
    if (!box.valid())
    {
        return;
    }
    const ImVec2 a = project_point(bounds, origin, size, box.min_x, box.min_y);
    const ImVec2 b = project_point(bounds, origin, size, box.max_x, box.max_y);
    const ImVec2 min{std::min(a.x, b.x), std::min(a.y, b.y)};
    const ImVec2 max{std::max(a.x, b.x), std::max(a.y, b.y)};
    draw_list->AddRect(min, max, color, 0.0f, 0, 1.4f);
}

void draw_hlr_preview(AppState* app)
{
    ImGui::TextUnformatted("HLR projection");
    const ImVec2 size = ImGui::GetContentRegionAvail();
    const ImVec2 canvas_size = {std::max(size.x, 120.0f), std::max(size.y, 120.0f)};
    ImGui::InvisibleButton("hlr_canvas", canvas_size);
    const ImVec2 origin = ImGui::GetItemRectMin();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(origin, {origin.x + canvas_size.x, origin.y + canvas_size.y},
                             IM_COL32(255, 255, 255, 255));
    draw_list->AddRect(origin, {origin.x + canvas_size.x, origin.y + canvas_size.y},
                       IM_COL32(190, 195, 200, 255));

    if (!app->has_projection || app->projection.views.empty())
    {
        draw_list->AddText({origin.x + 18.0f, origin.y + 18.0f}, IM_COL32(90, 98, 108, 255),
                           "No HLR projection loaded");
        return;
    }

    const geometer::ProjectedViewGeometry& view = app->projection.views.front();
    const geometer::ProjectedModeGeometry& outline = outline_geometry(view);
    const geometer::ProjectedModeGeometry& bbox = view.bbox;
    const Bounds2 bbox_bounds = geometry_bounds(bbox);
    Bounds2 bounds;
    if (app->mode == ProjectionMode::Detail || app->mode == ProjectionMode::Both)
    {
        include_geometry(&bounds, view.detail);
    }
    if (app->mode == ProjectionMode::Outline || app->mode == ProjectionMode::Both)
    {
        include_geometry(&bounds, outline);
    }
    if (app->mode == ProjectionMode::Bbox)
    {
        include_geometry(&bounds, bbox);
    }
    if (app->show_bbox && bbox_bounds.valid())
    {
        bounds.add(bbox_bounds.min_x, bbox_bounds.min_y);
        bounds.add(bbox_bounds.max_x, bbox_bounds.max_y);
    }
    if (!bounds.valid())
    {
        draw_list->AddText({origin.x + 18.0f, origin.y + 18.0f}, IM_COL32(90, 98, 108, 255),
                           "No projected geometry");
        return;
    }

    draw_list->PushClipRect(origin, {origin.x + canvas_size.x, origin.y + canvas_size.y}, true);
    if (app->mode == ProjectionMode::Outline || app->mode == ProjectionMode::Both)
    {
        draw_mode_geometry(draw_list, outline, bounds, origin, canvas_size, IM_COL32(0, 0, 0, 255));
    }
    if (app->mode == ProjectionMode::Detail || app->mode == ProjectionMode::Both)
    {
        draw_mode_geometry(draw_list, view.detail, bounds, origin, canvas_size,
                           IM_COL32(0, 0, 0, 255));
    }
    if (app->mode == ProjectionMode::Bbox)
    {
        draw_mode_geometry(draw_list, bbox, bounds, origin, canvas_size,
                           IM_COL32(218, 126, 22, 255));
    }
    if (app->show_bbox && app->mode != ProjectionMode::Bbox)
    {
        draw_bounds_box(draw_list, bbox_bounds, bounds, origin, canvas_size,
                        IM_COL32(218, 126, 22, 220));
    }
    draw_list->PopClipRect();

    const std::string label = "detail " + std::to_string(edge_count(view.detail)) + " | outline " +
                              std::to_string(edge_count(outline));
    draw_list->AddText({origin.x + 14.0f, origin.y + canvas_size.y - 24.0f},
                       IM_COL32(36, 48, 68, 255), label.c_str());
}

void apply_camera_preset(AppState* app, const char* preset)
{
    if (std::strcmp(preset, "top") == 0)
    {
        app->camera.direction = {0.0, 0.0, 1.0};
        app->camera.up = {0.0, 1.0, 0.0};
    }
    else if (std::strcmp(preset, "bottom") == 0)
    {
        app->camera.direction = {0.0, 0.0, -1.0};
        app->camera.up = {0.0, 1.0, 0.0};
    }
    else if (std::strcmp(preset, "front") == 0)
    {
        app->camera.direction = {0.0, -1.0, 0.0};
        app->camera.up = {0.0, 0.0, 1.0};
    }
    else if (std::strcmp(preset, "back") == 0)
    {
        app->camera.direction = {0.0, 1.0, 0.0};
        app->camera.up = {0.0, 0.0, 1.0};
    }
    else if (std::strcmp(preset, "left") == 0)
    {
        app->camera.direction = {-1.0, 0.0, 0.0};
        app->camera.up = {0.0, 0.0, 1.0};
    }
    else if (std::strcmp(preset, "right") == 0)
    {
        app->camera.direction = {1.0, 0.0, 0.0};
        app->camera.up = {0.0, 0.0, 1.0};
    }
    else
    {
        app->camera.direction = {1.0, 1.0, 1.0};
        app->camera.up = {0.0, 1.0, 0.0};
    }
    normalize_camera(app->camera);
    app->projection_dirty = true;
    app->last_camera_change_ms = SDL_GetTicks();
}

std::string projection_mode_name(ProjectionMode mode)
{
    switch (mode)
    {
    case ProjectionMode::Outline:
        return "outline";
    case ProjectionMode::Bbox:
        return "bbox";
    case ProjectionMode::Both:
        return "both";
    case ProjectionMode::Detail:
    default:
        return "detail";
    }
}

std::string outline_algorithm_name(geometer::ProjectionOutlineAlgorithm algorithm)
{
    switch (algorithm)
    {
    case geometer::ProjectionOutlineAlgorithm::FastMeshShadow:
        return "fast-mesh-shadow";
    case geometer::ProjectionOutlineAlgorithm::MeshShadow:
        return "mesh-shadow";
    case geometer::ProjectionOutlineAlgorithm::HlrClosedEdges:
    default:
        return "hlr-close";
    }
}

void project_hlr(AppState* app)
{
    if (app->step_bytes.empty())
    {
        return;
    }
    geometer::HlrProjectionOptions options;
    options.curve_mode = geometer::ProjectionCurveMode::Polyline;
    options.projection_algorithm = app->projection_algorithm;
    options.outline_algorithm = app->outline_algorithm;
    options.views = {{"camera",
                      {app->camera.direction.x, app->camera.direction.y, app->camera.direction.z},
                      {app->camera.up.x, app->camera.up.y, app->camera.up.z}}};

    geometer::Status status;
    geometer::HlrProjectionResult result;
    const int code = geometer::step_hlr_projection_from_bytes(
        app->step_bytes.data(), app->step_bytes.size(), options, &result, &status);
    if (code != 0)
    {
        app->has_projection = false;
        app->status = "HLR failed: " + status.message;
        return;
    }
    app->projection = std::move(result);
    app->has_projection = true;
    app->projection_dirty = false;
    const geometer::ProjectedViewGeometry& view = app->projection.views.front();
    std::ostringstream stream;
    stream << "Projected " << std::filesystem::path(app->step_path.data()).filename().string()
           << " | detail " << edge_count(view.detail) << " | outline "
           << edge_count(outline_geometry(view)) << " | bbox " << edge_count(view.bbox) << " | "
           << outline_algorithm_name(app->outline_algorithm) << " | HLR "
           << app->projection.timings.hlr_ms << " ms";
    app->status = stream.str();
}

void load_step(AppState* app)
{
    const std::filesystem::path path(app->step_path.data());
    app->status = "Loading " + path.filename().string();
    app->has_projection = false;
    app->preview = {};
    app->preview_texture_dirty = true;
    app->step_bytes = read_file_bytes(path);

    std::vector<unsigned char> glb_bytes;
    geometer::Status status;
    const int glb_code =
        geometer::step_to_glb_from_bytes(app->step_bytes.data(), app->step_bytes.size(),
                                         geometer::StepToGlbOptions{}, &glb_bytes, &status);
    if (glb_code != 0)
    {
        throw std::runtime_error("STEP-to-GLB failed: " + status.message);
    }
    app->preview = parse_glb_preview(glb_bytes);
    app->camera.zoom = 1.0;
    project_hlr(app);
}

void set_step_path(AppState* app, const std::filesystem::path& path)
{
    const std::string path_text = path.string();
    std::snprintf(app->step_path.data(), app->step_path.size(), "%s", path_text.c_str());
}

void try_load_step(AppState* app)
{
    try
    {
        load_step(app);
    }
    catch (const std::exception& exc)
    {
        app->status = exc.what();
    }
}

std::optional<std::filesystem::path> choose_step_file_dialog(const AppState* app)
{
#if defined(_WIN32)
    const std::filesystem::path current_path(app->step_path.data());
    std::array<wchar_t, 4096> file_name = {};
    const std::wstring initial_file = current_path.wstring();
    const std::size_t copy_size = std::min(initial_file.size(), file_name.size() - 1);
    std::copy_n(initial_file.c_str(), copy_size, file_name.data());

    std::wstring initial_dir;
    if (current_path.has_parent_path())
    {
        initial_dir = current_path.parent_path().wstring();
    }
    else
    {
        initial_dir = std::filesystem::current_path().wstring();
    }

    OPENFILENAMEW open_file = {};
    open_file.lStructSize = sizeof(open_file);
    open_file.lpstrFile = file_name.data();
    open_file.nMaxFile = static_cast<DWORD>(file_name.size());
    open_file.lpstrFilter = L"STEP files (*.step;*.stp)\0*.step;*.stp\0All files (*.*)\0*.*\0";
    open_file.nFilterIndex = 1;
    open_file.lpstrInitialDir = initial_dir.c_str();
    open_file.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameW(&open_file) == TRUE)
    {
        return std::filesystem::path(file_name.data());
    }
    return std::nullopt;
#else
    (void)app;
    return std::nullopt;
#endif
}

void draw_controls(AppState* app)
{
    ImGui::Text("Geometer %s | ABI %d", geometer::version_string(), geometer::abi_version());
    ImGui::Separator();

    ImGui::SetNextItemWidth(-174.0f);
    ImGui::InputText("STEP", app->step_path.data(), app->step_path.size());
    ImGui::SameLine();
    if (ImGui::Button("Open...", {82.0f, 0.0f}))
    {
        if (const std::optional<std::filesystem::path> selected = choose_step_file_dialog(app))
        {
            set_step_path(app, *selected);
            try_load_step(app);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Load", {70.0f, 0.0f}))
    {
        try_load_step(app);
    }

    ImGui::TextUnformatted("Camera");
    ImGui::SameLine();
    for (const char* preset : {"ISO", "Top", "Bottom", "Front", "Back", "Left", "Right"})
    {
        if (ImGui::Button(preset))
        {
            std::string lower = preset;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            apply_camera_preset(app, lower.c_str());
        }
        ImGui::SameLine();
    }
    if (ImGui::Button("Fit"))
    {
        app->camera.zoom = 1.0;
    }

    ImGui::TextUnformatted("Projection");
    ImGui::SameLine();
    const char* modes[] = {"detail", "outline", "both"};
    int mode_index = app->mode == ProjectionMode::Detail    ? 0
                     : app->mode == ProjectionMode::Outline ? 1
                                                            : 2;
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::Combo("Mode", &mode_index, modes, 3))
    {
        app->mode = mode_index == 0
                        ? ProjectionMode::Detail
                        : (mode_index == 1 ? ProjectionMode::Outline : ProjectionMode::Both);
    }
    ImGui::SameLine();
    ImGui::Checkbox("BBox", &app->show_bbox);
    ImGui::SameLine();
    const char* detail_algorithms[] = {"fast", "poly", "exact"};
    int detail_algorithm_index =
        app->projection_algorithm == geometer::ProjectionAlgorithm::Fast
            ? 0
            : (app->projection_algorithm == geometer::ProjectionAlgorithm::Poly ? 1 : 2);
    ImGui::SetNextItemWidth(85.0f);
    if (ImGui::Combo("Detail alg", &detail_algorithm_index, detail_algorithms, 3))
    {
        app->projection_algorithm =
            detail_algorithm_index == 0
                ? geometer::ProjectionAlgorithm::Fast
                : (detail_algorithm_index == 1 ? geometer::ProjectionAlgorithm::Poly
                                               : geometer::ProjectionAlgorithm::Exact);
        app->projection_dirty = true;
        app->last_camera_change_ms = SDL_GetTicks();
    }
    ImGui::SameLine();
    const char* outline_algorithms[] = {"fast-mesh-shadow", "mesh-shadow", "hlr-close"};
    int outline_algorithm_index =
        app->outline_algorithm == geometer::ProjectionOutlineAlgorithm::FastMeshShadow
            ? 0
            : (app->outline_algorithm == geometer::ProjectionOutlineAlgorithm::MeshShadow ? 1 : 2);
    ImGui::SetNextItemWidth(165.0f);
    if (ImGui::Combo("Outline alg", &outline_algorithm_index, outline_algorithms, 3))
    {
        app->outline_algorithm = outline_algorithm_index == 0
                                     ? geometer::ProjectionOutlineAlgorithm::FastMeshShadow
                                     : (outline_algorithm_index == 1
                                            ? geometer::ProjectionOutlineAlgorithm::MeshShadow
                                            : geometer::ProjectionOutlineAlgorithm::HlrClosedEdges);
        app->projection_dirty = true;
        app->last_camera_change_ms = SDL_GetTicks();
    }

    ImGui::TextUnformatted("Lighting");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(92.0f);
    ImGui::DragFloat("Az", &app->lighting.key_azimuth_deg, 1.0f, -180.0f, 180.0f, "%.0f");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(92.0f);
    ImGui::DragFloat("El", &app->lighting.key_elevation_deg, 1.0f, -80.0f, 80.0f, "%.0f");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(92.0f);
    ImGui::SliderFloat("Key", &app->lighting.key_intensity, 0.0f, 1.0f, "%.2f");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(92.0f);
    ImGui::SliderFloat("Fill", &app->lighting.fill_intensity, 0.0f, 1.0f, "%.2f");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(92.0f);
    ImGui::SliderFloat("Amb", &app->lighting.ambient, 0.0f, 1.0f, "%.2f");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(108.0f);
    ImGui::SliderFloat("Contrast", &app->lighting.contrast, 0.0f, 1.0f, "%.2f");

    ImGui::Text("Mode: %s | %s", projection_mode_name(app->mode).c_str(), app->status.c_str());
}

void draw_app(AppState* app)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::Begin("Geometer HLR Preview", nullptr,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    draw_controls(app);

    if (ImGui::BeginTable("preview_table", 2,
                          ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV |
                              ImGuiTableFlags_SizingStretchSame,
                          ImGui::GetContentRegionAvail()))
    {
        ImGui::TableNextColumn();
        ImGui::BeginChild("3d_child", ImGui::GetContentRegionAvail(), false);
        draw_3d_preview(app);
        ImGui::EndChild();

        ImGui::TableNextColumn();
        ImGui::BeginChild("hlr_child", ImGui::GetContentRegionAvail(), false);
        draw_hlr_preview(app);
        ImGui::EndChild();
        ImGui::EndTable();
    }

    ImGui::End();
}

std::filesystem::path default_step_path(int argc, char** argv)
{
    if (argc > 1)
    {
        return argv[1];
    }
    const std::filesystem::path relative =
        std::filesystem::path("tests") / "fixtures" / "step" / "embedded_models" / "SOT-23.STEP";
    std::vector<std::filesystem::path> roots = {std::filesystem::current_path()};
    if (argc > 0 && argv[0] != nullptr)
    {
        roots.push_back(std::filesystem::absolute(argv[0]).parent_path());
    }
    for (const std::filesystem::path& root : roots)
    {
        std::filesystem::path current = root;
        for (int depth = 0; depth < 6; ++depth)
        {
            const std::filesystem::path candidate = current / relative;
            if (std::filesystem::exists(candidate))
            {
                return candidate;
            }
            if (!current.has_parent_path() || current == current.parent_path())
            {
                break;
            }
            current = current.parent_path();
        }
    }
    return relative;
}

float dpi_scale_for_window(SDL_Window* window)
{
    const float scale = SDL_GetWindowDisplayScale(window);
    if (!std::isfinite(scale) || scale <= 0.0f)
    {
        return 1.0f;
    }
    return std::max(1.0f, scale);
}

} // namespace

int main(int argc, char** argv)
{
    AppState app;
    const std::string initial_path = default_step_path(argc, argv).string();
    std::snprintf(app.step_path.data(), app.step_path.size(), "%s", initial_path.c_str());
    normalize_camera(app.camera);

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_Window* window =
        SDL_CreateWindow("Geometer HLR Preview", 1500, 940,
                         SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (window == nullptr)
    {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_SetWindowMinimumSize(window, 1100, 720);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (gl_context == nullptr)
    {
        std::fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    const float dpi_scale = dpi_scale_for_window(window);
    style.FontScaleDpi = dpi_scale;
    style.ScaleAllSizes(dpi_scale);

    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 130");

    try
    {
        load_step(&app);
    }
    catch (const std::exception& exc)
    {
        app.status = exc.what();
    }

    bool done = false;
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                                                 event.window.windowID == SDL_GetWindowID(window)))
            {
                done = true;
            }
        }

        if (app.projection_dirty && SDL_GetTicks() - app.last_camera_change_ms > 350)
        {
            project_hlr(&app);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        draw_app(&app);

        ImGui::Render();
        int display_w = 0;
        int display_h = 0;
        SDL_GetWindowSizeInPixels(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.10f, 0.105f, 0.11f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    if (app.preview_texture != 0)
        glDeleteTextures(1, &app.preview_texture);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

#pragma once

// Renderer working data only. Public/serialized values belong to the generated
// MeshIllustrationA0 TypeSpec contracts, not to these private structures.
#include "geometer/generated/contracts/contracts.h"

#include <array>
#include <cmath>
#include <functional>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace geometer::illustration_detail
{
using Vec2 = std::array<double, 2>;
using Vec3 = std::array<double, 3>;
using Ring = std::vector<Vec2>;
using Rings = std::vector<Ring>;
using Matrix = std::array<double, 16>;
inline constexpr Matrix identity{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

struct Bounds
{
    double min_x = std::numeric_limits<double>::infinity();
    double min_y = std::numeric_limits<double>::infinity();
    double max_x = -std::numeric_limits<double>::infinity();
    double max_y = -std::numeric_limits<double>::infinity();
    void include(Vec2 point);
};

struct Triangle
{
    std::array<Vec2, 3> points;
    Vec3 depths, normal, geometric_normal, color;
    double depth = 0, opacity = 1;
    bool front = false, double_sided = false;
};

struct Edge
{
    std::array<Vec2, 2> points;
    double depth = 0;
    bool front_a = false;
    std::optional<bool> front_b;
    Vec3 normal_a;
    std::optional<Vec3> normal_b;
};

struct Scene
{
    Vec3 direction;
    Bounds bounds;
    std::vector<Triangle> triangles;
    std::vector<Edge> edges;
    std::vector<std::string> warnings;
};

struct Style
{
    contracts::MeshIllustrationShading shading = contracts::MeshIllustrationShading::toon;
    double ambient = .25, key_intensity = .9;
    Vec3 light_direction{.4, .7, 1};
    unsigned bands = 3;
    bool source_colors = true;
    Vec3 fallback_color{.72, .74, .78};
    std::string background = "#ffffff";
    bool transparent_background = false, fuse_surfaces = true, layer_coplanar_materials = true;
    bool show_hlr_outline = true, show_hlr_detail = false;
    bool show_outlines = true, show_creases = true;
    double crease_angle_degrees = 30;
    std::string outline_color = "#17252c", crease_color = "#33444a";
    double outline_width = .006, crease_width = .003;
    bool double_sided = false;
    double rim_amount = .12;
};

struct TriangleCommand
{
    const Triangle* triangle = nullptr;
    double depth = 0;
    std::size_t order = 0;
    std::string fill;
    double opacity = 1;
};

struct Layer
{
    Rings rings;
    std::string fill;
    double opacity = 1;
};

struct Surface
{
    // Triangle surfaces retain polygon emission; fused/layered surfaces use paths.
    bool polygon = false;
    bool layered = false;
    std::vector<Layer> layers;
};

struct Line
{
    std::array<Vec2, 2> points;
    std::string color;
    double width = 0, depth = 0;
    std::size_t order = 0;
};

struct Commands
{
    std::vector<Surface> surfaces;
    std::vector<Line> lines;
    contracts::MeshIllustrationRenderStats stats;
};

struct ResourceLimit : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

// Bound adversarial broad-phase work without silently switching renderers.
struct WorkBudget
{
    std::size_t remaining = 20000000;
    void consume()
    {
        if (remaining == 0)
            throw ResourceLimit("Mesh illustration exceeds the candidate comparison limit.");
        --remaining;
    }
};

double clamp(double value, double minimum = 0, double maximum = 1);
double js_round(double value);
std::string number_text(double value);
std::string integer_text(double value);
std::string fixed_text(double value);
Vec3 add(Vec3 a, Vec3 b);
Vec3 subtract(Vec3 a, Vec3 b);
double dot(Vec3 a, Vec3 b);
Vec3 cross(Vec3 a, Vec3 b);
double length(Vec3 a);
Vec3 normalize(Vec3 a, const char* label);
Vec3 vector3(const std::vector<double>& values);
Vec3 position(const std::vector<double>& values, std::size_t index);
Vec3 transform_point(const Matrix& matrix, Vec3 point);
double determinant_sign(const Matrix& matrix);
Vec3 transform_normal(const Matrix& matrix, Vec3 normal);
double cross2(Vec2 a, Vec2 b, Vec2 point);
double signed_area(const Ring& points);
double signed_area(const std::array<Vec2, 3>& points);
Bounds projected_bounds(const Triangle& triangle);
bool bounds_overlap(const Bounds& a, const Bounds& b, double epsilon);
Ring clip_polygon(const Triangle& a, const Triangle& b, double epsilon);
bool significant_overlap(const Ring& overlap, const Triangle& a, const Triangle& b, double epsilon);
double depth_at(const Triangle& triangle, Vec2 point);
void candidate_pairs(const std::vector<Bounds>& boxes, const Bounds& bounds, unsigned grid_size,
                     unsigned broad_limit, double minimum_cell, WorkBudget& budget,
                     const std::function<void(std::size_t, std::size_t)>& visit);
Scene prepare_scene(const contracts::MeshIllustrationInputA0& input);
Style resolve_style(const contracts::MeshIllustrationStyleA0& patch);
std::string triangle_fill(const Triangle& triangle, const Scene& scene, const Style& style);
std::vector<TriangleCommand> order_triangles(const std::vector<TriangleCommand>& commands,
                                             const Bounds& bounds, WorkBudget& budget);
Surface triangle_surface(const TriangleCommand& command);
std::vector<Surface> fuse_triangles(const std::vector<TriangleCommand>& commands,
                                    const Bounds& bounds, bool layer_materials, WorkBudget& budget);
Commands render_commands(const Scene& scene, const Style& style, WorkBudget& budget);
std::string render_svg(const Scene& scene, const Style& style, const Commands& commands,
                       const contracts::MeshIllustrationSvgOptions& options);
} // namespace geometer::illustration_detail

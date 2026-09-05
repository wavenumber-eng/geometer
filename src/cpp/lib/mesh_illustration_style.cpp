#include "mesh_illustration_internal.h"

#include <algorithm>

namespace geometer::illustration_detail
{
Style resolve_style(const contracts::MeshIllustrationStyleA0& patch)
{
    Style s;
    s.shading = patch.shading.value_or(s.shading);
    s.ambient = patch.ambient.value_or(s.ambient);
    s.key_intensity = patch.key_intensity.value_or(s.key_intensity);
    if (patch.light_direction)
        s.light_direction = vector3(*patch.light_direction);
    s.bands = patch.bands.value_or(s.bands);
    s.source_colors = patch.source_colors.value_or(s.source_colors);
    if (patch.fallback_color)
        s.fallback_color = vector3(*patch.fallback_color);
    s.background = patch.background.value_or(s.background);
    s.transparent_background = patch.transparent_background.value_or(s.transparent_background);
    s.fuse_surfaces = patch.fuse_surfaces.value_or(s.fuse_surfaces);
    s.layer_coplanar_materials =
        patch.layer_coplanar_materials.value_or(s.layer_coplanar_materials);
    s.show_hlr_outline = patch.show_hlr_outline.value_or(s.show_hlr_outline);
    s.show_hlr_detail = patch.show_hlr_detail.value_or(s.show_hlr_detail);
    s.show_outlines = patch.show_outlines.value_or(s.show_outlines);
    s.show_creases = patch.show_creases.value_or(s.show_creases);
    s.crease_angle_degrees = patch.crease_angle_degrees.value_or(s.crease_angle_degrees);
    s.outline_color = patch.outline_color.value_or(s.outline_color);
    s.crease_color = patch.crease_color.value_or(s.crease_color);
    s.outline_width = patch.outline_width.value_or(s.outline_width);
    s.crease_width = patch.crease_width.value_or(s.crease_width);
    s.double_sided = patch.double_sided.value_or(s.double_sided);
    s.rim_amount = patch.rim_amount.value_or(s.rim_amount);
    return s;
}

namespace
{
double srgb_to_linear(double value)
{
    const double v = clamp(value);
    return v <= .04045 ? v / 12.92 : std::pow((v + .055) / 1.055, 2.4);
}
double linear_to_srgb(double value)
{
    const double v = clamp(value);
    return v <= .0031308 ? v * 12.92 : 1.055 * std::pow(v, 1 / 2.4) - .055;
}
std::string rgb_css(Vec3 color, double intensity, double rim, double lift)
{
    std::string result = "rgb(";
    for (unsigned axis = 0; axis < 3; ++axis)
    {
        const double lit =
            srgb_to_linear(color[axis]) * std::max(0.0, intensity) + clamp(lift, 0, .08);
        const double with_rim = lit + (1 - lit) * clamp(rim);
        if (axis)
            result += ",";
        result += integer_text(clamp(linear_to_srgb(with_rim)) * 255);
    }
    return result + ")";
}
} // namespace

std::string triangle_fill(const Triangle& triangle, const Scene& scene, const Style& style)
{
    using Shading = contracts::MeshIllustrationShading;
    const auto base = style.source_colors ? triangle.color : style.fallback_color;
    if (style.shading == Shading::unlit)
        return rgb_css(base, 1, 0, 0);
    const auto light = normalize(style.light_direction, "Light direction");
    const auto normal =
        style.shading == Shading::flat ? triangle.geometric_normal : triangle.normal;
    double intensity = clamp(clamp(style.ambient) +
                             clamp(style.key_intensity, 0, 4) * std::max(0.0, dot(normal, light)));
    if (style.shading == Shading::banded || style.shading == Shading::toon)
    {
        const auto bands = std::clamp(style.bands, 2u, 32u);
        intensity = js_round(intensity * (bands - 1)) / (bands - 1);
    }
    const bool toon = style.shading == Shading::toon;
    const double rim = toon ? clamp(style.rim_amount) *
                                  clamp((1 - std::abs(dot(normal, scene.direction)) - .45) / .55)
                            : 0;
    return rgb_css(base, intensity, rim, toon ? .012 : 0);
}
} // namespace geometer::illustration_detail

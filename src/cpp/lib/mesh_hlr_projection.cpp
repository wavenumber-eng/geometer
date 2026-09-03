#include "geometer/projection.h"

#include "fast_mesh_shadow_outline.h"
#include "geometer/fast_hlr.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <limits>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

namespace geometer
{
namespace
{

struct Vec3
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct ViewBasis
{
    Vec3 x;
    Vec3 y;
    Vec3 z;
};

struct SegmentKey
{
    std::int64_t x1 = 0;
    std::int64_t y1 = 0;
    std::int64_t x2 = 0;
    std::int64_t y2 = 0;

    bool operator<(const SegmentKey& other) const
    {
        return std::tie(x1, y1, x2, y2) < std::tie(other.x1, other.y1, other.x2, other.y2);
    }
};

void set_status(Status* status, int code, const char* message)
{
    if (status != nullptr)
    {
        status->code = code;
        status->message = message == nullptr ? "" : message;
    }
}

double dot(const Vec3& left, const Vec3& right)
{
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

Vec3 cross(const Vec3& left, const Vec3& right)
{
    return {left.y * right.z - left.z * right.y, left.z * right.x - left.x * right.z,
            left.x * right.y - left.y * right.x};
}

bool normalize(Vec3* value)
{
    const double magnitude = std::sqrt(dot(*value, *value));
    if (!std::isfinite(magnitude) || magnitude <= 1.0e-15)
        return false;
    value->x /= magnitude;
    value->y /= magnitude;
    value->z /= magnitude;
    return true;
}

bool make_basis(const ProjectionViewSpec& view, ViewBasis* basis)
{
    basis->z = {view.direction[0], view.direction[1], view.direction[2]};
    Vec3 up{view.up[0], view.up[1], view.up[2]};
    if (!normalize(&basis->z))
        return false;
    const double depth = dot(up, basis->z);
    basis->y = {up.x - basis->z.x * depth, up.y - basis->z.y * depth, up.z - basis->z.z * depth};
    if (!normalize(&basis->y))
        return false;
    basis->x = cross(basis->y, basis->z);
    return normalize(&basis->x);
}

std::int64_t pow10_int(int digits)
{
    std::int64_t value = 1;
    for (int index = 0; index < digits; ++index)
        value *= 10;
    return value;
}

bool snap(double value, std::int64_t scale, std::int64_t* output)
{
    const double scaled = value * static_cast<double>(scale);
    if (!std::isfinite(scaled) ||
        scaled < static_cast<double>(std::numeric_limits<std::int64_t>::min()) ||
        scaled > static_cast<double>(std::numeric_limits<std::int64_t>::max()))
        return false;
    *output = static_cast<std::int64_t>(std::llround(scaled));
    return true;
}

bool canonicalize(ProjectedModeGeometry* geometry, std::int64_t scale)
{
    std::set<SegmentKey> keys;
    for (const ProjectedSegment& segment : geometry->segments)
    {
        SegmentKey key;
        if (!snap(segment.x1, scale, &key.x1) || !snap(segment.y1, scale, &key.y1) ||
            !snap(segment.x2, scale, &key.x2) || !snap(segment.y2, scale, &key.y2))
            return false;
        if (std::tie(key.x2, key.y2) < std::tie(key.x1, key.y1))
        {
            std::swap(key.x1, key.x2);
            std::swap(key.y1, key.y2);
        }
        if (key.x1 != key.x2 || key.y1 != key.y2)
            keys.insert(key);
    }
    geometry->segments.clear();
    geometry->segments.reserve(keys.size());
    for (const SegmentKey& key : keys)
    {
        geometry->segments.push_back(
            {static_cast<double>(key.x1) / scale, static_cast<double>(key.y1) / scale,
             static_cast<double>(key.x2) / scale, static_cast<double>(key.y2) / scale});
    }
    geometry->arcs.clear();
    return true;
}

std::vector<ProjectionViewSpec> effective_views(const HlrProjectionOptions& options)
{
    if (!options.views.empty())
        return options.views;
    return {{"top", {0.0, 0.0, 1.0}, {0.0, 1.0, 0.0}},
            {"bottom", {0.0, 0.0, -1.0}, {0.0, 1.0, 0.0}}};
}

bool nonsingular_linear_transform(const std::array<double, 16>& transform)
{
    std::array<std::array<double, 3>, 3> matrix = {
        std::array<double, 3>{transform[0], transform[1], transform[2]},
        std::array<double, 3>{transform[4], transform[5], transform[6]},
        std::array<double, 3>{transform[8], transform[9], transform[10]}};
    std::array<double, 3> row_scales{};
    for (std::size_t row = 0; row < matrix.size(); ++row)
    {
        row_scales[row] = std::max(
            {std::fabs(matrix[row][0]), std::fabs(matrix[row][1]), std::fabs(matrix[row][2])});
        if (!(row_scales[row] > 0.0) || !std::isfinite(row_scales[row]))
            return false;
    }

    constexpr double relative_pivot_tolerance = 64.0 * std::numeric_limits<double>::epsilon();
    for (std::size_t column = 0; column < matrix.size(); ++column)
    {
        std::size_t pivot = column;
        double best_scaled_pivot = 0.0;
        for (std::size_t row = column; row < matrix.size(); ++row)
        {
            const double scaled_pivot = std::fabs(matrix[row][column]) / row_scales[row];
            if (!std::isfinite(scaled_pivot))
                return false;
            if (scaled_pivot > best_scaled_pivot)
            {
                best_scaled_pivot = scaled_pivot;
                pivot = row;
            }
        }
        if (!(best_scaled_pivot > relative_pivot_tolerance))
            return false;
        if (pivot != column)
        {
            std::swap(matrix[pivot], matrix[column]);
            std::swap(row_scales[pivot], row_scales[column]);
        }
        for (std::size_t row = column + 1; row < matrix.size(); ++row)
        {
            const double factor = matrix[row][column] / matrix[column][column];
            if (!std::isfinite(factor))
                return false;
            for (std::size_t entry = column + 1; entry < matrix.size(); ++entry)
            {
                matrix[row][entry] -= factor * matrix[column][entry];
                if (!std::isfinite(matrix[row][entry]))
                    return false;
            }
        }
    }
    return true;
}

bool transformed_mesh(const FastHlrIndexedMesh& input, const std::array<double, 16>& transform,
                      FastHlrIndexedMesh* output)
{
    constexpr double tolerance = 1.0e-12;
    if (!std::all_of(transform.begin(), transform.end(),
                     [](double value) { return std::isfinite(value); }))
        return false;
    if (std::fabs(transform[12]) > tolerance || std::fabs(transform[13]) > tolerance ||
        std::fabs(transform[14]) > tolerance || std::fabs(transform[15] - 1.0) > tolerance)
        return false;
    if (!nonsingular_linear_transform(transform))
        return false;
    *output = input;
    for (FastHlrVec3& vertex : output->vertices)
    {
        const double x = vertex.x;
        const double y = vertex.y;
        const double z = vertex.z;
        vertex = {transform[0] * x + transform[1] * y + transform[2] * z + transform[3],
                  transform[4] * x + transform[5] * y + transform[6] * z + transform[7],
                  transform[8] * x + transform[9] * y + transform[10] * z + transform[11]};
        if (!std::isfinite(vertex.x) || !std::isfinite(vertex.y) || !std::isfinite(vertex.z))
            return false;
    }
    return true;
}

ProjectedModeGeometry bbox_geometry(const FastHlrPreparedMesh& prepared,
                                    const ProjectionViewSpec& view, std::int64_t scale)
{
    ProjectedModeGeometry result;
    ViewBasis basis;
    if (prepared.vertices.empty() || !make_basis(view, &basis))
        return result;
    double min_x = std::numeric_limits<double>::infinity();
    double min_y = std::numeric_limits<double>::infinity();
    double max_x = -std::numeric_limits<double>::infinity();
    double max_y = -std::numeric_limits<double>::infinity();
    for (const FastHlrVec3& vertex : prepared.vertices)
    {
        const Vec3 point{vertex.x, vertex.y, vertex.z};
        const double x = dot(point, basis.x);
        const double y = dot(point, basis.y);
        min_x = std::min(min_x, x);
        min_y = std::min(min_y, y);
        max_x = std::max(max_x, x);
        max_y = std::max(max_y, y);
    }
    min_x = std::floor(min_x * scale) / scale;
    min_y = std::floor(min_y * scale) / scale;
    max_x = std::ceil(max_x * scale) / scale;
    max_y = std::ceil(max_y * scale) / scale;
    result.segments = {{min_x, min_y, max_x, min_y},
                       {max_x, min_y, max_x, max_y},
                       {min_x, max_y, max_x, max_y},
                       {min_x, min_y, min_x, max_y}};
    return result;
}

double elapsed_ms(const std::chrono::high_resolution_clock::time_point& start)
{
    return std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() -
                                                     start)
        .count();
}

} // namespace

int mesh_hlr_projection(const FastHlrIndexedMesh& mesh, const HlrProjectionOptions& options,
                        HlrProjectionResult* result, Status* status)
{
    if (result == nullptr)
    {
        set_status(status, 2, "Projection result pointer is null.");
        return 2;
    }
    if (options.projection_algorithm != ProjectionAlgorithm::Fast)
    {
        set_status(status, 3, "Indexed-mesh projection requires projection_algorithm=fast.");
        return 3;
    }
    if (options.round_digits < 0 || options.round_digits > 9)
    {
        set_status(status, 3, "Projection round_digits must be between 0 and 9.");
        return 3;
    }
    if (options.strip_root_placement)
    {
        set_status(status, 3, "strip_root_placement is not applicable to indexed meshes.");
        return 3;
    }
    if (options.output_outline &&
        options.outline_algorithm == ProjectionOutlineAlgorithm::HlrClosedEdges)
    {
        set_status(status, 3, "Indexed-mesh outline requires mesh-shadow or fast-mesh-shadow.");
        return 3;
    }

    try
    {
        const auto prepare_start = std::chrono::high_resolution_clock::now();
        FastHlrIndexedMesh transformed;
        if (!transformed_mesh(mesh, options.model_transform, &transformed))
        {
            set_status(status, 3, "Projection model_transform is invalid or non-finite.");
            return 3;
        }
        FastHlrPreparedMesh prepared;
        const int prepare_code =
            prepare_fast_hlr_mesh(transformed, options.fast, &prepared, status);
        if (prepare_code != 0)
            return prepare_code;

        HlrProjectionResult output;
        output.schema = "geometry.hlr_projection.result.a0";
        output.units = "mm";
        output.timings.mesh_ms = elapsed_ms(prepare_start);
        const std::int64_t scale = pow10_int(options.round_digits);
        const std::vector<ProjectionViewSpec> views = effective_views(options);
        output.views.reserve(views.size());
        for (const ProjectionViewSpec& view : views)
        {
            ProjectedViewGeometry projected;
            projected.view = view;
            if (options.output_detail)
            {
                const auto detail_start = std::chrono::high_resolution_clock::now();
                ProjectedModeGeometry hidden;
                const int detail_code = project_fast_hlr_detail(
                    prepared, view, options.fast, &projected.detail,
                    options.fast.include_hidden ? &hidden : nullptr, nullptr, status);
                if (detail_code != 0)
                    return detail_code;
                if (options.fast.include_hidden)
                    projected.detail.segments.insert(projected.detail.segments.end(),
                                                     hidden.segments.begin(),
                                                     hidden.segments.end());
                if (!canonicalize(&projected.detail, scale))
                {
                    set_status(status, 5, "Projected detail exceeds the coordinate range.");
                    return 5;
                }
                output.timings.hlr_ms += elapsed_ms(detail_start);
            }
            if (options.output_outline)
            {
                const auto outline_start = std::chrono::high_resolution_clock::now();
                const int outline_code = fast_mesh_shadow_outline_geometry(
                    prepared, view, options.fast, scale, &projected.outline, nullptr, status);
                if (outline_code != 0)
                    return outline_code;
                output.timings.extract_ms += elapsed_ms(outline_start);
            }
            if (options.output_bbox)
                projected.bbox = bbox_geometry(prepared, view, scale);
            output.views.push_back(std::move(projected));
        }
        *result = std::move(output);
        set_status(status, 0, "");
        return 0;
    }
    catch (const std::exception& error)
    {
        if (status != nullptr)
        {
            status->code = 8;
            status->message = error.what();
        }
        return 8;
    }
}

} // namespace geometer

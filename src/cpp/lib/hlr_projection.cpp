#include "geometer/projection.h"

#include "fast_hlr_occt.h"
#include "fast_mesh_shadow_outline.h"
#include "mesh_shadow_outline.h"
#include "step_xcaf_root_frame.h"

#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_GTransform.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <Bnd_Box.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <STEPControl_Reader.hxx>
#include <Standard_Failure.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TDocStd_Document.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_GTrsf.hxx>
#include <gp_Mat.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <gp_XYZ.hxx>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace geometer
{
namespace
{

struct SegmentKey
{
    std::int64_t x1 = 0;
    std::int64_t y1 = 0;
    std::int64_t x2 = 0;
    std::int64_t y2 = 0;

    bool operator<(const SegmentKey& other) const
    {
        if (x1 != other.x1)
        {
            return x1 < other.x1;
        }
        if (y1 != other.y1)
        {
            return y1 < other.y1;
        }
        if (x2 != other.x2)
        {
            return x2 < other.x2;
        }
        return y2 < other.y2;
    }
};

void set_status(Status* status, int code, const std::string& message)
{
    if (status == nullptr)
    {
        return;
    }
    status->code = code;
    status->message = message;
}

std::int64_t pow10_int(int digits)
{
    std::int64_t value = 1;
    for (int i = 0; i < digits; ++i)
    {
        value *= 10;
    }
    return value;
}

std::int64_t snap(double value, std::int64_t scale)
{
    return static_cast<std::int64_t>(std::llround(value * static_cast<double>(scale)));
}

double unsnap(std::int64_t value, std::int64_t scale)
{
    return static_cast<double>(value) / static_cast<double>(scale);
}

std::string fnv1a64_hex(const unsigned char* data, std::size_t size)
{
    std::uint64_t hash = 14695981039346656037ull;
    for (std::size_t i = 0; i < size; ++i)
    {
        hash ^= static_cast<std::uint64_t>(data[i]);
        hash *= 1099511628211ull;
    }

    std::ostringstream out;
    out << "fnv1a64:" << std::hex << std::setfill('0') << std::setw(16) << hash;
    return out.str();
}

std::vector<ProjectionViewSpec> effective_views(const HlrProjectionOptions& options)
{
    if (!options.views.empty())
    {
        return options.views;
    }

    ProjectionViewSpec top;
    top.id = "top";
    top.direction = {0.0, 0.0, 1.0};
    top.up = {0.0, 1.0, 0.0};

    ProjectionViewSpec bottom;
    bottom.id = "bottom";
    bottom.direction = {0.0, 0.0, -1.0};
    bottom.up = {0.0, 1.0, 0.0};

    return {top, bottom};
}

TopoDS_Shape read_step_shape_from_bytes(const unsigned char* step_data, std::size_t step_size,
                                        bool strip_root_placement, Status* status)
{
    std::string step_text(reinterpret_cast<const char*>(step_data), step_size);
    std::istringstream step_stream(step_text);

    if (strip_root_placement)
    {
        const Handle(TDocStd_Document) document =
            new TDocStd_Document(TCollection_ExtendedString("XmlOcaf"));
        STEPCAFControl_Reader reader;
        const IFSelect_ReturnStatus read_status =
            reader.ChangeReader().ReadStream("memory.step", step_stream);
        if (read_status != IFSelect_RetDone)
        {
            set_status(status, 4, "Failed reading STEP bytes.");
            return TopoDS_Shape();
        }
        if (!reader.Transfer(document))
        {
            set_status(status, 5, "Failed transferring STEP roots.");
            return TopoDS_Shape();
        }
        strip_free_shape_root_locations(document);
        const TopoDS_Shape shape = free_shape_compound(document);
        if (shape.IsNull())
        {
            set_status(status, 6, "STEP transfer produced a null shape.");
        }
        return shape;
    }

    STEPControl_Reader reader;
    const IFSelect_ReturnStatus read_status = reader.ReadStream("memory.step", step_stream);
    if (read_status != IFSelect_RetDone)
    {
        set_status(status, 4, "Failed reading STEP bytes.");
        return TopoDS_Shape();
    }

    if (reader.TransferRoots() <= 0)
    {
        set_status(status, 5, "Failed transferring STEP roots.");
        return TopoDS_Shape();
    }

    TopoDS_Shape shape = reader.OneShape();
    if (shape.IsNull())
    {
        set_status(status, 6, "STEP transfer produced a null shape.");
        return TopoDS_Shape();
    }

    return shape;
}

bool validate_model_transform(const std::array<double, 16>& transform, std::string* error)
{
    for (double value : transform)
    {
        if (!std::isfinite(value))
        {
            *error = "Projection model_transform must contain only finite numbers.";
            return false;
        }
    }

    constexpr double tol = 1.0e-12;
    if (std::fabs(transform[12]) > tol || std::fabs(transform[13]) > tol ||
        std::fabs(transform[14]) > tol || std::fabs(transform[15] - 1.0) > tol)
    {
        *error = "Projection model_transform final row must be [0, 0, 0, 1].";
        return false;
    }

    gp_GTrsf gtrsf(gp_Mat(transform[0], transform[1], transform[2], transform[4], transform[5],
                          transform[6], transform[8], transform[9], transform[10]),
                   gp_XYZ(transform[3], transform[7], transform[11]));
    if (gtrsf.IsSingular())
    {
        *error = "Projection model_transform must not be singular.";
        return false;
    }

    return true;
}

bool is_identity_transform(const std::array<double, 16>& transform)
{
    static const std::array<double, 16> identity = {
        1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0,
    };
    constexpr double tol = 1.0e-12;
    for (std::size_t i = 0; i < transform.size(); ++i)
    {
        if (std::fabs(transform[i] - identity[i]) > tol)
        {
            return false;
        }
    }
    return true;
}

double dot3(double ax, double ay, double az, double bx, double by, double bz)
{
    return (ax * bx) + (ay * by) + (az * bz);
}

bool is_trsf_compatible_transform(const std::array<double, 16>& transform)
{
    const double c0_len2 =
        dot3(transform[0], transform[4], transform[8], transform[0], transform[4], transform[8]);
    const double c1_len2 =
        dot3(transform[1], transform[5], transform[9], transform[1], transform[5], transform[9]);
    const double c2_len2 =
        dot3(transform[2], transform[6], transform[10], transform[2], transform[6], transform[10]);
    constexpr double tol = 1.0e-10;
    if (c0_len2 <= tol || c1_len2 <= tol || c2_len2 <= tol)
    {
        return false;
    }
    if (std::fabs(c0_len2 - c1_len2) > tol || std::fabs(c0_len2 - c2_len2) > tol)
    {
        return false;
    }
    return std::fabs(dot3(transform[0], transform[4], transform[8], transform[1], transform[5],
                          transform[9])) <= tol &&
           std::fabs(dot3(transform[0], transform[4], transform[8], transform[2], transform[6],
                          transform[10])) <= tol &&
           std::fabs(dot3(transform[1], transform[5], transform[9], transform[2], transform[6],
                          transform[10])) <= tol;
}

TopoDS_Shape apply_model_transform(const TopoDS_Shape& shape,
                                   const std::array<double, 16>& transform)
{
    if (is_identity_transform(transform))
    {
        return shape;
    }

    if (is_trsf_compatible_transform(transform))
    {
        gp_Trsf trsf;
        trsf.SetValues(transform[0], transform[1], transform[2], transform[3], transform[4],
                       transform[5], transform[6], transform[7], transform[8], transform[9],
                       transform[10], transform[11]);
        return BRepBuilderAPI_Transform(shape, trsf, true).Shape();
    }

    gp_GTrsf gtrsf(gp_Mat(transform[0], transform[1], transform[2], transform[4], transform[5],
                          transform[6], transform[8], transform[9], transform[10]),
                   gp_XYZ(transform[3], transform[7], transform[11]));
    return BRepBuilderAPI_GTransform(shape, gtrsf, true).Shape();
}

gp_Ax2 make_view_axes(const ProjectionViewSpec& view)
{
    gp_Ax2 axes(gp_Pnt(0.0, 0.0, 0.0),
                gp_Dir(view.direction[0], view.direction[1], view.direction[2]));
    axes.SetYDirection(gp_Dir(view.up[0], view.up[1], view.up[2]));
    return axes;
}

SegmentKey make_segment_key(const ProjectedSegment& segment, std::int64_t scale)
{
    SegmentKey key{
        snap(segment.x1, scale),
        snap(segment.y1, scale),
        snap(segment.x2, scale),
        snap(segment.y2, scale),
    };

    if (key.x1 > key.x2 || (key.x1 == key.x2 && key.y1 > key.y2))
    {
        std::swap(key.x1, key.x2);
        std::swap(key.y1, key.y2);
    }

    return key;
}

ProjectedSegment segment_from_key(const SegmentKey& key, std::int64_t scale)
{
    return {
        unsnap(key.x1, scale),
        unsnap(key.y1, scale),
        unsnap(key.x2, scale),
        unsnap(key.y2, scale),
    };
}

void add_segment(std::set<SegmentKey>* keys, const ProjectedSegment& segment, std::int64_t scale)
{
    const SegmentKey key = make_segment_key(segment, scale);
    if (key.x1 == key.x2 && key.y1 == key.y2)
    {
        return;
    }
    keys->insert(key);
}

ProjectedModeGeometry geometry_from_keys(const std::set<SegmentKey>& segment_keys,
                                         std::int64_t scale)
{
    ProjectedModeGeometry geometry;
    geometry.segments.reserve(segment_keys.size());
    for (const SegmentKey& key : segment_keys)
    {
        geometry.segments.push_back(segment_from_key(key, scale));
    }

    return geometry;
}

double floor_to_scale(double value, std::int64_t scale)
{
    return std::floor(value * static_cast<double>(scale)) / static_cast<double>(scale);
}

double ceil_to_scale(double value, std::int64_t scale)
{
    return std::ceil(value * static_cast<double>(scale)) / static_cast<double>(scale);
}

ProjectedModeGeometry projected_shape_bbox_geometry(const TopoDS_Shape& shape,
                                                    const ProjectionViewSpec& view,
                                                    std::int64_t scale)
{
    ProjectedModeGeometry geometry;
    if (shape.IsNull())
    {
        return geometry;
    }

    Bnd_Box box;
    BRepBndLib::Add(shape, box, false);
    if (box.IsVoid())
    {
        return geometry;
    }

    double xmin = 0.0;
    double ymin = 0.0;
    double zmin = 0.0;
    double xmax = 0.0;
    double ymax = 0.0;
    double zmax = 0.0;
    box.Get(xmin, ymin, zmin, xmax, ymax, zmax);

    const gp_Ax2 axes = make_view_axes(view);
    const gp_XYZ x_dir = axes.XDirection().XYZ();
    const gp_XYZ y_dir = axes.YDirection().XYZ();

    double min_x = std::numeric_limits<double>::infinity();
    double min_y = std::numeric_limits<double>::infinity();
    double max_x = -std::numeric_limits<double>::infinity();
    double max_y = -std::numeric_limits<double>::infinity();

    const double xs[2] = {xmin, xmax};
    const double ys[2] = {ymin, ymax};
    const double zs[2] = {zmin, zmax};
    for (double x : xs)
    {
        for (double y : ys)
        {
            for (double z : zs)
            {
                const gp_XYZ point = gp_Pnt(x, y, z).XYZ();
                const double px = point.Dot(x_dir);
                const double py = point.Dot(y_dir);
                min_x = std::min(min_x, px);
                min_y = std::min(min_y, py);
                max_x = std::max(max_x, px);
                max_y = std::max(max_y, py);
            }
        }
    }

    if (!std::isfinite(min_x) || !std::isfinite(min_y) || !std::isfinite(max_x) ||
        !std::isfinite(max_y) || min_x > max_x || min_y > max_y)
    {
        return geometry;
    }

    min_x = floor_to_scale(min_x, scale);
    min_y = floor_to_scale(min_y, scale);
    max_x = ceil_to_scale(max_x, scale);
    max_y = ceil_to_scale(max_y, scale);
    geometry.segments = {
        {min_x, min_y, max_x, min_y},
        {max_x, min_y, max_x, max_y},
        {max_x, max_y, min_x, max_y},
        {min_x, max_y, min_x, min_y},
    };
    return geometry;
}

double elapsed_ms(const std::chrono::high_resolution_clock::time_point& start)
{
    const auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(now - start).count();
}

} // namespace

int step_hlr_projection_from_bytes(const unsigned char* step_data, std::size_t step_size,
                                   const HlrProjectionOptions& options, HlrProjectionResult* result,
                                   Status* status)
{
    if (step_data == nullptr || step_size == 0)
    {
        set_status(status, 1, "STEP input buffer is empty.");
        return 1;
    }
    if (result == nullptr)
    {
        set_status(status, 2, "Projection result pointer is null.");
        return 2;
    }
    if (options.round_digits < 0 || options.round_digits > 9)
    {
        set_status(status, 3, "Projection round_digits must be between 0 and 9.");
        return 3;
    }
    std::string transform_error;
    if (!validate_model_transform(options.model_transform, &transform_error))
    {
        set_status(status, 3, transform_error);
        return 3;
    }

    try
    {
        HlrProjectionTimings timings;

        const auto read_start = std::chrono::high_resolution_clock::now();
        TopoDS_Shape shape =
            read_step_shape_from_bytes(step_data, step_size, options.strip_root_placement, status);
        if (shape.IsNull())
        {
            return status == nullptr ? 6 : status->code;
        }
        timings.step_read_ms = elapsed_ms(read_start);
        shape = apply_model_transform(shape, options.model_transform);
        if (shape.IsNull())
        {
            set_status(status, 8, "Projection model_transform produced a null shape.");
            return 8;
        }

        HlrProjectionResult output;
        output.schema = "geometry.projection.b0";
        output.units = "mm";
        output.source_hash = fnv1a64_hex(step_data, step_size);

        const std::int64_t scale = pow10_int(options.round_digits);
        const std::vector<ProjectionViewSpec> views = effective_views(options);
        output.views.reserve(views.size());

        const bool needs_fast_detail = options.output_detail;
        const bool needs_fast_outline =
            options.output_outline &&
            options.outline_algorithm == ProjectionOutlineAlgorithm::FastMeshShadow;
        if (needs_fast_detail || options.output_outline)
        {
            const auto mesh_start = std::chrono::high_resolution_clock::now();
            double linear_deflection = options.mesh_linear_deflection;
            bool relative = options.mesh_relative;
            if (options.mesh_deflection_mode == MeshDeflectionMode::BboxRelative)
            {
                // Match Altium Draftsman: deflection = maxBBoxDim * coefficient,
                // applied as an absolute chordal tolerance (Prs3d::GetDeflection).
                Bnd_Box box;
                BRepBndLib::Add(shape, box, false);
                if (!box.IsVoid())
                {
                    double xmin, ymin, zmin, xmax, ymax, zmax;
                    box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
                    const double max_dim =
                        std::max(xmax - xmin, std::max(ymax - ymin, zmax - zmin));
                    if (max_dim > 0.0)
                    {
                        linear_deflection = max_dim * options.mesh_deflection_coefficient;
                        relative = false;
                    }
                }
            }
            BRepMesh_IncrementalMesh mesher(shape, linear_deflection, relative,
                                            options.mesh_angular_deflection,
                                            /*isInParallel=*/true);
            mesher.Perform();
            timings.mesh_ms = elapsed_ms(mesh_start);
        }

        FastHlrPreparedMesh fast_prepared;
        if (needs_fast_detail || needs_fast_outline)
        {
            const auto prepare_start = std::chrono::high_resolution_clock::now();
            const int fast_code =
                prepare_fast_hlr_shape(shape, options.fast, &fast_prepared, status);
            if (fast_code != 0)
            {
                return fast_code;
            }
            timings.mesh_ms += elapsed_ms(prepare_start);
        }

        for (const ProjectionViewSpec& view : views)
        {
            ProjectedViewGeometry projected_view;
            projected_view.view = view;
            if (needs_fast_detail)
            {
                const auto fast_start = std::chrono::high_resolution_clock::now();
                ProjectedModeGeometry hidden_geometry;
                const int fast_code = project_fast_hlr_detail(
                    fast_prepared, view, options.fast, &projected_view.detail,
                    options.fast.include_hidden ? &hidden_geometry : nullptr, nullptr, status);
                if (fast_code != 0)
                {
                    return fast_code;
                }
                if (options.fast.include_hidden)
                {
                    projected_view.detail.segments.insert(projected_view.detail.segments.end(),
                                                          hidden_geometry.segments.begin(),
                                                          hidden_geometry.segments.end());
                }
                std::set<SegmentKey> fast_segment_keys;
                for (const ProjectedSegment& segment : projected_view.detail.segments)
                {
                    add_segment(&fast_segment_keys, segment, scale);
                }
                projected_view.detail = geometry_from_keys(fast_segment_keys, scale);
                timings.hlr_ms += elapsed_ms(fast_start);
            }
            if (options.output_outline &&
                options.outline_algorithm == ProjectionOutlineAlgorithm::MeshShadow)
            {
                const auto outline_start = std::chrono::high_resolution_clock::now();
                projected_view.outline = mesh_shadow_outline_geometry(shape, view, options, scale);
                timings.extract_ms += elapsed_ms(outline_start);
            }
            else if (needs_fast_outline)
            {
                const auto outline_start = std::chrono::high_resolution_clock::now();
                const int outline_code =
                    fast_mesh_shadow_outline_geometry(fast_prepared, view, options.fast, scale,
                                                      &projected_view.outline, nullptr, status);
                if (outline_code != 0)
                {
                    return outline_code;
                }
                timings.extract_ms += elapsed_ms(outline_start);
            }
            if (options.output_bbox)
            {
                projected_view.bbox = projected_shape_bbox_geometry(shape, view, scale);
            }
            output.views.push_back(std::move(projected_view));
        }

        output.timings = timings;

        *result = std::move(output);
        set_status(status, 0, "");
        return 0;
    }
    catch (const Standard_Failure& failure)
    {
        set_status(status, 7, failure.what());
        return 7;
    }
    catch (const std::exception& error)
    {
        set_status(status, 8, error.what());
        return 8;
    }
}

} // namespace geometer

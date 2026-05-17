#include "geometer/projection.h"

#include "geometer/planar_contours.h"

#include <BRepAdaptor_Curve.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <GeomAbs_CurveType.hxx>
#include <HLRAlgo_Projector.hxx>
#include <HLRBRep_Algo.hxx>
#include <HLRBRep_HLRToShape.hxx>
#include <HLRBRep_PolyAlgo.hxx>
#include <HLRBRep_PolyHLRToShape.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <STEPControl_Reader.hxx>
#include <Standard_Failure.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

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

constexpr double kPi = 3.14159265358979323846264338327950288;

struct SegmentKey
{
    long long x1 = 0;
    long long y1 = 0;
    long long x2 = 0;
    long long y2 = 0;

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

struct ArcKey
{
    bool full_circle = false;
    long long sx = 0;
    long long sy = 0;
    long long ex = 0;
    long long ey = 0;
    long long cx = 0;
    long long cy = 0;
    long long radius = 0;
    long long extent = 0;
    bool ccw = true;

    bool operator<(const ArcKey& other) const
    {
        if (full_circle != other.full_circle)
        {
            return full_circle < other.full_circle;
        }
        if (sx != other.sx)
        {
            return sx < other.sx;
        }
        if (sy != other.sy)
        {
            return sy < other.sy;
        }
        if (ex != other.ex)
        {
            return ex < other.ex;
        }
        if (ey != other.ey)
        {
            return ey < other.ey;
        }
        if (cx != other.cx)
        {
            return cx < other.cx;
        }
        if (cy != other.cy)
        {
            return cy < other.cy;
        }
        if (radius != other.radius)
        {
            return radius < other.radius;
        }
        if (extent != other.extent)
        {
            return extent < other.extent;
        }
        return ccw < other.ccw;
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

long long pow10_int(int digits)
{
    long long value = 1;
    for (int i = 0; i < digits; ++i)
    {
        value *= 10;
    }
    return value;
}

long long snap(double value, long long scale)
{
    return static_cast<long long>(std::llround(value * static_cast<double>(scale)));
}

double unsnap(long long value, long long scale)
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
                                        Status* status)
{
    std::string step_text(reinterpret_cast<const char*>(step_data), step_size);
    std::istringstream step_stream(step_text);

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

gp_Ax2 make_view_axes(const ProjectionViewSpec& view)
{
    gp_Ax2 axes(gp_Pnt(0.0, 0.0, 0.0),
                gp_Dir(view.direction[0], view.direction[1], view.direction[2]));
    axes.SetYDirection(gp_Dir(view.up[0], view.up[1], view.up[2]));
    return axes;
}

SegmentKey make_segment_key(const ProjectedSegment& segment, long long scale)
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

ProjectedSegment segment_from_key(const SegmentKey& key, long long scale)
{
    return {
        unsnap(key.x1, scale),
        unsnap(key.y1, scale),
        unsnap(key.x2, scale),
        unsnap(key.y2, scale),
    };
}

void add_segment(std::set<SegmentKey>* keys, const ProjectedSegment& segment, long long scale)
{
    const SegmentKey key = make_segment_key(segment, scale);
    if (key.x1 == key.x2 && key.y1 == key.y2)
    {
        return;
    }
    keys->insert(key);
}

ArcKey make_arc_key(const ProjectedArc& arc, long long scale, long long extent_scale)
{
    ArcKey key;
    key.full_circle = arc.full_circle;
    key.sx = snap(arc.start[0], scale);
    key.sy = snap(arc.start[1], scale);
    key.ex = snap(arc.end[0], scale);
    key.ey = snap(arc.end[1], scale);
    key.cx = snap(arc.center[0], scale);
    key.cy = snap(arc.center[1], scale);
    key.radius = snap(arc.radius, scale);
    key.extent = snap(arc.extent_rad, extent_scale);
    key.ccw = arc.ccw;
    if (key.full_circle)
    {
        key.sx = 0;
        key.sy = 0;
        key.ex = 0;
        key.ey = 0;
        key.extent = snap(2.0 * kPi, extent_scale);
        key.ccw = true;
    }
    return key;
}

ProjectedArc arc_from_key(const ArcKey& key, long long scale, long long extent_scale)
{
    ProjectedArc arc;
    arc.start = {unsnap(key.sx, scale), unsnap(key.sy, scale)};
    arc.end = {unsnap(key.ex, scale), unsnap(key.ey, scale)};
    arc.center = {unsnap(key.cx, scale), unsnap(key.cy, scale)};
    arc.radius = unsnap(key.radius, scale);
    arc.extent_rad = unsnap(key.extent, extent_scale);
    arc.ccw = key.ccw;
    arc.full_circle = key.full_circle;
    return arc;
}

void add_arc(std::set<ArcKey>* keys, const ProjectedArc& arc, long long scale,
             long long extent_scale)
{
    if (!std::isfinite(arc.radius) || arc.radius <= 0.0)
    {
        return;
    }
    keys->insert(make_arc_key(arc, scale, extent_scale));
}

ProjectedArc circle_arc_from_adaptor(const BRepAdaptor_Curve& adaptor, double first, double last)
{
    const gp_Circ circle = adaptor.Circle();
    const gp_Pnt center = circle.Location();
    const gp_Pnt start_point = adaptor.Value(first);
    const gp_Pnt end_point = adaptor.Value(last);

    ProjectedArc arc;
    arc.start = {start_point.X(), start_point.Y()};
    arc.end = {end_point.X(), end_point.Y()};
    arc.center = {center.X(), center.Y()};
    arc.radius = circle.Radius();
    arc.ccw = (last - first) >= 0.0;

    const double raw_extent = last - first;
    double extent = std::fmod(std::fabs(raw_extent), 2.0 * kPi);
    if (extent < 1.0e-9 && std::fabs(raw_extent) > 1.0e-6)
    {
        extent = 2.0 * kPi;
    }

    const bool points_close = std::fabs(arc.start[0] - arc.end[0]) <= 1.0e-7 &&
                              std::fabs(arc.start[1] - arc.end[1]) <= 1.0e-7;
    arc.full_circle =
        extent >= ((2.0 * kPi) - 1.0e-5) || (points_close && std::fabs(raw_extent) > 1.0e-6);
    arc.extent_rad = arc.full_circle ? (2.0 * kPi) : extent;
    return arc;
}

std::vector<ProjectedSegment> arc_to_segments(const ProjectedArc& arc, int samples_per_curve)
{
    const int sample_count = std::max(samples_per_curve, 2);
    double signed_extent = arc.extent_rad;
    if (!arc.ccw)
    {
        signed_extent = -signed_extent;
    }

    const double start_angle =
        std::atan2(arc.start[1] - arc.center[1], arc.start[0] - arc.center[0]);
    const double angle_step = signed_extent / static_cast<double>(sample_count - 1);

    std::vector<std::array<double, 2>> points;
    points.reserve(static_cast<std::size_t>(sample_count));
    for (int i = 0; i < sample_count; ++i)
    {
        if (!arc.full_circle && i == 0)
        {
            points.push_back(arc.start);
            continue;
        }
        if (!arc.full_circle && i == sample_count - 1)
        {
            points.push_back(arc.end);
            continue;
        }

        const double angle = start_angle + (angle_step * static_cast<double>(i));
        points.push_back({
            arc.center[0] + (arc.radius * std::cos(angle)),
            arc.center[1] + (arc.radius * std::sin(angle)),
        });
    }

    std::vector<ProjectedSegment> result;
    if (points.size() < 2)
    {
        return result;
    }
    result.reserve(points.size() - 1);
    for (std::size_t i = 1; i < points.size(); ++i)
    {
        result.push_back({points[i - 1][0], points[i - 1][1], points[i][0], points[i][1]});
    }
    return result;
}

void add_edge_geometry(const TopoDS_Shape& shape, const HlrProjectionOptions& options,
                       std::set<SegmentKey>* segment_keys, std::set<ArcKey>* arc_keys,
                       std::vector<ProjectedSegment>* contour_source_segments, long long scale,
                       long long extent_scale)
{
    if (shape.IsNull())
    {
        return;
    }

    const int samples_per_curve = std::max(options.samples_per_curve, 2);
    for (TopExp_Explorer explorer(shape, TopAbs_EDGE); explorer.More(); explorer.Next())
    {
        const TopoDS_Edge& edge = TopoDS::Edge(explorer.Current());
        BRepAdaptor_Curve adaptor(edge);
        const double first = adaptor.FirstParameter();
        const double last = adaptor.LastParameter();
        if (!std::isfinite(first) || !std::isfinite(last))
        {
            continue;
        }

        const GeomAbs_CurveType curve_type = adaptor.GetType();
        if (curve_type == GeomAbs_Line)
        {
            const gp_Pnt start = adaptor.Value(first);
            const gp_Pnt end = adaptor.Value(last);
            const ProjectedSegment segment{start.X(), start.Y(), end.X(), end.Y()};
            add_segment(segment_keys, segment, scale);
            contour_source_segments->push_back(
                segment_from_key(make_segment_key(segment, scale), scale));
            continue;
        }

        if (curve_type == GeomAbs_Circle && options.curve_mode == ProjectionCurveMode::NativeArcs)
        {
            const ProjectedArc arc = circle_arc_from_adaptor(adaptor, first, last);
            add_arc(arc_keys, arc, scale, extent_scale);
            for (const ProjectedSegment& segment : arc_to_segments(arc, samples_per_curve))
            {
                contour_source_segments->push_back(
                    segment_from_key(make_segment_key(segment, scale), scale));
            }
            continue;
        }

        ProjectedSegment previous;
        bool have_previous = false;
        const double step = (last - first) / static_cast<double>(samples_per_curve - 1);
        for (int sample = 0; sample < samples_per_curve; ++sample)
        {
            const double parameter = first + (step * static_cast<double>(sample));
            const gp_Pnt point = adaptor.Value(parameter);
            if (have_previous)
            {
                ProjectedSegment segment{previous.x2, previous.y2, point.X(), point.Y()};
                add_segment(segment_keys, segment, scale);
                contour_source_segments->push_back(
                    segment_from_key(make_segment_key(segment, scale), scale));
            }
            previous = {point.X(), point.Y(), point.X(), point.Y()};
            have_previous = true;
        }
    }
}

ProjectedModeGeometry geometry_from_keys(const std::set<SegmentKey>& segment_keys,
                                         const std::set<ArcKey>& arc_keys, long long scale,
                                         long long extent_scale)
{
    ProjectedModeGeometry geometry;
    geometry.segments.reserve(segment_keys.size());
    for (const SegmentKey& key : segment_keys)
    {
        geometry.segments.push_back(segment_from_key(key, scale));
    }

    geometry.arcs.reserve(arc_keys.size());
    for (const ArcKey& key : arc_keys)
    {
        geometry.arcs.push_back(arc_from_key(key, scale, extent_scale));
    }
    return geometry;
}

ProjectedModeGeometry
simple_geometry_from_segments(const std::vector<ProjectedSegment>& contour_source_segments,
                              const HlrProjectionOptions& options, long long scale)
{
    std::vector<PlanarContourSegment> contour_segments;
    contour_segments.reserve(contour_source_segments.size());
    for (const ProjectedSegment& segment : contour_source_segments)
    {
        contour_segments.push_back({{segment.x1, segment.y1}, {segment.x2, segment.y2}});
    }

    PlanarContourOptions contour_options;
    contour_options.round_digits = options.round_digits;
    contour_options.union_polygons = options.union_simple_polygons;

    PlanarContourResult contour_result;
    Status contour_status;
    const int contour_code =
        build_planar_contours(contour_segments, contour_options, &contour_result, &contour_status);

    std::set<SegmentKey> simple_keys;
    if (contour_code == 0 && !contour_result.segments.empty())
    {
        for (const PlanarContourSegment& segment : contour_result.segments)
        {
            add_segment(&simple_keys,
                        {segment.start.x, segment.start.y, segment.end.x, segment.end.y}, scale);
        }
    }
    else
    {
        for (const ProjectedSegment& segment : contour_source_segments)
        {
            add_segment(&simple_keys, segment, scale);
        }
    }

    return geometry_from_keys(simple_keys, std::set<ArcKey>(), scale, pow10_int(6));
}

double elapsed_ms(const std::chrono::high_resolution_clock::time_point& start)
{
    const auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(now - start).count();
}

ProjectedViewGeometry project_view_exact(const TopoDS_Shape& shape, const ProjectionViewSpec& view,
                                         const HlrProjectionOptions& options, long long scale,
                                         long long extent_scale,
                                         HlrProjectionTimings* timings)
{
    const auto hlr_start = std::chrono::high_resolution_clock::now();

    Handle(HLRBRep_Algo) algorithm = new HLRBRep_Algo();
    algorithm->Add(shape);
    algorithm->Projector(HLRAlgo_Projector(make_view_axes(view)));
    algorithm->Update();
    algorithm->Hide();

    HLRBRep_HLRToShape hlr_to_shape(algorithm);

    if (timings != nullptr)
    {
        timings->hlr_ms += elapsed_ms(hlr_start);
    }

    const auto extract_start = std::chrono::high_resolution_clock::now();

    std::set<SegmentKey> detail_segment_keys;
    std::set<ArcKey> detail_arc_keys;
    std::vector<ProjectedSegment> contour_source_segments;

    // Pull each enabled edge category from HLRBRep_HLRToShape and merge.
    auto extract = [&](bool enabled, TopoDS_Shape shape)
    {
        if (!enabled || shape.IsNull()) return;
        add_edge_geometry(shape, options, &detail_segment_keys, &detail_arc_keys,
                          &contour_source_segments, scale, extent_scale);
    };
    extract(options.edge_v_sharp,   hlr_to_shape.VCompound());
    extract(options.edge_v_outline, hlr_to_shape.OutLineVCompound());
    extract(options.edge_v_smooth,  hlr_to_shape.Rg1LineVCompound());
    extract(options.edge_v_sewn,    hlr_to_shape.RgNLineVCompound());
    extract(options.edge_v_iso,     hlr_to_shape.IsoLineVCompound());
    extract(options.edge_h_sharp,   hlr_to_shape.HCompound());
    extract(options.edge_h_outline, hlr_to_shape.OutLineHCompound());
    extract(options.edge_h_smooth,  hlr_to_shape.Rg1LineHCompound());
    extract(options.edge_h_sewn,    hlr_to_shape.RgNLineHCompound());
    extract(options.edge_h_iso,     hlr_to_shape.IsoLineHCompound());

    ProjectedViewGeometry projected;
    projected.view = view;
    projected.detail =
        geometry_from_keys(detail_segment_keys, detail_arc_keys, scale, extent_scale);
    projected.simple = simple_geometry_from_segments(contour_source_segments, options, scale);

    if (timings != nullptr)
    {
        timings->extract_ms += elapsed_ms(extract_start);
    }
    return projected;
}

ProjectedViewGeometry project_view_poly(const TopoDS_Shape& shape, const ProjectionViewSpec& view,
                                        const HlrProjectionOptions& options, long long scale,
                                        long long extent_scale, HlrProjectionTimings* timings)
{
    const auto hlr_start = std::chrono::high_resolution_clock::now();

    Handle(HLRBRep_PolyAlgo) algorithm = new HLRBRep_PolyAlgo();
    algorithm->Load(shape);
    algorithm->Projector(HLRAlgo_Projector(make_view_axes(view)));
    if (options.hlr_angle_tolerance > 0.0)
    {
        // PolyAlgo uses TolAngular() for the silhouette/sharp-edge classification
        // tolerance (radians). HLRBRep_Algo uses Angle() with the same intent.
        algorithm->TolAngular(options.hlr_angle_tolerance);
    }
    algorithm->Update();

    HLRBRep_PolyHLRToShape poly_to_shape;
    poly_to_shape.Update(algorithm);

    if (timings != nullptr)
    {
        timings->hlr_ms += elapsed_ms(hlr_start);
    }

    const auto extract_start = std::chrono::high_resolution_clock::now();

    // PolyAlgo emits tessellated line segments; force polyline treatment so we
    // do not try to interpret any residual edge type as an analytic arc.
    HlrProjectionOptions poly_options = options;
    poly_options.curve_mode = ProjectionCurveMode::Polyline;

    std::set<SegmentKey> detail_segment_keys;
    std::set<ArcKey> detail_arc_keys;
    std::vector<ProjectedSegment> contour_source_segments;

    // HLRBRep_PolyHLRToShape only exposes V/H Compound + OutLine variants.
    // The smooth/sewn/iso flags are accepted but silently ignored in poly mode.
    auto extract = [&](bool enabled, TopoDS_Shape shape)
    {
        if (!enabled || shape.IsNull()) return;
        add_edge_geometry(shape, poly_options, &detail_segment_keys, &detail_arc_keys,
                          &contour_source_segments, scale, extent_scale);
    };
    extract(poly_options.edge_v_sharp,   poly_to_shape.VCompound());
    extract(poly_options.edge_v_outline, poly_to_shape.OutLineVCompound());
    extract(poly_options.edge_h_sharp,   poly_to_shape.HCompound());
    extract(poly_options.edge_h_outline, poly_to_shape.OutLineHCompound());

    ProjectedViewGeometry projected;
    projected.view = view;
    projected.detail =
        geometry_from_keys(detail_segment_keys, detail_arc_keys, scale, extent_scale);
    projected.simple = simple_geometry_from_segments(contour_source_segments, poly_options, scale);

    if (timings != nullptr)
    {
        timings->extract_ms += elapsed_ms(extract_start);
    }
    return projected;
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

    try
    {
        HlrProjectionTimings timings;

        const auto read_start = std::chrono::high_resolution_clock::now();
        TopoDS_Shape shape = read_step_shape_from_bytes(step_data, step_size, status);
        if (shape.IsNull())
        {
            return status == nullptr ? 6 : status->code;
        }
        timings.step_read_ms = elapsed_ms(read_start);

        HlrProjectionResult output;
        output.schema = "geometry.projection.a0";
        output.units = "mm";
        output.source_hash = fnv1a64_hex(step_data, step_size);

        const long long scale = pow10_int(options.round_digits);
        const long long extent_scale = pow10_int(std::max(options.round_digits, 6));
        const std::vector<ProjectionViewSpec> views = effective_views(options);
        output.views.reserve(views.size());

        const bool use_poly = options.projection_algorithm == ProjectionAlgorithm::Poly;
        if (use_poly)
        {
            const auto mesh_start = std::chrono::high_resolution_clock::now();
            BRepMesh_IncrementalMesh mesher(shape, options.mesh_linear_deflection,
                                            options.mesh_relative,
                                            options.mesh_angular_deflection,
                                            /*isInParallel=*/true);
            mesher.Perform();
            timings.mesh_ms = elapsed_ms(mesh_start);
        }

        for (const ProjectionViewSpec& view : views)
        {
            if (use_poly)
            {
                output.views.push_back(
                    project_view_poly(shape, view, options, scale, extent_scale, &timings));
            }
            else
            {
                output.views.push_back(
                    project_view_exact(shape, view, options, scale, extent_scale, &timings));
            }
        }

        output.timings = timings;

        *result = std::move(output);
        set_status(status, 0, "");
        return 0;
    }
    catch (const Standard_Failure& failure)
    {
        set_status(status, 7, failure.GetMessageString());
        return 7;
    }
    catch (const std::exception& error)
    {
        set_status(status, 8, error.what());
        return 8;
    }
}

} // namespace geometer

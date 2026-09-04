#include "geometer/projection.h"

#include "fast_hlr_occt.h"
#include "fast_mesh_shadow_outline.h"
#include "geometer/planar_contours.h"
#include "geometer/planar_solve.h"
#include "mesh_shadow_outline.h"
#include "step_xcaf_root_frame.h"

#include <clipper2/clipper.h>

#include <BRepAdaptor_Curve.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_GTransform.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <Bnd_Box.hxx>
#include <GeomAbs_CurveType.hxx>
#include <HLRAlgo_Projector.hxx>
#include <HLRBRep_Algo.hxx>
#include <HLRBRep_HLRToShape.hxx>
#include <HLRBRep_PolyAlgo.hxx>
#include <HLRBRep_PolyHLRToShape.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <STEPControl_Reader.hxx>
#include <Standard_Failure.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TDocStd_Document.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
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

constexpr double kPi = 3.14159265358979323846264338327950288;

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

struct ArcKey
{
    bool full_circle = false;
    std::int64_t sx = 0;
    std::int64_t sy = 0;
    std::int64_t ex = 0;
    std::int64_t ey = 0;
    std::int64_t cx = 0;
    std::int64_t cy = 0;
    std::int64_t radius = 0;
    std::int64_t extent = 0;
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

ArcKey make_arc_key(const ProjectedArc& arc, std::int64_t scale, std::int64_t extent_scale)
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

ProjectedArc arc_from_key(const ArcKey& key, std::int64_t scale, std::int64_t extent_scale)
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

void add_arc(std::set<ArcKey>* keys, const ProjectedArc& arc, std::int64_t scale,
             std::int64_t extent_scale)
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
                       std::vector<ProjectedSegment>* contour_source_segments, std::int64_t scale,
                       std::int64_t extent_scale)
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
            if (segment_keys != nullptr)
            {
                add_segment(segment_keys, segment, scale);
            }
            if (contour_source_segments != nullptr)
            {
                contour_source_segments->push_back(
                    segment_from_key(make_segment_key(segment, scale), scale));
            }
            continue;
        }

        if (curve_type == GeomAbs_Circle && options.curve_mode == ProjectionCurveMode::NativeArcs)
        {
            const ProjectedArc arc = circle_arc_from_adaptor(adaptor, first, last);
            if (arc_keys != nullptr)
            {
                add_arc(arc_keys, arc, scale, extent_scale);
            }
            if (contour_source_segments != nullptr)
            {
                for (const ProjectedSegment& segment : arc_to_segments(arc, samples_per_curve))
                {
                    contour_source_segments->push_back(
                        segment_from_key(make_segment_key(segment, scale), scale));
                }
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
                if (segment_keys != nullptr)
                {
                    add_segment(segment_keys, segment, scale);
                }
                if (contour_source_segments != nullptr)
                {
                    contour_source_segments->push_back(
                        segment_from_key(make_segment_key(segment, scale), scale));
                }
            }
            previous = {point.X(), point.Y(), point.X(), point.Y()};
            have_previous = true;
        }
    }
}

ProjectedModeGeometry geometry_from_keys(const std::set<SegmentKey>& segment_keys,
                                         const std::set<ArcKey>& arc_keys, std::int64_t scale,
                                         std::int64_t extent_scale)
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

void add_ring_segments(std::set<SegmentKey>* keys, const PlanarSolveRing& ring, std::int64_t scale)
{
    const std::size_t count = ring.size();
    if (count < 3)
    {
        return;
    }
    for (std::size_t i = 0; i < count; ++i)
    {
        const PlanarSolvePoint& start = ring[i];
        const PlanarSolvePoint& end = ring[(i + 1) % count];
        add_segment(keys, {start.x, start.y, end.x, end.y}, scale);
    }
}

// Walk a Clipper2 union PolyTree and collect only the OUTER contours of the
// dilated edge wireframe, dropping every hole. Each outer is therefore filled
// solid, so an enclosed region (the body interior) becomes solid. Recursion
// still descends through holes to pick up genuinely disconnected outer islands.
//
// NOTE: this fills ALL enclosed regions, so a part with a genuine through-hole
// would have that hole filled too. The parts in scope (TSOT/SOT23/SOT223) have
// no through-holes; preserving real through-holes needs a semantic signal the
// outline alone does not carry and is left as a follow-up.
void collect_silhouette_paths(const Clipper2Lib::PolyPathD& node, Clipper2Lib::PathsD* solids)
{
    using Clipper2Lib::PolyPathD;

    for (std::size_t i = 0; i < node.Count(); ++i)
    {
        const PolyPathD* child = node.Child(i);
        if (!child->IsHole())
        {
            solids->push_back(child->Polygon());
        }
        collect_silhouette_paths(*child, solids);
    }
}

// Build the outline from the projected segment soup, mirroring Draftsman's
// "union of polygons -> outer silhouette" with no interior detail.
//
// The HLR tessellation leaves micron-scale gaps between adjacent projected
// edges (corner splits, lead/body junctions), so an exact topological face
// trace cannot reliably close the outer loop. Instead we run a morphological
// close on the edge soup with Clipper2: dilate every edge into a thin band
// (square caps bridge the gaps), union and fill to recover the solid body, then
// erode by the same radius to restore the true size. Miter joins keep corners
// sharp. The close radius is sized off the bounding box because the gaps scale
// with the (bbox-relative) mesh deflection; it stays comfortably below real
// feature spacing, so only sub-gap detail is dissolved.
ProjectedModeGeometry
outline_geometry_from_segments(const std::vector<ProjectedSegment>& contour_source_segments,
                               const HlrProjectionOptions& options, std::int64_t scale)
{
    using Clipper2Lib::ClipperD;
    using Clipper2Lib::ClipType;
    using Clipper2Lib::EndType;
    using Clipper2Lib::FillRule;
    using Clipper2Lib::InflatePaths;
    using Clipper2Lib::JoinType;
    using Clipper2Lib::PathD;
    using Clipper2Lib::PathsD;
    using Clipper2Lib::PointD;
    using Clipper2Lib::PolyTreeD;
    using Clipper2Lib::SimplifyPaths;

    const auto empty_geometry = [&]()
    { return geometry_from_keys(std::set<SegmentKey>(), std::set<ArcKey>(), scale, pow10_int(6)); };

    PathsD edges;
    edges.reserve(contour_source_segments.size());
    bool have_bounds = false;
    double min_x = 0.0;
    double min_y = 0.0;
    double max_x = 0.0;
    double max_y = 0.0;
    for (const ProjectedSegment& segment : contour_source_segments)
    {
        if (segment.x1 == segment.x2 && segment.y1 == segment.y2)
        {
            continue;
        }
        edges.push_back(PathD{PointD(segment.x1, segment.y1), PointD(segment.x2, segment.y2)});
        const double xs[2] = {segment.x1, segment.x2};
        const double ys[2] = {segment.y1, segment.y2};
        for (int i = 0; i < 2; ++i)
        {
            if (!have_bounds)
            {
                min_x = max_x = xs[i];
                min_y = max_y = ys[i];
                have_bounds = true;
                continue;
            }
            min_x = std::min(min_x, xs[i]);
            max_x = std::max(max_x, xs[i]);
            min_y = std::min(min_y, ys[i]);
            max_y = std::max(max_y, ys[i]);
        }
    }

    if (edges.empty() || !have_bounds)
    {
        return empty_geometry();
    }

    if (!options.union_outline_polygons)
    {
        std::set<SegmentKey> raw_keys;
        for (const ProjectedSegment& segment : contour_source_segments)
        {
            add_segment(&raw_keys, segment, scale);
        }
        return geometry_from_keys(raw_keys, std::set<ArcKey>(), scale, pow10_int(6));
    }

    const double max_dim = std::max(max_x - min_x, max_y - min_y);
    if (max_dim <= 0.0)
    {
        return empty_geometry();
    }

    // Close radius: ~2x the gap scale (gaps ~= deflection ~= 0.004 * max_dim),
    // so 2*radius bridges the gaps but stays below real feature spacing
    // (real features are several deflections wide).
    const double radius = max_dim * 0.006;
    const int precision = 6; // sub-micron offset/clip grid
    const double miter_limit = 4.0;

    // 1) Dilate each open edge into a radius-fat band; square caps extend the
    //    band past each endpoint so adjacent edges overlap across the gaps.
    const PathsD fattened =
        InflatePaths(edges, radius, JoinType::Miter, EndType::Square, miter_limit, precision);
    if (fattened.empty())
    {
        return empty_geometry();
    }

    // 2) Union the bands into a solid region and walk the hierarchy: keep every
    //    outer contour (filled solid), drop all holes so the body interior fills.
    ClipperD clipper(precision);
    clipper.AddSubject(fattened);
    PolyTreeD tree;
    clipper.Execute(ClipType::Union, FillRule::NonZero, tree);
    PathsD solids;
    collect_silhouette_paths(tree, &solids);
    if (solids.empty())
    {
        return empty_geometry();
    }

    // 3) Erode by the same radius to restore the true silhouette size, then drop
    //    sub-micron offset noise.
    PathsD silhouette =
        InflatePaths(solids, -radius, JoinType::Miter, EndType::Polygon, miter_limit, precision);
    if (silhouette.empty())
    {
        return empty_geometry();
    }
    silhouette = SimplifyPaths(silhouette, max_dim * 1.0e-4, true);

    std::set<SegmentKey> outline_keys;
    for (const PathD& path : silhouette)
    {
        if (path.size() < 3)
        {
            continue;
        }
        PlanarSolveRing ring;
        ring.reserve(path.size());
        for (const PointD& point : path)
        {
            ring.push_back({point.x, point.y});
        }
        add_ring_segments(&outline_keys, ring, scale);
    }

    return geometry_from_keys(outline_keys, std::set<ArcKey>(), scale, pow10_int(6));
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

ProjectedViewGeometry project_view_exact(const TopoDS_Shape& shape, const ProjectionViewSpec& view,
                                         const HlrProjectionOptions& options, std::int64_t scale,
                                         std::int64_t extent_scale, HlrProjectionTimings* timings)
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

    // Detail respects the per-category checkboxes.
    auto extract_detail = [&](bool enabled, TopoDS_Shape shape)
    {
        if (!enabled || shape.IsNull())
            return;
        add_edge_geometry(shape, options, &detail_segment_keys, &detail_arc_keys, nullptr, scale,
                          extent_scale);
    };
    if (options.output_detail)
    {
        extract_detail(options.edge_v_sharp, hlr_to_shape.VCompound());
        extract_detail(options.edge_v_outline, hlr_to_shape.OutLineVCompound());
        extract_detail(options.edge_v_smooth, hlr_to_shape.Rg1LineVCompound());
        extract_detail(options.edge_v_sewn, hlr_to_shape.RgNLineVCompound());
        extract_detail(options.edge_v_iso, hlr_to_shape.IsoLineVCompound());
        extract_detail(options.edge_h_sharp, hlr_to_shape.HCompound());
        extract_detail(options.edge_h_outline, hlr_to_shape.OutLineHCompound());
        extract_detail(options.edge_h_smooth, hlr_to_shape.Rg1LineHCompound());
        extract_detail(options.edge_h_sewn, hlr_to_shape.RgNLineHCompound());
        extract_detail(options.edge_h_iso, hlr_to_shape.IsoLineHCompound());
    }

    // The outline always consumes the COMPLETE visible outline (sharp +
    // outline + smooth + sewn, V and H), independent of the detail checkboxes, so
    // the morphological close has a closed body to reconstruct. Iso lines are
    // isoparametric construction lines, not real boundary edges, so are excluded.
    auto extract_contour = [&](TopoDS_Shape shape)
    {
        if (shape.IsNull())
            return;
        add_edge_geometry(shape, options, nullptr, nullptr, &contour_source_segments, scale,
                          extent_scale);
    };
    if (options.output_outline)
    {
        extract_contour(hlr_to_shape.VCompound());
        extract_contour(hlr_to_shape.OutLineVCompound());
        extract_contour(hlr_to_shape.Rg1LineVCompound());
        extract_contour(hlr_to_shape.RgNLineVCompound());
        extract_contour(hlr_to_shape.HCompound());
        extract_contour(hlr_to_shape.OutLineHCompound());
        extract_contour(hlr_to_shape.Rg1LineHCompound());
        extract_contour(hlr_to_shape.RgNLineHCompound());
    }

    ProjectedViewGeometry projected;
    projected.view = view;
    projected.detail =
        geometry_from_keys(detail_segment_keys, detail_arc_keys, scale, extent_scale);
    projected.outline = outline_geometry_from_segments(contour_source_segments, options, scale);

    if (timings != nullptr)
    {
        timings->extract_ms += elapsed_ms(extract_start);
    }
    return projected;
}

ProjectedViewGeometry project_view_poly(const TopoDS_Shape& shape, const ProjectionViewSpec& view,
                                        const HlrProjectionOptions& options, std::int64_t scale,
                                        std::int64_t extent_scale, HlrProjectionTimings* timings)
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
    //
    // Detail respects the per-category checkboxes.
    auto extract_detail = [&](bool enabled, TopoDS_Shape shape)
    {
        if (!enabled || shape.IsNull())
            return;
        add_edge_geometry(shape, poly_options, &detail_segment_keys, &detail_arc_keys, nullptr,
                          scale, extent_scale);
    };
    if (poly_options.output_detail)
    {
        extract_detail(poly_options.edge_v_sharp, poly_to_shape.VCompound());
        extract_detail(poly_options.edge_v_outline, poly_to_shape.OutLineVCompound());
        extract_detail(poly_options.edge_h_sharp, poly_to_shape.HCompound());
        extract_detail(poly_options.edge_h_outline, poly_to_shape.OutLineHCompound());
    }

    // The outline always consumes the COMPLETE visible outline (every
    // sharp + outline edge, both V and H), independent of the detail checkboxes.
    // Filtering by category leaves whole body edges missing, so the morphological
    // close can never reconstruct a closed footprint.
    auto extract_contour = [&](TopoDS_Shape shape)
    {
        if (shape.IsNull())
            return;
        add_edge_geometry(shape, poly_options, nullptr, nullptr, &contour_source_segments, scale,
                          extent_scale);
    };
    if (poly_options.output_outline)
    {
        extract_contour(poly_to_shape.VCompound());
        extract_contour(poly_to_shape.OutLineVCompound());
        extract_contour(poly_to_shape.HCompound());
        extract_contour(poly_to_shape.OutLineHCompound());
    }

    ProjectedViewGeometry projected;
    projected.view = view;
    projected.detail =
        geometry_from_keys(detail_segment_keys, detail_arc_keys, scale, extent_scale);
    projected.outline =
        outline_geometry_from_segments(contour_source_segments, poly_options, scale);

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
        const std::int64_t extent_scale = pow10_int(std::max(options.round_digits, 6));
        const std::vector<ProjectionViewSpec> views = effective_views(options);
        output.views.reserve(views.size());

        const bool use_poly = options.projection_algorithm == ProjectionAlgorithm::Poly;
        const bool use_fast = options.projection_algorithm == ProjectionAlgorithm::Fast;
        const bool needs_classic_hlr =
            (!use_fast && options.output_detail) ||
            (options.output_outline &&
             options.outline_algorithm == ProjectionOutlineAlgorithm::HlrClosedEdges);
        const bool needs_fast_detail = use_fast && options.output_detail;
        const bool needs_fast_outline =
            options.output_outline &&
            options.outline_algorithm == ProjectionOutlineAlgorithm::FastMeshShadow;
        const bool needs_poly_hlr = needs_classic_hlr && (use_poly || use_fast);
        const bool needs_mesh =
            needs_poly_hlr || needs_fast_detail || needs_fast_outline ||
            (options.output_outline &&
             options.outline_algorithm == ProjectionOutlineAlgorithm::MeshShadow);
        if (needs_mesh)
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
            if (needs_classic_hlr && (use_poly || use_fast))
            {
                HlrProjectionOptions classic_options = options;
                if (use_fast)
                {
                    classic_options.output_detail = false;
                }
                projected_view =
                    project_view_poly(shape, view, classic_options, scale, extent_scale, &timings);
            }
            else if (needs_classic_hlr)
            {
                projected_view =
                    project_view_exact(shape, view, options, scale, extent_scale, &timings);
            }
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
                projected_view.detail =
                    geometry_from_keys(fast_segment_keys, std::set<ArcKey>(), scale, extent_scale);
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

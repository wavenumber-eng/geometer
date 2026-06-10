#include "mesh_shadow_outline.h"

#include <clipper2/clipper.h>

#include <BRep_Tool.hxx>
#include <Poly_Triangle.hxx>
#include <Poly_Triangulation.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <gp_XYZ.hxx>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <vector>

namespace geometer
{
namespace
{

struct PointKey
{
    std::int64_t x = 0;
    std::int64_t y = 0;

    bool operator<(const PointKey& other) const
    {
        if (x != other.x)
        {
            return x < other.x;
        }
        return y < other.y;
    }
};

std::int64_t snap(double value, std::int64_t scale)
{
    return static_cast<std::int64_t>(std::llround(value * static_cast<double>(scale)));
}

double unsnap(std::int64_t value, std::int64_t scale)
{
    return static_cast<double>(value) / static_cast<double>(scale);
}

PointKey point_key(double x, double y, std::int64_t scale)
{
    return {snap(x, scale), snap(y, scale)};
}

gp_Ax2 make_view_axes(const ProjectionViewSpec& view)
{
    gp_Ax2 axes(gp_Pnt(0.0, 0.0, 0.0),
                gp_Dir(view.direction[0], view.direction[1], view.direction[2]));
    axes.SetYDirection(gp_Dir(view.up[0], view.up[1], view.up[2]));
    return axes;
}

Clipper2Lib::PointD project_point(const gp_Pnt& point, const gp_Ax2& axes)
{
    const gp_XYZ xyz = point.XYZ();
    const gp_XYZ x_dir = axes.XDirection().XYZ();
    const gp_XYZ y_dir = axes.YDirection().XYZ();
    return {xyz.Dot(x_dir), xyz.Dot(y_dir)};
}

double signed_area2(const Clipper2Lib::PointD& a, const Clipper2Lib::PointD& b,
                    const Clipper2Lib::PointD& c)
{
    return ((b.x - a.x) * (c.y - a.y)) - ((b.y - a.y) * (c.x - a.x));
}

void add_path_segments(const Clipper2Lib::PathD& path, std::int64_t scale,
                       std::vector<ProjectedSegment>* segments)
{
    if (path.size() < 3 || segments == nullptr)
    {
        return;
    }

    std::vector<PointKey> points;
    points.reserve(path.size());
    for (const Clipper2Lib::PointD& point : path)
    {
        const PointKey key = point_key(point.x, point.y, scale);
        if (points.empty() || points.back().x != key.x || points.back().y != key.y)
        {
            points.push_back(key);
        }
    }
    if (points.size() > 1 && points.front().x == points.back().x &&
        points.front().y == points.back().y)
    {
        points.pop_back();
    }
    if (points.size() < 3)
    {
        return;
    }

    for (std::size_t i = 0; i < points.size(); ++i)
    {
        const PointKey& start = points[i];
        const PointKey& end = points[(i + 1) % points.size()];
        if (start.x == end.x && start.y == end.y)
        {
            continue;
        }
        segments->push_back({unsnap(start.x, scale), unsnap(start.y, scale), unsnap(end.x, scale),
                             unsnap(end.y, scale)});
    }
}

} // namespace

ProjectedModeGeometry mesh_shadow_outline_geometry(const TopoDS_Shape& shape,
                                                   const ProjectionViewSpec& view,
                                                   const HlrProjectionOptions& /*options*/,
                                                   std::int64_t scale)
{
    using Clipper2Lib::ClipperD;
    using Clipper2Lib::ClipType;
    using Clipper2Lib::FillRule;
    using Clipper2Lib::PathD;
    using Clipper2Lib::PathsD;
    using Clipper2Lib::PointD;
    using Clipper2Lib::SimplifyPaths;

    ProjectedModeGeometry geometry;
    if (shape.IsNull())
    {
        return geometry;
    }

    const gp_Ax2 axes = make_view_axes(view);
    PathsD triangles;

    for (TopExp_Explorer explorer(shape, TopAbs_FACE); explorer.More(); explorer.Next())
    {
        const TopoDS_Face& face = TopoDS::Face(explorer.Current());
        TopLoc_Location location;
        const Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, location);
        if (triangulation.IsNull())
        {
            continue;
        }

        const gp_Trsf transform = location.Transformation();
        for (int i = 1; i <= triangulation->NbTriangles(); ++i)
        {
            int n1 = 0;
            int n2 = 0;
            int n3 = 0;
            triangulation->Triangle(i).Get(n1, n2, n3);

            PointD a = project_point(triangulation->Node(n1).Transformed(transform), axes);
            PointD b = project_point(triangulation->Node(n2).Transformed(transform), axes);
            PointD c = project_point(triangulation->Node(n3).Transformed(transform), axes);
            const double area2 = signed_area2(a, b, c);
            if (std::fabs(area2) <= 1.0e-12)
            {
                continue;
            }
            if (area2 < 0.0)
            {
                std::swap(b, c);
            }
            triangles.push_back(PathD{a, b, c});
        }
    }

    if (triangles.empty())
    {
        return geometry;
    }

    ClipperD clipper(/*precision=*/6);
    clipper.AddSubject(triangles);
    PathsD solution;
    clipper.Execute(ClipType::Union, FillRule::NonZero, solution);
    if (solution.empty())
    {
        return geometry;
    }

    solution = SimplifyPaths(solution, 1.0 / static_cast<double>(std::max<std::int64_t>(scale, 1)),
                             /*is_open_path=*/false);
    for (const PathD& path : solution)
    {
        add_path_segments(path, scale, &geometry.segments);
    }
    return geometry;
}

} // namespace geometer

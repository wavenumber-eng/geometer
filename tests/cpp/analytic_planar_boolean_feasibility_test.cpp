#include <BRepAdaptor_Curve.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRep_Tool.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <GeomAbs_CurveType.hxx>
#include <NCollection_IndexedMap.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>
#include <TopAbs_Orientation.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_ShapeMapHasher.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{

constexpr double kNmPerMm = 1000000.0;
constexpr double kPi = 3.14159265358979323846;

struct ShapeWithSources
{
    TopoDS_Shape shape;
    std::vector<TopoDS_Edge> source_edges;
};

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

double nm(double value)
{
    return value / kNmPerMm;
}

std::int64_t normalize_nm(double millimeters)
{
    return static_cast<std::int64_t>(std::llround(millimeters * kNmPerMm));
}

gp_Pnt polar_point(double center_x, double center_y, double radius, double angle)
{
    return gp_Pnt(center_x + radius * std::cos(angle), center_y + radius * std::sin(angle), 0.0);
}

ShapeWithSources make_wire_face(std::vector<TopoDS_Edge> edges)
{
    BRepBuilderAPI_MakeWire wire_maker;
    for (const TopoDS_Edge& edge : edges)
    {
        wire_maker.Add(edge);
        require(wire_maker.IsDone(), "failed adding feasibility edge to wire");
    }
    BRepBuilderAPI_MakeFace face_maker(wire_maker.Wire(), true);
    require(face_maker.IsDone(), "failed creating feasibility face");
    return {face_maker.Face(), std::move(edges)};
}

ShapeWithSources make_rectangle(double min_x, double min_y, double max_x, double max_y)
{
    const gp_Pnt p0(min_x, min_y, 0.0);
    const gp_Pnt p1(max_x, min_y, 0.0);
    const gp_Pnt p2(max_x, max_y, 0.0);
    const gp_Pnt p3(min_x, max_y, 0.0);
    return make_wire_face(
        {BRepBuilderAPI_MakeEdge(p0, p1).Edge(), BRepBuilderAPI_MakeEdge(p1, p2).Edge(),
         BRepBuilderAPI_MakeEdge(p2, p3).Edge(), BRepBuilderAPI_MakeEdge(p3, p0).Edge()});
}

ShapeWithSources make_disk(double center_x, double center_y, double radius)
{
    const gp_Circ circle(gp_Ax2(gp_Pnt(center_x, center_y, 0.0), gp_Dir(0.0, 0.0, 1.0)), radius);
    return make_wire_face({BRepBuilderAPI_MakeEdge(circle).Edge()});
}

ShapeWithSources make_annular_sector(double center_x, double center_y, double inner_radius,
                                     double outer_radius, double start_angle, double sweep_angle)
{
    require(sweep_angle > 0.0 && sweep_angle < 2.0 * kPi,
            "feasibility sector expects a positive sub-turn sweep");
    const double end_angle = start_angle + sweep_angle;
    const gp_Ax2 axis(gp_Pnt(center_x, center_y, 0.0), gp_Dir(0.0, 0.0, 1.0));
    const gp_Circ outer(axis, outer_radius);
    const gp_Circ inner(axis, inner_radius);
    const gp_Pnt outer_start = polar_point(center_x, center_y, outer_radius, start_angle);
    const gp_Pnt outer_end = polar_point(center_x, center_y, outer_radius, end_angle);
    const gp_Pnt inner_end = polar_point(center_x, center_y, inner_radius, end_angle);
    const gp_Pnt inner_start = polar_point(center_x, center_y, inner_radius, start_angle);

    TopoDS_Edge inner_edge = BRepBuilderAPI_MakeEdge(inner, start_angle, end_angle).Edge();
    inner_edge.Reverse();
    return make_wire_face({BRepBuilderAPI_MakeEdge(outer, start_angle, end_angle).Edge(),
                           BRepBuilderAPI_MakeEdge(outer_end, inner_end).Edge(), inner_edge,
                           BRepBuilderAPI_MakeEdge(inner_start, outer_start).Edge()});
}

ShapeWithSources make_round_arc_sweep(double center_x, double center_y, double radius,
                                      double start_angle, double sweep_angle, double width)
{
    require(std::abs(sweep_angle) > 0.0 && std::abs(sweep_angle) < 2.0 * kPi,
            "feasibility arc sweep expects a nonzero sub-turn sweep");
    if (sweep_angle < 0.0)
    {
        start_angle += sweep_angle;
        sweep_angle = -sweep_angle;
    }
    const double end_angle = start_angle + sweep_angle;
    const double half_width = width / 2.0;
    const double inner_radius = radius - half_width;
    const double outer_radius = radius + half_width;
    require(inner_radius > 0.0, "feasibility arc sweep width exceeds its diameter");

    const gp_Ax2 axis(gp_Pnt(center_x, center_y, 0.0), gp_Dir(0.0, 0.0, 1.0));
    const gp_Circ outer(axis, outer_radius);
    const gp_Circ inner(axis, inner_radius);
    const gp_Pnt outer_start = polar_point(center_x, center_y, outer_radius, start_angle);
    const gp_Pnt outer_end = polar_point(center_x, center_y, outer_radius, end_angle);
    const gp_Pnt inner_end = polar_point(center_x, center_y, inner_radius, end_angle);
    const gp_Pnt inner_start = polar_point(center_x, center_y, inner_radius, start_angle);
    const gp_Pnt centerline_end = polar_point(center_x, center_y, radius, end_angle);
    const gp_Pnt centerline_start = polar_point(center_x, center_y, radius, start_angle);
    const gp_Pnt end_cap_mid(centerline_end.X() - half_width * std::sin(end_angle),
                             centerline_end.Y() + half_width * std::cos(end_angle), 0.0);
    const gp_Pnt start_cap_mid(centerline_start.X() + half_width * std::sin(start_angle),
                               centerline_start.Y() - half_width * std::cos(start_angle), 0.0);

    GC_MakeArcOfCircle end_cap(outer_end, end_cap_mid, inner_end);
    GC_MakeArcOfCircle start_cap(inner_start, start_cap_mid, outer_start);
    require(end_cap.IsDone() && start_cap.IsDone(), "failed creating round sweep caps");
    TopoDS_Edge inner_edge = BRepBuilderAPI_MakeEdge(inner, start_angle, end_angle).Edge();
    inner_edge.Reverse();
    return make_wire_face({BRepBuilderAPI_MakeEdge(outer, start_angle, end_angle).Edge(),
                           BRepBuilderAPI_MakeEdge(end_cap.Value()).Edge(), inner_edge,
                           BRepBuilderAPI_MakeEdge(start_cap.Value()).Edge()});
}

TopoDS_Shape unify(const TopoDS_Shape& shape)
{
    ShapeUpgrade_UnifySameDomain unifier(shape, true, true, true);
    unifier.Build();
    return unifier.Shape();
}

TopoDS_Shape fuse(const TopoDS_Shape& left, const TopoDS_Shape& right)
{
    BRepAlgoAPI_Fuse operation(left, right);
    operation.Build();
    require(operation.IsDone(), "OCCT feasibility fuse failed");
    return unify(operation.Shape());
}

int count_subshapes(const TopoDS_Shape& shape, TopAbs_ShapeEnum kind)
{
    int count = 0;
    for (TopExp_Explorer explorer(shape, kind); explorer.More(); explorer.Next())
    {
        ++count;
    }
    return count;
}

std::vector<std::string> canonical_boundary_signature(const TopoDS_Shape& shape)
{
    std::vector<std::string> result;
    for (TopExp_Explorer explorer(shape, TopAbs_EDGE); explorer.More(); explorer.Next())
    {
        const TopoDS_Edge edge = TopoDS::Edge(explorer.Current());
        const BRepAdaptor_Curve curve(edge);
        const gp_Pnt first = curve.Value(curve.FirstParameter());
        const gp_Pnt last = curve.Value(curve.LastParameter());
        std::pair<std::int64_t, std::int64_t> a = {normalize_nm(first.X()),
                                                   normalize_nm(first.Y())};
        std::pair<std::int64_t, std::int64_t> b = {normalize_nm(last.X()), normalize_nm(last.Y())};
        if (b < a)
        {
            std::swap(a, b);
        }

        std::ostringstream signature;
        if (curve.GetType() == GeomAbs_Line)
        {
            signature << "L:";
        }
        else if (curve.GetType() == GeomAbs_Circle)
        {
            const gp_Circ circle = curve.Circle();
            signature << "C:" << normalize_nm(circle.Location().X()) << ','
                      << normalize_nm(circle.Location().Y()) << ',' << normalize_nm(circle.Radius())
                      << ':';
        }
        else
        {
            signature << "UNSUPPORTED:" << static_cast<int>(curve.GetType()) << ':';
        }
        signature << a.first << ',' << a.second << ':' << b.first << ',' << b.second;
        result.push_back(signature.str());
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<std::string> endpoint_fragment_signature(const TopoDS_Shape& shape)
{
    std::vector<std::string> result;
    for (TopExp_Explorer explorer(shape, TopAbs_EDGE); explorer.More(); explorer.Next())
    {
        const TopoDS_Edge edge = TopoDS::Edge(explorer.Current());
        const TopoDS_Vertex first_vertex = TopExp::FirstVertex(edge, true);
        const TopoDS_Vertex last_vertex = TopExp::LastVertex(edge, true);
        require(!first_vertex.IsNull() && !last_vertex.IsNull(),
                "case-2 feasibility fragment unexpectedly has no endpoint");
        const gp_Pnt first = BRep_Tool::Pnt(first_vertex);
        const gp_Pnt last = BRep_Tool::Pnt(last_vertex);
        const BRepAdaptor_Curve curve(edge);

        std::ostringstream signature;
        if (curve.GetType() == GeomAbs_Line)
        {
            signature << "L:";
        }
        else
        {
            require(curve.GetType() == GeomAbs_Circle,
                    "case-2 endpoint signature encountered unsupported curve");
            const gp_Circ circle = curve.Circle();
            const bool forward = edge.Orientation() == TopAbs_FORWARD;
            const bool axis_positive = circle.Axis().Direction().Z() > 0.0;
            const bool ccw = forward == axis_positive;
            const double span = std::abs(curve.LastParameter() - curve.FirstParameter());
            const bool major_arc = span > kPi + 1.0e-12;
            signature << "A:" << normalize_nm(circle.Radius()) << ':' << (ccw ? "ccw" : "cw") << ':'
                      << (major_arc ? "major" : "minor") << ':';
        }
        signature << normalize_nm(first.X()) << ',' << normalize_nm(first.Y()) << ':'
                  << normalize_nm(last.X()) << ',' << normalize_nm(last.Y());
        result.push_back(signature.str());
    }
    std::sort(result.begin(), result.end());
    return result;
}

const std::vector<std::string>& expected_matz_endpoint_fragments()
{
    static const std::vector<std::string> expected = {
        "A:2600000:cw:minor:4213333,2299527:5560500,451485",
        "A:2600000:cw:minor:643600,1098807:2080000,2431789",
        "A:3200000:cw:minor:-3007016,1094464:540000,3154108",
        "A:3200000:cw:minor:2080000,2431789:3007016,1094464",
        "A:4000000:ccw:minor:540000,3154108:-625231,1690473",
        "A:4000000:ccw:minor:6939231,694593:2673333,3986639",
        "A:4800000:ccw:minor:2673333,3986639:-4510525,1641697",
        "A:4800000:ccw:minor:4510525,1641697:4213333,2299527",
        "A:700000:ccw:minor:-625231,1690473:643600,1098807",
        "A:700000:ccw:minor:5560500,451485:6939231,694593",
        "A:800000:ccw:minor:-4510525,1641697:-3007016,1094464",
        "A:800000:ccw:minor:3007016,1094464:4510525,1641697",
    };
    return expected;
}

void require_only_line_and_circle(const TopoDS_Shape& shape)
{
    for (TopExp_Explorer explorer(shape, TopAbs_EDGE); explorer.More(); explorer.Next())
    {
        const BRepAdaptor_Curve curve(TopoDS::Edge(explorer.Current()));
        require(curve.GetType() == GeomAbs_Line || curve.GetType() == GeomAbs_Circle,
                "OCCT produced a non-line/non-circle feasibility boundary");
    }
}

void require_safe_vertex_normalization(const TopoDS_Shape& shape)
{
    NCollection_IndexedMap<TopoDS_Shape, TopTools_ShapeMapHasher> vertices;
    TopExp::MapShapes(shape, TopAbs_VERTEX, vertices);
    std::set<std::pair<std::int64_t, std::int64_t>> representatives;
    for (int index = 1; index <= vertices.Extent(); ++index)
    {
        const gp_Pnt point = BRep_Tool::Pnt(TopoDS::Vertex(vertices(index)));
        const std::int64_t x = normalize_nm(point.X());
        const std::int64_t y = normalize_nm(point.Y());
        const double dx = point.X() * kNmPerMm - static_cast<double>(x);
        const double dy = point.Y() * kNmPerMm - static_cast<double>(y);
        require(dx * dx + dy * dy <= 0.500000001,
                "normalized feasibility vertex exceeded the half-square-nm error bound");
        require(representatives.insert({x, y}).second,
                "distinct feasibility vertices collapsed to one nm-grid representative");
    }
}

void test_regularized_topology()
{
    const ShapeWithSources left = make_disk(0.0, 0.0, 1.0);
    const ShapeWithSources tangent = make_disk(2.0, 0.0, 1.0);
    const TopoDS_Shape point_union = fuse(left.shape, tangent.shape);
    require(count_subshapes(point_union, TopAbs_FACE) == 2,
            "point-tangent disks should remain two regularized area faces");

    const ShapeWithSources first = make_rectangle(0.0, 0.0, 2.0, 1.0);
    const ShapeWithSources second = make_rectangle(2.0, 0.0, 4.0, 1.0);
    const TopoDS_Shape edge_union = fuse(first.shape, second.shape);
    require(count_subshapes(edge_union, TopAbs_FACE) == 1,
            "shared-edge rectangles should produce one regularized face");
    require(count_subshapes(edge_union, TopAbs_EDGE) == 4,
            "shared-edge rectangle seam should not survive canonical unification");
}

void test_analytic_extraction_and_order_independence()
{
    const ShapeWithSources first = make_disk(0.0, 0.0, 2.0);
    const ShapeWithSources second = make_disk(1.5, 0.0, 2.0);
    const TopoDS_Shape forward = fuse(first.shape, second.shape);
    const TopoDS_Shape reverse = fuse(second.shape, first.shape);
    require_only_line_and_circle(forward);
    require(count_subshapes(forward, TopAbs_FACE) == 1,
            "overlapping disks should produce one result face");
    require(canonical_boundary_signature(forward) == canonical_boundary_signature(reverse),
            "canonical analytic boundary should not depend on operand order within a stage");

    std::set<std::pair<std::int64_t, std::int64_t>> intersection_vertices;
    for (TopExp_Explorer explorer(forward, TopAbs_VERTEX); explorer.More(); explorer.Next())
    {
        const gp_Pnt point = BRep_Tool::Pnt(TopoDS::Vertex(explorer.Current()));
        const double first_radius_error = std::abs(point.Distance(gp_Pnt(0.0, 0.0, 0.0)) - 2.0);
        const double second_radius_error = std::abs(point.Distance(gp_Pnt(1.5, 0.0, 0.0)) - 2.0);
        if (first_radius_error < 1.0e-7 && second_radius_error < 1.0e-7)
        {
            intersection_vertices.insert({normalize_nm(point.X()), normalize_nm(point.Y())});
        }
    }
    require(intersection_vertices.size() == 2,
            "overlapping circles should expose two vertices classifiable to both sources");

    const ShapeWithSources sector_a =
        make_annular_sector(0.0, 0.0, 3.2, 4.8, 20.0 * kPi / 180.0, 140.0 * kPi / 180.0);
    const ShapeWithSources sector_b =
        make_annular_sector(3.0, 0.0, 2.6, 4.0, 10.0 * kPi / 180.0, 145.0 * kPi / 180.0);
    const TopoDS_Shape sector_union = fuse(sector_a.shape, sector_b.shape);
    require(count_subshapes(sector_union, TopAbs_FACE) >= 1,
            "intersecting analytic sectors should produce area output");
    require_only_line_and_circle(sector_union);

    const ShapeWithSources matz_arc_a =
        make_round_arc_sweep(0.0, 0.0, 4.0, 20.0 * kPi / 180.0, 140.0 * kPi / 180.0, 1.6);
    const ShapeWithSources matz_arc_b =
        make_round_arc_sweep(3.0, 0.0, 3.3, 155.0 * kPi / 180.0, -145.0 * kPi / 180.0, 1.4);
    const TopoDS_Shape matz_arc_union = fuse(matz_arc_a.shape, matz_arc_b.shape);
    require(count_subshapes(matz_arc_union, TopAbs_FACE) >= 1,
            "MATZ arbitrary-angle arc fixture should succeed");
    require_only_line_and_circle(matz_arc_union);
    require_safe_vertex_normalization(matz_arc_union);
    require(endpoint_fragment_signature(matz_arc_union) == expected_matz_endpoint_fragments(),
            "MATZ arbitrary-angle endpoint/radius fragment oracle changed");
}

void test_normalization_collapse_probe()
{
    const double first_intersection_nm = 1.25;
    const double second_intersection_nm = 1.4;
    require(normalize_nm(nm(first_intersection_nm)) == 1,
            "1.25 nm should normalize to the 1 nm representative");
    require(normalize_nm(nm(second_intersection_nm)) == 1,
            "1.4 nm should normalize to the 1 nm representative");
    require(std::abs(first_intersection_nm - second_intersection_nm) > 0.0,
            "probe vertices must be distinct before normalization");
}

void test_history_is_not_complete_lineage()
{
    const ShapeWithSources outer = make_disk(0.0, 0.0, 5.0);
    const ShapeWithSources absorbed = make_disk(0.0, 0.0, 1.0);
    BRepAlgoAPI_Fuse operation(outer.shape, absorbed.shape);
    operation.Build();
    require(operation.IsDone(), "absorbed-positive feasibility fuse failed");
    require(count_subshapes(unify(operation.Shape()), TopAbs_FACE) == 1,
            "absorbed positive should leave one result face");

    const TopoDS_Shape result = unify(operation.Shape());
    const TopoDS_Edge absorbed_edge = absorbed.source_edges.front();
    bool mapped_to_final_boundary = false;
    for (const TopoDS_Shape& mapped : operation.Modified(absorbed_edge))
    {
        for (TopExp_Explorer explorer(result, TopAbs_EDGE); explorer.More(); explorer.Next())
        {
            mapped_to_final_boundary =
                mapped_to_final_boundary || mapped.IsSame(explorer.Current());
        }
    }
    for (const TopoDS_Shape& mapped : operation.Generated(absorbed_edge))
    {
        for (TopExp_Explorer explorer(result, TopAbs_EDGE); explorer.More(); explorer.Next())
        {
            mapped_to_final_boundary =
                mapped_to_final_boundary || mapped.IsSame(explorer.Current());
        }
    }
    require(!mapped_to_final_boundary,
            "absorbed positive should have no mapped final boundary despite contributing material");
}

std::string parity_signature()
{
    const ShapeWithSources first = make_disk(0.0, 0.0, 2.0);
    const ShapeWithSources second = make_disk(1.5, 0.0, 2.0);
    const ShapeWithSources sector_a =
        make_annular_sector(0.0, 0.0, 3.2, 4.8, 20.0 * kPi / 180.0, 140.0 * kPi / 180.0);
    const ShapeWithSources sector_b =
        make_annular_sector(3.0, 0.0, 2.6, 4.0, 10.0 * kPi / 180.0, 145.0 * kPi / 180.0);
    const ShapeWithSources matz_arc_a =
        make_round_arc_sweep(0.0, 0.0, 4.0, 20.0 * kPi / 180.0, 140.0 * kPi / 180.0, 1.6);
    const ShapeWithSources matz_arc_b =
        make_round_arc_sweep(3.0, 0.0, 3.3, 155.0 * kPi / 180.0, -145.0 * kPi / 180.0, 1.4);

    std::ostringstream output;
    output << "disk";
    for (const std::string& item : canonical_boundary_signature(fuse(first.shape, second.shape)))
    {
        output << '|' << item;
    }
    output << "\nsector";
    for (const std::string& item :
         canonical_boundary_signature(fuse(sector_a.shape, sector_b.shape)))
    {
        output << '|' << item;
    }
    output << "\nmatz_arc";
    const TopoDS_Shape matz_arc_union = fuse(matz_arc_a.shape, matz_arc_b.shape);
    for (const std::string& item : canonical_boundary_signature(matz_arc_union))
    {
        output << '|' << item;
    }
    output << "\nmatz_endpoint_fragments";
    for (const std::string& item : endpoint_fragment_signature(matz_arc_union))
    {
        output << '|' << item;
    }
    output << "\ncollapse|" << normalize_nm(nm(1.25)) << '|' << normalize_nm(nm(1.4));
    return output.str();
}

} // namespace

int main()
{
    test_regularized_topology();
    test_analytic_extraction_and_order_independence();
    test_normalization_collapse_probe();
    test_history_is_not_complete_lineage();
    std::cout << parity_signature() << '\n';
    return 0;
}

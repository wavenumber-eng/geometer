#include <BRepBndLib.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGraph.hxx>
#include <BRepGraph_EditorView.hxx>
#include <BRepGraph_LayerRegistry.hxx>
#include <BRepGraph_LayerTopoSupplement.hxx>
#include <BRepGraph_MeshView.hxx>
#include <BRepGraph_ShapesView.hxx>
#include <BRepGraph_TopoView.hxx>
#include <BRepGraph_UIDsView.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <Bnd_Box.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

double minimum_x(const TopoDS_Shape& shape)
{
    Bnd_Box bounds;
    BRepBndLib::AddOptimal(shape, bounds, false, false);
    require(!bounds.IsVoid(), "BRepGraph reconstructed shape has empty bounds");
    double xmin = 0.0;
    double ymin = 0.0;
    double zmin = 0.0;
    double xmax = 0.0;
    double ymax = 0.0;
    double zmax = 0.0;
    bounds.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    return xmin;
}

void brep_graph_is_an_isolated_capability_probe()
{
    TopoDS_Shape box = BRepPrimAPI_MakeBox(2.0, 3.0, 1.0).Shape();
    BRepMesh_IncrementalMesh mesher(box, 0.2);
    mesher.Perform();
    require(mesher.IsDone(), "failed meshing BRepGraph probe box");

    BRepGraph graph;
    BRepGraph::ShapesView::Options options;
    options.TrackAddedNodes = true;
    const BRepGraph::ShapesView::Result added = graph.Shapes().Add(box, options);
    require(added.IsOk() && added.TopologyRoot.IsValid() && added.Product.IsValid(),
            "BRepGraph failed ingesting a valid box");
    require(graph.ValidateRelations(), "BRepGraph relations are invalid after population");
    require(graph.Topo().Products().Nb() == 1 && graph.Topo().Occurrences().Nb() == 1,
            "BRepGraph auto-product baseline is unexpected");
    require(graph.Topo().Solids().Nb() == 1 && graph.Topo().Shells().Nb() == 1 &&
                graph.Topo().Faces().Nb() == 6,
            "BRepGraph topology counts do not match a box");

    const BRepGraph_FaceId face = BRepGraph_FaceId::Start();
    const BRepGraph_UID face_uid = graph.UIDs().Of(face);
    require(face_uid.IsValid() && graph.UIDs().Has(face_uid) &&
                graph.UIDs().NodeIdFrom(face_uid) == BRepGraph_NodeId(face),
            "BRepGraph UID round trip failed");
    require(graph.Mesh().Persistent().Faces().Has(face),
            "BRepGraph did not retain imported face triangulation");

    const TopoDS_Shape reconstructed = graph.Shapes().Reconstruct(added.TopologyRoot);
    require(!reconstructed.IsNull() && BRepCheck_Analyzer(reconstructed).IsValid(),
            "BRepGraph topology reconstruction is invalid");
    require(std::abs(minimum_x(reconstructed)) < 1.0e-7,
            "BRepGraph reconstructed box moved unexpectedly");

    const Handle(BRepGraph_LayerTopoSupplement) layer =
        graph.LayerRegistry().Ensure<BRepGraph_LayerTopoSupplement>();
    require(!layer.IsNull() && graph.LayerRegistry().NbLayers() >= 1,
            "BRepGraph layer registration is unavailable");

    const BRepGraph_ProductId assembly = graph.Editor().Products().Add();
    graph.Editor().Products().AppendDocumentRoot(assembly);
    gp_Trsf first_transform;
    first_transform.SetTranslation(gp_Vec(10.0, 0.0, 0.0));
    gp_Trsf second_transform;
    second_transform.SetTranslation(gp_Vec(20.0, 0.0, 0.0));
    const BRepGraph_OccurrenceId first =
        graph.Editor().Products().Append(assembly, added.Product, TopLoc_Location(first_transform));
    const BRepGraph_OccurrenceId second = graph.Editor().Products().Append(
        assembly, added.Product, TopLoc_Location(second_transform));
    require(first.IsValid() && second.IsValid() && first != second,
            "BRepGraph failed creating repeated occurrences");
    require(graph.Topo().Occurrences().Product(first) ==
                    graph.Topo().Occurrences().Product(second) &&
                graph.Topo().Products().NbComponents(assembly) == 2,
            "BRepGraph repeated occurrences lost shared product identity");
    require(std::abs(minimum_x(graph.Shapes().Reconstruct(first)) - 10.0) < 1.0e-7 &&
                std::abs(minimum_x(graph.Shapes().Reconstruct(second)) - 20.0) < 1.0e-7,
            "BRepGraph occurrence reconstruction lost placement");
    require(graph.UIDs().NodeIdFrom(face_uid) == BRepGraph_NodeId(face),
            "BRepGraph face UID changed after unrelated product edits");

    graph.Clear();
    require(!graph.UIDs().Has(face_uid),
            "BRepGraph UID should be invalid after graph generation reset");
}

} // namespace

int main()
{
    try
    {
        brep_graph_is_an_isolated_capability_probe();
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

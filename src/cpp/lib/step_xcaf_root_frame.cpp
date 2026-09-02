#include "step_xcaf_root_frame.h"

#include <BRep_Builder.hxx>
#include <NCollection_Sequence.hxx>
#include <TDF_Label.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS_Compound.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_Location.hxx>
#include <XCAFDoc_ShapeTool.hxx>

namespace geometer
{

void strip_free_shape_root_locations(const Handle(TDocStd_Document) & document)
{
    const Handle(XCAFDoc_ShapeTool) shape_tool = XCAFDoc_DocumentTool::ShapeTool(document->Main());
    NCollection_Sequence<TDF_Label> free_shapes;
    shape_tool->GetFreeShapes(free_shapes);
    for (int index = 1; index <= free_shapes.Length(); ++index)
    {
        const TDF_Label label = free_shapes.Value(index);
        const TopoDS_Shape shape = shape_tool->GetShape(label);
        if (shape.IsNull() || shape.Location().IsIdentity())
        {
            continue;
        }
        XCAFDoc_Location::Set(label, TopLoc_Location());
        shape_tool->SetShape(label, shape.Located(TopLoc_Location()));
    }
}

TopoDS_Shape free_shape_compound(const Handle(TDocStd_Document) & document)
{
    const Handle(XCAFDoc_ShapeTool) shape_tool = XCAFDoc_DocumentTool::ShapeTool(document->Main());
    NCollection_Sequence<TDF_Label> free_shapes;
    shape_tool->GetFreeShapes(free_shapes);
    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);
    bool added_shape = false;
    for (int index = 1; index <= free_shapes.Length(); ++index)
    {
        const TopoDS_Shape shape = shape_tool->GetShape(free_shapes.Value(index));
        if (!shape.IsNull())
        {
            builder.Add(compound, shape);
            added_shape = true;
        }
    }
    return added_shape ? TopoDS_Shape(compound) : TopoDS_Shape();
}

} // namespace geometer

#include "geometer/step_to_glb.h"

#include <BRepMesh_IncrementalMesh.hxx>
#include <Message_ProgressRange.hxx>
#include <RWGltf_CafWriter.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <TColStd_IndexedDataMapOfStringString.hxx>
#include <TCollection_AsciiString.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TDF_LabelSequence.hxx>
#include <TDocStd_Document.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>

namespace geometer
{

int step_to_glb(const std::string& step_path, const std::string& glb_path,
                const StepToGlbOptions& options)
{
    Handle(TDocStd_Document) doc = new TDocStd_Document(TCollection_ExtendedString("XmlOcaf"));

    STEPCAFControl_Reader reader;
    reader.SetColorMode(true);
    reader.SetNameMode(true);
    reader.SetLayerMode(true);

    IFSelect_ReturnStatus status = reader.ReadFile(step_path.c_str());
    if (status != IFSelect_RetDone)
    {
        return 1;
    }

    if (!reader.Transfer(doc))
    {
        return 2;
    }

    Handle(XCAFDoc_ShapeTool) shape_tool = XCAFDoc_DocumentTool::ShapeTool(doc->Main());
    TDF_LabelSequence free_shapes;
    shape_tool->GetFreeShapes(free_shapes);

    for (Standard_Integer i = 1; i <= free_shapes.Length(); ++i)
    {
        TopoDS_Shape shape = shape_tool->GetShape(free_shapes.Value(i));
        BRepMesh_IncrementalMesh mesher(shape, options.linear_deflection, Standard_False,
                                        options.angular_deflection, Standard_False);
    }

    RWGltf_CafWriter writer(TCollection_AsciiString(glb_path.c_str()), Standard_True);

    Standard_Boolean write_ok =
        writer.Perform(doc, TColStd_IndexedDataMapOfStringString(), Message_ProgressRange());

    return write_ok ? 0 : 3;
}

} // namespace geometer

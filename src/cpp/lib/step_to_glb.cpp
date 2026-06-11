#include "geometer/step_to_glb.h"

#include <BRepMesh_IncrementalMesh.hxx>
#include <Message_ProgressRange.hxx>
#include <NCollection_IndexedDataMap.hxx>
#include <NCollection_Sequence.hxx>
#include <RWGltf_CafWriter.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <TCollection_AsciiString.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TDF_Label.hxx>
#include <TDocStd_Document.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS_Shape.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_Location.hxx>
#include <XCAFDoc_ShapeTool.hxx>

#include <atomic>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace geometer
{
namespace
{

std::string temp_step_to_glb_path(const char* extension)
{
    static std::atomic<unsigned long> counter{0};
    const unsigned long id = ++counter;
    return std::string("geometer_step_to_glb_") + std::to_string(id) + extension;
}

void set_status(Status* status, int code, const std::string& message)
{
    if (status == nullptr)
    {
        return;
    }
    status->code = code;
    status->message = message;
}

bool write_binary_file(const std::string& path, const unsigned char* data, std::size_t size)
{
    std::ofstream output(path, std::ios::binary);
    if (!output)
    {
        return false;
    }
    if (size > 0)
    {
        output.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
    }
    return output.good();
}

bool read_binary_file(const std::string& path, std::vector<unsigned char>* bytes)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
    {
        return false;
    }
    bytes->assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    return input.good() || input.eof();
}

} // namespace

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
    NCollection_Sequence<TDF_Label> free_shapes;
    shape_tool->GetFreeShapes(free_shapes);

    // Transform-chain note: OCCT's STEPCAFControl_Reader preserves a STEP file's
    // source coordinate system (the top-level AXIS2_PLACEMENT_3D attached to the
    // SHAPE_REPRESENTATION) as the location on the free-shape label. The location
    // is stored in two places that must both be cleared for it to disappear from
    // the emitted GLB:
    //   - on the TopoDS_Shape returned by ShapeTool::GetShape (cleared by
    //     SetShape with shape.Located(identity))
    //   - as an XCAFDoc_Location attribute on the label itself (cleared by
    //     XCAFDoc_Location::Set(label, identity))
    // RWGltf_CafWriter reads the XCAFDoc_Location attribute, so stripping only
    // the shape's location is not enough. Without both, the placement becomes
    // the GLB root node's transform.
    //
    // Downstream consumers (Altium body placement records, our matrixForPcbModel)
    // expect a "naked" model whose local frame matches the modelling origin, so
    // any non-identity root transform ends up combined with the body record's
    // own RotX/Y/Z and body_projection (top/bottom) and produces an upside-down
    // or otherwise mis-oriented part. (Concrete failure: Alps RK09K1130AP5
    // potentiometer's STEP file carries a 180 deg rotation about Y in its
    // AXIS2_PLACEMENT_3D; combined with body_projection=TOP and rotx=roty=rotz=0,
    // it rendered upside-down on the PCB.)
    //
    // Important: only strip the FREE-SHAPE-LABEL location. Do NOT touch the
    // locations of assembly components: those carry the relative placement of
    // sub-parts (e.g. pad positions around an IC body) and are part of the
    // model's geometry, not the file's coordinate system. Stripping them
    // collapses all sub-parts to the origin.
    for (int i = 1; i <= free_shapes.Length(); ++i)
    {
        TDF_Label label = free_shapes.Value(i);
        TopoDS_Shape shape = shape_tool->GetShape(label);
        if (shape.IsNull())
        {
            continue;
        }
        if (!shape.Location().IsIdentity())
        {
            XCAFDoc_Location::Set(label, TopLoc_Location());
            shape_tool->SetShape(label, shape.Located(TopLoc_Location()));
            shape = shape_tool->GetShape(label);
        }
        BRepMesh_IncrementalMesh mesher(shape, options.linear_deflection, false,
                                        options.angular_deflection, false);
    }

    RWGltf_CafWriter writer(TCollection_AsciiString(glb_path.c_str()), true);

    const bool write_ok = writer.Perform(
        doc, NCollection_IndexedDataMap<TCollection_AsciiString, TCollection_AsciiString>(),
        Message_ProgressRange());

    return write_ok ? 0 : 3;
}

int step_to_glb_from_bytes(const unsigned char* step_data, std::size_t step_size,
                           const StepToGlbOptions& options, std::vector<unsigned char>* glb_bytes,
                           Status* status)
{
    if (glb_bytes == nullptr)
    {
        set_status(status, 92, "GLB output buffer pointer is null.");
        return 92;
    }
    glb_bytes->clear();
    if (step_data == nullptr || step_size == 0)
    {
        set_status(status, 1, "STEP byte input is empty.");
        return 1;
    }

    const std::string step_path = temp_step_to_glb_path(".step");
    const std::string glb_path = temp_step_to_glb_path(".glb");
    if (!write_binary_file(step_path, step_data, step_size))
    {
        set_status(status, 1, "Failed writing STEP bytes to temporary input.");
        std::remove(step_path.c_str());
        std::remove(glb_path.c_str());
        return 1;
    }

    const int code = step_to_glb(step_path, glb_path, options);
    if (code != 0)
    {
        set_status(status, code, "STEP-to-GLB conversion failed.");
        std::remove(step_path.c_str());
        std::remove(glb_path.c_str());
        return code;
    }

    if (!read_binary_file(glb_path, glb_bytes))
    {
        set_status(status, 3, "Failed reading temporary GLB output.");
        std::remove(step_path.c_str());
        std::remove(glb_path.c_str());
        return 3;
    }

    std::remove(step_path.c_str());
    std::remove(glb_path.c_str());
    set_status(status, 0, "");
    return 0;
}

} // namespace geometer

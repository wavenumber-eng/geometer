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

int step_to_glb_from_bytes(const unsigned char* step_data, std::size_t step_size,
                           const StepToGlbOptions& options,
                           std::vector<unsigned char>* glb_bytes, Status* status)
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

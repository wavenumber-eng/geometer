#include "geometer/model_tessellation.h"

#include "model_tessellation_status.h"
#include "step_xcaf_root_frame.h"

#include <BRepMesh_Context.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <IMeshData_Face.hxx>
#include <IMeshData_Model.hxx>
#include <IMeshData_Wire.hxx>
#include <Quantity_Color.hxx>
#include <RWMesh_FaceIterator.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <STEPConstruct_ExternRefs.hxx>
#include <Standard_Failure.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TDocStd_Document.hxx>
#include <UnitsMethods_LengthUnit.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFPrs_DocumentExplorer.hxx>

#include <cmath>
#include <cstdint>
#include <exception>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>

namespace geometer
{
namespace
{
constexpr std::size_t kMaxVertices = 2000000;
constexpr std::size_t kMaxMeshes = 65536;

struct MeshProblems
{
    // Triangulations are attached to TShape, shared by every located occurrence.
    // A problematic definition must therefore be omitted at every placement.
    std::unordered_set<const TopoDS_TShape*> shapes;
    std::size_t faces = 0;
    std::vector<std::string> details;
};

MeshProblems inspect_mesh(const Handle(IMeshData_Model) & model)
{
    if (model.IsNull())
        throw std::runtime_error("STEP tessellation produced no meshing context.");
    MeshProblems problems;
    for (int index = 0; index < model->FacesNb(); ++index)
    {
        const auto& face = model->GetFace(index);
        int flags = face->GetStatusMask();
        for (int wire = 0; wire < face->WiresNb(); ++wire)
            flags |= face->GetWire(wire)->GetStatusMask();
        TopLoc_Location location;
        const auto triangulation = BRep_Tool::Triangulation(face->GetFace(), location);
        const bool missing = triangulation.IsNull() || triangulation->NbTriangles() == 0;
        if (!missing && (flags & ~model_tessellation_detail::successful_flags) == 0)
            continue;
        ++problems.faces;
        problems.shapes.insert(face->GetFace().TShape().get());
        if (problems.details.size() < 255)
            problems.details.push_back(
                "Omitted meshing face " + std::to_string(index) +
                ": OCCT status flags=" + std::to_string(flags) +
                (missing ? "; no triangles." : "; unreliable triangulation."));
    }
    return problems;
}

void append_vector(std::vector<double>* values, double x, double y, double z)
{
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))
        throw std::runtime_error("STEP tessellation produced non-finite geometry.");
    values->insert(values->end(), {x, y, z});
}

contracts::MeshIllustrationMesh face_mesh(const RWMesh_FaceIterator& face, std::size_t index)
{
    contracts::MeshIllustrationMesh mesh;
    mesh.id = "face-" + std::to_string(index);
    const auto vertices = static_cast<std::size_t>(face.NbNodes());
    mesh.positions.reserve(vertices * 3);
    if (face.HasNormals())
    {
        mesh.normals.emplace();
        mesh.normals->reserve(vertices * 3);
    }
    for (int node = face.NodeLower(); node <= face.NodeUpper(); ++node)
    {
        const gp_Pnt point = face.NodeTransformed(node);
        append_vector(&mesh.positions, point.X(), point.Y(), point.Z());
        if (mesh.normals)
        {
            const gp_Dir normal = face.NormalTransformed(node);
            append_vector(&*mesh.normals, normal.X(), normal.Y(), normal.Z());
        }
    }
    mesh.indices.emplace();
    mesh.indices->reserve(static_cast<std::size_t>(face.NbTriangles()) * 3);
    for (int element = face.ElemLower(); element <= face.ElemUpper(); ++element)
    {
        const Poly_Triangle triangle = face.TriangleOriented(element);
        for (int corner = 1; corner <= 3; ++corner)
        {
            const int node = triangle.Value(corner) - face.NodeLower();
            if (node < 0 || static_cast<std::size_t>(node) >= vertices)
                throw std::runtime_error("STEP tessellation produced an invalid vertex index.");
            mesh.indices->push_back(static_cast<std::uint32_t>(node));
        }
    }
    contracts::MeshIllustrationMaterial material;
    material.color = {0.72, 0.74, 0.78};
    material.opacity = 1.0;
    if (face.HasFaceColor())
    {
        double red = 0, green = 0, blue = 0;
        face.FaceColor().GetRGB().Values(red, green, blue, Quantity_TOC_sRGB);
        material.color = {red, green, blue};
        material.opacity = face.FaceColor().Alpha();
    }
    mesh.materials.push_back(std::move(material));
    return mesh;
}

void collect_meshes(const Handle(TDocStd_Document) & document, std::size_t max_triangles,
                    const MeshProblems& problems, contracts::MeshCollectionA0* collection)
{
    std::size_t triangles = 0;
    std::size_t vertices = 0;
    std::size_t occurrences = 0;
    for (XCAFPrs_DocumentExplorer nodes(document, XCAFPrs_DocumentExplorerFlags_OnlyLeafNodes);
         nodes.More(); nodes.Next())
    {
        if (++occurrences > kMaxMeshes)
            throw std::length_error("STEP tessellation exceeds the 65536 occurrence limit.");
        const auto& node = nodes.Current();
        if (!node.Style.IsVisible())
            continue;
        for (RWMesh_FaceIterator face(node.RefLabel, node.Location, true, node.Style); face.More();
             face.Next())
        {
            if (!face.FaceStyle().IsVisible() || face.NbTriangles() == 0 ||
                problems.shapes.count(face.Face().TShape().get()) != 0)
                continue;
            const auto count = static_cast<std::size_t>(face.NbTriangles());
            const auto node_count = static_cast<std::size_t>(face.NbNodes());
            if (count > max_triangles - triangles || node_count > kMaxVertices - vertices ||
                collection->meshes.size() >= kMaxMeshes)
                throw std::length_error(
                    "STEP tessellation exceeds its triangle, vertex or mesh limit.");
            triangles += count;
            vertices += node_count;
            collection->meshes.push_back(face_mesh(face, collection->meshes.size()));
        }
    }
    if (collection->meshes.empty())
        throw std::runtime_error("STEP tessellation produced no visible triangle meshes.");
}
} // namespace

int model_tessellation_from_bytes(const unsigned char* data, std::size_t size,
                                  const contracts::ModelTessellationRequestA0& options,
                                  contracts::MeshCollectionA0* meshes, Status* status,
                                  std::vector<std::string>* warnings)
{
    if (warnings)
        warnings->clear();
    const auto fail = [&](int code, const std::string& message)
    {
        if (meshes)
            *meshes = {};
        if (status)
        {
            status->code = code;
            status->message = message;
        }
        return code;
    };
    if (!meshes || !data || size == 0 || size > 268435456)
        return fail(
            1, "STEP tessellation requires nonempty model bytes (at most 256 MiB) and an output.");
    try
    {
        std::string validated;
        contracts::ContractError error;
        if (!contracts::encode_json(options, &validated, &error))
            return fail(1, error.message);
        std::istringstream stream(std::string(reinterpret_cast<const char*>(data), size));
        STEPCAFControl_Reader reader;
        reader.SetColorMode(true);
        reader.SetNameMode(true);
        reader.SetMatMode(true);
        if (reader.ReadStream("memory.step", stream) != IFSelect_RetDone)
            return fail(1, "Failed reading STEP bytes for tessellation.");
        STEPConstruct_ExternRefs external(reader.Reader().WS());
        external.LoadExternRefs();
        if (external.NbExternRefs() != 0)
            return fail(103, "External STEP file references are unsupported; supply one "
                             "self-contained model attachment.");
        Handle(TDocStd_Document) document =
            new TDocStd_Document(TCollection_ExtendedString("XmlOcaf"));
        XCAFDoc_DocumentTool::SetLengthUnit(document, 1.0, UnitsMethods_LengthUnit_Millimeter);
        if (!reader.Transfer(document))
            return fail(1, "Failed transferring STEP geometry and colors.");
        if (options.root_placement.value_or(contracts::ModelRootPlacement::strip) ==
            contracts::ModelRootPlacement::strip)
            strip_free_shape_root_locations(document);
        const auto shape = free_shape_compound(document);
        if (shape.IsNull())
            return fail(1, "STEP transfer produced no shapes.");
        Handle(BRepMesh_Context) context = new BRepMesh_Context();
        BRepMesh_IncrementalMesh mesher;
        mesher.SetShape(shape);
        auto& parameters = mesher.ChangeParameters();
        parameters.Deflection = options.linear_deflection_mm.value_or(0.1);
        parameters.Angle = options.angular_deflection_rad.value_or(0.5);
        parameters.Relative = false;
        parameters.InParallel = false;
        parameters.CleanModel = false;
        mesher.Perform(context);
        const bool partial = options.allow_partial.value_or(true);
        int flags = mesher.GetStatusFlags();
        if (mesher.IsDone() && (flags & IMeshData_Outdated) != 0)
        {
            // Some STEP files carry triangulations whose deflection is coarser
            // than requested. Retry once after removing those stale meshes;
            // otherwise OCCT reports Outdated beside recoverable face errors.
            context = new BRepMesh_Context();
            parameters.CleanModel = true;
            mesher.Perform(context);
            flags = mesher.GetStatusFlags();
        }
        if (!model_tessellation_detail::meshing_succeeded(mesher.IsDone(), flags, true))
            return fail(
                1, "STEP meshing did not complete safely: done=" + std::to_string(mesher.IsDone()) +
                       "; OCCT status flags=" + std::to_string(flags) + ".");
        const auto problems = inspect_mesh(context->GetModel());
        if (!partial &&
            (!model_tessellation_detail::meshing_succeeded(true, flags) || problems.faces))
            return fail(
                1, "STEP tessellation has " + std::to_string(problems.faces) +
                       " problematic meshing face(s); OCCT status flags=" + std::to_string(flags) +
                       "; partial meshes are not returned "
                       "unless allow_partial=true.");
        contracts::MeshCollectionA0 output;
        collect_meshes(document, options.max_triangles.value_or(750000), problems, &output);
        if (warnings && problems.faces)
        {
            warnings->push_back(
                "Partial STEP tessellation: omitted " + std::to_string(problems.faces) + " of " +
                std::to_string(context->GetModel()->FacesNb()) +
                " meshing faces; OCCT status flags=" + std::to_string(flags) +
                ". Face indices are local to this tessellation; details capped at 255.");
            warnings->insert(warnings->end(), problems.details.begin(), problems.details.end());
        }
        *meshes = std::move(output);
        if (status)
        {
            status->code = 0;
            status->message.clear();
        }
        return 0;
    }
    catch (const std::length_error& error)
    {
        return fail(102, error.what());
    }
    catch (const Standard_Failure& error)
    {
        return fail(1, error.GetMessageString());
    }
    catch (const std::exception& error)
    {
        return fail(1, error.what());
    }
}
} // namespace geometer

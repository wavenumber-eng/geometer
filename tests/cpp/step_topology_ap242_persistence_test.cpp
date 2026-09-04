#include "geometer/sha256.h"

#include <BRepAdaptor_Surface.hxx>
#include <BRepBndLib.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRep_Builder.hxx>
#include <BinXCAFDrivers.hxx>
#include <Bnd_Box.hxx>
#include <DESTEP_Parameters.hxx>
#include <GProp_GProps.hxx>
#include <HeaderSection_FileSchema.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <Interface_Check.hxx>
#include <Interface_CheckIterator.hxx>
#include <Interface_CheckTool.hxx>
#include <Interface_EntityIterator.hxx>
#include <Interface_Graph.hxx>
#include <NCollection_Sequence.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <STEPCAFControl_Writer.hxx>
#include <StepAP242_GeometricItemSpecificUsage.hxx>
#include <StepBasic_GeneralProperty.hxx>
#include <StepBasic_GeneralPropertyAssociation.hxx>
#include <StepBasic_ProductDefinition.hxx>
#include <StepData_Protocol.hxx>
#include <StepData_StepModel.hxx>
#include <StepData_StepWriter.hxx>
#include <StepRepr_DescriptiveRepresentationItem.hxx>
#include <StepRepr_PropertyDefinitionRepresentation.hxx>
#include <StepRepr_Representation.hxx>
#include <StepRepr_ShapeAspect.hxx>
#include <StepShape_AdvancedFace.hxx>
#include <StepShape_ManifoldSolidBrep.hxx>
#include <StepShape_ShapeDefinitionRepresentation.hxx>
#include <StepShape_ShapeRepresentation.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TCollection_HAsciiString.hxx>
#include <TDataStd_Name.hxx>
#include <TDataStd_NamedData.hxx>
#include <TDocStd_Document.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Shape.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XSControl_TransferReader.hxx>
#include <XSControl_TransferWriter.hxx>
#include <XSControl_WorkSession.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>

#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace
{

constexpr const char* kMetadataKey = "wn.geometer.research.annotation.summary";
constexpr const char* kMetadataValue =
    R"({"schema":"wn.geometer.research.ap242-carrier.a0","target_count":3,"logical_group":"wn.geometer.research.group.two-faces"})";
constexpr const char* kBodyId = "wn.geometer.research.annotation.body.demo";
constexpr const char* kFaceAId = "wn.geometer.research.annotation.face.demo-a";
constexpr const char* kFaceBId = "wn.geometer.research.annotation.face.demo-b";
constexpr const char* kBodyPayload =
    R"({"schema":"wn.geometer.research.ap242-link.a0","target":"body"})";
constexpr const char* kFaceAPayload =
    R"({"schema":"wn.geometer.research.ap242-link.a0","target":"face","logical_group":"wn.geometer.research.group.two-faces","member":0})";
constexpr const char* kFaceBPayload =
    R"({"schema":"wn.geometer.research.ap242-link.a0","target":"face","logical_group":"wn.geometer.research.group.two-faces","member":1})";

struct TemporaryDirectory
{
    std::filesystem::path path;

    explicit TemporaryDirectory(const std::string& prefix)
    {
        const auto nonce = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        for (unsigned int attempt = 0; attempt < 256; ++attempt)
        {
            path = std::filesystem::temp_directory_path() /
                   (prefix + std::to_string(nonce) + "-" + std::to_string(attempt));
            std::error_code error;
            if (std::filesystem::create_directory(path, error))
                return;
            if (error)
                throw std::filesystem::filesystem_error(
                    "failed creating isolated temporary directory", path, error);
        }
        throw std::runtime_error("failed finding an unused isolated temporary directory");
    }

    TemporaryDirectory(const TemporaryDirectory&) = delete;
    TemporaryDirectory& operator=(const TemporaryDirectory&) = delete;

    ~TemporaryDirectory()
    {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }
};

struct GeometryEvidence
{
    std::size_t face_count = 0;
    double volume = 0.0;
    double surface_area = 0.0;
    double center_x = 0.0;
    double center_y = 0.0;
    double center_z = 0.0;
};

struct CarrierEvidence
{
    std::size_t body_count = 0;
    std::size_t face_count = 0;
    std::size_t metadata_graph_count = 0;
};

struct FaceEvidence
{
    double area = 0.0;
    std::array<double, 3> center{};
    std::array<double, 6> bounds{};
    GeomAbs_SurfaceType surface_type = GeomAbs_OtherSurface;
};

class ResearchStepWriter : public STEPCAFControl_Writer
{
  public:
    occ::handle<StepRepr_ShapeAspect> add_topology_link(
        const occ::handle<XSControl_WorkSession>& work_session, const TDF_Label& label,
        const TopoDS_Shape& shape, const std::string& authored_id, const std::string& role,
        const std::string& payload, occ::handle<StepAP242_GeometricItemSpecificUsage>* usage)
    {
        occ::handle<StepRepr_RepresentationContext> context;
        occ::handle<StepAP242_GeometricItemSpecificUsage> local_usage;
        const occ::handle<StepRepr_ShapeAspect> aspect =
            writeShapeAspect(work_session, label, shape, context, local_usage);
        if (!aspect.IsNull() && !local_usage.IsNull())
        {
            const occ::handle<TCollection_HAsciiString> id =
                new TCollection_HAsciiString(authored_id.c_str());
            aspect->SetName(id);
            aspect->SetDescription(new TCollection_HAsciiString(role.c_str()));
            local_usage->SetName(id);
            local_usage->SetDescription(new TCollection_HAsciiString(payload.c_str()));
        }
        *usage = local_usage;
        return aspect;
    }
};

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void require_no_failures(const Interface_CheckIterator& checks, const std::string& stage)
{
    if (!checks.IsEmpty(true))
    {
        std::ostringstream detail;
        checks.Print(detail, true);
        throw std::runtime_error(stage + " reported failures:\n" + detail.str());
    }
}

GeometryEvidence geometry_evidence(const TopoDS_Shape& shape)
{
    require(!shape.IsNull() && BRepCheck_Analyzer(shape).IsValid(),
            "geometry evidence requires a valid B-rep");
    GeometryEvidence result;
    for (TopExp_Explorer explorer(shape, TopAbs_FACE); explorer.More(); explorer.Next())
        ++result.face_count;
    GProp_GProps volume_properties;
    BRepGProp::VolumeProperties(shape, volume_properties);
    result.volume = volume_properties.Mass();
    const gp_Pnt center = volume_properties.CentreOfMass();
    result.center_x = center.X();
    result.center_y = center.Y();
    result.center_z = center.Z();
    GProp_GProps surface_properties;
    BRepGProp::SurfaceProperties(shape, surface_properties);
    result.surface_area = surface_properties.Mass();
    return result;
}

void require_geometry_equal(const GeometryEvidence& expected, const GeometryEvidence& actual,
                            const std::string& stage)
{
    constexpr double tolerance = 1.0e-8;
    require(expected.face_count == actual.face_count &&
                std::abs(expected.volume - actual.volume) <= tolerance &&
                std::abs(expected.surface_area - actual.surface_area) <= tolerance &&
                std::abs(expected.center_x - actual.center_x) <= tolerance &&
                std::abs(expected.center_y - actual.center_y) <= tolerance &&
                std::abs(expected.center_z - actual.center_z) <= tolerance,
            stage + " changed the per-body geometry/property fingerprint");
}

std::string geometry_fingerprint(const GeometryEvidence& evidence)
{
    std::ostringstream canonical;
    canonical << evidence.face_count << '|' << std::fixed << std::setprecision(8) << evidence.volume
              << '|' << evidence.surface_area << '|' << evidence.center_x << '|'
              << evidence.center_y << '|' << evidence.center_z;
    const std::string bytes = canonical.str();
    return geometer::sha256_hex(reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size());
}

FaceEvidence face_evidence(const TopoDS_Shape& face)
{
    require(!face.IsNull() && face.ShapeType() == TopAbs_FACE, "face evidence requires a face");
    FaceEvidence evidence;
    GProp_GProps properties;
    BRepGProp::SurfaceProperties(face, properties);
    evidence.area = properties.Mass();
    const gp_Pnt center = properties.CentreOfMass();
    evidence.center = {center.X(), center.Y(), center.Z()};
    Bnd_Box bounds;
    BRepBndLib::Add(face, bounds);
    bounds.Get(evidence.bounds[0], evidence.bounds[1], evidence.bounds[2], evidence.bounds[3],
               evidence.bounds[4], evidence.bounds[5]);
    evidence.surface_type = BRepAdaptor_Surface(TopoDS::Face(face)).GetType();
    return evidence;
}

bool same_face_evidence(const FaceEvidence& left, const FaceEvidence& right)
{
    constexpr double tolerance = 1.0e-8;
    if (left.surface_type != right.surface_type || std::abs(left.area - right.area) > tolerance)
        return false;
    for (std::size_t index = 0; index < left.center.size(); ++index)
    {
        if (std::abs(left.center[index] - right.center[index]) > tolerance)
            return false;
    }
    for (std::size_t index = 0; index < left.bounds.size(); ++index)
    {
        if (std::abs(left.bounds[index] - right.bounds[index]) > tolerance)
            return false;
    }
    return true;
}

void require_ap242_schema(const occ::handle<StepData_StepModel>& model, const std::string& stage)
{
    const occ::handle<HeaderSection_FileSchema> schema = occ::down_cast<HeaderSection_FileSchema>(
        model->HeaderEntity(STANDARD_TYPE(HeaderSection_FileSchema)));
    require(!schema.IsNull() && schema->NbSchemaIdentifiers() == 1,
            stage + " has no singular FILE_SCHEMA identifier");
    const std::string identifier = schema->SchemaIdentifiersValue(1)->ToCString();
    require(identifier.find("AP242_MANAGED_MODEL_BASED_3D_ENGINEERING_MIM_LF") != std::string::npos,
            stage + " did not use the requested AP242 schema");
}

occ::handle<StepShape_ShapeDefinitionRepresentation>
find_shape_definition(const occ::handle<StepData_StepModel>& model,
                      const occ::handle<StepRepr_ProductDefinitionShape>& product_shape,
                      const occ::handle<StepRepr_Representation>& representation)
{
    for (int index = 1; index <= model->NbEntities(); ++index)
    {
        const occ::handle<StepShape_ShapeDefinitionRepresentation> shape_definition =
            occ::down_cast<StepShape_ShapeDefinitionRepresentation>(model->Value(index));
        if (shape_definition.IsNull() || shape_definition->UsedRepresentation() != representation)
            continue;
        const occ::handle<StepRepr_PropertyDefinition> property_definition =
            shape_definition->Definition().PropertyDefinition();
        if (property_definition == product_shape)
            return shape_definition;
    }
    return nullptr;
}

bool representation_reaches(const occ::handle<StepData_StepModel>& model,
                            const occ::handle<StepRepr_Representation>& representation,
                            const occ::handle<StepRepr_RepresentationItem>& identified)
{
    Interface_Graph graph(model);
    std::vector<occ::handle<Standard_Transient>> pending;
    pending.emplace_back(representation.get());
    std::unordered_set<int> visited;
    while (!pending.empty())
    {
        const occ::handle<Standard_Transient> current = pending.back();
        pending.pop_back();
        if (current.get() == identified.get())
            return true;
        const int number = model->Number(current);
        if (number <= 0 || !visited.insert(number).second)
            continue;
        for (Interface_EntityIterator shared = graph.Shareds(current); shared.More(); shared.Next())
            pending.push_back(shared.Value());
    }
    return false;
}

CarrierEvidence inspect_entity_graph(const occ::handle<StepData_StepModel>& model)
{
    CarrierEvidence evidence;
    occ::handle<StepBasic_ProductDefinition> metadata_product;
    occ::handle<StepBasic_ProductDefinition> body_product;
    occ::handle<StepBasic_ProductDefinition> face_a_product;
    occ::handle<StepBasic_ProductDefinition> face_b_product;
    occ::handle<StepRepr_RepresentationItem> face_a_item;
    occ::handle<StepRepr_RepresentationItem> face_b_item;
    for (int index = 1; index <= model->NbEntities(); ++index)
    {
        const occ::handle<Standard_Transient> entity = model->Value(index);
        const occ::handle<StepBasic_GeneralPropertyAssociation> association =
            occ::down_cast<StepBasic_GeneralPropertyAssociation>(entity);
        if (!association.IsNull() && !association->GeneralProperty().IsNull() &&
            !association->GeneralProperty()->Name().IsNull() &&
            association->GeneralProperty()->Name()->String() == kMetadataKey)
        {
            const occ::handle<StepRepr_PropertyDefinition> property_definition =
                association->PropertyDefinition();
            require(
                !property_definition.IsNull() && !property_definition->Name().IsNull() &&
                    property_definition->Name()->String() == kMetadataKey,
                "general-property association does not reference the named property definition");
            metadata_product = property_definition->Definition().ProductDefinition();
            require(!metadata_product.IsNull(),
                    "namespaced metadata property does not characterize a product definition");
            std::size_t representation_count = 0;
            for (int candidate_index = 1; candidate_index <= model->NbEntities(); ++candidate_index)
            {
                const occ::handle<StepRepr_PropertyDefinitionRepresentation>
                    property_representation =
                        occ::down_cast<StepRepr_PropertyDefinitionRepresentation>(
                            model->Value(candidate_index));
                if (property_representation.IsNull() ||
                    property_representation->Definition().PropertyDefinition() !=
                        property_definition)
                    continue;
                ++representation_count;
                const occ::handle<StepRepr_Representation> representation =
                    property_representation->UsedRepresentation();
                require(!representation.IsNull() && representation->NbItems() == 1,
                        "metadata property representation must contain exactly one payload item");
                const occ::handle<StepRepr_DescriptiveRepresentationItem> payload =
                    occ::down_cast<StepRepr_DescriptiveRepresentationItem>(
                        representation->ItemsValue(1));
                require(!payload.IsNull() && !payload->Name().IsNull() &&
                            !payload->Description().IsNull() &&
                            payload->Name()->String() == kMetadataKey &&
                            payload->Description()->String() == kMetadataValue,
                        "metadata property representation has the wrong payload");
                require(model->Number(association) > 0 &&
                            model->Number(association->GeneralProperty()) > 0 &&
                            model->Number(property_definition) > 0 &&
                            model->Number(property_representation) > 0 &&
                            model->Number(representation) > 0 && model->Number(payload) > 0,
                        "metadata carrier graph references an entity outside the STEP model");
            }
            require(representation_count == 1,
                    "metadata property must have exactly one property-definition representation");
            ++evidence.metadata_graph_count;
        }

        const occ::handle<StepAP242_GeometricItemSpecificUsage> usage =
            occ::down_cast<StepAP242_GeometricItemSpecificUsage>(entity);
        if (usage.IsNull())
            continue;
        require(!usage->Name().IsNull() && !usage->Description().IsNull(),
                "GISU annotation link has no name or payload");
        const std::string id = usage->Name()->ToCString();
        const bool is_body = id == kBodyId;
        const bool is_face_a = id == kFaceAId;
        const bool is_face_b = id == kFaceBId;
        const bool is_face = is_face_a || is_face_b;
        if (!is_body && !is_face)
            continue;
        const std::string expected_payload =
            is_body ? kBodyPayload : (is_face_a ? kFaceAPayload : kFaceBPayload);
        require(usage->Description()->String() == expected_payload &&
                    usage->NbIdentifiedItem() == 1,
                "GISU annotation link payload or identified-item cardinality changed");
        const occ::handle<StepRepr_ShapeAspect> aspect = usage->Definition().ShapeAspect();
        const occ::handle<StepRepr_RepresentationItem> identified = usage->IdentifiedItemValue(1);
        require(
            !aspect.IsNull() && !aspect->Name().IsNull() && aspect->Name()->String() == id &&
                !aspect->OfShape().IsNull() && aspect->ProductDefinitional() == StepData_LTrue &&
                !usage->UsedRepresentation().IsNull() &&
                usage->UsedRepresentation()->IsKind(STANDARD_TYPE(StepShape_ShapeRepresentation)) &&
                !identified.IsNull(),
            "GISU annotation link violates its required shape-aspect relationship");
        require(model->Number(aspect) > 0 && model->Number(aspect->OfShape()) > 0 &&
                    model->Number(usage->UsedRepresentation()) > 0 && model->Number(identified) > 0,
                "GISU relationship references an entity outside the STEP model");
        const occ::handle<StepShape_ShapeDefinitionRepresentation> shape_definition =
            find_shape_definition(model, aspect->OfShape(), usage->UsedRepresentation());
        require(!shape_definition.IsNull(),
                "GISU used representation is not connected to the shape-aspect product shape");
        require((is_body && identified->IsKind(STANDARD_TYPE(StepShape_ManifoldSolidBrep))) ||
                    (is_face && identified->IsKind(STANDARD_TYPE(StepShape_AdvancedFace))),
                "GISU identified item has the wrong body/face STEP entity type");
        require(
            representation_reaches(model, usage->UsedRepresentation(), identified),
            "GISU identified item is not in the transitive content of its shape representation");
        if (is_body)
        {
            ++evidence.body_count;
            body_product = aspect->OfShape()->Definition().ProductDefinition();
        }
        else
        {
            ++evidence.face_count;
            if (is_face_a)
            {
                face_a_product = aspect->OfShape()->Definition().ProductDefinition();
                face_a_item = identified;
            }
            else
            {
                face_b_product = aspect->OfShape()->Definition().ProductDefinition();
                face_b_item = identified;
            }
        }
    }
    require(evidence.body_count == 1 && evidence.face_count == 2 &&
                evidence.metadata_graph_count == 1,
            "carrier graph cardinality changed");
    require(
        !body_product.IsNull() && !face_a_product.IsNull() && !face_b_product.IsNull() &&
            body_product == metadata_product && face_a_product == metadata_product &&
            face_b_product == metadata_product,
        "product metadata, body GISU, and both face GISUs do not characterize the same product");
    require(!face_a_item.IsNull() && !face_b_item.IsNull() && face_a_item != face_b_item &&
                model->Number(face_a_item) != model->Number(face_b_item),
            "logical-group face GISUs do not identify distinct STEP representation items");
    return evidence;
}

void require_reloaded_target_resolution(const occ::handle<StepData_StepModel>& model,
                                        const occ::handle<XSControl_WorkSession>& work_session,
                                        const TopoDS_Shape& restored_definition)
{
    const occ::handle<XSControl_TransferReader> transfer_reader = work_session->TransferReader();
    std::size_t explicit_transfer_count = 0;
    TopoDS_Shape resolved_face_a;
    TopoDS_Shape resolved_face_b;
    for (int index = 1; index <= model->NbEntities(); ++index)
    {
        const occ::handle<StepAP242_GeometricItemSpecificUsage> usage =
            occ::down_cast<StepAP242_GeometricItemSpecificUsage>(model->Value(index));
        if (usage.IsNull() || usage->Name().IsNull())
            continue;
        const std::string id = usage->Name()->ToCString();
        if (id != kBodyId && id != kFaceAId && id != kFaceBId)
            continue;
        const occ::handle<StepRepr_RepresentationItem> identified = usage->IdentifiedItemValue(1);
        TopoDS_Shape resolved = transfer_reader->ShapeResult(identified);
        require(resolved.IsNull(),
                "root transfer unexpectedly pre-bound a research GISU identified item");
        require(work_session->TransferReadOne(identified) > 0,
                "explicit OCCT transfer of a GISU identified item failed");
        ++explicit_transfer_count;
        require_no_failures(transfer_reader->LastCheckList(),
                            "targeted GISU identified-item transfer");
        resolved = transfer_reader->ShapeResult(identified);
        if (id == kFaceAId || id == kFaceBId)
        {
            require(!resolved.IsNull() && resolved.ShapeType() == TopAbs_FACE,
                    "face GISU did not resolve through an explicit OCCT entity transfer");
            const FaceEvidence resolved_evidence = face_evidence(resolved);
            std::size_t corresponding_faces = 0;
            for (TopExp_Explorer explorer(restored_definition, TopAbs_FACE); explorer.More();
                 explorer.Next())
            {
                if (same_face_evidence(resolved_evidence, face_evidence(explorer.Current())))
                    ++corresponding_faces;
            }
            require(corresponding_faces == 1,
                    "face GISU does not correspond uniquely to a face of the restored body");
            if (id == kFaceAId)
                resolved_face_a = resolved;
            else
                resolved_face_b = resolved;
        }
        else if (id == kBodyId)
        {
            std::size_t represented_body_count = 0;
            bool identified_body_is_represented = false;
            const occ::handle<StepRepr_Representation> representation = usage->UsedRepresentation();
            for (int item_index = 1; item_index <= representation->NbItems(); ++item_index)
            {
                const occ::handle<StepRepr_RepresentationItem> item =
                    representation->ItemsValue(item_index);
                if (item->IsKind(STANDARD_TYPE(StepShape_ManifoldSolidBrep)))
                {
                    ++represented_body_count;
                    identified_body_is_represented = item == identified;
                }
            }
            std::size_t restored_solid_count = 0;
            for (TopExp_Explorer explorer(restored_definition, TopAbs_SOLID); explorer.More();
                 explorer.Next())
                ++restored_solid_count;
            require(represented_body_count == 1 && identified_body_is_represented &&
                        restored_solid_count == 1 && !resolved.IsNull() &&
                        resolved.ShapeType() == TopAbs_SOLID,
                    "body GISU could not resolve uniquely through its product representation");
        }
    }
    require(explicit_transfer_count == 3,
            "all three research GISU targets must require and complete explicit transfer");
    require(!resolved_face_a.IsNull() && !resolved_face_b.IsNull() &&
                !resolved_face_a.IsSame(resolved_face_b) &&
                !same_face_evidence(face_evidence(resolved_face_a), face_evidence(resolved_face_b)),
            "logical-group face GISUs do not resolve to distinct restored faces");
}

void require_all_carriers(const CarrierEvidence& evidence, const std::string& stage)
{
    require(evidence.body_count == 1 && evidence.face_count == 2 &&
                evidence.metadata_graph_count == 1,
            stage + " did not contain the product, body, and two-face group carrier graph");
}

NCollection_Sequence<TDF_Label> direct_components(const TDF_Label& assembly,
                                                  std::size_t expected_count)
{
    NCollection_Sequence<TDF_Label> components;
    require(XCAFDoc_ShapeTool::GetComponents(assembly, components, false) &&
                components.Length() == static_cast<Standard_Integer>(expected_count),
            "reloaded AP242 assembly has the wrong direct component count");
    return components;
}

} // namespace

int main()
{
    try
    {
        const occ::handle<XCAFApp_Application> application = XCAFApp_Application::GetApplication();
        BinXCAFDrivers::DefineFormat(application);
        occ::handle<TDocStd_Document> source_document;
        application->NewDocument("BinXCAF", source_document);
        require(!source_document.IsNull(), "failed creating AP242 source XCAF document");
        const occ::handle<XCAFDoc_ShapeTool> shape_tool =
            XCAFDoc_DocumentTool::ShapeTool(source_document->Main());

        const TopoDS_Shape box = BRepPrimAPI_MakeBox(10.0, 20.0, 30.0).Shape();
        const GeometryEvidence original_geometry = geometry_evidence(box);
        const std::string original_fingerprint = geometry_fingerprint(original_geometry);
        const TDF_Label definition = shape_tool->AddShape(box, false);
        TDataStd_Name::Set(definition, TCollection_ExtendedString("AP242 research box", true));
        const occ::handle<TDataStd_NamedData> metadata = TDataStd_NamedData::Set(definition);
        metadata->SetString(TCollection_ExtendedString(kMetadataKey, true),
                            TCollection_ExtendedString(kMetadataValue, true));

        TopoDS_Compound root_shape;
        TopoDS_Compound subassembly_shape;
        BRep_Builder builder;
        builder.MakeCompound(root_shape);
        builder.MakeCompound(subassembly_shape);
        const TDF_Label root_assembly = shape_tool->AddShape(root_shape, true);
        const TDF_Label subassembly = shape_tool->AddShape(subassembly_shape, true);
        shape_tool->AddComponent(root_assembly, definition, TopLoc_Location());
        gp_Trsf subassembly_transform;
        subassembly_transform.SetTranslation(gp_Vec(10.0, 0.0, 0.0));
        shape_tool->AddComponent(root_assembly, subassembly,
                                 TopLoc_Location(subassembly_transform));
        gp_Trsf nested_definition_transform;
        nested_definition_transform.SetTranslation(gp_Vec(30.0, 0.0, 0.0));
        shape_tool->AddComponent(subassembly, definition,
                                 TopLoc_Location(nested_definition_transform));
        shape_tool->UpdateAssemblies();
        const GeometryEvidence original_root_geometry =
            geometry_evidence(XCAFDoc_ShapeTool::GetShape(root_assembly));
        require(original_root_geometry.face_count == original_geometry.face_count * 2,
                "source AP242 hierarchy did not materialize both occurrences");

        TopExp_Explorer face_explorer(box, TopAbs_FACE);
        require(face_explorer.More(), "generated AP242 source has no face");
        const TopoDS_Shape selected_face_a = face_explorer.Current();
        face_explorer.Next();
        require(face_explorer.More(), "generated AP242 source has fewer than two faces");
        const TopoDS_Shape selected_face_b = face_explorer.Current();
        const TDF_Label face_a_label = shape_tool->AddSubShape(definition, selected_face_a);
        const TDF_Label face_b_label = shape_tool->AddSubShape(definition, selected_face_b);
        require(!face_a_label.IsNull() && !face_b_label.IsNull() && face_a_label != face_b_label,
                "failed adding AP242 face subshape labels");

        ResearchStepWriter writer;
        writer.SetMetadataMode(true);
        writer.SetNameMode(true);
        DESTEP_Parameters write_parameters;
        write_parameters.WriteSchema = DESTEP_Parameters::WriteMode_StepSchema_AP242DIS;
        write_parameters.WriteMetadata = true;
        write_parameters.WriteName = true;
        write_parameters.WriteSubshapeNames = true;
        require(writer.Transfer(source_document, write_parameters),
                "AP242 XCAF-to-STEP transfer failed");
        const occ::handle<XSControl_WorkSession> writer_session = writer.ChangeWriter().WS();
        require_no_failures(writer_session->TransferWriter()->CheckList(), "writer transfer");

        occ::handle<StepAP242_GeometricItemSpecificUsage> body_usage;
        occ::handle<StepAP242_GeometricItemSpecificUsage> face_a_usage;
        occ::handle<StepAP242_GeometricItemSpecificUsage> face_b_usage;
        const occ::handle<StepRepr_ShapeAspect> body_aspect = writer.add_topology_link(
            writer_session, definition, box, kBodyId, "body", kBodyPayload, &body_usage);
        const occ::handle<StepRepr_ShapeAspect> face_a_aspect =
            writer.add_topology_link(writer_session, face_a_label, selected_face_a, kFaceAId,
                                     "logical-group-face-member", kFaceAPayload, &face_a_usage);
        const occ::handle<StepRepr_ShapeAspect> face_b_aspect =
            writer.add_topology_link(writer_session, face_b_label, selected_face_b, kFaceBId,
                                     "logical-group-face-member", kFaceBPayload, &face_b_usage);
        require(!body_aspect.IsNull() && !body_usage.IsNull() && !face_a_aspect.IsNull() &&
                    !face_a_usage.IsNull() && !face_b_aspect.IsNull() && !face_b_usage.IsNull(),
                "OCCT could not construct AP242 topology links for body and two face members");

        const occ::handle<StepData_StepModel> writer_model = writer.ChangeWriter().Model(false);
        require_ap242_schema(writer_model, "writer model");
        require_all_carriers(inspect_entity_graph(writer_model), "writer model");
        occ::handle<Interface_Check> model_check = new Interface_Check();
        writer_model->VerifyCheck(model_check);
        require(!model_check->HasFailed(), "writer STEP header integrity check failed");
        Interface_CheckTool writer_check_tool(writer_model);
        require_no_failures(writer_check_tool.VerifyCheckList(),
                            "writer model implemented integrity check");

        const occ::handle<StepData_Protocol> protocol =
            occ::down_cast<StepData_Protocol>(writer_model->Protocol());
        require(!protocol.IsNull(), "writer model has no STEP protocol");
        StepData_StepWriter direct_step_writer(writer_model);
        direct_step_writer.SendModel(protocol);
        require_no_failures(direct_step_writer.CheckList(), "StepData serialization preflight");

        TemporaryDirectory temporary("geometer-ap242-persistence-");
        const std::filesystem::path artifact = temporary.path / "annotated.step";
        const std::string artifact_utf8 = artifact.u8string();
        require(writer.Write(artifact_utf8.c_str()) == IFSelect_RetDone &&
                    std::filesystem::is_regular_file(artifact) &&
                    std::filesystem::file_size(artifact) > 0,
                "AP242 write failed");

        STEPCAFControl_Reader reader;
        reader.SetMetaMode(true);
        reader.SetNameMode(true);
        DESTEP_Parameters read_parameters;
        read_parameters.ReadMetadata = true;
        read_parameters.ReadShapeAspect = true;
        read_parameters.ReadSubshapeNames = true;
        require(reader.ReadFile(artifact_utf8.c_str(), read_parameters) == IFSelect_RetDone,
                "AP242 read failed");
        const occ::handle<XSControl_WorkSession> reader_session = reader.ChangeReader().WS();
        const occ::handle<StepData_StepModel> reader_model =
            occ::down_cast<StepData_StepModel>(reader_session->Model());
        require_ap242_schema(reader_model, "reader model");
        Interface_CheckTool reader_check_tool(reader_model);
        require_no_failures(reader_check_tool.AnalyseCheckList(), "reader syntax check");
        require_no_failures(reader_check_tool.VerifyCheckList(),
                            "reader model implemented integrity check");

        occ::handle<TDocStd_Document> restored_document;
        application->NewDocument("BinXCAF", restored_document);
        require(reader.Transfer(restored_document), "AP242 STEP-to-XCAF transfer failed");
        require_no_failures(reader_session->TransferReader()->LastCheckList(), "reader transfer");
        require_all_carriers(inspect_entity_graph(reader_model), "reader model");

        const occ::handle<XCAFDoc_ShapeTool> restored_shape_tool =
            XCAFDoc_DocumentTool::ShapeTool(restored_document->Main());
        NCollection_Sequence<TDF_Label> roots;
        restored_shape_tool->GetFreeShapes(roots);
        require(roots.Length() == 1 && XCAFDoc_ShapeTool::IsAssembly(roots.Value(1)),
                "AP242 reload lost the root assembly");
        const TDF_Label restored_root = roots.Value(1);
        const NCollection_Sequence<TDF_Label> root_components = direct_components(restored_root, 2);
        TDF_Label restored_occurrence_a;
        TDF_Label restored_subassembly_occurrence;
        TDF_Label restored_definition;
        TDF_Label restored_subassembly;
        for (const TDF_Label& component : root_components)
        {
            TDF_Label referred;
            require(XCAFDoc_ShapeTool::GetReferredShape(component, referred),
                    "AP242 reload lost a root occurrence reference");
            if (XCAFDoc_ShapeTool::IsAssembly(referred))
            {
                restored_subassembly_occurrence = component;
                restored_subassembly = referred;
            }
            else
            {
                restored_occurrence_a = component;
                restored_definition = referred;
            }
        }
        require(!restored_occurrence_a.IsNull() && !restored_subassembly_occurrence.IsNull() &&
                    !restored_definition.IsNull() && !restored_subassembly.IsNull(),
                "AP242 reload did not preserve the nested hierarchy roles");
        const NCollection_Sequence<TDF_Label> nested_components =
            direct_components(restored_subassembly, 1);
        const TDF_Label restored_occurrence_b = nested_components.Value(1);
        TDF_Label restored_definition_b;
        require(XCAFDoc_ShapeTool::GetReferredShape(restored_occurrence_b, restored_definition_b) &&
                    restored_definition_b == restored_definition,
                "AP242 reload lost repeated definition identity");
        const gp_XYZ direct_translation = XCAFDoc_ShapeTool::GetLocation(restored_occurrence_a)
                                              .Transformation()
                                              .TranslationPart();
        const gp_XYZ subassembly_translation =
            XCAFDoc_ShapeTool::GetLocation(restored_subassembly_occurrence)
                .Transformation()
                .TranslationPart();
        const gp_XYZ nested_translation = XCAFDoc_ShapeTool::GetLocation(restored_occurrence_b)
                                              .Transformation()
                                              .TranslationPart();
        require(std::abs(direct_translation.X()) <= 1.0e-9 &&
                    std::abs(direct_translation.Y()) <= 1.0e-9 &&
                    std::abs(direct_translation.Z()) <= 1.0e-9 &&
                    std::abs(subassembly_translation.X() - 10.0) <= 1.0e-9 &&
                    std::abs(subassembly_translation.Y()) <= 1.0e-9 &&
                    std::abs(subassembly_translation.Z()) <= 1.0e-9 &&
                    std::abs(nested_translation.X() - 30.0) <= 1.0e-9 &&
                    std::abs(nested_translation.Y()) <= 1.0e-9 &&
                    std::abs(nested_translation.Z()) <= 1.0e-9,
                "AP242 reload changed a local occurrence transform");
        const gp_XYZ nested_global_translation =
            (XCAFDoc_ShapeTool::GetLocation(restored_subassembly_occurrence) *
             XCAFDoc_ShapeTool::GetLocation(restored_occurrence_b))
                .Transformation()
                .TranslationPart();
        require(std::abs(nested_global_translation.X() - 40.0) <= 1.0e-9 &&
                    std::abs(nested_global_translation.Y()) <= 1.0e-9 &&
                    std::abs(nested_global_translation.Z()) <= 1.0e-9,
                "AP242 reload changed the accumulated nested occurrence transform");

        occ::handle<TDataStd_NamedData> restored_metadata;
        require(restored_definition.FindAttribute(TDataStd_NamedData::GetID(), restored_metadata),
                "normal STEPCAF metadata import did not restore NamedData");
        const TCollection_ExtendedString metadata_key(kMetadataKey, true);
        const TCollection_ExtendedString metadata_value(kMetadataValue, true);
        require(restored_metadata->HasString(metadata_key) &&
                    restored_metadata->GetString(metadata_key).IsEqual(metadata_value),
                "normal STEPCAF metadata import changed the product metadata payload");

        const TopoDS_Shape restored_definition_shape =
            XCAFDoc_ShapeTool::GetShape(restored_definition);
        require_reloaded_target_resolution(reader_model, reader_session, restored_definition_shape);
        const GeometryEvidence restored_geometry = geometry_evidence(restored_definition_shape);
        require_geometry_equal(original_geometry, restored_geometry, "AP242 round trip");
        require(geometry_fingerprint(restored_geometry) == original_fingerprint,
                "AP242 round trip changed the quantized per-body fingerprint");
        require_geometry_equal(original_root_geometry,
                               geometry_evidence(XCAFDoc_ShapeTool::GetShape(restored_root)),
                               "AP242 hierarchy round trip");

        application->Close(source_document);
        application->Close(restored_document);
        std::cout << "AP242 product/body/two-face-group/nested-hierarchy persistence passed\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << '\n';
        return 1;
    }
}

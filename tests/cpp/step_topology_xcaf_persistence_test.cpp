#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRep_Builder.hxx>
#include <BinXCAFDrivers.hxx>
#include <FSD_BinaryFile.hxx>
#include <GProp_GProps.hxx>
#include <NCollection_Sequence.hxx>
#include <PCDM_ReaderStatus.hxx>
#include <PCDM_StoreStatus.hxx>
#include <Storage_HeaderData.hxx>
#include <Storage_OpenMode.hxx>
#include <TCollection_AsciiString.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TDF_Tool.hxx>
#include <TDataStd_Name.hxx>
#include <TDataStd_NamedData.hxx>
#include <TDocStd_Document.hxx>
#include <TDocStd_FormatVersion.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Shape.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XmlXCAFDrivers.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{

struct LabelProbe
{
    std::string entry;
    std::string role;
};

struct AuthoredProbe
{
    std::string entry;
    std::string key;
    std::string value;
};

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

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

bool contains_format(const NCollection_Sequence<TCollection_AsciiString>& formats,
                     const std::string& expected)
{
    for (const TCollection_AsciiString& format : formats)
    {
        if (format.IsEqual(expected.c_str()))
            return true;
    }
    return false;
}

std::string entry(const TDF_Label& label)
{
    TCollection_AsciiString value;
    TDF_Tool::Entry(label, value);
    return value.ToCString();
}

void attach_probe(const TDF_Label& label, const std::string& role, std::vector<LabelProbe>* probes)
{
    const TCollection_ExtendedString key("wn.geometer.research.probe.key.persistence", true);
    const TCollection_ExtendedString value(role.c_str(), true);
    const occ::handle<TDataStd_NamedData> named = TDataStd_NamedData::Set(label);
    named->SetString(key, value);
    TDataStd_Name::Set(label, value);
    probes->push_back({entry(label), role});
}

void verify_probe(const occ::handle<TDocStd_Document>& document, const LabelProbe& probe)
{
    TDF_Label label;
    TDF_Tool::Label(document->GetData(), probe.entry.c_str(), label, false);
    require(!label.IsNull(), "persisted XCAF label entry is missing: " + probe.entry);

    occ::handle<TDataStd_NamedData> named;
    require(label.FindAttribute(TDataStd_NamedData::GetID(), named),
            "persisted label lost TDataStd_NamedData: " + probe.role);
    const TCollection_ExtendedString key("wn.geometer.research.probe.key.persistence", true);
    const TCollection_ExtendedString expected(probe.role.c_str(), true);
    require(named->HasString(key) && named->GetString(key).IsEqual(expected),
            "persisted NamedData value changed: " + probe.role);

    occ::handle<TDataStd_Name> name;
    require(label.FindAttribute(TDataStd_Name::GetID(), name) && name->Get().IsEqual(expected),
            "persisted standard name changed: " + probe.role);
}

void attach_authored_probe(const TDF_Label& label, const std::string& key, const std::string& value,
                           std::vector<AuthoredProbe>* probes)
{
    const occ::handle<TDataStd_NamedData> named = TDataStd_NamedData::Set(label);
    named->SetString(TCollection_ExtendedString(key.c_str(), true),
                     TCollection_ExtendedString(value.c_str(), true));
    probes->push_back({entry(label), key, value});
}

void verify_authored_probe(const occ::handle<TDocStd_Document>& document,
                           const AuthoredProbe& probe)
{
    TDF_Label label;
    TDF_Tool::Label(document->GetData(), probe.entry.c_str(), label, false);
    require(!label.IsNull(), "authored XCAF label entry is missing: " + probe.entry);
    occ::handle<TDataStd_NamedData> named;
    require(label.FindAttribute(TDataStd_NamedData::GetID(), named),
            "authored XCAF label lost NamedData: " + probe.key);
    const TCollection_ExtendedString key(probe.key.c_str(), true);
    require(named->HasString(key) && named->GetString(key).IsEqual(
                                         TCollection_ExtendedString(probe.value.c_str(), true)),
            "authored XCAF value changed: " + probe.key);
}

TopLoc_Location translation(double x, double y, double z)
{
    gp_Trsf transform;
    transform.SetTranslation(gp_Vec(x, y, z));
    return TopLoc_Location(transform);
}

bool contains_label(const NCollection_Sequence<TDF_Label>& labels, const TDF_Label& expected)
{
    for (const TDF_Label& label : labels)
    {
        if (label == expected)
            return true;
    }
    return false;
}

void require_translation(const TDF_Label& occurrence, double x, double y, double z,
                         const std::string& message)
{
    const gp_XYZ actual =
        XCAFDoc_ShapeTool::GetLocation(occurrence).Transformation().TranslationPart();
    constexpr double tolerance = 1.0e-12;
    require(std::abs(actual.X() - x) <= tolerance && std::abs(actual.Y() - y) <= tolerance &&
                std::abs(actual.Z() - z) <= tolerance,
            message);
}

std::size_t face_count(const TopoDS_Shape& shape)
{
    std::size_t count = 0;
    for (TopExp_Explorer explorer(shape, TopAbs_FACE); explorer.More(); explorer.Next())
        ++count;
    return count;
}

double volume(const TopoDS_Shape& shape)
{
    GProp_GProps properties;
    BRepGProp::VolumeProperties(shape, properties);
    return properties.Mass();
}

int serialized_storage_version(const std::filesystem::path& artifact, const std::string& format)
{
    if (format == "BinXCAF")
    {
        const occ::handle<FSD_BinaryFile> driver = new FSD_BinaryFile();
        require(driver->Open(TCollection_AsciiString(artifact.u8string().c_str()),
                             Storage_VSRead) == Storage_VSOk,
                "failed opening binary XCAF header");
        const occ::handle<Storage_HeaderData> header = new Storage_HeaderData();
        const bool read = header->Read(driver);
        driver->Close();
        require(read && header->StorageVersion().IsIntegerValue(),
                "binary XCAF header has no integer storage version");
        return header->StorageVersion().IntegerValue();
    }

    std::ifstream input(artifact, std::ios::binary);
    require(input.good(), "failed opening XML XCAF artifact");
    const std::string text((std::istreambuf_iterator<char>(input)),
                           std::istreambuf_iterator<char>());
    constexpr const char* marker = "DocVersion=\"";
    const std::size_t start = text.find(marker);
    require(start != std::string::npos, "XML XCAF header has no DocVersion attribute");
    const std::size_t value_start = start + std::char_traits<char>::length(marker);
    const std::size_t value_end = text.find('"', value_start);
    require(value_end != std::string::npos && value_end > value_start,
            "XML XCAF DocVersion attribute is malformed");
    return std::stoi(text.substr(value_start, value_end - value_start));
}

void exercise_format(const occ::handle<XCAFApp_Application>& application,
                     const TemporaryDirectory& temporary, const std::string& format,
                     const std::string& extension)
{
    occ::handle<TDocStd_Document> document;
    application->NewDocument(TCollection_ExtendedString(format.c_str(), true), document);
    require(!document.IsNull(), format + " document creation failed");

    const occ::handle<XCAFDoc_ShapeTool> shape_tool =
        XCAFDoc_DocumentTool::ShapeTool(document->Main());
    const TopoDS_Shape box = BRepPrimAPI_MakeBox(10.0, 20.0, 30.0).Shape();
    const TDF_Label definition = shape_tool->AddShape(box, false);

    TopoDS_Compound root_shape;
    TopoDS_Compound subassembly_shape;
    BRep_Builder builder;
    builder.MakeCompound(root_shape);
    builder.MakeCompound(subassembly_shape);
    const TDF_Label root_assembly = shape_tool->AddShape(root_shape, true);
    const TDF_Label subassembly = shape_tool->AddShape(subassembly_shape, true);
    const TDF_Label occurrence_a =
        shape_tool->AddComponent(root_assembly, definition, TopLoc_Location());
    const TDF_Label subassembly_occurrence =
        shape_tool->AddComponent(root_assembly, subassembly, translation(10.0, 0.0, 0.0));
    const TDF_Label occurrence_b =
        shape_tool->AddComponent(subassembly, definition, translation(30.0, 0.0, 0.0));
    shape_tool->UpdateAssemblies();

    TopExp_Explorer face_explorer(box, TopAbs_FACE);
    require(face_explorer.More(), "generated XCAF definition has no face");
    const TDF_Label face_a = shape_tool->AddSubShape(definition, face_explorer.Current());
    face_explorer.Next();
    require(face_explorer.More(), "generated XCAF definition has fewer than two faces");
    const TDF_Label face_b = shape_tool->AddSubShape(definition, face_explorer.Current());
    require(!face_a.IsNull() && !face_b.IsNull() && face_a != face_b,
            "XCAF face label creation failed");

    std::vector<LabelProbe> probes;
    attach_probe(document->Main(), "document", &probes);
    attach_probe(definition, "definition", &probes);
    attach_probe(root_assembly, "root-assembly", &probes);
    attach_probe(subassembly, "subassembly", &probes);
    attach_probe(occurrence_a, "occurrence-a", &probes);
    attach_probe(subassembly_occurrence, "subassembly-occurrence", &probes);
    attach_probe(occurrence_b, "occurrence-b", &probes);
    attach_probe(face_a, "group-member-face-a", &probes);
    attach_probe(face_b, "group-member-face-b", &probes);

    std::vector<AuthoredProbe> authored_probes;
    attach_authored_probe(
        document->Main(), "wn.geometer.research.logical-group.state",
        R"({"authoredId":"wn.geometer.research.group.two-faces","revision":1,"memberCount":2})",
        &authored_probes);
    attach_authored_probe(
        document->Main(), "wn.geometer.research.hierarchy.state",
        R"({"authoredId":"wn.geometer.research.hierarchy.repeated-box","revision":1,"rootCount":1,"occurrenceCount":3})",
        &authored_probes);
    attach_authored_probe(definition, "wn.geometer.research.hierarchy.authored-id",
                          "wn.geometer.research.product.box", &authored_probes);
    attach_authored_probe(root_assembly, "wn.geometer.research.hierarchy.authored-id",
                          "wn.geometer.research.assembly.root", &authored_probes);
    attach_authored_probe(subassembly, "wn.geometer.research.hierarchy.authored-id",
                          "wn.geometer.research.assembly.nested", &authored_probes);
    attach_authored_probe(occurrence_a, "wn.geometer.research.hierarchy.authored-id",
                          "wn.geometer.research.occurrence.box-a", &authored_probes);
    attach_authored_probe(subassembly_occurrence, "wn.geometer.research.hierarchy.authored-id",
                          "wn.geometer.research.occurrence.nested", &authored_probes);
    attach_authored_probe(occurrence_b, "wn.geometer.research.hierarchy.authored-id",
                          "wn.geometer.research.occurrence.box-b", &authored_probes);
    attach_authored_probe(face_a, "wn.geometer.research.logical-group.member-of",
                          "wn.geometer.research.group.two-faces", &authored_probes);
    attach_authored_probe(face_b, "wn.geometer.research.logical-group.member-of",
                          "wn.geometer.research.group.two-faces", &authored_probes);

    const std::string definition_entry = entry(definition);
    const std::string root_assembly_entry = entry(root_assembly);
    const std::string subassembly_entry = entry(subassembly);
    const std::string occurrence_a_entry = entry(occurrence_a);
    const std::string subassembly_occurrence_entry = entry(subassembly_occurrence);
    const std::string occurrence_b_entry = entry(occurrence_b);
    const std::string face_a_entry = entry(face_a);
    const std::string face_b_entry = entry(face_b);
    const std::size_t expected_faces = face_count(box);
    const double expected_volume = volume(box);
    const TopoDS_Shape expected_root_shape = XCAFDoc_ShapeTool::GetShape(root_assembly);
    require(!expected_root_shape.IsNull() && BRepCheck_Analyzer(expected_root_shape).IsValid(),
            format + " created invalid root assembly geometry");
    const std::size_t expected_root_faces = face_count(expected_root_shape);
    const double expected_root_volume = volume(expected_root_shape);
    require(expected_root_faces == expected_faces * 2,
            format + " did not materialize both repeated definition occurrences");
    require(std::abs(expected_root_volume - expected_volume * 2.0) <= 1.0e-9,
            format + " changed volume while materializing repeated occurrences");
    const std::filesystem::path artifact = temporary.path / ("authored-state." + extension);
    const TCollection_ExtendedString artifact_path(artifact.u8string().c_str(), true);
    require(application->SaveAs(document, artifact_path) == PCDM_SS_OK, format + " storage failed");
    require(std::filesystem::is_regular_file(artifact) && std::filesystem::file_size(artifact) > 0,
            format + " did not produce a nonempty artifact");
    require(serialized_storage_version(artifact, format) == TDocStd_FormatVersion_CURRENT,
            format + " serialized an unexpected OCAF storage version");
    application->Close(document);

    occ::handle<TDocStd_Document> restored;
    require(application->Open(artifact_path, restored) == PCDM_RS_OK && !restored.IsNull(),
            format + " retrieval failed");
    require(restored->StorageFormat().IsEqual(TCollection_ExtendedString(format.c_str(), true)),
            format + " storage format was not reported after reload");
    for (const LabelProbe& probe : probes)
        verify_probe(restored, probe);
    for (const AuthoredProbe& probe : authored_probes)
        verify_authored_probe(restored, probe);

    const occ::handle<XCAFDoc_ShapeTool> restored_shape_tool =
        XCAFDoc_DocumentTool::ShapeTool(restored->Main());
    TDF_Label restored_definition;
    TDF_Tool::Label(restored->GetData(), definition_entry.c_str(), restored_definition, false);
    TDF_Label restored_root_assembly;
    TDF_Tool::Label(restored->GetData(), root_assembly_entry.c_str(), restored_root_assembly,
                    false);
    TDF_Label restored_subassembly;
    TDF_Tool::Label(restored->GetData(), subassembly_entry.c_str(), restored_subassembly, false);
    TDF_Label restored_occurrence_a;
    TDF_Tool::Label(restored->GetData(), occurrence_a_entry.c_str(), restored_occurrence_a, false);
    TDF_Label restored_subassembly_occurrence;
    TDF_Tool::Label(restored->GetData(), subassembly_occurrence_entry.c_str(),
                    restored_subassembly_occurrence, false);
    TDF_Label restored_occurrence_b;
    TDF_Tool::Label(restored->GetData(), occurrence_b_entry.c_str(), restored_occurrence_b, false);
    TDF_Label restored_face_a;
    TDF_Tool::Label(restored->GetData(), face_a_entry.c_str(), restored_face_a, false);
    TDF_Label restored_face_b;
    TDF_Tool::Label(restored->GetData(), face_b_entry.c_str(), restored_face_b, false);
    require(XCAFDoc_ShapeTool::IsAssembly(restored_root_assembly) &&
                XCAFDoc_ShapeTool::IsAssembly(restored_subassembly),
            format + " lost a nested assembly label role");
    require(XCAFDoc_ShapeTool::IsReference(restored_occurrence_a) &&
                XCAFDoc_ShapeTool::IsComponent(restored_occurrence_a) &&
                XCAFDoc_ShapeTool::IsReference(restored_subassembly_occurrence) &&
                XCAFDoc_ShapeTool::IsComponent(restored_subassembly_occurrence) &&
                XCAFDoc_ShapeTool::IsReference(restored_occurrence_b) &&
                XCAFDoc_ShapeTool::IsComponent(restored_occurrence_b),
            format + " lost an occurrence reference/component role");
    TDF_Label referred_definition_a;
    TDF_Label referred_subassembly;
    TDF_Label referred_definition_b;
    require(XCAFDoc_ShapeTool::GetReferredShape(restored_occurrence_a, referred_definition_a) &&
                referred_definition_a == restored_definition &&
                XCAFDoc_ShapeTool::GetReferredShape(restored_subassembly_occurrence,
                                                    referred_subassembly) &&
                referred_subassembly == restored_subassembly &&
                XCAFDoc_ShapeTool::GetReferredShape(restored_occurrence_b, referred_definition_b) &&
                referred_definition_b == restored_definition,
            format + " lost a repeated occurrence-to-definition relationship");
    NCollection_Sequence<TDF_Label> root_components;
    NCollection_Sequence<TDF_Label> nested_components;
    require(XCAFDoc_ShapeTool::GetComponents(restored_root_assembly, root_components, false) &&
                root_components.Length() == 2 &&
                contains_label(root_components, restored_occurrence_a) &&
                contains_label(root_components, restored_subassembly_occurrence),
            format + " lost the root assembly component set");
    require(XCAFDoc_ShapeTool::GetComponents(restored_subassembly, nested_components, false) &&
                nested_components.Length() == 1 &&
                contains_label(nested_components, restored_occurrence_b),
            format + " lost the nested assembly component set");
    require_translation(restored_occurrence_a, 0.0, 0.0, 0.0,
                        format + " changed occurrence A placement");
    require_translation(restored_subassembly_occurrence, 10.0, 0.0, 0.0,
                        format + " changed nested assembly placement");
    require_translation(restored_occurrence_b, 30.0, 0.0, 0.0,
                        format + " changed occurrence B local placement");
    const gp_XYZ restored_nested_global =
        (XCAFDoc_ShapeTool::GetLocation(restored_subassembly_occurrence) *
         XCAFDoc_ShapeTool::GetLocation(restored_occurrence_b))
            .Transformation()
            .TranslationPart();
    require(std::abs(restored_nested_global.X() - 40.0) <= 1.0e-12 &&
                std::abs(restored_nested_global.Y()) <= 1.0e-12 &&
                std::abs(restored_nested_global.Z()) <= 1.0e-12,
            format + " changed accumulated nested occurrence placement");
    for (const TDF_Label& restored_face : {restored_face_a, restored_face_b})
    {
        require(XCAFDoc_ShapeTool::IsSubShape(restored_face),
                format + " lost an explicit face subshape role");
        const TopoDS_Shape restored_face_shape = XCAFDoc_ShapeTool::GetShape(restored_face);
        require(!restored_face_shape.IsNull() && restored_face_shape.ShapeType() == TopAbs_FACE &&
                    restored_shape_tool->IsSubShape(restored_definition, restored_face_shape),
                format + " lost a definition-to-face subshape relationship");
    }
    const TopoDS_Shape restored_box = XCAFDoc_ShapeTool::GetShape(restored_definition);
    require(!restored_box.IsNull() && BRepCheck_Analyzer(restored_box).IsValid(),
            format + " restored invalid definition geometry");
    require(face_count(restored_box) == expected_faces,
            format + " changed definition face cardinality");
    require(std::abs(volume(restored_box) - expected_volume) <= 1.0e-9,
            format + " changed definition volume");
    const TopoDS_Shape restored_root_shape = XCAFDoc_ShapeTool::GetShape(restored_root_assembly);
    require(!restored_root_shape.IsNull() && BRepCheck_Analyzer(restored_root_shape).IsValid(),
            format + " restored invalid root assembly geometry");
    require(face_count(restored_root_shape) == expected_root_faces,
            format + " changed root assembly face cardinality");
    require(std::abs(volume(restored_root_shape) - expected_root_volume) <= 1.0e-9,
            format + " changed root assembly volume");
    application->Close(restored);
}

} // namespace

int main()
{
    try
    {
        static_assert(TDocStd_FormatVersion_CURRENT == TDocStd_FormatVersion_VERSION_12);
        const occ::handle<XCAFApp_Application> application = XCAFApp_Application::GetApplication();
        BinXCAFDrivers::DefineFormat(application);
        XmlXCAFDrivers::DefineFormat(application);
        NCollection_Sequence<TCollection_AsciiString> reading_formats;
        NCollection_Sequence<TCollection_AsciiString> writing_formats;
        application->ReadingFormats(reading_formats);
        application->WritingFormats(writing_formats);
        for (const std::string format : {"BinXCAF", "XmlXCAF"})
        {
            require(contains_format(reading_formats, format),
                    format + " retrieval driver was not registered");
            require(contains_format(writing_formats, format),
                    format + " storage driver was not registered");
        }

        TemporaryDirectory temporary("geometer-xcaf-persistence-");

        exercise_format(application, temporary, "BinXCAF", "xbf");
        exercise_format(application, temporary, "XmlXCAF", "xml");
        std::cout << "XCAF binary/XML authored-state persistence passed\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << '\n';
        return 1;
    }
}

#include "geometer/sha256.h"
#include "geometer/step_topology_session.h"

#include <APIHeaderSection_MakeHeader.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBndLib.hxx>
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRep_Builder.hxx>
#include <Bnd_Box.hxx>
#include <DESTEP_Parameters.hxx>
#include <GProp_GProps.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <NCollection_IndexedMap.hxx>
#include <NCollection_Sequence.hxx>
#include <Quantity_Color.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <STEPCAFControl_Writer.hxx>
#include <Standard_Version.hxx>
#include <TCollection_AsciiString.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TCollection_HAsciiString.hxx>
#include <TDF_ChildIterator.hxx>
#include <TDF_Label.hxx>
#include <TDF_Tool.hxx>
#include <TDataStd_Name.hxx>
#include <TDataStd_NamedData.hxx>
#include <TDocStd_Document.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp.hxx>
#include <TopLoc_Location.hxx>
#include <TopTools_ShapeMapHasher.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Shape.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_LayerTool.hxx>
#include <XCAFDoc_MaterialTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>

#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <random>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{

namespace fs = std::filesystem;

struct DefinitionRecord
{
    std::string label_entry;
    std::string name;
    bool is_assembly = false;
    std::size_t solid_count = 0;
    std::size_t shell_count = 0;
    std::size_t face_count = 0;
    std::array<double, 6> bounds{};
    double volume = 0.0;
};

struct OccurrenceRecord
{
    std::string path;
    std::string name;
    std::size_t definition_index = 0;
    std::size_t depth = 0;
    bool has_non_identity_location = false;
    std::array<double, 12> transform{};
};

struct Observation
{
    std::string kind;
    std::string step_path;
    std::string glb_path;
    std::string source_sha256;
    std::vector<std::string> file_schemas;
    std::uintmax_t step_bytes = 0;
    std::uintmax_t glb_bytes = 0;
    std::size_t free_shape_count = 0;
    std::size_t definition_count = 0;
    std::size_t assembly_definition_count = 0;
    std::size_t simple_shape_definition_count = 0;
    std::size_t component_label_count = 0;
    std::size_t component_occurrence_count = 0;
    std::size_t located_occurrence_count = 0;
    std::size_t maximum_occurrence_depth = 0;
    std::size_t solid_count = 0;
    std::size_t shell_count = 0;
    std::size_t face_count = 0;
    std::size_t labeled_subshape_count = 0;
    std::size_t named_label_count = 0;
    std::size_t color_assignment_count = 0;
    std::size_t layer_assignment_count = 0;
    std::size_t material_definition_count = 0;
    std::size_t named_data_label_count = 0;
    std::size_t compact_snapshot_bytes = 0;
    std::size_t verbose_snapshot_bytes = 0;
    std::size_t render_meshes = 0;
    std::size_t render_instances = 0;
    std::size_t render_vertices = 0;
    std::size_t render_indices = 0;
    std::size_t render_primitives = 0;
    std::size_t render_bindings = 0;
    std::size_t render_geometry_triangles = 0;
    std::size_t render_instanced_triangles = 0;
    std::size_t render_binding_table_logical_bytes = 0;
    std::size_t binding_glb_bytes = 0;
    std::size_t binding_glb_json_bytes = 0;
    std::size_t binding_glb_binary_bytes = 0;
    std::size_t binding_glb_per_face_draw_calls = 0;
    std::size_t projected_merged_draw_calls = 0;
    std::string occurrence_path_transform_sha256;
    std::string semantic_evidence_sha256;
    std::vector<DefinitionRecord> definitions;
    std::vector<OccurrenceRecord> occurrences;
};

std::vector<std::uint8_t> read_bytes(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
    {
        throw std::runtime_error("failed to open " + path.generic_string());
    }
    const std::vector<char> chars{std::istreambuf_iterator<char>(input),
                                  std::istreambuf_iterator<char>()};
    return {chars.begin(), chars.end()};
}

std::vector<std::uint8_t> read_canonical_step_bytes(const fs::path& path)
{
    const std::vector<std::uint8_t> raw = read_bytes(path);
    std::vector<std::uint8_t> canonical;
    canonical.reserve(raw.size());
    for (std::size_t index = 0; index < raw.size(); ++index)
    {
        if (raw[index] == '\r' && index + 1 < raw.size() && raw[index + 1] == '\n')
        {
            continue;
        }
        canonical.push_back(raw[index]);
    }
    return canonical;
}

std::vector<std::string> extract_file_schemas(const std::vector<std::uint8_t>& bytes)
{
    const std::string source(bytes.begin(), bytes.end());
    const std::regex schema_block(R"(FILE_SCHEMA\s*\(\s*\(([^;]*)\)\s*\)\s*;)", std::regex::icase);
    std::smatch block_match;
    if (!std::regex_search(source, block_match, schema_block))
    {
        return {};
    }
    const std::string block = block_match[1].str();
    const std::regex quoted(R"('([^']+)')");
    std::vector<std::string> schemas;
    for (std::sregex_iterator iterator(block.begin(), block.end(), quoted), end; iterator != end;
         ++iterator)
    {
        schemas.push_back((*iterator)[1].str());
    }
    return schemas;
}

std::string label_entry(const TDF_Label& label)
{
    TCollection_AsciiString entry;
    TDF_Tool::Entry(label, entry);
    return entry.ToCString();
}

std::string label_name(const TDF_Label& label)
{
    Handle(TDataStd_Name) name;
    if (!label.FindAttribute(TDataStd_Name::GetID(), name))
    {
        return {};
    }
    return TCollection_AsciiString(name->Get()).ToCString();
}

std::array<double, 6> shape_bounds(const TopoDS_Shape& shape)
{
    std::array<double, 6> result{};
    Bnd_Box box;
    BRepBndLib::Add(shape, box);
    if (!box.IsVoid())
    {
        box.Get(result[0], result[1], result[2], result[3], result[4], result[5]);
    }
    return result;
}

double shape_volume(const TopoDS_Shape& shape)
{
    GProp_GProps properties;
    BRepGProp::VolumeProperties(shape, properties);
    return properties.Mass();
}

std::array<double, 12> transform_values(const TopLoc_Location& location)
{
    std::array<double, 12> values{};
    const gp_Trsf& transform = location.Transformation();
    std::size_t index = 0;
    for (int row = 1; row <= 3; ++row)
    {
        for (int column = 1; column <= 4; ++column)
        {
            values[index++] = transform.Value(row, column);
        }
    }
    return values;
}

bool is_identity_transform(const std::array<double, 12>& transform, double tolerance = 1.0e-9)
{
    constexpr std::array<double, 12> identity = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0,
                                                 0.0, 0.0, 0.0, 0.0, 1.0, 0.0};
    for (std::size_t index = 0; index < transform.size(); ++index)
    {
        if (std::abs(transform[index] - identity[index]) > tolerance)
        {
            return false;
        }
    }
    return true;
}

std::size_t count_shapes(const TopoDS_Shape& shape, TopAbs_ShapeEnum type)
{
    if (shape.IsNull())
    {
        return 0;
    }
    NCollection_IndexedMap<TopoDS_Shape, TopTools_ShapeMapHasher> shapes;
    TopExp::MapShapes(shape, type, shapes);
    return static_cast<std::size_t>(shapes.Extent());
}

class DocumentCounter
{
  public:
    DocumentCounter(const Handle(TDocStd_Document) & document,
                    const Handle(XCAFDoc_ShapeTool) & shape_tool)
        : document_(document), shape_tool_(shape_tool)
    {
    }

    void inspect(Observation& observation)
    {
        NCollection_Sequence<TDF_Label> free_shapes;
        shape_tool_->GetFreeShapes(free_shapes);
        observation.free_shape_count = static_cast<std::size_t>(free_shapes.Length());
        std::set<std::string> definition_path;
        for (int index = 1; index <= free_shapes.Length(); ++index)
        {
            visit_definition(free_shapes.Value(index), 0, definition_path, observation);
        }
        observation.definition_count = observation.definitions.size();
        std::set<std::string> occurrence_definition_path;
        for (int index = 1; index <= free_shapes.Length(); ++index)
        {
            const TDF_Label root = free_shapes.Value(index);
            expand_occurrences(root, XCAFDoc_ShapeTool::GetLocation(root), label_entry(root), 0,
                               occurrence_definition_path, observation);
        }
        observation.component_occurrence_count = observation.occurrences.size();
        count_document_metadata(observation);
        measure_evidence(observation);
        measure_payloads(observation);
    }

  private:
    std::size_t visit_definition(const TDF_Label& definition, std::size_t depth,
                                 std::set<std::string>& recursion_path, Observation& observation)
    {
        const std::string entry = label_entry(definition);
        if (depth > 64 || recursion_path.count(entry) != 0)
        {
            throw std::runtime_error("cyclic or excessively deep XCAF definition graph");
        }
        const auto known = definition_indices_.find(entry);
        if (known != definition_indices_.end())
        {
            return known->second;
        }
        recursion_path.insert(entry);

        DefinitionRecord record;
        record.label_entry = entry;
        record.name = label_name(definition);
        record.is_assembly = XCAFDoc_ShapeTool::IsAssembly(definition);
        if (!record.is_assembly)
        {
            const TopoDS_Shape shape = XCAFDoc_ShapeTool::GetShape(definition);
            record.solid_count = count_shapes(shape, TopAbs_SOLID);
            record.shell_count = count_shapes(shape, TopAbs_SHELL);
            record.face_count = count_shapes(shape, TopAbs_FACE);
            record.bounds = shape_bounds(shape);
            record.volume = shape_volume(shape);
        }
        const std::size_t definition_index = observation.definitions.size();
        definition_indices_.emplace(entry, definition_index);
        observation.definitions.push_back(record);
        observation.solid_count += record.solid_count;
        observation.shell_count += record.shell_count;
        observation.face_count += record.face_count;
        if (record.is_assembly)
        {
            ++observation.assembly_definition_count;
        }
        else if (XCAFDoc_ShapeTool::IsSimpleShape(definition) || record.solid_count > 0 ||
                 record.shell_count > 0 || record.face_count > 0)
        {
            ++observation.simple_shape_definition_count;
        }

        NCollection_Sequence<TDF_Label> subshapes;
        if (XCAFDoc_ShapeTool::GetSubShapes(definition, subshapes))
        {
            observation.labeled_subshape_count += static_cast<std::size_t>(subshapes.Length());
        }

        NCollection_Sequence<TDF_Label> components;
        if (XCAFDoc_ShapeTool::GetComponents(definition, components, false))
        {
            for (int index = 1; index <= components.Length(); ++index)
            {
                const TDF_Label component = components.Value(index);
                TDF_Label referred;
                if (!XCAFDoc_ShapeTool::GetReferredShape(component, referred))
                {
                    throw std::runtime_error("XCAF component has no referred definition");
                }
                ++observation.component_label_count;
                visit_definition(referred, depth + 1, recursion_path, observation);
            }
        }
        recursion_path.erase(entry);
        return definition_index;
    }

    void expand_occurrences(const TDF_Label& definition, const TopLoc_Location& parent_location,
                            const std::string& parent_path, std::size_t depth,
                            std::set<std::string>& recursion_path, Observation& observation) const
    {
        const std::string definition_entry = label_entry(definition);
        if (depth > 64 || !recursion_path.insert(definition_entry).second)
        {
            throw std::runtime_error("cyclic or excessively deep XCAF occurrence graph");
        }
        NCollection_Sequence<TDF_Label> components;
        if (XCAFDoc_ShapeTool::GetComponents(definition, components, false))
        {
            for (int index = 1; index <= components.Length(); ++index)
            {
                if (observation.occurrences.size() >= 100000)
                {
                    throw std::runtime_error("expanded XCAF occurrence limit exceeded");
                }
                const TDF_Label component = components.Value(index);
                TDF_Label referred;
                if (!XCAFDoc_ShapeTool::GetReferredShape(component, referred))
                {
                    throw std::runtime_error("XCAF component has no referred definition");
                }
                const auto definition_index = definition_indices_.find(label_entry(referred));
                if (definition_index == definition_indices_.end())
                {
                    throw std::runtime_error("expanded occurrence references unknown definition");
                }
                const TopLoc_Location accumulated_location =
                    parent_location.Multiplied(XCAFDoc_ShapeTool::GetLocation(component));
                OccurrenceRecord occurrence;
                occurrence.path = parent_path + "/" + label_entry(component);
                occurrence.name = label_name(component);
                occurrence.definition_index = definition_index->second;
                occurrence.depth = depth + 1;
                occurrence.transform = transform_values(accumulated_location);
                occurrence.has_non_identity_location = !is_identity_transform(occurrence.transform);
                observation.occurrences.push_back(occurrence);
                if (occurrence.has_non_identity_location)
                {
                    ++observation.located_occurrence_count;
                }
                observation.maximum_occurrence_depth =
                    std::max(observation.maximum_occurrence_depth, occurrence.depth);
                expand_occurrences(referred, accumulated_location, occurrence.path, depth + 1,
                                   recursion_path, observation);
            }
        }
        recursion_path.erase(definition_entry);
    }

    static std::string digest_buffer(const rapidjson::StringBuffer& buffer)
    {
        return geometer::sha256_hex(reinterpret_cast<const std::uint8_t*>(buffer.GetString()),
                                    buffer.GetSize());
    }

    static void write_occurrence_evidence(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                          const Observation& observation)
    {
        writer.StartArray();
        for (const OccurrenceRecord& occurrence : observation.occurrences)
        {
            writer.StartObject();
            writer.Key("path");
            writer.String(occurrence.path.c_str());
            writer.Key("name");
            writer.String(occurrence.name.c_str());
            writer.Key("definition");
            writer.Uint64(occurrence.definition_index);
            writer.Key("transform");
            writer.StartArray();
            for (double value : occurrence.transform)
            {
                writer.Double(value);
            }
            writer.EndArray();
            writer.EndObject();
        }
        writer.EndArray();
    }

    static void measure_evidence(Observation& observation)
    {
        rapidjson::StringBuffer occurrence_buffer;
        rapidjson::Writer<rapidjson::StringBuffer> occurrence_writer(occurrence_buffer);
        write_occurrence_evidence(occurrence_writer, observation);
        observation.occurrence_path_transform_sha256 = digest_buffer(occurrence_buffer);

        rapidjson::StringBuffer semantic_buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(semantic_buffer);
        writer.StartObject();
        writer.Key("definitions");
        writer.StartArray();
        for (const DefinitionRecord& definition : observation.definitions)
        {
            writer.StartObject();
            writer.Key("name");
            writer.String(definition.name.c_str());
            writer.Key("assembly");
            writer.Bool(definition.is_assembly);
            writer.Key("solids");
            writer.Uint64(definition.solid_count);
            writer.Key("shells");
            writer.Uint64(definition.shell_count);
            writer.Key("faces");
            writer.Uint64(definition.face_count);
            writer.Key("bounds");
            writer.StartArray();
            for (double value : definition.bounds)
            {
                writer.Double(value);
            }
            writer.EndArray();
            writer.Key("volume");
            writer.Double(definition.volume);
            writer.EndObject();
        }
        writer.EndArray();
        writer.Key("occurrences");
        write_occurrence_evidence(writer, observation);
        writer.EndObject();
        observation.semantic_evidence_sha256 = digest_buffer(semantic_buffer);
    }

    void count_document_metadata(Observation& observation) const
    {
        const Handle(XCAFDoc_ColorTool) color_tool =
            XCAFDoc_DocumentTool::ColorTool(document_->Main());
        const Handle(XCAFDoc_LayerTool) layer_tool =
            XCAFDoc_DocumentTool::LayerTool(document_->Main());
        const Handle(XCAFDoc_MaterialTool) material_tool =
            XCAFDoc_DocumentTool::MaterialTool(document_->Main());
        NCollection_Sequence<TDF_Label> material_labels;
        material_tool->GetMaterialLabels(material_labels);
        observation.material_definition_count = static_cast<std::size_t>(material_labels.Length());

        for (TDF_ChildIterator iterator(document_->Main(), true); iterator.More(); iterator.Next())
        {
            const TDF_Label label = iterator.Value();
            Handle(TDataStd_Name) name;
            if (label.FindAttribute(TDataStd_Name::GetID(), name))
            {
                ++observation.named_label_count;
            }
            Handle(TDataStd_NamedData) named_data;
            if (label.FindAttribute(TDataStd_NamedData::GetID(), named_data))
            {
                ++observation.named_data_label_count;
            }
            for (const XCAFDoc_ColorType color_type :
                 {XCAFDoc_ColorGen, XCAFDoc_ColorSurf, XCAFDoc_ColorCurv})
            {
                if (color_tool->IsSet(label, color_type))
                {
                    ++observation.color_assignment_count;
                }
            }
            NCollection_Sequence<TDF_Label> layers;
            if (layer_tool->GetLayers(label, layers))
            {
                observation.layer_assignment_count += static_cast<std::size_t>(layers.Length());
            }
        }
    }

    static void write_sized_snapshot(Observation& observation, bool verbose)
    {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        writer.StartObject();
        writer.Key("definitions");
        writer.StartArray();
        for (std::size_t index = 0; index < observation.definitions.size(); ++index)
        {
            const DefinitionRecord& definition = observation.definitions[index];
            writer.StartObject();
            writer.Key("id");
            writer.Uint64(index);
            writer.Key("assembly");
            writer.Bool(definition.is_assembly);
            if (verbose)
            {
                writer.Key("diagnostic_label_entry");
                writer.String(definition.label_entry.c_str());
            }
            writer.EndObject();
        }
        writer.EndArray();
        writer.Key("occurrences");
        writer.StartArray();
        for (std::size_t index = 0; index < observation.occurrences.size(); ++index)
        {
            const OccurrenceRecord& occurrence = observation.occurrences[index];
            writer.StartObject();
            writer.Key("id");
            writer.Uint64(index);
            writer.Key("definition");
            writer.Uint64(occurrence.definition_index);
            writer.Key("depth");
            writer.Uint64(occurrence.depth);
            if (verbose)
            {
                writer.Key("path");
                writer.String(occurrence.path.c_str());
                writer.Key("transform");
                writer.StartArray();
                for (double value : occurrence.transform)
                {
                    writer.Double(value);
                }
                writer.EndArray();
                writer.Key("has_non_identity_location");
                writer.Bool(occurrence.has_non_identity_location);
                writer.Key("source_step_entity");
                writer.Null();
            }
            writer.EndObject();
        }
        writer.EndArray();
        for (const std::pair<const char*, std::size_t>& table :
             {std::pair{"bodies", observation.solid_count},
              std::pair{"shells", observation.shell_count},
              std::pair{"faces", observation.face_count}})
        {
            writer.Key(table.first);
            writer.StartArray();
            for (std::size_t index = 0; index < table.second; ++index)
            {
                if (verbose)
                {
                    writer.StartObject();
                    writer.Key("id");
                    writer.Uint64(index);
                    writer.Key("carrier_locator");
                    writer.Null();
                    writer.Key("diagnostics");
                    writer.StartArray();
                    writer.EndArray();
                    writer.EndObject();
                }
                else
                {
                    writer.Uint64(index);
                }
            }
            writer.EndArray();
        }
        writer.EndObject();
        const std::size_t size = buffer.GetSize();
        if (verbose)
        {
            observation.verbose_snapshot_bytes = size;
        }
        else
        {
            observation.compact_snapshot_bytes = size;
        }
    }

    static void measure_payloads(Observation& observation)
    {
        write_sized_snapshot(observation, false);
        write_sized_snapshot(observation, true);
    }

    Handle(TDocStd_Document) document_;
    Handle(XCAFDoc_ShapeTool) shape_tool_;
    std::unordered_map<std::string, std::size_t> definition_indices_;
};

struct ReaderPosture
{
    bool product = true;
    bool all_product_contexts = true;
    bool all_shape_representations = true;
    bool tessellated = true;
    bool all_assembly_levels = true;
    bool relationships = true;
    bool shape_aspects = true;
    bool constructive_geometry = false;
    bool subshape_names = true;
    bool nonmanifold = false;
    bool all_top_level_shapes = false;
    bool root_transformations = true;
    bool colors = true;
    bool names = true;
    bool layers = true;
    bool validation_properties = true;
    bool metadata = true;
    bool product_metadata = true;
    bool shuo = true;
    bool gdt = true;
    bool materials = true;
    bool views = true;

    DESTEP_Parameters step_parameters() const
    {
        DESTEP_Parameters parameters;
        parameters.ReadProductMode = product;
        parameters.ReadProductContext = all_product_contexts
                                            ? DESTEP_Parameters::ReadMode_ProductContext_All
                                            : DESTEP_Parameters::ReadMode_ProductContext_Design;
        parameters.ReadShapeRepr = all_shape_representations
                                       ? DESTEP_Parameters::ReadMode_ShapeRepr_All
                                       : DESTEP_Parameters::ReadMode_ShapeRepr_ABSR;
        parameters.ReadTessellated = tessellated ? DESTEP_Parameters::RWMode_Tessellated_On
                                                 : DESTEP_Parameters::RWMode_Tessellated_Off;
        parameters.ReadAssemblyLevel = all_assembly_levels
                                           ? DESTEP_Parameters::ReadMode_AssemblyLevel_All
                                           : DESTEP_Parameters::ReadMode_AssemblyLevel_Assembly;
        parameters.ReadRelationship = relationships;
        parameters.ReadShapeAspect = shape_aspects;
        parameters.ReadConstrRelation = constructive_geometry;
        parameters.ReadSubshapeNames = subshape_names;
        parameters.ReadNonmanifold = nonmanifold;
        parameters.ReadAllShapes = all_top_level_shapes;
        parameters.ReadRootTransformation = root_transformations;
        parameters.ReadColor = colors;
        parameters.ReadName = names;
        parameters.ReadLayer = layers;
        parameters.ReadProps = validation_properties;
        parameters.ReadMetadata = metadata;
        parameters.ReadProductMetadata = product_metadata;
        return parameters;
    }

    void configure(STEPCAFControl_Reader& reader) const
    {
        reader.SetColorMode(colors);
        reader.SetNameMode(names);
        reader.SetLayerMode(layers);
        reader.SetPropsMode(validation_properties);
        reader.SetMetaMode(metadata);
        reader.SetProductMetaMode(product_metadata);
        reader.SetSHUOMode(shuo);
        reader.SetGDTMode(gdt);
        reader.SetMatMode(materials);
        reader.SetViewMode(views);

        if (reader.GetColorMode() != colors || reader.GetNameMode() != names ||
            reader.GetLayerMode() != layers || reader.GetPropsMode() != validation_properties ||
            reader.GetMetaMode() != metadata || reader.GetProductMetaMode() != product_metadata ||
            reader.GetSHUOMode() != shuo || reader.GetGDTMode() != gdt ||
            reader.GetMatMode() != materials || reader.GetViewMode() != views)
        {
            throw std::runtime_error("STEPCAF reader mode configuration did not stick");
        }
    }
};

const ReaderPosture& research_reader_posture()
{
    static const ReaderPosture posture;
    return posture;
}

Observation inspect_step(const std::string& kind, const fs::path& step_path,
                         const fs::path& glb_path)
{
    Observation observation;
    observation.kind = kind;
    observation.step_path = step_path.generic_string();
    if (!glb_path.empty())
    {
        observation.glb_path = glb_path.generic_string();
        observation.glb_bytes = fs::file_size(glb_path);
    }
    const std::vector<std::uint8_t> source = read_canonical_step_bytes(step_path);
    observation.step_bytes = source.size();
    observation.source_sha256 = geometer::sha256_hex(source.data(), source.size());
    observation.file_schemas = extract_file_schemas(source);

    const Handle(TDocStd_Document) document =
        new TDocStd_Document(TCollection_ExtendedString("BinXCAF"));
    STEPCAFControl_Reader reader;
    const ReaderPosture& posture = research_reader_posture();
    posture.configure(reader);
    const DESTEP_Parameters parameters = posture.step_parameters();
    if (reader.ReadFile(step_path.string().c_str(), parameters) != IFSelect_RetDone)
    {
        throw std::runtime_error("OCCT failed reading " + step_path.generic_string());
    }
    if (!reader.Transfer(document))
    {
        throw std::runtime_error("OCCT failed transferring " + step_path.generic_string());
    }
    const Handle(XCAFDoc_ShapeTool) shape_tool = XCAFDoc_DocumentTool::ShapeTool(document->Main());
    DocumentCounter(document, shape_tool).inspect(observation);
    std::unique_ptr<geometer::StepTopologySession> session;
    geometer::Status status;
    if (geometer::StepTopologySession::open_step(source.data(), source.size(), {}, &session,
                                                 &status) != 0)
    {
        throw std::runtime_error("topology render inventory open failed for " +
                                 step_path.generic_string() + ": " + status.message);
    }
    geometer::StepTopologyGlbWorkPacket packet;
    if (session->render_glb_work_packet({}, &packet, &status) != 0)
    {
        throw std::runtime_error("topology render/GLB inventory failed for " +
                                 step_path.generic_string() + ": " + status.message);
    }
    const geometer::StepTopologyRenderArtifact& render = packet.render;
    observation.render_meshes = render.meshes.size();
    observation.render_instances = render.instances.size();
    observation.render_bindings = render.bindings.size();
    observation.render_geometry_triangles = render.geometry_triangle_count;
    observation.render_instanced_triangles = render.instanced_triangle_count;
    observation.binding_glb_bytes = packet.glb.size();
    observation.binding_glb_json_bytes = packet.json_bytes;
    observation.binding_glb_binary_bytes = packet.binary_bytes;
    for (const geometer::StepTopologyTriangleBinding& binding : render.bindings)
    {
        observation.render_binding_table_logical_bytes +=
            4U * sizeof(std::uint64_t) + binding.occurrence_handle.size() +
            binding.body_handle.size() + binding.face_handle.size();
    }
    for (const geometer::StepTopologyRenderMesh& mesh : render.meshes)
    {
        observation.render_vertices += mesh.vertices.size();
        observation.render_indices += mesh.indices.size();
        observation.render_primitives += mesh.primitives.size();
    }
    for (std::size_t instance_index = 0; instance_index < render.instances.size(); ++instance_index)
    {
        const geometer::StepTopologyRenderInstance& instance = render.instances[instance_index];
        if (instance.mesh_index >= render.meshes.size())
        {
            throw std::runtime_error("render inventory instance references an invalid mesh");
        }
        const std::size_t triangle_count = render.meshes[instance.mesh_index].indices.size() / 3U;
        for (std::size_t triangle_index = 0; triangle_index < triangle_count; ++triangle_index)
        {
            geometer::StepTopologyRenderHit hit;
            if (session->resolve_render_hit(render, instance_index, triangle_index, &hit,
                                            &status) != 0 ||
                hit.occurrence_handle != instance.occurrence_handle || hit.body_handle.empty() ||
                hit.face_handle.empty())
            {
                throw std::runtime_error("render inventory triangle failed exact reverse lookup");
            }
        }
    }
    observation.binding_glb_per_face_draw_calls = render.bindings.size();
    observation.projected_merged_draw_calls = render.instances.size();
    return observation;
}

void set_name(const TDF_Label& label, const char* value)
{
    TDataStd_Name::Set(label, TCollection_ExtendedString(value));
}

void write_generated_document(const Handle(TDocStd_Document) & document, const fs::path& path)
{
    STEPCAFControl_Writer writer;
    writer.SetColorMode(true);
    writer.SetNameMode(true);
    writer.SetLayerMode(true);
    writer.SetPropsMode(true);
    writer.SetMetadataMode(true);
    writer.SetSHUOMode(true);
    writer.SetDimTolMode(true);
    writer.SetMaterialMode(true);

    DESTEP_Parameters parameters;
    parameters.WriteSchema = DESTEP_Parameters::WriteMode_StepSchema_AP242DIS;
    parameters.WriteColor = true;
    parameters.WriteName = true;
    parameters.WriteLayer = true;
    parameters.WriteProps = true;
    parameters.WriteMetadata = true;
    parameters.WriteSubshapeNames = true;
    if (!writer.Transfer(document, parameters))
    {
        throw std::runtime_error("failed to transfer generated XCAF fixture");
    }

    const Handle(StepData_StepModel) model = writer.ChangeWriter().Model();
    APIHeaderSection_MakeHeader header(model);
    header.SetName(new TCollection_HAsciiString(path.filename().string().c_str()));
    header.SetTimeStamp(new TCollection_HAsciiString("2000-01-01T00:00:00"));
    header.SetAuthorValue(1, new TCollection_HAsciiString("Wavenumber"));
    header.SetOrganizationValue(1, new TCollection_HAsciiString("Wavenumber"));
    header.SetPreprocessorVersion(new TCollection_HAsciiString("Geometer OCCT fixture generator"));
    header.SetOriginatingSystem(new TCollection_HAsciiString("Geometer"));
    header.SetAuthorisation(new TCollection_HAsciiString("test fixture"));
    header.Apply(model);
    if (writer.Write(path.string().c_str()) != IFSelect_RetDone)
    {
        throw std::runtime_error("failed to write generated XCAF fixture");
    }
}

void add_generated_root_placement(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
    {
        throw std::runtime_error("failed to reopen generated root-placement fixture");
    }
    std::string text{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    const auto replace_once = [&text](const std::string& original, const std::string& replacement)
    {
        const std::size_t offset = text.find(original);
        if (offset == std::string::npos ||
            text.find(original, offset + original.size()) != std::string::npos)
        {
            throw std::runtime_error(
                "generated STEP root placement anchor is missing or ambiguous");
        }
        text.replace(offset, original.size(), replacement);
    };
    // The first AXIS2_PLACEMENT_3D item in OCCT's one-root representation is the
    // source coordinate frame consumed by read.step.root.transformation.
    replace_once("#12 = CARTESIAN_POINT('',(0.,0.,0.));", "#12 = CARTESIAN_POINT('',(20.,0.,0.));");
    replace_once("#13 = DIRECTION('',(0.,0.,1.));", "#13 = DIRECTION('',(0.,0.,-1.));");
    replace_once("#14 = DIRECTION('',(1.,0.,-0.));", "#14 = DIRECTION('',(-1.,0.,0.));");
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output || !(output << text))
    {
        throw std::runtime_error("failed to write generated root-placement fixture");
    }
}

void generate_fixtures(const fs::path& directory)
{
    fs::create_directories(directory);
    {
        const Handle(TDocStd_Document) document =
            new TDocStd_Document(TCollection_ExtendedString("BinXCAF"));
        const Handle(XCAFDoc_ShapeTool) shape_tool =
            XCAFDoc_DocumentTool::ShapeTool(document->Main());
        const TDF_Label part =
            shape_tool->AddShape(BRepPrimAPI_MakeBox(2.0, 3.0, 1.0).Shape(), false);
        set_name(part, "repeated_part_definition");
        const Handle(XCAFDoc_MaterialTool) material_tool =
            XCAFDoc_DocumentTool::MaterialTool(document->Main());
        material_tool->SetMaterial(part, new TCollection_HAsciiString("fixture steel"),
                                   new TCollection_HAsciiString("generated AP242 material probe"),
                                   7.85, new TCollection_HAsciiString("density"),
                                   new TCollection_HAsciiString("g/cm^3"));
        const TDF_Label subassembly = shape_tool->NewShape();
        set_name(subassembly, "generated_repeated_subassembly");
        shape_tool->AddComponent(subassembly, part, TopLoc_Location());
        gp_Trsf transform;
        transform.SetTranslation(gp_Vec(5.0, 0.0, 0.0));
        shape_tool->AddComponent(subassembly, part, TopLoc_Location(transform));

        const TDF_Label root = shape_tool->NewShape();
        set_name(root, "generated_repeated_occurrence_root");
        shape_tool->AddComponent(root, subassembly, TopLoc_Location());
        transform.SetRotation(gp_Ax1(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0)),
                              std::acos(-1.0) / 2.0);
        transform.SetTranslationPart(gp_Vec(0.0, 10.0, 0.0));
        shape_tool->AddComponent(root, subassembly, TopLoc_Location(transform));
        shape_tool->UpdateAssemblies();
        write_generated_document(document, directory / "generated_repeated_occurrences.step");
    }
    {
        const Handle(TDocStd_Document) document =
            new TDocStd_Document(TCollection_ExtendedString("BinXCAF"));
        const Handle(XCAFDoc_ShapeTool) shape_tool =
            XCAFDoc_DocumentTool::ShapeTool(document->Main());
        const TopoDS_Shape first = BRepPrimAPI_MakeBox(6.0, 4.0, 1.0).Shape();
        gp_Trsf transform;
        transform.SetTranslation(gp_Vec(3.0, 0.0, 0.0));
        const TopoDS_Shape second =
            BRepPrimAPI_MakeBox(6.0, 4.0, 1.0).Shape().Moved(TopLoc_Location(transform));
        const TopoDS_Shape fused = BRepAlgoAPI_Fuse(first, second).Shape();
        const TDF_Label slab = shape_tool->AddShape(fused, false);
        set_name(slab, "generated_fused_slab");
        const fs::path path = directory / "generated_fused_slab.step";
        write_generated_document(document, path);
        add_generated_root_placement(path);
    }
    {
        const Handle(TDocStd_Document) document =
            new TDocStd_Document(TCollection_ExtendedString("BinXCAF"));
        const Handle(XCAFDoc_ShapeTool) shape_tool =
            XCAFDoc_DocumentTool::ShapeTool(document->Main());
        BRep_Builder builder;
        TopoDS_Compound compound;
        builder.MakeCompound(compound);
        builder.Add(compound, BRepPrimAPI_MakeBox(2.0, 3.0, 1.0).Shape());
        gp_Trsf transform;
        transform.SetTranslation(gp_Vec(10.0, 0.0, 0.0));
        builder.Add(compound,
                    BRepPrimAPI_MakeBox(4.0, 2.0, 2.0).Shape().Moved(TopLoc_Location(transform)));
        const TDF_Label shape = shape_tool->AddShape(compound, false);
        set_name(shape, "generated_flat_multi_solid");
        write_generated_document(document, directory / "generated_flat_multi_solid.step");
    }
}

template <typename Writer> void write_count(Writer& writer, const char* name, std::size_t value)
{
    writer.Key(name);
    writer.Uint64(value);
}

template <typename Writer> void write_observation(Writer& writer, const Observation& observation)
{
    writer.StartObject();
    writer.Key("kind");
    writer.String(observation.kind.c_str());
    writer.Key("step");
    writer.String(observation.step_path.c_str());
    writer.Key("glb");
    if (observation.glb_path.empty())
    {
        writer.Null();
    }
    else
    {
        writer.String(observation.glb_path.c_str());
    }
    writer.Key("source_sha256");
    writer.String(observation.source_sha256.c_str());
    writer.Key("file_schemas");
    writer.StartArray();
    for (const std::string& schema : observation.file_schemas)
    {
        writer.String(schema.c_str());
    }
    writer.EndArray();
    write_count(writer, "step_bytes", observation.step_bytes);
    write_count(writer, "glb_bytes", observation.glb_bytes);
    write_count(writer, "free_shapes", observation.free_shape_count);
    write_count(writer, "definitions", observation.definition_count);
    write_count(writer, "assembly_definitions", observation.assembly_definition_count);
    write_count(writer, "simple_shape_definitions", observation.simple_shape_definition_count);
    write_count(writer, "component_labels", observation.component_label_count);
    write_count(writer, "expanded_occurrence_paths", observation.component_occurrence_count);
    write_count(writer, "located_occurrences", observation.located_occurrence_count);
    write_count(writer, "maximum_occurrence_depth", observation.maximum_occurrence_depth);
    write_count(writer, "solids_in_definitions", observation.solid_count);
    write_count(writer, "shells_in_definitions", observation.shell_count);
    write_count(writer, "faces_in_definitions", observation.face_count);
    write_count(writer, "labeled_subshapes", observation.labeled_subshape_count);
    write_count(writer, "named_labels", observation.named_label_count);
    write_count(writer, "color_assignments", observation.color_assignment_count);
    write_count(writer, "layer_assignments", observation.layer_assignment_count);
    write_count(writer, "material_definitions", observation.material_definition_count);
    write_count(writer, "named_data_labels", observation.named_data_label_count);
    write_count(writer, "compact_snapshot_bytes", observation.compact_snapshot_bytes);
    write_count(writer, "verbose_snapshot_bytes", observation.verbose_snapshot_bytes);
    write_count(writer, "render_meshes", observation.render_meshes);
    write_count(writer, "render_instances", observation.render_instances);
    write_count(writer, "render_vertices", observation.render_vertices);
    write_count(writer, "render_indices", observation.render_indices);
    write_count(writer, "render_primitives", observation.render_primitives);
    write_count(writer, "render_bindings", observation.render_bindings);
    write_count(writer, "render_geometry_triangles", observation.render_geometry_triangles);
    write_count(writer, "render_instanced_triangles", observation.render_instanced_triangles);
    write_count(writer, "render_binding_table_logical_bytes",
                observation.render_binding_table_logical_bytes);
    write_count(writer, "binding_glb_bytes", observation.binding_glb_bytes);
    write_count(writer, "binding_glb_json_bytes", observation.binding_glb_json_bytes);
    write_count(writer, "binding_glb_binary_bytes", observation.binding_glb_binary_bytes);
    write_count(writer, "binding_glb_per_face_draw_calls",
                observation.binding_glb_per_face_draw_calls);
    write_count(writer, "projected_merged_draw_calls", observation.projected_merged_draw_calls);
    writer.Key("occurrence_path_transform_sha256");
    writer.String(observation.occurrence_path_transform_sha256.c_str());
    writer.Key("semantic_evidence_sha256");
    writer.String(observation.semantic_evidence_sha256.c_str());
    writer.EndObject();
}

std::string inventory_json(const std::vector<Observation>& observations)
{
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    writer.SetIndent(' ', 2);
    writer.StartObject();
    writer.Key("schema");
    writer.String("wn.geometer.step_topology_fixture_baseline.a2");
    writer.Key("occt_version");
    writer.String(OCC_VERSION_STRING_EXT);
    writer.Key("step_text_normalization");
    writer.String("CRLF and LF canonicalized to LF for cross-platform fixture evidence");
    writer.Key("reader_modes");
    writer.StartObject();
    const ReaderPosture& posture = research_reader_posture();
    for (const std::pair<const char*, bool>& mode :
         {std::pair{"product", posture.product},
          std::pair{"all_product_contexts", posture.all_product_contexts},
          std::pair{"all_shape_representations", posture.all_shape_representations},
          std::pair{"tessellated", posture.tessellated},
          std::pair{"all_assembly_levels", posture.all_assembly_levels},
          std::pair{"relationships", posture.relationships},
          std::pair{"shape_aspects", posture.shape_aspects},
          std::pair{"constructive_geometry", posture.constructive_geometry},
          std::pair{"subshape_names", posture.subshape_names},
          std::pair{"nonmanifold", posture.nonmanifold},
          std::pair{"all_top_level_shapes", posture.all_top_level_shapes},
          std::pair{"root_transformations", posture.root_transformations},
          std::pair{"colors", posture.colors},
          std::pair{"names", posture.names},
          std::pair{"layers", posture.layers},
          std::pair{"validation_properties", posture.validation_properties},
          std::pair{"metadata", posture.metadata},
          std::pair{"product_metadata", posture.product_metadata},
          std::pair{"shuo", posture.shuo},
          std::pair{"gdt", posture.gdt},
          std::pair{"materials", posture.materials},
          std::pair{"views", posture.views}})
    {
        writer.Key(mode.first);
        writer.Bool(mode.second);
    }
    writer.EndObject();
    writer.Key("payload_measurement");
    writer.String("synthetic normalized tables; diagnostic carrier fields are null placeholders");
    writer.Key("render_measurement");
    writer.String("direct native binding; 0.1 mm absolute linear deflection; 0.5 rad angular "
                  "deflection; serial meshing; identity signed-rigid source-to-render transform; "
                  "logical table bytes count four uint64 fields plus UTF-8 handle bytes; binding "
                  "GLB uses one primitive per face; merged draw calls are projected as one per "
                  "leaf occurrence and are not measured render timings");
    writer.Key("cases");
    writer.StartArray();
    for (const Observation& observation : observations)
    {
        write_observation(writer, observation);
    }
    writer.EndArray();
    writer.EndObject();
    return std::string(buffer.GetString(), buffer.GetSize()) + "\n";
}

std::vector<Observation> inspect_arguments(int argc, char** argv, int first_index)
{
    std::vector<Observation> observations;
    for (int index = first_index; index < argc; index += 3)
    {
        const fs::path glb_path =
            std::string(argv[index + 2]) == "-" ? fs::path{} : fs::path(argv[index + 2]);
        observations.push_back(inspect_step(argv[index], argv[index + 1], glb_path));
    }
    return observations;
}

class ScopedTempDirectory
{
  public:
    ScopedTempDirectory()
    {
        const std::uint64_t timestamp =
            static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
        const std::uint64_t entropy =
            (static_cast<std::uint64_t>(std::random_device{}()) << 32U) ^ std::random_device{}();
        for (std::uint64_t attempt = 0; attempt < 100; ++attempt)
        {
            const fs::path candidate =
                fs::temp_directory_path() /
                ("geometer_topology_fixtures_" + std::to_string(timestamp) + "_" +
                 std::to_string(entropy) + "_" + std::to_string(attempt));
            std::error_code error;
            if (fs::create_directory(candidate, error))
            {
                path_ = candidate;
                return;
            }
            if (error)
            {
                throw std::runtime_error("failed to create unique fixture directory: " +
                                         error.message());
            }
        }
        throw std::runtime_error("failed to allocate unique fixture directory");
    }

    ~ScopedTempDirectory()
    {
        std::error_code ignored;
        fs::remove_all(path_, ignored);
    }

    ScopedTempDirectory(const ScopedTempDirectory&) = delete;
    ScopedTempDirectory& operator=(const ScopedTempDirectory&) = delete;

    const fs::path& path() const
    {
        return path_;
    }

  private:
    fs::path path_;
};

void require_generated_fixture_invariants(const fs::path& directory)
{
    const Observation repeated = inspect_step(
        "generated_repeated_assembly", directory / "generated_repeated_occurrences.step", {});
    if (repeated.definition_count != 3 || repeated.assembly_definition_count != 2 ||
        repeated.simple_shape_definition_count != 1 || repeated.component_label_count != 4 ||
        repeated.component_occurrence_count != 6 || repeated.located_occurrence_count != 4 ||
        repeated.maximum_occurrence_depth != 2 || repeated.solid_count != 1 ||
        repeated.face_count != 6)
    {
        throw std::runtime_error(
            "generated repeated-occurrence fixture lost nested expansion or placement evidence");
    }
    constexpr std::array<std::array<double, 3>, 6> expected_translations = {
        std::array{0.0, 0.0, 0.0},  std::array{0.0, 0.0, 0.0},  std::array{5.0, 0.0, 0.0},
        std::array{0.0, 10.0, 0.0}, std::array{0.0, 10.0, 0.0}, std::array{0.0, 15.0, 0.0}};
    constexpr std::array<double, 9> identity_rotation = {1.0, 0.0, 0.0, 0.0, 1.0,
                                                         0.0, 0.0, 0.0, 1.0};
    constexpr std::array<double, 9> quarter_turn_rotation = {0.0, -1.0, 0.0, 1.0, 0.0,
                                                             0.0, 0.0,  0.0, 1.0};
    for (std::size_t occurrence_index = 0; occurrence_index < repeated.occurrences.size();
         ++occurrence_index)
    {
        const std::array<double, 12>& transform = repeated.occurrences[occurrence_index].transform;
        const std::array<double, 9> rotation = {transform[0], transform[1], transform[2],
                                                transform[4], transform[5], transform[6],
                                                transform[8], transform[9], transform[10]};
        const std::array<double, 9>& expected_rotation =
            occurrence_index < 3 ? identity_rotation : quarter_turn_rotation;
        for (std::size_t index = 0; index < rotation.size(); ++index)
        {
            if (std::abs(rotation[index] - expected_rotation[index]) > 1.0e-9)
            {
                throw std::runtime_error(
                    "generated repeated-occurrence fixture has an unexpected rotation");
            }
        }
        const std::array<double, 3> translation = {transform[3], transform[7], transform[11]};
        for (std::size_t index = 0; index < translation.size(); ++index)
        {
            if (std::abs(translation[index] - expected_translations[occurrence_index][index]) >
                1.0e-9)
            {
                throw std::runtime_error("generated repeated-occurrence fixture lost an expected "
                                         "accumulated translation");
            }
        }
    }
    const Observation fused =
        inspect_step("generated_fused_slab", directory / "generated_fused_slab.step", {});
    if (fused.definition_count != 1 || fused.simple_shape_definition_count != 1 ||
        fused.assembly_definition_count != 0 || fused.component_label_count != 0 ||
        fused.component_occurrence_count != 0 || fused.solid_count != 1 || fused.face_count < 6)
    {
        throw std::runtime_error("generated fused fixture is not one monolithic solid");
    }
    const Observation multi_solid = inspect_step("generated_flat_multi_solid",
                                                 directory / "generated_flat_multi_solid.step", {});
    if (multi_solid.definition_count != 1 || multi_solid.simple_shape_definition_count != 1 ||
        multi_solid.assembly_definition_count != 0 || multi_solid.component_label_count != 0 ||
        multi_solid.component_occurrence_count != 0 || multi_solid.solid_count != 2 ||
        multi_solid.face_count != 12)
    {
        throw std::runtime_error(
            "generated flat multi-solid fixture is not one definition with two solids");
    }
}

void require_generated_fixture_semantics_match(const fs::path& committed_directory,
                                               const fs::path& regenerated_directory)
{
    for (const std::pair<const char*, const char*>& fixture :
         {std::pair{"generated_repeated_assembly", "generated_repeated_occurrences.step"},
          std::pair{"generated_fused_slab", "generated_fused_slab.step"},
          std::pair{"generated_flat_multi_solid", "generated_flat_multi_solid.step"}})
    {
        const Observation committed =
            inspect_step(fixture.first, committed_directory / fixture.second, {});
        const Observation regenerated =
            inspect_step(fixture.first, regenerated_directory / fixture.second, {});
        if (committed.semantic_evidence_sha256 != regenerated.semantic_evidence_sha256 ||
            committed.occurrence_path_transform_sha256 !=
                regenerated.occurrence_path_transform_sha256 ||
            committed.definition_count != regenerated.definition_count ||
            committed.component_label_count != regenerated.component_label_count ||
            committed.component_occurrence_count != regenerated.component_occurrence_count ||
            committed.solid_count != regenerated.solid_count ||
            committed.face_count != regenerated.face_count)
        {
            throw std::runtime_error(std::string("generated fixture semantic evidence is stale: ") +
                                     fixture.second);
        }
    }
}

} // namespace

int main(int argc, char** argv)
{
    try
    {
        if (argc == 3 && std::string(argv[1]) == "--generate")
        {
            const fs::path directory = argv[2];
            generate_fixtures(directory);
            require_generated_fixture_invariants(directory);
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "--verify-generated")
        {
            require_generated_fixture_invariants(argv[2]);
            ScopedTempDirectory regenerated;
            generate_fixtures(regenerated.path());
            require_generated_fixture_invariants(regenerated.path());
            require_generated_fixture_semantics_match(argv[2], regenerated.path());
            return 0;
        }
        if (argc == 2 && std::string(argv[1]) == "--self-test")
        {
            ScopedTempDirectory directory;
            generate_fixtures(directory.path());
            require_generated_fixture_invariants(directory.path());
            return 0;
        }
        if (argc >= 5 && std::string(argv[1]) == "--inventory" && (argc - 2) % 3 == 0)
        {
            std::cout << inventory_json(inspect_arguments(argc, argv, 2));
            return 0;
        }
        if (argc >= 6 && std::string(argv[1]) == "--check-report" && (argc - 3) % 3 == 0)
        {
            const std::vector<std::uint8_t> expected_bytes = read_bytes(argv[2]);
            const std::string expected(expected_bytes.begin(), expected_bytes.end());
            const std::string actual = inventory_json(inspect_arguments(argc, argv, 3));
            if (actual != expected)
            {
                const auto difference =
                    std::mismatch(expected.begin(), expected.end(), actual.begin(), actual.end());
                const std::size_t offset =
                    static_cast<std::size_t>(std::distance(expected.begin(), difference.first));
                const std::size_t context_start = offset > 80U ? offset - 80U : 0U;
                throw std::runtime_error("STEP topology fixture baseline report is stale at byte " +
                                         std::to_string(offset) +
                                         "\nexpected: " + expected.substr(context_start, 160U) +
                                         "\nactual:   " + actual.substr(context_start, 160U));
            }
            return 0;
        }
        std::cerr << "usage: step_topology_fixture_inventory --generate DIR | "
                     "--verify-generated DIR | --self-test | --inventory KIND STEP GLB [... ] | "
                     "--check-report REPORT KIND STEP GLB [... ]\n";
        return 2;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

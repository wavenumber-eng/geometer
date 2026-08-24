#include "step_topology_session_internal.h"

#include "geometer/sha256.h"

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBndLib.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <DESTEP_Parameters.hxx>
#include <GProp_GProps.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <Message_ProgressIndicator.hxx>
#include <Message_ProgressScope.hxx>
#include <NCollection_DataMap.hxx>
#include <NCollection_IndexedMap.hxx>
#include <NCollection_Sequence.hxx>
#include <Standard_Failure.hxx>
#include <StepData_StepModel.hxx>
#include <TCollection_AsciiString.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TDF_ChildIterator.hxx>
#include <TDF_Label.hxx>
#include <TDF_Tool.hxx>
#include <TDataStd_Name.hxx>
#include <TDataStd_NamedData.hxx>
#include <TDataStd_TreeNode.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopTools_ShapeMapHasher.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopoDS_Shape.hxx>
#include <XCAFDoc.hxx>
#include <XCAFDoc_Area.hxx>
#include <XCAFDoc_Centroid.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_LayerTool.hxx>
#include <XCAFDoc_MaterialTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFDoc_Volume.hxx>
#include <XSControl_TransferReader.hxx>
#include <XSControl_WorkSession.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <limits>
#include <locale>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace geometer::step_topology_internal
{
namespace
{

using ShapeMap = NCollection_IndexedMap<TopoDS_Shape, TopTools_ShapeMapHasher>;

class CancellationProgressIndicator final : public Message_ProgressIndicator
{
    DEFINE_STANDARD_RTTIEXT(CancellationProgressIndicator, Message_ProgressIndicator)

  public:
    explicit CancellationProgressIndicator(const StepTopologyCancellation* cancellation)
        : cancellation_(cancellation)
    {
    }

  protected:
    Standard_Boolean UserBreak() override
    {
        return cancellation_ != nullptr && cancellation_->is_cancelled();
    }

    void Show(const Message_ProgressScope&, Standard_Boolean) override {}

  private:
    const StepTopologyCancellation* cancellation_;
};

IMPLEMENT_STANDARD_RTTIEXT(CancellationProgressIndicator, Message_ProgressIndicator)

DESTEP_Parameters step_parameters(const StepReaderPosture& posture)
{
    DESTEP_Parameters parameters;
    parameters.ReadProductMode = posture.product;
    parameters.ReadProductContext = posture.all_product_contexts
                                        ? DESTEP_Parameters::ReadMode_ProductContext_All
                                        : DESTEP_Parameters::ReadMode_ProductContext_Design;
    parameters.ReadShapeRepr = posture.all_shape_representations
                                   ? DESTEP_Parameters::ReadMode_ShapeRepr_All
                                   : DESTEP_Parameters::ReadMode_ShapeRepr_ABSR;
    parameters.ReadTessellated = posture.tessellated ? DESTEP_Parameters::RWMode_Tessellated_On
                                                     : DESTEP_Parameters::RWMode_Tessellated_Off;
    parameters.ReadAssemblyLevel = posture.all_assembly_levels
                                       ? DESTEP_Parameters::ReadMode_AssemblyLevel_All
                                       : DESTEP_Parameters::ReadMode_AssemblyLevel_Assembly;
    parameters.ReadRelationship = posture.relationships;
    parameters.ReadShapeAspect = posture.shape_aspects;
    parameters.ReadConstrRelation = posture.constructive_geometry;
    parameters.ReadSubshapeNames = posture.subshape_names;
    parameters.ReadNonmanifold = posture.nonmanifold;
    parameters.ReadAllShapes = posture.all_top_level_shapes;
    parameters.ReadRootTransformation = posture.root_transformations;
    parameters.ReadColor = posture.colors;
    parameters.ReadName = posture.names;
    parameters.ReadLayer = posture.layers;
    parameters.ReadProps = posture.validation_properties;
    parameters.ReadMetadata = posture.metadata;
    parameters.ReadProductMetadata = posture.product_metadata;
    return parameters;
}

void configure_reader(STEPCAFControl_Reader& reader, const StepReaderPosture& posture)
{
    reader.SetColorMode(posture.colors);
    reader.SetNameMode(posture.names);
    reader.SetLayerMode(posture.layers);
    reader.SetPropsMode(posture.validation_properties);
    reader.SetMetaMode(posture.metadata);
    reader.SetProductMetaMode(posture.product_metadata);
    reader.SetSHUOMode(posture.shuo);
    reader.SetGDTMode(posture.gdt);
    reader.SetMatMode(posture.materials);
    reader.SetViewMode(posture.views);
    if (reader.GetColorMode() != posture.colors || reader.GetNameMode() != posture.names ||
        reader.GetLayerMode() != posture.layers ||
        reader.GetPropsMode() != posture.validation_properties ||
        reader.GetMetaMode() != posture.metadata ||
        reader.GetProductMetaMode() != posture.product_metadata ||
        reader.GetSHUOMode() != posture.shuo || reader.GetGDTMode() != posture.gdt ||
        reader.GetMatMode() != posture.materials || reader.GetViewMode() != posture.views)
    {
        throw std::runtime_error("STEPCAF reader mode configuration did not stick.");
    }
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

std::array<double, 6> shape_bounds(const TopoDS_Shape& shape)
{
    std::array<double, 6> result{};
    Bnd_Box box;
    BRepBndLib::AddOptimal(shape, box, false, false);
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

void face_properties(const TopoDS_Shape& face, double* area, std::array<double, 3>* centroid)
{
    GProp_GProps properties;
    BRepGProp::SurfaceProperties(face, properties);
    *area = properties.Mass();
    const gp_Pnt center = properties.CentreOfMass();
    *centroid = {center.X(), center.Y(), center.Z()};
}

void map_shapes(const TopoDS_Shape& shape, TopAbs_ShapeEnum type, ShapeMap* map);

void hash_size(Sha256Builder* hash, std::size_t value)
{
    std::array<std::uint8_t, 8> encoded{};
    const std::uint64_t narrowed = static_cast<std::uint64_t>(value);
    for (unsigned int shift = 0; shift < 64U; shift += 8U)
        encoded[shift / 8U] = static_cast<std::uint8_t>((narrowed >> shift) & 0xffU);
    hash->update(encoded.data(), encoded.size());
}

int brep_digest(const SessionData& data, const StepTopologyCancellation* cancellation,
                std::string* digest, std::size_t* work_items, Status* status)
{
    *digest = {};
    *work_items = 0;
    Sha256Builder hash;
    const auto cancelled = [&]()
    {
        if (cancellation == nullptr || !cancellation->is_cancelled())
            return false;
        set_status(status, kCancelled, "B-rep evidence hashing was cancelled.");
        return true;
    };
    const auto map_for_digest = [&](const TopoDS_Shape& shape, TopAbs_ShapeEnum type, ShapeMap* map)
    {
        map->Clear();
        if (shape.IsNull())
            return true;
        if (shape.ShapeType() == type)
        {
            map->Add(shape);
            if (*work_items == std::numeric_limits<std::size_t>::max())
                return false;
            ++*work_items;
        }
        for (TopExp_Explorer explorer(shape, type); explorer.More(); explorer.Next())
        {
            if (cancelled() || *work_items == std::numeric_limits<std::size_t>::max())
                return false;
            ++*work_items;
            map->Add(explorer.Current());
        }
        return true;
    };
    hash_size(&hash, data.snapshot.definitions.size());
    for (const StepTopologyDefinition& definition : data.snapshot.definitions)
    {
        if (cancelled())
            return kCancelled;
        const auto found = data.handles.find(definition.handle);
        if (found == data.handles.end() || found->second.kind != StepTopologyTargetKind::definition)
            throw std::runtime_error("Definition handle is missing while hashing the B-rep.");
        const TopoDS_Shape& shape = found->second.shape;
        const auto number = [](double value)
        {
            std::ostringstream stream;
            stream.imbue(std::locale::classic());
            stream << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
            return stream.str();
        };
        const auto append_bounds =
            [&number](std::ostringstream* stream, const std::array<double, 6>& values)
        {
            for (double value : values)
                *stream << '|' << number(value);
        };
        const auto append_point = [&number](std::ostringstream* stream, const gp_Pnt& point)
        {
            *stream << '|' << number(point.X()) << '|' << number(point.Y()) << '|'
                    << number(point.Z());
        };
        const auto hash_sorted = [&hash, &cancelled](std::vector<std::string>* evidence)
        {
            std::sort(evidence->begin(), evidence->end());
            hash_size(&hash, evidence->size());
            for (const std::string& item : *evidence)
            {
                if (cancelled())
                    return false;
                hash_size(&hash, item.size());
                hash.update(reinterpret_cast<const std::uint8_t*>(item.data()), item.size());
            }
            return true;
        };

        std::ostringstream summary;
        summary.imbue(std::locale::classic());
        summary << static_cast<int>(shape.ShapeType()) << '|'
                << static_cast<int>(shape.Orientation()) << '|'
                << static_cast<int>(BRepCheck_Analyzer(shape, true).IsValid()) << '|'
                << number(shape_volume(shape));
        append_bounds(&summary, shape_bounds(shape));
        for (TopAbs_ShapeEnum type : {TopAbs_COMPOUND, TopAbs_COMPSOLID, TopAbs_SOLID, TopAbs_SHELL,
                                      TopAbs_FACE, TopAbs_WIRE, TopAbs_EDGE, TopAbs_VERTEX})
        {
            ShapeMap topology;
            if (!map_for_digest(shape, type, &topology))
                return cancelled() ? kCancelled : kResourceLimit;
            summary << '|' << topology.Extent();
        }
        const std::string summary_text = summary.str();
        hash_size(&hash, summary_text.size());
        hash.update(reinterpret_cast<const std::uint8_t*>(summary_text.data()),
                    summary_text.size());

        std::vector<std::string> face_evidence;
        ShapeMap faces;
        if (!map_for_digest(shape, TopAbs_FACE, &faces))
            return cancelled() ? kCancelled : kResourceLimit;
        face_evidence.reserve(static_cast<std::size_t>(faces.Extent()));
        for (int index = 1; index <= faces.Extent(); ++index)
        {
            if (cancelled())
                return kCancelled;
            const TopoDS_Face face = TopoDS::Face(faces(index));
            double area = 0.0;
            std::array<double, 3> centroid{};
            face_properties(face, &area, &centroid);
            ShapeMap wires;
            ShapeMap edges;
            ShapeMap vertices;
            if (!map_for_digest(face, TopAbs_WIRE, &wires) ||
                !map_for_digest(face, TopAbs_EDGE, &edges) ||
                !map_for_digest(face, TopAbs_VERTEX, &vertices))
                return cancelled() ? kCancelled : kResourceLimit;
            std::ostringstream item;
            item.imbue(std::locale::classic());
            item << static_cast<int>(BRepAdaptor_Surface(face, false).GetType()) << '|'
                 << static_cast<int>(face.Orientation()) << '|'
                 << number(BRep_Tool::Tolerance(face)) << '|' << number(area) << '|'
                 << number(centroid[0]) << '|' << number(centroid[1]) << '|' << number(centroid[2])
                 << '|' << wires.Extent() << '|' << edges.Extent() << '|' << vertices.Extent();
            append_bounds(&item, shape_bounds(face));
            face_evidence.push_back(item.str());
        }
        if (!hash_sorted(&face_evidence))
            return kCancelled;

        std::vector<std::string> edge_evidence;
        ShapeMap edges;
        if (!map_for_digest(shape, TopAbs_EDGE, &edges))
            return cancelled() ? kCancelled : kResourceLimit;
        edge_evidence.reserve(static_cast<std::size_t>(edges.Extent()));
        for (int index = 1; index <= edges.Extent(); ++index)
        {
            if (cancelled())
                return kCancelled;
            const TopoDS_Edge edge = TopoDS::Edge(edges(index));
            GProp_GProps properties;
            BRepGProp::LinearProperties(edge, properties);
            ShapeMap vertices;
            if (!map_for_digest(edge, TopAbs_VERTEX, &vertices))
                return cancelled() ? kCancelled : kResourceLimit;
            std::ostringstream item;
            item.imbue(std::locale::classic());
            item << static_cast<int>(BRepAdaptor_Curve(edge).GetType()) << '|'
                 << static_cast<int>(edge.Orientation()) << '|'
                 << number(BRep_Tool::Tolerance(edge)) << '|' << number(properties.Mass()) << '|'
                 << vertices.Extent();
            append_point(&item, properties.CentreOfMass());
            append_bounds(&item, shape_bounds(edge));
            edge_evidence.push_back(item.str());
        }
        if (!hash_sorted(&edge_evidence))
            return kCancelled;

        std::vector<std::string> vertex_evidence;
        ShapeMap vertices;
        if (!map_for_digest(shape, TopAbs_VERTEX, &vertices))
            return cancelled() ? kCancelled : kResourceLimit;
        vertex_evidence.reserve(static_cast<std::size_t>(vertices.Extent()));
        for (int index = 1; index <= vertices.Extent(); ++index)
        {
            if (cancelled())
                return kCancelled;
            const TopoDS_Vertex vertex = TopoDS::Vertex(vertices(index));
            std::ostringstream item;
            item.imbue(std::locale::classic());
            item << number(BRep_Tool::Tolerance(vertex));
            append_point(&item, BRep_Tool::Pnt(vertex));
            vertex_evidence.push_back(item.str());
        }
        if (!hash_sorted(&vertex_evidence))
            return kCancelled;
    }
    *digest = hash.hex_digest();
    return 0;
}

void map_shapes(const TopoDS_Shape& shape, TopAbs_ShapeEnum type, ShapeMap* map)
{
    if (!shape.IsNull())
    {
        TopExp::MapShapes(shape, type, *map);
    }
}

class SnapshotBuilder
{
  public:
    SnapshotBuilder(SessionData* data, const StepTopologyCancellation* cancellation)
        : data_(data), cancellation_(cancellation),
          shape_tool_(XCAFDoc_DocumentTool::ShapeTool(data->document->Main())),
          color_tool_(XCAFDoc_DocumentTool::ColorTool(data->document->Main())),
          layer_tool_(XCAFDoc_DocumentTool::LayerTool(data->document->Main())),
          material_tool_(XCAFDoc_DocumentTool::MaterialTool(data->document->Main())),
          step_model_(data->reader.Reader().StepModel()),
          transfer_reader_(data->reader.Reader().WS()->TransferReader())
    {
        data_->snapshot.research_format = "geometer.step_topology_inspection.research";
        data_->snapshot.session = data_->info;
        data_->snapshot.reader_posture = data_->reader_posture;
    }

    int build(Status* status)
    {
        status_ = status;
        if (cancel_requested())
        {
            return status_code_;
        }
        if (!index_source_shape_results())
        {
            return status_code_;
        }
        NCollection_Sequence<TDF_Label> free_shapes;
        shape_tool_->GetFreeShapes(free_shapes);
        data_->snapshot.free_shape_count = static_cast<std::size_t>(free_shapes.Length());
        std::set<std::string> definition_path;
        for (int index = 1; index <= free_shapes.Length(); ++index)
        {
            if (!visit_definition(free_shapes.Value(index), 0, definition_path))
            {
                return status_code_;
            }
        }
        std::vector<std::string> root_handles;
        root_handles.reserve(static_cast<std::size_t>(free_shapes.Length()));
        for (int index = 1; index <= free_shapes.Length(); ++index)
        {
            const TDF_Label root = free_shapes.Value(index);
            const auto definition = definition_indices_.find(label_entry(root));
            if (definition == definition_indices_.end())
            {
                return fail(kInternalFailure, "Free shape has no normalized definition.");
            }
            const TopLoc_Location root_location = XCAFDoc_ShapeTool::GetLocation(root);
            const TopoDS_Shape root_shape = local_definition_shape(root).Moved(root_location);
            StepTopologyRootOccurrence root_occurrence;
            if (!new_handle(StepTopologyTargetKind::occurrence, root_shape,
                            &root_occurrence.handle))
            {
                return status_code_;
            }
            root_occurrence.definition_handle =
                data_->snapshot.definitions[definition->second].handle;
            root_occurrence.transform = transform_values(root_location);
            root_occurrence.label = label_summary(root);
            if (status_code_ != 0)
            {
                return status_code_;
            }
            root_handles.push_back(root_occurrence.handle);
            data_->snapshot.root_occurrences.push_back(std::move(root_occurrence));
            add_diagnostic(data_->snapshot.root_occurrences.back().handle, root);
            if (status_code_ != 0)
            {
                return status_code_;
            }
        }
        std::set<std::string> occurrence_path;
        for (int index = 1; index <= free_shapes.Length(); ++index)
        {
            const TDF_Label root = free_shapes.Value(index);
            if (!expand_occurrences(root, XCAFDoc_ShapeTool::GetLocation(root),
                                    root_handles[static_cast<std::size_t>(index - 1)], 0,
                                    occurrence_path))
            {
                return status_code_;
            }
        }
        count_document_metadata();
        if (!count_memberships())
            return status_code_;
        data_->snapshot.source_transfer_work_items = transfer_work_item_count_;
        return 0;
    }

  private:
    struct BodyDraft
    {
        TopoDS_Shape shape;
        std::string kind;
        std::vector<int> shells;
        std::vector<int> faces;
    };

    bool fail(int code, const std::string& message)
    {
        status_code_ = code;
        set_status(status_, code, message);
        return false;
    }

    bool cancel_requested()
    {
        if (cancellation_ == nullptr || !cancellation_->is_cancelled())
        {
            return false;
        }
        fail(kCancelled, "STEP topology inspection was cancelled.");
        return true;
    }

    bool bounded(std::size_t value, std::size_t limit, const char* what)
    {
        if (value > limit)
        {
            return fail(kResourceLimit,
                        std::string("STEP topology ") + what + " exceeds the configured limit.");
        }
        return true;
    }

    bool count_memberships()
    {
        constexpr std::size_t kMaxMemberships = 5000000U;
        std::size_t count = 0U;
        const auto add = [&](std::size_t value)
        {
            if (value > kMaxMemberships - count)
                return false;
            count += value;
            return true;
        };
        for (const auto& body : data_->snapshot.bodies)
        {
            if (!add(body.shell_handles.size()) || !add(body.face_handles.size()))
                return fail(kResourceLimit,
                            "STEP topology membership-edge count exceeds the wire contract limit.");
        }
        for (const auto& shell : data_->snapshot.shells)
        {
            if (!add(shell.face_handles.size()))
                return fail(kResourceLimit,
                            "STEP topology membership-edge count exceeds the wire contract limit.");
        }
        data_->snapshot.membership_count = count;
        return true;
    }

    bool add_accounted_string(const std::string& value)
    {
        if (!account_string(data_, value, status_))
        {
            status_code_ = kResourceLimit;
            return false;
        }
        return true;
    }

    bool new_handle(StepTopologyTargetKind kind, const TopoDS_Shape& shape, std::string* handle)
    {
        if (data_->handles.size() >= data_->limits.max_handles)
        {
            return fail(kResourceLimit, "STEP topology handle count exceeds the configured limit.");
        }
        *handle = issue_handle(data_, kind, shape);
        return true;
    }

    TopoDS_Shape local_definition_shape(const TDF_Label& definition) const
    {
        const TopoDS_Shape shape = XCAFDoc_ShapeTool::GetShape(definition);
        return shape.IsNull() ? shape : shape.Located(TopLoc_Location());
    }

    StepTopologyLabelSummary label_summary(const TDF_Label& label)
    {
        StepTopologyLabelSummary summary;
        if (label.IsNull())
        {
            return summary;
        }
        summary.present = true;
        summary.name = label_name(label);
        if (!add_accounted_string(summary.name))
        {
            return {};
        }
        for (const XCAFDoc_ColorType color_type :
             {XCAFDoc_ColorGen, XCAFDoc_ColorSurf, XCAFDoc_ColorCurv})
        {
            if (color_tool_->IsSet(label, color_type))
            {
                ++summary.color_assignments;
            }
        }
        NCollection_Sequence<TDF_Label> layers;
        if (layer_tool_->GetLayers(label, layers))
        {
            summary.layer_assignments = static_cast<std::size_t>(layers.Length());
        }
        summary.has_named_data = label.IsAttribute(TDataStd_NamedData::GetID());
        Handle(TDataStd_TreeNode) material_reference;
        summary.has_material_assignment =
            label.FindAttribute(XCAFDoc::MaterialRefGUID(), material_reference) &&
            material_reference->HasFather();
        summary.has_validation_properties = label.IsAttribute(XCAFDoc_Area::GetID()) ||
                                            label.IsAttribute(XCAFDoc_Volume::GetID()) ||
                                            label.IsAttribute(XCAFDoc_Centroid::GetID());
        return summary;
    }

    TDF_Label topology_label(const TDF_Label& definition, const TopoDS_Shape& shape,
                             const TopLoc_Location& source_location) const
    {
        const TopoDS_Shape source_shape = shape.Moved(source_location);
        if (source_shape.IsSame(XCAFDoc_ShapeTool::GetShape(definition)))
        {
            return definition;
        }
        TDF_Label subshape;
        shape_tool_->FindSubShape(definition, source_shape, subshape);
        return subshape;
    }

    void add_diagnostic(const std::string& handle, const TDF_Label& label)
    {
        if (label.IsNull())
        {
            return;
        }
        const std::string entry = label_entry(label);
        if (!add_accounted_string(entry))
        {
            return;
        }
        data_->snapshot.diagnostic_carriers.push_back({handle, entry});
    }

    StepSourceEntityEvidence source_evidence(const TopoDS_Shape& shape)
    {
        StepSourceEntityEvidence evidence;
        if (shape.IsNull() || transfer_reader_.IsNull() || step_model_.IsNull())
        {
            ++data_->snapshot.metadata.unmapped_source_entities;
            return evidence;
        }
        Handle(Standard_Transient) entity = transfer_reader_->EntityFromShapeResult(shape, 3);
        if (!entity.IsNull())
        {
            evidence.mapping_method = "entity_from_shape_result";
        }
        else if (exact_source_shape_entities_.IsBound(shape))
        {
            entity = exact_source_shape_entities_.Find(shape);
            evidence.mapping_method = "exact_shape_result_index";
        }
        else if (containing_source_shape_entities_.IsBound(shape))
        {
            entity = containing_source_shape_entities_.Find(shape);
            evidence.mapping_method = "containing_shape_result";
        }
        if (entity.IsNull())
        {
            ++data_->snapshot.metadata.unmapped_source_entities;
            return evidence;
        }
        evidence.mapped = true;
        evidence.model_number = step_model_->Number(entity);
        evidence.entity_type = entity->DynamicType()->Name();
        const TopoDS_Shape reverse_shape = transfer_reader_->ShapeResult(entity);
        evidence.shape_result_round_trip = !reverse_shape.IsNull() && reverse_shape.IsSame(shape);
        if (!add_accounted_string(evidence.entity_type) ||
            !add_accounted_string(evidence.mapping_method))
        {
            return {};
        }
        ++data_->snapshot.metadata.mapped_source_entities;
        return evidence;
    }

    bool index_source_shape_results()
    {
        if (transfer_reader_.IsNull() || step_model_.IsNull())
        {
            return true;
        }
        for (int index = 1; index <= step_model_->NbEntities(); ++index)
        {
            if (!charge_transfer_work())
            {
                return false;
            }
            const Handle(Standard_Transient) entity = step_model_->Value(index);
            if (entity.IsNull())
            {
                continue;
            }
            const TopoDS_Shape shape = transfer_reader_->ShapeResult(entity);
            if (shape.IsNull())
            {
                continue;
            }
            if (!exact_source_shape_entities_.IsBound(shape))
            {
                if (transfer_index_shape_count_ >= data_->limits.max_transfer_index_shapes)
                {
                    return fail(kResourceLimit,
                                "STEP transfer shape index exceeds the configured limit.");
                }
                exact_source_shape_entities_.Bind(shape, entity);
                ++transfer_index_shape_count_;
            }
            if (!index_containing_shape(shape, entity))
            {
                return false;
            }
        }
        return true;
    }

    bool index_containing_shape(const TopoDS_Shape& shape,
                                const Handle(Standard_Transient) & entity)
    {
        if (!charge_transfer_work())
        {
            return false;
        }
        std::vector<TopoDS_Shape> pending = {shape};
        while (!pending.empty())
        {
            const TopoDS_Shape current = pending.back();
            pending.pop_back();
            if (!containing_source_shape_entities_.IsBound(current))
            {
                if (transfer_index_shape_count_ >= data_->limits.max_transfer_index_shapes)
                {
                    return fail(kResourceLimit,
                                "STEP transfer shape index exceeds the configured limit.");
                }
                containing_source_shape_entities_.Bind(current, entity);
                ++transfer_index_shape_count_;
            }
            // Cumulative orientation/location matches TopExp::MapShapes defaults. Charge before
            // retaining each child so deeply nested or highly branching shapes fail preemptively.
            for (TopoDS_Iterator iterator(current, true, true); iterator.More(); iterator.Next())
            {
                if (!charge_transfer_work())
                {
                    return false;
                }
                pending.push_back(iterator.Value());
            }
        }
        return true;
    }

    bool charge_transfer_work()
    {
        if (cancel_requested())
        {
            return false;
        }
        if (transfer_work_item_count_ >= data_->limits.max_transfer_work_items)
        {
            return fail(kResourceLimit,
                        "STEP transfer indexing work exceeds the configured limit.");
        }
        ++transfer_work_item_count_;
        return true;
    }

    bool visit_definition(const TDF_Label& definition, std::size_t depth,
                          std::set<std::string>& recursion_path)
    {
        const std::string entry = label_entry(definition);
        if (depth > 64 || recursion_path.count(entry) != 0)
        {
            return fail(kResourceLimit, "STEP topology definition graph is cyclic or too deep.");
        }
        if (definition_indices_.count(entry) != 0)
        {
            return true;
        }
        if (!bounded(data_->snapshot.definitions.size() + 1U, data_->limits.max_definitions,
                     "definition count"))
        {
            return false;
        }
        recursion_path.insert(entry);

        const TopoDS_Shape imported_shape = XCAFDoc_ShapeTool::GetShape(definition);
        const TopLoc_Location source_location = imported_shape.Location();
        const TopoDS_Shape shape = local_definition_shape(definition);
        StepTopologyDefinition record;
        if (!new_handle(StepTopologyTargetKind::definition, shape, &record.handle))
        {
            return false;
        }
        record.is_assembly = XCAFDoc_ShapeTool::IsAssembly(definition);
        record.label = label_summary(definition);
        if (status_code_ != 0)
        {
            return false;
        }
        record.source_entity = source_evidence(imported_shape);
        if (status_code_ != 0)
        {
            return false;
        }
        const std::size_t definition_index = data_->snapshot.definitions.size();
        definition_indices_.emplace(entry, definition_index);
        data_->snapshot.definitions.push_back(record);
        add_diagnostic(record.handle, definition);
        if (status_code_ != 0)
        {
            return false;
        }

        if (!record.is_assembly &&
            !build_definition_topology(definition_index, definition, shape, source_location))
        {
            return false;
        }

        NCollection_Sequence<TDF_Label> components;
        if (XCAFDoc_ShapeTool::GetComponents(definition, components, false))
        {
            if (!bounded(data_->snapshot.component_label_count +
                             static_cast<std::size_t>(components.Length()),
                         data_->limits.max_component_labels, "component-label count"))
            {
                return false;
            }
            data_->snapshot.component_label_count += static_cast<std::size_t>(components.Length());
            for (int index = 1; index <= components.Length(); ++index)
            {
                TDF_Label referred;
                if (!XCAFDoc_ShapeTool::GetReferredShape(components.Value(index), referred))
                {
                    return fail(kTransferFailed,
                                "XCAF component has no referred shape definition.");
                }
                if (!visit_definition(referred, depth + 1, recursion_path))
                {
                    return false;
                }
            }
        }
        recursion_path.erase(entry);
        return true;
    }

    bool build_definition_topology(std::size_t definition_index, const TDF_Label& definition,
                                   const TopoDS_Shape& shape,
                                   const TopLoc_Location& source_location)
    {
        ShapeMap solids;
        ShapeMap shells;
        ShapeMap faces;
        map_shapes(shape, TopAbs_SOLID, &solids);
        map_shapes(shape, TopAbs_SHELL, &shells);
        map_shapes(shape, TopAbs_FACE, &faces);
        if (!bounded(data_->snapshot.shells.size() + static_cast<std::size_t>(shells.Extent()),
                     data_->limits.max_shells, "shell count") ||
            !bounded(data_->snapshot.faces.size() + static_cast<std::size_t>(faces.Extent()),
                     data_->limits.max_faces, "face count"))
        {
            return false;
        }

        std::vector<BodyDraft> bodies;
        std::vector<bool> assigned_shell(static_cast<std::size_t>(shells.Extent()) + 1U, false);
        std::vector<bool> assigned_face(static_cast<std::size_t>(faces.Extent()) + 1U, false);
        for (int solid_index = 1; solid_index <= solids.Extent(); ++solid_index)
        {
            BodyDraft body;
            body.shape = solids.FindKey(solid_index);
            body.kind = "solid";
            ShapeMap body_shells;
            ShapeMap body_faces;
            map_shapes(body.shape, TopAbs_SHELL, &body_shells);
            map_shapes(body.shape, TopAbs_FACE, &body_faces);
            for (int index = 1; index <= body_shells.Extent(); ++index)
            {
                const int global = shells.FindIndex(body_shells.FindKey(index));
                if (global > 0)
                {
                    body.shells.push_back(global);
                    assigned_shell[static_cast<std::size_t>(global)] = true;
                }
            }
            for (int index = 1; index <= body_faces.Extent(); ++index)
            {
                const int global = faces.FindIndex(body_faces.FindKey(index));
                if (global > 0)
                {
                    body.faces.push_back(global);
                    assigned_face[static_cast<std::size_t>(global)] = true;
                }
            }
            bodies.push_back(std::move(body));
        }
        for (int shell_index = 1; shell_index <= shells.Extent(); ++shell_index)
        {
            if (assigned_shell[static_cast<std::size_t>(shell_index)])
                continue;
            BodyDraft body;
            body.shape = shells.FindKey(shell_index);
            body.kind = "shell";
            body.shells.push_back(shell_index);
            ShapeMap body_faces;
            map_shapes(body.shape, TopAbs_FACE, &body_faces);
            for (int index = 1; index <= body_faces.Extent(); ++index)
            {
                const int global = faces.FindIndex(body_faces.FindKey(index));
                if (global > 0)
                {
                    body.faces.push_back(global);
                    assigned_face[static_cast<std::size_t>(global)] = true;
                }
            }
            bodies.push_back(std::move(body));
        }
        for (int face_index = 1; face_index <= faces.Extent(); ++face_index)
        {
            if (assigned_face[static_cast<std::size_t>(face_index)])
                continue;
            BodyDraft body;
            body.shape = faces.FindKey(face_index);
            body.kind = "surface";
            body.faces.push_back(face_index);
            bodies.push_back(std::move(body));
        }
        if (!bounded(data_->snapshot.bodies.size() + bodies.size(), data_->limits.max_bodies,
                     "body count"))
        {
            return false;
        }

        std::vector<std::string> shell_handles(static_cast<std::size_t>(shells.Extent()) + 1U);
        std::vector<std::string> face_handles(static_cast<std::size_t>(faces.Extent()) + 1U);
        const std::string definition_handle = data_->snapshot.definitions[definition_index].handle;
        const std::size_t shell_offset = data_->snapshot.shells.size();
        for (int shell_index = 1; shell_index <= shells.Extent(); ++shell_index)
        {
            const TopoDS_Shape shell_shape = shells.FindKey(shell_index);
            StepTopologyShell shell;
            if (!new_handle(StepTopologyTargetKind::shell, shell_shape, &shell.handle))
                return false;
            shell_handles[static_cast<std::size_t>(shell_index)] = shell.handle;
            shell.definition_handle = definition_handle;
            const TDF_Label label = topology_label(definition, shell_shape, source_location);
            shell.label = label_summary(label);
            if (status_code_ != 0)
                return false;
            shell.source_entity = source_evidence(shell_shape.Moved(source_location));
            if (status_code_ != 0)
                return false;
            data_->snapshot.shells.push_back(std::move(shell));
            add_diagnostic(data_->snapshot.shells.back().handle, label);
            if (status_code_ != 0)
                return false;
        }
        const std::size_t face_offset = data_->snapshot.faces.size();
        data_->snapshot.definitions[definition_index].face_count +=
            static_cast<std::size_t>(faces.Extent());
        for (int face_index = 1; face_index <= faces.Extent(); ++face_index)
        {
            const TopoDS_Shape face_shape = faces.FindKey(face_index);
            StepTopologyFace face;
            if (!new_handle(StepTopologyTargetKind::face, face_shape, &face.handle))
                return false;
            face_handles[static_cast<std::size_t>(face_index)] = face.handle;
            face.definition_handle = definition_handle;
            face.bounds = shape_bounds(face_shape);
            face_properties(face_shape, &face.area, &face.centroid);
            const TDF_Label label = topology_label(definition, face_shape, source_location);
            face.label = label_summary(label);
            if (status_code_ != 0)
                return false;
            face.source_entity = source_evidence(face_shape.Moved(source_location));
            if (status_code_ != 0)
                return false;
            data_->snapshot.faces.push_back(std::move(face));
            add_diagnostic(data_->snapshot.faces.back().handle, label);
            if (status_code_ != 0)
                return false;
        }

        for (const BodyDraft& draft : bodies)
        {
            StepTopologyBody body;
            if (!new_handle(StepTopologyTargetKind::body, draft.shape, &body.handle))
                return false;
            body.definition_handle = definition_handle;
            body.topology_kind = draft.kind;
            body.bounds = shape_bounds(draft.shape);
            body.volume = draft.kind == "solid" ? shape_volume(draft.shape) : 0.0;
            const TDF_Label label = topology_label(definition, draft.shape, source_location);
            body.label = label_summary(label);
            if (status_code_ != 0)
                return false;
            body.source_entity = source_evidence(draft.shape.Moved(source_location));
            if (status_code_ != 0)
                return false;
            for (int index : draft.shells)
            {
                body.shell_handles.push_back(shell_handles[static_cast<std::size_t>(index)]);
            }
            for (int index : draft.faces)
            {
                body.face_handles.push_back(face_handles[static_cast<std::size_t>(index)]);
            }
            data_->snapshot.bodies.push_back(std::move(body));
            StepTopologyBody& stored_body = data_->snapshot.bodies.back();
            data_->snapshot.definitions[definition_index].body_handles.push_back(
                stored_body.handle);
            for (int index : draft.shells)
            {
                data_->snapshot.shells[shell_offset + static_cast<std::size_t>(index - 1)]
                    .body_handles.push_back(stored_body.handle);
            }
            for (int index : draft.faces)
            {
                data_->snapshot.faces[face_offset + static_cast<std::size_t>(index - 1)]
                    .body_handles.push_back(stored_body.handle);
            }
            add_diagnostic(stored_body.handle, label);
            if (status_code_ != 0)
                return false;
        }

        for (int shell_index = 1; shell_index <= shells.Extent(); ++shell_index)
        {
            ShapeMap shell_faces;
            map_shapes(shells.FindKey(shell_index), TopAbs_FACE, &shell_faces);
            StepTopologyShell& stored_shell =
                data_->snapshot.shells[shell_offset + static_cast<std::size_t>(shell_index - 1)];
            for (int index = 1; index <= shell_faces.Extent(); ++index)
            {
                const int global = faces.FindIndex(shell_faces.FindKey(index));
                if (global <= 0)
                    continue;
                const std::string& face_handle = face_handles[static_cast<std::size_t>(global)];
                stored_shell.face_handles.push_back(face_handle);
                data_->snapshot.faces[face_offset + static_cast<std::size_t>(global - 1)]
                    .shell_handles.push_back(stored_shell.handle);
            }
        }
        return true;
    }

    bool expand_occurrences(const TDF_Label& definition, const TopLoc_Location& parent_location,
                            const std::string& parent_occurrence_handle, std::size_t depth,
                            std::set<std::string>& recursion_path)
    {
        const std::string definition_entry = label_entry(definition);
        if (depth > 64 || !recursion_path.insert(definition_entry).second)
        {
            return fail(kResourceLimit, "STEP topology occurrence graph is cyclic or too deep.");
        }
        NCollection_Sequence<TDF_Label> components;
        if (XCAFDoc_ShapeTool::GetComponents(definition, components, false))
        {
            for (int index = 1; index <= components.Length(); ++index)
            {
                if (!bounded(data_->snapshot.occurrences.size() + 1U,
                             data_->limits.max_expanded_occurrences, "expanded occurrence count"))
                {
                    return false;
                }
                const TDF_Label component = components.Value(index);
                TDF_Label referred;
                if (!XCAFDoc_ShapeTool::GetReferredShape(component, referred))
                {
                    return fail(kTransferFailed,
                                "XCAF component has no referred shape definition.");
                }
                const auto known = definition_indices_.find(label_entry(referred));
                if (known == definition_indices_.end())
                {
                    return fail(kInternalFailure,
                                "Expanded occurrence references an unknown definition.");
                }
                const TopLoc_Location location =
                    parent_location.Multiplied(XCAFDoc_ShapeTool::GetLocation(component));
                const TopoDS_Shape definition_shape = local_definition_shape(referred);
                const TopoDS_Shape occurrence_shape = definition_shape.Moved(location);
                StepTopologyOccurrence occurrence;
                if (!new_handle(StepTopologyTargetKind::occurrence, occurrence_shape,
                                &occurrence.handle))
                {
                    return false;
                }
                occurrence.definition_handle = data_->snapshot.definitions[known->second].handle;
                occurrence.parent_occurrence_handle = parent_occurrence_handle;
                occurrence.depth = depth + 1;
                occurrence.transform = transform_values(location);
                occurrence.label = label_summary(component);
                if (status_code_ != 0)
                    return false;
                data_->snapshot.occurrences.push_back(occurrence);
                add_diagnostic(occurrence.handle, component);
                if (status_code_ != 0)
                    return false;
                if (!expand_occurrences(referred, location, occurrence.handle, depth + 1,
                                        recursion_path))
                {
                    return false;
                }
            }
        }
        recursion_path.erase(definition_entry);
        return true;
    }

    void count_document_metadata()
    {
        NCollection_Sequence<TDF_Label> materials;
        material_tool_->GetMaterialLabels(materials);
        data_->snapshot.metadata.material_definitions =
            static_cast<std::size_t>(materials.Length());
        for (TDF_ChildIterator iterator(data_->document->Main(), true); iterator.More();
             iterator.Next())
        {
            const TDF_Label label = iterator.Value();
            if (label.IsAttribute(TDataStd_Name::GetID()))
                ++data_->snapshot.metadata.named_labels;
            if (label.IsAttribute(TDataStd_NamedData::GetID()))
                ++data_->snapshot.metadata.named_data_labels;
            if (label.IsAttribute(XCAFDoc_Area::GetID()) ||
                label.IsAttribute(XCAFDoc_Volume::GetID()) ||
                label.IsAttribute(XCAFDoc_Centroid::GetID()))
                ++data_->snapshot.metadata.validation_property_labels;
            for (const XCAFDoc_ColorType color_type :
                 {XCAFDoc_ColorGen, XCAFDoc_ColorSurf, XCAFDoc_ColorCurv})
            {
                if (color_tool_->IsSet(label, color_type))
                    ++data_->snapshot.metadata.color_assignments;
            }
            NCollection_Sequence<TDF_Label> layers;
            if (layer_tool_->GetLayers(label, layers))
                data_->snapshot.metadata.layer_assignments +=
                    static_cast<std::size_t>(layers.Length());
            Handle(TDataStd_TreeNode) material_reference;
            if (label.FindAttribute(XCAFDoc::MaterialRefGUID(), material_reference) &&
                material_reference->HasFather())
            {
                ++data_->snapshot.metadata.material_assignments;
            }
        }
    }

    SessionData* data_;
    const StepTopologyCancellation* cancellation_;
    Handle(XCAFDoc_ShapeTool) shape_tool_;
    Handle(XCAFDoc_ColorTool) color_tool_;
    Handle(XCAFDoc_LayerTool) layer_tool_;
    Handle(XCAFDoc_MaterialTool) material_tool_;
    Handle(StepData_StepModel) step_model_;
    Handle(XSControl_TransferReader) transfer_reader_;
    NCollection_DataMap<TopoDS_Shape, Handle(Standard_Transient), TopTools_ShapeMapHasher>
        exact_source_shape_entities_;
    NCollection_DataMap<TopoDS_Shape, Handle(Standard_Transient), TopTools_ShapeMapHasher>
        containing_source_shape_entities_;
    std::unordered_map<std::string, std::size_t> definition_indices_;
    Status* status_ = nullptr;
    int status_code_ = 0;
    std::size_t transfer_index_shape_count_ = 0;
    std::size_t transfer_work_item_count_ = 0;
};

} // namespace

int import_step_session(SessionData* data, const StepTopologyCancellation* cancellation,
                        Status* status)
{
    try
    {
        if (cancellation != nullptr && cancellation->is_cancelled())
        {
            set_status(status, kCancelled, "STEP topology import was cancelled.");
            return kCancelled;
        }
        data->document = new TDocStd_Document(TCollection_ExtendedString("BinXCAF"));
        configure_reader(data->reader, data->reader_posture);
        const DESTEP_Parameters parameters = step_parameters(data->reader_posture);
        const std::string source(reinterpret_cast<const char*>(data->source.data()),
                                 data->source.size());
        std::istringstream stream(source);
        if (data->reader.ChangeReader().ReadStream("memory.step", parameters, stream) !=
            IFSelect_RetDone)
        {
            set_status(status, kReadFailed, "OCCT failed reading STEP session bytes.");
            return kReadFailed;
        }
        if (cancellation != nullptr && cancellation->is_cancelled())
        {
            set_status(status, kCancelled, "STEP topology import was cancelled.");
            return kCancelled;
        }
        Handle(CancellationProgressIndicator) progress =
            new CancellationProgressIndicator(cancellation);
        if (!data->reader.Transfer(data->document, progress->Start()))
        {
            if (cancellation != nullptr && cancellation->is_cancelled())
            {
                set_status(status, kCancelled, "STEP topology transfer was cancelled.");
                return kCancelled;
            }
            set_status(status, kTransferFailed, "OCCT failed transferring STEP session into XCAF.");
            return kTransferFailed;
        }
        if (cancellation != nullptr && cancellation->is_cancelled())
        {
            set_status(status, kCancelled, "STEP topology transfer was cancelled.");
            return kCancelled;
        }
        set_status(status, 0, "");
        return 0;
    }
    catch (const Standard_Failure& failure)
    {
        set_status(status, kInternalFailure, failure.GetMessageString());
        return kInternalFailure;
    }
    catch (const std::exception& error)
    {
        set_status(status, kInternalFailure, error.what());
        return kInternalFailure;
    }
}

int rebuild_snapshot(SessionData* data, const StepTopologyCancellation* cancellation,
                     Status* status)
{
    StepTopologySnapshot previous_snapshot = std::move(data->snapshot);
    auto previous_handles = std::move(data->handles);
    const std::size_t previous_string_bytes = data->total_string_bytes;
    const std::size_t previous_snapshot_string_bytes = data->snapshot_string_bytes;
    const std::size_t previous_probe_string_bytes = data->metadata_probe_string_bytes;
    const std::size_t previous_journal_string_bytes = data->journal_string_bytes;
    const std::size_t previous_estimate = data->info.estimated_resident_bytes;
    const std::size_t previous_accounted_string_bytes = data->info.accounted_string_bytes;
    data->snapshot = {};
    data->handles.clear();
    data->snapshot_string_bytes = 0;
    data->metadata_probe_string_bytes = 0;
    data->journal_string_bytes = 0;
    data->total_string_bytes = 0;
    try
    {
        SnapshotBuilder builder(data, cancellation);
        const int code = builder.build(status);
        if (code != 0)
        {
            data->snapshot = std::move(previous_snapshot);
            data->handles = std::move(previous_handles);
            data->snapshot_string_bytes = previous_snapshot_string_bytes;
            data->metadata_probe_string_bytes = previous_probe_string_bytes;
            data->journal_string_bytes = previous_journal_string_bytes;
            data->total_string_bytes = previous_string_bytes;
            data->info.estimated_resident_bytes = previous_estimate;
            data->info.accounted_string_bytes = previous_accounted_string_bytes;
            return code;
        }
        const int brep_code = brep_digest(*data, cancellation, &data->snapshot.brep_sha256,
                                          &data->snapshot.brep_digest_work_items, status);
        if (brep_code != 0)
        {
            data->snapshot = std::move(previous_snapshot);
            data->handles = std::move(previous_handles);
            data->snapshot_string_bytes = previous_snapshot_string_bytes;
            data->metadata_probe_string_bytes = previous_probe_string_bytes;
            data->journal_string_bytes = previous_journal_string_bytes;
            data->total_string_bytes = previous_string_bytes;
            data->info.estimated_resident_bytes = previous_estimate;
            data->info.accounted_string_bytes = previous_accounted_string_bytes;
            return brep_code;
        }
        if (!account_string(data, data->snapshot.brep_sha256, status))
        {
            data->snapshot = std::move(previous_snapshot);
            data->handles = std::move(previous_handles);
            data->snapshot_string_bytes = previous_snapshot_string_bytes;
            data->metadata_probe_string_bytes = previous_probe_string_bytes;
            data->journal_string_bytes = previous_journal_string_bytes;
            data->total_string_bytes = previous_string_bytes;
            data->info.estimated_resident_bytes = previous_estimate;
            data->info.accounted_string_bytes = previous_accounted_string_bytes;
            return kResourceLimit;
        }
        data->snapshot_string_bytes = data->total_string_bytes;
        const int group_string_code = account_logical_group_strings(data, cancellation, status);
        if (group_string_code != 0)
        {
            data->snapshot = std::move(previous_snapshot);
            data->handles = std::move(previous_handles);
            data->snapshot_string_bytes = previous_snapshot_string_bytes;
            data->metadata_probe_string_bytes = previous_probe_string_bytes;
            data->journal_string_bytes = previous_journal_string_bytes;
            data->total_string_bytes = previous_string_bytes;
            data->info.estimated_resident_bytes = previous_estimate;
            data->info.accounted_string_bytes = previous_accounted_string_bytes;
            return group_string_code;
        }
        const int probe_string_code = account_metadata_probe_strings(data, cancellation, status);
        if (probe_string_code != 0)
        {
            data->snapshot = std::move(previous_snapshot);
            data->handles = std::move(previous_handles);
            data->snapshot_string_bytes = previous_snapshot_string_bytes;
            data->metadata_probe_string_bytes = previous_probe_string_bytes;
            data->journal_string_bytes = previous_journal_string_bytes;
            data->total_string_bytes = previous_string_bytes;
            data->info.estimated_resident_bytes = previous_estimate;
            data->info.accounted_string_bytes = previous_accounted_string_bytes;
            return probe_string_code;
        }
        const int journal_string_code = account_edit_journal_strings(data, cancellation, status);
        if (journal_string_code != 0)
        {
            data->snapshot = std::move(previous_snapshot);
            data->handles = std::move(previous_handles);
            data->snapshot_string_bytes = previous_snapshot_string_bytes;
            data->metadata_probe_string_bytes = previous_probe_string_bytes;
            data->journal_string_bytes = previous_journal_string_bytes;
            data->total_string_bytes = previous_string_bytes;
            data->info.estimated_resident_bytes = previous_estimate;
            data->info.accounted_string_bytes = previous_accounted_string_bytes;
            return journal_string_code;
        }
        data->info.edit_journal_revision = static_cast<std::uint64_t>(data->edit_journal.size());
        data->info.accounted_string_bytes = data->total_string_bytes;
        data->info.estimated_resident_bytes = estimated_resident_bytes(*data);
        if (data->info.estimated_resident_bytes > data->limits.max_session_estimated_bytes)
        {
            data->snapshot = std::move(previous_snapshot);
            data->handles = std::move(previous_handles);
            data->snapshot_string_bytes = previous_snapshot_string_bytes;
            data->metadata_probe_string_bytes = previous_probe_string_bytes;
            data->journal_string_bytes = previous_journal_string_bytes;
            data->total_string_bytes = previous_string_bytes;
            data->info.estimated_resident_bytes = previous_estimate;
            data->info.accounted_string_bytes = previous_accounted_string_bytes;
            set_status(status, kResourceLimit,
                       "STEP topology session exceeds the resident-byte estimate.");
            return kResourceLimit;
        }
        data->snapshot.session = data->info;
        set_status(status, 0, "");
        return 0;
    }
    catch (const Standard_Failure& failure)
    {
        data->snapshot = std::move(previous_snapshot);
        data->handles = std::move(previous_handles);
        data->snapshot_string_bytes = previous_snapshot_string_bytes;
        data->metadata_probe_string_bytes = previous_probe_string_bytes;
        data->journal_string_bytes = previous_journal_string_bytes;
        data->total_string_bytes = previous_string_bytes;
        data->info.estimated_resident_bytes = previous_estimate;
        data->info.accounted_string_bytes = previous_accounted_string_bytes;
        set_status(status, kInternalFailure, failure.GetMessageString());
        return kInternalFailure;
    }
    catch (const std::exception& error)
    {
        data->snapshot = std::move(previous_snapshot);
        data->handles = std::move(previous_handles);
        data->snapshot_string_bytes = previous_snapshot_string_bytes;
        data->metadata_probe_string_bytes = previous_probe_string_bytes;
        data->journal_string_bytes = previous_journal_string_bytes;
        data->total_string_bytes = previous_string_bytes;
        data->info.estimated_resident_bytes = previous_estimate;
        data->info.accounted_string_bytes = previous_accounted_string_bytes;
        set_status(status, kInternalFailure, error.what());
        return kInternalFailure;
    }
}

} // namespace geometer::step_topology_internal

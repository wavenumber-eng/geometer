#include "geometer/operation_registry.h"

#include "analytic_filtered_operation.h"
#include "geometer/fast_hlr.h"
#include "geometer/indexed_mesh_packet.h"
#include "geometer/model_bounds.h"
#include "geometer/projection.h"
#include "geometer/projection_options_json.h"
#include "geometer/sha256.h"
#include "illustration_clipping.h"
#include "mesh_illustration_operation.h"
#include "model_illustration_operation.h"
#include "model_tessellation_operation.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <type_traits>
#include <utility>

namespace geometer
{
bool operation_uses_b0(const std::string& operation_id)
{
    const char* request_contract = operation_request_contract(operation_id);
    if (request_contract == nullptr)
        return false;
    return std::strcmp(request_contract, "geometry.model_illustration.request.b0") == 0 ||
           std::strcmp(request_contract, "geometry.model_illustration_geometry.request.b0") == 0 ||
           std::strcmp(request_contract, "geometry.mesh_illustration.request.b0") == 0 ||
           std::strcmp(request_contract, "geometry.mesh_illustration_geometry.request.b0") == 0 ||
           std::strcmp(request_contract, "geometry.mesh_hlr_projection.request.b0") == 0;
}

bool encode_operation_outcome(const OperationExecution::Outcome& outcome, std::string* json,
                              contracts::ContractError* error)
{
    return std::visit(
        [&](const auto& value)
        {
            using Value = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Value, contracts::OperationSuccessB0> ||
                          std::is_same_v<Value, contracts::OperationFailureB0>)
            {
                contracts::OperationOutcomeB0 wire = value;
                return contracts::encode_json(wire, json, error);
            }
            else
            {
                contracts::OperationOutcomeA0 wire = value;
                return contracts::encode_json(wire, json, error);
            }
        },
        outcome);
}

namespace
{

constexpr const char* kModelBoundsOperation = "geometry.model_bounds.a0";
constexpr const char* kModelHlrOperation = "geometry.model_hlr_projection.a0";
constexpr const char* kMeshHlrOperation = "geometry.mesh_hlr_projection.a0";
constexpr const char* kMeshHlrOperationB0 = "geometry.mesh_hlr_projection.b0";
constexpr const char* kIndexedMeshMediaType =
    "application/vnd.wavenumber.geometer.indexed-triangle-mesh";

bool valid_utf8(const std::string& value)
{
    const auto* bytes = reinterpret_cast<const unsigned char*>(value.data());
    std::size_t index = 0;
    while (index < value.size())
    {
        const unsigned char first = bytes[index++];
        if (first <= 0x7fU)
        {
            continue;
        }
        unsigned int remaining = 0;
        unsigned int code_point = 0;
        if (first >= 0xc2U && first <= 0xdfU)
        {
            remaining = 1;
            code_point = first & 0x1fU;
        }
        else if (first >= 0xe0U && first <= 0xefU)
        {
            remaining = 2;
            code_point = first & 0x0fU;
        }
        else if (first >= 0xf0U && first <= 0xf4U)
        {
            remaining = 3;
            code_point = first & 0x07U;
        }
        else
        {
            return false;
        }
        if (index + remaining > value.size())
        {
            return false;
        }
        for (unsigned int offset = 0; offset < remaining; ++offset)
        {
            const unsigned char next = bytes[index++];
            if ((next & 0xc0U) != 0x80U)
            {
                return false;
            }
            code_point = (code_point << 6U) | (next & 0x3fU);
        }
        if ((remaining == 2 && code_point < 0x800U) || (remaining == 3 && code_point < 0x10000U) ||
            code_point > 0x10ffffU || (code_point >= 0xd800U && code_point <= 0xdfffU))
        {
            return false;
        }
    }
    return true;
}

std::string json_pointer_token(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (const char character : value)
    {
        if (character == '~')
        {
            escaped += "~0";
        }
        else if (character == '/')
        {
            escaped += "~1";
        }
        else
        {
            escaped += character;
        }
    }
    return escaped;
}

contracts::DiagnosticA0 diagnostic(std::string code, contracts::DiagnosticCategory category,
                                   std::string message, const std::string& operation,
                                   std::string path = {})
{
    contracts::DiagnosticA0 value;
    value.code = std::move(code);
    value.category = category;
    value.message = std::move(message);
    value.retryable = false;
    value.operation = operation;
    if (!path.empty())
    {
        value.path = std::move(path);
    }
    return value;
}

void fail(OperationExecution* execution, const std::string& operation,
          contracts::DiagnosticA0 value)
{
    if (operation_uses_b0(operation))
    {
        contracts::OperationFailureB0 failure;
        failure.operation = operation;
        failure.diagnostics.push_back(std::move(value));
        execution->outcome = std::move(failure);
        execution->attachments.clear();
        return;
    }
    contracts::OperationFailureA0 failure;
    failure.operation = operation;
    failure.diagnostics.push_back(std::move(value));
    execution->outcome = std::move(failure);
    execution->attachments.clear();
}

void fail_b0(OperationExecution* execution, const std::string& operation,
             contracts::DiagnosticA0 value)
{
    contracts::OperationFailureB0 failure;
    failure.operation = operation;
    failure.diagnostics.push_back(std::move(value));
    execution->outcome = std::move(failure);
    execution->attachments.clear();
}

const OperationAttachmentView*
find_model_attachment(const std::string& operation,
                      const std::vector<OperationAttachmentView>& attachments,
                      OperationExecution* execution)
{
    const OperationAttachmentView* model = nullptr;
    for (const auto& attachment : attachments)
    {
        if (attachment.name != "model")
        {
            fail(execution, operation,
                 diagnostic("geometer.contract.undeclared_attachment",
                            contracts::DiagnosticCategory::contract,
                            "The operation does not declare this attachment.", operation,
                            "/attachments/" + json_pointer_token(attachment.name)));
            return nullptr;
        }
        if (model != nullptr)
        {
            fail(execution, operation,
                 diagnostic("geometer.contract.duplicate_attachment",
                            contracts::DiagnosticCategory::contract,
                            "The model attachment occurs more than once.", operation,
                            "/attachments/model"));
            return nullptr;
        }
        model = &attachment;
    }
    if (model == nullptr)
    {
        fail(execution, operation,
             diagnostic(
                 "geometer.contract.missing_attachment", contracts::DiagnosticCategory::contract,
                 "The required model attachment is missing.", operation, "/attachments/model"));
        return nullptr;
    }
    if (model->media_type != "application/step" && model->media_type != "model/step")
    {
        fail(execution, operation,
             diagnostic("geometer.contract.attachment_media_type_mismatch",
                        contracts::DiagnosticCategory::contract,
                        "The model attachment media type is not supported.", operation,
                        "/attachments/model/media_type"));
        return nullptr;
    }
    if (model->size > 268435456U)
    {
        fail(execution, operation,
             diagnostic("geometer.contract.attachment_limit_exceeded",
                        contracts::DiagnosticCategory::contract,
                        "The model attachment exceeds its operation limit.", operation,
                        "/attachments/model/data"));
        return nullptr;
    }
    return model;
}

const OperationAttachmentView*
find_hlr_attachment(const std::string& operation,
                    const std::vector<OperationAttachmentView>& attachments,
                    OperationExecution* execution)
{
    const bool mesh_operation = operation == kMeshHlrOperation;
    const std::string expected_name = mesh_operation ? "mesh" : "model";
    const OperationAttachmentView* result = nullptr;
    for (const auto& attachment : attachments)
    {
        if (attachment.name != expected_name)
        {
            fail(execution, operation,
                 diagnostic("geometer.contract.undeclared_attachment",
                            contracts::DiagnosticCategory::contract,
                            "The operation does not declare this attachment.", operation,
                            "/attachments/" + json_pointer_token(attachment.name)));
            return nullptr;
        }
        if (result != nullptr)
        {
            fail(execution, operation,
                 diagnostic("geometer.contract.duplicate_attachment",
                            contracts::DiagnosticCategory::contract,
                            "The required HLR attachment occurs more than once.", operation,
                            "/attachments/" + expected_name));
            return nullptr;
        }
        result = &attachment;
    }
    if (result == nullptr)
    {
        fail(execution, operation,
             diagnostic("geometer.contract.missing_attachment",
                        contracts::DiagnosticCategory::contract,
                        "The required HLR attachment is missing.", operation,
                        "/attachments/" + expected_name));
        return nullptr;
    }
    const bool supported_media = mesh_operation ? result->media_type == kIndexedMeshMediaType
                                                : (result->media_type == "application/step" ||
                                                   result->media_type == "model/step");
    if (!supported_media)
    {
        fail(execution, operation,
             diagnostic("geometer.contract.attachment_media_type_mismatch",
                        contracts::DiagnosticCategory::contract,
                        "The HLR attachment media type is not supported.", operation,
                        "/attachments/" + expected_name + "/media_type"));
        return nullptr;
    }
    if (result->size > 268435456U)
    {
        fail(execution, operation,
             diagnostic("geometer.contract.attachment_limit_exceeded",
                        contracts::DiagnosticCategory::contract,
                        "The HLR attachment exceeds its operation limit.", operation,
                        "/attachments/" + expected_name + "/data"));
        return nullptr;
    }
    return result;
}

void include_point(contracts::ProjectionBounds* bounds, bool* valid, double x, double y)
{
    if (!*valid)
    {
        bounds->min_x = bounds->max_x = x;
        bounds->min_y = bounds->max_y = y;
        *valid = true;
        return;
    }
    bounds->min_x = std::min(bounds->min_x, x);
    bounds->min_y = std::min(bounds->min_y, y);
    bounds->max_x = std::max(bounds->max_x, x);
    bounds->max_y = std::max(bounds->max_y, y);
}

contracts::ProjectedGeometry contract_geometry(const ProjectedModeGeometry& focused)
{
    contracts::ProjectedGeometry result;
    result.segments.reserve(focused.segments.size());
    contracts::ProjectionBounds bounds;
    bool bounds_valid = false;
    for (const ProjectedSegment& segment : focused.segments)
    {
        result.segments.push_back({segment.x1, segment.y1, segment.x2, segment.y2});
        include_point(&bounds, &bounds_valid, segment.x1, segment.y1);
        include_point(&bounds, &bounds_valid, segment.x2, segment.y2);
    }
    result.arcs.reserve(focused.arcs.size());
    for (const ProjectedArc& arc : focused.arcs)
    {
        contracts::ProjectedArc converted;
        converted.start.assign(arc.start.begin(), arc.start.end());
        converted.end.assign(arc.end.begin(), arc.end.end());
        converted.center.assign(arc.center.begin(), arc.center.end());
        converted.radius = arc.radius;
        converted.extent_rad = arc.extent_rad;
        converted.ccw = arc.ccw;
        converted.full_circle = arc.full_circle;
        result.arcs.push_back(std::move(converted));
        if (arc.full_circle)
        {
            include_point(&bounds, &bounds_valid, arc.center[0] - arc.radius,
                          arc.center[1] - arc.radius);
            include_point(&bounds, &bounds_valid, arc.center[0] + arc.radius,
                          arc.center[1] + arc.radius);
        }
        else
        {
            include_point(&bounds, &bounds_valid, arc.start[0], arc.start[1]);
            include_point(&bounds, &bounds_valid, arc.end[0], arc.end[1]);
        }
    }
    if (bounds_valid)
    {
        bounds.width = bounds.max_x - bounds.min_x;
        bounds.height = bounds.max_y - bounds.min_y;
        result.bounds = bounds;
    }
    return result;
}

contracts::HlrProjectionResultA0 contract_hlr_result(const HlrProjectionResult& focused,
                                                     contracts::HlrSourceKind source_kind,
                                                     std::string source_hash)
{
    contracts::HlrProjectionResultA0 result;
    result.source.kind = source_kind;
    result.source.hash = std::move(source_hash);
    result.views.reserve(focused.views.size());
    for (const ProjectedViewGeometry& view : focused.views)
    {
        contracts::HlrProjectedView converted;
        converted.id = view.view.id;
        converted.direction.assign(view.view.direction.begin(), view.view.direction.end());
        converted.up.assign(view.view.up.begin(), view.view.up.end());
        converted.modes.outline = contract_geometry(view.outline);
        converted.modes.detail = contract_geometry(view.detail);
        converted.modes.bbox = contract_geometry(view.bbox);
        result.views.push_back(std::move(converted));
    }
    result.timings.step_read_ms = focused.timings.step_read_ms;
    result.timings.mesh_ms = focused.timings.mesh_ms;
    result.timings.hlr_ms = focused.timings.hlr_ms;
    result.timings.extract_ms = focused.timings.extract_ms;
    return result;
}

std::optional<std::vector<double>>
column_major_transform(const std::optional<contracts::HlrMatrix4x4>& row_major)
{
    if (!row_major)
        return {};
    if (row_major->size() != 16)
        return *row_major;
    std::vector<double> result(16);
    for (std::size_t row = 0; row < 4; ++row)
        for (std::size_t column = 0; column < 4; ++column)
            result[column * 4 + row] = (*row_major)[row * 4 + column];
    return result;
}

FastHlrOptions fast_hlr_options(const std::optional<contracts::FastHlrOptionsA0>& patch)
{
    FastHlrOptions result;
    if (!patch)
        return result;
    result.include_boundaries = patch->include_boundaries.value_or(result.include_boundaries);
    result.include_creases = patch->include_creases.value_or(result.include_creases);
    result.include_silhouettes = patch->include_silhouettes.value_or(result.include_silhouettes);
    result.include_hidden = patch->include_hidden.value_or(result.include_hidden);
    result.suppress_coplanar_seams =
        patch->suppress_coplanar_seams.value_or(result.suppress_coplanar_seams);
    result.crease_angle_rad = patch->crease_angle_rad.value_or(result.crease_angle_rad);
    result.weld_tolerance = patch->weld_tolerance.value_or(result.weld_tolerance);
    result.projected_tolerance = patch->projected_tolerance.value_or(result.projected_tolerance);
    result.depth_tolerance = patch->depth_tolerance.value_or(result.depth_tolerance);
    result.coplanar_seam_angle_rad =
        patch->coplanar_seam_angle_rad.value_or(result.coplanar_seam_angle_rad);
    result.coplanar_seam_depth_tolerance =
        patch->coplanar_seam_depth_tolerance.value_or(result.coplanar_seam_depth_tolerance);
    result.coplanar_seam_lateral_tolerance =
        patch->coplanar_seam_lateral_tolerance.value_or(result.coplanar_seam_lateral_tolerance);
    if (patch->limits)
    {
        const auto& limits = *patch->limits;
        result.limits.max_vertices = limits.max_vertices.value_or(result.limits.max_vertices);
        result.limits.max_triangles = limits.max_triangles.value_or(result.limits.max_triangles);
        result.limits.max_edges = limits.max_edges.value_or(result.limits.max_edges);
        result.limits.max_grid_references =
            limits.max_grid_references.value_or(result.limits.max_grid_references);
        result.limits.max_candidate_pairs =
            limits.max_candidate_pairs.value_or(result.limits.max_candidate_pairs);
        result.limits.max_fragments = limits.max_fragments.value_or(result.limits.max_fragments);
        result.limits.max_output_segments =
            limits.max_output_segments.value_or(result.limits.max_output_segments);
    }
    return result;
}

HlrProjectionOptions hlr_options(const contracts::MeshHlrProjectionRequestB0& request)
{
    HlrProjectionOptions result;
    if (request.views)
        for (const auto& view : *request.views)
            result.views.push_back({view.id,
                                    {view.direction[0], view.direction[1], view.direction[2]},
                                    {view.up[0], view.up[1], view.up[2]}});
    result.output_outline = request.output_outline.value_or(true);
    result.output_detail = request.output_detail.value_or(true);
    result.output_bbox = request.output_bbox.value_or(true);
    result.round_digits = static_cast<int>(request.round_digits.value_or(3));
    result.projection_algorithm = ProjectionAlgorithm::Fast;
    result.outline_algorithm = ProjectionOutlineAlgorithm::FastMeshShadow;
    result.curve_mode = ProjectionCurveMode::Polyline;
    result.fast = fast_hlr_options(request.fast);
    return result;
}

bool append_fast_hlr_mesh(const contracts::MeshIllustrationMesh& source, std::uint32_t face,
                          FastHlrIndexedMesh* target)
{
    const std::size_t first_vertex = target->vertices.size();
    if (source.positions.size() % 3 != 0 ||
        first_vertex + source.positions.size() / 3 > std::numeric_limits<std::uint32_t>::max())
        return false;
    for (std::size_t index = 0; index < source.positions.size(); index += 3)
        target->vertices.push_back(
            {source.positions[index], source.positions[index + 1], source.positions[index + 2]});
    const std::size_t elements =
        source.indices ? source.indices->size() : source.positions.size() / 3;
    if (elements % 3 != 0)
        return false;
    for (std::size_t element = 0; element < elements; element += 3)
    {
        FastHlrIndexedTriangle triangle;
        triangle.source_face = face;
        for (std::size_t corner = 0; corner < 3; ++corner)
        {
            const std::size_t local =
                source.indices ? (*source.indices)[element + corner] : element + corner;
            if (local >= source.positions.size() / 3)
                return false;
            triangle.vertices[corner] = static_cast<std::uint32_t>(first_vertex + local);
        }
        target->triangles.push_back(triangle);
    }
    return true;
}

contracts::HlrProjectedView empty_hlr_view(const ProjectionViewSpec& view)
{
    contracts::HlrProjectedView result;
    result.id = view.id;
    result.direction.assign(view.direction.begin(), view.direction.end());
    result.up.assign(view.up.begin(), view.up.end());
    return result;
}

void execute_mesh_hlr_b0(const unsigned char* request_json, std::size_t request_json_size,
                         const std::vector<OperationAttachmentView>& attachments,
                         OperationExecution* execution)
{
    const std::string operation = kMeshHlrOperationB0;
    const auto report = [&](std::string code, contracts::DiagnosticCategory category,
                            std::string message, std::string path = {})
    {
        fail_b0(
            execution, operation,
            diagnostic(std::move(code), category, std::move(message), operation, std::move(path)));
    };
    contracts::MeshHlrProjectionRequestB0 request;
    contracts::ContractError error;
    if (!contracts::decode_json(request_json, request_json_size, &request, &error))
    {
        report(error.code, contracts::DiagnosticCategory::contract, error.message, error.path);
        return;
    }
    const std::size_t limit = operation_input_attachment_max_bytes(operation, "mesh_collection");
    if (attachments.size() != 1 || attachments[0].name != "mesh_collection" ||
        attachments[0].media_type != "application/vnd.wavenumber.geometer.mesh-collection+json" ||
        attachments[0].size > limit)
    {
        report("geometer.contract.invalid_attachment", contracts::DiagnosticCategory::contract,
               "Expected one bounded mesh_collection JSON attachment.",
               "/attachments/mesh_collection");
        return;
    }
    const auto& attachment = attachments[0];
    contracts::MeshCollectionA0 collection;
    if (!contracts::decode_json(attachment.data, attachment.size, &collection, &error, limit))
    {
        report(error.code, contracts::DiagnosticCategory::contract, error.message, error.path);
        return;
    }
    const std::string raw_sha = sha256_hex(attachment.data, attachment.size);
    PreparedIllustrationFragment fragment;
    Status status;
    const int clip_code = prepare_illustration_fragment(
        std::move(collection.meshes), request.clipping,
        column_major_transform(request.model_transform), raw_sha, &fragment, &status);
    if (clip_code != 0)
    {
        report(clip_code == 102 ? "geometer.operation.resource_limit_exceeded"
                                : "geometer.contract.invalid_clipping",
               clip_code == 102 ? contracts::DiagnosticCategory::operation
                                : contracts::DiagnosticCategory::contract,
               status.message);
        return;
    }

    const HlrProjectionOptions options = hlr_options(request);
    contracts::HlrProjectionResultB0 result;
    result.empty = fragment.empty;
    result.source.hash = raw_sha;
    result.fragment = fragment.metadata;
    if (fragment.empty)
    {
        const std::vector<ProjectionViewSpec> views =
            options.views.empty()
                ? std::vector<ProjectionViewSpec>{{"top", {0, 0, 1}, {0, 1, 0}},
                                                  {"bottom", {0, 0, -1}, {0, 1, 0}}}
                : options.views;
        for (const auto& view : views)
            result.views.push_back(empty_hlr_view(view));
    }
    else
    {
        FastHlrIndexedMesh indexed;
        std::uint32_t face = 0;
        for (const auto& mesh : fragment.collection.meshes)
            if (!append_fast_hlr_mesh(mesh, face++, &indexed))
            {
                report("geometer.contract.invalid_mesh_collection",
                       contracts::DiagnosticCategory::contract,
                       "The prepared mesh collection cannot be converted to Fast HLR geometry.");
                return;
            }
        HlrProjectionResult focused;
        const int code = mesh_hlr_projection(indexed, options, &focused, &status);
        if (code != 0)
        {
            report(code == 102 ? "geometer.operation.resource_limit_exceeded"
                               : "geometer.operation.hlr_projection.execution_failed",
                   contracts::DiagnosticCategory::operation, status.message);
            return;
        }
        const auto converted =
            contract_hlr_result(focused, contracts::HlrSourceKind::indexed_mesh, raw_sha);
        result.views = converted.views;
        result.timings = converted.timings;
    }
    contracts::OperationSuccessB0 success;
    success.operation = operation;
    success.result = std::move(result);
    execution->outcome = std::move(success);
    execution->attachments.clear();
}

void execute_hlr_operation(const std::string& operation, const unsigned char* request_json,
                           std::size_t request_json_size,
                           const std::vector<OperationAttachmentView>& attachments,
                           OperationExecution* execution)
{
    contracts::HlrProjectionOptionsA0 request;
    contracts::ContractError contract_error;
    if (!contracts::decode_json(request_json, request_json_size, &request, &contract_error))
    {
        fail(execution, operation,
             diagnostic(contract_error.code, contracts::DiagnosticCategory::contract,
                        contract_error.message, operation, contract_error.path));
        return;
    }
    HlrProjectionOptions options;
    Status status;
    const std::string request_text(reinterpret_cast<const char*>(request_json), request_json_size);
    if (parse_hlr_projection_options_json(request_text.c_str(), &options, &status) != 0)
    {
        fail(execution, operation,
             diagnostic("geometer.contract.invalid_hlr_options",
                        contracts::DiagnosticCategory::contract, status.message, operation));
        return;
    }
    const bool mesh_operation = operation == kMeshHlrOperation;
    if (!request.projection_algorithm.has_value())
        options.projection_algorithm = ProjectionAlgorithm::Fast;
    if (!request.outline_algorithm.has_value())
        options.outline_algorithm = ProjectionOutlineAlgorithm::FastMeshShadow;
    const OperationAttachmentView* attachment =
        find_hlr_attachment(operation, attachments, execution);
    if (attachment == nullptr)
        return;

    HlrProjectionResult focused;
    contracts::HlrSourceKind source_kind = contracts::HlrSourceKind::step;
    int code = 0;
    if (mesh_operation)
    {
        const IndexedMeshPacketDecodeResult decoded =
            decode_indexed_mesh_packet(attachment->data, attachment->size);
        if (!decoded.value.has_value())
        {
            const bool limit = decoded.error == IndexedMeshPacketError::limit_exceeded;
            fail(execution, operation,
                 diagnostic(
                     limit ? "geometer.contract.indexed_mesh_limit_exceeded"
                           : "geometer.contract.invalid_indexed_mesh_packet",
                     contracts::DiagnosticCategory::contract,
                     limit ? "The indexed-mesh packet exceeds its governed limits."
                           : "The indexed-mesh packet is malformed or contains invalid geometry.",
                     operation, "/attachments/mesh/data"));
            return;
        }
        source_kind = contracts::HlrSourceKind::indexed_mesh;
        code = mesh_hlr_projection(*decoded.value, options, &focused, &status);
    }
    else
    {
        code = step_hlr_projection_from_bytes(attachment->data, attachment->size, options, &focused,
                                              &status);
    }
    if (code != 0)
    {
        fail(execution, operation,
             diagnostic("geometer.operation.hlr_projection.execution_failed",
                        contracts::DiagnosticCategory::operation, status.message, operation));
        return;
    }

    contracts::OperationSuccessA0 success;
    success.operation = operation;
    success.result = contract_hlr_result(
        focused, source_kind,
        sha256_hex(reinterpret_cast<const std::uint8_t*>(attachment->data), attachment->size));
    execution->outcome = std::move(success);
    execution->attachments.clear();
}

} // namespace

void execute_operation(const std::string& operation_id, const unsigned char* request_json,
                       std::size_t request_json_size,
                       const std::vector<OperationAttachmentView>& attachments,
                       OperationExecution* execution)
{
    for (const auto& attachment : attachments)
    {
        if (!valid_utf8(attachment.name) || !valid_utf8(attachment.media_type))
        {
            fail(execution, operation_id,
                 diagnostic("geometer.contract.invalid_attachment_encoding",
                            contracts::DiagnosticCategory::contract,
                            "Attachment names and media types must be valid UTF-8.", operation_id,
                            "/attachments"));
            return;
        }
    }
    if (operation_id == "geometry.model_tessellation.a0")
    {
        execute_model_tessellation(request_json, request_json_size, attachments, execution);
        return;
    }
    if (operation_id == "geometry.model_illustration.a0" ||
        operation_id == "geometry.model_illustration_geometry.a0" ||
        operation_id == "geometry.model_illustration.b0" ||
        operation_id == "geometry.model_illustration_geometry.b0")
    {
        execute_model_illustration(operation_id, request_json, request_json_size, attachments,
                                   execution);
        return;
    }
    if (operation_id == "geometry.mesh_illustration.a0" ||
        operation_id == "geometry.mesh_illustration_geometry.a0" ||
        operation_id == "geometry.mesh_illustration.b0" ||
        operation_id == "geometry.mesh_illustration_geometry.b0")
    {
        execute_mesh_illustration(operation_id, request_json, request_json_size, attachments,
                                  execution);
        return;
    }
    if (operation_id == analytic_operation_detail::kOperationId)
    {
        analytic_operation_detail::execute(request_json, request_json_size, attachments, execution);
        return;
    }
    if (operation_id == kMeshHlrOperationB0)
    {
        execute_mesh_hlr_b0(request_json, request_json_size, attachments, execution);
        return;
    }
    if (operation_id == kModelHlrOperation || operation_id == kMeshHlrOperation)
    {
        execute_hlr_operation(operation_id, request_json, request_json_size, attachments,
                              execution);
        return;
    }
    if (operation_id != kModelBoundsOperation)
    {
        fail(execution, operation_id,
             diagnostic("geometer.contract.unsupported_operation",
                        contracts::DiagnosticCategory::contract,
                        "The operation identity is not present in this catalog.", operation_id,
                        "/operation"));
        return;
    }

    contracts::ModelBoundsOptionsA0 request;
    contracts::ContractError contract_error;
    if (!contracts::decode_json(request_json, request_json_size, &request, &contract_error))
    {
        fail(execution, operation_id,
             diagnostic(contract_error.code, contracts::DiagnosticCategory::contract,
                        contract_error.message, operation_id, contract_error.path));
        return;
    }

    const OperationAttachmentView* model =
        find_model_attachment(operation_id, attachments, execution);
    if (model == nullptr)
    {
        return;
    }

    ModelBoundsOptions options;
    if (request.format.has_value())
    {
        options.format = ModelFormat::Step;
    }
    if (request.model_transform.has_value())
    {
        if (request.model_transform->size() != options.model_transform.size())
        {
            fail(execution, operation_id,
                 diagnostic("geometer.contract.array_size", contracts::DiagnosticCategory::contract,
                            "model_transform must contain exactly 16 numbers.", operation_id,
                            "/model_transform"));
            return;
        }
        std::copy(request.model_transform->begin(), request.model_transform->end(),
                  options.model_transform.begin());
    }

    ModelBoundsResult focused;
    Status status;
    if (model_bounds_from_bytes(model->data, model->size, options, &focused, &status) != 0)
    {
        fail(execution, operation_id,
             diagnostic("geometer.operation.model_bounds.execution_failed",
                        contracts::DiagnosticCategory::operation, status.message, operation_id));
        return;
    }

    contracts::ModelBoundsResultA0 result;
    result.source.format = contracts::ModelFormat::step;
    result.source.hash = std::move(focused.source_hash);
    result.bounds.min.assign(focused.min.begin(), focused.min.end());
    result.bounds.max.assign(focused.max.begin(), focused.max.end());
    result.bounds.size.assign(focused.size.begin(), focused.size.end());
    result.bounds.center.assign(focused.center.begin(), focused.center.end());
    result.timings.model_read_ms = focused.timings.model_read_ms;
    result.timings.bounds_ms = focused.timings.bounds_ms;

    contracts::OperationSuccessA0 success;
    success.operation = operation_id;
    success.result = std::move(result);
    execution->outcome = std::move(success);
    execution->attachments.clear();
}

} // namespace geometer

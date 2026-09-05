#include "geometer/operation_registry.h"

#include "analytic_filtered_operation.h"
#include "geometer/indexed_mesh_packet.h"
#include "geometer/model_bounds.h"
#include "geometer/projection.h"
#include "geometer/projection_options_json.h"
#include "geometer/sha256.h"
#include "model_tessellation_operation.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <utility>

namespace geometer
{
namespace
{

constexpr const char* kModelBoundsOperation = "geometry.model_bounds.a0";
constexpr const char* kModelHlrOperation = "geometry.model_hlr_projection.a0";
constexpr const char* kMeshHlrOperation = "geometry.mesh_hlr_projection.a0";
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
    contracts::OperationFailureA0 failure;
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
    if (mesh_operation)
    {
        if (!request.projection_algorithm.has_value())
            options.projection_algorithm = ProjectionAlgorithm::Fast;
        if (!request.outline_algorithm.has_value())
            options.outline_algorithm = ProjectionOutlineAlgorithm::FastMeshShadow;
    }
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
    if (operation_id == analytic_operation_detail::kOperationId)
    {
        analytic_operation_detail::execute(request_json, request_json_size, attachments, execution);
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

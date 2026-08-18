#include "analytic_filtered_operation.h"

#include "geometer/analytic_filtered_batch.h"

#include <rapidjson/document.h>

#include <string>
#include <utility>

namespace geometer::analytic_operation_detail
{
namespace
{

struct PackedMetadata
{
    const char* request_schema = nullptr;
    const char* request_attachment = nullptr;
    const char* request_format = nullptr;
    std::size_t request_max_bytes = 0;
    const char* result_schema = nullptr;
    const char* result_attachment = nullptr;
    const char* result_format = nullptr;
    const char* result_media_type = nullptr;
    std::size_t result_max_bytes = 0;
};

contracts::DiagnosticA0 diagnostic(std::string code, contracts::DiagnosticCategory category,
                                   std::string message, std::string path = {})
{
    contracts::DiagnosticA0 value;
    value.code = std::move(code);
    value.category = category;
    value.message = std::move(message);
    value.retryable = false;
    value.operation = kOperationId;
    if (!path.empty())
        value.path = std::move(path);
    return value;
}

void fail(OperationExecution* execution, contracts::DiagnosticA0 value)
{
    contracts::OperationFailureA0 failure;
    failure.operation = kOperationId;
    failure.diagnostics.push_back(std::move(value));
    execution->outcome = std::move(failure);
    execution->attachments.clear();
}

bool load_metadata(PackedMetadata* metadata, OperationExecution* execution)
{
    metadata->request_schema = operation_request_contract(kOperationId);
    metadata->result_schema = operation_result_contract(kOperationId);
    if (metadata->request_schema == nullptr || metadata->result_schema == nullptr ||
        !operation_request_projection(kOperationId, &metadata->request_attachment,
                                      &metadata->request_format) ||
        !operation_result_projection(kOperationId, &metadata->result_attachment,
                                     &metadata->result_format))
    {
        fail(execution, diagnostic("geometer.contract.catalog_incompatible",
                                   contracts::DiagnosticCategory::contract,
                                   "The packed operation catalog metadata is incomplete."));
        return false;
    }
    metadata->request_max_bytes =
        operation_input_attachment_max_bytes(kOperationId, metadata->request_attachment);
    metadata->result_max_bytes =
        operation_output_attachment_max_bytes(kOperationId, metadata->result_attachment);
    metadata->result_media_type =
        operation_output_attachment_primary_media_type(kOperationId, metadata->result_attachment);
    if (metadata->request_max_bytes == 0 || metadata->result_max_bytes == 0 ||
        metadata->result_media_type == nullptr)
    {
        fail(execution, diagnostic("geometer.contract.catalog_incompatible",
                                   contracts::DiagnosticCategory::contract,
                                   "The packed operation attachment metadata is incomplete."));
        return false;
    }
    return true;
}

bool exact_string_member(const rapidjson::Value& object, const char* name, const char* expected)
{
    const auto member = object.FindMember(name);
    return member != object.MemberEnd() && member->value.IsString() &&
           std::string(member->value.GetString(), member->value.GetStringLength()) == expected;
}

std::string attachment_path(const char* name, const char* suffix = "")
{
    std::string path = "/attachments/";
    for (const char character : std::string(name))
    {
        if (character == '~')
            path += "~0";
        else if (character == '/')
            path += "~1";
        else
            path += character;
    }
    path += suffix;
    return path;
}

bool validate_projection_json(const unsigned char* data, std::size_t size,
                              const PackedMetadata& metadata, OperationExecution* execution)
{
    if (data == nullptr || size == 0)
    {
        fail(execution,
             diagnostic("geometer.contract.invalid_json", contracts::DiagnosticCategory::contract,
                        "The packed operation request must be one strict UTF-8 JSON object."));
        return false;
    }
    rapidjson::Document document;
    document.Parse<rapidjson::kParseValidateEncodingFlag>(reinterpret_cast<const char*>(data),
                                                          size);
    if (document.HasParseError() || !document.IsObject())
    {
        fail(execution,
             diagnostic("geometer.contract.invalid_json", contracts::DiagnosticCategory::contract,
                        "The packed operation request must be one strict UTF-8 JSON object."));
        return false;
    }
    if (document.MemberCount() != 2 ||
        !exact_string_member(document, "schema", metadata.request_schema))
    {
        fail(execution,
             diagnostic("geometer.contract.literal", contracts::DiagnosticCategory::contract,
                        "The packed operation request schema is missing or incompatible.",
                        "/schema"));
        return false;
    }
    const auto packet = document.FindMember("packet");
    if (packet == document.MemberEnd() || !packet->value.IsObject() ||
        packet->value.MemberCount() != 2)
    {
        fail(execution,
             diagnostic("geometer.contract.object", contracts::DiagnosticCategory::contract,
                        "The packed operation request packet reference is invalid.", "/packet"));
        return false;
    }
    if (!exact_string_member(packet->value, "attachment", metadata.request_attachment))
    {
        fail(execution,
             diagnostic("geometer.contract.literal", contracts::DiagnosticCategory::contract,
                        "The packed request references an incompatible attachment.",
                        "/packet/attachment"));
        return false;
    }
    if (!exact_string_member(packet->value, "format", metadata.request_format))
    {
        fail(execution,
             diagnostic("geometer.contract.literal", contracts::DiagnosticCategory::contract,
                        "The packed request references an incompatible format.", "/packet/format"));
        return false;
    }
    return true;
}

const OperationAttachmentView*
find_request_attachment(const std::vector<OperationAttachmentView>& attachments,
                        const PackedMetadata& metadata, OperationExecution* execution)
{
    const OperationAttachmentView* request = nullptr;
    for (const auto& attachment : attachments)
    {
        if (attachment.name != metadata.request_attachment)
        {
            fail(execution,
                 diagnostic("geometer.contract.undeclared_attachment",
                            contracts::DiagnosticCategory::contract,
                            "The operation does not declare this attachment.", "/attachments"));
            return nullptr;
        }
        if (request != nullptr)
        {
            fail(execution, diagnostic("geometer.contract.duplicate_attachment",
                                       contracts::DiagnosticCategory::contract,
                                       "The packed request attachment occurs more than once.",
                                       attachment_path(metadata.request_attachment)));
            return nullptr;
        }
        request = &attachment;
    }
    if (request == nullptr)
    {
        fail(execution, diagnostic("geometer.contract.missing_attachment",
                                   contracts::DiagnosticCategory::contract,
                                   "The required packed request attachment is missing.",
                                   attachment_path(metadata.request_attachment)));
        return nullptr;
    }
    if (!operation_input_attachment_declared(kOperationId, request->name, request->media_type))
    {
        fail(execution, diagnostic("geometer.contract.attachment_media_type_mismatch",
                                   contracts::DiagnosticCategory::contract,
                                   "The packed request attachment media type is not supported.",
                                   attachment_path(metadata.request_attachment, "/media_type")));
        return nullptr;
    }
    if (request->size > metadata.request_max_bytes)
    {
        fail(execution, diagnostic("geometer.contract.attachment_limit_exceeded",
                                   contracts::DiagnosticCategory::contract,
                                   "The packed request attachment exceeds its operation limit.",
                                   attachment_path(metadata.request_attachment, "/data")));
        return nullptr;
    }
    return request;
}

const char* packet_error_code(AnalyticRequestPacketError error)
{
    switch (error)
    {
    case AnalyticRequestPacketError::invalid_id:
        return "geometer.contract.analytic_planar_boolean_packet.invalid_id";
    case AnalyticRequestPacketError::invalid_reference:
        return "geometer.contract.analytic_planar_boolean_packet.invalid_reference";
    case AnalyticRequestPacketError::limit_exceeded:
        return "geometer.contract.analytic_planar_boolean_packet.limit_exceeded";
    case AnalyticRequestPacketError::invalid_packet:
    case AnalyticRequestPacketError::none:
        return "geometer.contract.analytic_planar_boolean_packet.invalid_packet";
    }
    return "geometer.contract.analytic_planar_boolean_packet.invalid_packet";
}

void fail_batch(OperationExecution* execution, AnalyticFilteredBatchError error)
{
    const bool resource = error == AnalyticFilteredBatchError::resource_limit_exceeded;
    fail(execution,
         diagnostic(
             resource ? "geometer.operation.analytic_planar_boolean.resource_limit_exceeded"
                      : "geometer.operation.analytic_planar_boolean.solver_failed",
             contracts::DiagnosticCategory::operation,
             resource ? "The analytic planar Boolean batch exceeded a governed resource limit."
                      : "The analytic planar Boolean batch could not produce a governed result."));
}

} // namespace

void execute(const unsigned char* request_json, std::size_t request_json_size,
             const std::vector<OperationAttachmentView>& attachments, OperationExecution* execution)
{
    PackedMetadata metadata;
    if (!load_metadata(&metadata, execution) ||
        !validate_projection_json(request_json, request_json_size, metadata, execution))
        return;
    const OperationAttachmentView* attachment =
        find_request_attachment(attachments, metadata, execution);
    if (attachment == nullptr)
        return;

    AnalyticRequestPacketRecordsResult decoded =
        decode_analytic_request_packet(attachment->data, attachment->size);
    if (decoded.error != AnalyticRequestPacketError::none || !decoded.value)
    {
        fail(execution,
             diagnostic(packet_error_code(decoded.error), contracts::DiagnosticCategory::contract,
                        "The packed analytic planar Boolean request is invalid.",
                        attachment_path(metadata.request_attachment, "/data")));
        return;
    }

    AnalyticFilteredBatchResult batch = build_analytic_filtered_batch(*decoded.value);
    if (batch.error != AnalyticFilteredBatchError::none || !batch.packet)
    {
        fail_batch(execution, batch.error);
        return;
    }
    if (batch.packet->bytes.size() > metadata.result_max_bytes)
    {
        fail_batch(execution, AnalyticFilteredBatchError::resource_limit_exceeded);
        return;
    }

    contracts::PackedAttachmentProjectionA0 result;
    result.schema = metadata.result_schema;
    result.packet.attachment = metadata.result_attachment;
    result.packet.format = metadata.result_format;
    contracts::OperationSuccessA0 success;
    success.operation = kOperationId;
    success.result = std::move(result);
    execution->outcome = std::move(success);
    execution->attachments = {
        {metadata.result_attachment, metadata.result_media_type, std::move(batch.packet->bytes)}};
}

} // namespace geometer::analytic_operation_detail

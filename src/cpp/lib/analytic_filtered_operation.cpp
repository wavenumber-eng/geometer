#include "analytic_filtered_operation.h"

#include "geometer/analytic_filtered_batch.h"

#include <rapidjson/document.h>

#include <string>
#include <utility>

namespace geometer::analytic_operation_detail
{
namespace
{

constexpr const char* kRequestAttachment = "analytic_planar_boolean_request";
constexpr const char* kResultAttachment = "analytic_planar_boolean_result";
constexpr const char* kRequestMediaType =
    "application/vnd.wavenumber.geometer.analytic-planar-boolean-request";
constexpr const char* kResultMediaType =
    "application/vnd.wavenumber.geometer.analytic-planar-boolean-result";
constexpr const char* kRequestSchema = "geometry.analytic_planar_boolean_batch.request.a0";
constexpr const char* kResultSchema = "geometry.analytic_planar_boolean_batch.result.a0";
constexpr const char* kPacketFormat = "geometry.analytic_planar_boolean.packet.a0";
constexpr std::size_t kMaximumAttachmentBytes = 268'435'456U;

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

bool exact_string_member(const rapidjson::Value& object, const char* name, const char* expected)
{
    const auto member = object.FindMember(name);
    return member != object.MemberEnd() && member->value.IsString() &&
           std::string(member->value.GetString(), member->value.GetStringLength()) == expected;
}

bool validate_projection_json(const unsigned char* data, std::size_t size,
                              OperationExecution* execution)
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
    if (document.MemberCount() != 2 || !exact_string_member(document, "schema", kRequestSchema))
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
    if (!exact_string_member(packet->value, "attachment", kRequestAttachment))
    {
        fail(execution,
             diagnostic("geometer.contract.literal", contracts::DiagnosticCategory::contract,
                        "The packed request references an incompatible attachment.",
                        "/packet/attachment"));
        return false;
    }
    if (!exact_string_member(packet->value, "format", kPacketFormat))
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
                        OperationExecution* execution)
{
    const OperationAttachmentView* request = nullptr;
    for (const auto& attachment : attachments)
    {
        if (attachment.name != kRequestAttachment)
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
                                       "/attachments/analytic_planar_boolean_request"));
            return nullptr;
        }
        request = &attachment;
    }
    if (request == nullptr)
    {
        fail(execution, diagnostic("geometer.contract.missing_attachment",
                                   contracts::DiagnosticCategory::contract,
                                   "The required packed request attachment is missing.",
                                   "/attachments/analytic_planar_boolean_request"));
        return nullptr;
    }
    if (request->media_type != kRequestMediaType)
    {
        fail(execution, diagnostic("geometer.contract.attachment_media_type_mismatch",
                                   contracts::DiagnosticCategory::contract,
                                   "The packed request attachment media type is not supported.",
                                   "/attachments/analytic_planar_boolean_request/media_type"));
        return nullptr;
    }
    if (request->size > kMaximumAttachmentBytes)
    {
        fail(execution, diagnostic("geometer.contract.attachment_limit_exceeded",
                                   contracts::DiagnosticCategory::contract,
                                   "The packed request attachment exceeds its operation limit.",
                                   "/attachments/analytic_planar_boolean_request/data"));
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
    if (!validate_projection_json(request_json, request_json_size, execution))
        return;
    const OperationAttachmentView* attachment = find_request_attachment(attachments, execution);
    if (attachment == nullptr)
        return;

    AnalyticRequestPacketRecordsResult decoded =
        decode_analytic_request_packet(attachment->data, attachment->size);
    if (decoded.error != AnalyticRequestPacketError::none || !decoded.value)
    {
        fail(execution,
             diagnostic(packet_error_code(decoded.error), contracts::DiagnosticCategory::contract,
                        "The packed analytic planar Boolean request is invalid.",
                        "/attachments/analytic_planar_boolean_request/data"));
        return;
    }

    AnalyticFilteredBatchResult batch = build_analytic_filtered_batch(*decoded.value);
    if (batch.error != AnalyticFilteredBatchError::none || !batch.packet)
    {
        fail_batch(execution, batch.error);
        return;
    }
    if (batch.packet->bytes.size() > kMaximumAttachmentBytes)
    {
        fail_batch(execution, AnalyticFilteredBatchError::resource_limit_exceeded);
        return;
    }

    contracts::PackedAttachmentProjectionA0 result;
    result.schema = kResultSchema;
    result.packet.attachment = kResultAttachment;
    result.packet.format = kPacketFormat;
    contracts::OperationSuccessA0 success;
    success.operation = kOperationId;
    success.result = std::move(result);
    execution->outcome = std::move(success);
    execution->attachments = {
        {kResultAttachment, kResultMediaType, std::move(batch.packet->bytes)}};
}

} // namespace geometer::analytic_operation_detail

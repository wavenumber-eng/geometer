#include "geometer/operation_registry.h"

#include "geometer/model_bounds.h"

#include <algorithm>
#include <array>
#include <utility>

namespace geometer
{
namespace
{

constexpr const char* kModelBoundsOperation = "geometry.model_bounds.a0";

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

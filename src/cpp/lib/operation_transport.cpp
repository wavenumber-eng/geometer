#include "geometer/operation_transport.h"

#include <rapidjson/document.h>

#include <cstddef>
#include <unordered_set>

namespace geometer
{
namespace
{

constexpr std::size_t kMaxJsonBytes = 8U * 1024U * 1024U;
constexpr std::size_t kMaxAttachmentCount = 16U;
constexpr std::size_t kMaxAttachmentTextBytes = 128U;
constexpr std::size_t kMaxAttachmentBytes = 256U * 1024U * 1024U;
#ifdef __EMSCRIPTEN__
constexpr std::size_t kMaxAggregateAttachmentBytes = 256U * 1024U * 1024U;
#else
constexpr std::size_t kMaxAggregateAttachmentBytes = 512U * 1024U * 1024U;
#endif

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

OperationResponseValidationStatus fail(OperationResponseValidationStatus status,
                                       std::string* message, const char* value)
{
    if (message != nullptr)
    {
        *message = value;
    }
    return status;
}

} // namespace

OperationResponseValidationStatus
validate_operation_response(const std::string& operation_id, const std::string& json,
                            const std::vector<OperationOutputAttachment>& attachments,
                            std::string* message)
{
    if (message != nullptr)
    {
        message->clear();
    }
    if (json.empty() || json.size() > kMaxJsonBytes)
    {
        return fail(OperationResponseValidationStatus::limit_exceeded, message,
                    "Operation response JSON is empty or exceeds 8 MiB.");
    }
    if (!valid_utf8(json))
    {
        return fail(OperationResponseValidationStatus::invalid, message,
                    "Operation response JSON is not valid UTF-8.");
    }
    if (attachments.size() > kMaxAttachmentCount)
    {
        return fail(OperationResponseValidationStatus::limit_exceeded, message,
                    "Operation response has more than 16 attachments.");
    }

    std::size_t aggregate_size = 0;
    std::unordered_set<std::string> names;
    for (const auto& attachment : attachments)
    {
        if (attachment.name.empty() || attachment.media_type.empty() ||
            !valid_utf8(attachment.name) || !valid_utf8(attachment.media_type))
        {
            return fail(OperationResponseValidationStatus::invalid, message,
                        "An output attachment has invalid name or media-type text.");
        }
        if (attachment.name.size() > kMaxAttachmentTextBytes ||
            attachment.media_type.size() > kMaxAttachmentTextBytes ||
            attachment.data.size() > kMaxAttachmentBytes ||
            aggregate_size > kMaxAggregateAttachmentBytes - attachment.data.size())
        {
            return fail(OperationResponseValidationStatus::limit_exceeded, message,
                        "An output attachment exceeds a generic ABI response limit.");
        }
        aggregate_size += attachment.data.size();
        if (!names.insert(attachment.name).second)
        {
            return fail(OperationResponseValidationStatus::invalid, message,
                        "An output attachment name occurs more than once.");
        }
        if (!operation_output_attachment_declared(operation_id, attachment.name,
                                                  attachment.media_type))
        {
            return fail(OperationResponseValidationStatus::invalid, message,
                        "An output attachment is not declared by the operation catalog.");
        }
    }
    rapidjson::Document outcome;
    outcome.Parse<rapidjson::kParseValidateEncodingFlag>(json.data(), json.size());
    if (outcome.IsObject())
    {
        const auto ok = outcome.FindMember("ok");
        if (ok != outcome.MemberEnd() && ok->value.IsBool() && ok->value.GetBool())
        {
            const std::size_t required = operation_required_output_attachment_count(operation_id);
            for (std::size_t index = 0; index < required; ++index)
            {
                const char* name = operation_required_output_attachment_name(operation_id, index);
                if (name == nullptr || names.find(name) == names.end())
                {
                    return fail(
                        OperationResponseValidationStatus::invalid, message,
                        "A successful operation response is missing a required attachment.");
                }
            }
        }
    }
    return OperationResponseValidationStatus::ok;
}

} // namespace geometer

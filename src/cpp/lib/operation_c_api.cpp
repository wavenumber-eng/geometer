#include "geometer/c_api.h"

#include "geometer/generated/contracts/contracts.h"
#include "geometer/operation_registry.h"

#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <utility>
#include <vector>

struct GeometerOperationResult
{
    std::string json;
    std::vector<geometer::OperationOutputAttachment> attachments;
};

namespace
{

constexpr uint32_t kMaxOperationIdBytes = 128U;
constexpr uint32_t kMaxJsonBytes = 8U * 1024U * 1024U;
constexpr uint32_t kMaxAttachmentCount = 16U;
constexpr uint32_t kMaxAttachmentTextBytes = 128U;
constexpr uint32_t kMaxAttachmentBytes = 256U * 1024U * 1024U;
#ifdef __EMSCRIPTEN__
constexpr std::size_t kMaxAggregateAttachmentBytes = 256U * 1024U * 1024U;
#else
constexpr std::size_t kMaxAggregateAttachmentBytes = 512U * 1024U * 1024U;
#endif

void initialize_outputs(GeometerOperationResult** result, char** error)
{
    if (result != nullptr)
    {
        *result = nullptr;
    }
    if (error != nullptr)
    {
        *error = nullptr;
    }
}

void assign_error(char** error, const char* message) noexcept
{
    if (error == nullptr || message == nullptr)
    {
        return;
    }
    const std::size_t size = std::strlen(message);
    auto* copied = static_cast<char*>(std::malloc(size + 1U));
    if (copied == nullptr)
    {
        return;
    }
    std::memcpy(copied, message, size + 1U);
    *error = copied;
}

int local_failure(int code, char** error, const char* message) noexcept
{
    assign_error(error, message);
    return code;
}

bool valid_operation_id(const char* value, uint32_t size)
{
    if (value == nullptr || size == 0U || size > kMaxOperationIdBytes)
    {
        return false;
    }
    bool previous_dot = true;
    for (uint32_t index = 0; index < size; ++index)
    {
        const unsigned char character = static_cast<unsigned char>(value[index]);
        if (character == '.')
        {
            if (previous_dot)
            {
                return false;
            }
            previous_dot = true;
        }
        else if ((character >= 'a' && character <= 'z') || (character >= '0' && character <= '9') ||
                 character == '_')
        {
            previous_dot = false;
        }
        else
        {
            return false;
        }
    }
    return !previous_dot;
}

bool pointer_matches_size(const void* pointer, uint32_t size)
{
    return (pointer != nullptr) == (size != 0U);
}

} // namespace

extern "C" int geometer_operation_catalog_json(char** value, char** error)
{
    if (value == nullptr ||
        (error != nullptr && reinterpret_cast<void*>(value) == reinterpret_cast<void*>(error)))
    {
        if (error != nullptr)
        {
            *error = nullptr;
        }
        return local_failure(GEOMETER_OPERATION_ABI_INVALID_ARGUMENT, error,
                             "Invalid or aliased output holder.");
    }
    *value = nullptr;
    if (error != nullptr)
    {
        *error = nullptr;
    }
    try
    {
        const char* catalog = geometer::operation_catalog_json();
        const std::size_t size = std::strlen(catalog);
        auto* copied = static_cast<char*>(std::malloc(size + 1U));
        if (copied == nullptr)
        {
            return local_failure(GEOMETER_OPERATION_ABI_NO_MEMORY, error,
                                 "Could not allocate the operation catalog.");
        }
        std::memcpy(copied, catalog, size + 1U);
        *value = copied;
        return GEOMETER_OPERATION_ABI_OK;
    }
    catch (...)
    {
        return local_failure(GEOMETER_OPERATION_ABI_INTERNAL, error,
                             "Unexpected operation catalog failure.");
    }
}

extern "C" int geometer_operation_execute(const char* operation_id, uint32_t operation_id_size,
                                          const unsigned char* request_json,
                                          uint32_t request_json_size,
                                          const GeometerAttachmentView* attachments,
                                          uint32_t attachment_count,
                                          GeometerOperationResult** result, char** error)
{
    if (result == nullptr ||
        (error != nullptr && reinterpret_cast<void*>(result) == reinterpret_cast<void*>(error)))
    {
        if (error != nullptr)
        {
            *error = nullptr;
        }
        return local_failure(GEOMETER_OPERATION_ABI_INVALID_ARGUMENT, error,
                             "Invalid or aliased output holder.");
    }
    initialize_outputs(result, error);
    if (operation_id_size > kMaxOperationIdBytes)
    {
        return local_failure(GEOMETER_OPERATION_ABI_LIMIT_EXCEEDED, error,
                             "The operation identifier exceeds the generic ABI limit.");
    }
    if (!valid_operation_id(operation_id, operation_id_size) ||
        !pointer_matches_size(request_json, request_json_size) ||
        !pointer_matches_size(attachments, attachment_count))
    {
        return local_failure(GEOMETER_OPERATION_ABI_INVALID_ARGUMENT, error,
                             "Invalid operation identifier or pointer/size pair.");
    }
    if (request_json_size > kMaxJsonBytes || attachment_count > kMaxAttachmentCount)
    {
        return local_failure(GEOMETER_OPERATION_ABI_LIMIT_EXCEEDED, error,
                             "The invocation exceeds a generic ABI limit.");
    }

    try
    {
        std::vector<geometer::OperationAttachmentView> views;
        views.reserve(attachment_count);
        std::size_t aggregate_size = 0;
        for (uint32_t index = 0; index < attachment_count; ++index)
        {
            const auto& attachment = attachments[index];
            if (attachment.struct_size != sizeof(GeometerAttachmentView) ||
                attachment.flags != 0U || attachment.reserved0 != 0U ||
                !pointer_matches_size(attachment.name, attachment.name_size) ||
                !pointer_matches_size(attachment.media_type, attachment.media_type_size) ||
                !pointer_matches_size(attachment.data, attachment.data_size))
            {
                return local_failure(GEOMETER_OPERATION_ABI_INVALID_ARGUMENT, error,
                                     "An attachment descriptor is malformed.");
            }
            if (attachment.name_size > kMaxAttachmentTextBytes ||
                attachment.media_type_size > kMaxAttachmentTextBytes ||
                attachment.data_size > kMaxAttachmentBytes ||
                aggregate_size > kMaxAggregateAttachmentBytes - attachment.data_size)
            {
                return local_failure(GEOMETER_OPERATION_ABI_LIMIT_EXCEEDED, error,
                                     "An attachment descriptor exceeds a generic ABI limit.");
            }
            aggregate_size += attachment.data_size;
            views.push_back({std::string(attachment.name, attachment.name_size),
                             std::string(attachment.media_type, attachment.media_type_size),
                             attachment.data, attachment.data_size});
        }

        geometer::OperationExecution execution;
        geometer::execute_operation(std::string(operation_id, operation_id_size), request_json,
                                    request_json_size, views, &execution);
        auto owned = std::make_unique<GeometerOperationResult>();
        geometer::contracts::ContractError contract_error;
        if (!geometer::contracts::encode_json(execution.outcome, &owned->json, &contract_error))
        {
            return local_failure(GEOMETER_OPERATION_ABI_INTERNAL, error,
                                 "Could not encode the governed operation outcome.");
        }
        if (owned->json.size() > std::numeric_limits<uint32_t>::max())
        {
            return local_failure(GEOMETER_OPERATION_ABI_LIMIT_EXCEEDED, error,
                                 "The operation result JSON exceeds the ABI size range.");
        }
        owned->attachments = std::move(execution.attachments);
        *result = owned.release();
        return GEOMETER_OPERATION_ABI_OK;
    }
    catch (const std::bad_alloc&)
    {
        return local_failure(GEOMETER_OPERATION_ABI_NO_MEMORY, error,
                             "Operation invocation allocation failed.");
    }
    catch (...)
    {
        return local_failure(GEOMETER_OPERATION_ABI_INTERNAL, error,
                             "Unexpected operation invocation failure.");
    }
}

extern "C" const unsigned char*
geometer_operation_result_json_data(const GeometerOperationResult* result)
{
    return result == nullptr ? nullptr
                             : reinterpret_cast<const unsigned char*>(result->json.data());
}

extern "C" uint32_t geometer_operation_result_json_size(const GeometerOperationResult* result)
{
    return result == nullptr ? 0U : static_cast<uint32_t>(result->json.size());
}

extern "C" uint32_t
geometer_operation_result_attachment_count(const GeometerOperationResult* result)
{
    return result == nullptr ? 0U : static_cast<uint32_t>(result->attachments.size());
}

extern "C" const char*
geometer_operation_result_attachment_name(const GeometerOperationResult* result, uint32_t index,
                                          uint32_t* size)
{
    if (size != nullptr)
    {
        *size = 0U;
    }
    if (result == nullptr || size == nullptr || index >= result->attachments.size())
    {
        return nullptr;
    }
    const auto& value = result->attachments[index].name;
    *size = static_cast<uint32_t>(value.size());
    return value.data();
}

extern "C" const char*
geometer_operation_result_attachment_media_type(const GeometerOperationResult* result,
                                                uint32_t index, uint32_t* size)
{
    if (size != nullptr)
    {
        *size = 0U;
    }
    if (result == nullptr || size == nullptr || index >= result->attachments.size())
    {
        return nullptr;
    }
    const auto& value = result->attachments[index].media_type;
    *size = static_cast<uint32_t>(value.size());
    return value.data();
}

extern "C" const unsigned char*
geometer_operation_result_attachment_data(const GeometerOperationResult* result, uint32_t index,
                                          uint32_t* size)
{
    if (size != nullptr)
    {
        *size = 0U;
    }
    if (result == nullptr || size == nullptr || index >= result->attachments.size())
    {
        return nullptr;
    }
    const auto& value = result->attachments[index].data;
    *size = static_cast<uint32_t>(value.size());
    return value.empty() ? nullptr : value.data();
}

extern "C" void geometer_operation_result_free(GeometerOperationResult* result)
{
    delete result;
}

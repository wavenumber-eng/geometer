#pragma once

#include "geometer/operation_registry.h"

#include <string>

namespace geometer
{

enum class OperationResponseValidationStatus
{
    ok,
    limit_exceeded,
    invalid,
};

OperationResponseValidationStatus
validate_operation_response(const std::string& operation_id, const std::string& json,
                            const std::vector<OperationOutputAttachment>& attachments,
                            std::string* message = nullptr);

} // namespace geometer

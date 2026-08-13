#pragma once

#include "geometer/generated/contracts/contracts.h"

#include <cstddef>
#include <string>
#include <vector>

namespace geometer
{

struct OperationAttachmentView
{
    std::string name;
    std::string media_type;
    const unsigned char* data = nullptr;
    std::size_t size = 0;
};

struct OperationOutputAttachment
{
    std::string name;
    std::string media_type;
    std::vector<unsigned char> data;
};

struct OperationExecution
{
    contracts::OperationOutcomeA0 outcome;
    std::vector<OperationOutputAttachment> attachments;
};

const char* operation_catalog_json();

void execute_operation(const std::string& operation_id, const unsigned char* request_json,
                       std::size_t request_json_size,
                       const std::vector<OperationAttachmentView>& attachments,
                       OperationExecution* execution);

} // namespace geometer

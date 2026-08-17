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
const char* normalized_contract_catalog_sha256();
bool operation_output_attachment_declared(const std::string& operation_id,
                                          const std::string& attachment_name,
                                          const std::string& media_type);
bool operation_input_attachment_declared(const std::string& operation_id,
                                         const std::string& attachment_name,
                                         const std::string& media_type);
std::size_t operation_input_attachment_max_bytes(const std::string& operation_id,
                                                 const std::string& attachment_name);
const char* operation_input_attachment_primary_media_type(const std::string& operation_id,
                                                          const std::string& attachment_name);
std::size_t operation_output_attachment_max_bytes(const std::string& operation_id,
                                                  const std::string& attachment_name);
const char* operation_output_attachment_primary_media_type(const std::string& operation_id,
                                                           const std::string& attachment_name);
const char* operation_request_contract(const std::string& operation_id);
bool operation_request_projection(const std::string& operation_id, const char** attachment_name,
                                  const char** format);
const char* operation_result_contract(const std::string& operation_id);
bool operation_result_projection(const std::string& operation_id, const char** attachment_name,
                                 const char** format);
bool operation_logical_result_matches(const std::string& operation_id,
                                      const contracts::OperationResultValueA0& result);
std::size_t operation_required_output_attachment_count(const std::string& operation_id);
const char* operation_required_output_attachment_name(const std::string& operation_id,
                                                      std::size_t index);

void execute_operation(const std::string& operation_id, const unsigned char* request_json,
                       std::size_t request_json_size,
                       const std::vector<OperationAttachmentView>& attachments,
                       OperationExecution* execution);

} // namespace geometer

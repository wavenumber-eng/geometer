#pragma once

#include "geometer/operation_registry.h"

#include <cstddef>
#include <vector>

namespace geometer::analytic_operation_detail
{

inline constexpr const char* kOperationId = "geometry.analytic_planar_boolean_batch.a0";

void execute(const unsigned char* request_json, std::size_t request_json_size,
             const std::vector<OperationAttachmentView>& attachments,
             OperationExecution* execution);

} // namespace geometer::analytic_operation_detail

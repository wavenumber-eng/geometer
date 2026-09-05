#pragma once

#include "geometer/operation_registry.h"

namespace geometer
{
void execute_mesh_illustration(const unsigned char* request, std::size_t size,
                               const std::vector<OperationAttachmentView>& attachments,
                               OperationExecution* execution);
} // namespace geometer

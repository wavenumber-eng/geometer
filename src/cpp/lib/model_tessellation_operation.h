#pragma once
#include "geometer/operation_registry.h"

namespace geometer
{
void execute_model_tessellation(const unsigned char* request, std::size_t size,
                                const std::vector<OperationAttachmentView>& attachments,
                                OperationExecution* execution);
}

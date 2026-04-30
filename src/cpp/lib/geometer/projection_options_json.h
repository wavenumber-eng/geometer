#pragma once

#include "projection.h"
#include "status.h"

namespace geometer
{

int parse_hlr_projection_options_json(const char* json, HlrProjectionOptions* options,
                                      Status* status = nullptr);

} // namespace geometer

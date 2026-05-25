#pragma once

#include "model_bounds.h"
#include "status.h"

namespace geometer
{

int parse_model_bounds_options_json(const char* json, ModelBoundsOptions* options,
                                    Status* status = nullptr);

} // namespace geometer

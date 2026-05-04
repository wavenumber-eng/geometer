#pragma once

#include "status.h"
#include "step_to_glb.h"

namespace geometer
{

int parse_step_to_glb_options_json(const char* json, StepToGlbOptions* options,
                                   Status* status = nullptr);

} // namespace geometer

#pragma once

#include "geometer.h"

#include <string>

void ensure_projection_view(geometer::HlrProjectionOptions* options, const std::string& id);
int parse_projection_options(int argc, char* argv[], int start,
                             geometer::HlrProjectionOptions* options, std::string* view_id,
                             std::string* mode);

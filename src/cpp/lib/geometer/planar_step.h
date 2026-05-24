#pragma once

#include "status.h"

#include <cstddef>
#include <string>
#include <vector>

namespace geometer
{

struct PlanarStepResult
{
    int body_count = 0;
    int region_count = 0;
    int cutout_count = 0;
};

/// Generate a STEP file from a geometry.planar_step.request.a0 JSON request.
int planar_step_from_json(const char* request_json, const std::string& step_path,
                          PlanarStepResult* result = nullptr, Status* status = nullptr);

/// Generate STEP bytes from a geometry.planar_step.request.a0 JSON request.
int planar_step_from_json_bytes(const char* request_json, std::vector<unsigned char>* step_bytes,
                                PlanarStepResult* result = nullptr, Status* status = nullptr);

} // namespace geometer

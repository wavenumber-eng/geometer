#pragma once

#include "status.h"

#include <cstddef>
#include <string>
#include <vector>

namespace geometer
{

struct StepToGlbOptions
{
    double linear_deflection = 0.1;
    double angular_deflection = 0.5;
};

/// Convert a STEP file to GLB. Returns 0 on success.
int step_to_glb(const std::string& step_path, const std::string& glb_path,
                const StepToGlbOptions& options = {});

/// Convert STEP bytes to GLB bytes. Returns 0 on success.
int step_to_glb_from_bytes(const unsigned char* step_data, std::size_t step_size,
                           const StepToGlbOptions& options, std::vector<unsigned char>* glb_bytes,
                           Status* status = nullptr);

} // namespace geometer

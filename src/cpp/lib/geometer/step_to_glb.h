#pragma once

#include <string>

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

} // namespace geometer

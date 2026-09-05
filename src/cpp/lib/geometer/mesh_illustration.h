#pragma once

#include "geometer/generated/contracts/contracts.h"
#include "geometer/status.h"

namespace geometer
{
/// Render the governed A0 illustration input without a JavaScript/WASM runtime.
/// This direct value API shares the browser renderer's production styling policy.
/// Returns 102 on native resource limits; errors clear the result (no partial SVG).
int illustrate_mesh(const contracts::MeshIllustrationInputA0& input,
                    contracts::MeshIllustrationResultA0* result, Status* status = nullptr);
} // namespace geometer

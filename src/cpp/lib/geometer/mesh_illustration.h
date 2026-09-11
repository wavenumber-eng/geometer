#pragma once

#include "geometer/generated/contracts/contracts.h"
#include "geometer/status.h"

namespace geometer
{
/// Return owning, ordered shaded geometry in millimeters without generating SVG.
/// Reuses the SVG renderer's preparation, shading, fusion and supplied-HLR rules.
/// Array order is painter order; layers use implicit closed even-odd rings.
/// Returns 102 on resource limits; all failures clear the result.
int illustrate_mesh_geometry(const contracts::MeshIllustrationGeometryInputA0& input,
                             contracts::MeshIllustrationGeometryA0* result,
                             Status* status = nullptr);
int illustrate_mesh_geometry(const contracts::MeshIllustrationGeometryInputA0& input,
                             const contracts::HlrProjectionResultA0& hlr,
                             contracts::MeshIllustrationGeometryA0* result,
                             Status* status = nullptr);

/// Render the governed A0 illustration input without a JavaScript/WASM runtime.
/// This direct value API shares the browser renderer's production styling policy.
/// Returns 102 on native resource limits; errors clear the result (no partial SVG).
int illustrate_mesh(const contracts::MeshIllustrationInputA0& input,
                    contracts::MeshIllustrationResultA0* result, Status* status = nullptr);

/// Compose already visibility-filtered HLR from the same model/frame (millimeters).
/// Exactly one view with a matching projection basis is required. Request polyline
/// HLR: arcs are rejected, not approximated. The renderer applies mirror_x and
/// draws selected detail then outline over surfaces, as in the browser Lab.
/// Supplied linework is not re-occluded; callers must exclude hidden HLR edges.
/// Maximum 1,000,000 segments across all layers, including disabled layers.
int illustrate_mesh(const contracts::MeshIllustrationInputA0& input,
                    const contracts::HlrProjectionResultA0& hlr,
                    contracts::MeshIllustrationResultA0* result, Status* status = nullptr);
} // namespace geometer

+++
type = "requirement"
id = "geometer-req-002"
domain = "geometer"
status = "implemented"
title = "STEP Geometry Operations"
created = "2026-08-18"

[[verification_refs]]
kind = "local_file"
target = "tests/L0_cpp_foundation/STRATUM.toml"
+++

# REQ-002: STEP Geometry Operations

## Summary

Geometer converts STEP models to renderable 3D GLB bytes and projects STEP
models into backend-neutral hidden-line geometry.

## Requirements

1. Accept STEP input from filesystem paths for native CLI workflows.
2. Accept STEP input as byte buffers for C ABI, WASM, and Python workflows.
3. Convert STEP to GLB while preserving model colors and names where OCCT
   exposes them.
4. Support configurable tessellation through linear and angular deflection.
5. Project one or more orthographic HLR views, including top and bottom helper
   presets.
6. Preserve the compatibility `geometry.projection.b0` JSON writer and return
   governed operations as `geometry.hlr_projection.result.a0`; both expose
   independently composable `detail`, `outline`, and `bbox` modes.
7. Preserve line segments and circular arcs when `native_arcs` mode is
   requested.
8. Support deterministic polyline flattening with configurable samples per
   curve.
9. Support row-major 4x4 `model_transform` source-model normalization before
   projection.
10. Provide CLI output to JSON, SVG, and GLB for visual inspection and release
    validation.
11. Provide transformed source-model bounds as `geometry.model_bounds.a0` JSON.
12. Prefer generic `model_*` source-model operation names while STEP remains the
    only supported source format.
13. Keep polygonal projection as the default, exact projection explicit, and
    Fast vector HLR additive under the option and operation boundaries in
    REQ-010.

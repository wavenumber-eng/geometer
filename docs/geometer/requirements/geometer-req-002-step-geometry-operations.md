+++
type = "requirement"
id = "geometer-req-002"
domain = "geometer"
status = "draft"
title = "REQ-002: STEP Geometry Operations"
created = "2026-07-07"
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
6. Return projection geometry as generic `geometry.projection.b0` JSON with
   `detail`, `outline`, and `bbox` modes.
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

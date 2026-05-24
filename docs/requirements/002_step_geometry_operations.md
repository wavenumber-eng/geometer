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
6. Return projection geometry as generic `geometry.projection.a0` JSON with
   `detail` and `simple` modes.
7. Preserve line segments and circular arcs when `native_arcs` mode is
   requested.
8. Support deterministic polyline flattening with configurable samples per
   curve.
9. Support row-major 4x4 `model_transform` source-model normalization before
   projection.
10. Provide CLI output to JSON, SVG, and GLB for visual inspection and release
    validation.

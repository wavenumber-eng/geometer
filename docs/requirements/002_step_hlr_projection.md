# REQ-002: STEP HLR Projection Core

## Summary

Provide a reusable C++/WASM API for projecting STEP model data into backend-neutral
2D hidden-line output. The API must support browser, native CLI, and Python
tooling consumers without depending on Altium-specific concepts.

## Requirements

1. Accept STEP model input as bytes for browser/Python use.
2. Provide file-path wrappers for native CLI and local developer workflows.
3. Support one or more projection views, including top and bottom orthographic
   views by default.
4. Return geometry in a generic model that can be consumed by Canvas, SVG,
   Three.js, WebGPU, or Python tools.
5. Provide both `detail` and `simple` projection modes:
   - `detail`: projected visible and outline HLR edges.
   - `simple`: contour output equivalent in intent to the current
     Shapely-backed polygonize/union path used by altium-cruncher.
6. Preserve line segments and circular arcs when requested.
7. Support polyline flattening with configurable samples per curve.
8. Use deterministic rounding/snapping so cache keys and outputs are stable.
9. Cache repeated STEP model parses and repeated model-pose projections in
   callers where appropriate.
10. Provide development CLI output to JSON and SVG for visual comparison.
11. Avoid viewer-specific styling in the core output.
12. Keep the C++ implementation modular; projection, planar contouring,
    serialization, and CLI code must live in separate files.

## Non-Goals

1. Do not embed Altium component placement rules in geometer.
2. Do not make SVG the authoritative projection format.
3. Do not make GLB the only mesh transport for browser use.
4. Do not require Python-only geometry libraries at runtime.


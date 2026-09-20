+++
type = "requirement"
id = "geometer-req-010"
domain = "geometer"
status = "implemented"
title = "Fast HLR And Illustration Interfaces"
created = "2026-09-03"

[[verification_refs]]
kind = "local_file"
target = "tests/cpp/fast_hlr_test.cpp"

[[verification_refs]]
kind = "local_file"
target = "tests/cpp/operation_contract_test.cpp"

[[verification_refs]]
kind = "local_file"
target = "tests/typescript/STRATUM.toml"

[[verification_refs]]
kind = "local_file"
target = "tests/python/test_fast_hlr_benchmark.py"

[[verification_refs]]
kind = "local_file"
target = "tests/cpp/mesh_illustration_geometry_test.cpp"

[[verification_refs]]
kind = "local_file"
target = "tests/python/test_illustration_geometry.py"

[[verification_refs]]
kind = "local_file"
target = "tests/python/test_model_illustration.py"

[[verification_refs]]
kind = "local_file"
target = "tests/cpp/illustration_clipping_test.cpp"

[[verification_refs]]
kind = "local_file"
target = "tests/cpp/illustration_b0_operation_test.cpp"
+++

# REQ-010: Fast HLR And Illustration Interfaces

## Summary

Geometer provides an additive Fast vector-HLR backend, a bounded indexed-mesh
source, and production TypeScript mesh illustration without changing existing
exact or polygonal behavior.

## Requirements

1. Use `fast` detail and `fast-mesh-shadow` as the model/STEP and indexed-mesh
   defaults, and retain all documented exact/poly and older outline options,
   aliases, output layers, CLI calls, Python calls, and focused C ABI
   compatibility functions as explicit selections.
2. Publish `geometry.model_hlr_projection.a0` and
   `geometry.mesh_hlr_projection.a0` through the generic C ABI, executable IPC,
   full browser WASM, Python, TypeScript, and Rust support lanes.
3. Use one presence-preserving `geometry.hlr_projection.options.a0` family and
   one `geometry.hlr_projection.result.a0` family for both operations.
4. Keep Fast-only candidate selection, tolerances, seam suppression, and
   resource limits nested under `fast`; do not reinterpret OCCT-specific edge
   flags.
5. Accept a bounded, validated indexed-triangle-mesh A0 packet so synthesized
   geometry can use Fast HLR without STEP or application-specific policy.
6. Provide direct C++ one-shot and prepare-once APIs. Repeated-view consumers
   must be able to reuse prepared mesh incidence rather than repeat it per view.
7. Keep `detail`, `outline`, and `bbox` independently selectable and keep
   `fast-mesh-shadow` additive to the existing outline algorithms.
8. Publish TypeScript mesh-illustration input, style, and result identities at
   A0 plus one-shot and reusable render APIs for SVG and Canvas.
9. Keep browser raster HLR outside the production A0 surface. The evaluated GPU
   prototype was removed and creates no supported package or contract.
10. Provide a convenience composition for Fast vector linework plus
    illustration without making consumers reimplement preparation, ordering,
    fusion, colorization, rendering, caching, or disposal.
11. Keep HLR and Illustration Labs as package consumers. Demo code may own UI,
    camera, upload/download, and presentation behavior, but not the production
    geometry or illustration algorithms.
12. Replay canonical HLR vectors through C++, TypeScript, Rust, and Python;
    validate native/WASM equivalence; preserve downstream compatibility
    snapshots; and keep the native/WASM benchmark reproducible.
13. Keep PCB, documentation-generator, visualizer, and other application style
    policy outside the generic Geometer implementation.
14. Provide native SVG illustration and additive illustration drawing geometry
    through shared preparation, styling, fusion and HLR composition. The geometry
    value API must avoid SVG construction and output serialization; the IPC API
    must carry bounded geometry JSON in a declared attachment. Retain owning
    ordered fills, ring holes, opacity, visible linework and millimeter coordinates.
15. Preserve existing numeric/SVG output while optimizing illustration-specific
    integer formatting. Qualify native and TypeScript geometry consumers, typed
    Python/Rust IPC helpers and full WASM dispatch; do not advertise this operation
    for the planar-only WASM target.
16. Provide a one-pass model-illustration boundary that accepts one attached
    STEP model or one bounded analytic 2.5D scene and returns SVG or existing
    renderer-neutral illustration geometry without exposing an intermediate
    mesh collection.
17. Lower analytic definitions once and place repeated occurrences with finite,
    well-conditioned affine transforms. Support planar extrusions with holes
    and circular arcs, vertical cylinders, and spheres under explicit authored,
    sampling, expanded-triangle, visibility, and drawing-command limits.
18. The combined operation must use Fast detail and Fast Mesh Shadow, reject
    hidden linework, preserve partial STEP warnings, and expose truthful phase
    timings through TypeSpec-governed generated clients.
19. B0 model/composed-mesh illustration and composed-mesh HLR must apply the
    same bounded ordered half-space clipper after all placements/transforms and
    before bounds, HLR, outlines, visibility, shading, and projection.
20. Preserve materials, interpolate boundary normals, remove degenerates
    deterministically, support only `cap_policy: "none"` initially, bind
    normalized clipping and effective limits into fragment identity, and reject
    supplied linework whose geometry digest does not match the shaded fragment.
21. Treat fully clipped fragments as successful explicit empty results with
    absent bounds. Keep A0 operation identities as strict compatibility
    adapters while maintained consumers use the complete B0 operation roots.

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
+++

# REQ-010: Fast HLR And Illustration Interfaces

## Summary

Geometer provides an additive Fast vector-HLR backend, a bounded indexed-mesh
source, production TypeScript mesh illustration, and a separately named browser
raster-HLR helper without changing existing exact or polygonal behavior.

## Requirements

1. Keep `poly` as the model/STEP default projection algorithm, use `fast` as
   the indexed-mesh default, and retain all documented exact/poly options,
   aliases, output layers, CLI calls, Python calls, and focused C ABI
   compatibility functions.
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
9. Publish browser raster HLR under a separate renderer name and document that
   it returns pixels, not deterministic vector geometry.
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

+++
type = "requirement"
id = "geometer-req-005"
domain = "geometer"
status = "implemented"
title = "WASM Browser Interfaces"
created = "2026-08-18"

[[verification_refs]]
kind = "local_file"
target = "tests/wasm/STRATUM.toml"
+++

# REQ-005: WASM Browser Interfaces

## Summary

Geometer ships browser-oriented WASM artifacts for STEP/HLR/GLB and planar-only
workflows.

## Requirements

1. Build the full browser/Web Worker target under `dist/wasm/browser/`.
2. Build the Node CLI parity target under `dist/wasm/node-test/`.
3. Build the planar-only browser target under `dist/wasm/planar-browser/`.
4. Export version, ABI, allocation/free helpers, STEP HLR, STEP-to-GLB, and
   planar byte functions from the full browser target.
5. Export version, ABI, allocation/free helpers, and planar byte functions from
   the planar-only browser target.
6. Keep user-facing browser examples under `examples/wasm/`.
7. Keep test-only validation and benchmark harnesses under `tests/wasm/`.
8. Apply REQ-009 to distributable browser-demo sites without treating demo UI
   policy as part of the core WASM geometry interface.
9. Export the generic operation C ABI required for model and indexed-mesh HLR
   and expose those operations through the generated TypeScript direct and
   Worker clients.
10. Package mesh illustration as a separately named TypeScript module with the
    output boundary defined by REQ-010. A raster-HLR module is not part of the
    production browser interface.

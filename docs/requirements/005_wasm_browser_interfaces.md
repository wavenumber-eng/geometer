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

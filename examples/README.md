# Geometer Examples

This folder contains user-facing examples that exercise Geometer outside the
test harness.

See the [dated runtime audit and disposition table](../docs/developer/demo-status.md)
for verified status and gaps across every demo. Start with
[model-bounds IPC](node/ipc-model-bounds.mjs) for persistent executable calls.

- `python/` - Python wrapper examples and demo tools.
- `cpp/` - native C++ example applications.
- `node/` - native-process TypeScript reference applications.
- `wasm/` - browser/WASM examples, the HLR Lab, reusable TypeScript demo
  tooling, and sources for self-contained hosted demos.

Test-only browser validation and benchmark harnesses live under `tests/wasm/`.
The maintained build, packaging, panel, and publication boundaries are in
[`docs/developer/browser-demos.md`](../docs/developer/browser-demos.md).

The WASM examples include a retained
[production-package Illustration Lab](wasm/README.md#step-illustration-lab).
It is available for design evaluation and regression testing, but is not a
production renderer, supported public API, or commitment that Geometer will own
the eventual illustration architecture.

# WASM Examples

Standalone browser/WASM examples will live here.

Current browser-facing HLR and GLB pages still live under `tests/wasm/` because
they double as smoke tests and benchmarks:

- `tests/wasm/embedded_model_viewer.html`
- `tests/wasm/hlr_benchmark.html`
- `tests/wasm/browser_hlr_smoke.html`

When the public browser wrapper is added, duplicate or move the polished example
pages here and keep the test-only harnesses under `tests/wasm/`.

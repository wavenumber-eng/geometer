# Distribution Artifacts

The repository policy is to commit distributable outputs in `dist/` so another
project can clone Geometer and use the CLI/WASM artifacts without rebuilding.

Persist these when publishing interface changes:

- Native CLI: `dist/native/<platform>/geometer.exe` or
  `dist/native/<platform>/geometer`.
- Native static library: `dist/native/<platform>/geometer.lib` or
  `dist/native/<platform>/libgeometer.a`.
- Full browser WASM C ABI: `dist/wasm/browser/geometer.js` and
  `dist/wasm/browser/geometer.wasm`.
- Node WASM CLI parity/test target: `dist/wasm/node-test/geometer-node-test.js`
  and `dist/wasm/node-test/geometer-node-test.wasm`.
- Planar-only browser WASM C ABI optimization:
  `dist/wasm/planar-browser/geometer-planar-browser.js` and
  `dist/wasm/planar-browser/geometer-planar-browser.wasm`.

Native platform directory names use:

- `windows-x64`
- `linux-x64`
- `macos-x64`
- `macos-arm64`

Root-level build artifacts are intentionally not produced. Source-checkout
consumers must use grouped native and WASM paths.

Do not commit local generated build state:

- `.deps/`
- `build/`
- `build-wasm/`

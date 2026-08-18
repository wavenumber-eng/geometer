# Distribution Artifacts

The repository policy is to commit distributable outputs in `dist/` so another
project can clone Geometer and use the CLI/WASM artifacts without rebuilding.

Persist these when publishing interface changes:

- Native CLI: `dist/native/<platform>/geometer.exe` or
  `dist/native/<platform>/geometer`.
- Native executable attestation:
  `dist/native/<platform>/geometer.build-attestation.json`.
- Native static library: `dist/native/<platform>/geometer.lib` or
  `dist/native/<platform>/libgeometer.a`.
- Full browser WASM C ABI: `dist/wasm/browser/geometer.js` and
  `dist/wasm/browser/geometer.wasm`.
- Node WASM CLI parity/test target: `dist/wasm/node-test/geometer-node-test.js`
  and `dist/wasm/node-test/geometer-node-test.wasm`, with a local CommonJS
  `package.json` boundary so the CLI runs beneath the repository's ESM root.
- Planar-only browser WASM C ABI optimization:
  `dist/wasm/planar-browser/geometer-planar-browser.js` and
  `dist/wasm/planar-browser/geometer-planar-browser.wasm`.
- Generated TypeScript ESM package: `dist/wasm/npm/geometer/` with explicit root,
  contracts, direct WASM, Worker-client, and Worker-host exports.

Native platform directory names use:

- `windows-x64`
- `linux-x64`
- `linux-arm64`
- `macos-arm64`

Root-level build artifacts are intentionally not produced. Source-checkout
consumers must use grouped native, npm, and WASM paths.

Do not commit local generated build state:

- `.deps/`
- `build/`
- `build-wasm/`

OCCT binary dependency archives may be stored in the Wavenumber R2-backed public
artifact cache, but they remain generated dependency state. They are restored
under `.deps/` and are not committed to `dist/`.

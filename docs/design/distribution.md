# Distribution Artifacts

The repository policy is to commit distributable outputs in `dist/` so another
project can clone Geometer and use the CLI/WASM artifacts without rebuilding.

Persist these when publishing interface changes:

- Native CLI: `dist/native/<platform>/geometer.exe` or
  `dist/native/<platform>/geometer`.
- Native executable attestation:
  `dist/native/<platform>/geometer.build-attestation.json`.
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
- Browser demos: self-contained HTML artifacts and deploy-unchanged static
  directories under `dist/wasm/demos/`. A single-HTML hosted directory has
  `index.html` as its only runtime file; `_headers` and `asset-manifest.json`
  are deployment and verification metadata.

Native platform directory names use:

- `windows-x64`
- `linux-x64`
- `linux-arm64`
- `macos-arm64`

Root-level build artifacts are intentionally not produced. Source-checkout
consumers must use grouped native, npm, and WASM paths.

Native `.lib` and `.a` files remain ordinary build/cache outputs under the
configured CMake build tree. They are not committed or included in native
runtime releases. A future native SDK must be a separately designed package
with public headers, exported CMake targets, ABI/toolchain metadata, licenses,
and a complete dependency-link strategy; a bare static archive is not an SDK.

Demo build scripts do not publish. See
[Browser demo packaging and UI](../developer/browser-demos.md) for the local build, closure,
review, and explicit publication boundary.

Do not commit local generated build state:

- `.deps/`
- `build/`
- `build-wasm/`

OCCT binary dependency archives may be stored in the Wavenumber R2-backed public
artifact cache, but they remain generated dependency state. They are restored
under `.deps/` and are not committed to `dist/`.

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
runtime archives. Each dated release separately publishes one static SDK for
Windows x64, Linux x64, Linux arm64, and macOS arm64. Those SDKs contain public
C ABI headers, exported relocatable CMake targets, Geometer and the exact OCCT
link closure, ABI/toolchain metadata, integrity/provenance records, and license
material. See [Static native SDK](static-native-sdk.md).

`Publish` is a manual workflow dispatched at the exact tag also supplied as its
input, which binds GitHub provenance to the released source revision. It
rebuilds all four native archives,
all four platform wheels, all four static SDKs, and WASM; validates a single
fail-closed inventory; and stages the unchanged bytes on a draft GitHub
Release. PyPI receives exactly the four inventoried wheels. Only after trusted
publishing succeeds is the GitHub Release made public, downloaded again, and
checked against its inventory and GitHub attestations.

Browser and native Lab build scripts do not publish demo applications. See
[Browser demo packaging and UI](../developer/browser-demos.md) for the local build, closure,
review, and explicit publication boundary.

Do not commit local generated build state:

- `.deps/`
- `build/`
- `build-wasm/`

OCCT binary dependency archives may be stored in the Wavenumber R2-backed public
artifact cache, but they remain generated dependency state. They are restored
under `.deps/` and are not committed to `dist/`.

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

`Build Release Candidate` and `Promote Release Candidate` are separate manual
workflows. Candidate production builds all four native archives, four platform
wheels, four static SDKs, and WASM once, validates one fail-closed inventory,
and retains those exact bytes as the `qualified-release` artifact of that
successful GitHub Actions run. The promotion workflow contains no compilation
or packaging tools. Given that run ID, it verifies the workflow, source,
inventory, and immutable tag, then idempotently publishes the same bytes to
PyPI and GitHub Releases.

The release inventory uses the B0 envelope. Besides exact asset names, sizes,
and SHA-256 digests, it embeds the canonical candidate root and its digest. The
candidate root contains only the exact Git commit, synchronized release
identity, and SHA-256 of `dependencies/occt-lock.json`. Draft, PyPI, and public
verification must match the candidate source revision to the checked-out
release tag. Tool versions and workflow timings remain evidence rather than
alternate cache or candidate identities.

Candidate artifacts are retained for 30 days. Promotion verifies that the run
completed successfully on `main` using `.github/workflows/release-candidate.yml`
before downloading `qualified-release`. GitHub Releases are the durable public
archive after promotion. R2 is intentionally not part of the product release
path; it stores only locked third-party dependency archives such as OCCT.

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

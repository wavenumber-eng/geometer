# OCCT Exact-Tag Qualification

Use `scripts/qualify_occt.py` for a governed side-by-side kernel decision. The
harness accepts only the reviewed exact tags `V8_0_0` and `V8_0_1`; upstream
master and arbitrary refs are rejected.

The repository production pin in `scripts/dependency_versions.py` is not
changed during qualification. Each tag uses isolated generated paths:

```text
.deps/occt-qualification/<tag>/
out/occt-qualification/<tag>/
```

The first root contains source, native build/install, and WASM build/install
state. The second contains isolated Geometer builds, isolated distribution
copies, lane-specific command logs, and qualification JSON reports. A complete
`--lane all` run writes `qualification-report.json`; partial runs write
`qualification-report-native.json` or `qualification-report-wasm.json`.
Neither root is a release artifact.

Run both exact tags from the same checkout and toolchain:

```powershell
uv run python scripts/qualify_occt.py --tag V8_0_0 --lane all
uv run python scripts/qualify_occt.py --tag V8_0_1 --lane all
```

Use `--binary-cache off` to require a source build, `--binary-cache only` to
probe a published cache without falling back, or `--prepare-only` to stop after
the dependency install. Native and WASM lanes can be run separately with
`--lane native` and `--lane wasm`.

For a complete lane, the harness:

1. verifies the source checkout is exactly at the requested tag;
2. builds/restores OCCT under the tag-specific generated root;
3. configures Geometer with an explicit `OpenCASCADE_DIR` and isolated
   `GEOMETER_DIST_ROOT`;
4. rebuilds and runs the registered native and WASM CTests with bounded test
   timeouts;
5. compares the OCCT feasibility output byte for byte across native/WASM; and
6. runs every governed exact/synthetic native-WASM parity validator;
7. runs the isolated native CLI, Python, HLR, GLB, SVG, and planar validation;
8. runs the WASM compatibility and TypeScript package/direct/Worker suites
   against the tag-specific browser artifacts;
9. executes the tag-specific HLR bundle in installed Chrome/Chromium; and
10. records browser JS/WASM digests plus repeated STEP-to-GLB heap behavior.

After both complete reports exist, compare their governed outputs:

```powershell
uv run python scripts/compare_occt_qualification.py
```

The comparison requires exact native GLB, SVG, and request bytes; exact HLR
JSON after excluding runtime timings; exact planar STEP after excluding only
the generated header timestamp; and identical repeated WASM STEP-to-GLB bytes
and heap observations. It writes `out/occt-qualification/comparison.json`.

The JSON report records exact Geometer and OCCT revisions, install sizes,
per-command wall time, log paths and digests, feasibility digests, isolated
consumer outputs, artifact sizes/digests, full-browser results, and runtime
memory observations. Keep raw logs under `out/`; commit only the reviewed
summary evidence required by the active plan.

Do not publish new dependency-cache profiles or change the production pin until
both tag reports and the existing STEP, HLR, GLB, planar, CLI, Python, browser,
package, runtime, and memory evidence receive independent review. If V8_0_1 is
rejected, retain V8_0_0 and commit the minimized rejecting fixture and decision.

## Accepted 8.0.1 Decision

The complete comparison at Geometer revision
`64d598e1d41523f5b940ccdbc7130c1b3fc94526` was independently accepted in
review packet `reviewer-01a00241-a037-752d-ad0b-820e8412a786`. The reviewed
baseline, candidate, and comparison report SHA-256 values are respectively:

- `a658e6ab6699a18d58eb175c704e08af287a1b7baa9274425d23504d6e4eef3d`
- `badd9db71e52ba0ed751e36241ae0bd52fff6e387a85ba3f49a6f9acb10671de`
- `931f1cd56a85e59c2924ba034a21339531430c6541f122c07a18a32241e92a7c`

The comparison found no governed semantic or byte difference. V8_0_1 adds
1,157,984 bytes to the Windows native OCCT install, 1,115,658 bytes to the
WASM OCCT install, 54,784 bytes to the native executable, and 23,267 bytes to
the browser WASM artifact. This evidence authorizes the exact `V8_0_1` pin and
new immutable cache profiles; it does not authorize production analytic
solver dispatch.

## Published 8.0.1 Cache Profiles

Both immutable V8_0_1 dependency-cache profiles were published from the
reviewed local install trees using credentialed producer upload:

- `occt-native-v8-0-1-windows-x64-release-static-recipe-02d3ac07fe672579`
  with archive SHA-256
  `255ad723184c62ef4e6dc82c20c1acd5b0aa43407cbefc6e26e484eb74a05df9`
- `occt-wasm-v8-0-1-wasm-emscripten-release-static-emsdk-3.1.56-recipe-a15818c33b508d24`
  with archive SHA-256
  `44fe6d6294c7a26ac77cfa17e1fd4a312578638a5669b7032c227750d032614e`

Publication was verified without credentials: both `manifest.json` objects are
served publicly under `https://artifacts.wavenumber.net/deps/v1/geometer/occt/`
with schema `wavenumber.dependency_cache_manifest.a1`, and both profiles were
restored in `only` mode into a clean isolated state root using only the public
HTTPS path. Each restored install tree reports OCCT 8.0.1 and is byte-for-byte
identical to the local install tree it was packaged from. This completes the
qualification step's cache-publication evidence; production analytic solver
dispatch remains unauthorized.

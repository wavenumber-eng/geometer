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
6. runs every governed exact/synthetic native-WASM parity validator.

The JSON report records exact Geometer and OCCT revisions, install sizes,
per-command wall time, log paths and digests, and feasibility digests. Keep raw
logs under `out/`; commit only the reviewed summary evidence required by the
active plan.

Do not publish new dependency-cache profiles or change the production pin until
both tag reports and the existing STEP, HLR, GLB, planar, CLI, Python, browser,
package, runtime, and memory evidence receive independent review. If V8_0_1 is
rejected, retain V8_0_0 and commit the minimized rejecting fixture and decision.

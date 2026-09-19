# Build Automation Strategy

Geometer does not run GitHub Actions for pushes or pull requests. In particular,
documentation changes trigger no automation. Developers run the affected
checks locally before pushing and record important validation in the pull
request. Every workflow is manual-only. `Publish` is dispatched only for an
existing reviewed release tag and is the sole path that rebuilds and publishes
the complete supported release matrix.

## Local development gate

Choose checks in proportion to the change, using the commands in the canonical
[developer guide](README.md). Before a release, run the complete local gate:

```powershell
uv sync --group dev
cmake --preset default
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
uv run --group dev rack run --all
uv run pytest tests\L99_release -q
npm run check:contracts
npm run check:docs
uvx --from wn-dev-std==2026.9.8 wn-dev-std check . --format json
```

The native build runs CTest's `production` label by default. Set
`GEOMETER_TEST_PROFILE=production` for client strata when a focused production
run should omit retained analytic-solver and STEP topology research suites.

## Manual workflows

The `Full Validation (Manual)` workflow is an explicit diagnostic fallback. It
never runs automatically. The experimental, macOS wheel, dependency-cache, and
operation-transport workflows are likewise manual-only. Dispatch one only when
its specific remote environment or controlled evidence is required. Every job
has an explicit timeout so a stalled runner or test cannot consume minutes
indefinitely.

## Experimental qualification

The `Experimental qualification` workflow runs only by manual dispatch when its
research surfaces need qualification. It owns the analytic solver, STEP topology
research, seeded synthetic qualification, and executable/WASM analytic
cross-transport parity. These tests do not run on a schedule and do not block
ordinary pull requests or releases.

Run all native research tests locally with:

```powershell
uv run python scripts/validate_native.py --include-experimental-tests
```

## Release automation

Dispatch `Publish` manually at an exact existing `vYYYY-MM-DD` tag and supply
that same tag as the workflow input. The workflow rejects any dispatch-ref,
input-tag, or checked-out-commit mismatch, then builds and packages Windows
x64, Linux x64, Linux arm64, macOS arm64, and WASM. Each native platform runs
the production C++ suite and rebuilds its platform-specific `wn-geometer`
wheel, native archive, and static SDK. Python, Rust, and TypeScript integration
runs once against Linux x64; the other platforms concentrate on native and
wheel packaging. Experimental qualification is independent of publishing.

The four platform jobs and WASM feed one exact digest inventory. The workflow
attests the inventory and all four SDK archives, uploads the exact bytes to a
draft GitHub Release, re-downloads the draft, and only then sends the four
qualified wheels to PyPI through trusted publishing. A successful PyPI publish
allows the GitHub Release to become public. A final job downloads the public
assets again, checks every size and SHA-256, and verifies GitHub attestations.
No push, pull request, documentation change, tag creation, or GitHub Release
event starts this workflow.

## Dependency caches

Every native OCCT consumer and producer uses this key family:

```text
occt-v2-native-<platform>-<compiler>-<occt-tag>-<recipe-hash>
```

Every WASM OCCT consumer and producer uses:

```text
occt-v2-wasm-linux-x64-emscripten-<occt-tag>-<emsdk-version>-<recipe-hash>
```

The platform values are the repository's canonical package tags:
`windows-x64`, `linux-x64`, `linux-arm64`, and `macos-arm64`. Cache paths always
include the source, build, and install trees. The dependency-cache workflow is
the authenticated R2 producer; ordinary workflows consume GitHub cache or the
public binary cache without R2 credentials.

Legacy restore prefixes remain temporarily below the primary `v2` prefix. On
the first run they avoid a cold OCCT compile and cause `actions/cache` to save
the restored content under the new exact key. New caches are always written
with the canonical key.

## Historical cost baseline

Before pull-request CI was retired, a production C++ pull request ran four
native matrices plus WASM, standards, and experimental cross-transport
qualification. The observed job times on issue #34 were:

| Job | Time |
| --- | ---: |
| Windows x64 native | 19m 56s |
| Linux x64 native | 13m 35s |
| Linux arm64 native | 12m 16s |
| macOS arm64 native | 11m 45s |
| WASM | 9m 57s |
| Experimental cross-transport | 34m 22s |
| Standards | 1m 24s |

The cross-transport job missed its distinct GitHub cache prefix and rebuilt
OCCT from source. The first production run with this strategy completed in
8m33s wall time: Linux native took 7m51s, WASM took 7m30s, and standards took
1m21s in parallel. Linux migrated its legacy GitHub OCCT cache to the canonical
key. WASM restored OCCT from the public binary cache and populated its canonical
GitHub caches. The measured run and cache evidence are recorded on
[issue #34](https://github.com/wavenumber-eng/geometer/issues/34). These figures
explain why pull-request automation is intentionally disabled; they are not a
current PR validation budget.

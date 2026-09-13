# CI Strategy

Geometer CI validates the code that a change can affect. Cross-platform package
production remains a release responsibility, while experimental geometry is
qualified on an explicit or scheduled workflow.

## Pull requests

The `CI` workflow first classifies the exact base-to-head pull-request diff. Its stable
`CI policy` job is the branch-protection boundary; it succeeds only when every
selected lane succeeds or is intentionally skipped.

| Change | Pull-request validation |
| --- | --- |
| Markdown, documentation assets, demo HTML/CSS | Scope and policy only |
| Python package or Python tests | Standards, production Python tests, wheel validation |
| Rust client | Standards and production Rust validation |
| TypeScript, TypeSpec, contracts | Standards and production TypeScript validation |
| C++ or native build code | Standards, Linux x64 production native/client validation, production WASM |
| WASM build/runtime code | Standards and production WASM |
| CI or release workflow | All production lanes |

Normal CI has no `push` trigger. Merging an already validated pull request does
not repeat the same build on `main`. A manual CI dispatch selects all production
lanes.

The production native lane builds explicitly promoted targets and runs CTest's
`production` label. The Python, Rust, and TypeScript strata use
`GEOMETER_TEST_PROFILE=production` to omit retained analytic-solver and STEP
topology research suites.

## Experimental qualification

The `Experimental qualification` workflow runs monthly or by manual dispatch.
It owns the analytic solver, STEP topology research, seeded synthetic
qualification, and executable/WASM analytic cross-transport parity. These tests
do not block ordinary pull requests or releases.

Run all native research tests locally with:

```powershell
uv run python scripts/validate_native.py --include-experimental-tests
```

## Releases

Publishing a GitHub release builds and packages Windows x64, Linux x64, Linux
arm64, macOS arm64, and WASM. Each native platform runs the production C++
suite. Python, Rust, and TypeScript integration runs once against Linux x64;
the other platforms concentrate on native and wheel packaging. Experimental
qualification is independent of publishing.

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

## Measured baseline

Before this split, a production C++ pull request ran four native matrices plus
WASM, standards, and experimental cross-transport qualification. The observed
job times on issue #34 were:

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
[issue #34](https://github.com/wavenumber-eng/geometer/issues/34).

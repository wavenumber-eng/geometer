# Build Automation Strategy

Geometer does not run GitHub Actions for pushes or pull requests. In particular,
documentation changes trigger no automation. Developers run the affected
checks locally before pushing and record important validation in the pull
request. Every workflow is manual-only. `Publish` is dispatched only for an
existing reviewed release tag and is the current path that builds and publishes
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
Production TypeScript validation has two explicit, disjoint scopes:
`GEOMETER_TYPESCRIPT_SCOPE=host` runs source and native-process checks in the
Linux native lane, while `GEOMETER_TYPESCRIPT_SCOPE=wasm` validates the package,
browser clients, Workers, and built sites only after the current WASM candidate
exists. Omitting the variable runs both scopes for local development.

## Manual workflows

The `Full Validation (Manual)` workflow is an explicit diagnostic fallback. It
never runs automatically. The experimental, macOS wheel, dependency-cache, and
operation-transport workflows are likewise manual-only. Dispatch one only when
its specific remote environment or controlled evidence is required. Every job
has an explicit timeout so a stalled runner or test cannot consume minutes
indefinitely. Python and Rust production strata run once in the Linux native
job. TypeScript's host scope runs there and its disjoint WASM scope runs against
the freshly built WASM candidate. Separate jobs must not repeat either scope or
rebuild the same wheel.

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
host/native integration runs once against Linux x64; candidate-WASM TypeScript
integration runs once in the WASM lane. The other platforms concentrate on
native and wheel packaging. Experimental qualification is independent of
publishing.

The four platform jobs and WASM feed one exact digest inventory. The workflow
attests the inventory and all four SDK archives, uploads the exact bytes to a
draft GitHub Release, re-downloads the draft, and only then sends the four
qualified wheels to PyPI through trusted publishing. A successful PyPI publish
allows the GitHub Release to become public. A final job downloads the public
assets again, checks every size and SHA-256, and verifies GitHub attestations.
No push, pull request, documentation change, tag creation, or GitHub Release
event starts this workflow.

## Locked OCCT dependency

All native and WASM jobs select one explicit profile from
`dependencies/occt-lock.json`. A clean runner downloads one exact
archive-digest-addressed R2 object; a warm local machine reuses an extracted
install only when its lock marker matches. Workflows do not use GitHub Actions
cache for OCCT, derive recipe keys, search legacy prefixes, or build from source
after a miss.

The manual `OCCT Dependency Producer` workflow is the only credentialed path.
It always source-builds, conditionally creates a new digest-addressed candidate,
and uploads only its small evidence files to GitHub. The candidate is inert
until a reviewed commit updates the lock.

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

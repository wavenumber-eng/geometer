# Build Automation Strategy

Geometer does not run GitHub Actions for pushes or pull requests. In particular,
documentation changes trigger no automation. Developers run the affected
checks locally before pushing and record important validation in the pull
request. Every workflow is manual-only. Candidate production and release
promotion are separate operations: candidate jobs compile and qualify once,
while promotion only moves verified existing bytes to PyPI and GitHub.

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

## Shared candidate command

Release lanes use the same entry point locally and in GitHub Actions. Run it
from a clean checkout with a new output directory:

```powershell
uv run --group dev python scripts/build_release_candidate.py native --platform windows-x64
uv run --group dev python scripts/build_release_candidate.py wasm
```

On the dedicated WSL builder, select `linux-x64` instead. The command has one
fixed SDK-before-native build, test, package, and metadata-check order for each
native platform and one fixed build/browser/package order for WASM. It writes
only under `out/release-candidate/<lane>/` plus the execution ledger. It refuses
a dirty checkout or an existing lane directory; use a fresh worktree or an
explicit new `--output-dir` rather than merging partial and new candidate bytes.
It does not infer changed files, hash recipes, discover cache aliases, or choose
tests dynamically.

Hosted WASM jobs reuse `build-wasm` only for an exact Git commit. They do not
maintain a hand-written source-file fingerprint or accept a nearest-prefix
fallback. The Emscripten SDK cache is keyed only by its pinned version and also
has no legacy fallback.

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

`Build Release Candidate` is dispatched from `main` without parameters. It uses
the dispatch revision and derives the expected tag from the checked-out package
metadata. Secret-free jobs build and qualify Windows x64, Linux x64, Linux
ARM64, macOS ARM64, and WASM through `scripts/build_release_candidate.py`.
Python, Rust, and TypeScript host integration run once on Linux x64; the
disjoint browser/Worker scope runs once against the WASM candidate.

The hosted aggregate creates the canonical candidate root and B0 release
inventory, verifies the complete payload, and attests the SDKs and inventory.
It retains the exact payload for 30 days as the run's `qualified-release`
artifact. Candidate jobs receive no publishing credentials.

After creating the immutable Git tag at the inventoried source, dispatch
`Promote Release Candidate` with only the successful candidate run ID. It
contains no compiler, CMake, Cargo, Emscripten, wheel builder, or packager. It
proves that the selected run is a successful candidate run from `main`,
downloads `qualified-release`, derives its source and tag from the inventory,
verifies the immutable tag, and idempotently publishes the exact wheels and
GitHub assets. An occupied filename is accepted only when its bytes are
identical. A rerun resumes missing channel operations and performs no
compilation.

Both workflows are manual-only. Credentialed jobs execute pinned actions and
code from the protected dispatch revision. Candidate builders receive no R2,
PyPI, or GitHub-release credentials. PyPI and GitHub publication use the
protected `pypi` and `release-github-production` environments. R2 is not a
release channel.

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

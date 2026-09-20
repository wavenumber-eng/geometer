+++
type = "plan_log"
id = "test-runtime-impact-audit-2026-09-20"
plan_id = "release-cycle-time-and-artifact-pipeline"
step_id = "test-runtime-impact-audit"
created = "2026-09-20T16:20:00-04:00"
+++

# Log: test runtime and critical-path audit

## Evidence boundary

This audit uses the current Rack index, the latest complete local Rack records,
current workflow source, and GitHub run `35411030740`. Local durations are
measured wall times from one workstation run and rank work; they are not treated
as stable cache identities or promises.

## One owning lane for every Rack test ID

| Rack IDs | Count | Measured local time | Candidate owner | Justification |
| --- | ---: | ---: | --- | --- |
| `L0_001` | 1 subtest / 5 pytest cases | 25.32 sec | each native platform build | `validate_native.py` already builds and runs the governed production CTests against that platform's candidate; do not run the Rack wrapper again in the candidate workflow |
| `PY_001`-`PY_032` | 32 subtests / 350 cases | 24.09 sec | Linux x64 native/client lane | one canonical executable-backed host is sufficient for cross-language behavior; wheel construction/install remains a separate per-platform package smoke |
| `TSP_001` | 1 subtest / 1 case | 10.90 sec | source preflight | contract generation is source-only and must finish before compilation |
| `DOC_001` | 1 subtest / 3 cases | 1.61 sec | source preflight | generated/offline documentation is source-only |
| `WASM_001`-`WASM_006` | 6 subtests / 12 cases | 33.84 sec | WASM candidate lane | browser, Worker, static-site, and cross-transport checks must consume the just-built WASM candidate |
| `TS_001` | 1 subtest / 30 cases | 6.36 sec | Linux x64 client plus WASM candidate profiles | source/package/native-process scripts run with Linux native; browser-WASM scripts run only after the candidate WASM build |
| `RUST_001` | 1 subtest / 2 cases | 46.07 sec | Linux x64 native/client lane | format, lint, generated contracts, and executable IPC need one canonical native candidate |
| `RUST_002` | 1 subtest / 2 skipped cases | 0 sec by default | optional native-viewer qualification | GPU/UI smoke remains opt-in and is required only for viewer-affecting changes or a viewer release |
| `L99_001` | 1 subtest / 20 cases | 13.98 sec | source preflight | release policy, lint, formatting, lock, and hygiene checks are source-only |

All 45 governed Rack subtests therefore have one candidate owner. Platform
specific behavior not represented by Rack remains explicit: 20 production
CTest registrations per native platform, wheel build/install/Twine checks per
wheel, relocated C/Rust link checks per SDK, and native/WASM archive validation.

## Proven duplicate work

The manual CI workflow ran Python, Rust, and TypeScript once inside its Linux
native job and also defined standalone jobs that invoked the same three Rack
strata. It likewise built and installed the Python wheel in both the Linux
native and standalone Python jobs. The current manual-only workflow has no path
selector, so those jobs are unconditional duplicate work. They are removed;
release tests require exactly one invocation of each stratum and one package
validation.

GitHub run `35411030740` predates the manual-only simplification and skipped the
standalone jobs through change classification. Its Linux native job still
shows the consolidated shape works: native build/test took 253 seconds, the
three client strata together took 102 seconds, and wheel validation took 13
seconds. Reintroducing three fresh runners would add checkout/tool setup and
repeat those same assertions without testing different candidate bytes.

The release workflow previously ran these TypeScript scripts in both the Linux
client stratum and WASM lane:

- `hlr_static_site_validation.mjs`;
- `illustration_static_site_validation.mjs`; and
- `wasm_client_validation.mjs`.

The Linux copy read checked-in WASM distribution files while the WASM lane read
the just-built candidate. The stratum now has two fixed scopes. `host` owns the
three source checks and two native-process checks. `wasm` owns every script that
consumes the generated package or built demo, including the Worker client and
protocol checks. CI and release invoke each scope once in its owning lane; the
unscoped local default still runs both. Direct workflow invocations of the
three scripts above were removed, as was the duplicate generic single-HTML
packager unit test from WASM jobs.

Relocated SDK validation compiles the Rust dependency graph once for the direct
backend test and again for the packaged illustration example. Those consumers
exercise different public package layouts, so neither test is removed. A shared
Cargo target cache may be benchmarked only if relocation and build-script
rerun behavior remain independently proven; correctness must not depend on it.

## Critical-path decision

The candidate matrix runs source preflight first, then platform builds in
parallel. Linux client tests stay attached to the Linux x64 build and the full
browser suite stays attached to WASM. Per-platform package and SDK smokes remain
parallel platform obligations. No test is moved to promotion: promotion only
rehashes, uploads, and verifies inventoried bytes.

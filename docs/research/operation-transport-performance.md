# Operation transport performance baseline

This record defines the sibling-process baseline protocol and numeric
regression budgets for the static SDK and embedded-worker work. It is not a
promise that arbitrary hardware will reproduce absolute times. The gates
compare transports built from the same clean revision on the same controlled
runner.

## Harness and corpus

[`benchmark_operation_transport.py`](../../scripts/benchmark_operation_transport.py)
executes the governed native
`geometry.model_illustration_geometry.a0` operation through a persistent
`geometer serve --stdio` client. Every run records process construction, the
first operation, warmed operations, a four-process pool, attachment sizes and
digests, executable identity, and memory observations. Windows records both
current resident sets and peak working sets. macOS records current resident
sets because the portable harness does not claim an OS peak counter there.

The corpus deliberately spans committed STEP models rather than using a
synthetic no-op:

| Size | Fixture | Input bytes | Input SHA-256 |
| --- | --- | ---: | --- |
| Small | `SOT-23.STEP` | 144,222 | `143a92fc2ada1a8a230381827df2ba3de8d33ee18aacb183792ff08513ec9f7e` |
| Medium | `BGA90-8X13mm.step` | 1,038,103 | `89f55fa5cca8a50212ff74c9593f84c275ec8f854b05a9c547f04d9f232ea95d` |
| Large | `XF2M_6015_1AH.step` | 6,377,912 | `3956d41047b456e17406ff67d4197203bac68ea98df870e4fed4108e6858c5dd` |

Each fixture receives three independent runs, two unmeasured warmups, ten warm
samples, and sustained one- and four-worker batches of at least five seconds.
One host thread owns one serial request loop per worker, and those loops start
at a barrier. The
operation request, input bytes, and output comparison rule remain identical for
the sibling executable, a self-hosted executable, and the direct static
backend. A direct four-worker static measurement means four host processes with
one process-wide Geometer lane each, never four concurrent native lanes in one
process.

The later cross-transport qualification adds a canonical indexed-mesh
illustration request supported by the portable catalog. Its exact request and
attachment bytes are replayed through executable IPC, native C ABI, direct
WASM, and browser Worker WASM. The STEP corpus remains the native stress corpus;
an unsupported WASM STEP operation is reported rather than silently replaced.

## Preliminary Windows x64 diagnostic

An initial local capture on Windows 11 x64 on 2026-09-18 used the MSVC Release build produced
by `scripts/validate_native.py`. The executable is 20,338,176 bytes with
SHA-256
`e68d27b6d0dd662e969361ec67945692bf9db75572532bb1ac8088b48fc127a0`.
The source HEAD recorded by the report is
`03e0c5c14ce29c38e51ba50b34c4a3020e5762d1`; each JSON report also preserves
the dirty-worktree diagnostic. Review found that its pool scheduler could idle
one child behind another child's client lock, its small batch was only about a
quarter second, and its memory totals were not simultaneous samples. The values
below are retained only to explain that discarded experiment. They are not an
accepted baseline and must not set a gate.

Times are milliseconds. Startup, first-operation, warm p50, and pool throughput
are medians across the relevant populations; warm p95 covers all 30 warm
samples.

| Fixture | Startup | First | Warm p50 | Warm p95 | Four-worker jobs/s |
| --- | ---: | ---: | ---: | ---: | ---: |
| Small | 10.211 | 39.907 | 36.808 | 38.709 | 83.542 |
| Medium | 9.510 | 638.560 | 844.966 | 887.841 | 3.457 |
| Large | 8.566 | 2,093.764 | 2,604.227 | 3,187.316 | 1.195 |

| Fixture | Output bytes | Output SHA-256 | Max single peak working set | Max four-worker aggregate peak |
| --- | ---: | --- | ---: | ---: |
| Small | 70,147 | `505904d6bb6c7d23f5643813029de7f270d929ee4717e0816e0de060ec785d11` | 16,789,504 | 65,671,168 |
| Medium | 1,389,252 | `a95396d0ddb28e38f961d4174b93fbafd64f3e4b6cb05d3c78a1625edb4eedb6` | 48,807,936 | 191,266,816 |
| Large | 1,448,345 | `6a658b6c234587c5c4981f1aadb495eaa9b0acec2dd4879c9f3cc88cc72faac9` | 120,598,528 | 478,392,320 |

The discarded detailed local reports are generated under
`out/operation-transport-performance/` and are intentionally untracked.

## Reviewed comparison budgets

Qualification runs compare the candidate and sibling baseline back-to-back on
the same runner in alternating AB/BA order. For every fixture, the candidate
must satisfy all applicable rules below. A timing miss is re-run once in an
otherwise idle qualification job; a repeated miss blocks promotion.

| Measure | Self-hosted stdio budget | Direct static budget |
| --- | --- | --- |
| Startup or construction median | no more than `max(1.15 * sibling, sibling + 25 ms)` | record separately; no process-start equivalence claim |
| First-operation median | no more than `max(1.10 * sibling, sibling + 5 ms)` | no more than `1.10 * sibling` |
| Warm p50 | no more than `max(1.10 * sibling, sibling + 2 ms)` | no more than `1.10 * sibling` |
| Warm p95 | no more than `max(1.15 * sibling, sibling + 2 ms)` | no more than `1.15 * sibling` |
| One-worker throughput | at least `0.90 * sibling` | at least `0.90 * sibling` |
| Four-worker throughput | at least `0.90 * sibling` | at least `0.90 * sibling` using four host processes |
| Four/one scaling | no more than 10 percent below sibling scaling | no more than 10 percent below sibling scaling |
| Output | exact attachment length and SHA-256 | exact attachment length and SHA-256 |

Self-hosted and direct measurements must also report the host's idle resident
set and the incremental resident set after each fixture. The candidate's
four-worker sampled peak incremental resident set may not exceed
`max(1.15 * sibling, sibling + 32 MiB)`. Windows OS-recorded per-process peaks
are supplemental and are not summed as though they occurred simultaneously.
Static SDK,
final executable, and stripped-symbol sizes are recorded in release evidence;
they are not substituted for runtime memory.

These budgets are regression limits, not a rule for choosing the direct
backend. Containment, cancellation, four-worker throughput, and exact behavior
remain co-equal acceptance criteria.

## macOS ARM64 evidence gate

The controlled baseline workflow runs this same three-fixture matrix on
Windows x64 and `macos-15`. The raw JSON, workflow URL/run ID, artifact digest,
executable attestation and digest, exact source revision, and clean-checkout
proof are retained as governed evidence. Both platforms' results must be
reviewed before dispatcher implementation is allowed to start. Each platform
uses its own sibling baseline and the relative budgets above; Windows numbers
are never used as macOS thresholds.

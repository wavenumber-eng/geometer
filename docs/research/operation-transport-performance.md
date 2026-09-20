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
| Small | `SOT-23.STEP` | 142,275 | `52020ac35972e9102d1eac88f15b565babc5265db8d0629036678df78217abbf` |
| Medium | `BGA90-8X13mm.step` | 1,024,844 | `6226820bd328584cf2d3fbeb5b30a164b9add21d7a839de0287eef883a07e989` |
| Large | `XF2M_6015_1AH.step` | 6,291,779 | `8c4a0f48e8693ef30c0575ffbaddbc8303d52084b8c338ea437dfa1430d7d23e` |

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

## Accepted sibling-process baseline

The controlled captures on 2026-09-19 are the pre-implementation authority.
Times are milliseconds; timing and throughput values are medians across three
independent runs, and warm p95 is computed from each report's full warm sample
population. Resident-set values are the maximum complete simultaneous sample
observed in any run.

Windows x64 used source revision
`1563422c035aa8399f7262b94eefb9f763a60248`. Its job completed successfully in
[run 35411030723](https://github.com/wavenumber-eng/geometer/actions/runs/35411030723);
the overall matrix was later cancelled to stop an unrelated cold macOS OCCT
build. The executable is 20,332,032 bytes with SHA-256
`f352f374584e74ac7966ca2a919d4423c00064467928182678a809c5d82f785f`.

| Fixture | Startup | First | Warm p50 | Warm p95 | One-worker jobs/s | Four-worker jobs/s | Scaling |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Small | 11.661 | 73.627 | 68.552 | 82.287 | 14.787 | 31.210 | 2.156 |
| Medium | 11.540 | 1,102.595 | 1,103.065 | 1,203.609 | 0.858 | 1.554 | 1.811 |
| Large | 11.533 | 3,546.384 | 3,569.108 | 3,998.669 | 0.270 | 0.508 | 1.910 |

| Fixture | Output bytes | Output SHA-256 | Single RSS | Four-process RSS |
| --- | ---: | --- | ---: | ---: |
| Small | 70,147 | `505904d6bb6c7d23f5643813029de7f270d929ee4717e0816e0de060ec785d11` | 16,334,848 | 68,378,624 |
| Medium | 1,389,252 | `a95396d0ddb28e38f961d4174b93fbafd64f3e4b6cb05d3c78a1625edb4eedb6` | 48,398,336 | 170,524,672 |
| Large | 1,448,345 | `6a658b6c234587c5c4981f1aadb495eaa9b0acec2dd4879c9f3cc88cc72faac9` | 120,119,296 | 466,001,920 |

macOS ARM64 used source revision
`f4039933a581d2f80b0e6b8a35501f2658228085` in successful manual
[run 35412675064](https://github.com/wavenumber-eng/geometer/actions/runs/35412675064).
The revisions differ only by automation policy and its documentation/tests;
native source, contracts, fixtures, and build inputs are identical. The
executable is 35,422,472 bytes with SHA-256
`f3942cfa38fbfd6619788f4e30b546af98db50fcf84f3f321310a132b807eb9f`.

| Fixture | Startup | First | Warm p50 | Warm p95 | One-worker jobs/s | Four-worker jobs/s | Scaling |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Small | 13.682 | 40.695 | 31.121 | 35.488 | 31.786 | 78.650 | 2.615 |
| Medium | 16.277 | 510.462 | 548.755 | 652.616 | 1.734 | 4.193 | 2.361 |
| Large | 24.174 | 1,995.572 | 1,942.788 | 2,818.712 | 0.507 | 1.199 | 2.328 |

| Fixture | Output bytes | Output SHA-256 | Single RSS | Four-process RSS |
| --- | ---: | --- | ---: | ---: |
| Small | 70,028 | `eb62259528ee94101a83f19cf4b8cd4f557789ce6a789128755ca7abdc11f888` | 43,450,368 | 189,906,944 |
| Medium | 1,389,162 | `22fc0868967378ab9cb3cefdaffb3bdedc7ff9cf83a151b9db7501a6ce017319` | 200,130,560 | 609,566,720 |
| Large | 1,448,437 | `15e8ba6b9c10eb3563c61f9673fd231890e78dd28003ff1933eaa2286e62baba` | 776,847,360 | 1,882,243,072 |

The raw reviewed reports and artifact identities are retained in the
[evidence inventory](evidence/operation-transport/sibling-baseline-2026-09-19/inventory.json).
The earlier local Windows diagnostic remains under ignored `out/` state and is
not an acceptance baseline.

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

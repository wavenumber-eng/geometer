# Illustration performance and geometry output

Measured on Windows 11 x64 on 2026-09-11. Native builds used MSVC Release
(`/O2`) and the existing static OCCT 8.0.1 cache. OCCT was not rebuilt.
These measurements qualify this development change; they are not a release tag
or cross-platform performance promise.

## Why integer formatting was expensive

The native illustration renderer emulates JavaScript decimal formatting for
stable welding keys, projected-edge keys, fusion keys and SVG text. Its general
shortest-decimal fallback repeatedly formats scientific decimal candidates with
streams and parses them with RapidJSON's full-precision conversion. Its separate
fixed decimal rounding path can expand to 1,074 digits.
Applying that general machinery to already integral, bounded coordinates made
thousands of small key constructions dominate preparation and fusion.

`number_text` now directly emits finite integral values with magnitude below
1e12. `integer_text` first applies the existing JavaScript-compatible rounding,
then uses the same bounded integer conversion. Bounds are checked before casts;
all values in the fast range are exactly representable in binary64 and within
the existing 12-significant-digit number policy. Both paths normalize negative
zero to `0`. Nonintegral numbers, larger values, nonfinite handling and
`fixed_text` keep the established fallback. Numeric boundary tests include
halfway neighbors, the positive/negative cutoff and representable extremes.

This change applies only to the native illustration implementation, including
shared preparation/fusion for SVG and drawing geometry. It does not change
STEP import, tessellation, HLR, GLB or the separate TypeScript renderer.

## Paired measurements

[benchmark_mesh_illustration.py](../../scripts/benchmark_mesh_illustration.py)
uses the SOT-23 fixture and a deterministic tilted, two-material 4,608-triangle
grid. Three alternating baseline/candidate samples run in each lane. Fixture
preparation and STEP tessellation occur outside measured calls. Native-process
timing includes process startup and JSON I/O; persistent IPC includes Python
adaptation, transport and typed decoding. Output re-encoding occurs afterward.
The full result, including SVG, statistics and warnings, matched exactly in
all 24 samples. Timings below are medians in seconds.

| Fixture | Lane | Baseline | Integer fast path |
| --- | --- | ---: | ---: |
| Grid | Native process | 4.8288 | 0.2691 |
| Grid | Persistent IPC | 5.2132 | 0.3223 |
| SOT-23 | Native process | 0.2448 | 0.0272 |
| SOT-23 | Persistent IPC | 0.2740 | 0.0257 |

Full Altium Cruncher `toon` runs disabled PNG and disk caching, included project
loading, STEP/HLR, layers, composition, writing and worker shutdown. RT Super C1
included all three variants and both sides (six SVGs); Loz Old Man included both
sides (two SVGs). Every output hash and model/warning count matched in each pair.
These are single whole-job samples, not medians or confidence intervals.

| Board | Workers | Baseline seconds | Integer fast path seconds |
| --- | ---: | ---: | ---: |
| RT Super C1 | 1 | 16.817 | 10.713 |
| RT Super C1 | 4 | 15.449 | 11.292 |
| Loz Old Man | 1 | 230.030 | 60.741 |
| Loz Old Man | 4 | 106.494 | 56.901 |

Local reports preserve fixture/binary hashes, individual samples and outputs in
`out/illustration-performance/paired.json` and `boards/summary.json`. Baseline
executable SHA-256 was
`6d1cc85d1e91b6f68074f2949e9c2574582ef1ccf1f1d0d3ebf0e0c434516aae`;
the frozen phase-1 candidate was
`04ba13b41272b1bb01dac3ac82e8aa0ed414f0ff182dd222d8df51c0d9b5339f`.
Reproduce the phase-1 IPC comparison with matching generated clients: the later
additive geometry catalog requires a matching client/runtime handshake.

After the shared geometry refactor, RT Super C1 and Loz were rerun with four
workers and the matching source client/runtime. Their eight SVGs remained
byte-identical to the frozen phase-1 candidate. Timings were 11.27 and 61.13
seconds; concurrent build work means these are compatibility checks rather than
a second controlled speedup experiment. An isolated RT cache-fill/warm pair
took 12.98/7.54 seconds and retained identical SVG hashes.

## B0 clipping release check

Before the 2026.9.19 release, a focused Windows 11 x64 MSVC Release replay
compared the retained A0 operations with B0 over one persistent executable IPC
process. Each case used three warmups followed by nine alternating samples. The
direct-model case used `SOT-23.STEP`; the composed-mesh case used the existing
deterministic tilted grid. This is comparative release evidence, not a
cross-platform timing promise.

| Input | B0 no clipping / A0 | B0 retain-all plane / A0 | B0 midpoint clip / A0 | B0 empty clip / A0 |
| --- | ---: | ---: | ---: | ---: |
| SOT-23 direct model | 1.019 | 1.031 | 0.970 | 0.875 |
| 32,768-triangle composed mesh | 1.050 | 1.134 | 0.476 | 0.049 |

The composed-mesh A0 median was 0.373 seconds. Its B0 no-clip median was
0.392 seconds, and the deliberately pessimistic plane retaining every triangle
was 0.423 seconds. The SOT-23 medians were 0.0312, 0.0318, and 0.0322 seconds,
respectively. Smaller 1,152-, 4,608-, and 18,432-triangle grids showed linear
growth and no pathological change. Enabled midpoint and empty clipping reduced
total time because bounds, projection, and SVG generation consumed less output
geometry. No serious B0 speed regression was observed.

## Geometry without SVG

The additive [drawing API](../design/mesh-illustration-geometry.md) shares native
validation, preparation, ordering, fusion and HLR composition with SVG. It stops
before SVG writing. A C++ link-time tripwire independently verifies this, and the
direct value call does not serialize its output. The IPC adapter serializes one
declared JSON attachment and typed clients validate/decode it.

[benchmark_illustration_geometry.py](../../scripts/benchmark_illustration_geometry.py)
compares current SVG and geometry calls using identical meshes/view/style. It
also measures Python output re-encoding separately. Initial three-sample medians
were 0.309/0.307 seconds for grid SVG/geometry and 0.026/0.032 seconds for SOT-23.
SOT-23 geometry JSON was 47,390 bytes versus 15,259 for the SVG result envelope;
Python geometry re-encoding alone took about 0.004 seconds. These are combined
client-call measurements, not separate native preparation/DTO/writer timers;
concurrent build work makes them directional rather than a speedup claim.

The opt-in large case returned 72,200 individual triangle surfaces in a
15,426,820-byte geometry JSON attachment in 6.51 seconds. This exceeds the 8 MiB
inline envelope limit without changing that limit. A subsequent SVG request
on the same connection succeeded. The geometry API serves custom renderers;
compact fused SVG can remain smaller and faster to transport.

## Candidates identified after the first pass

The bounded source audit found no copy of illustration's pathological decimal
search in other algorithms. Ordinary formatting calls elsewhere are not evidence
of the same bottleneck. That first pass changed no other algorithms; the
user-requested owned-linework follow-up is recorded below.

Earlier private component instrumentation, after the integer fix, measured U24
fusion at about 0.353 seconds and preparation at 0.158 seconds of a 0.608-second
render; U19 preparation took 0.184 seconds and fusion 0.033 of 0.244 seconds.
These are diagnostic replays, not fresh phase-2 or whole-board phase timings.

1. Reuse repeated material/opacity key strings within one render. U24 fusion
   still invoked `fixed_text` 30,704 times; U19 invoked it 2,772 times. These are
   call counts, not exclusive formatter timings. Preserve the exact rounded key.
2. Investigate reusing or avoiding diagnostic raw-edge preparation when raw
   outlines and creases are disabled. Preserve validation, diagnostics and any
   incidence data needed by later stages.
3. Profile projected-bounds reuse and candidate clipping within visibility/fusion
   before changing either algorithm. They involve more semantic risk.

The follow-up below implements repeated material/vertex-key reductions from
this list. Raw-edge preparation remains present to preserve validation and
incident-edge semantics.

## Second pass: captured models and repeated work

The opt-in corpus now includes Loz Old Man, NXP FRDM i.MX93 and OV Tech PiMX8.
Cruncher owns project fixtures and source-hash manifests; Geometer captures are
local diagnostic inputs under `out/illustration-performance/second-pass/`.
Each capture identifies unique STEP sources, extruded bodies, component pose and
side. Loz has 44 unique STEP sources and 90 posed illustration requests; NXP and
OV have 167 and 141 posed requests respectively. Replays hold tessellated meshes,
view, style and HLR fixed. They do not repeat STEP import during native rendering.

Two alternating samples per mode produced these sums of native render medians:

| Corpus | Original formatting | First integer fix | Second pass |
| --- | ---: | ---: | ---: |
| Loz, 90 poses | 133.280 s | 4.100 s | 2.301 s |
| NXP, 167 poses | 240.604 s | 7.576 s | 4.877 s |
| OV, 141 poses | 69.097 s | 2.839 s | 1.648 s |

These are independent native-call totals, not parallel whole-board wall times.
Private probes exclude process startup, JSON decoding/encoding, tessellation and
HLR generation. Some diagnostic runs overlapped other captures or probes; small
timing differences are not significant. All 2,388 complete illustration results
(398 poses, three modes, two repetitions) compare exactly, including SVG,
statistics and warnings. Filterable per-pose HTML reports and source-model names
are beside `loz-comparison.json`, `nxp-comparison.json` and `ov-comparison.json`.

Native changes reuse each triangle's three endpoint keys, remove an unused
fusion-edge key, avoid a temporary triangle ring allocation, hoist determinant
signs, and make exact zero/one fixed-format strings cheap. Material distinction
formats keys only when raw values differ, preserving 12-place collisions and
continuing coplanarity/depth checks after finding two materials. Regression tests
cover these two easily missed semantics.

### Python transport

The full-board profile exposed repeated per-number dispatch and JSON-pointer
construction in generated-codec runtime support. Direct `float64`, `uint32` and
`uint64` arrays now resolve their type once and build indexed paths only for an
invalid element, using the original scalar decoder to report the same error.
Duplicate-field traversal skips scalars but still visits every nested container.
SVG and geometry facades validate their complete input once, then serialize the
already validated mesh subtree rather than encoding it twice.

Isolated three-sample median codec comparisons preserve exact bytes/values:

| Loz input | Encode before/after | Decode before/after |
| --- | ---: | ---: |
| U19 bottom | 0.446 / 0.140 s | 0.681 / 0.180 s |
| U24 bottom | 0.489 / 0.148 s | 0.628 / 0.172 s |

This helps Python mesh/geometry codecs beyond SVG. It does not change wire
contracts, C++ DTO semantics, or other clients. Numeric decoding temporarily
holds a list before producing its tuple, costing one extra pointer per element.
Whole-board comparisons must include peak memory as well as time.

### Owned Fast HLR and mesh shadow

Prepared validation was a small measured share of detail projection; retaining
it avoids trusting mutable public prepared values. The largest NXP capture spent
about 0.102 of 0.161 seconds in welding, and 0.074 of 0.225 shadow seconds in
candidate-pair budget accounting. Selected changes are:

- Use a lookup-only weld hash table, retaining input order and minimum matching
  vertex ID; skip distance checks for IDs that cannot improve the match.
- Deduplicate grid bucket references with per-view generation stamps, then sort
  candidates in the same order before visibility tests and budget charging.
- Compute reported hidden intervals only when hidden output is requested.
- Cache shadow segment Y ranks for the sweep's repeated queries/inserts/removals.
  Inclusive endpoint contact and candidate budget charging remain unchanged.

Three alternating samples per mode compared full-precision detail and shadow
segment arrays exactly for all 398 poses. Candidate-pair counts also match.

| Corpus | Preparation before/after | Detail before/after | Shadow before/after |
| --- | ---: | ---: | ---: |
| Loz | 0.526 / 0.361 s | 0.395 / 0.362 s | 0.506 / 0.472 s |
| NXP | 0.853 / 0.589 s | 0.670 / 0.629 s | 1.034 / 0.980 s |
| OV | 0.233 / 0.161 s | 0.249 / 0.241 s | 0.270 / 0.254 s |

Stamps add four bytes per triangle; shadow ranks add two `size_t` fields per
segment. Weld hash buckets scale with input vertices. No OCCT meshing changes
were made. Further visibility/Clipper work has smaller absolute benefit and
greater semantic risk than the repeated-work changes captured here.

### Whole-board follow-up

Single isolated pairs used four workers, SVG only and no disk model cache. The
baseline is the frozen first-integer-fix runtime with the original Python
runtime/illustration adapter. The candidate combines the second-pass native and
Python changes. Both use matching generated contracts. RT renders all three
project variants; the stress boards use their base variants. Source inputs,
configuration, per-layer/side/variant timings and executable hashes are retained
in the local `board-comparison.json` evidence and its individual job reports.

| Board | Before | After | Sampled peak aggregate RSS before/after |
| --- | ---: | ---: | ---: |
| Loz Old Man | 52.30 s | 36.95 s | 693.9 / 692.5 MiB |
| RT Super C1, three variants | 9.95 s | 10.04 s | 322.0 / 321.4 MiB |
| NXP FRDM i.MX93 | 122.61 s | 102.91 s | 1615.9 / 1596.9 MiB |
| OV Tech PiMX8 | 51.20 s | 42.65 s | 922.4 / 923.7 MiB |

All 12 SVGs and job counts/warnings are identical. Small differences such as RT's
0.09 seconds are noise, not evidence of a speedup or regression. RSS is sampled
every 250 ms, can miss transient peaks and counts shared pages in each process;
these observations do not establish a guaranteed memory reduction.

Loz's Python CPU sample fell from 41.02 to 26.55 seconds. Its illustration layer
wall time fell from 40.70 to 26.11 seconds across both sides. These layer times
include tessellation, placement, IPC and scheduling, not just native rendering.
NXP's remaining ordinary copper/mask/silk layers each take several seconds, with
another roughly 17 seconds in project and board loading. Further whole-board
work requires fresh attribution on the Cruncher side; the remaining Python CPU
must not be labeled entirely as Geometer overhead.

The local `index.html` links every board output and per-model report. Its links
were checked and its rendered desktop page inspected in headless Chrome.

### Second-pass review and focused test cost

Independent reviews found no blocking implementation issue. They checked exact
numeric/error behavior, validated-subtree reuse, rounded material collisions,
deterministic welding, grid candidate order and inclusive shadow budget counts.
Suggested tests were added for continued depth checks after material distinction,
minimum-ID welding across reverse cell order, and exact 215/216 grid budgets.

The numeric-array/subtree test file adds 36 small parameterized cases; with the
existing three governed-codec tests it runs in about 0.25 seconds. The final
combined Python contract/SVG/geometry set passed 47 tests in 2.75 seconds. Five
focused native tests passed in 10.74 seconds, including 4.39-second exact
TypeScript parity and 6.18-second projection-outline coverage. Rust's existing
view/mirror/line-toggle integration passed in 3.61 seconds. Timing comparisons
remain opt-in; normal tests contain no speed thresholds or full-board fixtures.

The review found no further demonstrated low-risk native optimization worth
expanding this slice. Full release gates and other platform qualification remain
separate from this development handoff.

The second-pass Windows distribution and local wheel were rebuilt and checked,
including installed-package illustration and STEP/HLR/SVG paths. Full WASM was
rebuilt using existing OCCT installs and passed the generic browser client suite,
including exact SVG and drawing-geometry fixtures. The regenerated Illustration
Lab passed static closure checks and its Chrome smoke (21.60 seconds). The
Python Canvas example was rerun with the new runtime and rendered successfully.
Current artifact hashes are in the [development handoff](../developer/illustration-geometry-handoff.md).

## First-pass test cost and qualification

This section records the earlier geometry-API candidate. It does not qualify
second-pass artifacts until the follow-up artifact refresh is recorded.

The focused three-test CTest illustration suite took 3.39 seconds, including
numeric/SVG parity and raw drawing parity; the new link-time test took 0.04
seconds. Four new Python IPC tests took 1.04 seconds. Rust's existing 24-view/
mirror/toggle composition loop, extended with geometry calls, took 4.31 seconds.
Full WASM replays the drawing fixtures and exact SVG results. Large timing and
transport trials are opt-in and introduce no timing assertions into normal tests.

The source API, Windows native artifact, full browser WASM operation and installed
Windows wheel have focused validation. Planar-only WASM intentionally has no
generic operation API. The new Python example was run through native IPC and
its standalone Canvas output inspected in headless Chrome. Native Rust GUI uses
the same SVG operation; browser Lab remains on its existing TypeScript renderer.
Cross-platform binaries, full release gates, versioning and downstream package
pins remain separate release work. No package was published or tag created.

The rebuilt Illustration Lab passed its static artifact check and headless
Chrome smoke (22.62 seconds). The native Rust sample rebuilt against the new
generated client and passed 11 ordinary tests (0.47 seconds) and three opt-in
native/settings/view tests (1.83 seconds). Installed Windows
wheel validation exercised both old SVG and new geometry conveniences using
the wheel's bundled executable, plus the existing STEP/HLR/SVG example.

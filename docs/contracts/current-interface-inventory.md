# Current Interface And Contract Authority

Updated for release target `v2026-09-07`. This inventory
describes interfaces, not a promotion decision. The
[promotion manifest](promotion-manifest.toml) governs lifecycle evidence;
the [generated coverage matrix](../generated/contracts/coverage.html) joins
every inventoried operation and contract with the normalized TypeSpec catalog.
The [2026-08-12 snapshot](../research/history/interface-inventory-2026-08-12.md)
is historical and must not be used for runtime discovery.

## Authority Layers

| Layer | Authority and generation |
| --- | --- |
| Generated logical roots and operation metadata | Authored `src/tsp/geometer/main.tsp` and its imports; normalized catalog drives JSON Schema, C++, TypeScript, Rust, Python and HTML. Promotion remains per-contract. |
| Solver behavior and semantic validity | Focused C++ implementations; browser illustration behavior lives in TypeScript. DTO generation does not generate algorithms. |
| IPC control JSON | TypeSpec `ipc-a0.tsp`; framing is separately specified GMIPCA01, not a generated binary codec. |
| Legacy file CLI and JSON batch | Handwritten parsing/adapters in `src/cpp/cli/`; preserve aliases, defaults, patch presence, diagnostics and permissive compatibility behavior. |
| C ABI and WASM exports | `src/cpp/lib/geometer/c_api.h` and `src/cpp/lib/CMakeLists.txt`; export inventory/checks live in the promotion manifest and its tests. |
| Python public API | `python/geometer/__init__.py`, convenience wrappers and executable transport. Generated internals do not replace every public dictionary API. |
| Mesh illustration | Generated options/results; browser TypeScript renderer and native C++ renderer exposed through executable IPC and Rust/Python clients since 2026.9.6. |
| Native value helpers | Focused public headers and [STEP](../design/step-geometry.md)/[planar](../design/planar-geometry.md) references; aliases, helpers and ownership/version functions are not counted as separate wire operations. |

The catalog contains 18 generated operation declarations: six with the portable
runtime flag (model bounds, model HLR, mesh HLR, experimental analytic,
tessellation and illustration), nine additional native-only experimental
topology operations, and three structural-only
topology operations. Effective native availability is portable OR additional
native-only availability. Always discover the actual executable's operations
through its welcome catalog; see the [IPC guide](../design/executable-ipc.md).
The new tessellation/illustration APIs are qualified through native clients;
catalog flags alone do not establish browser build/test coverage.

Model bounds is promoted. HLR structural contracts are pilots. Analytic and
topology remain experimental/non-production regardless of callable transports
or frozen bytes. Public package support and demo-application support are
different decisions.

## Operations Awaiting Generated Contracts/Adapters

| Inventoried operation | Handwritten source | Existing boundary | Migration |
| --- | --- | --- | --- |
| `geometry.model_to_glb.a0` | `step_to_glb_options_json.cpp`, `step_to_glb.cpp` | C++, file CLI/Python, legacy STEP C ABI/WASM | Wave 1 |
| `geometry.planar_step.a0` | `planar_step.cpp` | C++, file CLI/Python | Wave 1 |
| `geometry.planar_batch_solve.a0` | `planar_solve.cpp` | C++, C ABI/WASM, file CLI/Python | Wave 2 |
| `geometry.planar_triangulate.a0` | `planar_triangulate.cpp` | C++, packed C ABI/WASM | Wave 2 |
| `geometry.clipper2_boolean.a0` | `clipper2_bytes.cpp` | C++, packed C ABI/WASM | Wave 2 |
| `geometry.clipper2_inflate_open.a0` | `clipper2_bytes.cpp` | C++, packed C ABI/WASM | Wave 2 |

These are inventory identities, not names accepted by generic IPC today.
The legacy `geometer.batch.request.a0` / response and
`geometry.projection.b0` compatibility boundary are wave 3. The
[roadmap](typespec-coverage-assessment.md) requires field-level compatibility,
cross-language vectors and runtime adapter evidence before authority switches.
The [public entry-point reconciliation](public-entrypoints.md) maps direct
helpers, client exports and aliases to these families or explicit support/
research boundaries. This is an inventory, not completion of their migration.

## Packet Authority

| Packet | Magic / version | Layout/codec ownership |
| --- | --- | --- |
| IPC framing | GMIPCA01 / A0 generation 0 | [Wire reference](../design/executable-ipc-a0.md); handwritten framing in each client/server |
| Indexed mesh | GMIMSH01 / 1 | [Binary reference](../design/binary-formats.md#indexed-triangle-mesh-a0); handwritten C++/TS/Rust/Python |
| Analytic request/result | GMABRQ01 / GMABRS01, A0 | [Analytic packet specification](../design/analytic-planar-boolean-packet-a0.md); generated logical DTOs, handwritten packed codecs |
| Planar batch | GMPBRQ01 / GMPBRS01, 2 | `planar_solve.cpp`; [binary reference](../design/binary-formats.md) |
| Triangulation | GMTRRQ01 / GMTRRS01, 1 | `planar_triangulate.cpp`; [binary reference](../design/binary-formats.md) |
| Clipper2 Boolean | GMC2BQ01 / GMC2BS01, 1 | `clipper2_bytes.cpp`; [binary reference](../design/binary-formats.md) |
| Clipper2 open inflate | GMC2IQ01 / GMC2IS01, 1 | `clipper2_bytes.cpp`; [binary reference](../design/binary-formats.md) |

Native topology also transports STEP, GLB and experimental checkpoint/compact
tables. Their formats and failure boundaries remain in the
[Slice A](../design/step-topology-contract-a0.md),
[Slice B](../design/step-topology-contract-slice-b.md),
[Slice C](../design/step-topology-contract-slice-c.md) and
[edit-journal](../design/step-topology-edit-journal.md) references. Generated
attachment descriptors do not imply generated internals of these payloads.

## Compatibility And Toolchain

Keep file-oriented APIs and legacy STEP aliases intact during migration.
[Named consumer snapshots](compatibility/viz-2026.6.10.toml) record frozen
compatibility requirements, not a claim about today's downstream installation.
[TypeSpec authoring](typespec-toolchain.md) documents the single catalog and
[HTML generation](generated-contract-reference.md) documents its presentation.

`analytic-candidate.tsp` imports `main.tsp`, which already includes analytic
declarations. Retain that entry point for now: `check:analytic-candidate`
uses it as a focused candidate/vector gate. Import overlap alone is not evidence
that removing the validation boundary is safe.

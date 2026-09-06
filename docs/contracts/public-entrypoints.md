# Public Entry-Point Reconciliation

Release baseline `v2026-09-04`, with explicitly labeled native development
additions below. This maps semantic operations separately from
aliases, byte projections and implementation helpers. The
[generated inventory](../generated/contracts/coverage.html) supplies individual
operation/root identities; the [source review lock](public-surface-review.json)
records the reviewed public-source boundary. A changed or newly added source
in that boundary requires documentation review, not automatic promotion.

## User-Facing Operation Families

| Family | C++ / C ABI and CLI boundary | Python / TypeScript / Rust mapping | TypeSpec work |
| --- | --- | --- | --- |
| Model bounds | `model_bounds.h`; generic operation ABI; `model-bounds` / batch `model_bounds_json` | Python `model_bounds[_json]`; generic TS/Worker/IPC operation; Rust `ModelBoundsRequest` | Promoted generated roots; retain compatibility normalization. Wave 0 reconciliation, no new solver. |
| Model HLR / SVG presentation | `projection.h`, `fast_hlr.h`; generic operation plus legacy STEP byte ABI; `model-project-hlr/svg`, STEP aliases | Python `project_model_hlr`, `model_hlr_projection_json`, STEP wrappers; TS/Rust model-HLR requests | A0 options/result generated pilot; legacy projection B0/SVG file boundary remains wave 3. SVG is presentation of projection, not an extra generated geometry operation. |
| Mesh HLR | `fast_hlr.h`, `indexed_mesh_packet.h`; generic operation; `mesh-project-hlr` / `mesh_hlr_projection_json` | Python `GeometerClient.mesh_hlr_projection`; TS/Rust mesh-HLR requests | Generated HLR envelope; handwritten indexed-mesh codec pilot is wave 4. |
| Model-to-GLB | `step_to_glb.h`; legacy STEP byte ABI; `model-to-glb` / `step-to-glb` and batch aliases | Python `model_to_glb` / `step_to_glb`; retained direct WASM export | Wave 1 structural options/result descriptor and IPC adapter; preserve GLB bytes. |
| Planar STEP synthesis | `planar_step.h`; `planar-step` / batch `planar_step` | Python `planar_step` / `write_planar_step` | Wave 1 request/root and STEP attachment; preserve unit/shape/alias compatibility. |
| Planar batch solve | `planar_solve.h`; packed/JSON C ABI variants; `planar-batch-solve` | Python `planar_batch_solve[_json]`; retained WASM byte functions | Wave 2 logical contracts and IPC adapter, wave 4 packed codec assessment. |
| Triangulation, Clipper2 Boolean/open inflate | `planar_triangulate.h`, `clipper2_bytes.h`; matching C ABI and WASM functions | No invented public Python/Rust convenience method; use documented existing byte boundary | Wave 2, then wave 4; three distinct operation families. |
| Analytic Boolean batch | `analytic_filtered_batch.h`, generic dispatch; packed request/result projections | Python/TS/Rust analytic convenience calls | Logical candidate generated; packed codec remains handwritten. Wave 0 inventory and wave 4 codec assessment, never a production-promotion claim. |
| Topology sessions and mutations | `step_topology_session.h`; nine native IPC operations and three structural-only declarations | Generic generated clients plus native Node research reference | Wave 0 preserves each operation's availability. Save/analyze-recovery/apply-hierarchy are not executable capabilities. |
| Colored model tessellation (development) | `model_tessellation.h`; governed native IPC operation | Typed Rust/Python executable methods | Generated mesh collection/request/result/operation; unreleased candidate. |
| Mesh illustration | Browser renderer; unreleased direct C++ `mesh_illustration.h` and `geometry.mesh_illustration.a0` native operation | Browser exports, typed Rust/Python executable methods and Python one-shot helper | Existing generated A0 input/style/result reused; generated settings plus mesh attachment and optional bounded HLR-result attachment. Native composition handles selected detail/outline ordering. See [native boundary](../design/mesh-illustration-native.md). |

## Helpers, Aliases And Ownership Are Not Additional Wire Operations

- `planar_contours.h` exposes a direct C++ contour-building helper used by HLR.
  Its native value structures remain handwritten. If offered as an independent
  portable operation, include its structural request/result in wave 2 before
  exposing an adapter; do not silently count it as already TypeSpec-covered.
- `analytic_*.h` lowering, arrangement, lineage, numeric filters and packet
  components implement the analytic family. `exact_*.h` exposes non-primary
  experimental algebra/oracle components, not an alternative production IPC
  operation catalog. Preserve their code and evidence; generating solver
  internals is outside structural-contract migration.
- `step_topology_hierarchy.h` and `step_topology_recovery.h` expose native value
  models independently of their currently unavailable wire declarations.
- Model/status/version/SHA utilities, allocation/free functions, frame codecs,
  Worker hosts and process supervision are support APIs. They retain their
  explicit ownership/lifecycle contracts; they are not geometry jobs.
- Python `run_batch`, `GeometerBatchRunner` and batch dataclasses compose the
  legacy batch file protocol. CLI `init-request` is a request-file authoring
  utility. Their file schema/alias normalization belongs to wave 3.
- TypeScript `.`, `./contracts`, `./analytic-packet-a0`, `./ipc-a0`,
  `./ipc-client-a0`, `./illustrated-hlr`, `./mesh-illustration`,
  `./node-process-a0`, `./wasm`, `./worker`, and `./worker-host` export
  mappings are owned by the package manifest. Root wildcard re-exports do not
  create new semantic operations.
- Rust exposes generated contracts, model-bounds/HLR request conveniences,
  analytic and indexed-mesh codecs, IPC and its client. Python's public
  `__all__` and TypeScript/Rust export sources remain review boundaries, not
  evidence that every client implementation is generated.

## Drift Gate And Migration Acceptance

`node scripts/check-public-surface-docs.mjs` compares the reviewed source
inventory and normalized source digests, detecting additions, removals and
changes in public headers, client sources, export manifests and CLI/registry
boundaries. Existing promotion tests additionally check C ABI/WASM exports
and dispatch aliases. This intentionally conservative gate may also request
review for an implementation-only change; a hash is not proof of semantic
documentation completeness.

After inspecting the changed API against this map and its normative reference,
explicitly refresh the lock with `--refresh` and commit the reviewed document
and lock together. Normal HTML generation never updates the lock. Generated
language internals remain covered by the contract freshness pipeline instead.

The migration roadmap covers all inventoried user-facing families above.
Each future migration must still compare fields, aliases, presence/default
semantics, error behavior and exact packet bytes. Structural-only declarations,
opaque payloads and public native helper structures remain visible exceptions;
do not claim that all public operations are already TypeSpec-generated.

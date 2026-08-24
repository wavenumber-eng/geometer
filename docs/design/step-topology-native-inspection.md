# Native STEP Topology Inspection Research

Status: experimental native value API; not a promoted wire contract

Date: 2026-08-22

## Outcome

Geometer now has a contained native C++ STEP/XCAF document-session layer in
`geometer/step_topology_session.h`. It reads exact STEP bytes from an in-memory
stream, retains the `STEPCAFControl_Reader` work session and XCAF document, and
returns normalized research records for:

- unique product/assembly definitions;
- explicit free-root occurrences carrying source root placement separately from
  definition-local geometry;
- fully expanded component occurrence paths with accumulated transforms;
- independent bodies, shells, and faces owned by simple definitions;
- applicable XCAF names, colors, layers, material assignments, validation
  properties, and `TDataStd_NamedData` summaries; and
- opt-in, model-local STEP transfer-map evidence.

The C++ value structures remain behavioral research evidence rather than a
schema promise. The generated experimental surface now routes native executable
open, paged inspect, render, hit-resolution, close, logical-group/probe
mutation, journal checkpoint, and exact-source journal restore operations.
Those operations remain absent from the portable C ABI and browser/WASM runtime.

## Identity And Resolution

Each open document receives a 256-bit operating-system CSPRNG secret, an opaque `gts_...`
session token, and generation 1. Every definition, occurrence, body, shell, and
face receives a separate opaque `gtt_...` token derived from the session secret,
generation, target kind, and a monotonic counter. The token does not encode an
XCAF label, STEP entity number, or traversal index.

Resolution checks the owning live session registry and current generation.
Forged, stale-generation, cross-session, closed-session, expired, evicted, and
pre-process-replacement handles fail closed. `refresh()` is the current
generation boundary: it rebuilds the normalized snapshot atomically, advances
the generation, and invalidates all prior target tokens. Later apply/render
operations must use the same registry rather than parsing tokens.

Definitions and occurrences are intentionally distinct. Each free shape gets
an explicit root occurrence, including an identity root, so a STEP source-root
placement is never silently baked into definition-local body/face properties.
The definition graph is visited once with its root location stripped, while
component occurrence paths recursively expand every use of a repeated
subassembly and reference their root/parent occurrence. Body/shell/face tokens
remain definition-local; a render selection will later pair them with an
occurrence context.

## Lifecycle

| Event | Current behavior |
| --- | --- |
| Open | Validate source/limits, hash exact submitted bytes, import from an in-memory stream, build one bounded snapshot, then publish the session atomically |
| Inspect | The value API can return a full snapshot copy. Native IPC pages the immutable retained snapshot without copying it wholesale; source-entity evidence is opt-in and diagnostic carriers are explicitly unsupported on the wire |
| Refresh | Atomically rebuild at generation + 1; old target handles become invalid only after success. A failed or cancelled refresh clears its output and preserves the prior generation and handles |
| Close | Destroy the source bytes, OCCT reader/work session, XCAF document, snapshot, and handle registry |
| Expiry | `StepTopologySessionStore` removes sessions at the configured inactivity timeout |
| Eviction | The store removes least-recently-used sessions before exceeding session-count or estimated-resident-byte limits and reports their session tokens |
| Worker replacement | In-process `clear_for_process_replacement()` supports orderly replacement; a deadline/cancel kill destroys the whole worker. The supervisor starts a new process generation, so every old session and target token remains invalid |
| Restart/restore | Native IPC can reopen the exact original STEP bytes plus a bounded edit journal under explicit source, B-rep, inventory, OCCT-version, and transaction-count preconditions. It always issues a new session identity. No GLB can restore authoring state; changed-source recovery and general XBF/XML restore remain later runtime work |
| Render | Native IPC emits a bounded GLB while retaining one memory-accounted authoritative render artifact per live session; a new render supersedes the prior artifact |
| Mutation/checkpoint | Native IPC routes atomic logical-group and metadata-probe transactions and emits a source-bound edit-journal attachment. Successful mutations advance the generation and remint every topology handle |
| Hierarchy/save/export | Generated experimental shapes exist, but native process routing remains unavailable. Future implementations must preserve the same session, attachment, and atomic-publication boundaries |

The value API is serialized-worker infrastructure, not a thread-safe shared
document service. It performs no filesystem writes during import. The internal
Python `TopologyWorkerSupervisor` research layer supplies the missing hard
boundary without publishing a premature topology wire contract. Each worker
generation runs in a private temporary directory (mode 0700 on POSIX; protected by the current
user's inherited temporary-directory ACL on Windows); Windows assigns it
to a kill-on-close Job Object with process/job memory ceilings, while POSIX uses
a small exec launcher to set `RLIMIT_AS` before replacing itself with the worker
in a new process session. Deadline or explicit cancellation kills
the entire contained generation, waits for exit, removes its private directory,
and requires a new process/session generation. Geometer-owned resident
accounting remains useful for admission and LRU decisions, but the OS boundary
contains OCCT allocation that those counters cannot measure.

The containment test server opens the generated AP242 fixture through the real
native session API before deliberately hanging. Tests prove deadline kill of a
worker and its descendant, explicit cancellation, cleanup after an injected
supervisor communication failure, private-directory cleanup, a replacement
process with a different session token, and rejection of an allocation above
the OS ceiling. The native API also passes a thread-safe cancellation token into
OCCT's transfer progress range and polls it during Geometer-owned topology
indexing; worker termination remains the hard fallback for non-cooperative code.
The start-gate handshake prevents worker code from beginning before Windows Job
Object assignment completes. A later TypeSpec operation/process adapter may
reuse this behavior; this test protocol is not a public or durable contract.

## Limits And Accounting

`StepTopologyLimits` rejects oversized source bytes before import and bounds
definitions, component labels, expanded occurrence paths, bodies, shells,
faces, handles, transfer-index shapes, transfer entity/subshape work items,
individual strings, aggregate strings, active sessions, and estimated resident
bytes. Transfer indexing charges every scanned model entity and every visited
result subshape, including repeats, and fails the inspection atomically at its
own limits; it never silently truncates and then reports a false negative.

The session information reports aggregate accounted string bytes. That total
includes normalized inspection strings plus live logical-group authored ids
and names. The snapshot's normalized B-rep evidence digest excludes
tessellation and is intended for change detection and recovery evidence only;
it is not a persistent target identifier.
Resident-byte arithmetic saturates on overflow and
conservatively accounts vector and string capacity, nested collections, retained
source bytes, normalized records, and handle-map nodes/buckets. It is an
admission/accounting estimate, not a measurement of all OCCT/XCAF heap
allocations; hard process memory containment remains required.

Native GLB rendering adds a separate aggregate transient ceiling. After the
candidate render artifact is charged, the encoder progressively accounts its
BIN, JSON stacks, layout metadata, old-plus-new source reallocation peaks, and
destination GLB while enforcing the independent final-wire ceiling. Source
allocations are admitted through a shared budget allocator before system
allocation; the once-reserved destination is preflighted against all live
source storage. The store derives that transient ceiling from currently
available resident allowance without releasing the prior retained artifact, so
failure is atomic.

Native IPC exposes a precomputed, bounded record count and pages no more than
1,024 combined target/membership-edge records at a time. Its continuation
cursor can resume within a high-degree membership list, so a body or shell
cannot force an oversized response. The optional compact topology-table
attachment remains deferred.

## STEP Transfer-Map Evidence

The inspector tries three increasingly weak, explicitly named routes:

1. `XSControl_TransferReader::EntityFromShapeResult(shape, 3)`;
2. an exact reverse index of every recorded `ShapeResult(entity)`; and
3. an entity whose recorded result contains the target as a subshape.

For a successful mapping it records only the model-local entity number, dynamic
entity type, mapping method, and whether `ShapeResult(entity)` returns the exact
target shape. These values are evidence and repair hints, never identity.

On the generated nested AP242 fixture, OCCT 8.0.1 currently returns no mapping
for any normalized definition, body, shell, or face through these public
transfer hooks. Across the full AP203/AP214/AP242 fixture corpus, however, the
same paths produce both positive and negative results (109 targets mapped in the
current measurement). Tests preserve both cases and require positive evidence
to include a model-local entity number, dynamic type, and mapping method. Direct
AP242 entity-graph inspection and OCCT's internal `FindEntities` pattern
therefore remain required before source STEP provenance can be promised.

## BRepGraph Probe

The isolated `geometer_step_topology_brep_graph_test` proves that the pinned
OCCT build can:

- populate a graph from a meshed box and reconstruct valid B-rep geometry;
- expose the expected solid/shell/face topology and persistent face mesh;
- round-trip a graph UID and invalidate it after a graph-generation reset;
- register a topology-supplement layer; and
- create two placed occurrences sharing one product definition.

This establishes availability, not adoption. XCAF remains the session/product
source of truth until BRepGraph-to-XCAF correspondence, metadata persistence,
and exact benefits are measured. BRepGraph does not block the inspection API.

## Validation

The native tests cover explicit source-root placement, nested repeated
occurrences, composed rotation and translation, flat multi-solid definitions,
exact body/shell/face membership counts and separately paged relationship edges, material definition and assignment
round-trip, metadata summaries,
opt-in diagnostics and source evidence, exact source hashing, all target kinds,
OS-random session-token uniqueness, forged and cross-session handles,
generation refresh, cancelled-open rejection, failed-refresh rollback, close,
cardinality and transfer-index limits,
session/store-byte admission, access-sensitive LRU eviction, inactivity expiry,
process-replacement invalidation, exact-journal replay, replay-precondition
failure before store eviction, fresh restored identity, and the isolated
BRepGraph capability probe. The TypeScript integration additionally performs a
real Three.js raycast, mutation/checkpoint, process replacement, exact restore,
and post-restore group/probe edits.

Process-containment validation is separate because it needs the native test
worker executable:

```powershell
cmake --build build --config Release --target `
  geometer_step_topology_worker_test_server
uv run pytest tests/python/test_topology_worker_supervisor.py -q
```

```powershell
cmake --build build --config Release --target `
  geometer_step_topology_session_test geometer_step_topology_brep_graph_test
ctest --test-dir build -C Release -R geometer_step_topology --output-on-failure
```

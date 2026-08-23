# STEP Topology Contract Slice A

Status: unpromoted experimental structural candidate; not runtime-advertised

Date: 2026-08-22

## Purpose And Authority

Slice A gives the native STEP/XCAF research a generated, bounded vocabulary for
future process and TypeScript adapters. It covers open, close, paged inspection,
GLB rendering, and renderer-neutral hit resolution. It does not expose those
operations through the generic C ABI or executable IPC yet.

The authored source is
`src/tsp/geometer/operations/step-topology-a0.tsp`; the operation declarations
are in `step-topology-operation-a0.tsp`. TypeSpec owns only this candidate wire
structure. The focused C++ session, render-artifact, GLB, and hit-resolution
implementations remain behavioral authority. The promotion manifest records
every root and operation as `experimental_candidate`.

This is greenfield Geometer research. It does not reuse the legacy
`appz/data_models` GLTF enrichment vocabulary and is not the eventual
carrier-neutral annotation contract.

## Lifecycle And Identity

Every operation after open carries both the opaque session handle and the
positive session generation. Target handles identify definitions,
occurrences, bodies, shells, or faces only inside that live generation. Render
and GLB artifact handles are separate identities. None is durable across
refresh, close, eviction, worker replacement, restart, or export/reload.

The generated codecs enforce closed objects, fixed handle/hash byte lengths,
positive generations, bounded indices, bounded strings and arrays, and exact
literal schema/attachment values. They deliberately do not interpret token
prefixes or cryptographic seals. Native lookup verifies the session secret,
generation, artifact digest/seal, exact indices, and claimed
occurrence/body/face relationship. This prevents language projections from
implementing weaker copies of session security.

The normalized inspection result separates definitions, root occurrences,
component occurrences, bodies, shells, and faces. Component occurrences carry
their parent and positive depth; root occurrences cannot accidentally acquire
those fields. Body/shell/face ownership is plural and reciprocal because an
OCCT subshape may participate in more than one containing shape. Definitions,
bodies, shells, and faces may carry the same source-entity evidence shape,
including the source model number, entity type, mapping method, and whether the
shape-result relationship round-trips.

A page contains no more than 1,024 records of each kind and has an opaque
continuation cursor. Counts describe the complete snapshot. Verbose
source-entity evidence and diagnostics are explicit request choices. Large
normalized data may move through the optional compact topology table once that
table format is separately specified; JSON remains bounded by the existing 8
MiB contract limit.

Whole-snapshot validation is stateful. The generated TypeScript helper exposes
a session/generation-bound inspection accumulator that accepts every page
through the terminal page. It retains target handles and relationships across
page boundaries, rejects changed counts or repeated cursors, and only at the
terminal page accepts exact counts, complete definition and occurrence
parentage, non-dangling topology edges, definition agreement, and reciprocal
body/shell/face membership. The one-call helper is only for terminal
single-page snapshots and rejects a continuation cursor.

The accumulator rejects a declared count as soon as any record category
exceeds it, and every nonterminal page must add at least one target record.
This bounds retained cursors and records by the declared snapshot counts.
Definition cardinalities are accumulated incrementally and occurrence depths
are memoized during a color-marked parent-graph walk, keeping terminal
validation linear in records and membership edges. Component depth is capped
at 64, matching native traversal containment.

All Slice A JSON counters, byte lengths, generations, and indices are bounded
`uint32` values. This is intentional: JavaScript `number` represents that
entire integer domain exactly, so generated TypeScript codecs do not imply a
`bigint` value while parsing ordinary JSON numbers.

## Attachment Contract

Binary content is never base64 JSON:

| Operation | Direction | Attachment | Required | Limit |
| --- | --- | --- | --- | ---: |
| open | input | `step` | yes | 256 MiB |
| inspect | output | `topology_table` | no | 128 MiB |
| render | output | `glb` | yes | 256 MiB |
| render | output | `topology_binding_table` | no | 128 MiB |

The JSON result descriptors use literal names, media types, and format
identities so an attachment descriptor cannot silently describe a different
slot. Transport adapters must additionally verify that every required external
attachment exists exactly once, that no undeclared attachment is present, and
that byte length and SHA-256 match its descriptor. The compact table formats
are reserved identities, not implemented formats in Slice A.

## Render And Hit Shape

The render request fixes tessellation deflections, relative/parallel policy,
and the 3x4 source-to-render transform. The result binds a sealed GLB artifact
to the originating session generation and records definition mesh, occurrence
instance, primitive, and triangle cardinality. The required GLB descriptor is
distinct from the sealed native render artifact descriptor.

A hit request returns the artifact handle and GLB byte digest, instance,
primitive, primitive-local triangle, and the occurrence/body/face claims read
from the loaded GLB extras. The native `resolve_glb_hit` path treats all of
those fields as untrusted and returns the authoritative native triangle and
targets only after exact validation.

## Candidate Availability

The normalized catalog contains the five operations with
`runtime_available: false`. Generated TypeScript metadata exposes the
candidate models and that availability flag for research consumers. Generated
C++, Rust, and Python expected runtime catalogs, and the native generic
operation catalog, omit the operations. The TypeScript WASM catalog checker
likewise ignores structurally known operations that are not runtime available.
This keeps generated DTO/codecs useful without claiming a callable transport.

## Validation

Twenty-one Slice A governed vectors are recorded explicitly in the promotion
manifest: eleven strict/schema vectors and ten semantic vectors. They prove canonical C++
and TypeScript hit-request round-trip; rejection of unknown fields, zero
generations, and out-of-range indices; stale-session rejection; global target
handle uniqueness across pages; complete cross-page definition, occurrence,
and topology relationships; reciprocal plural membership; positive and
negative source-entity evidence; bounded page progress/counts; the native
maximum occurrence depth; exact resolve-result maxima; and exact
external attachment name, media type, byte length, SHA-256, and artifact/GLB
digest relationships. Python and Rust receive the same generated strict
structural projection; C++ and TypeScript own the additional semantic vector
oracles.

The semantic corpus includes a 4,096-body shared-shell high-fan-in snapshot
split across four pages. Reciprocity uses membership sets built once per
record, so both directed edge checks are constant-time lookups rather than
repeated scans of high-degree arrays.

Native tests separately prove unique target issuance, stale and cross-session
generation rejection, artifact/attachment digest mismatch, forged target
relationships, and exact triangle resolution. Duplicate normalized target ids
cannot be created by the native snapshot builder; consumers must still treat a
duplicate in any future compact table as a fatal whole-artifact error.

Run the focused contract checks with:

```powershell
npm run check:contracts
node tests\typescript\contract_codec_validation.mjs
uv run pytest tests\python\test_generated_contracts.py `
  tests\python\test_contract_promotion_manifest.py -q
```

The native behavior remains covered by
`geometer_step_topology_session_test` and
`geometer_step_topology_render_binding_test`.

## Deferred Surface

Slice A does not define apply/edit commands, logical groups, hierarchy,
save/export, XBF/XML, AP242 persistence, restart restore, edit journals, or
recovery evidence. Those structures are admitted only after their respective
native evidence slices stabilize.

# Analytic Planar Boolean A0 Interface

The current logical and failure contract is retained here. Historical solver
architecture, benchmarks and abandoned promotion gates are preserved in the
[research record](../research/analytic/analytic-planar-boolean-history.md), not
as production requirements. The [packed specification](analytic-planar-boolean-packet-a0.md)
remains the byte-layout authority.

## Status

**Experimental; not production-ready.** The implementation and public client
surface are retained for research, diagnostics, and compatibility, but callers
must expect valid jobs to fail closed at numeric, topology, carrier, or resource
boundaries. Do not use this operation as the dependable path for combining an
entire PCB, copper layer, or similarly large artwork set into one analytic
entity. Prefer Geometer's Clipper2-backed planar operations when polygonized
output is suitable. [ADR-017](../geometer/adr/geometer-adr-017-retain_analytic_planar_boolean_as_experimental.md)
governs this current maturity decision.

The logical TypeSpec shapes and packed A0 record layout remain a frozen
candidate. Their availability and wire stability do not constitute production
support. Existing center-form
bytes stay compatible while A0 adds an endpoint/radius authored arc variant
whose exact center may be non-integral. The solver and numeric policy are governed under
[ADR-013](../geometer/adr/geometer-adr-013-filtered_resolution_bounded_planar_boolean.md) for a
speed-first filtered implementation with a 50 nm general topology-resolution
envelope and one reported 1 um same-width union-capsule input recovery.
The arbitrary-precision algebraic engine is explicitly non-primary. MATZ
accepted the original
consumer/provider design input, and independent review approved the analytic
architecture at normative revision
`529c768e559b4c88874264748d4186e775c8a4dd`. The separately compiled TypeSpec
candidate and its logical/packed reconciliation were accepted at
`f4b6a9b87bf16f57ef29dae22150b16f2a742b64`; it remains a frozen candidate in
the shared generated projections. ADR-017 ends the former MATZ production-
promotion direction.

The current contract authorizes raw packet goldens and generated codec work.
The former MATZ visualization gates are retained in the research record as historical evidence,
not as an active production-promotion plan. ADR-012 records
the historical exact-first feasibility decision; ADR-013 supersedes it for
the filtered implementation architecture. References to “production” in those
historical sections describe the intended execution path and qualification
target at the time; they do not override the experimental status above.

The endpoint/radius authored variant is admitted for closed planar-region
rings. Constant-width swept-path centerlines retain line and exact
integer-center arc segments in A0; endpoint/radius swept offsets are a separate
future geometry capability.

The filtered solver currently certifies coincident endpoint/radius circle
carriers only when they use the same canonical unordered endpoint pair,
radius, and selected center side. Equivalent circles described by different
endpoint pairs, or by mixed center-form and endpoint/radius-form arcs, may fail
closed with the governed `resource_limit_exceeded` job diagnostic under
ADR-013. They are never approximated or omitted from a completed result. The former
MATZ real-board gates are historical, not production acceptance criteria;
scalable general carrier canonicalization is
tracked in [Geometer issue #21](https://github.com/wavenumber-eng/geometer/issues/21).

The stable operation identity is
`geometry.analytic_planar_boolean_batch.a0`. Friendly generated methods are
`analyticPlanarBooleanBatch` in TypeScript and
`analytic_planar_boolean_batch` in Python and Rust.

## Authority And Scope

Geometer owns the generic logical contract, packed attachment format,
normalization, provenance representation, diagnostics, solver, native/WASM
implementation, and release. The MATZ observation manifest is a governed
consumer input, not a proposed TypeSpec or binary schema.

A0 supports:

- independent ordered Boolean jobs in one batch;
- `union` and `difference` stages, with operands within a stage treated as an
  unordered set;
- topology-indexed line/circular-arc planar regions;
- disks, annuli, capsules, and constant-width line/circular-arc swept paths
  with round caps and joins;
- authoritative analytic result topology, holes, islands, provenance, and
  generic contact relationships; and
- native, full-browser WASM, and executable IPC execution through the generic
  operation/attachment transports.

A0 does not support ellipses, elliptical arcs, Beziers, splines, arbitrary
offset policy, PCB nets, layers, conductive domains, or application source
keys. Unsupported geometry fails its isolated job. It is never silently
sampled into lines. Existing sampled planar and Clipper2 operations remain
unchanged.

## Logical Request

TypeSpec governs the logical request even though the hot wire projection is a
packed attachment. The normalized catalog must expose both the semantic model
and its attachment projection so generated clients do not build JSON geometry.
The candidate entrypoint is `src/tsp/geometer/analytic-candidate.tsp`; it emits
the logical request/result identities and marks both operation projections as
`packed_attachment` using format
`geometry.analytic_planar_boolean.packet.a0`. The production TypeSpec
entrypoint remains unchanged until this candidate and the packet freeze are
accepted.

### Identities

All caller identities are packet-local, nonzero unsigned 64-bit integers.
Separate declared spaces exist for jobs, stages, operands, authored regions,
rings, paths, vertices, segments, curves, compact features, and relationship
queries. Zero is reserved. Duplicate ids, missing references, and out-of-range
values fail batch structural validation before geometry execution.

TypeScript uses `bigint` and `BigUint64Array`, never `number`, for these
identities. Python uses a bounded `int` codec. Ids are lifting tokens; they are
not stable application keys and do not enter MATZ hashes.

### Batch And Jobs

The semantic request is:

```text
AnalyticPlanarBooleanBatchRequest
  jobs: AnalyticPlanarBooleanJob[]
  relationshipQueries: PlanarRelationshipQuery[]

AnalyticPlanarBooleanJob
  jobId: JobId
  stages: AnalyticPlanarBooleanStage[]

AnalyticPlanarBooleanStage
  stageId: StageId
  operation: union | difference
  operands: AnalyticPlanarOperand[]
```

The accumulator begins empty. Stages execute in request order. Operands within
one stage are a mathematical set and are canonicalized by operand id before
solver submission. Let `R(S) = closure(interior(S))` in the Euclidean plane.
For the current regularized closed material `A` and the union `U` of all closed
operand sets in a stage, union computes `R(A union U)` and difference computes
`R(A set-minus U)`. Boundary-only remnants and zero-area components are not
material. A zero-operand union or difference is a successful no-op. Difference
from an empty accumulator is a successful no-effect stage. Complete
subtraction is a successful empty result.

### Geometry

Every coordinate is a signed integer nanometer. Positive lengths are unsigned
integer nanometers. A0 closed-ring arcs use topology endpoints plus either an
exact integer center or an exact positive integer radius, together with an
explicit direction/major-arc branch; the contract carries no angle or
trigonometric field. Open swept-path centerlines admit only line and exact
integer-center arc segments. Consumer fixture shorthand may describe a source
sweep in microdegrees, but the imported conformance vector freezes its expanded
integer vertices and governed arc construction before packet encoding.

```text
PointNm
  x: int64
  y: int64

PlanarRing
  ringId: RingId
  vertices: AuthoredVertex[]
  segments: AuthoredSegment[]
  invariant: segments.length == vertices.length

PlanarPath
  pathId: PathId
  vertices: AuthoredVertex[]
  segments: AuthoredPathSegment[]
  invariant: segments.length + 1 == vertices.length

AuthoredSegment
  segmentId: SegmentId
  curveId: CurveId
  geometry: line | CircularArcByCenterDescriptor | CircularArcByRadiusDescriptor

AuthoredPathSegment
  segmentId: SegmentId
  curveId: CurveId
  geometry: line | CircularArcByCenterDescriptor

CircularArcByCenterDescriptor
  center: PointNm
  direction: cw | ccw
  majorArc: bool

CircularArcByRadiusDescriptor
  radiusNm: uint64
  direction: cw | ccw
  majorArc: bool
```

A topology-indexed authored arc may not be a full circle. Center form requires
the exact integer squared distances from its center to both topology endpoints
to be equal and nonzero. Radius form requires a nonzero chord no longer than
the diameter; its topology endpoints, radius, direction, and major-arc branch
select the unique authored arc even when its center is not integral. Full
circles use compact disk/annulus features. Radius form is not admitted in an
open `PlanarPath`.

The operand union is:

```text
PlanarRegionOperand
  regionId, outer ring, hole rings

DiskOperand
  featureId, center, radius

AnnulusOperand
  featureId, center, innerRadius, outerRadius

CapsuleOperand
  featureId, start, end, width

SweptPathOperand
  featureId, centerline path, width
  A0 cap/join policy: round
```

Rectangle fixture shorthand lowers to a four-line planar region. An arc sweep
lowers to a one-arc swept path. Compact primitive boundary roles are derived
features of the compact source; clients are not required to serialize a
lowered boundary ring. Result provenance may refer to a compact feature plus a
canonical boundary role. Authored ring/path segments retain their caller ids.

Input planar regions must be individually valid and non-self-intersecting.
Each ring has at least two analytic fragments and encloses positive area; every
hole is strictly inside its outer ring, hole interiors are pairwise disjoint,
and hole boundaries neither touch nor cross another ring. Nested islands are
represented as separate operands rather than nested holes in one authored
region. A planar-region operand denotes the regularized closed outer set minus
the union of its hole interiors. Any ambiguous or invalid containment fails the
job. Input winding is accepted in either direction; canonical result winding
is fixed below. Disk radius is positive. An annulus requires
`0 < innerRadius < outerRadius`.

Capsule endpoints must be distinct and width must be positive; its set is the
segment Minkowski-summed with a closed disk of radius `width / 2`. A swept path
must contain at least one nonzero segment, may not retrace or self-intersect,
and consecutive segments may not reverse through an exact 180-degree cusp.
Every circular centerline segment must have radius strictly greater than half
the sweep width so both analytic offsets remain positive-radius curves. The
regularized swept area is the Minkowski sum of the accepted centerline with a
closed disk of radius `width / 2`; overlap between nonadjacent parts of that
swept area is allowed and is resolved by the filtered indexed arrangement.
Inputs outside these A0 rules fail the isolated job with `invalid_topology` or
`invalid_arc`.

### Relationship Queries

An A0 query names two request-known job ids:

```text
PlanarRelationshipQuery
  queryId: QueryId
  leftJobId: JobId
  rightJobId: JobId
```

Only listed pairs are evaluated. A0 does not imply all-pairs work and does not
accept solver-generated result-region ids in the one-pass request.

## Resolution-Bounded Production Normalization

The public coordinate unit and governed output grid remain one nanometer. The
separate topology-resolution envelope is 50 nm and cannot be relaxed per
request.

1. Authored values begin as bounded integers. Working intersections carry
   conservative error bounds in job-local coordinates.
2. Fast-path line-line, line-circle, and circle-circle construction uses
   binary64 plus outward bounds. Fixed-width wide integers certify applicable
   orientation, incidence, squared-distance, and threshold predicates without
   building symbolic roots.
3. Nearest-nanometer, ties-away-from-zero rounding remains deterministic. A
   bounded uncertainty that cannot establish one rounding or topology outcome
   takes the governed slow path or returns `resource_limit_exceeded` for that
   job.
4. Features, gaps, and separations at or below 50 nm may be deterministically
   merged, bridged, shortened, or collapsed. A separation or feature width
   proven greater than 50 nm must retain its topology. Threshold fixtures at
   49, 50, and 51 nm govern this boundary.
5. Every published vertex must remain within 50 nm of its resolved analytic
   position, and every published line/arc boundary must have Hausdorff
   displacement no greater than 50 nm. A normalized radius may move by no more
   than 50 nm, subject to the same whole-arc Hausdorff bound.
6. Distinct required vertices separated by more than 50 nm may not share one
   representative. Above that threshold a fragment may not collapse, cyclic
   order may not invert, and containment may not change.
7. The retained exact arc checker should compare squared distances against
   squared tolerances so ordinary irrational endpoints do not force a nested
   outer square root. This is an oracle/fallback repair, not a reason to make
   the algebraic engine primary again.

Full result circles still use the canonical two-half-arc representation below.

## Canonical Result

A successful job returns an analytic planar arrangement:

```text
AnalyticPlanarBooleanJobResult
  jobId
  status: success
  diagnostics
  vertices
  directedFragments
  rings
  resultRegions
  inline source-set values (interned by the packed projection)
  operandOutcomes
  digestSha256 (derived from the canonical standalone job-result packet)

DirectedFragment
  start/end vertex references
  line, or circular arc with radius, direction, and major-arc branch
  coincident positive source set
  surviving subtractive-effect source set

ResultRegion
  generated resultRegionId
  outer ring
  positive contributor source set
```

Generated result ids are deterministic one-based uint64 ordinals in separate
result spaces and are meaningful only within that result packet.

Canonicalization is independent of OCCT traversal and allocation:

- vertices sort by `(x, y, incident analytic signature, complete intersection
  source-set tuple sequence)`. The incident analytic signature is the
  lexicographically sorted sequence of
  `(incidenceSide, otherEndpointX, otherEndpointY, kind, direction, majorArc,
  radius)` for every semantically interned retained fragment touching the
  vertex, where `incidenceSide` is `0` for fragment start and `1` for fragment
  end. It contains no generated id or source-set handle;
- fragments are local analytic records and sort by `(start vertex key, end
  vertex key, kind, direction, majorArc, radius, coincident-positive source-set
  tuple, surviving-subtraction source-set tuple)`; line records use direction
  `not_applicable`, `majorArc = false`, and radius zero;
- an outer ring is CCW and a hole ring is CW;
- each ring rotates to the lexicographically least vertex/outgoing-fragment
  key;
- containment children sort by their canonical ring key;
- ring parent/depth is the sole containment authority: parent links are
  acyclic, name the smallest strict containing ring, and hole parity equals
  odd depth; every even-depth ring is the outer ring of exactly one result
  region and odd-depth rings belong to none;
- maximal interior-connected area components become distinct result regions,
  so point-tangent areas are separate regions sharing a vertex;
- shared-edge unions remove the internal seam;
- result regions sort by outer-ring key;
- source sets contain unique sorted source references, sort lexicographically
  by the complete reference tuple sequence, and are interned by full content;
  an optional internal hash never affects ids or bytes; and
- diagnostics, operand events, and relationship pairs sort by governed keys.

Canonical job-result records exclude enclosing batch offsets/indices,
relationship results, wall time, peak memory, and other telemetry. The same job
therefore has identical canonical record bytes/digest alone or in a mixed
batch.

The logical `digestSha256` field is required on successful and failed job
results, but it is not stored in the 48-byte packed job-result record. A
decoder constructs the canonical standalone job-result packet using the closure
and rebasing algorithm in the packet specification, computes SHA-256 over those
bytes, and exposes the lowercase hexadecimal digest as this deterministic
derived field. Encoders verify a supplied logical digest against the same bytes
and never serialize it as an independent value. Logical source-set values are
inline at their use sites; the packed projector interns their complete content
into the source-set and source-reference tables without changing their logical
meaning. Projection in the other direction is bounded by a batch-wide maximum
of 1,048,576 logical source-reference expansions. The checked count sums a
set's members at every vertex, fragment source-set field, region, and operand
event that exposes it, including repeated uses of one interned packed handle.
Native and WASM enforce the same bound before publishing a successful packet,
so every production result remains projectable by the generated clients.

## Provenance And Outcomes

Positive lineage is coverage-based and stage-aware. Union adds the positive
source to covered material even when material was already covered. Difference
removes all positive lineage from removed cells. A later union into the empty
area begins fresh lineage. A subtraction later filled does not own the restored
boundary, but remains history evidence.

The result exposes:

- exact many-to-many positive-operand/result-region associations computed from
  actual surviving lineage;
- all coincident source curves/features for each surviving boundary fragment;
- surviving subtractive operands responsible for outer/hole fragments;
- all source segments/features forming each new intersection vertex; and
- nonexclusive, canonically sorted outcome events for each operand.

Outcome events include:

- `contributes_final_material` with result-region references;
- `redundant_or_absorbed_coverage`;
- `partially_removed_later`;
- `completely_removed_later`;
- `subtraction_effect_survives` with every surviving boundary/result reference,
  or an empty reference range for unfilled complete subtraction with no final
  material boundary;
- `subtraction_effect_overwritten_later`; and
- `no_effect`.

These are not forced into one enum. A source may have several simultaneous
events. A0 does not serialize contributor-coverage subregions.

Their truth conditions are exact and use positive-area arrangement cells;
boundary-only contact does not count as material effect:

- `contributes_final_material` exists for a union operand iff at least one
  final present cell carries that operand's positive lineage. Its references
  are exactly the result regions containing such cells.
- `redundant_or_absorbed_coverage` exists iff a union operand covered positive
  area at its stage but some or all of that area was already covered by the
  pre-stage accumulator or another operand in the same unordered stage. It may
  coexist with `contributes_final_material`.
- `partially_removed_later` exists iff an operand contributed positive-area
  material immediately after its union stage and later differences remove some
  but not all of its lineage-bearing area before the final result.
- `completely_removed_later` exists iff such material existed immediately
  after insertion and none of its lineage remains in final material. It is
  mutually exclusive with `contributes_final_material` and
  `partially_removed_later`.
- `subtraction_effect_survives` exists iff a difference operand removes
  positive area that remains unfilled in the final result. Its references are
  exactly the final boundary fragments that separate its attributed removed
  set from material. Still-unfilled removal can have no attributable final
  boundary after complete subtraction or when only another disconnected
  removed component was refilled. In either case this event has an empty
  reference range rather than being omitted. It may coexist with
  `subtraction_effect_overwritten_later` when only part of the attributed
  removed area was restored.
- `subtraction_effect_overwritten_later` exists iff positive area removed by a
  difference operand is restored by a later union. It may coexist with a
  surviving effect when only part of the removed area is restored.
- `no_effect` exists iff the operand changes neither the regularized material
  set nor any positive/subtractive lineage or history cell. It is exclusive
  with every other event.

Same-stage union operands are evaluated symmetrically against the pre-stage
accumulator and the complete same-stage union, so no authored operand order can
change these predicates.

Same-stage difference operands are likewise evaluated symmetrically, never as
a sequential subtraction. For operand `D`, its attributed removed set is
`R(interior(pre-stage material) intersection interior(D))`, independent of
every other operand in that stage. A positive-area cell covered by several
same-stage subtractors records every such operand, so coincident and partially
overlapping subtractors each receive removal/history credit for their own
attributed set. A surviving boundary fragment names every credited subtractor
whose attributed removed set lies on its removed side and whose pre-stage
material lies on its retained side. Each operand's
`subtraction_effect_survives`, `subtraction_effect_overwritten_later`, and
`no_effect` predicates are evaluated from that independent attributed set and
the later-stage history. Consequently operand-id sorting is canonical encoding
only and cannot change difference lineage.

## Relationship Result

For each selected successful job pair, Geometer returns:

- aggregate maximum intersection dimension: `disjoint`, `point`, `curve`, or
  `area`;
- canonically sorted concrete left/right result-region pairs with their
  dimension;
- symmetric equality; and
- directional containment where applicable.

For closed regularized result-region sets `L` and `R`, dimension is `area` iff
`interior(L intersection R)` is nonempty; otherwise `curve` iff their
intersection contains a nonzero-length line or circular-arc interval;
otherwise `point` iff the intersection is nonempty; otherwise `disjoint`.
Boundary contact is therefore reported as `point` or `curve`, never area.
`equality` means exact set equality. `leftContainsRight` means `R` is a subset
of `L`, and `rightContainsLeft` is the converse; these flags are non-strict, so
equality sets both. Proper containment is derived as containment without
equality and is not separately encoded. Containment and equality include
boundaries and are decided by the exact arrangement, not sampling.

An empty successful job is `disjoint` from every job and yields no concrete
pairs. A query depending on a failed job is
`skipped_dependency_failed`. Geometer does not return or interpret net/domain
policy.

## Failure Boundary And Diagnostics

Malformed generic framing and foreign-memory descriptor failures retain the
generic transport behavior. A well-formed invocation with unsupported packet
generation, bad attachment metadata, impossible packet table
bounds/counts/offsets, duplicate ids, missing/cross-space references, or
ambiguous job indexing produces a typed contract diagnostic and rejects the
whole batch before geometry. Generated clients surface that typed rejection as
`GeometerContractError`; no partial result attachment exists. Once jobs are
structurally isolated, geometry, capability, solver, resource, and
normalization failures are job-local; independent jobs continue.

The scope matrix is normative:

| Failure | Wire category/scope | Generated client |
| --- | --- | --- |
| generic framing, pointer/descriptor, transport allocation | `geometer.transport.*`, no typed operation result when the transport cannot construct one | transport/invocation exception |
| packet magic/generation/table/limit defect | `geometer.contract.analytic_planar_boolean_packet.*`, batch rejected | `GeometerContractError` |
| duplicate/out-of-range id | `geometer.contract.analytic_planar_boolean_packet.invalid_id`, batch rejected | `GeometerContractError` |
| missing/cross-space reference | `geometer.contract.analytic_planar_boolean_packet.invalid_reference`, batch rejected | `GeometerContractError` |
| valid isolated job with invalid topology/arc/capability/normalization/solver/resource outcome | operation diagnostic in failed job result | returned failed job result |
| relationship depending on failed job | `skipped_dependency_failed` relationship status | returned relationship result |
| relationship evaluation for otherwise successful jobs cannot complete within exact solver/resource rules | outer `geometer.operation.analytic_planar_boolean.solver_failed` or `.resource_limit_exceeded`; whole invocation fails and no result attachment is returned | operation exception |

A0 has no query-scoped diagnostic record. Query ids and references are
validated with the request packet before job isolation. Dependency failure has
the dedicated relationship status above; any other relationship-computation
failure is an outer operation failure, so the result packet's diagnostic table
contains job-scoped records only.

Operation diagnostic identities are namespaced strings in generated APIs and
compact governed integers in a structurally valid result packet:

- `geometer.operation.analytic_planar_boolean.invalid_topology`
- `geometer.operation.analytic_planar_boolean.invalid_arc`
- `geometer.operation.analytic_planar_boolean.unsupported_geometry`
- `geometer.operation.analytic_planar_boolean.normalization_error_exceeded`
- `geometer.operation.analytic_planar_boolean.normalization_topology_collapse`
- `geometer.operation.analytic_planar_boolean.nonanalytic_result`
- `geometer.operation.analytic_planar_boolean.solver_failed`
- `geometer.operation.analytic_planar_boolean.resource_limit_exceeded`
- `geometer.operation.analytic_planar_boolean.resolution_coalesced` (warning)

Diagnostics carry trustworthy job/stage/operand/geometry ids and an optional
governed logical path identity. Unknown or untrusted ids are omitted rather
than guessed.

The logical `pathIdentity` is the symbolic name of the packed `path_token` from
the numeric catalog. Token zero maps to absence; every nonzero token maps
one-to-one to the same-named `JobDiagnosticPath` enum member. This standalone
projection never depends on the original request, request array indexes, or an
ID-to-index lookup, so a persisted result packet always decodes identically.
Generated convenience code may combine the identity, trusted ids, and an
available request to present a navigable location, but that presentation is not
part of the canonical result DTO or bytes. Batch-rejecting outer contract
diagnostics continue to use `DiagnosticA0.path` as an RFC 6901 pointer because
they are produced while validating the request document.

## Generated Clients

TypeScript:

```ts
client.analyticPlanarBooleanBatch(
  request: AnalyticPlanarBooleanBatchRequest,
  options?: OperationCallOptions,
): Promise<AnalyticPlanarBooleanBatchResult>
```

The encoder writes directly to `ArrayBuffer`/typed arrays. Identity types are
`bigint`; compile-time and runtime tests reject `number` ids.

Python:

```python
client.analytic_planar_boolean_batch(
    request: AnalyticPlanarBooleanBatchRequest,
    *,
    timeout: float | None = None,
) -> AnalyticPlanarBooleanBatchResult
```

The encoder accepts generated models and writes bytes/memoryview without JSON
on the geometry hot path. Public convenience adapters may map existing values
into these generated types but cannot remain an independent structural
authority.

All generated suites must prove fixed uint64 round trips, strict validation,
portable fixture encoding/decoding, job-local failures, canonical job-result
digests, attachment ownership, and capability errors.

Rust exposes the same logical model and a friendly
`analytic_planar_boolean_batch` method through the persistent executable
client. Identity newtypes wrap `u64` and reject zero. Its packed codec writes
directly to byte buffers and shares every strict, malformed, canonical-byte,
diagnostic, and capability vector with C++, TypeScript, and Python. Rust is a
required production projection, not a follow-up.


## Historical Section Links

The following anchors preserve existing links without retaining research
results as interface requirements. These records do not establish production
readiness:

<a id="feasibility-evidence"></a>
- [Feasibility Evidence](../research/analytic/analytic-planar-boolean-history.md#feasibility-evidence)

<a id="production-solver-architecture"></a>
- [Production Solver Architecture](../research/analytic/analytic-planar-boolean-history.md#production-solver-architecture)

<a id="historical-exact-first-architecture-non-primary"></a>
- [Historical Exact-First Architecture (Non-Primary)](../research/analytic/analytic-planar-boolean-history.md#historical-exact-first-architecture-non-primary)

<a id="historical-exact-normalization-oracle-only"></a>
- [Historical Exact Normalization (Oracle Only)](../research/analytic/analytic-planar-boolean-history.md#historical-exact-normalization-oracle-only)

<a id="browser-packaging"></a>
- [Browser Packaging](../research/analytic/analytic-planar-boolean-history.md#browser-packaging)

<a id="performance-and-telemetry"></a>
- [Performance And Telemetry](../research/analytic/analytic-planar-boolean-history.md#performance-and-telemetry)

<a id="promotion-gates"></a>
- [Promotion Gates](../research/analytic/analytic-planar-boolean-history.md#promotion-gates)

<a id="filtered-production-gates"></a>
- [Filtered Production Gates](../research/analytic/analytic-planar-boolean-history.md#filtered-production-gates)

<a id="exact-result-replay-and-mutation-sentinels"></a>
- [Exact Result Replay And Mutation Sentinels](../research/analytic/analytic-planar-boolean-history.md#exact-result-replay-and-mutation-sentinels)

<a id="closed-form-analytic-invariants"></a>
- [Closed-Form Analytic Invariants](../research/analytic/analytic-planar-boolean-history.md#closed-form-analytic-invariants)

<a id="exact-degeneracy-boundaries"></a>
- [Exact Degeneracy Boundaries](../research/analytic/analytic-planar-boolean-history.md#exact-degeneracy-boundaries)

<a id="exact-boolean-identities"></a>
- [Exact Boolean Identities](../research/analytic/analytic-planar-boolean-history.md#exact-boolean-identities)

<a id="exact-boolean-metamorphic-relations"></a>
- [Exact Boolean Metamorphic Relations](../research/analytic/analytic-planar-boolean-history.md#exact-boolean-metamorphic-relations)

<a id="bounded-rectangle-enumeration"></a>
- [Bounded Rectangle Enumeration](../research/analytic/analytic-planar-boolean-history.md#bounded-rectangle-enumeration)

<a id="deterministic-seeded-rectangle-properties"></a>
- [Deterministic Seeded Rectangle Properties](../research/analytic/analytic-planar-boolean-history.md#deterministic-seeded-rectangle-properties)

<a id="exact-many-to-many-lineage-matrix"></a>
- [Exact Many-To-Many Lineage Matrix](../research/analytic/analytic-planar-boolean-history.md#exact-many-to-many-lineage-matrix)

<a id="occt-801-qualification"></a>
- [OCCT 8.0.1 Qualification](../research/analytic/analytic-planar-boolean-history.md#occt-801-qualification)

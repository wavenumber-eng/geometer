# Analytic Planar Boolean A0 Design

## Status

Proposed for joint Geometer/MATZ review. This document does not authorize
transport implementation or freeze generated code. The generic operation C ABI
and executable IPC remain gated by their independent review.

The stable operation identity is
`geometry.analytic_planar_boolean_batch.a0`. Friendly generated methods are
`analyticPlanarBooleanBatch` in TypeScript and
`analytic_planar_boolean_batch` in Python.

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

## Feasibility Evidence

The nonproduction CTest spike
`geometer_analytic_planar_boolean_feasibility_test` proves on OCCT 8.0.0:

- point-tangent disks remain two regularized area faces;
- shared-edge rectangles unify into one face without an internal seam;
- overlapping disks and intersecting analytic sectors extract only line and
  circle curves;
- normalized boundary signatures are independent of operand order within a
  union stage;
- circle-intersection vertices can be classified to both source curves;
- the exact MATZ arbitrary-angle, signed-sweep, round-cap case succeeds and
  retains unique nm-grid representatives within the point error bound;
- the 1.25/1.4 nm collision probe maps two distinct required vertices to one
  representative and is therefore rejectable; and
- OCCT modification/generation history alone does not retain absorbed-positive
  material lineage.

The same test passes natively and under Emscripten/Node. Its current four-line
canonical feasibility signature is byte-identical with SHA-256
`87db47536893aa98464d81be69cd4e2dd89370cfd05f8ad825d18d19021c3de4`.
This is feasibility evidence, not the eventual packet golden.

## Logical Request

TypeSpec governs the logical request even though the hot wire projection is a
packed attachment. The normalized catalog must expose both the semantic model
and its attachment projection so generated clients do not build JSON geometry.

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
kernel submission. Difference from an empty accumulator is a successful
no-effect stage. Complete subtraction is a successful empty result.

### Geometry

Every coordinate is a signed integer nanometer. Positive lengths are unsigned
integer nanometers. Compact arc-sweep helper input uses signed integer
microdegrees; normalized ring/path arcs use endpoints plus an analytic circle
and explicit direction/major-arc branch so no redundant floating angle is
required.

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
  segments: AuthoredSegment[]
  invariant: segments.length + 1 == vertices.length

AuthoredSegment
  segmentId: SegmentId
  curveId: CurveId
  geometry: line | CircularArcDescriptor

CircularArcDescriptor
  center: PointNm
  direction: cw | ccw
  majorArc: bool
```

A topology-indexed arc may not be a full circle and its endpoints, center, and
branch must be coherent. Full circles use compact disk/annulus features.

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
Outer/hole containment may be normalized, but an ambiguous or invalid topology
fails the job. Input winding is accepted in either direction; canonical result
winding is fixed below.

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

## Solver Architecture

OCCT is the selected A0 analytic kernel, subject to completion of the portable
fixture suite and performance gates. Clipper2 may be used only as a sampled
area/topology oracle; its output is never authoritative.

Each job uses a deterministic integer-nm local origin near its bounds before
conversion to OCCT millimeters. This limits double magnitude without changing
wire identities. Each stage submits its operands in canonical id order as one
Boolean set operation, regularizes same-domain output, and rejects non-line or
non-circle authoritative boundaries.

Material lineage cannot be reconstructed from OCCT boundary history alone.
The implementation maintains stage-aware arrangement cells internally:

1. split the current material and stage operands at analytic boundaries;
2. classify each cell against prior material and current operands;
3. apply union/difference to cell presence and positive lineage;
4. retain subtraction-effect history separately;
5. merge cells into final regularized result regions without serializing the
   internal lineage partition; and
6. derive exact operand-to-result associations from the lineage carried by
   cells inside each connected result.

Spatial bounds prune cell/operand comparisons. This is one in-process batch,
not a per-operand IPC or WASM bridge.

Boundary provenance combines kernel history with analytic coincidence and
incidence classification. Intersection vertices list all authored segments or
compact features whose curves form that vertex. An absorbed or coincident
positive remains material lineage even when it owns no surviving boundary.

## Normalization

The governed grid is one nanometer. Normalization is part of the operation and
cannot be relaxed per request.

1. Source integer quantities and exact rational derived quantities are retained
   as such until kernel conversion where practical.
2. A kernel scalar is rounded to the nearest integer nanometer; exact ties are
   rounded away from zero.
3. A kernel value indistinguishably close to a half-nm tie without an exact
   source/rational derivation fails with `normalization_ambiguous_tie` rather
   than choosing a platform-dependent side.
4. A normalized vertex must have squared displacement no greater than
   `0.5 nm^2` from the kernel vertex (maximum Euclidean displacement
   `sqrt(0.5) nm`).
5. A normalized derived circle center or radius may move no more than `0.5 nm`.
   A normalized arc endpoint must remain within `1.25 nm` of its normalized
   analytic circle. These are separate point, curve, and coherence bounds.
6. Distinct required vertices may not share one representative. A fragment may
   not collapse, cyclic order may not invert, containment may not change, and a
   line/circle fragment may not become incoherent.

Any violation fails the isolated job with a stable normalization diagnostic.
The MATZ 1.25/1.4 nm notch is the normative topology-collapse failure. The
arbitrary-angle arc fixture is the normative successful non-grid-intersection
case.

## Canonical Result

A successful job returns an analytic planar arrangement:

```text
AnalyticPlanarBooleanJobResult
  jobId
  status: success
  vertices
  curves
  directedFragments
  rings
  resultRegions
  sourceSets
  operandOutcomes

NormalizedCurve
  line | circle(centerNm, radiusNm)

DirectedFragment
  curve reference
  start/end vertex references
  direction and major-arc branch for circles
  coincident positive source set
  surviving subtractive-effect source set

ResultRegion
  generated resultRegionId
  outer ring
  hole/child containment
  positive contributor source set
```

Generated result ids are deterministic one-based uint64 ordinals in separate
result spaces and are meaningful only within that result packet.

Canonicalization is independent of OCCT traversal and allocation:

- vertices sort by `(x, y, incident analytic signature, source-set key)`;
- lines sort by canonical endpoints; circles sort by center/radius;
- an outer ring is CCW and a hole ring is CW;
- each ring rotates to the lexicographically least vertex/outgoing-fragment
  key;
- containment children sort by their canonical ring key;
- maximal interior-connected area components become distinct result regions,
  so point-tangent areas are separate regions sharing a vertex;
- shared-edge unions remove the internal seam;
- result regions sort by outer-ring key;
- source sets contain unique sorted source references and are interned by
  content; and
- diagnostics, operand events, and relationship pairs sort by governed keys.

Canonical job-result records exclude enclosing batch offsets/indices,
relationship results, wall time, peak memory, and other telemetry. The same job
therefore has identical canonical record bytes/digest alone or in a mixed
batch.

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
- `subtraction_effect_survives` with boundary/result references;
- `subtraction_effect_overwritten_later`; and
- `no_effect`.

These are not forced into one enum. A source may have several simultaneous
events. A0 does not serialize contributor-coverage subregions.

## Relationship Result

For each selected successful job pair, Geometer returns:

- aggregate maximum intersection dimension: `disjoint`, `point`, `curve`, or
  `area`;
- canonically sorted concrete left/right result-region pairs with their
  dimension;
- symmetric equality; and
- directional containment where applicable.

An empty successful job is `disjoint` from every job and yields no concrete
pairs. A query depending on a failed job is
`skipped_dependency_failed`. Geometer does not return or interpret net/domain
policy.

## Failure Boundary And Diagnostics

Malformed generic framing, unsupported protocol/packet generation, bad
attachment references, impossible table bounds/counts/offsets, duplicate ids,
or ambiguous job indexing reject the batch. Once jobs are structurally
isolated, geometry, capability, solver, resource, and normalization failures are
job-local; independent jobs continue.

Operation diagnostic identities are namespaced strings in generated APIs and
compact governed integers in the packet:

- `geometry.analytic_planar_boolean.invalid_id`
- `geometry.analytic_planar_boolean.invalid_reference`
- `geometry.analytic_planar_boolean.invalid_topology`
- `geometry.analytic_planar_boolean.invalid_arc`
- `geometry.analytic_planar_boolean.unsupported_geometry`
- `geometry.analytic_planar_boolean.normalization_ambiguous_tie`
- `geometry.analytic_planar_boolean.normalization_error_exceeded`
- `geometry.analytic_planar_boolean.normalization_topology_collapse`
- `geometry.analytic_planar_boolean.nonanalytic_result`
- `geometry.analytic_planar_boolean.solver_failed`
- `geometry.analytic_planar_boolean.resource_limit_exceeded`

Diagnostics carry trustworthy job/stage/operand/geometry ids and a generated
logical path when available. Unknown or untrusted ids are omitted rather than
guessed.

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

Both generated suites must prove fixed uint64 round trips, strict validation,
portable fixture encoding/decoding, job-local failures, canonical job-result
digests, attachment ownership, and capability errors.

## Browser Packaging

The reviewed generic ABI specification already assigns the generic functions
to the full browser/Web Worker target. A0 therefore uses the full OCCT browser
artifact initially. It does not expand the legacy lightweight planar-only
artifact and does not change the locked transport-review packet. The generated
high-level client negotiates the operation capability and hides the artifact
factory details.

## Performance And Telemetry

Correctness takes precedence. The initial real-board design target is at most
5 seconds and 1 GiB peak working memory per all-copper batch on the recorded
reference machine; the stretch target is 1 second and 512 MiB.

Wall time, peak memory, operand/input-segment/result-region/result-segment
counts, and normalization-failure count are noncanonical telemetry. They are
reported outside canonical job-result bytes.

## Promotion Gates

Before design freeze:

- MATZ must ratchet the arbitrary-angle fixture from provisional to successful;
- this design, the packet specification, diagnostic assignments, capability
  table, and generated API shapes must receive joint review;
- portable logical fixtures must be imported into Geometer with digests and
  structural expectations; and
- raw-byte goldens must be generated only after the packet layout freezes.

Before release, Geometer must pass native/full-browser/executable parity,
generated TypeScript/Python consumption, malformed/resource tests,
documentation generation, native/WASM/package/Rack/L99 gates, and candidate
MATZ integration. MATZ switches production only after pinning the additive
tagged release and passing its real-board suite.

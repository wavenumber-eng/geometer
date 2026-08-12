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

The same test passes natively and under Emscripten/Node. Its current five-line
canonical feasibility signature, including the twelve exact case-2
endpoint/radius/direction/branch fragments, is byte-identical with SHA-256
`c21b03c1b42a6cb3212cec5b3051987f645e21062eddecc82d3e3b0e0fd6dfc7`.
This is feasibility evidence, not the eventual packet golden.

The case-2 success oracle is the following lexicographically sorted fragment
list. `A:radius:direction:branch:start:end` uses integer nanometers:

```text
A:2600000:cw:minor:4213333,2299527:5560500,451485
A:2600000:cw:minor:643600,1098807:2080000,2431789
A:3200000:cw:minor:-3007016,1094464:540000,3154108
A:3200000:cw:minor:2080000,2431789:3007016,1094464
A:4000000:ccw:minor:540000,3154108:-625231,1690473
A:4000000:ccw:minor:6939231,694593:2673333,3986639
A:4800000:ccw:minor:2673333,3986639:-4510525,1641697
A:4800000:ccw:minor:4510525,1641697:4213333,2299527
A:700000:ccw:minor:-625231,1690473:643600,1098807
A:700000:ccw:minor:5560500,451485:6939231,694593
A:800000:ccw:minor:-4510525,1641697:-3007016,1094464
A:800000:ccw:minor:3007016,1094464:4510525,1641697
```

The committed feasibility test asserts this list on both targets. The packet
implementation must later add ring order, provenance/source-set indices,
generated ids, and the standalone job-result digest without changing these
normalized analytic fragments.

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
integer nanometers. A0 wire arcs use topology endpoints plus an integer center
and explicit direction/major-arc branch; the contract carries no angle or
trigonometric field. Consumer fixture shorthand may describe a source sweep in
microdegrees, but the imported conformance vector freezes its expanded integer
vertices/center/branch before packet encoding.

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

A topology-indexed authored arc may not be a full circle. The exact integer
squared distances from its center to both topology endpoints must be equal and
nonzero; direction and major-arc branch select the unique authored arc. Full
circles use compact disk/annulus features.

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
non-circle topology candidates. OCCT selects candidate topology; it is not the
authority for canonical coordinates or rounding.

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

Across ordered stages, the accumulator retains certified exact real-algebraic
line/circle/intersection geometry and lineage. OCCT receives a local binary64
projection only to select candidate topology; every retained candidate is
reclassified against the exact algebraic arrangement before the next stage.
The governed nm-grid normalization occurs once, after the final stage. Snapping
an intermediate stage is forbidden because it could change whether a later
operand touches, crosses, or removes material.

The final published arrangement uses canonical integer-nm
endpoint-authoritative fragments. Lines use their exact shared topology
endpoints. Circular arcs use exact shared topology endpoints, an integer-nm
radius, direction, and major-arc branch; the circle center is derived and is
not stored as a competing result fact.

## Certified Normalization

The governed grid is one nanometer. Normalization is part of the operation and
cannot be relaxed per request.

1. Authored coordinates, centers, squared distances, radii, and line
   coefficients begin as exact integers. Rational constructions remain exact
   reduced rationals. Supported intersections and endpoint/radius-derived
   centers are represented by immutable real-algebraic expression DAGs over
   exact integers using `+`, `-`, `*`, `/`, and square root. Nested expressions
   remain algebraic across stages. Trigonometric functions and OCCT binary64
   coordinates are not canonical inputs.
2. Line-line, line-circle, and circle-circle candidates are reconstructed from
   their exact source or previous-stage algebraic endpoint/radius equations.
   Exact algebraic sign predicates select the candidate consistent with the
   OCCT topology and source incidence. Unsupported/nonclassifiable candidates
   fail closed.
3. Each algebraic scalar is enclosed by outward-rounded binary intervals. The
   initial precision is 256 bits and doubles through 512, 1024, 2048, and 4096
   bits. Every primitive operation rounds its lower bound toward negative
   infinity and upper bound toward positive infinity.
4. Nearest-nanometer, ties-away-from-zero rounding is certified only when the
   complete interval lies inside one integer's open half-nm Voronoi cell. If an
   interval contains a half-nm boundary, an exact algebraic comparison against
   that half integer determines equality or side. Exact equality selects the
   integer away from zero. If neither a unique cell nor exact tie/side can be
   certified at 4096 bits, the job fails with
   `normalization_ambiguous_tie`. This precision schedule and result are part
   of A0, not an implementation tolerance.
5. A normalized vertex must have squared displacement no greater than
   `0.5 nm^2` from the kernel vertex (maximum Euclidean displacement
   `sqrt(0.5) nm`).
6. A circular result fragment is reconstructed exactly from its normalized
   endpoints, normalized integer radius, direction, and major branch. The
   radius may move no more than `0.5 nm`, and the exact replay arc must have
   certified Hausdorff displacement no greater than `1.25 nm` from the
   pre-normalized analytic candidate over the fragment domain. No separately
   normalized center is serialized.
7. Distinct required vertices may not share one representative. A fragment may
   not collapse, cyclic order may not invert, containment may not change, and a
   line/arc fragment may not become incoherent.

Full result circles are decomposed into exactly two half arcs. After the center
and radius have certified integer-nm representatives, the ring starts at the
lexicographically least split vertex `L = (center.x - radius, center.y)`; the
other split vertex is `R = (center.x + radius, center.y)`. An outer circle emits
`L -> R` CCW through the lower half, then `R -> L` CCW through the upper half.
A hole emits `L -> R` CW through the upper half, then `R -> L` CW through the
lower half. Both fragments set `majorArc = false`. This is an exact
endpoint/radius replay form, not tessellation.

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
  directedFragments
  rings
  resultRegions
  sourceSets
  operandOutcomes

DirectedFragment
  start/end vertex references
  line, or circular arc with radius, direction, and major-arc branch
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

- vertices sort by `(x, y, incident analytic signature, complete intersection
  source-set tuple sequence)`;
- fragments are local analytic records and sort by exact endpoint pair, kind,
  radius/branch, and complete source-set content;
- an outer ring is CCW and a hole ring is CW;
- each ring rotates to the lexicographically least vertex/outgoing-fragment
  key;
- containment children sort by their canonical ring key;
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

Operation diagnostic identities are namespaced strings in generated APIs and
compact governed integers in a structurally valid result packet:

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

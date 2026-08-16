# Analytic Planar Boolean A0 Design

## Status

The logical TypeSpec shapes and packed A0 record layout are accepted and
frozen. The solver and numeric policy are reopened under
[ADR-013](../adr/013_filtered_resolution_bounded_planar_boolean.md) for a
speed-first filtered implementation with a 50 nm topology-resolution envelope.
The arbitrary-precision algebraic engine is explicitly non-primary. MATZ
accepted the original
consumer/provider design input, and independent review approved the analytic
architecture at normative revision
`529c768e559b4c88874264748d4186e775c8a4dd`. The separately compiled TypeSpec
candidate and its logical/packed reconciliation were accepted at
`f4b6a9b87bf16f57ef29dae22150b16f2a742b64`; it remains isolated from the
promoted-contract entrypoint until production promotion.

The structural freeze authorizes raw packet goldens and generated codec work.
It does not authorize production solver promotion, generated production
projections, or release before the filtered-solver gates pass. ADR-012 records
the historical exact-first feasibility decision; ADR-013 supersedes it for
production implementation.

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

The same test passes natively and under Emscripten/Node. After decoding stdout
as UTF-8, normalizing CRLF to LF, and removing exactly one final LF, its current
five-line canonical feasibility signature, including the twelve exact case-2
endpoint/radius/direction/branch fragments, has SHA-256
`c21b03c1b42a6cb3212cec5b3051987f645e21062eddecc82d3e3b0e0fd6dfc7`.
Raw console bytes are not compared because Windows text mode emits CRLF. The
canonical-byte construction and digest must be tested directly. This is
feasibility evidence, not the eventual packet golden.

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

The 1.25/1.4 nm collision probe above is retained as historical feasibility
evidence. Under ADR-013 it is inside the 50 nm topology-resolution envelope and
may merge deterministically; it is no longer a normative production failure.

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
swept area is allowed and is resolved by the exact arrangement. Inputs outside
these A0 rules fail the isolated job with `invalid_topology` or `invalid_arc`.

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

## Production Solver Architecture

The primary production engine is a Geometer-owned filtered line/circle
arrangement. It preserves analytic lines and circular arcs without making the
canonical real-algebraic backend part of the normal execution path. OCCT and
Clipper2 may be used as differential, feasibility, or sampled topology oracles;
their output is not independently authoritative.

Each job uses a deterministic integer-nm local origin near its bounds before
conversion to binary64 working coordinates. A deterministic sweep over
conservative outward-rounded curve bounds produces candidate pairs. The
secondary axis uses an augmented interval index; dense overlap on a single
projection therefore does not trigger a linear active-list scan for every
curve. This broad phase is a conservative filter: it may retain a disjoint pair
but must never discard an intersecting or within-resolution pair. Typical work
is `O(n log n + k)` for `n` boundary occurrences and `k` examined 2D pairs.
Adversarial inputs where `k` itself is quadratic are stopped by the governed
job-local examined-pair limit.

The implementation collects solver budgets in one internal limits value
object. Catalog values are immutable hard ceilings, while a host may select and
advertise lower effective memory/work/pair limits and tests may inject smaller
budgets. The 50 nm topology-resolution policy is not in that object and is not
caller-programmable.

Narrow-phase intersections and same-domain tests use filtered binary64
predicates with conservative outward error bounds. The internal curve domain
accepts bounded point and radius intervals, so authored irrational-radius arcs
and non-integral offset geometry do not need a second representation. Optional
integer certificates use fixed-width wide-integer predicates where practical;
job-local construction tokens certify shared carriers, parallel/concentric
families, and correlated arc sweeps emitted by the trusted lowering stage.
Integer certificates bind only singleton-equal filtered coordinates, and a
certified radius must bind either its integer radius or the complete outward
square-root enclosure. A decision is accepted
from the fast path only when its error interval proves the relevant side of the
50 nm topology threshold. An uncertain decision takes a deterministic bounded
slow path or fails the isolated job. It must never silently choose a topology
from an unchecked epsilon.

The implemented narrow-phase boundary is candidate-driven: it accepts a
strictly canonical curve table and only the pairs emitted by the broad phase.
It does not enumerate pairs, classify faces, run winding tests, or stitch
fragments. Each supplied pair performs bounded line-line, line-circle, or
circle-circle carrier construction followed by trig-free segment/arc-domain
tests. Ordinary irrational circle crossings remain small binary64 coordinate
intervals; they do not construct algebraic roots. Exact integer signs decide
bounded parallel/collinear, circle separation/tangency, and authored arc
validity cases. Square-root endpoints are adjusted outward until FMA residuals
verify the enclosure. A result interval wider than the 50 nm publication
radius, an unprovable domain decision, or an unprovable resolution collapse
returns the job-local resource failure with algebraic fallback still at zero.

Each input interval must itself fit the fixed 50 nm displacement envelope, and
all point construction and repair is checked against that same bound. Exact
tangent contact is reported separately from a certified resolution collapse.
In cancellation-heavy near-tangent cases, radial closeness to a circle is not
sufficient: the kernel also proves that the possible intersection displacement
from the published point is at most 25 nm when two roots share one
representative, so their separation cannot exceed 50 nm; otherwise the job
fails closed. Large exact integer line/circle and circle/circle tangencies use
fixed-width carrier certificates rather than cancellation-prone subtraction.
Coincident carriers are reported without partitioning finite overlap spans; the
indexed same-domain overlay stage owns that partition so no pair kernel grows a
hidden global scan.

The implemented production lowering boundary reads the already validated
packet directly and never instantiates the algebraic construction arena. It
chooses the least job coordinate as a deterministic local integer-nm origin,
checks the governed span using overflow-safe signed ordering, and precharges
the complete emitted curve count and a target-independent logical memory
budget before allocation. Authored line/arc coordinates remain exact integer
certificates while irrational radii use FMA-verified outward square-root
intervals. Disks and annuli lower to paired semicircles. Capsules lower to two
parallel offset lines and two certified semicircular caps, including arbitrary
angles and odd widths. Only this trusted stage mints construction carrier,
family, and sweep proof tokens; request bytes cannot supply them. Lowering uses
single-pass ownership traversal and expected-amortized-linear token lookup; it
does not enumerate curve pairs, and every token probe consumes the hard work
ceiling so adversarial hashing cannot become unbounded quadratic work.
Finite-arc bounds contain their endpoints and only the cardinal extrema that
may lie in the actual sweep, avoiding full-supporting-circle candidate
explosion for short arcs. Fixed-capacity open-address tables intern exact
carrier/family keys with every probe metered; capsule line keys retain a
fixed-width rational plus at most one square-root coefficient, sufficient to
prove equality or inequality without a general symbolic value. Stage and
operand visits share the same work meter, zero-operand jobs return an empty
geometry, exact maximum radii are canonicalized to singleton intervals, and
expanded geometry must remain representable after restoring the signed 64-bit
global origin. The aggregate sweep-tight emitted bounds, including finite-arc
cardinal extrema, must also remain inside the governed job-local coordinate
span.
Swept paths currently return job-local `unsupported_geometry` on this path;
their old exact piece-union and quadratic centerline validation are not used as
a production fallback. They become supported when the filtered indexed
arrangement can resolve the piece union under the same work budgets.

The implemented filtered split/overlay boundary groups exact lowering-issued
carrier ids, accepts only canonical broad-phase pairs, and runs the
candidate-driven narrow phase internally. There is no public boundary for
caller-constructed intersection records. Bounded
endpoint and intersection events sort once in canonical carrier order; event
enclosures merge only when their complete distance bound is at or below 50 nm.
Line groups use a monotone dominant-axis projection. Circle groups use
outward-certified half-plane/cross-product ordering and the deterministic
leftmost seam, without angles or trigonometric sort keys. A rightmost
partition is also emitted so each surviving circular span is x-monotone; the
left seam alone remains the cyclic ordering origin. Constructed
semicircle sweep certificates resolve the cancellation-prone antipodal case.
A cardinal seam is not an independent snap target. Each merged event retains
the ordered support endpoints of its complete same-carrier cluster, and a seam
may become the published representative only after all support pairs certify
at-or-below-50-nm separation. For example, the two intersections of a
radius-1000-nm circle with `x = 999 nm` remain distinct: each is less than
50 nm from the right seam, but they are about 89.4 nm apart from each other.
A strict total preliminary scalar key satisfies the sort contract even when a
rounded cross product is zero; the subsequent outward predicates remain the
authority and fail closed on uncertifiable adjacency. Split points are rebound
to both named carriers and finite domains within the fixed envelope before
event insertion. Finite-domain repair also requires an actual pair of curve
witnesses at or below 50 nm; two independent endpoint-to-carrier allowances
cannot bridge a larger diagonal gap. Narrow work and pair storage are included
in the integrated stage budgets. Before allocating the narrow result table, the
integrated boundary checks the target-independent minimum live storage for all
candidate results plus curve/carrier-group tables and the minimum work required
to evaluate and validate them. A known-impossible job therefore performs zero
narrow predicates and allocates no narrow result storage.

Each carrier group is swept with a fixed-array Fenwick/indexed active set.
Updates are logarithmic, membership enumeration is proportional to memberships
actually emitted, and a two-pass count/fill scheme fixes allocation before
output. There is no carrier-group pair cross product, per-cell full-group scan,
or linear duplicate search. Arrangement vertex/half-edge, provenance/source
membership, work, and target-independent logical-memory limits are enforced
before overrun. The output is deterministic atomic spans plus complete ordered
occurrence memberships. Each span names the first active member covering that
finite interval, not merely the carrier group's first curve; shared-carrier
partial overlaps and circle halves therefore retain a finite-domain witness.
The following face-classification stage decides which spans survive while
retaining positive and subtractive lineage.

The implemented filtered arrangement entry point accepts filtered geometry and
canonical broad-phase pairs, then runs the narrow phase and carrier overlay
internally. There is no public path for caller-constructed overlay spans,
memberships, split coordinates, or upstream telemetry. The arrangement globally
reconciles the resulting carrier-local span endpoints before it constructs
topology. Endpoint records
sort once by their complete coordinate intervals. An x sweep expires clusters
from a fixed-capacity heap and queries a deterministic secondary-y interval
index. An endpoint can join a cluster only if its complete outward distance to
the accumulated cluster hull is at most 50 nm. This is intentionally not a
transitive union-find proximity relation: a 0/40/80 nm chain produces two
vertices, never one 80 nm-wide repair. Mathematically exact threshold box
distances retain inclusive 50 nm behavior by evaluating the extreme endpoint
differences independently before squaring. Each global vertex stores the
complete hull, while every edge separately retains its certified carrier
start/end points so a permitted vertex merge cannot blur tangent order.

Noncollapsed spans become canonical edges and twin half-edges. A domain that
the overlay resolves to one event is forwarded as an isolated arrangement
vertex, and spans collapsed during global reconciliation use the same explicit
vertex-attached record. Both retain complete ordered occurrence memberships, so
sub-resolution removal does not erase lineage. Outgoing germs
first receive a strict scalar total key, including the clockwise positive-x
arc case, and every adjacent pair is then certified by outward tangent,
cross/dot, curvature, and radius predicates. The certified rotation system
builds `next`/`previous` links and enumerates components and directed cycles in
linear traversal work after sorting. Arc cycle orientation is valid because
the overlay has partitioned interior x extrema. No cycle-pair containment or
face/operand winding scan occurs in this boundary.

The arrangement inherits the overlay's consumed predicate count and algebraic
fallback count. Endpoint-index node visits, balanced-index and heap updates,
sort work, topology traversal, and angular predicates all consume the same job
ceiling. Target-independent logical charges model the clustering, edge,
half-edge, and cycle phases separately; the maximum phase and prior overlay
peak are checked before allocation. Before narrow/overlay execution, a
zero-allocation scan of lowering's dense first-use carrier-token stream derives
a safe lower bound on distinct noncollapsed carrier spans. Their unavoidable
endpoint, membership, topology, sort, and live-memory work is combined with the
upstream minimum; a budget that cannot admit the arrangement returns with zero
overlay work or storage. The scan bulk-charges one shared work unit per curve
before traversal, includes that charge in successful telemetry, and does no
scan when the remaining ceiling cannot admit the whole visit. Noncanonical
internal token streams retain only a proven strictly increasing prefix as a
conservative lower bound and are still validated by the owned overlay. Sparse
1x/2x fixtures guard expected
`O(n log n + k)` behavior. A direct 128-entry overlapping, nonmergeable interval
fixture proves the shared index stops exactly at its injected visit ceiling;
integrated tests separately prove budget propagation through certified carrier
inputs. Native/WASM bytes include overlapping irrational disks, partial
same-carrier domains, early rejection, the dense index result, the complete
rotation system, cycles, collapsed lineage, and canonical telemetry.

The implemented filtered Boolean-selection boundary owns arrangement
construction from ordered request records, trusted lowered geometry, and
canonical broad-phase pairs. It assigns directed cycles and half-edges to
faces with a certified vertical-slab sweep. Event columns are processed
atomically in certified y order, while a fixed-capacity implicit AVL sequence
stores only branches crossing the open slab. A disjoint-set structure joins
the cycle sides that border the same open gap. Vertical edges are resolved by
the rotation system rather than inserted into an open slab. Lowering-issued
vertical construction-column identities preserve exact shared x coordinates
for non-integral capsule offsets; unrelated overlapping x intervals remain an
uncertain predicate and fail the job. Circle events use canonical left/right
seam identities and x-monotone branch certificates. The stage does not run a
cycle-by-cycle containment test and does not spend a second 50 nm repair
allowance: it preserves the arrangement coordinates and accepts only certified
ordering decisions.

Face 0 is the canonical unbounded face. Every other face is ordered by its
unique counterclockwise boundary cycle and owns its directly adjacent
clockwise boundary cycles in canonical order. Edge memberships become sorted
coverage transitions in a fixed dual adjacency structure. Coverage propagates
from face 0 as canonical persistent binary-set roots, so each face stores one
root id rather than a copy of every operand bit. Non-tree dual edges validate
the propagated root exactly. A separate indexed stage tree updates only the
stage touched by each transition and evaluates ordered union/difference
semantics incrementally, including add-subtract-add and zero-operand stages.
Work is proportional to sorted topology plus actual edge/coverage transitions
and logarithmic state updates; there is no face-by-operand replay. Admission,
sweep, disjoint-set, dual, persistent-table, stage-tree, sort, and output work
share the programmable job ceiling, and target-independent logical memory is
checked before each phase. Each occurrence is rebound once to a dense operand
ordinal, making transition construction linear in emitted memberships. Every
stage-table pass is charged before traversal, known-size vectors use fixed
reserved capacities, and telemetry takes the maximum actual live phase instead
of adding sequential peaks. Separated short domains reserve their unavoidable
collapsed vertices and sort work before arrangement runs. The allocation-free
evidence comes from one validated, bulk-metered curve/bounds scan. Selection
models topology and persistent coverage as distinct live phases: face outputs
are counted once, and the proven transition prefix reserves fixed-capacity
coverage nodes and its canonical table before arrangement. Every two-support
and cyclic four-support seam-distance kernel is precharged. The result also
retains the lowering origin so later publication can recover global integer-nm
coordinates without rescanning the request. Canonical native/WASM fixtures
cover line and arc faces, nested components, tangent contact, odd-width
vertical capsules, ordered stage combinations, exact/one-short budgets, and
sparse scaling. A fixed-work, bulk-precharged scan of canonical broad-phase
pairs also computes a conservative possible split-span and coverage-transition
capacity before arrangement begins. Line/line pairs add at most one point and
line/circle or circle/circle pairs add at most two; exact shared authored
endpoints are removed because they do not split either domain. Membership
capacity starts from each curve's own line or partitioned-arc segments, adds
both incidences of each possible intersection, and adds at most two foreign
endpoints per curve for a same-carrier candidate. The bound is therefore
`O(curves + candidates)` even for normal repeated carriers such as every disk's
paired semicircles. The scan also proves the pair stream globally sorted and
unique before derived allocation, so malformed sequences are never classified
by the available memory budget. This makes a sparse split-heavy comb fail at
admission, with zero arrangement work, when the persistent coverage phase
cannot fit. Malformed filtered point enclosures remain `invalid_argument`
rather than becoming budget-dependent, and all proportional cycle-publication
traversals are charged before they begin.

The implemented filtered material-region stage owns that selection boundary
and never accepts caller-created faces or half-edge topology. It marks exactly
the directed half-edges with material on the left and empty space on the right.
At each vertex, one cyclic pass over the arrangement's already certified
outgoing order assigns every selected incoming half-edge exactly one selected
successor and predecessor. Rings are then traced once; there is no repeated
seam walk through same-material edges. Equal-material faces are joined through
a governed disjoint set, and the resulting material/empty component graph is
traversed once from the unbounded component to derive canonical ring parents,
depths, directions, and distinct interior-connected material regions. A
point-tangent contact therefore keeps separate regions, while nested
outer/hole/island structures form one parent chain. Isolated collapsed vertices
remain successful zero-area lineage and create no ring or region.

The selection admission pass uses its existing `O(curves + candidates)` split
envelope to reserve the possible narrow result table, overlay event/output and
arrangement phases, and the complete retained-selection plus material-region
live phase before arrangement begins. Endpoint-coincident major/full-circle
domains participate in that bound. It also reserves a conservative downstream
work allowance; selection executes against the remaining ceiling and region
traversals consume the reservation with actual work reported separately. A
known-doomed region job therefore performs no arrangement work. Every
half-edge, rotation, component, ring, adjacency, output, and sort traversal is
precharged, known arrays are fixed-capacity, and native/WASM topology and
telemetry bytes are identical. Complete lineage projection, final 1 nm
normalization, and operation dispatch remain subsequent stages.

Across ordered stages, the accumulator retains the resolved analytic
arrangement and lineage. Publication to the governed nm grid occurs once after
the final stage. Publishing an intermediate stage is forbidden because it
could change whether a later operand touches, crosses, or removes material.

The arbitrary-precision algebraic implementation is isolated behind a narrow
non-primary interface. It may support conformance tests, offline diagnosis, or
a measured and governed fallback for a genuinely uncertain predicate. Normal
production success must not depend on constructing canonical algebraic values,
resultants, or nested radicals.

## Historical Exact-First Architecture (Non-Primary)

The following section records the already implemented ADR-012 prototype and
its invariants for oracle and fallback maintenance. It does not prescribe the
ADR-013 production hot path.

OCCT is the selected A0 analytic kernel, subject to completion of the portable
fixture suite and performance gates. Clipper2 may be used only as a sampled
area/topology oracle; its output is never authoritative.

Each job uses a deterministic integer-nm local origin near its bounds before
conversion to OCCT millimeters. This limits double magnitude without changing
wire identities. Each stage submits its operands in canonical id order as one
Boolean set operation and rejects non-line or non-circle OCCT candidates.

The authoritative topology is nevertheless an independently enumerated exact
arrangement. Geometer enumerates every authored or derived line/circle boundary
occurrence, uses conservative exact or outward-rounded bounds only to discard
provably disjoint pairs, evaluates every remaining pair with exact predicates,
splits every curve at the complete exact intersection set, constructs the full
half-edge arrangement, and classifies every face against prior material and all
current-stage operands. An acceleration structure may change pair visitation
order but may not omit a pair unless disjointness is certified exactly.

Coincident carriers use an exact same-domain overlay rather than a finite
intersection set. Two line carriers are same-domain exactly when their
homogeneous line equations are projectively equal under the exact algebraic
predicate system; two circle carriers are same-domain exactly when their exact
centers and radii are equal. Each group uses the lexicographically least
complete source-expression key as its carrier key. That key is the canonical
source-reference tuple followed by the exact canonical expression-DAG bytes
for the carrier equation, closed-domain endpoints, and authored orientation; it
contains no OCCT identity or allocation order. A line group chooses the exact
tangent direction whose first nonzero component is positive. A circle group
chooses CCW as its positive direction and uses its exact leftmost point as the
cyclic ordering seam.

For every same-domain group, Geometer collects every boundary-occurrence
endpoint (and the circle seam when a covered domain is cyclic), deduplicates
points by exact equality, and splits every occurrence at all collected points
that lie in its closed domain. Line points sort by exact projection on the
canonical tangent. Circle points sort CCW from the seam by exact half-plane and
cross-product predicates; no angle or trigonometric approximation participates.
The open interval between each consecutive point pair is one atomic carrier
interval. Its occurrence membership is the complete set of source occurrences
whose domains contain that interval, and each membership records whether the
occurrence agrees with or opposes the canonical carrier orientation.

Face classification, rather than signed-multiplicity cancellation alone,
decides whether an atomic interval survives and in which direction. All
memberships are retained while that decision is made: equal and opposite
occurrences may cancel a topological seam without losing lineage, while a
surviving interval accumulates the unique canonical source references of every
coincident positive or subtractive occurrence that owns its corresponding
side transition. Partial overlaps therefore split at both occurrences'
endpoints; identical operands, coincident disks, and shared line/arc edges have
the same result independent of visitation order. Endpoint provenance is the
union of all incident memberships after exact source-reference deduplication.

OCCT may propose adjacency, same-domain grouping, and a traversal seed. It may
not create or suppress an authoritative vertex, half-edge, face, component, or
classification. Before a stage commits, a bidirectional audit maps every OCCT
candidate boundary occurrence to the exact arrangement and every exact
material boundary occurrence back to an OCCT candidate. Missing, extra, or
incompatibly oriented topology fails the job with `solver_failed`; no tolerant
repair becomes canonical. The exact arrangement remains complete even when
the audit fails, so the failure is deterministic and does not accept an OCCT
omission.

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

Across ordered stages, the accumulator retains the authoritative exact
real-algebraic arrangement and lineage. OCCT receives a local binary64
projection only for its independently audited candidate topology. The governed
nm-grid normalization occurs once, after the final stage. Snapping an
intermediate stage is forbidden because it could change whether a later
operand touches, crosses, or removes material.

The final published arrangement uses canonical integer-nm
endpoint-authoritative fragments. Lines use their exact shared topology
endpoints. Circular arcs use exact shared topology endpoints, an integer-nm
radius, direction, and major-arc branch; the circle center is derived and is
not stored as a competing result fact.

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

## Historical Exact Normalization (Oracle Only)

The following 1 nm exact-first rules define the existing conformance engine.
They are retained for oracle coverage but are superseded as production
acceptance rules by the 50 nm policy above.

The governed grid is one nanometer. Normalization is part of the operation and
cannot be relaxed per request.

1. Authored coordinates, centers, squared distances, radii, and line
   coefficients begin as exact integers. Rational constructions remain exact
   reduced rationals. Every nonrational scalar uses the selected portable
   real-algebraic representation: a primitive square-free defining polynomial
   in `Z[x]`, a dyadic rational isolating interval containing exactly one of
   that polynomial's real roots, and a signed-subresultant Thom encoding that
   identifies the root. A defining polynomial need not be irreducible.
   Coefficients and rational numerators/denominators use arbitrary-precision
   signed integers. Trigonometric functions and OCCT binary64 coordinates are
   not canonical inputs.
2. Line-line, line-circle, and circle-circle candidates are reconstructed from
   their exact source or previous-stage algebraic endpoint/radius equations.
   Arithmetic constructs defining polynomials with exact resultants, removes
   repeated factors by polynomial GCD, and identifies the intended result root
   using operand isolating intervals and exact subresultant signs. Square root
   selects the certified nonnegative root of `y^2 - x`. Exact equality uses
   polynomial GCD plus common-root isolation; exact order/sign refines disjoint
   isolating intervals and, when they overlap, decides equality or the sign of
   the exact algebraic difference with the same resultant/subresultant
   procedure. Identically zero expressions and exact half-grid equality are
   therefore decidable rather than delegated to interval convergence.
3. The integer backend is `boost::multiprecision::cpp_int`; polynomial,
   resultant, signed-subresultant, GCD, square-free, and root-isolation code is
   Geometer-owned deterministic C++17 shared by native and Emscripten. A
   focused native/WASM feasibility target must prove this backend and the
   governed limits before production solver implementation. There is no
   binary64, OCCT, platform `long double`, or tolerance fallback.
   Before that target is complete, the exact expression DAG must freeze its
   node catalog, child order, integer/algebraic payload representation,
   structural interning rules, and canonical byte encoding. Native and
   Emscripten conformance vectors must produce identical bytes, including
   algebraically equal constructions reached through different traversal
   orders.
   If constructing or deciding a value would exceed the governed polynomial
   degree, coefficient-bit, storage, predicate-work, or memory limit, the job
   terminates with `resource_limit_exceeded` before an approximate decision is
   used. Thus every admitted predicate has a total exact result and every
   non-admitted predicate has one stable fail-closed result.
4. Each algebraic scalar is additionally enclosed by outward-rounded binary
   intervals for acceleration. The
   initial precision is 256 bits and doubles through 512, 1024, 2048, and 4096
   bits. Every primitive operation rounds its lower bound toward negative
   infinity and upper bound toward positive infinity.
5. Nearest-nanometer, ties-away-from-zero rounding is certified only when the
   complete interval lies inside one integer's open half-nm Voronoi cell. If an
   interval contains a half-nm boundary, an exact algebraic comparison against
   that half integer determines equality or side. Exact equality selects the
   integer away from zero. Interval refinement stops as soon as it certifies a
   cell; reaching 4096 bits without doing so invokes the total exact comparison
   from rule 2. That comparison yields side or equality, unless a governed
   algebraic/predicate resource bound is exceeded, in which case the only
   fail-closed outcome is `resource_limit_exceeded`. There is no separate
   ambiguous-tie outcome in A0.
6. A normalized vertex must have squared displacement no greater than
   `0.5 nm^2` from the kernel vertex (maximum Euclidean displacement
   `sqrt(0.5) nm`).
7. A circular result fragment is reconstructed exactly from its normalized
   endpoints, normalized integer radius, direction, and major branch. The
   radius may move no more than `0.5 nm`, and the exact replay arc must have
   certified Hausdorff displacement no greater than `1.25 nm` from the
   pre-normalized analytic candidate over the fragment domain. No separately
   normalized center is serialized.
8. Distinct required vertices may not share one representative. A fragment may
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

Under the historical exact-first policy, any violation failed the isolated job
with a stable normalization diagnostic. The MATZ 1.25/1.4 nm notch was its
topology-collapse sentinel. ADR-013 places that notch below the production
resolution envelope, so it may now collapse deterministically. The
arbitrary-angle arc fixture remains a required successful non-grid-intersection
case.

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
meaning.

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

## Browser Packaging

The reviewed generic ABI specification already assigns the generic functions
to the full browser/Web Worker target. A0 therefore uses the full OCCT browser
artifact initially. It does not expand the legacy lightweight planar-only
artifact and does not change the locked transport-review packet. The generated
high-level client negotiates the operation capability and hides the artifact
factory details.

## Performance And Telemetry

Speed within the 50 nm correctness envelope is the primary production goal.
The initial real-board acceptance target is at most 1 second and 512 MiB peak
working memory per all-copper batch on the recorded reference machine. A
5-second/1-GiB ceiling remains a temporary bring-up limit, not the desired
steady-state performance.

The A0 qualification reference is a single serialized worker in a Release
build on Windows 11 Pro build 26200, AMD Ryzen 9 9950X (16 physical/32 logical
cores), and 66,125,668,352 bytes installed RAM. Native compiler, browser,
Emscripten, OCCT tag, Geometer revision, fixture digest, power mode, warmup
count, repeat count, median, and peak-memory measurement method are recorded
with every accepted benchmark. A different machine may supply comparative
telemetry but cannot silently replace this release target.

Wall time, peak memory, operand/input-segment/result-region/result-segment
counts, broad-phase candidate pairs, narrow-phase predicates, exact-fallback
count, and normalization-failure count are noncanonical telemetry. They are
reported outside canonical job-result bytes.

## Promotion Gates

Before design freeze:

- MATZ must ratchet the arbitrary-angle fixture from provisional to successful;
- this design, the packet specification, diagnostic assignments, capability
  table, and generated API shapes must receive joint review;
- portable logical fixtures must be imported into Geometer with digests and
  structural expectations; and
- raw-byte goldens must be generated only after the packet layout freezes.

These conditions are met. The vendored MATZ manifest records the
arbitrary-angle case as successful with the reviewed twelve-fragment native/
WASM feasibility signature; the consumer/provider and independent architecture
reviews are recorded in the compatibility snapshot and plan log; the isolated
TypeSpec API shapes received focused independent review; and the ten portable
plus two real-board observation cases are digest-locked in this repository.
Raw-byte goldens deliberately remain the first post-freeze codec artifact.

### Filtered Production Gates

ADR-013 adds gates that supersede exact-first production acceptance:

- ordinary line/arc and real-board fixtures execute without algebraic fallback;
- 49/50/51 nm sweeps prove deterministic at-threshold collapse and
  above-threshold topology preservation;
- all published boundaries remain within the 50 nm Hausdorff envelope;
- broad-phase pruning and governed candidate-pair counts are exercised by
  sparse, dense, and adversarial inputs;
- native and WASM produce equivalent topology, canonical bytes, threshold
  decisions, and diagnostics; and
- the 1-second/512-MiB target and 5-second/1-GiB temporary ceiling are measured
  on the recorded reference corpus.

The exact-focused sections below describe retained conformance evidence from
the prototype. They remain valuable oracle and mutation lanes, but their
sub-50-nm topology expectations do not override ADR-013 and they do not make
the algebraic backend the production hot path.

### Exact Result Replay And Mutation Sentinels

Solver output has a second, independent topology oracle before production
promotion. It reconstructs each job's normalized directed line/arc fragments,
including the unique circle center selected by endpoint, radius, direction, and
major-branch data, then runs that published geometry through the exact
arrangement, union-stage, and result-region pipeline. Reconstructed rings are
matched to claimed rings by their complete fragment-occurrence sets. Ring
winding, smallest strict parent containment, depth, connected result regions,
and outer-ring ownership must agree exactly. Jobs are replayed independently so
overlapping geometry in different batch jobs cannot interact.

This semantic replay is deliberately separate from the allocation-bounded raw
packet decoder. The production solver must pass it before its packet is exposed;
foreign bytes still receive full structural and canonical validation without
silently invoking a geometry operation in a codec.

The pull-request mutation lane currently proves that the oracle rejects
structurally valid reversed line winding, reversed circular-arc winding, a hole
assigned to a non-containing root, and a false point-tangent hierarchy merge.
Valid outer/hole/island replay proves that every even-depth island owns its own
result component, while an island collapsed into the surrounding component is
rejected. A single linear ownership index assigns each ring to its nearest
even-depth result ring and builds per-job ring/region lists; 16,384-deep and
65,535-empty-job regressions protect the bounded traversal before exact replay.
Canonical packet bytes separately detect removal of an otherwise geometrically
invisible contributor, and the exact normalization oracle detects a
ties-to-even substitution. A separate ordered-stage sentinel replaces the
first-stage boundary at `7/5` nm with its premature 1-nm-grid representation
before subtracting the exact `[6/5, 7/5]` tail. Both paths publish the same
final normalized rectangle, but the mutation erases the required subtraction
effect and is rejected by the lineage oracle. Native and Emscripten must report
the identical governed sentinel inventories. These sentinels are one layer of
the synthetic-correctness program; the companion sections below govern its
closed-form, degeneracy, exhaustive, property, lineage, and
between-stage-normalization lanes.

### Closed-Form Analytic Invariants

The pull-request lane also includes an OCCT-independent closed-form oracle for
controlled normalized result packets. Its corpus covers an axis-aligned
rectangle, a disk, concentric annulus, line-and-semicircle capsule, nested
rectangular outer/hole/island topology, and an L-shaped constant-width swept
path with a controlled quarter-circle join. The oracle first requires exact
topology replay, then integrates each directed line or circular arc
independently using Green's theorem. It retains area and perimeter as exact
rational-plus-pi coefficients rather than comparing floating-point
approximations. Component, hole, and Euler-characteristic expectations are
part of the same governed signature, and native and Emscripten must reproduce
that signature byte for byte.

Full circular arrangement cycles use exact squared-distance containment against
their common circle carrier. This avoids an artificial algebraic ray-casting
cost for simple concentric disks while preserving the generic ray oracle for
mixed line/arc cycles. Diameter-defined packet arcs canonicalize their proven
zero center offset to rational zero before arrangement replay.

This oracle intentionally accepts only axis-aligned lines and exact
semicircles or quarter circles, for which the independent formulas are small
and auditable. Quarter-circle centers are derived as doubled rational
coordinates from endpoints, radius, direction, and branch, avoiding integer
division or a duplicated implementation center. It is also replayed through
integer translation, exact 90-degree rotation, reflection with winding
recanonicalization, positive integer scaling, and source-ID renaming. The
mixed line/arc capsule, swept path, and nested outer/hole/island fixture must
retain exact topology and the appropriately transformed rational-plus-pi
measure. Each transformed packet has a governed canonical SHA-256, with an
identical aggregate signature under native and Emscripten execution.
Before those aggregate checks, an independent field-level relation oracle ties
every output vertex, radius, directed endpoint, ring reference, source table,
and source identity to the named transformation. Reflection specifically
requires reversed ring traversal and swapped fragment endpoints while arc
direction remains unchanged after the two orientation reversals. Source-ID
renaming requires byte-for-byte-equivalent geometry/topology fields and the
explicit mapped source tuple.

This is evidence for the closed-form and controlled-angle metamorphic lanes,
not completion of arbitrary-angle certified integration or production
cross-transport execution.

### Exact Degeneracy Boundaries

The pull-request lane governs an OCCT-independent exact parameter sweep around
the first high-risk boundaries. Unit circles are classified immediately below,
at, and above external tangency; radius-two and radius-one circles receive the
same treatment at internal tangency. Concentric radii immediately below, equal
to, and above one prove that exact coincidence is not tolerance-based.
Near-collinear rational line carriers likewise distinguish a shared-point
intersection, exact coincidence, and the opposite shared-point perturbation.

The normalization sweep covers both signs around one-half nm and the interval
around three-halves nm, including exact ties, and requires the governed
ties-away-from-zero result. Rational Pythagorean endpoints on a radius-101
circle exercise counterclockwise arcs immediately above 0 degrees and below
360 degrees, as well as immediately below and above 180 degrees with the exact
semicircle between them. Their minor/major flags must be coherent; flipped
flags, the alternate branch at exactly 180 degrees, and a single-arc
start-equals-end encoding fail closed. Full circles remain represented by
multiple analytic fragments. Native and Emscripten executions must reproduce
the complete classification signature byte for byte.

The final-result normalization lane separately sweeps vanishing holes in both
axes and a boundary-connected notch immediately around a positive half-grid
tie. Feature boundaries at `1.49 nm` and exactly `1.5 nm` retain distinct
ties-away representatives and must preserve the independently expected hole or
notch topology. Boundaries at exactly `1.5 nm` and `1.51 nm` share a required
representative and must return `normalization_topology_collapse` without
exposing a partial result. Notch inputs are atomically split at their
outer-boundary intersections before the arrangement is constructed.
Successful canonical geometry and failed outcome identities form one
digest-locked signature that is identical under native and Emscripten
execution.

The same exact degeneracy signature includes a valid U-shaped line centerline
whose two nonadjacent 12-nm legs are swept at width 10 nm. Separations of 9,
10, and 11 nm respectively produce a positive 12-nm2 interior-strip overlap
witness, exact contact, and a 12-nm2 gap witness. The exact circle-intersection
backend independently classifies the corresponding round endpoint caps as
two intersections, one tangent point, and disjoint. All three centerlines
remain valid: A0 permits overlap between nonadjacent parts of the swept area
even though it forbids centerline retracing and self-intersection. The
classifications are `overlap_allowed`, `contact_allowed`, and
`disjoint_allowed`; no tolerance or solver-generated geometry decides the
boundary.

### Exact Boolean Identities

The pull-request lane evaluates the regularized identity cases through the
exact arrangement, ordered-stage selection, result-region, provenance,
operand-outcome, and final normalization pipeline. Two coincident rectangles
with distinct coverage and operand identities prove that `union(A, A)`
publishes the same normalized analytic geometry as `A` while retaining both
contributors in result-region lineage and symmetric contributor/absorbed
outcome events. `difference(A, A)` must succeed with an empty normalized
result, record that the original positive operand was completely removed, and
emit a concrete `subtraction_effect_survives` event for the subtractor with an
empty boundary/result reference range.

A zero-operand union stage and a zero-operand difference stage are each run
after the baseline union. Both must preserve normalized geometry, per-face
positive/subtractive lineage, and all operand outcome events exactly. Native
and Emscripten executions reproduce one governed identity signature byte for
byte.

### Exact Boolean Metamorphic Relations

The pull-request lane exhaustively permutes three operands within one union
stage and two operands within one difference stage. Every permutation must
produce byte-identical normalized geometry and byte-identical face/result
lineage projections. Independent unit-cell area and connected-result counts
anchor the union L-shape and the two disconnected difference results.

Splitting the same union or difference operands into separate ordered stages
must preserve normalized geometry for these mathematically equivalent cases;
lineage is deliberately excluded from that relation because stage structure
may legitimately affect attribution history. Conversely, swapping an ordered
union-full-grid/difference-unit stage pair must change the exact result from
three cells to four. This `ordered_stage_swap` mutation sentinel proves that
the suite does not accidentally erase governed stage order. The complete
geometry and lineage projections are digest-locked under native/Emscripten
parity.

### Bounded Rectangle Enumeration

The pull-request lane exhaustively enumerates all nine nonempty axis-aligned
rectangles whose vertices lie on the integer `0..2` grid. One-stage cases run
both union and difference from the empty accumulator. Two-stage cases cover
every ordered rectangle pair and every union/difference operation pair.
Three-stage cases cover every operation triple over the lower-left unit,
full-grid, and upper-right unit rectangles, producing 558 deterministic cases
in total.

The input boundaries are split at every integer grid vertex before exact
arrangement construction. An independent four-unit-cell oracle evaluates the
ordered set operations, directed exterior unit-edge set, and expected
four-neighbor connected-component count. Every arrangement face must match the
independently replayed stage value. Final normalized fragments must remain
analytic lines. The test expands their actual directed boundaries into unit
edges, independently reconstructs cell occupancy by a winding query at each
cell center, and requires both the actual occupancy mask and exact directed
edge set to equal the oracle. Exact signed shoelace area must equal the
occupied-cell area, and normalized region/ring counts must equal the oracle
component count. The per-case actual occupancy, actual canonical directed-edge
projection, component, vertex, and fragment signature is digest-locked and
identical under native and Emscripten execution.

The native scheduled lane additionally projects the 117 enumeration cases of
the form one union followed only by differences through the existing Clipper2
planar solver. This comparison is explicitly warning-only: it cannot accept or
reject exact geometry. The governed baseline reports two useful disagreements.
After subtracting opposite diagonal unit cells from the full `2x2` rectangle,
the exact regularized oracle returns two point-contact components, while
Clipper2 reports one component with the same area; both subtraction orders are
recorded with complete replay descriptors. The monthly/manual workflow emits
these warnings to its job log as investigation evidence.

### Deterministic Seeded Rectangle Properties

The pull-request property lane uses a repository-owned SplitMix64 generator
with four frozen 64-bit seeds. It generates 32 cases on a `4x4` integer
unit-cell grid; each case contains one to five ordered union/difference stages
and one to three rectangle operands per stage. Every authored boundary is
split into atomic unit carriers.

An independent cell-mask evaluator applies the union of operands within each
stage and then the ordered stage operation. It derives the exact directed
unit-edge boundary and four-neighbor result-component count. Published
normalized geometry must equal that boundary exactly and contain no duplicate
edges, duplicate vertices, or unreferenced vertices. Reversing every
same-stage operand list must preserve canonical geometry, face lineage, and
result count byte for byte. Each failure identifies its seed, case index, and
complete stage/rectangle replay descriptor. Before failing, the harness
deterministically removes stages and operands and shrinks/translates rectangle
extents while the same property failure persists. It reports the resulting
one-minimal fixture under those transformations. An injected reducer sentinel
must reduce a three-stage, four-rectangle input to the single unit rectangle
`U[0,0,1,1]`. Rectangle mutation uses guarded index iteration and immediately
restarts after an accepted replacement; a second sentinel retains two stages
while accepting the rectangle shrink and must reach `D[]U[0,0,1,1]`. The
complete actual boundary, face-lineage, result-count, and reducer projection is
digest-locked under native/Emscripten parity.

The default profile is the modest deterministic pull-request seed set. A
separate conventional `nightly` profile expands this to eight frozen seeds and
16 cases per seed (128 cases and 256 exact baseline/permutation executions).
Native and Emscripten must produce the same separately governed digest. Because
Geometer changes infrequently, CI schedules that extended profile monthly and
also exposes it through an explicit manual workflow input for release or
kernel-change validation. Any real minimized failure must be promoted into the
committed regression corpus before the affected change can proceed; no real
property failure has been discovered by the frozen profiles to date.

### Exact Many-To-Many Lineage Matrix

A dedicated exact fixture overlays two distinct positive operands across two
disconnected rectangles. Both final result regions must carry both operand
identities, yielding the complete four-edge operand/result association matrix
`{A->R0, A->R1, B->R0, B->R1}`. Every one of the eight surviving line
fragments must retain both coincident authored sources. Both operands publish
`contributes_final_material` with references to both regions and the symmetric
`redundant_or_absorbed_coverage` event.

The fixture requires the exact association matrix and separately injects an
omitted-association mutation that the equality oracle must reject. Reversing
the unordered union operands must preserve the complete region, provenance,
and outcome projection. Native and Emscripten executions are digest-locked
byte for byte. Together with the absorbed/coincident, partial/complete
removal, surviving/overwritten subtraction, and refill fixtures above, this
closes the preproduction lineage matrix. Production operation replay through
all released client transports remains a later promotion gate.

### OCCT 8.0.1 Qualification

Production solver work and conformance-golden freeze require a side-by-side
qualification of exact upstream tags `V8_0_0` and `V8_0_1`; upstream master is
not eligible. The qualification runs the feasibility corpus, the governed
synthetic-correctness program, bounded timeout regressions for fixed Boolean
hangs, and all existing STEP, HLR, GLB, planar, CLI, Python, native, and WASM
suites. It compares topology, diagnostics, canonical signature/bytes,
provenance expectations, runtime, memory, and native/WASM parity.

The Emscripten install-rule patch in `scripts/build_wasm.py` remains required
and is explicitly retested. Native and WASM dependency-cache profiles are
rebuilt and published only after acceptance. If 8.0.1 passes, Geometer pins the
exact `V8_0_1` tag and reviews any feasibility-signature rebaseline before
goldens. If it fails, Geometer retains 8.0.0 and commits the rejecting
regression fixture and decision. Either outcome leaves the exact arrangement
authoritative and OCCT candidate topology independently audited.

The governed harness uses tag-specific dependency, build, distribution, and
validation roots. In addition to CTest and exact/synthetic parity, it exercises
the native CLI and Python wrapper, STEP/HLR/GLB/SVG/planar outputs, retained
WASM interfaces, generated TypeScript package and Worker clients, and a real
Chrome/Chromium HLR load. Cross-tag comparison excludes only measured HLR
timings and the generated STEP header timestamp; all remaining governed output
projections compare exactly. Browser artifact sizes/digests and repeated
STEP-to-GLB heap observations are retained for review.

The complete comparison at Geometer revision
`64d598e1d41523f5b940ccdbc7130c1b3fc94526` passed and received independent
acceptance in review packet
`reviewer-01a00241-a037-752d-ad0b-820e8412a786`. No governed semantic or byte
difference was found, the repeated browser heap remained flat at 64 MiB, and
the exact feasibility signature remained unchanged. This selects the exact
`V8_0_1` production dependency pin. Cache publication and restore verification
remain part of the pin-integration gate; production operation dispatch remains
disabled.

Before release, Geometer must pass native/full-browser/executable parity,
generated TypeScript/Rust/Python consumption, malformed/resource tests,
documentation generation, native/WASM/package/Rack/L99 gates, and candidate
MATZ integration. MATZ switches production only after pinning the additive
tagged release and passing its real-board suite.

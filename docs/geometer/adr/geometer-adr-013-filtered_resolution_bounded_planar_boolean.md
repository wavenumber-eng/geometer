+++
type = "adr"
id = "geometer-adr-013"
domain = "geometer"
status = "accepted"
title = "Use Filtered Resolution-Bounded Analytic Planar Booleans"
created = "2026-08-18"
+++

# ADR-013: Filtered, Resolution-Bounded Analytic Planar Booleans

## Status

Accepted for implementation by project-owner direction on 2026-08-15. This
decision supersedes ADR-012 for the production solver architecture. It does not
promote `geometry.analytic_planar_boolean_batch.a0` to production.

[ADR-017](geometer-adr-017-retain_analytic_planar_boolean_as_experimental.md)
records the current disposition: the implementation is retained but is
experimental and not production-ready. “Production” below describes the target
architecture considered in this historical decision, not the operation's
current support level.

## Context

The primary consumers need fast, deterministic planar resolution for:

- PCB artwork visualization and documentation, including pours, mask films,
  drills, and cutouts;
- planar faces used to construct 3D viewer geometry; and
- PCB manufacturing geometry.

For these uses, 50 nm (about 0.002 mil) is below the required practical
resolution, while the exact-first prototype has substantial complexity and can
consume excessive time or memory on ordinary irrational circle intersections.
Its result-normalization path can construct nested radicals whose canonical
storage estimate exceeds the job budget even for a small overlapping-disk
case. Raising that budget would not address the production need for predictable
speed or the quadratic number of possible curve pairs.

The exact prototype is valuable evidence and a useful oracle, but it does not
need to be the normal execution engine.

## Decision

Geometer will implement a speed-first filtered analytic arrangement. The
normal production path is deliberately not the arbitrary-precision algebraic
engine.

- Public coordinates remain signed integer nanometers, and published line and
  circular-arc geometry remains analytic. The output coordinate grid remains
  1 nm; the 50 nm value is a topology-resolution envelope, not a coarser wire
  unit.
- Features, gaps, or separations at or below 50 nm may be deterministically
  merged, bridged, shortened, or collapsed. Features and separations greater
  than 50 nm must not be lost or connected by numeric repair.
- Published boundaries must stay within 50 nm Hausdorff distance of the
  resolved analytic result. Every repair and threshold decision is
  deterministic and shared by native and WASM builds.
- A narrower, explicitly reported input-normalization exception applies to
  same-width positive capsules in one job. If both corresponding centerline
  endpoints are within 1 um Euclidean distance, the later operand may be
  lowered on the earliest-authored representative. The original operand and
  feature identities remain in lineage and operand outcomes. Every nonzero
  adjustment emits the successful-job warning
  `geometer.operation.analytic_planar_boolean.resolution_coalesced`, identifying
  the adjusted operand and representative feature. Native/WASM telemetry
  reports the coalescence count and maximum endpoint adjustment. Clusters are
  representative-bounded, not transitively chained, and never mix widths or
  positive and subtractive operands. Intervening difference stages still run
  in their original order and with their original geometry; only the later
  positive capsule's source coordinates are normalized. Consequently, the
  later positive operand and final result may move within 1 um even when a
  difference stage intervenes. All other topology resolution remains governed
  by 50 nm.
- The primary predicates use job-local binary64 coordinates with conservative
  outward error bounds. Fixed-width wide-integer predicates are used where
  integer-nanometer inputs make them practical. Uncertain predicates take a
  deterministic slow path or fail the isolated job; unchecked tolerance
  guesses are not permitted.
- A spatial broad phase must prune disjoint curve pairs before narrow-phase
  intersection. Typical work should scale as `O(n log n + k)`, where `k` is the
  number of examined 2D curve pairs. Dense overlap on either projection alone
  must not cause an active-list scan: the production broad phase indexes the
  secondary axis and meters index visits separately. The unavoidable case in
  which `k` itself is quadratic remains governed by the job-local
  examined-curve-pair limit.
- No later stage may reintroduce an unmeasured whole-job all-pairs scan.
  Intersection deduplication, arrangement insertion, provenance aggregation,
  and normalization must use indexed or sorted structures, expose work
  telemetry, and include sparse 1x/2x scaling tests. A genuinely dense output
  may require quadratic work in its reported examined-pair count; it must terminate
  at a job-local budget instead of degrading without a bound.
- Relationship evaluation over already published result geometry uses a
  private strict execution policy through broad phase, narrow phase, overlay,
  arrangement, face selection, and material-region construction. That policy
  does not spend the 50 nm topology-resolution allowance a second time: only
  exact singleton endpoints or trusted equal construction roots merge, while
  unresolved ordering, equality, or carrier predicates fail the invocation.
  Existing public solver entry points retain the normal 50 nm policy, and the
  strict policy is not caller programmable.
- The narrow phase is a focused Geometer-owned C++17 module. It consumes only
  canonical pairs supplied by the broad phase and performs constant carrier
  and finite-domain work for each pair; it has no internal all-curves loop,
  winding classifier, arrangement traversal, or dependency on OCCT, Rust, or
  the algebraic backend. Line-line parallelism, collinearity, circle
  separation, tangency, and integer arc validity use portable fixed-width
  wide-integer signs where their bounded integer inputs make that sufficient.
  Constructed coordinates use outward interval operations, and a square-root
  enclosure is accepted only after an FMA residual verifies both endpoints.
  Any point interval or resolution collapse that cannot prove the 50 nm
  displacement bound fails the isolated job. A line parameter strictly outside
  its finite domain is discarded immediately in strict replay; under the normal
  policy its complete along-carrier overshoot must also exceed 50 nm so the
  existing pairwise endpoint witness remains authoritative. Finite-domain repairs are
  certified pairwise: when both carrier intersections lie beyond their finite
  curves, actual endpoint witnesses must be at most 50 nm apart. Two
  independently sub-50-nm endpoint distances may not compose into a bridge
  above the envelope, and a domain predicate that merely straddles an endpoint
  fails closed. For nonparallel line pairs, the kernel may reject before
  constructing the point enclosure only when a fixed-width rational parameter
  from validated integer construction lines, or a conservative interval
  parameter otherwise, lies strictly outside `[0,1]`; boundary-touching or
  straddling parameters continue through the existing fail-closed domain path.
  This exact/interval exclusion is metered and spends no tolerance. Its internal curves carry bounded
  point and radius intervals plus optional integer certificates, so irrational
  authored radii and non-integral offset geometry do not require an integer-only
  side representation. Radial sagitta alone never certifies a near-tangent
  collapse; the possible point displacement itself must be within half the
  envelope when two roots merge. Trusted lowering may attach validated
  job-local construction tokens for correlated offset endpoints, shared
  carriers, parallel/concentric families, and arc sweeps; these are bounded
  fixed-width facts, not symbolic expressions.
- The production lowerer translates each job to a deterministic local integer
  origin before any binary64 construction. Authored lines and arcs retain exact
  integer point certificates; irrational authored radii receive verified
  outward square-root intervals. Disks and annuli use paired semicircles, and
  capsules use four directly constructed filtered curves, including
  arbitrary-angle and odd-width offsets. Carrier, parallel/concentric family,
  and correlated sweep tokens are minted only by this trusted lowering stage.
  The lowerer precharges curve count and canonical logical memory before
  allocation and performs no curve-pair scan. Finite-arc broad-phase bounds use
  endpoints plus only contained cardinal extrema. Proof tokens are interned in
  fixed-capacity open-address tables from exact fixed-width construction keys;
  table probes, input traversal, and geometric predicates consume one shared
  work budget. Duplicate capsules and authored/constructed equal lines
  therefore reuse carriers, while distinct parallel carriers are certified as
  such. Empty stages are bounded no-ops, and global expansion is rejected when
  a later origin translation could leave signed 64-bit public coordinates.
  Aggregate sweep-tight output bounds, including finite-arc cardinal extrema,
  must remain inside the governed job-local coordinate span.
  Swept paths use the filtered indexed arrangement for their bounded local
  piece union; the exact swept pre-arrangement is not a production fallback.
  Ordinary noncollapsed intersections between disks at consecutive authored
  vertices are admitted by exact path adjacency. Repeated vertex centers and
  any overlap or contact between nonconsecutive vertex disks fail as invalid
  centerline topology, preserving the conservative self-contact boundary.
- The filtered split/overlay stage groups only lowering-issued exact carrier
  ids, accepts only canonical broad-phase pairs, and invokes the narrow phase
  internally. No API accepts caller-constructed intersection records, so an
  unverified point cannot cross the narrow/overlay trust boundary. It sorts bounded
  endpoint/intersection events once, merges events only when their complete
  outward enclosure proves at-or-below-50-nm equivalence, and partitions each
  line or circle in canonical carrier order. Circle carriers receive both the
  canonical leftmost seam and a rightmost partition, so every published arc
  span is x-monotone for indexed face classification. A fixed-array Fenwick/indexed
  active set makes membership maintenance logarithmic and membership emission
  output-proportional; no carrier-group cross product or per-cell full-group
  scan is permitted. A count pass fixes span and membership allocation before
  publication, and arrangement, provenance, work, and target-independent
  logical-memory ceilings fail the job before overrun. Each emitted span names
  an active member whose finite domain covers it, rather than an arbitrary
  shared-carrier representative. Circle groups use the
  deterministic leftmost seam, with certified sweep tokens resolving
  cancellation-prone constructed semicircles. The preliminary circle sort uses
  a strict total scalar key; outward carrier/domain predicates then certify the
  resulting adjacency. Every internally produced narrow-phase split is
  independently rebound to both named finite curves before it can become an
  arrangement vertex. Narrow-phase work and retained pair storage remain live
  charges in the overlay's total predicate and logical-memory telemetry. The
  integrated boundary preflights the unavoidable retained pair, curve, and
  carrier-group storage plus the minimum pair/validation work before narrow
  allocation or evaluation; a job that cannot possibly fit stops with zero
  narrow work.
  Circle seam representatives never create an independent second repair: each
  event cluster retains its ordered source supports, and a synthetic cardinal
  point may represent the cluster only after every support-to-support distance
  is certified at or below 50 nm. Thus two roots on opposite sides of a seam
  cannot collapse through it when they are more than 50 nm apart.
- The filtered arrangement entry point accepts filtered geometry plus only the
  canonical broad-phase pairs and owns narrow-phase and overlay execution. No
  caller-constructed overlay or split point crosses this trust boundary. It
  reconciles the resulting carrier-local span endpoints into global vertices
  with an x sweep and a secondary-axis interval index. A new
  endpoint joins a cluster only when the complete outward distance between it
  and the cluster hull is at most 50 nm. The cluster hull, rather than a
  transitive union-find chain, is authoritative: points at 0, 40, and 80 nm
  cannot all acquire one representative. Exact-threshold box distances use
  independently evaluated endpoint differences so outward rounding does not
  turn a mathematically exact 50 nm bound into an accidental rejection.
  Edge-specific certified carrier endpoints remain attached to each edge for
  tangent decisions; a repaired global vertex enclosure never perturbs the
  carrier germ. Domains collapsed by overlay resolution and spans collapsed by
  global reconciliation both retain an explicit vertex and their complete
  membership ranges; a permitted sub-resolution feature never silently loses
  lineage. Before owned narrow/overlay execution, a zero-allocation scan of the
  lowering-issued dense first-use carrier-token stream derives unavoidable
  distinct noncollapsed spans and reserves their minimum downstream work and
  phase memory. The proportional scan is bulk-charged before traversal and its
  work remains visible in arrangement telemetry; a ceiling too small for the
  scan performs neither the scan nor upstream work. A known-doomed arrangement
  therefore reports zero overlay work and storage; later exact-count checks
  retain phase-accurate enforcement.
- The arrangement constructs twins, canonical outgoing tables, `next` and
  `previous` links, connected components, and directed boundary cycles without
  a cycle-pair scan. A strict total binary64 key is used only to satisfy the
  sort contract; adjacent outgoing germs are then certified with outward
  tangent, cross, dot, curvature, and radius predicates. Uncertain angular
  order fails the isolated job. A lowering-issued endpoint-tangent identity
  may be projected ephemerally across incident spans only through the same
  nonzero construction carrier, byte-identical full endpoint enclosure and x
  column, a unique validated tangent token, and a tokenful line on the same
  strictly positive outgoing ray. The resulting vertex-local class receives
  one immutable angle key before sorting; all resolved pair comparisons are
  audited afterward for antisymmetry and order consistency. Published tokens
  and coordinates are never mutated. If the canonical cycle germ is
  unresolved, one metered full-cycle scan may collect other certified
  right-half-plane leftmost germs; every nonzero result must agree and a
  conflict, zero, or absence fails closed. Endpoint-index visits, balanced-tree/heap
  updates, sorts, traversals, angular predicates, and phase-specific logical
  memory are governed, and the stage inherits the overlay's already consumed
  work. A direct adversarial interval-index fixture forces all earlier
  enclosures to be visited and terminates at the same exact work ceiling on
  native and WASM. This arrangement boundary does not perform containment
  tests.
- The filtered Boolean-selection stage accepts ordered request records only
  together with trusted lowered geometry and canonical broad-phase pairs, and
  owns arrangement construction. A certified vertical-slab sweep processes
  each x event column atomically, maintains crossing branches in a
  fixed-capacity implicit AVL sequence, and joins cycle sides bordering the
  same open gap with a governed disjoint set. This assigns nested and
  disconnected boundary components without cycle-pair containment tests.
  Lowering-issued construction-column identities prove correlated non-integral
  vertical coordinates. Independently produced event vertices may share an
  atomic column only while their x enclosures retain a non-empty common
  intersection and their complete y enclosures remain strictly ordered. This
  admits commuting, y-separated events without a coordinate merge or another
  repair allowance, rejects transitive x-overlap chains, and still fails closed
  on ambiguous y order. Circle branches retain the left/right seam and x-monotone
  certificates issued by overlay. A replay-only endpoint-authoritative arc may
  remain local to one atomic column only when its partition token names the
  filtered cardinal enclosure, its opposite endpoint is the named singleton
  carrier endpoint, x enclosures intersect, and complete y order is strict;
  it is omitted from open-slab status while remaining in the arrangement cycle.
  Vertical-column tokens, carrierless arcs, unnamed endpoints, conflicting
  partition identities, and ambiguous y order cannot use this rule. Face
  assignment performs no coordinate merge and consumes no additional 50 nm
  allowance.
- Face coverage is sparse. Sorted edge-membership transitions propagate from
  the unbounded face through a fixed dual adjacency structure. Canonical
  persistent binary-set roots represent complete active-operand states without
  copying an operand vector per face, and non-tree dual edges validate root
  equality exactly. An indexed stage tree updates only changed stages and
  evaluates ordered union/difference semantics incrementally, including
  add-subtract-add and zero-operand stages. The implementation therefore has no
  face-by-operand replay. Sweep/index visits, disjoint-set probes, transition
  normalization, persistent-table probes, stage updates, sorts, and output
  writes share the programmable work ceiling. Occurrences are rebound once to
  dense operand ordinals, so transition construction is linear in emitted
  memberships rather than a membership-by-log-operand lookup. Every repeated
  stage traversal is precharged, all known-size vectors use fixed reserved
  capacities, and phase telemetry reports actual canonical live storage rather
  than summing non-overlapping peaks. An allocation-free separated-short-domain
  certificate reserves unavoidable collapsed vertices and downstream sort work
  before arrangement execution. That certificate is produced by one validated,
  bulk-metered curve/bounds scan; malformed parallel arrays or point enclosures
  cannot enter downstream admission. Topology and persistent-coverage memory
  are reserved as separate live phases, outputs are counted once, and the
  guaranteed transition prefix reserves canonical coverage nodes/table storage
  before arrangement execution. A separately precharged canonical-candidate
  scan derives a conservative possible split-span and coverage-transition
  capacity before arrangement execution: line/line pairs contribute at most
  one point, line/circle and circle/circle pairs at most two, and exact shared
  authored endpoints do not count as new splits. Membership capacity starts
  from each curve's own line/partitioned-arc segments, adds both incidences of
  every possible intersection point, and, for same-carrier candidates, adds at
  most two foreign endpoints per curve. This stays `O(curves + candidates)` for
  ordinary repeated carriers such as the paired semicircles of each disk.
  Candidate pairs must also be globally sorted and unique before any derived
  allocation; malformed sequences remain `invalid_argument` at every admitted
  memory budget. Split-heavy sparse inputs therefore
  cannot complete arrangement and face topology only to fail at persistent
  coverage allocation. Malformed filtered points remain `invalid_argument`,
  independent of the resource budget, and every proportional cycle publication
  pass is precharged. Complete-cluster seam distance predicates are charged
  before evaluation. The selection result retains the lowering origin
  required to reconstruct global coordinates.
- Material-boundary extraction owns selection rather than accepting a
  caller-constructed DCEL. A single cyclic scan of each certified outgoing
  rotation assigns exactly one selected successor and predecessor for every
  half-edge separating material on the left from empty space on the right.
  Each selected half-edge is traced once. Equal-material face components and
  their material/empty boundary graph derive ring parentage and distinct
  interior-connected regions without a seam walk, cycle-pair containment, or
  point-in-ring query. Isolated collapsed vertices retain lineage but create no
  area region; point-tangent material components remain separate.
- The candidate-derived selection admission envelope also reserves the full
  possible narrow-result, overlay-event/output, arrangement, and
  material-region live-memory phases plus complete worst-case downstream
  memory before arrangement starts. The envelope is linear in curves plus
  canonical candidates and counts endpoint-coincident full-circle domains.
  Selection consumes only the remaining work; material-region work is reserved
  conservatively, then its unused remainder is released after transactional
  success. Lineage and outcome work are subsequently bounded from the actual
  retained topology before publication. Fixed-capacity region, lineage, and
  operand-history traversals consume their reservations and report actual work
  separately. Lineage follows only material-component
  dual transitions, then performs an allocation-free source-incidence count
  before fixed-capacity publication; it never unions every face's coverage set
  or scans every operand per face. Every rotation, ring, component, adjacency,
  source-membership, output, and sort pass is precharged. Native and WASM use
  identical logical charges and canonical ring/component/source ordering.
  Operand outcomes are derived during the same face-dual walk with six
  active-and-unseen stage reporters. Each `(operand, history fact)` is emitted
  at most once. The lineage walk also retains canonical unique operand/topology
  associations, so later reference projection is proportional to emitted
  ring/region associations rather than repeated occurrence-source
  multiplicity. Outcome sources remain the complete original occurrence tuples;
  ring/region references remain tagged local topology handles until packet
  assembly applies the normalizer's old-to-normalized topology maps. No
  face-by-operand replay or second geometric tolerance is permitted.
- The owned one-time normalizer publishes every material-boundary vertex on the
  global 1 nm grid, certifies its complete source hull within 50 nm, and
  reconstructs line and circular-arc fragments from those authoritative
  endpoints. Arc certification evaluates a finite filtered critical-point set
  with squared distances; it does not construct or serialize nested radicals.
  A strict zero-repair replay rebuilds material rings, parents, and regions and
  requires a one-to-one boundary/topology map. Candidate discovery and topology
  comparison are indexed, and normalization reserves its downstream work and
  logical memory before outcomes execute. Normalized arcs carry an internal,
  verified endpoint/radius/center-branch certificate. At a certified shared
  endpoint, replay factors the possible second line/circle or circle/circle
  root directly with outward intervals instead of rediscovering the known root
  through a square root. A residual root whose complete parameter enclosure is
  strictly outside either finite curve domain is discarded without spending a
  tolerance. Complete domain exclusion precedes the enclosure-width gate,
  which remains mandatory for a root that may survive on both domains; every
  root that enters or straddles both domains rejects, even within 50 nm.
  Production-owned split arcs transport carrier identity only when the
  validated nonzero source carrier, curve kind, and byte-identical full
  normalized center/radius descriptor all match. Differing normalized
  descriptors or distinct source carriers remain separate. The reconstruction
  certificate is separate from the x-monotone-half
  refinement: arcs that remain on their named half need no internal seam;
  others retain distinct internal cardinal partitions while mapping all such
  spans back to the one published fragment. Replay arrangement reconciles
  endpoint-authoritative endpoints and seams only by exact construction
  identity/equal enclosure; a collapsed such span is a topology failure. A
  deterministic endpoint/cardinal-side token may correlate overlapping x
  enclosures for atomic face-sweep processing, but cannot change coordinates
  or merge vertices. Overlay revalidates that these tokens form a dense
  canonical table ordered by side and exact integer endpoint, rejecting zero
  payloads and reused or split endpoint identities before the sweep. Generic
  irrational intersections and irrational-radius
  arcs therefore remain on the fast filtered path without a second proximity
  merge.
- The owned filtered packet stage consumes only the owned normalizer, copies
  normalized integer coordinates without another tolerance, validates the
  explicit old-topology maps and request-owned provenance, and publishes one
  canonical standalone result packet with SHA-256 closure. Production source
  types are independent of the algebraic namespace. Source sets and the other
  variable-length canonical keys use a fixed-capacity exact prefix trie with
  governed probes and scalar ranks; record sorting never compares long vectors
  or replays every operand for every face. Predictable packet work and logical
  memory are reserved before normalization, followed by exact fixed-capacity
  publication gates. A non-dispatched batch continuation now owns
  sequential job isolation and a specialized canonical job-major merge with
  global fixed-capacity source-set interning and one final encode. Its request
  validation and all retained capacities are deterministic and governed; no
  job-pair scan is introduced. Relationship queries are evaluated from the
  merged published geometry under the private strict policy. Distinct job
  pairs use a two-color indexed broad phase, cached unordered job-pair
  evaluation, strict face coverage, and edge/vertex incidence; all work,
  logical memory, candidates, cache rows, output rows, and packet bytes are
  admitted before use. Unresolved proofs fail closed as solver failures.
  Operation dispatch remains gated.
- Solver resource limits are supplied through one internal limits value object
  rather than scattered production constants. The catalog values are governed
  hard ceilings; a host may advertise and enforce lower effective limits, and
  tests may inject smaller budgets. Callers cannot change the 50 nm resolution
  semantics per request. Working-memory telemetry uses fixed conservative
  logical byte charges shared by native and WASM rather than ABI-dependent
  `sizeof` values. Fixed-capacity index and expiry storage may not grow beyond
  the reported charge.
- `boost::multiprecision::cpp_int`, canonical algebraic values, resultants, and
  root isolation are not part of the normal production path. The existing
  exact backend is isolated behind a narrow interface for conformance tests,
  offline diagnosis, and only those bounded ambiguous cases for which measured
  evidence justifies a fallback. Production correctness may not depend on
  unbounded symbolic expansion.
- Exact squared-distance comparisons should replace nested-radical distance
  construction in the retained exact arc-normalization checker. This is a
  bounded fallback and oracle improvement, not a reason to restore the
  algebraic engine as the primary solver.
- OCCT and Clipper2 may remain differential or feasibility oracles. Neither is
  an independent structural authority for the public result.

The production `geometer_lib` target excludes the exact/algebraic and exact
topology-oracle sources; those compile only in the test-only
`geometer_exact_feasibility` target on native and WASM builds.

The generic TypeSpec shapes and packed A0 record layout remain frozen. The
solver and numeric policy are reopened until the filtered implementation,
50 nm threshold suite, performance corpus, and native/WASM parity gates pass.
Production dispatch remains disabled during that work.

## Consequences

- Ordinary PCB-scale line/arc work avoids canonical symbolic construction and
  receives predictable bounded behavior.
- Sub-resolution topology is intentionally not guaranteed. Diagnostics and
  tests must distinguish an allowed at-or-below-50-nm merge, an explicitly
  reported same-width capsule coalescence at or below 1 um, and an error that
  changes topology outside those scoped envelopes.
- The exact code is no longer allowed to dictate production module structure.
  It may be simplified, isolated, or removed from production linkage as the
  filtered implementation lands, provided the retained oracle/fallback tests
  remain independently useful.
- Existing MATZ observations that require distinct topology below 50 nm are
  historical consumer evidence, not the new production acceptance rule. They
  require consumer re-review when that external worktree is available.
- Performance qualification must include dense real-board geometry and must
  report broad-phase candidates, narrow-phase predicates, fallback use, wall
  time, and peak memory.

## Alternatives Considered

- **Adopt an existing line/arc Boolean engine without adaptation.** A review of
  Cavalier Contours 0.8.0 found reusable analytic intersection, overlap,
  slicing, stitching, and spatial-index work, but not the governed 50 nm
  certification, n-ary region/stage semantics, canonical bytes, resource
  accounting, or provenance required here. Its pairwise classifier also has
  whole-contour winding scans that can become quadratic. Before further custom
  arrangement work, a bounded reuse spike may compare its algorithms and
  slice metadata against Geometer fixtures. C++ remains authoritative under
  issue 18; adding a Rust production kernel would require a separate approved
  architecture decision. If adaptation requires replacing the upstream
  predicates, classifier, stitcher, and lineage flow, retain it only as a
  differential oracle/reference. The bounded follow-up compared upstream
  commit `8f0b85739d65a12128200f3d064f41547b76244c`: its maintained Rust core and
  C FFI are about 12,194 and 2,420 lines respectively, while the three carrier
  intersection modules are only about 460 lines and still use caller epsilon
  rather than outward certification. The superseded C++ implementation has
  the same fuzzy-threshold issue. Importing either implementation would add
  more integration and replacement work than the focused governed kernel, so
  production adopts the direct C++ module and cites Cavalier only as prior art.
- **Keep exact algebraic evaluation primary.** Rejected because its complexity
  and resource behavior do not match the speed-first product requirement.
- **Only raise algebraic memory limits or loosen the old Hausdorff check.**
  Rejected because it leaves symbolic expansion and pair enumeration on the
  hot path.
- **Use unchecked floating-point tolerances.** Rejected because platform drift
  and silent topology changes would undermine native/WASM determinism.
- **Polygonize all arcs.** Rejected because analytic line/arc output is a core
  requirement for documentation, viewers, and manufacturing consumers.

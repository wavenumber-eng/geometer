# ADR-013: Filtered, Resolution-Bounded Analytic Planar Booleans

## Status

Accepted for implementation by project-owner direction on 2026-08-15. This
decision supersedes ADR-012 for the production solver architecture. It does not
promote `geometry.analytic_planar_boolean_batch.a0` to production.

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
  displacement bound fails the isolated job. Finite-domain repairs are
  certified pairwise: when both carrier intersections lie beyond their finite
  curves, actual endpoint witnesses must be at most 50 nm apart. Two
  independently sub-50-nm endpoint distances may not compose into a bridge
  above the envelope, and a domain predicate that merely straddles an endpoint
  fails closed. Its internal curves carry bounded
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
  Swept paths fail job-locally as unsupported until their piece union is
  implemented by the filtered indexed arrangement; the exact swept
  pre-arrangement is not a production fallback.
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
  order fails the isolated job. Endpoint-index visits, balanced-tree/heap
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
  vertical coordinates; unrelated overlapping coordinate enclosures fail
  closed. Circle branches retain the left/right seam and x-monotone
  certificates issued by overlay. Face assignment performs no coordinate
  merge and consumes no additional 50 nm allowance.
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
  material-region live-memory phases plus a conservative downstream work
  budget before arrangement starts. The envelope is linear in curves plus
  canonical candidates and counts endpoint-coincident full-circle domains.
  Selection consumes only the remaining work;
  fixed-capacity region traversals consume the reservation and report actual
  work separately. Every rotation, ring, component, adjacency, output, and
  sort pass is precharged. Native and WASM use identical logical charges and
  canonical ring/component ordering. Complete lineage projection, final grid
  normalization, and dispatch remain gated.
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

The generic TypeSpec shapes and packed A0 record layout remain frozen. The
solver and numeric policy are reopened until the filtered implementation,
50 nm threshold suite, performance corpus, and native/WASM parity gates pass.
Production dispatch remains disabled during that work.

## Consequences

- Ordinary PCB-scale line/arc work avoids canonical symbolic construction and
  receives predictable bounded behavior.
- Sub-resolution topology is intentionally not guaranteed. Diagnostics and
  tests must distinguish an allowed at-or-below-50-nm merge from an error that
  changes topology above 50 nm.
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

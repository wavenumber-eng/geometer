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
  line or circle in canonical carrier order. A fixed-array Fenwick/indexed
  active set makes membership maintenance logarithmic and membership emission
  output-proportional; no carrier-group cross product or per-cell full-group
  scan is permitted. A count pass fixes span and membership allocation before
  publication, and arrangement, provenance, work, and target-independent
  logical-memory ceilings fail the job before overrun. Circle groups use the
  deterministic leftmost seam, with certified sweep tokens resolving
  cancellation-prone constructed semicircles. The preliminary circle sort uses
  a strict total scalar key; outward carrier/domain predicates then certify the
  resulting adjacency. Every internally produced narrow-phase split is
  independently rebound to both named finite curves before it can become an
  arrangement vertex. Narrow-phase work and retained pair storage remain live
  charges in the overlay's total predicate and logical-memory telemetry.
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

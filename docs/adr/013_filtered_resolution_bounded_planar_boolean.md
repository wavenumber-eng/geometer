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
  differential oracle/reference.
- **Keep exact algebraic evaluation primary.** Rejected because its complexity
  and resource behavior do not match the speed-first product requirement.
- **Only raise algebraic memory limits or loosen the old Hausdorff check.**
  Rejected because it leaves symbolic expansion and pair enumeration on the
  hot path.
- **Use unchecked floating-point tolerances.** Rejected because platform drift
  and silent topology changes would undermine native/WASM determinism.
- **Polygonize all arcs.** Rejected because analytic line/arc output is a core
  requirement for documentation, viewers, and manufacturing consumers.

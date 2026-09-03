+++
type = "plan"
id = "fast-hlr-backend"
status = "active"
created = "2026-09-02"

[[steps]]
id = "gpu-feasibility-spike"
title = "Prove retained depth-tested mesh edges in the Illustration Lab and record small and large fixture timings"
status = "done"

[[steps]]
id = "early-design-review"
title = "Complete independent plan review and resolve the evaluation architecture, silhouette, option, simplification, and output-contract findings"
status = "done"
depends_on = ["gpu-feasibility-spike"]

[[steps]]
id = "baseline-and-budgets"
title = "Measure exact, poly, mesh-shadow, and retained GPU stages over a governed fixture corpus and freeze fast-backend budgets"
status = "done"
depends_on = ["early-design-review"]

[[steps]]
id = "fast-backend-contract"
title = "Specify the additive evaluation API, packed prepared-data transport, provisional fast-only options, limits, and timing channel"
status = "active"
depends_on = ["baseline-and-budgets"]

[[steps]]
id = "generic-prepared-mesh"
title = "Implement a renderer-neutral prepared mesh and projected-triangle representation reusable across views"
status = "done"
depends_on = ["fast-backend-contract"]

[[steps]]
id = "candidate-edge-graph"
title = "Build bounded topology-aware candidates, including intra-face triangulation adjacency for smooth-surface silhouettes"
status = "done"
depends_on = ["generic-prepared-mesh"]

[[steps]]
id = "vector-visibility-core"
title = "Implement deterministic indexed segment-versus-triangle visibility intervals for fast detail output"
status = "done"
depends_on = ["candidate-edge-graph"]

[[steps]]
id = "fast-mesh-shadow-outline"
title = "Implement a separately selectable prepared-mesh outline path because the current Clipper2 triangle union misses the frozen BGA90 budget"
status = "done"
depends_on = ["baseline-and-budgets"]

[[steps]]
id = "path-reconstruction"
title = "Reconstruct visibility-safe lines and bounded circular arcs, with post-change occlusion validation"
status = "pending"
depends_on = ["vector-visibility-core"]

[[steps]]
id = "projection-integration"
title = "After evaluation, add fast as a parallel C++ and JSON projection algorithm and route CLI, C ABI, WASM, and Python option passthrough"
status = "done"
depends_on = ["path-reconstruction"]

[[steps]]
id = "browser-fast-hlr"
title = "Adapt the shared prepared-edge semantics to the retained browser GPU renderer and Illustration Lab"
status = "pending"
depends_on = ["candidate-edge-graph", "fast-backend-contract"]

[[steps]]
id = "performance-hardening"
title = "Batch work, parallelize independent candidates, bound memory, and qualify native and WASM performance"
status = "pending"
depends_on = ["projection-integration", "browser-fast-hlr"]

[[steps]]
id = "design-doc-intent-audit"
title = "Audit design, requirement, ADR, and interface documentation against the intended additive fast-backend behavior"
status = "pending"
depends_on = ["projection-integration", "browser-fast-hlr"]

[[steps]]
id = "test-runtime-impact-audit"
title = "Audit correctness coverage, Rack registration, benchmark evidence, and test-suite runtime impact"
status = "pending"
depends_on = ["performance-hardening"]

[[steps]]
id = "external-review"
title = "Complete independent clean-room, implementation, numerical robustness, and performance review"
status = "pending"
depends_on = ["design-doc-intent-audit", "test-runtime-impact-audit"]

[[steps]]
id = "durable-docs-and-closeout"
title = "Update ADRs, requirements, design and interface docs, release evidence, then remove the completed temporary plan"
status = "pending"
depends_on = ["external-review"]

[[exit_criteria]]
id = "additive-backend"
title = "Fast is available alongside exact and poly without changing their defaults or supported behavior"
status = "met"

[[exit_criteria]]
id = "compatible-projection"
title = "Fast returns geometry.projection.b0 outline, detail, and bbox data through the existing projection surfaces"
status = "met"

[[exit_criteria]]
id = "independent-layers"
title = "Callers can request outline, detail, or both, and combined results preserve separate independently composable geometry layers"
status = "met"

[[exit_criteria]]
id = "warm-view-budget"
title = "Prepared-mesh fast HLR completes the governed medium fixture corpus in less than 100 ms per view at p95"
status = "met"

[[exit_criteria]]
id = "outline-quality"
title = "Fast mesh-shadow outline preserves projected holes and stays within its documented geometric tolerance"
status = "met"

[[exit_criteria]]
id = "detail-quality"
title = "Fast visible and optional hidden detail correctly handles occlusion, silhouettes, creases, instances, and self-occlusion"
status = "pending"

[[exit_criteria]]
id = "compact-paths"
title = "Fragment chaining and bounded simplification materially reduce output segments without exceeding declared error"
status = "pending"

[[exit_criteria]]
id = "deterministic"
title = "Native repeated runs and native-versus-WASM runs produce equivalent canonical geometry within the governed policy"
status = "pending"

[[exit_criteria]]
id = "interactive-raster"
title = "The browser GPU adapter retains sub-frame visibility updates without rerunning STEP import or OCCT HLR"
status = "met"

[[exit_criteria]]
id = "bounded"
title = "Work, memory, malformed mesh input, cancellation boundaries, and over-limit behavior fail deterministically"
status = "pending"

[[exit_criteria]]
id = "no-regression"
title = "Exact, poly, existing mesh-shadow, CLI, Python, WASM, and packaged demos pass their existing validation"
status = "pending"

[[exit_criteria]]
id = "design-doc-intent-audit"
title = "The design-document intent audit confirms durable docs describe the delivered additive behavior and compatibility boundaries"
status = "pending"

[[exit_criteria]]
id = "test-runtime-impact-audit"
title = "The test runtime-impact audit confirms coverage, Rack placement, benchmark evidence, and acceptable suite cost"
status = "pending"

[[exit_criteria]]
id = "external-review"
title = "Independent clean-room, implementation, numerical robustness, and performance review findings are resolved"
status = "pending"

[[exit_criteria]]
id = "signoff"
title = "Focused checks, Rack strata, L99 release gate, native validation, package validation, and independent review pass"
status = "pending"
+++

# Parallel Fast HLR Backend

## Purpose

Add a third hidden-line projection implementation named `fast` alongside the
existing `exact` and `poly` implementations. The work does not select a single
preferred implementation, replace OCCT HLR, retire an existing algorithm, or
move downstream illustration policy into Geometer. It delivers an additional
backend that callers can choose when bounded latency matters.

The fast detail backend must compose with both of Geometer's existing outline
choices:

- `hlr-close` or `mesh-shadow` outline, selected independently by
  `ProjectionOutlineAlgorithm`;
- fast visible and optionally hidden feature, crease, boundary, and silhouette
  detail linework.

Outline and detail are independent products, not stages of one merged path.
Callers must be able to request outline only, detail only, or both. When both
are requested, `geometry.projection.b0` retains them in separate `outline` and
`detail` members so downstream documentation and illustration code can apply
different strokes, fills, opacity, ordering, and other presentation policy.
Neither implementation may merge, simplify, or deduplicate geometry across
that layer boundary.

It must return the existing renderer-neutral projection values so JSON, SVG,
the CLI, Python, native C++, and browser WASM can use it without learning a
GPU-specific framebuffer format.

## Current evidence

The 2026-09-02 Illustration Lab spike proves the fast raster architecture:

- TypeScript builds retained boundary/crease candidates once from the loaded
  GLB;
- opaque faces populate the WebGL depth buffer;
- line fragments use the GPU depth test on every frame;
- camera motion performs no STEP import, tessellation, OCCT HLR, projected
  triangle sorting, or SVG regeneration;
- the 24,150-triangle BGA90 fixture produced 7,250 candidate edges, a 17.5 ms
  one-time edge build, 3.24 ms mean CPU submission, and 0.27 ms mean GPU time
  on the measured Radeon RX 7600 XT hardware path.

The spike establishes feasibility, not the public implementation. It uses
position-hashed Three.js edges, one-pixel line primitives, and a coarse polygon
offset. It does not yet produce vector fragments or topology-derived smooth
silhouettes.

The Alibre/HOOPS research archive provides a clean-room architectural
reference. HOOPS FastHiddenLine is a deferred multi-pass z-buffer display path;
HOOPS also has a separate CPU projected-triangle/line visibility engine using a
spatial index and line-parameter intervals. Geometer will use only those
functional requirements and standard geometry techniques, not decompiler
expressions or proprietary source structure.

### Governed native baseline

`scripts/benchmark_fast_hlr.py` records native and Node-hosted WASM artifact
digests, environment, warmup/repeat policy, phase timings, canonical geometry
digests, and output counts for a fixed SOT-23, SOIC-8, SOT-223, TSOT-23-5, and
BGA90 corpus. The initial Windows x64 run used
commit `59f62a73d487fc728e8a2fc2a26e9b8a3089c9ee`, the current native build, one
warmup, and three measured one-shot processes. Its selected p95 results were:

| Fixture | Detail / outline | Wall | STEP read | Mesh | HLR | Extract |
|---|---|---:|---:|---:|---:|---:|
| SOT-23 | poly / HLR-close | 66.3 ms | 27.0 ms | 11.0 ms | 0.6 ms | 1.0 ms |
| SOIC-8 | poly / HLR-close | 104.8 ms | 56.1 ms | 20.0 ms | 1.9 ms | 2.8 ms |
| TSOT-23-5 | poly / HLR-close | 198.8 ms | 111.8 ms | 44.0 ms | 6.7 ms | 4.1 ms |
| BGA90 | poly / HLR-close | 431.6 ms | 189.4 ms | 99.9 ms | 23.7 ms | 79.6 ms |
| BGA90 | poly / mesh-shadow | 863.8 ms | 195.3 ms | 104.7 ms | 20.3 ms | 503.3 ms |
| BGA90 | exact / HLR-close | 12,440.9 ms | 186.8 ms | 0.0 ms | 2,839.0 ms | 9,385.2 ms |
| BGA90 | exact / mesh-shadow | 10,655.2 ms | 183.1 ms | 98.7 ms | 2,823.8 ms | 7,512.4 ms |

These are one-shot reference values, not prepared-view targets. Exact timing
variation is material at this sample count, but the order-of-magnitude result
is unambiguous. The existing timing schema also combines detail extraction and
outline replacement, so the mesh-shadow row cannot isolate outline perfectly.
Against the poly/HLR-close comparison it adds roughly 0.4 seconds on BGA90 and
therefore requires a separate optimized outline track. The ignored raw report
is `.bench-tmp/fast-hlr-baseline.json`; it is reproducible from the committed
harness rather than treated as release evidence.

The first independent-layer implementation then measured BGA90 with one warmup
and three repetitions. Poly detail alone had p95 22.4 ms HLR plus 3.6 ms
extraction after reusable preparation. HLR-close outline had p95 24.0 ms HLR
plus 68.8 ms extraction. Mesh-shadow outline bypassed HLR as intended but spent
371.1 ms in its projected-triangle union. This freezes two priorities: retain a
sub-100 ms prepared fast-detail budget, and replace the separately selectable
mesh-shadow triangle union with a faster equivalent outline implementation.

## Early independent review resolution

An independent read-only review on 2026-09-02 compared this plan with the
current C++, JSON/C ABI, mesh-shadow, SVG, and browser prototype. It found five
decisions that had to be resolved before production implementation. The
evaluation phase adopts these resolutions:

1. Geometry preparation, adjacency, semantic edge candidates, and vector
   visibility are centralized in generic C++. The browser does not maintain an
   independent authoritative Three.js adjacency algorithm.
2. C++ exports a versioned packed prepared-data payload for the browser. It
   contains the model-space triangles, adjacency, candidate records, normals,
   provenance, and declared limits needed by the retained GPU adapter. This is
   an evaluation transport, not yet a promised stable public format or
   persistent model-handle API.
3. Smooth-face silhouettes are generated from triangle adjacency within CAD
   faces and activated per view when adjacent projected orientations oppose.
   Analytic face-silhouette tracing is not required for the first evaluation.
4. `fast` starts with a separate provisional option block. It does not claim
   that OCCT-specific flags have fast equivalents. The evaluation will
   determine which controls deserve a durable contract before the normal
   projection API is extended.
5. Evaluation output remains `geometry.projection.b0`: independent straight
   segments and circular arcs. Connected paths, Beziers, and splines are
   deferred until measured output demonstrates that lines and arcs are
   insufficient.

Simplification is never allowed to bridge a hidden interval. Any replacement
line or fitted arc must pass a second visibility check against the same index,
and the combined tessellation, visibility, fitting, simplification, and
rounding error budget must be reported. The final `external-review` gate remains
pending for review of delivered code and evidence; this early review does not
satisfy that gate.

### First engine implementation review

The first generic C++ engine slice received a second independent read-only
review on 2026-09-02. Before checkpointing the slice, the implementation was
hardened to preserve disconnected coincident components, validate all public
prepared-mesh indices and incidence spans, use dimensionally consistent
projected-length tolerances, bound spatial-index references, cap JSON limits to
the portable 32-bit range, validate numeric options, retain grazing and
non-manifold candidates conservatively, and canonicalize output through the
existing rounding/deduplication policy. Tests now cover malformed input,
resource failure, opposite view depth, oblique projection, sloped depth
crossing, grazing silhouettes, and coincident disconnected OCCT shapes.

This checkpoint does not complete the contract step: the versioned packed
prepared-data transport is still pending, so the existing Three.js GPU path is
an explicitly non-authoritative comparator. Path reconstruction is active.

The first reconstruction slice preserves visibility fragments until a focused
post-process can join exact-collinear runs. A join is permitted only through a
real prepared-mesh vertex, with identical edge-category and source-face
provenance, and only at an unbranched degree-two continuation. Occlusion-created
endpoints carry no topology token, so visible reconstruction cannot bridge a
hidden interval. General tolerance-based simplification and circular-arc fitting
remain pending because they require replacement-geometry visibility checks.
The next evaluation slice adds an opt-in coplanar-continuation filter with
separate angle, view-depth, and lateral-probe tolerances. It records suppressed
parameter intervals separately from hidden intervals and retains ambiguous,
opposed-normal, unsupported, and same-side coincident boundaries.

### Fast mesh-shadow checkpoint

The reviewed fast outline implementation keeps `mesh-shadow` unchanged and adds
`fast-mesh-shadow` as an independently selectable evaluation backend. It
normalizes projected triangle winding, opportunistically reconstructs CAD-face
triangle-patch loops, and uses an associative hierarchical fallback: union
projected triangles within each CAD face, then union the reduced face contours
globally. The fallback preserves the existing triangle-shadow semantics while
avoiding one monolithic boolean over the entire tessellation.

After independent review, all five governed fixtures pass top/front closure,
bounds, aggregate area, and per-loop signed-area parity against the existing
triangle union. On BGA90, the existing all-triangle union spent 363-389 ms p95
in outline extraction while the corrected hierarchical implementation spent
17.93 ms p95. A combined fast-detail plus fast-outline request spent 89.87 ms
p95 in the two prepared view phases, below the frozen 150 ms target. Across the
five-fixture governed package corpus, fast outline extraction p95 ranged from
0.21 ms to 17.93 ms; one-shot wall time remains dominated by STEP import and
mesh preparation.

The evaluation implementation checks configured prepared-input and final-output
limits, malformed indices, coordinate ranges, and strict segment insertion.
Clipper2 overlay itself is not interruptible, so an adversarial polygon overlay
can still consume substantial intermediate time or memory before the final
output limit is observed. Hardening that internal work bound remains part of the
pending resource-safety exit criterion rather than blocking this evaluation
checkpoint.

### Native versus WASM checkpoint

The 2026-09-03 runtime matrix runs the same one-shot batch requests through the
Windows x64 `geometer.exe` and the Node-hosted browser WASM build, with one
warmup and three measured repetitions. Fast detail geometry was byte-equivalent
across runtimes on all five governed package fixtures. Exact HLR differed on
TSOT-23-5 by one detail primitive (1,655 native versus 1,654 WASM); this is an
existing exact-backend discrepancy rather than a fast-backend difference.

For BGA90 detail-only, p50 internal HLR time was 59.63 ms native and 114.98 ms
WASM for fast, 15.52 ms and 27.26 ms for poly, and 2,308.23 ms and 12,490.64 ms
for exact. One-shot p50 wall time was respectively 329.92 ms and 1,466.46 ms,
277.65 ms and 1,454.39 ms, and 2,485.54 ms and 13,427.56 ms. WASM STEP import
and module/process startup dominate small and medium one-shot requests.

The combined fast-detail plus fast-mesh-shadow corpus also remained
byte-equivalent. On BGA90, p50 prepared view work was 77.62 ms native and
121.92 ms WASM; the corresponding summed per-phase p95 values were 79.43 ms and
123.27 ms. One-shot p50 wall time was 370.13 ms and 1,306.43 ms. Raw reports are
ignored reproducible evidence under `.bench-tmp/fast-hlr-native-wasm-detail.json`
and `.bench-tmp/fast-hlr-native-wasm-combined.json`.

These results support the native prepared-view target and show that the first
single-threaded WASM build misses the sub-100 ms BGA90 combined-view target.
They also make reusable prepared-data transport material to the publishable
browser interface: reparsing STEP and rebuilding preparation for every view is
not representative of the retained fast architecture.

## Execution architecture

The alternate implementation has one shared preparation layer and two output
adapters:

```text
STEP shape or generic indexed mesh
            |
            v
central C++ prepared triangles + candidate-edge graph
            |
            +-------------------------------+
            |                               |
            v                               v
CPU/WASM vector visibility             browser GPU raster
            |                               |
            v                               v
geometry.projection.b0                interactive pixels
outline/detail/bbox                   using the same edge classes
            |
            v
JSON / SVG / CLI / Python
```

The C++ vector path is the authority for projection JSON and SVG. It must work
without a GPU and produce deterministic geometry natively and under WASM. The
GPU path is a renderer acceleration and behavioral visualization of the same
candidate-edge semantics. GPU readback or pixel tracing is not the authority
for documentation vectors.

### Where computation occurs

- OCCT remains responsible for STEP import and tessellation in the STEP
  adapter.
- Generic C++ builds the prepared mesh, complete adjacency and candidate-edge
  graph, projected spatial index, vector visibility intervals, outline paths,
  and simplified output.
- A C ABI/WASM byte-buffer adapter serializes the versioned evaluation payload;
  the browser uploads it to retained GPU buffers and uses the hardware depth
  test while the camera moves.
- TypeScript owns browser integration and display controls, not authoritative
  CAD topology, candidate generation, or vector reconstruction.

The GPU materially helps interactive display because depth comparison is
performed independently per covered pixel in parallel. It is not required for
the sub-100 ms vector target, and it does not by itself yield clean model-space
SVG paths.

## Relationship to mesh shadow

Mesh shadow does not require hidden-edge classification. Its semantic result is
the union of all projected triangle footprints, including preserved holes. The
current implementation in `mesh_shadow_outline.cpp` projects every triangle,
unions them through Clipper2, simplifies the paths, and emits one segment for
each remaining path edge.

Fast detail and outline selection form an explicit compatibility matrix:

```text
projection_algorithm = exact | poly | fast
outline_algorithm    = hlr-close | mesh-shadow | fast-mesh-shadow
```

Layer selection is orthogonal to that matrix. An outline-only request must not
run detail visibility, and a detail-only request must not run mesh-shadow or
HLR-close construction. A combined request may reuse prepared mesh/view data,
but it emits and times the two layers independently.

The baseline shows that the existing Clipper2 union exceeds the BGA90 outline
budget. Implement a focused projected-mesh boundary solver with equivalent
mesh-shadow semantics and compare it against the existing union as an oracle.
Keep the existing implementation available during evaluation. Boolean-created
outline vertices use loop-topology reconstruction and are not subject to detail
edges' source-edge identity rules.

A GPU mask-and-contour implementation may remain a diagnostic or interactive
outline option, but it is resolution-dependent and is not the first
documentation/SVG authority.

## Public surface

The evaluation begins behind a direct C++ value API and a versioned
C ABI/WASM byte-buffer adapter. After its semantics are measured, subject to
the normal interface review, extend the existing enum and JSON option
additively:

```cpp
enum class ProjectionAlgorithm {
    Poly,
    Exact,
    Fast,
};
```

```json
{
  "projection_algorithm": "fast"
}
```

Do not change the default as part of this plan. Existing `poly` and `exact`
requests retain their behavior. `fast` returns the existing
`HlrProjectionResult` / `geometry.projection.b0` structure and continues to use
`ProjectionOutlineAlgorithm` to select closed-HLR, existing mesh-shadow, or
fast mesh-shadow outline semantics.

The provisional evaluation contract reuses only genuinely common inputs: view,
model transform, tessellation controls, independent output-layer selection,
outline selection, and output rounding.
Fast-only controls live below a separate `fast` option block and are explicitly
provisional. Initial candidates are:

- boundary, crease, silhouette, and hidden-category inclusion;
- crease angle;
- visibility/depth tolerance in projected model units;
- output simplification tolerance;
- circular-arc fitting tolerance and enable flag;
- deterministic work and memory ceilings.

OCCT-specific smooth, sewn, parametric-isoline, native-source-arc, and HLR gap
closure options do not silently acquire new meanings. During evaluation they
are outside the fast contract. Before integration, publish a matrix marking
every existing option as common, fast-supported, approximated, rejected,
delegated, or inapplicable. This evidence-driven contract decision is part of
`projection-integration`, not a prerequisite for experimenting with useful
fast controls.

Thread count, tile size, spatial-index fanout, and other implementation details
remain internal unless evidence proves that callers need them.

The CLI, C ABI, WASM compatibility function, and Python wrapper already pass an
options JSON object. The initial public integration should use that route and
the existing projection result schema rather than introduce a second operation.
The packed evaluation payload removes the need for a process-global cache or a
stable persistent model handle. A durable prepared-model session API remains a
separate follow-on decision.

## Generic prepared mesh

Create small responsibility-focused modules under `src/cpp/lib/`; do not add
the implementation to the existing `hlr_projection.cpp` catchment.

The prepared representation should contain:

- finite definition-local vertex positions and indexed triangles;
- face normals and optional per-vertex normals;
- triangle-to-face/body/occurrence or generic source identities when present;
- instance transforms, including mirrored transforms;
- projected triangle bounds and depth-plane coefficients per view;
- candidate polylines with adjacent triangle/face identities and semantic
  classes;
- explicit byte, triangle, edge, segment, instance, and candidate-pair limits.

Separate immutable model preparation from view preparation. A direct C++ value
API should allow multiple view projections to reuse the tessellation and edge
graph even though the existing one-shot STEP wrapper may rebuild them per call.
No process-global mutable cache is required.

### Candidate edges

For STEP, construct candidate polylines while OCCT topology and face
triangulations are both available. Prefer tessellated source edges and their
adjacent faces over global position welding. Also retain triangle adjacency
inside each CAD face so a view can activate a mesh edge when adjacent triangle
facings oppose; this supplies approximate silhouettes through the interior of
cylinders, cones, spheres, tori, and other smooth or trimmed faces. Preserve a
stable source identity within result construction so unrelated candidates are
never merged.

Classify at least:

- open boundary edges;
- manifold sharp/crease edges;
- smooth/sewn edges when requested;
- view-dependent silhouettes from opposing face orientation;
- non-manifold edges under a deterministic documented policy;
- optional hidden candidates using the same categories as current detail
  options.

For arbitrary indexed meshes without CAD topology, provide a separate generic
adjacency builder with explicitly scoped positional welding. Material, object,
instance, and authored-normal seams must not merge accidentally.

## Vector visibility algorithm

The first supported camera is the existing orthographic HLR view. Keep camera
and depth conventions identical to current projection output.

For each view:

1. transform and project triangles and candidate polyline segments;
2. reject degenerate and out-of-range primitives;
3. build a deterministic spatial index over projected triangle bounds;
4. query overlapping triangles for every candidate segment;
5. exclude incident triangles from self-occlusion;
6. clip the line parameter domain against each projected triangle;
7. solve depth ordering at interval endpoints and internal depth crossings;
8. union hidden parameter intervals and emit their visible complement, or emit
   both sets when hidden output is requested;
9. preserve source identity, edge class, and source parameter range on every
   fragment until final reconstruction;
10. sort and merge results deterministically after parallel work completes.

Use flat arrays and arenas rather than one heap allocation per interval. The
spatial index may be a uniform tiled grid, BVH, or loose quadtree; choose it by
corpus measurement. Candidate segments are independent after view preparation
and may run in parallel natively. WASM starts with deterministic single-threaded
execution unless the shipped build and deployment headers support the required
thread model.

Numerical decisions must distinguish:

- projected 2D containment tolerance;
- depth ordering tolerance;
- source/incident-face self-occlusion bias;
- final output rounding and simplification tolerance.

Do not use one scale-relative epsilon for all four decisions. Test very small,
very large, shallow-angle, and nearly coplanar fixtures.

## Path reconstruction and curves

The visibility core will naturally emit many short fragments. Compact output is
therefore a required stage, not downstream cleanup.

Reconstruct in this order:

1. group fragments by source edge, semantic class, visibility, instance, and
   style-relevant provenance;
2. order them by original source parameter where available;
3. join contiguous ranges within the declared endpoint tolerance;
4. remove zero-length and exact duplicate fragments;
5. merge collinear runs using a perpendicular-error and direction test;
6. simplify remaining polylines with a bounded Hausdorff-style error while
   preserving required corners and loop closure;
7. optionally fit circular arcs only when every sample stays within the fit
   tolerance, radial residual is bounded, sweep direction is unambiguous, and
   the fit does not cross a source-identity or visibility boundary;
8. reclassify every replacement line or arc against the visibility index and
   reject it if it enters an occluded interval or violates the combined error
   budget;
9. emit the existing `ProjectedSegment` and `ProjectedArc` values and apply
   normal result rounding.

Do not promise recovery of the source analytic curve from an arbitrary mesh.
An accepted fitted arc is a bounded approximation. When exact source arcs are
required, the existing exact backend remains available.

The first evaluation intentionally treats one compact line or circular arc as
the available output unit. It does not add connected-polyline, Bezier, or spline
schema members. Record cases where that restriction prevents useful
compression so a later schema decision is based on evidence.

Track raw candidates, raw visible fragments, chained paths, emitted segments,
emitted arcs, rejected fits, and simplification error in diagnostic telemetry
or benchmark output. Timing and counters do not enter canonical projection
JSON unless the existing timing policy explicitly includes them.

## Performance budgets

The principal product requirement is sub-100 ms projection after STEP import
and mesh preparation. Measure first-call and warm-view costs separately:

- `step_read_ms`: STEP/XCAF import;
- `mesh_ms`: tessellation;
- `edge_graph_ms`: reusable candidate construction;
- `view_prepare_ms`: transform, projection, and spatial index;
- `visibility_ms`: interval classification;
- `outline_ms`: mesh-shadow or alternate outline construction;
- `detail_ms`: fast candidate visibility and reconstruction;
- `simplify_ms`: chaining, line merging, and arc fitting;
- `serialize_ms`: JSON/SVG writing;
- total one-shot elapsed time.

Frozen evaluation targets are:

- BGA90-class prepared mesh: fast detail visibility and reconstruction p95
  below 100 ms per view;
- BGA90-class prepared mesh: separately requested mesh-shadow outline p95 below
  100 ms per view;
- a combined outline-plus-detail request: p95 below 150 ms on BGA90 while
  retaining separate result layers;
- governed medium corpus: each separately requested prepared layer has p95
  below 100 ms and no case exceeds 250 ms without an explicit over-complexity
  diagnostic;
- repeated identical prepared view: byte-equivalent geometry and no unbounded
  retained allocation growth;
- interactive browser raster at 1080p: p95 CPU submission below 8 ms and GPU
  elapsed time below 4 ms on the reference hardware;
- one-time candidate preparation: linear or n-log-n scaling over the governed
  triangle/edge range;
- peak transient memory: bounded by an explicit multiple of admitted mesh and
  candidate records, with no linked-node allocation proportional to every
  triangle/segment pair.

The 100 ms claim applies to the HLR work on a prepared mesh. Report one-shot
STEP parse and tessellation separately so a slow import is not mislabeled as a
visibility regression or hidden from end-to-end users.

## Benchmark corpus and comparisons

Use the embedded-model manifest plus generated adversarial fixtures. The corpus
must include:

- SOT-23, SOIC, SOT-223, TSOT, and BGA90 package models;
- a cube with known crease, silhouette, visible, and hidden sets;
- cylinders, cones, spheres, tori, and trimmed smooth faces at several
  tessellation densities and view angles;
- one line crossing an occluding triangle and one depth-order crossing inside
  a projected overlap;
- open, non-manifold, and duplicate-vertex meshes;
- repeated and mirrored instances;
- coincident and nearly coplanar bodies;
- nested projected holes for mesh-shadow outline;
- very small and very large coordinate ranges;
- a large repeated assembly sized to expose index and batching behavior.

Record `exact`, `poly`, `fast`, mesh-shadow-only, and browser GPU results from
the same source/options where meaningful. Exact/poly output is a comparison and
oracle input, not a requirement for segment-for-segment identity. Governed
checks should compare visibility classifications, loop topology, bounds,
Hausdorff distance, and category counts under declared tolerances.

## Validation

### C++ foundation

- Unit tests for projection basis, triangle depth planes, 2D clipping,
  line-parameter interval union/complement, incident-face exclusion, depth
  crossings, deterministic spatial-index queries, and work limits.
- Edge-graph tests for boundaries, smooth pairs, creases, silhouettes,
  non-manifold topology, face-local duplicate vertices, and mirrored instances.
- Path tests for ordering, chaining, collinear merging, loop closure,
  simplification error, accepted circular arcs, and rejected false fits.
- Visibility regression tests prove replacement lines and arcs cannot bridge a
  narrow hidden interval or move across an occluder.
- Partial-arc bounds tests cover every crossed cardinal extremum before fitted
  arcs are enabled, correcting the current endpoint-only bounds behavior.
- Mesh-shadow tests for solid outlines, holes, disjoint bodies, overlapping
  instances, degenerates, and equivalence to the current Clipper2 oracle.
- Fixture comparisons against exact/poly under explicit geometric rather than
  byte-identical policy.

### Interfaces and transports

- JSON option parsing accepts `fast` and rejects unknown algorithms.
- Layer selection exercises outline-only, detail-only, and combined requests;
  combined output remains separately composable and avoids duplicated work.
- Existing CLI JSON and SVG commands execute fast on named and arbitrary
  orthographic views.
- Direct C++ and C ABI byte-buffer calls return the same projection schema.
- Native and browser WASM results satisfy the governed equivalence policy.
- Python option passthrough exercises `fast` through the bundled executable.
- Existing exact/poly vectors remain unchanged.

### Browser

- The Illustration Lab renders nonblank GPU HLR for bundled and uploaded STEP.
- Camera motion in GPU mode does not increment a STEP/OCCT projection
  generation.
- Candidate counts, CPU submission, optional disjoint-safe GPU timings, frame
  cadence, and draw calls are observable.
- Screenshot cases cover outer silhouette, internal occlusion, depth bias,
  curved surfaces, mirrored instances, and coincident geometry.
- SVG produced by the C++/WASM fast vector path overlays the GPU result within
  the declared pixel/geometric tolerance at controlled views.

### Repository and release

- Register new C++, Python, TypeScript, and WASM tests in the appropriate Rack
  strata and record their runtime impact.
- Run Ruff, Pyright, uv lock checks, clang-format checks, CMake/CTest native
  validation, TypeScript freshness checks, browser packaging/static closure,
  Python package validation, and the L99 release gate.
- Update generated `dist/` artifacts only through their normal build paths.
- Obtain an independent clean-room and implementation review before closeout.

## Expected repository areas

- `src/cpp/lib/geometer/` for focused public/internal value declarations;
- new focused `src/cpp/lib/mesh_*` or `fast_hlr_*` preparation, adjacency,
  visibility, outline, and reconstruction modules;
- the existing projection orchestrator and JSON option parser for additive
  routing only;
- CLI, C ABI/WASM exports, and Python tests using the existing projection
  surface;
- `examples/wasm/fast_hlr.ts` and Illustration Lab integration for the GPU
  adapter;
- C++, TypeScript, Python, WASM/browser, fixture, and Rack metadata;
- `docs/design/step-geometry.md`, JSON/C ABI/WASM/CLI/Python docs, REQ-002,
  changelog, and release evidence after behavior settles.

## Explicit non-goals

- selecting one HLR backend as the permanent default;
- removing, replacing, or deprecating exact, poly, or current mesh-shadow;
- requiring a GPU for projection JSON, SVG, CLI, or Python use;
- reproducing proprietary HOOPS implementation details;
- source-exact analytic curve recovery from a tessellated mesh;
- application-specific board, Altium, visualizer, or documentation policy;
- ambient occlusion, object-occlusion styling, shadows, or a complete
  downstream illustration compositor;
- perspective-vector guarantees in the first delivery;
- process-global model caching or a stable persistent public session contract;
- publishing the demo externally.

## Completion evidence and closeout

Before closing the plan, record:

- the final algorithm description and clean-room provenance boundary;
- option/default/limit semantics and compatibility impact;
- small, medium, BGA90, and large-assembly timing and memory tables;
- raw-to-simplified segment counts and accepted/rejected arc-fit evidence;
- outline topology and geometric-error comparisons;
- native/WASM equivalence and repeated-run determinism;
- browser screenshots and GPU/CPU metrics;
- focused test runtimes, Rack placement, independent review findings, and full
  signoff commands.

After the alternate backend ships, move settled behavior into requirements,
ADRs where a durable decision was actually made, design/interface docs, tests,
and the changelog. Remove this completed temporary plan according to repository
policy.

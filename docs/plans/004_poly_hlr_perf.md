# Plan 004 — Poly HLR Perf Rework (branch 1.1)

Date: 2026-05-16
Branch: `1.1`

## Motivation

The current HLR projection path in `src/cpp/lib/hlr_projection.cpp` calls
`HLRBRep_Algo` (the exact, B-rep edge-based HLR algorithm). On real component
STEP geometry the WASM benchmark at `tests/wasm/hlr_benchmark.html` regularly
takes seconds per view.

Altium Draftsman, which feeds visually similar geometry through OCCT, uses the
`HLRBRep_PolyAlgo` family (tessellation-based HLR) and stays fast even on
boards with hundreds of components. The decompiled OccProxy entry point is
`ProjectOCCShape::ProjectOCCShapeOnSideByPoly`, mapping to OCCT's poly-algo.

This plan switches geometer's HLR path to poly-algo, exposes the meaningful
tunables (mesh deflection, HLR merge angle), and adds an opt-in fallback to
the exact path for callers that need analytic arcs.

## Goals

1. Drop typical per-view HLR cost on the embedded_models corpus by at least 5x
   versus the current exact path, measured in the same WASM harness.
2. Keep the `geometry.projection.a0` JSON schema unchanged (same fields, same
   shapes). Behavior of `arcs[]` in poly mode is documented (poly tessellates,
   so arcs are typically empty unless the underlying edge happens to land in
   the projection plane and is preserved).
3. Expose tunables that downstream clients actually need:
   - `projection_algorithm`: `poly` (default) | `exact`
   - `mesh_linear_deflection` (mm)
   - `mesh_angular_deflection` (rad)
   - `mesh_relative` (bool)
   - `hlr_angle_tolerance` (rad) — passed to `HLRBRep_PolyAlgo::Angle`
4. Surface every option in `hlr_benchmark.html` and the embedded model viewer
   so we can sweep them interactively.
5. Provide a before/after benchmark comparing the v0.1.0 baseline (`dist/baseline/*`)
   and the v1.1 build (`dist/*`) over the embedded_models fixtures.

## Output Contract

No schema change. Existing types in `src/cpp/lib/geometer/projection.h` are
preserved:

- `HlrProjectionResult { schema = "geometry.projection.a0", units, source_hash, views[] }`
- `ProjectedViewGeometry { view, simple, detail }`
- `ProjectedModeGeometry { segments[], arcs[] }`

Semantic notes added to `INTERFACES.md`:

- In `poly` mode, edges are returned as tessellated line segments. The `arcs[]`
  field will normally be empty; callers that previously relied on receiving
  `ProjectedArc` records for circular silhouettes should either run with
  `projection_algorithm: "exact"` or accept the polyline approximation.
- Default algorithm changes from implicit-exact to `poly`. This is a
  behavioral change for existing callers and triggers a `GEOMETER_ABI_VERSION`
  bump (5 → 6).

## New Options

Added to `HlrProjectionOptions` in `projection.h`:

```cpp
enum class ProjectionAlgorithm { Poly, Exact };

struct HlrProjectionOptions {
    // ... existing fields ...
    ProjectionAlgorithm projection_algorithm = ProjectionAlgorithm::Poly;
    double mesh_linear_deflection = 0.01;   // mm
    double mesh_angular_deflection = 0.5;   // rad (~28.6 deg)
    bool mesh_relative = false;
    double hlr_angle_tolerance = 0.0174533; // ~1 deg
};
```

JSON keys accepted by `parse_hlr_projection_options_json`:

- `projection_algorithm` or `projectionAlgorithm`: `"poly"` | `"exact"`
- `mesh_linear_deflection` or `meshLinearDeflection`
- `mesh_angular_deflection` or `meshAngularDeflection`
- `mesh_relative` or `meshRelative`
- `hlr_angle_tolerance` or `hlrAngleTolerance`

Defaults reflect Draftsman-style settings: poly algorithm, 0.01 mm linear
deflection (matches `wire.GetPolygon(0.01)` calls seen in the decompiled
`AsyncProjectionMaker`).

## Implementation

### `src/cpp/lib/hlr_projection.cpp`

Add a new `project_view_poly` next to the existing `project_view`:

1. `BRepMesh_IncrementalMesh mesher(shape, opts.mesh_linear_deflection, opts.mesh_relative, opts.mesh_angular_deflection, /*parallel=*/true);` — mesh in place once per shape, not per view. Hoist outside the view loop.
2. `Handle(HLRBRep_PolyAlgo) algo = new HLRBRep_PolyAlgo(); algo->Load(shape); algo->Projector(HLRAlgo_Projector(axes)); algo->Angle(opts.hlr_angle_tolerance); algo->Update();`
3. `HLRBRep_PolyHLRToShape poly_to_shape; poly_to_shape.Update(algo); TopoDS_Shape visible = poly_to_shape.VCompound(); TopoDS_Shape outline = poly_to_shape.OutLineVCompound();`
4. Feed `visible` / `outline` into the existing `add_edge_geometry` — it already handles `GeomAbs_Line` edges, which is what poly-algo emits.
5. The simple-mode pipeline downstream (`simple_geometry_from_segments` →
   `build_planar_contours`) is unchanged.

The exact path stays as `project_view_exact` and is selected when
`options.projection_algorithm == ProjectionAlgorithm::Exact`.

Mesh hoisting: a single STEP shape often gets projected from several views.
Mesh once before the view loop. If the algorithm is exact, skip meshing.

Edge type sanity: in poly mode, do not call `circle_arc_from_adaptor`. Add a
guard in `add_edge_geometry` that forces polyline treatment when the caller
opted into poly. This avoids spurious arcs from any leftover circular tessellation
remnants.

### CMake / ABI

`HLRBRep_PolyAlgo` and `HLRBRep_PolyHLRToShape` live in `TKHLR`, which is
already linked. `BRepMesh_IncrementalMesh` lives in `TKMesh`, which is already
linked.

Bump `GEOMETER_ABI_VERSION` from 5 to 6 in `src/cpp/lib/CMakeLists.txt`.

### Timings

Add optional `timings_ms` to `HlrProjectionResult` for the WASM caller:

```cpp
struct HlrProjectionTimings {
    double step_read_ms = 0.0;
    double mesh_ms = 0.0;
    double hlr_ms = 0.0;     // sum of per-view hlr ms
    double extract_ms = 0.0; // sum of per-view edge extraction + simple build
};
```

Mirrored into the JSON as a `timings` object. Worker reads it; benchmark UI
shows mesh vs HLR vs extract as separate columns.

## WASM Tests

### `tests/wasm/hlr_benchmark.html`

Add toolbar controls:

- Backend select: `dist/` (1.1) | `dist/baseline/` (v0.1.0)
- `projection_algorithm` select
- numeric inputs for `mesh_linear_deflection`, `mesh_angular_deflection`, `hlr_angle_tolerance`
- `mesh_relative` checkbox

Add columns: `Mesh`, `Extract`. Keep `HLR`, `JSON`, `Detail`, `Simple` and the
existing summary metrics. Update worker to forward options and parse extra
timings.

### `tests/wasm/embedded_model_viewer.html`

Same option controls in a collapsible "HLR options" panel. Recompute on
change. Show the same `Mesh`/`HLR`/`Extract` sub-timings.

### `tests/wasm/hlr_projection_worker.js`

Accept full `options` object from the page, forward to WASM. Surface
`timings` from the JSON response back to the page.

## Benchmark Methodology

1. Fixtures: `tests/fixtures/step/embedded_models/*` (36 STEP files, per
   manifest). All views default to `top` per existing harness behavior.
2. For each backend (`baseline` and `1.1`):
   - Cold cache (fresh worker per backend).
   - Run all fixtures sequentially.
   - Record per-file: STEP-to-bytes ms, module ms (cold), hlrMs total,
     mesh/extract sub-timings (1.1 only), detail count, simple count.
3. Save two CSVs into `tests/wasm/results/`:
   - `bench_baseline.csv`
   - `bench_poly.csv`
4. Compute per-fixture speedup and overall geometric mean.
5. Summarize in `docs/plans/004_poly_hlr_perf_results.md`.

## Acceptance Criteria

- All fixtures complete in both backends without errors.
- Total HLR time for `1.1` is at most 0.2x of baseline (5x speedup) on the
  embedded_models corpus.
- JSON schema validates: every fixture in both backends parses, `views[0].simple.segments` non-empty, view counts of segments per backend within an order of magnitude (poly produces more segments because each curve becomes polylines).
- Benchmark page and viewer load and respond to control changes without
  errors when pointed at either backend.
- Default options match Draftsman-style values; explicit `exact` still works
  end-to-end.

## Out of Scope

- Cross-call result caching (Tier-A persistence and Tier-C rotation keying
  that Draftsman uses). That belongs in a separate plan once the per-call
  cost is reduced.
- Native arc reconstruction from poly tessellation.
- Multi-threaded view projection at the C++ level (workers already give us
  per-call parallelism in the browser).

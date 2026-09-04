# HLR Projection A0

## Boundary

`geometry.hlr_projection.options.a0` and
`geometry.hlr_projection.result.a0` are the canonical additive contracts shared
by the model and indexed-mesh projection operations. The default projection
algorithm remains `poly` for model/STEP projection. Indexed-mesh projection
defaults to `fast`; explicit selectors remain available where the source
supports them.

The existing `geometry.projection.options.b0` and `geometry.projection.b0`
identities, focused C ABI functions, CLI commands, and Python helpers remain
compatibility surfaces. Their accepted aliases are mapped into the canonical
A0 options before execution. A0 emits `geometry.hlr_projection.result.a0`;
legacy entry points continue to emit `geometry.projection.b0`.

Fast vector HLR returns renderer-neutral segments in the same independent
`outline`, `detail`, and `bbox` layers as the older paths. It is not the WebGL
raster comparator. Mesh illustration is also independent: it consumes mesh and
linework geometry, applies presentation policy, and produces SVG or Canvas
output.

Generated field/type references are available for the
[options contract](../generated/contracts/contracts/geometry-hlr-projection-options-a0.html),
[result contract](../generated/contracts/contracts/geometry-hlr-projection-result-a0.html),
[STEP operation](../generated/contracts/operations/geometry-model-hlr-projection-a0.html),
and [indexed-mesh operation](../generated/contracts/operations/geometry-mesh-hlr-projection-a0.html).
Runtime consumers should also inspect the negotiated operation catalog instead
of assuming a particular executable or WASM build exposes an operation.

## Algorithm selection

| Selection | Geometry source | Detail behavior | Curve output | Performance posture |
|---|---|---|---|---|
| `poly` (model default) | STEP through OCCT | OCCT polygonal HLR | Segments | Existing compatibility path |
| `exact` | STEP through OCCT | OCCT exact HLR | Native circular arcs or sampled segments | Highest-fidelity, higher-cost path |
| `fast` (mesh default) | STEP tessellation or an indexed mesh | Triangle incidence plus spatial visibility classification | Segments | Low-latency one-shot; prepare-once API supports repeated views |

`outline_algorithm` is independent of `projection_algorithm`:

| Value | Effect |
|---|---|
| `hlr-close` (model default) | Forms outline polygons from HLR edges and closes small gaps. |
| `mesh-shadow` | Unions projected tessellated triangles through the established Clipper2 path. |
| `fast-mesh-shadow` (mesh default) | Reconstructs projected source-face loops, with bounded triangle-union fallbacks. |

## Direct C++ Fast API

`geometer/fast_hlr.h` is the stable semantic boundary. The simplest call uses
the indexed-mesh overload:

```cpp
FastHlrIndexedMesh mesh = make_mesh();
ProjectedModeGeometry visible;
Status status;
int code = project_fast_hlr_detail(
    mesh, {"top", {0, 0, 1}, {0, 1, 0}}, {}, &visible, nullptr, nullptr, &status);
```

For near-realtime view changes, prepare once and retain the value object:

```cpp
FastHlrPreparedMesh prepared;
prepare_fast_hlr_mesh(mesh, options, &prepared, &status);
project_fast_hlr_detail(prepared, top_view, options, &top_lines, nullptr, &top_stats, &status);
project_fast_hlr_detail(prepared, side_view, options, &side_lines, nullptr, &side_stats, &status);
```

The one-shot overload is exactly preparation followed by prepared projection.
`FastHlrPreparedMesh` is an in-process value, not a serialized or cross-process
contract. The operation transports use the indexed-mesh A0 packet for input
and prepare within the receiving process.

## Governed operation transports

Both HLR operations are advertised through the generic C ABI catalog and are
available in native IPC and browser/Node WASM. Generated TypeScript clients
provide `modelHlrProjection` and `meshHlrProjection`; Python provides
`GeometerClient.model_hlr_projection` and `mesh_hlr_projection`; Rust provides
the matching methods on `GeometerClient`. The mesh methods accept the governed
indexed-mesh packet, and the Python and TypeScript facades also accept their
language-native `IndexedTriangleMeshA0` value.

The STEP operation retains the common `poly` default. Because an indexed mesh
has no OCCT topology to run through the old backends, the mesh operation uses
Fast detail and Fast mesh-shadow outline when those selectors are absent. It
rejects explicit `poly`, `exact`, or `hlr-close` selections rather than silently
changing them. Typed IPC clients materialize the existing
`output_detail=true` default on the wire to distinguish HLR's presence-only
options from the older model-bounds options; this does not change projection
semantics.

### TypeScript/WASM

```ts
import { createGeometerWasmClient } from "@wavenumber/geometer/wasm";

const client = await createGeometerWasmClient(createGeometerModule);
const fromStep = await client.modelHlrProjection({
  model: stepBytes,
  options: {
    projection_algorithm: "fast",
    outline_algorithm: "fast-mesh-shadow",
    fast: { crease_angle_rad: (25 * Math.PI) / 180 },
  },
});
const fromMesh = await client.meshHlrProjection({
  mesh: {
    positions: [0, 0, 0, 10, 0, 0, 0, 10, 0],
    indices: [0, 1, 2],
    sourceFaces: [1],
  },
});
```

The dedicated Worker and persistent TypeScript IPC clients use the same
`modelHlrProjection` and `meshHlrProjection` method names.

### Python executable IPC

```python
from pathlib import Path

import geometer

options = geometer.HlrProjectionOptionsA0(
    projection_algorithm=geometer.HlrProjectionAlgorithm.FAST,
    outline_algorithm=geometer.HlrOutlineAlgorithm.FAST_MESH_SHADOW,
    fast=geometer.FastHlrOptionsA0(crease_angle_rad=0.4363323129985824),
)
mesh = geometer.IndexedTriangleMeshA0(
    positions=(0.0, 0.0, 0.0, 10.0, 0.0, 0.0, 10.0, 0.0),
    indices=(0, 1, 2),
    source_faces=(1,),
)
with geometer.GeometerClient() as client:
    step_result = client.model_hlr_projection(Path("part.step").read_bytes(), options)
    mesh_result = client.mesh_hlr_projection(mesh)
```

### Rust executable IPC

```rust
use geometer_client::{contracts, GeometerClient, ModelHlrProjectionRequest};

let options = contracts::decode_hlr_projection_options_a0_json(
    br#"{"projection_algorithm":"fast","outline_algorithm":"fast-mesh-shadow"}"#,
)?;
let result = client
    .model_hlr_projection(ModelHlrProjectionRequest {
        model: std::fs::read("part.step")?,
        media_type: "application/step".to_owned(),
        options,
    })
    .await?;
```

## Option applicability

All defaults below are focused C++ defaults. Canonical option DTOs preserve
absence, so applying a partial DTO over another configured option set does not
reset omitted fields. Geometric tolerances use millimeters for STEP operations
and source/model units for direct indexed-mesh preparation.

| Canonical option | Default | Units | Applicability and effect |
|---|---:|---|---|
| `views` | caller/default top view | model coordinates | Common. One or more orthographic direction/up pairs. |
| `output_outline` | `true` | — | Common. Enables the independently composable outline layer. |
| `output_detail` | `true` | — | Common. Enables the detail layer. It contains visible linework and, when requested, hidden Fast fragments. |
| `output_bbox` | `true` | — | Common. Enables projected source bounds. |
| `model_transform` | identity | model coordinates | Common source transform; row-major affine 4x4. |
| `strip_root_placement` | `false` | — | STEP only. Removes free-shape root placements while preserving child placements. |
| `curve_mode` | `native_arcs` | — | Exact/poly extraction. Fast emits segments and ignores native-arc selection. |
| `samples_per_curve` | `24` | samples | Exact/poly curve sampling. Fast detail does not sample curves. |
| `round_digits` | `3` | decimal digits | Common result quantization, accepted range 0–9. |
| `edge_v_sharp`, `edge_v_outline` | `true`, `true` | — | Exact and poly detail categories. Fast uses its nested candidate flags. |
| `edge_v_smooth`, `edge_v_sewn`, `edge_v_iso` | `false` | — | Exact only. Poly and Fast do not provide these OCCT categories. |
| `edge_h_sharp`, `edge_h_outline` | `false` | — | Exact and poly hidden categories. Fast uses `fast.include_hidden` with Fast candidate categories. |
| `edge_h_smooth`, `edge_h_sewn`, `edge_h_iso` | `false` | — | Exact only. Poly and Fast do not provide these OCCT categories. |
| `union_outline_polygons` | `true` | — | `hlr-close` outline only; ignored by mesh-shadow algorithms. |
| `projection_algorithm` | operation-specific | — | Common selector: `poly`, `exact`, or `fast`. Omission means `poly` for model HLR and `fast` for indexed-mesh HLR. |
| `mesh_linear_deflection` | `0.01` | mm/model units | STEP tessellation when absolute mode is active; affects poly, Fast, and mesh-shadow inputs. Not used by exact detail. |
| `mesh_angular_deflection` | `0.5` | radians | STEP tessellation for poly/Fast/mesh-shadow paths. |
| `mesh_relative` | `false` | — | Compatibility input to OCCT tessellation. Bbox-relative mode computes an absolute value first. |
| `mesh_deflection_mode` | `bbox-relative` | — | STEP tessellation: `absolute` or model-bounds-scaled. |
| `mesh_deflection_coefficient` | `0.004` | ratio | STEP tessellation in bbox-relative mode. |
| `outline_algorithm` | operation-specific | — | Common outline selector, independent of detail algorithm. Omission means `hlr-close` for model HLR and `fast-mesh-shadow` for indexed-mesh HLR. |
| `hlr_angle_tolerance` | `0.0174533` | radians | Exact HLR angular tolerance and delegated HLR outline behavior. |
| `fast` | focused Fast defaults | — | Fast detail and `fast-mesh-shadow` only. Never changes exact/poly OCCT edge semantics. |

### Fast controls

| `fast` member | Default | Units | Effect |
|---|---:|---|---|
| `include_boundaries` | `true` | — | Includes mesh boundary candidates. |
| `include_creases` | `true` | — | Includes adjacent-face crease candidates. |
| `include_silhouettes` | `true` | — | Includes view-dependent front/back silhouette candidates. |
| `include_hidden` | `false` | — | Direct C++ projection can return hidden fragments separately. The governed A0 model and mesh operations append requested hidden fragments to `modes.detail.segments` because A0 has no separate hidden layer. |
| `suppress_coplanar_seams` | `false` | — | Removes only intervals proven to continue across a separate coplanar source face. |
| `crease_angle_rad` | `0.5235987755982988` | radians (30°) | Minimum adjacent-face angle classified as a crease. UI degree controls convert at the boundary. |
| `weld_tolerance` | `1e-7` | model units | Radial vertex welding used while preparing incidence topology. Direct indexed meshes weld duplicate positions globally; OCCT-derived meshes weld only within connected topological components. |
| `projected_tolerance` | `1e-8` | model units | 2D geometric comparison tolerance. |
| `depth_tolerance` | `1e-7` | model units | Occlusion depth comparison tolerance. |
| `coplanar_seam_angle_rad` | `0.017453292519943295` | radians (1°) | Maximum normal-angle difference for seam continuation. |
| `coplanar_seam_depth_tolerance` | `1e-6` | model units | Maximum depth mismatch for seam continuation. |
| `coplanar_seam_lateral_tolerance` | `1e-6` | model units | Opposite-side lateral probe distance; must exceed projected tolerance. |

Fast resource defaults are `max_vertices=2,000,000`,
`max_triangles=2,000,000`, `max_edges=4,000,000`,
`max_grid_references=64,000,000`, `max_candidate_pairs=100,000,000`,
`max_fragments=8,000,000`, and `max_output_segments=4,000,000`.
Crossing a limit is a reported resource-limit failure, never silent truncation.
The candidate-pair ceiling also preflights the broad-phase segment-bound
overlaps before each Clipper outline union. Spatially disjoint segment pairs do
not consume this budget.

## Compatibility aliases

Strict A0 DTOs accept only the canonical spelling. Compatibility readers for
the existing JSON, CLI, Python, and C ABI lanes continue to accept:

- camelCase forms of all established top-level and nested Fast fields;
- `samples` for `samples_per_curve`;
- `native-arcs` for `curve_mode=native_arcs`;
- `bbox_relative` for `mesh_deflection_mode=bbox-relative`;
- `hlr_close` or `hlr` for `outline_algorithm=hlr-close`;
- `mesh_shadow` or `shadow` for `outline_algorithm=mesh-shadow`;
- `fast_mesh_shadow` for `outline_algorithm=fast-mesh-shadow`;
- `unionPolygons` for `union_outline_polygons`;
- `include_visible`/`includeVisible`, which sets visible sharp and outline
  categories before any explicit granular edge flags; and
- `include_outline`/`includeOutline`, which sets the visible outline category
  before any explicit `edge_v_outline` value.

The granular field wins when a historical include toggle and that field are
both present. Unknown fields are rejected by strict generated A0 codecs;
compatibility-reader behavior remains governed by its existing lane.

## Migration guide

No existing caller must migrate to keep its current behavior. The focused
`step-project-hlr`, `model-project-hlr`, Python `project_step_hlr`/
`project_model_hlr`, and focused C ABI entry points retain their defaults and
legacy result identity.

Use the governed A0 operations when a caller needs a cross-language typed
contract, indexed-mesh input, or canonical Fast options. Select Fast explicitly
for STEP with `projection_algorithm="fast"`; do not depend on it becoming a
default. For indexed mesh, omit the selectors to accept the mesh operation's
Fast-only defaults, or send `fast` plus explicit Fast selectors. Use the direct
C++ prepared-mesh API when several views share one mesh in a process. Use
`@wavenumber/geometer/illustrated-hlr` only when the consumer wants the
optional colorized SVG/Canvas composition as well as the independently
returned linework.

## Illustration identities

Production illustration serialization begins at A0:

- `geometry.mesh_illustration.input.a0` describes one-shot meshes, view,
  preparation options, style, and SVG options;
- `geometry.mesh_illustration.style.a0` is a presence-preserving style patch;
  and
- `geometry.mesh_illustration.result.a0` contains SVG, rendering statistics,
  and warnings.

The package additionally exposes opaque reusable prepared-scene and Canvas
APIs. Consumers therefore do not reimplement preparation, visibility ordering,
fusion, coplanar layering, shading, colorization, caching, or disposal. The
experimental `geometry.mesh_illustration.prototype.a0` scene is neither an
input nor a production predecessor.

Omitted A0 preparation controls materialize as `max_triangles=750,000` and
`weld_tolerance=1e-7`. Illustration results return at most 256 warnings; when
more diagnostics arise, the final entry reports how many were suppressed.

## Native and WASM performance evidence

The 2026-09-03 Windows x64 qualification used one warmup and three measured
runs through the native executable and the Node-hosted WASM CLI. Fast geometry
was byte-equivalent between runtimes on every fixture in the five-model package
corpus. Exact HLR differed on TSOT-23-5 by one detail primitive (1,655 native
versus 1,654 WASM); that difference is isolated to the existing OCCT exact
backend rather than Fast.

ABM8 top-view detail produced equivalent counts and geometry in both runtimes:

| Algorithm | Native one-shot p50 | WASM one-shot p50 | Native view-phase p50 | WASM view-phase p50 |
|---|---:|---:|---:|---:|
| `poly` | 157.58 ms | 915.04 ms | 3.13 ms | 11.13 ms |
| `exact` | 161.92 ms | 940.41 ms | 36.44 ms | 117.63 ms |
| `fast` | 157.20 ms | 945.08 ms | 2.92 ms | 6.31 ms |

On the 24,150-triangle BGA90 fixture, Fast detail-only internal HLR p50 was
59.63 ms native and 114.98 ms WASM. Combined Fast detail plus
`fast-mesh-shadow` view-phase p50 was 77.62 ms native and 121.92 ms WASM;
one-shot wall p50 was 370.13 ms and 1,306.43 ms. The native prepared-view goal
is met, while this single-threaded WASM build does not meet a sub-100 ms
combined-view target.

One-shot wall time includes process/module startup, STEP import, tessellation,
projection, extraction, and serialization. The reported view phase is
`hlr_ms + extract_ms` after STEP import and tessellation; the benchmark does not
invoke the in-process reusable `FastHlrPreparedMesh` object. Reproduce the ABM8
comparison with:

```powershell
uv run python scripts/benchmark_fast_hlr.py `
  --model ABM8-272-T3.STEP `
  --runtime native --runtime wasm `
  --algorithm poly --algorithm exact --algorithm fast `
  --outline hlr-close --layer detail `
  --warmup 1 --repeat 3 `
  --output .bench-tmp/fast-hlr-native-wasm-abm8-detail.json
```

The JSON report records native executable digests and, for WASM, separate
launcher, module, and Node-host digests, plus environment details, per-sample
phase timings, geometry digests, counts, percentile summaries, and runtime
ratios. Reports remain ignored local evidence because timings are machine- and
load-dependent; the harness and fixture manifest are the reproducible record.

## Known A0 limitations

Fast vector HLR is orthographic and segment-based. It performs only
visibility-safe exact-collinear joins; general curve fitting, perspective
guarantees, multithreaded WASM visibility, and a serialized reusable prepared
model are post-A0 work. The evaluated browser raster-HLR prototype is not part
of the production A0 package.

## TypeScript convenience composition

`@wavenumber/geometer/illustrated-hlr` composes the indexed-mesh operation with
the production illustration renderer. `illustrateMeshWithFastHlr` is the
one-shot SVG path; `createFastHlrIllustrator` returns both the governed HLR
result and a reusable, disposable illustrator. Input positions and transforms
are millimeters in this composition. The facade does not merge the contracts:
callers can still inspect or store HLR linework independently of colorized
output.

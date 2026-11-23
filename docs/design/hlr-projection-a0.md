# HLR Projection A0

## Boundary

`geometry.hlr_projection.options.a0` and
`geometry.hlr_projection.result.a0` are the canonical additive contracts shared
by the model and indexed-mesh projection operations. Both operations default to
Fast detail and Fast Mesh Shadow when their algorithm selectors are omitted.
The retired OCCT selectors `poly`, `exact`, and `hlr-close` are rejected.

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

Since 2026.9.6, the [native illustration implementation](mesh-illustration-native.md)
reuses the same generated A0 values through direct C++, executable IPC and typed
Rust/Python methods. Use matching client/executable catalogs; browser APIs remain
supported. Native HLR composition is separate from the pure illustration call.

Generated field/type references are available for the
[options contract](../generated/contracts/contracts/geometry-hlr-projection-options-a0.html),
[result contract](../generated/contracts/contracts/geometry-hlr-projection-result-a0.html),
[STEP operation](../generated/contracts/operations/geometry-model-hlr-projection-a0.html),
and [indexed-mesh operation](../generated/contracts/operations/geometry-mesh-hlr-projection-a0.html).
Runtime consumers should also inspect the negotiated operation catalog instead
of assuming a particular executable or WASM build exposes an operation.

## View-plane coordinate frame

Every projected XY value belongs to the requested view plane. It is not model
XY or board XY merely because a caller names the view `top` or `bottom`. This
applies uniformly to outline and detail segments, silhouette edges, native
arcs, layer bounds, and the `bbox` geometry.

For a view with `direction = D` and `up = U`, Geometer constructs an
orthonormal basis at the model-coordinate origin:

1. `Z = normalize(D)`;
2. `Y = normalize(U - Z * dot(U, Z))`; and
3. `X = Y cross Z`.

`D` therefore names the positive view-depth direction, from the model toward
the observer; it is not a camera ray pointing into the model. A point `P` has
view coordinates `(dot(P, X), dot(P, Y), dot(P, Z))`. Greater view Z is closer
to the observer and can occlude smaller view Z. Correspondingly, an outward
face normal with a positive dot product against `Z` is front-facing. A
silhouette separates incident faces on opposite sides of that facing test,
including its defined grazing tolerance.

The view origin is `(0, 0, 0)` in the coordinates presented to projection.
Geometer does not recenter a shape on its bounding box. `model_transform` is a
row-major affine 4x4 applied to source points before the view basis, so the
complete point mapping is:

```text
transformed_point = model_transform * model_point
view_point = [dot(transformed_point, X),
              dot(transformed_point, Y),
              dot(transformed_point, Z)]
```

Translations in `model_transform` are consequently retained. Translation
along X or Y moves the projected result; translation along Z changes depth.

With `up = [0, 1, 0]`, the two axial board-style views make the reflection
explicit:

| Direction | View basis `(X, Y, Z)` | Emitted XY |
|---|---|---|
| `[0, 0, 1]` | `([1,0,0], [0,1,0], [0,0,1])` | `(x_model, y_model)` |
| `[0, 0, -1]` | `([-1,0,0], [0,1,0], [0,0,-1])` | `(-x_model, y_model)` |

Here `x_model` and `y_model` mean coordinates after `model_transform`. The
negative-Z result is already expressed in a reflected view frame.

### Reconstructing model and board coordinates

A consumer that places footprint-local projection geometry on a board must
first convert view-plane coordinates back to model-local coordinates and only
then apply the occurrence placement:

```text
board_point = occurrence_transform * view_to_model * view_point
```

For the axial views above, the top 2D `view_to_model` is identity and the
bottom 2D `view_to_model` is `diag(-1, 1)`. The bottom conversion is not the
physical bottom-side occurrence transform. Omitting it leaves view XY mirrored;
applying it after occurrence placement or adding a renderer-specific
counter-mirror mixes distinct coordinate frames.

For example, consider the asymmetric model-local triangle
`A=(-2,0)`, `B=(1,0)`, `C=(1,1)`. The top view emits the same coordinates. The
bottom view emits `A=(2,0)`, `B=(-1,0)`, `C=(-1,1)`. Applying the bottom
`view_to_model = diag(-1,1)` reconstructs the original model-local triangle,
after which `occurrence_transform` gives its one correct board placement.

`ProjectedArc.start`, `end`, `center`, and `ccw` likewise describe the emitted
view plane. Carrying `view_to_model` and `occurrence_transform` as affine
transforms preserves the arc without rewriting it. A consumer that instead
bakes a reflection into arc coordinates must also reverse its clockwise/
counterclockwise interpretation when the applied 2D linear transform has a
negative determinant.

View selection and coordinate reconstruction are separate decisions. A caller
chooses `direction` according to the geometry frame it submits and the faces it
wants visible. A `top`, `bottom`, or layer label does not establish whether the
model is footprint-local or already board-placed, and Geometer does not infer
an occurrence transform from that label.

## Algorithm selection

| Selection | Geometry source | Detail behavior | Curve output | Performance posture |
|---|---|---|---|---|
| `fast` (default) | STEP tessellation or an indexed mesh | Triangle incidence plus spatial visibility classification | Segments | Low-latency one-shot; prepare-once API supports repeated views |

`outline_algorithm` is independent of `projection_algorithm`:

| Value | Effect |
|---|---|
| `mesh-shadow` | Unions projected tessellated triangles through the established Clipper2 path. |
| `fast-mesh-shadow` (default) | Reconstructs projected source-face loops, with bounded triangle-union fallbacks. |

## Direct C++ Fast API

Fast preparation uses a lookup-only weld table while retaining input order and
the minimum matching vertex ID. Per-view triangle search deduplicates grid
references before sorting candidate IDs, preserving visibility order and
candidate-pair limit accounting. Mesh-shadow sweep ranks are reused without
changing inclusive endpoint overlap. Prepared values remain validated on each
public projection call. These implementation optimizations do not change
projection tolerances or linework contracts.

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

Both operations default to Fast detail and Fast Mesh Shadow.
Typed IPC clients materialize the existing
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
const fromMesh = await client.meshHlrProjectionA0({
  mesh: {
    positions: [0, 0, 0, 10, 0, 0, 0, 10, 0],
    indices: [0, 1, 2],
    sourceFaces: [1],
  },
});
```

The dedicated Worker and persistent TypeScript IPC clients use the same
`modelHlrProjection` and explicit `meshHlrProjectionA0` compatibility method
names. Unqualified `meshHlrProjection` is the canonical B0 operation.

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
    mesh_result = client.mesh_hlr_projection_a0(mesh)
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
        media_type: "model/step".to_owned(),
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
| `round_digits` | `3` | decimal digits | Common result quantization, accepted range 0–9. |
| `projection_algorithm` | `fast` | — | Selects Fast detail for both HLR operations. |
| `mesh_linear_deflection` | `0.01` | mm/model units | STEP tessellation when absolute mode is active; affects Fast detail and mesh shadows. |
| `mesh_angular_deflection` | `0.5` | radians | STEP tessellation for poly/Fast/mesh-shadow paths. |
| `mesh_relative` | `false` | — | Compatibility input to OCCT tessellation. Bbox-relative mode computes an absolute value first. |
| `mesh_deflection_mode` | `bbox-relative` | — | STEP tessellation: `absolute` or model-bounds-scaled. |
| `mesh_deflection_coefficient` | `0.004` | ratio | STEP tessellation in bbox-relative mode. |
| `outline_algorithm` | `fast-mesh-shadow` | — | Common outline selector, independent of detail algorithm. Omission selects Fast Mesh Shadow for both HLR operations. |
| `fast` | focused Fast defaults | — | Fast detail and `fast-mesh-shadow` controls. |

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
- `bbox_relative` for `mesh_deflection_mode=bbox-relative`;
- `mesh_shadow` or `shadow` for `outline_algorithm=mesh-shadow`;
- `fast_mesh_shadow` for `outline_algorithm=fast-mesh-shadow`.

Unknown fields are rejected by strict generated A0 codecs;
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
corpus.

ABM8 top-view detail produced equivalent counts and geometry in both runtimes:

| Algorithm | Native one-shot p50 | WASM one-shot p50 | Native view-phase p50 | WASM view-phase p50 |
|---|---:|---:|---:|---:|
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
  --layer detail `
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
the production A0 illustration renderer. `illustrateMeshWithFastHlrA0` is the
one-shot SVG path; `createFastHlrIllustratorA0` returns both the governed HLR
result and a reusable, disposable illustrator. Its explicit A0 suffix prevents
new callers from confusing this compatibility composition with correlated B0
mesh HLR and clipping. Input positions and transforms are millimeters in this
composition. The facade does not merge the contracts: callers can still inspect
or store HLR linework independently of colorized output.
## STEP source media semantics

STEP projection inputs follow the [STEP media policy](../geometer/adr/geometer-adr-020-standard-step-media-types.md). New requests use `model/step`; valid auxiliary parameters and `application/step` remain accepted. MIME normalization does not change model bytes, transforms, projection options or geometry.

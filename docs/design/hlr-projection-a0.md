# HLR Projection A0

## Boundary

`geometry.hlr_projection.options.a0` and
`geometry.hlr_projection.result.a0` are the canonical additive contracts shared
by the model and indexed-mesh projection operations. The default projection
algorithm remains `poly`. `exact` and `fast` are explicit selections.

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

## Algorithm selection

| Selection | Geometry source | Detail behavior | Curve output | Performance posture |
|---|---|---|---|---|
| `poly` (default) | STEP through OCCT | OCCT polygonal HLR | Segments | Existing compatibility path |
| `exact` | STEP through OCCT | OCCT exact HLR | Native circular arcs or sampled segments | Highest-fidelity, higher-cost path |
| `fast` | STEP tessellation or an indexed mesh | Triangle incidence plus spatial visibility classification | Segments | Low-latency one-shot; prepare-once API supports repeated views |

`outline_algorithm` is independent of `projection_algorithm`:

| Value | Effect |
|---|---|
| `hlr-close` (default) | Forms outline polygons from HLR edges and closes small gaps. |
| `mesh-shadow` | Unions projected tessellated triangles through the established Clipper2 path. |
| `fast-mesh-shadow` | Reconstructs projected source-face loops, with bounded triangle-union fallbacks. |

## Option applicability

All defaults below are focused C++ defaults. Canonical option DTOs preserve
absence, so applying a partial DTO over another configured option set does not
reset omitted fields. Geometric tolerances use millimeters for STEP operations
and source/model units for direct indexed-mesh preparation.

| Canonical option | Default | Units | Applicability and effect |
|---|---:|---|---|
| `views` | caller/default top view | model coordinates | Common. One or more orthographic direction/up pairs. |
| `output_outline` | `true` | — | Common. Enables the independently composable outline layer. |
| `output_detail` | `true` | — | Common. Enables visible detail linework. |
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
| `projection_algorithm` | `poly` | — | Common selector: `poly`, `exact`, or `fast`. |
| `mesh_linear_deflection` | `0.01` | mm/model units | STEP tessellation when absolute mode is active; affects poly, Fast, and mesh-shadow inputs. Not used by exact detail. |
| `mesh_angular_deflection` | `0.5` | radians | STEP tessellation for poly/Fast/mesh-shadow paths. |
| `mesh_relative` | `false` | — | Compatibility input to OCCT tessellation. Bbox-relative mode computes an absolute value first. |
| `mesh_deflection_mode` | `bbox-relative` | — | STEP tessellation: `absolute` or model-bounds-scaled. |
| `mesh_deflection_coefficient` | `0.004` | ratio | STEP tessellation in bbox-relative mode. |
| `outline_algorithm` | `hlr-close` | — | Common outline selector, independent of detail algorithm. |
| `hlr_angle_tolerance` | `0.0174533` | radians | Exact HLR angular tolerance and delegated HLR outline behavior. |
| `fast` | focused Fast defaults | — | Fast detail and `fast-mesh-shadow` only. Never changes exact/poly OCCT edge semantics. |

### Fast controls

| `fast` member | Default | Units | Effect |
|---|---:|---|---|
| `include_boundaries` | `true` | — | Includes mesh boundary candidates. |
| `include_creases` | `true` | — | Includes adjacent-face crease candidates. |
| `include_silhouettes` | `true` | — | Includes view-dependent front/back silhouette candidates. |
| `include_hidden` | `false` | — | Returns hidden classified fragments in the hidden result channel where exposed by the semantic API. |
| `suppress_coplanar_seams` | `false` | — | Removes only intervals proven to continue across a separate coplanar source face. |
| `crease_angle_rad` | `0.5235987755982988` | radians (30°) | Minimum adjacent-face angle classified as a crease. UI degree controls convert at the boundary. |
| `weld_tolerance` | `1e-7` | model units | Vertex welding used while preparing incidence topology. |
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

## Known A0 limitations

Fast vector HLR is orthographic and segment-based. It performs only
visibility-safe exact-collinear joins; general curve fitting, perspective
guarantees, multithreaded WASM visibility, and a serialized reusable prepared
model are post-A0 work. Browser raster HLR remains a separate pixel renderer.

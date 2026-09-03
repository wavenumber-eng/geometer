# JSON Formats

## Model Bounds Options JSON

Defined in `src/cpp/lib/geometer/model_bounds_options_json.h`.

Accepted option keys:

- `format` or `model_format`: currently only `step`.
- `model_transform` or `modelTransform`: row-major 4x4 number matrix. A flat
  array of 16 numbers is also accepted.

Empty or `null` JSON keeps the default options.

## Model Bounds Result JSON

Model bounds JSON uses schema `geometry.model_bounds.a0`.

Required top-level fields:

- `schema`: currently `geometry.model_bounds.a0`.
- `units`: currently `mm`.
- `source`: object containing `format` and `hash`.
- `bounds`: object containing `min`, `max`, `size`, and `center` XYZ arrays.

The current model-bounds writer always emits these nondeterministic timing
fields; semantic conformance excludes them only through an explicit vector
projection:

- `model_read_ms`
- `bounds_ms`

## STEP To GLB Options JSON

Defined in `src/cpp/lib/geometer/step_to_glb_options_json.h`.

```cpp
int parse_step_to_glb_options_json(
    const char* json,
    StepToGlbOptions* options,
    Status* status = nullptr
);
```

Accepted option keys:

- `linear_deflection`, `linearDeflection`, or `deflection`.
- `angular_deflection`, `angularDeflection`, or `angular`.

Both values must be positive finite numbers. Empty or `null` JSON keeps the
default options.

## HLR Options JSON

Defined in `src/cpp/lib/geometer/projection_options_json.h`.

```cpp
int parse_hlr_projection_options_json(
    const char* json,
    HlrProjectionOptions* options,
    Status* status = nullptr
);
```

Accepted option keys:

- `views`: array of `{ id, direction, up }`.
- `model_transform` or `modelTransform`: row-major 4x4 number matrix. A flat
  array of 16 numbers is also accepted.
- `strip_root_placement` or `stripRootPlacement`: remove only free-shape root
  placements before projection, matching STEP-to-GLB's definition-local frame;
  assembly-child placements remain intact. Defaults to `false` for backward
  compatibility.
- `curve_mode` or `curveMode`: `native_arcs`, `native-arcs`, or `polyline`.
- `samples_per_curve` or `samples`.
- `round_digits` or `roundDigits`.
- `output_outline` or `outputOutline`: emit the independently composable
  outline layer; defaults to `true`.
- `output_detail` or `outputDetail`: emit the independently composable detail
  layer; defaults to `true`.
- `output_bbox` or `outputBbox`: emit the projected bounding-box layer;
  defaults to `true`.
- `projection_algorithm` or `projectionAlgorithm`: `poly`, `exact`, or the
  additive evaluation backend `fast`.
- `mesh_linear_deflection` or `meshLinearDeflection`.
- `mesh_angular_deflection` or `meshAngularDeflection`.
- `mesh_relative` or `meshRelative`.
- `mesh_deflection_mode` or `meshDeflectionMode`: `absolute` or
  `bbox-relative`.
- `mesh_deflection_coefficient` or `meshDeflectionCoefficient`.
- `outline_algorithm` or `outlineAlgorithm`: `hlr-close`, `mesh-shadow`, or
  the additive evaluation backend `fast-mesh-shadow`.
- `hlr_angle_tolerance` or `hlrAngleTolerance`.
- `edge_v_sharp`, `edge_v_outline`, `edge_v_smooth`, `edge_v_sewn`,
  `edge_v_iso`, `edge_h_sharp`, `edge_h_outline`, `edge_h_smooth`,
  `edge_h_sewn`, and `edge_h_iso` plus camelCase aliases.
- legacy `include_visible` or `includeVisible`, which toggles visible sharp
  and visible outline edges together.
- legacy `include_outline` or `includeOutline`, which toggles visible outline
  edges.
- `union_outline_polygons`, `unionOutlinePolygons`, or `unionPolygons`.
- `fast`: provisional options used by `projection_algorithm=fast` and
  `outline_algorithm=fast-mesh-shadow`:
  `include_boundaries`, `include_creases`, `include_silhouettes`,
  `include_hidden`, `suppress_coplanar_seams`, `crease_angle_rad`,
  `weld_tolerance`, `projected_tolerance`, `depth_tolerance`,
  `coplanar_seam_angle_rad`, `coplanar_seam_depth_tolerance`, and
  `coplanar_seam_lateral_tolerance`, with camelCase aliases. Its
  nested `limits` object accepts `max_vertices`, `max_triangles`, `max_edges`,
  `max_grid_references`, `max_candidate_pairs`, `max_fragments`, and
  `max_output_segments`, also with camelCase aliases.

The `fast` option block is an evaluation contract. Its controls may be refined
before the backend is promoted, but they cannot change the meaning of the
OCCT-specific exact/poly flags.

The browser test pages currently use the viz-compatible setting set:

```json
{
  "curve_mode": "polyline",
  "samples_per_curve": 24,
  "round_digits": 3,
  "projection_algorithm": "poly",
  "outline_algorithm": "mesh-shadow",
  "mesh_deflection_mode": "bbox-relative",
  "mesh_deflection_coefficient": 0.004,
  "edge_v_sharp": true,
  "edge_v_outline": true,
  "union_outline_polygons": true
}
```

## HLR Projection Result JSON

Projection JSON uses schema `geometry.projection.b0`.

Required top-level fields:

- `schema`: currently `geometry.projection.b0`.
- `units`: currently `mm`.
- `source`: source metadata, including `kind` and `hash`.
- `views`: array of projected view payloads.

Each projected view contains:

- `id`, `direction`, and `up`.
- `modes.outline`: assembly projection outline geometry.
- `modes.detail`: HLR detail geometry.
- `modes.bbox`: projected 3D shape bounding box geometry.

All mode members remain present for schema compatibility. A layer disabled by
its `output_*` option contains empty segment and arc arrays. Mesh-shadow
outline-only requests bypass detail HLR; detail-only requests do not construct
either outline algorithm. Combined requests keep outline and detail separate so
callers can style and composite them independently.

Segment objects contain `x1`, `y1`, `x2`, and `y2`. Arc objects contain
`start`, `end`, `center`, `radius`, `extent_rad`, `ccw`, and `full_circle`.

Optional timing fields are emitted when available:

- `step_read_ms`
- `mesh_ms`
- `hlr_ms`
- `extract_ms`

## Planar STEP Request JSON

Planar STEP uses schema `geometry.planar_step.request.a0`. It converts closed
2D topology into exact OCCT wires, faces, and extruded STEP solids. The schema is
aligned with the shared `Geom*` direction: a planar region has an `outer` ring
and optional `holes`; rings use `points[]` with one segment per point. Segment
endpoints are implied by adjacent points, and the final segment wraps to point
zero.

```json
{
  "schema": "geometry.planar_step.request.a0",
  "units": "mm",
  "name": "fixture_alignment",
  "bodies": [
    {
      "id": "copper",
      "name": "copper",
      "color": "#B87333",
      "z_mm": 0,
      "thickness_mm": 0.035,
      "fuse_regions": true,
      "regions": [
        {
          "outer": {
            "points": [[0, 0], [10, 0], [10, 5], [0, 5]],
            "segments": [
              { "kind": "line" },
              { "kind": "line" },
              { "kind": "line" },
              { "kind": "line" }
            ]
          }
        }
      ],
      "cutouts": []
    }
  ]
}
```

Accepted root `units` are `mm`, `nm`, `mils`, and `in`. Length fields also
accept suffixes such as `thickness_mm`, `thickness_nm`, `radius_mm`, and
`radius_nm`.

Ring segments:

- `{"kind": "line"}` for straight edges.
- `{"kind": "arc", "radius_nm": 1000000, "sweep": "ccw"}` for the target
  topology-first arc form.
- `{"kind": "arc", "center": [5, 2.5], "sweep": "cw"}` for transitional
  center-based `GeomContour`/Altium adapters.

The parser also accepts transitional `GeomContour` JSON with `start`,
`segments[].end`, optional `segments[].center`, and optional
`segments[].clockwise`. New producers should prefer the `points[]` topology
form.

Body-level `fuse_regions` can be set to `true` to union that body's closed
regions with Geometer's Clipper2-backed planar solver before extrusion. This
removes internal edges where same-body regions overlap, but fused output uses
line-segment topology rather than preserving source arc segments exactly.
`fuseRegions` and `fuse` are accepted compatibility aliases; new producers
should write `fuse_regions`.

## Batch Request JSON

The native CLI batch command accepts `geometer.batch.request.a0`:

```json
{
  "schema": "geometer.batch.request.a0",
  "version": "2026.8.21",
  "abi": 20260821,
  "options": {
    "curve_mode": "polyline"
  },
  "jobs": [
    {
      "id": "part-top",
      "operation": "step_hlr_projection_json",
      "step_path": "part.step",
      "output_path": "part.top.projection.json",
      "options": {
        "views": [{ "id": "top", "direction": [0, 0, 1], "up": [0, 1, 0] }]
      }
    }
  ]
}
```

`run` only requires the `jobs` array. `version` and `abi` are metadata written
by `init-request`. Top-level `options` are parsed first; job-level `options`
override them.

Supported operations:

- `model_bounds_json`
- `model_hlr_projection_json`
- `model_hlr_projection_svg`
- `model_to_glb`
- `step_hlr_projection_json`
- `step_hlr_projection_svg`
- `step_to_glb`
- `planar_step`

The `model_*` operations are preferred for source-model work. They currently
accept only `format: "step"` and accept either `model_path` or the compatibility
`step_path`. The `step_*` operations remain compatibility aliases.

`planar_step` jobs accept either:

- `request_path`: path to a `geometry.planar_step.request.a0` file.
- `planar_step_request`: inline request object.

## Batch Response JSON

The native CLI writes `geometer.batch.response.a0`:

```json
{
  "schema": "geometer.batch.response.a0",
  "version": "2026.8.21",
  "abi": 20260821,
  "ok": true,
  "jobs": [
    {
      "id": "part-top",
      "operation": "step_hlr_projection_json",
      "ok": true,
      "code": 0,
      "elapsed_ms": 12.34,
      "output_path": "part.top.projection.json"
    }
  ]
}
```

Failed jobs set `ok` to `false`, return a nonzero `code`, and include
`message`.

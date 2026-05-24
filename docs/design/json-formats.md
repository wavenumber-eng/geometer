# JSON Formats

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
- `curve_mode` or `curveMode`: `native_arcs`, `native-arcs`, or `polyline`.
- `samples_per_curve` or `samples`.
- `round_digits` or `roundDigits`.
- `projection_algorithm` or `projectionAlgorithm`: `poly` or `exact`.
- `mesh_linear_deflection` or `meshLinearDeflection`.
- `mesh_angular_deflection` or `meshAngularDeflection`.
- `mesh_relative` or `meshRelative`.
- `hlr_angle_tolerance` or `hlrAngleTolerance`.
- `edge_v_sharp`, `edge_v_outline`, `edge_v_smooth`, `edge_v_sewn`,
  `edge_v_iso`, `edge_h_sharp`, `edge_h_outline`, `edge_h_smooth`,
  `edge_h_sewn`, and `edge_h_iso` plus camelCase aliases.
- legacy `include_visible` or `includeVisible`, which toggles visible sharp
  and visible outline edges together.
- legacy `include_outline` or `includeOutline`, which toggles visible outline
  edges.
- `union_simple_polygons` or `unionPolygons`.

The browser test pages currently use the viz-compatible setting set:

```json
{
  "curve_mode": "polyline",
  "samples_per_curve": 24,
  "round_digits": 3,
  "projection_algorithm": "poly",
  "edge_v_sharp": true,
  "edge_v_outline": true,
  "union_simple_polygons": true
}
```

## HLR Projection Result JSON

Projection JSON uses schema `geometry.projection.a0`.

Required top-level fields:

- `schema`: currently `geometry.projection.a0`.
- `units`: currently `mm`.
- `source_hash`: hash of the STEP source bytes.
- `views`: array of projected view payloads.

Each projected view contains:

- `view`: `{ id, direction, up }`.
- `simple`: simplified geometry with `segments` and `arcs`.
- `detail`: HLR detail geometry with `segments` and `arcs`.

Segment objects contain `x1`, `y1`, `x2`, and `y2`. Arc objects contain
`start`, `end`, `center`, `radius`, `extent_rad`, `ccw`, and `full_circle`.

Optional timing fields are emitted when available:

- `step_read_ms`
- `mesh_ms`
- `hlr_ms`
- `extract_ms`

## Batch Request JSON

The native CLI batch command accepts `geometer.batch.request.a0`:

```json
{
  "schema": "geometer.batch.request.a0",
  "version": "2026.5.24",
  "abi": 20260524,
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

- `step_hlr_projection_json`
- `step_hlr_projection_svg`
- `step_to_glb`

## Batch Response JSON

The native CLI writes `geometer.batch.response.a0`:

```json
{
  "schema": "geometer.batch.response.a0",
  "version": "2026.5.24",
  "abi": 20260524,
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

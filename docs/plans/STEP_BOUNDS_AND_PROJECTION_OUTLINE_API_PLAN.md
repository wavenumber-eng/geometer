# STEP Bounds And Projection Outline API Plan

Status: bounds slice implemented; projection outline not started
Last updated: 2026-05-25

This is a focused working plan for adding generic STEP bounds and optional
projection-outline APIs to Geometer. Per the repository policy, durable results
from this work should move into `docs/design/`, `docs/requirements/`, or an ADR
when the implementation is complete.

## Motivation

Downstream tools need a small, reliable geometry dependency for STEP model
metadata. The immediate need is transformed 3D bounds for embedded model
placement. A later need is a simple closed 2D projected outline that can be used
where a rectangle is too coarse.

The bounds API must not depend on HLR. HLR can fail to produce closed planar
regions on some models, while bounding boxes are still required as the stable
fallback.

This is also the right time to normalize the public model-input API names.
Current Geometer model-input APIs are STEP-specific because STEP is the only
supported source model format today. New API names should use the generic
`model_*` convention while documenting that the only accepted `format` value is
currently `"step"`. Existing STEP-specific API names remain compatibility
wrappers and are deprecated only after one additional release cycle.

## Design Principles

- Keep the API generic; no board, footprint, layer, or Altium-specific policy.
- Use millimeters for all geometer outputs.
- Reuse the existing executable-backed Python package model.
- Accept both STEP bytes and STEP paths in Python.
- Keep `model_transform` as the generic row-major 4x4 transform used by current
  HLR APIs.
- Make bounds the first stable surface; keep projection outlines explicitly
  experimental until ring closure quality is proven.
- Prefer generic public names for model-input operations, with explicit
  `format="step"` validation until more source formats are supported.
- Preserve old STEP-specific names as wrappers with identical behavior.
- Do not rename `planar_step` as part of this cleanup. It creates STEP from
  planar regions; it is not a source-model import operation.

## API Rename Direction

Preferred names:

| Current name | Preferred name | Notes |
| --- | --- | --- |
| `step_to_glb(...)` | `model_to_glb(..., format="step")` | Converts a source model to GLB bytes. |
| `project_step_hlr(...)` | `project_model_hlr(..., format="step")` | Returns the typed HLR projection wrapper. |
| `hlr_projection_json(...)` | `model_hlr_projection_json(..., format="step")` | Returns projection JSON text. |
| new bounds API | `model_bounds(..., format="step")` | First stable bounds surface. |
| new outline API | `model_projection_outline(..., format="step")` | Experimental until ring closure is validated. |

Compatibility wrappers:

- `step_to_glb(...)` calls `model_to_glb(..., format="step")`.
- `project_step_hlr(...)` calls `project_model_hlr(..., format="step")`.
- `hlr_projection_json(...)` calls
  `model_hlr_projection_json(..., format="step")`.
- If a STEP-specific bounds or outline helper is ever added during migration,
  it should also be only a wrapper around the generic form.

Deprecation policy:

- Add generic names first, then keep compatibility wrappers supported for one
  full release cycle before marking them deprecated.
- Mark compatibility wrappers as deprecated in docs and release notes in the
  release after the generic names ship.
- Python wrappers may emit `DeprecationWarning` with `stacklevel=2`; this is
  normally hidden by default but visible to users who enable warnings.
- C++ headers should start with deprecation comments and release-note warnings.
  Add `[[deprecated]]` attributes only after downstream warning-as-error impact
  is reviewed.
- CLI aliases should stay quiet by default so scripts that parse stderr do not
  break. Mark them as deprecated in `--help` and docs.
- Batch operation aliases should remain accepted and should produce the same
  response shape as the preferred operation names.
- No removal date is set in this plan. Removal needs a separate compatibility
  decision.
- ADR 007 records this naming and deprecation policy as the durable decision.

## Impact On Planar STEP

The recent planar STEP API is not part of the model-input rename. It takes
incoming 2D region topology and creates a STEP model; it does not import or
project a source model file.

Keep these names stable:

- Python: `planar_step(...)` and `write_planar_step(...)`.
- CLI: `planar-step <request.json> <output.step>`.
- Batch operation: `planar_step`.
- C++: `planar_step_from_json(...)` and
  `planar_step_from_json_bytes(...)`.
- JSON schema: `geometry.planar_step.request.a0`.

Renaming this API to `model_*` would be misleading because the input is not a
model. It would also create unnecessary churn for the current planar batch and
2D-to-STEP tests. If this surface needs a more generic name later, prefer a
separate additive alias such as `planar_regions_to_step(...)`; do not deprecate
the current names as part of the STEP source-model cleanup.

## Proposed Python Surface

```python
import geometer

bounds = geometer.model_bounds(
    model_bytes_or_path,
    format="step",
    model_transform=transform,
)

outline = geometer.model_projection_outline(
    model_bytes_or_path,
    format="step",
    view=geometer.ProjectionView.top(),
    model_transform=transform,
    options=geometer.ProjectionOutlineOptions.simple(),
)
```

Candidate dataclasses:

```python
@dataclass(frozen=True)
class ModelBoundsResult:
    data: dict[str, Any]

    @property
    def bounds(self) -> Mapping[str, Any]: ...

    @property
    def units(self) -> str | None: ...

@dataclass(frozen=True)
class ProjectionOutlineOptions:
    round_digits: int = 3
    join_tolerance_mm: float = 0.001
    prefer_native_arcs: bool = True
    fallback_to_bounds: bool = False
```

The Python wrapper should remain a thin result wrapper over JSON so schema
changes stay visible and do not require immediate rich object modeling for
every field.

`format` validation should be deliberately strict in the first implementation:
accept `"step"` and reject every other value with a clear error. Automatic file
extension detection can be added later, but first-slice behavior should be
predictable.

## Proposed JSON Schemas

Bounds result schema:

```json
{
  "schema": "geometry.model_bounds.a0",
  "units": "mm",
  "source": { "format": "step", "hash": "..." },
  "bounds": {
    "min": [0.0, 0.0, 0.0],
    "max": [1.0, 2.0, 3.0],
    "size": [1.0, 2.0, 3.0],
    "center": [0.5, 1.0, 1.5]
  },
  "timings": {
    "step_read_ms": 0.0,
    "bounds_ms": 0.0
  }
}
```

Projection outline result schema:

```json
{
  "schema": "geometry.model_projection_outline.a0",
  "units": "mm",
  "source": { "format": "step", "hash": "..." },
  "view": {
    "id": "top",
    "direction": [0.0, 0.0, 1.0],
    "up": [0.0, 1.0, 0.0]
  },
  "mode": "simple",
  "quality": {
    "closed": true,
    "fallback_used": false,
    "message": ""
  },
  "rings": [
    {
      "role": "outer",
      "points": [[0.0, 0.0], [1.0, 0.0], [1.0, 1.0], [0.0, 1.0]],
      "segments": [
        { "kind": "line" },
        { "kind": "line" },
        { "kind": "line" },
        { "kind": "line" }
      ]
    }
  ],
  "fallback_bounds": {
    "min": [0.0, 0.0, 0.0],
    "max": [1.0, 1.0, 0.5]
  }
}
```

Questions to settle before implementation:

- Should projection outline support circular arc segments in the first schema,
  or should the first version emit polyline rings only?
- Should `fallback_bounds` always be present, or only when ring closure fails?
- Should multiple disjoint outer rings be accepted as a normal result, or
  marked as degraded quality for first consumers?

## Proposed CLI And Batch Surface

Direct CLI commands:

```powershell
geometer model-bounds input.step output.json --format step
geometer model-to-glb input.step output.glb --format step
geometer model-project-hlr input.step output.json --format step
geometer model-project-svg input.step output.svg --format step --view top --mode simple
geometer model-project-outline input.step output.json --format step --view top --mode simple
```

Compatibility CLI aliases:

- `step-to-glb`
- `step-project-hlr`
- `step-project-svg`

Batch operations:

- `model_bounds_json`
- `model_to_glb`
- `model_hlr_projection_json`
- `model_hlr_projection_svg`
- `model_projection_outline_json`

Batch compatibility aliases:

- `step_to_glb`
- `step_hlr_projection_json`
- `step_hlr_projection_svg`

Model-input operations should accept `format` and `model_transform` in job or
top-level options. First-slice `format` support is `"step"` only.
`model_projection_outline_json` can reuse the current HLR options where useful,
but should keep ring-joining options separate from raw HLR projection options.

## Proposed C++ Surface

Headers should mirror the current STEP/HLR style:

```cpp
enum class ModelFormat {
    Step
};

struct ModelBoundsOptions {
    ModelFormat format = ModelFormat::Step;
    std::array<double, 16> model_transform = identity;
};

struct ModelBoundsResult {
    std::string schema = "geometry.model_bounds.a0";
    std::string units = "mm";
    std::string source_format = "step";
    std::string source_hash;
    std::array<double, 3> min;
    std::array<double, 3> max;
    std::array<double, 3> size;
    std::array<double, 3> center;
};

int model_bounds_from_bytes(
    const unsigned char* model_data,
    std::size_t model_size,
    const ModelBoundsOptions& options,
    ModelBoundsResult* result,
    Status* status = nullptr
);
```

Projection outline can build on current HLR extraction, but should be separated
from raw `HlrProjectionResult` so consumers do not need to join unordered
segments themselves.

Current C++ model-input functions should remain available as wrappers:

- `step_to_glb(...)`
- `step_to_glb_from_bytes(...)`
- `step_hlr_projection_from_bytes(...)`

New generic counterparts should be added beside them. The old functions should
not be removed during this plan.

## Proposed C ABI And WASM Surface

The C ABI and WASM exports currently expose STEP-specific names. Add generic
entry points beside them and keep the old exports:

- `geometer_model_to_glb_bytes(...)`
- `geometer_model_hlr_projection_json_bytes(...)`
- `geometer_model_bounds_json_bytes(...)`

Optional convenience result-returning C ABI wrappers can mirror the existing
string/byte result helpers. The legacy exports remain:

- `geometer_step_to_glb(...)`
- `geometer_step_to_glb_bytes(...)`
- `geometer_step_hlr_projection_json(...)`
- `geometer_step_hlr_projection_json_bytes(...)`

Adding exports changes the ABI generation metadata for a release, but it should
not remove or alter existing symbols.

## Implementation Sequence

1. Done: finalize the generic model-input naming, bounds JSON schema, and
   Python dataclass shape.
2. Done: add C++ bounds value types and JSON writer/parser coverage.
3. Done: add generic direct CLI and batch operations for bounds.
4. Done: add Python `model_bounds(...)` and `ModelBoundsResult`.
5. Done for Python/CLI/batch: add generic names for existing GLB and HLR
   operations while keeping STEP-specific compatibility wrappers.
6. In progress: compare bounds against downstream CadQuery oracles across more
   STEP fixtures. Altium Monkey PcbLib embedded-model authoring tests pass
   against the local `model_bounds` implementation, including rotated real-world
   model bodies.
7. Next: decide whether C++ and C ABI generic wrappers for existing GLB/HLR
   operations should land before the next package release or with a later ABI
   generation.
8. Next: add Altium-extracted embedded STEP fixtures from real projects,
   including Hydroscope models generated through `altium_cruncher megamaid`.
9. Future: design the outline ring schema after reviewing more failure cases.
10. Future: implement projection outline as an experimental API only after ring
   joining tests are in place.

## Test Plan

- Unit tests for transform parsing and bounds JSON serialization.
- Native C++ tests for known STEP fixtures.
- CLI tests for path input.
- Python tests for path and byte input.
- Batch-runner tests for `model_bounds_json`.
- Compatibility tests proving old Python, CLI, C++, C ABI, and batch names still
  call the same implementation and return the same results.
- Downstream comparison cases against existing CadQuery-derived bounds.
- Additional user-provided STEP fixtures before outline API promotion.

## Breaking-Change Assessment

The intended implementation is additive and should not break existing behavior
when compatibility wrappers are used correctly.

Changes that would be breaking and should be avoided:

- changing the existing `geometry.projection.a0` JSON shape;
- changing HLR default views, edge options, transform semantics, or arc/segment
  output;
- removing or changing positional arguments for existing CLI commands;
- removing or renaming existing batch operations;
- changing `step_to_glb`, `project_step_hlr`, or `hlr_projection_json` return
  types;
- emitting default CLI deprecation text on stderr;
- adding C++ `[[deprecated]]` attributes before checking downstream
  warning-as-error usage.

Expected low-risk compatibility impact:

- docs and release notes will prefer `model_*` names;
- Python users who enable deprecation warnings may see warnings from old names;
- C++ users may see doc-level deprecation notes before any compiler-level
  deprecation attribute is introduced.

## Release Notes For Future Implementation

When implemented, release notes should say that generic model-input APIs are now
preferred, the only supported source format is currently STEP, and old
STEP-specific names remain compatibility wrappers. Bounds are stable; projection
outlines are experimental unless the ring-joining validation has been completed.
Downstream Altium projection policy belongs in the caller, not in Geometer.

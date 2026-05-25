# ADR 007: Generic Model Input API Names

## Status

Accepted

## Context

Geometer's first source-model operations were STEP-specific. Public names such
as `step_to_glb`, `project_step_hlr`, `step-to-glb`, and
`step_hlr_projection_json` accurately described the original implementation,
because STEP was the only supported source format.

The next API slice adds model bounds and may later add projected model outlines.
Those operations are conceptually source-model operations, not STEP-only
concepts. Geometer still supports only STEP as the source model format today,
but the public naming should not force every future format through
STEP-specific names.

The planar STEP API is different. It generates STEP from 2D region topology and
does not import a source model file. It should not be renamed as part of this
model-input cleanup.

## Decision

New source-model APIs use generic `model` naming and accept an explicit
`format` option. The only supported format value in the first implementation is
`"step"`.

Preferred Python names:

- `model_to_glb(..., format="step")`
- `project_model_hlr(..., format="step")`
- `model_hlr_projection_json(..., format="step")`
- `model_bounds(..., format="step")`
- future `model_projection_outline(..., format="step")`

Preferred CLI names:

- `model-to-glb ... --format step`
- `model-project-hlr ... --format step`
- `model-project-svg ... --format step`
- `model-bounds ... --format step`
- future `model-project-outline ... --format step`

Preferred batch operation names:

- `model_to_glb`
- `model_hlr_projection_json`
- `model_hlr_projection_svg`
- `model_bounds_json`
- future `model_projection_outline_json`

Preferred C++ and C ABI names should follow the same source-model distinction.
For example, the bounds API should be `model_bounds_from_bytes(...)` and the C
ABI should add symbols such as `geometer_model_bounds_json_bytes(...)`.

Existing STEP-specific names remain compatibility wrappers. They must preserve
current behavior, argument order, return types, output schemas, and command-line
behavior.

The old API is not deprecated in the same release that introduces the generic
names. Deprecation begins after one additional release cycle:

1. Release N: add generic `model_*` APIs and keep old STEP-specific names as
   supported compatibility wrappers.
2. Release N+1: mark old STEP-specific names deprecated in docs and release
   notes. Python wrappers may emit `DeprecationWarning`; CLI aliases remain
   quiet by default and only advertise deprecation in help/docs.
3. Removal is not authorized by this ADR. Removing compatibility wrappers needs
   a separate compatibility decision.

Planar STEP names stay stable:

- Python: `planar_step(...)` and `write_planar_step(...)`
- CLI: `planar-step`
- Batch operation: `planar_step`
- C++: `planar_step_from_json(...)` and
  `planar_step_from_json_bytes(...)`
- JSON schema: `geometry.planar_step.request.a0`

## Consequences

The generic names make future support for other source model formats possible
without another public rename.

The explicit `format="step"` boundary prevents accidental implied support for
GLB, STL, OBJ, or other formats before those paths exist.

Existing callers get at least one full release cycle before deprecation starts,
and removals require a later decision.

The model-input APIs and planar STEP generator remain intentionally separate.
This keeps `planar_step` descriptive for its actual input and avoids churn in
recent planar-region-to-STEP integrations.

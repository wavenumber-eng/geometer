# STEP Bounds And Projection Outline API Plan

Status: design plan, not started
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

## Design Principles

- Keep the API generic; no board, footprint, layer, or Altium-specific policy.
- Use millimeters for all geometer outputs.
- Reuse the existing executable-backed Python package model.
- Accept both STEP bytes and STEP paths in Python.
- Keep `model_transform` as the generic row-major 4x4 transform used by current
  HLR APIs.
- Make bounds the first stable surface; keep projection outlines explicitly
  experimental until ring closure quality is proven.

## Proposed Python Surface

```python
import geometer

bounds = geometer.step_bounds(
    step_bytes_or_path,
    model_transform=transform,
)

outline = geometer.step_projection_outline(
    step_bytes_or_path,
    view=geometer.ProjectionView.top(),
    model_transform=transform,
    options=geometer.ProjectionOutlineOptions.simple(),
)
```

Candidate dataclasses:

```python
@dataclass(frozen=True)
class StepBoundsResult:
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

## Proposed JSON Schemas

Bounds result schema:

```json
{
  "schema": "geometry.step_bounds.a0",
  "units": "mm",
  "source": { "hash": "..." },
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
  "schema": "geometry.step_projection_outline.a0",
  "units": "mm",
  "source": { "hash": "..." },
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
geometer step-bounds input.step output.json
geometer step-outline input.step output.json --view top --mode simple
```

Batch operations:

- `step_bounds_json`
- `step_projection_outline_json`

Both operations should accept `model_transform` in job or top-level options.
`step_projection_outline_json` can reuse the current HLR options where useful,
but should keep ring-joining options separate from raw HLR projection options.

## Proposed C++ Surface

Headers should mirror the current STEP/HLR style:

```cpp
struct StepBoundsOptions {
    std::array<double, 16> model_transform = identity;
};

struct StepBoundsResult {
    std::string schema = "geometry.step_bounds.a0";
    std::string units = "mm";
    std::string source_hash;
    std::array<double, 3> min;
    std::array<double, 3> max;
    std::array<double, 3> size;
    std::array<double, 3> center;
};

int step_bounds_from_bytes(
    const unsigned char* step_data,
    std::size_t step_size,
    const StepBoundsOptions& options,
    StepBoundsResult* result,
    Status* status = nullptr
);
```

Projection outline can build on current HLR extraction, but should be separated
from raw `HlrProjectionResult` so consumers do not need to join unordered
segments themselves.

## Implementation Sequence

1. Finalize the bounds JSON schema and Python dataclass shape.
2. Add C++ bounds value types and JSON writer/parser coverage.
3. Add direct CLI and batch operations for bounds.
4. Add Python `step_bounds(...)` and `StepBoundsResult`.
5. Validate against existing STEP fixtures and current downstream CadQuery
   bounds oracles.
6. Design the outline ring schema after reviewing more failure cases.
7. Implement projection outline as an experimental API only after ring joining
   tests are in place.

## Test Plan

- Unit tests for transform parsing and bounds JSON serialization.
- Native C++ tests for known STEP fixtures.
- CLI tests for path input.
- Python tests for path and byte input.
- Batch-runner tests for `step_bounds_json`.
- Downstream comparison cases against existing CadQuery-derived bounds.
- Additional user-provided STEP fixtures before outline API promotion.

## Release Notes For Future Implementation

When implemented, release notes should say that STEP bounds are a stable generic
API and projection outlines are experimental unless the ring-joining validation
has been completed. Downstream Altium projection policy belongs in the caller,
not in Geometer.

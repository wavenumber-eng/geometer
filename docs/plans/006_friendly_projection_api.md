# Plan 006 - Friendly HLR Projection API

Date: 2026-05-22
Status: planning

## Goal

Define a friendly HLR/projection API that feels similar across:

- native C++
- Python
- browser JavaScript/WASM

The API does not need to be identical in every language. It should share the
same nouns, defaults, and result shape so downstream tools can move between
native tooling, Python automation, and browser workers without relearning the
Geometer projection model.

This plan sits above the current low-level C ABI. The C ABI remains the stable
binding boundary. The friendly APIs hide allocation, JSON option plumbing,
library/module loading, and pointer ownership.

## Current Browser Path

The current HTML test pages do not call a high-level projection API directly.
They use a worker at `tests/wasm/hlr_projection_worker.js`.

Current flow:

1. The page fetches STEP bytes as an `ArrayBuffer`.
2. The page builds a JavaScript options object from UI controls.
3. The page sends `{ stepBuffer, views, options, backend }` to the worker.
4. The worker loads `dist/geometer.js` with `importScripts`.
5. The worker initializes `createGeometerModule(...)`.
6. The worker converts views/options into the C ABI JSON options payload.
7. The worker allocates WASM memory for STEP bytes, options JSON, and output
   pointers.
8. The worker calls `geometer_step_hlr_projection_json_bytes`.
9. The worker frees returned strings and temporary buffers.
10. The worker parses `geometry.projection.a0` JSON and sends the result back
    to the page.

That path is correct for a low-level bridge, but it is not the API shape we
want users or downstream tooling to think about.

## Initial Python Consumer: Altium Cruncher

Use the existing Altium Cruncher assembly projection path as the first serious
Python-wrapper use case.

Current Altium flow:

1. `altium_cruncher` delegates assembly SVG and Canvas projection work into
   `toolz/viz`.
2. `viz.altium_pcb_assembly_svg_renderer` resolves component bodies, embedded
   STEP payloads, Altium body pose fields, top/bottom side, and instance
   placement.
3. `viz.altium_pcb_svg_assembly_projection` uses the older Python OCP/ocp
   binding to load STEP, apply a pre-HLR model transform, run OCCT HLR, and
   return simple/detail line art.

That makes Altium Cruncher a useful migration target because the behavior is
already concrete:

- top view uses direction `[0, 0, 1]` and up `[0, 1, 0]`;
- bottom view uses direction `[0, 0, -1]` and up `[0, 1, 0]`;
- only visible sharp and visible outline edges are consumed today;
- `detail` is visible plus outline edge geometry;
- `simple` is contour/polygonized geometry built from selected projected edges;
- Altium-specific side, component placement, screen mirroring, and SVG Y-down
  handling live outside the HLR operation.

The key requirement this exposes is generic model transform support. The old
path calls HLR after applying a 4x4 transform matrix to the STEP shape. If the
new Python wrapper is going to replace that path without keeping OCP in
Altium/Viz, Geometer needs one of these generic options:

- accept an optional pre-projection `model_transform` matrix in the HLR request;
- accept a per-input `source_to_model` or `source_to_world` transform with
  clearly documented multiplication order;
- provide a separate native helper that loads STEP, applies a transform, and
  then projects.

The first option is the v1 direction. It keeps Altium and KiCad policy outside
Geometer while giving downstream tools a generic way to normalize source
geometry before projection.

## Shared Mental Model

All friendly APIs should expose these concepts:

- `ProjectionView`: one named orthographic view.
- `HlrProjectionOptions`: algorithm, mesh quality, curve mode, edge categories,
  and result precision.
- `HlrProjectionResult`: parsed projection result with views, simple/detail
  geometry, source hash, units, and timings.
- `Projector`: optional reusable object that owns a loaded backend or native
  library handle.
- `project_step_hlr`: simple function for one-shot STEP-byte projection.

The raw C ABI continues to expose:

- byte pointers
- JSON strings
- heap-allocated output pointers
- explicit free functions

Friendly APIs should not expose those details.

## Coordinate Convention

Geometer's friendly projection API should use the same right-handed coordinate
frame as the generic `pcb_a0` model:

- `+X`: east/right on a 2D monitor.
- `+Y`: north/up on a 2D monitor.
- `+Z`: out of the 2D screen, toward the viewer.

That convention is right-handed because `+X cross +Y = +Z`. Geometer should not
reinterpret source STEP coordinates by default. Instead, downstream importers
such as Altium Cruncher and KiCad Monkey should provide an optional
`model_transform` that maps the source model's local coordinates into this
Geometer/`pcb_a0` coordinate frame before projection.

`ProjectionView.direction` is the projection-axis normal used to construct the
OCCT projection axes, not a viewer-screen camera ray. With this convention,
`ProjectionView.top()` uses direction `[0, 0, 1]` and up `[0, 1, 0]`;
`ProjectionView.bottom()` uses direction `[0, 0, -1]` and up `[0, 1, 0]`.
Screen-space mirroring and SVG/canvas Y-down conversion remain caller policy.

## Defaults

The friendly default should target the common browser/KiCad assembly use case:

- algorithm: `poly`
- curve mode: `polyline`
- view: `top`
- visible sharp edges: enabled
- visible outline edges: enabled
- hidden edges: disabled
- smooth/sewn/iso edges: disabled
- union simple polygons: enabled
- round digits: `3`
- mesh linear deflection: `0.01` mm
- mesh angular deflection: `0.5` rad
- mesh relative: `false`
- HLR angle tolerance: `0.0174533` rad

Advanced callers can opt into hidden edges, exact HLR, native arcs, or custom
mesh quality. The defaults should stay useful for fast assembly/fabrication
projection.

## Core Request Shape

The shared request shape should be expressible as JSON, even when C++ uses value
types:

```json
{
  "views": [
    {
      "id": "top",
      "direction": [0.0, 0.0, 1.0],
      "up": [0.0, 1.0, 0.0]
    }
  ],
  "model_transform": [
    [1.0, 0.0, 0.0, 0.0],
    [0.0, 1.0, 0.0, 0.0],
    [0.0, 0.0, 1.0, 0.0],
    [0.0, 0.0, 0.0, 1.0]
  ],
  "projection_algorithm": "poly",
  "curve_mode": "polyline",
  "samples_per_curve": 24,
  "round_digits": 3,
  "union_simple_polygons": true,
  "edge_v_sharp": true,
  "edge_v_outline": true,
  "edge_v_smooth": false,
  "edge_v_sewn": false,
  "edge_v_iso": false,
  "edge_h_sharp": false,
  "edge_h_outline": false,
  "edge_h_smooth": false,
  "edge_h_sewn": false,
  "edge_h_iso": false,
  "mesh_linear_deflection": 0.01,
  "mesh_angular_deflection": 0.5,
  "mesh_relative": false,
  "hlr_angle_tolerance": 0.0174533
}
```

Use snake_case for the canonical serialized form. JavaScript wrappers may accept
camelCase aliases, but the documented cross-language form should be snake_case
to match the existing C ABI JSON parser.

`model_transform` is optional and defaults to identity. It is a generic
pre-projection transform applied to the loaded source shape before each view is
projected. The serialized form is row-major 4x4 and is applied to a column point
`[x, y, z, 1]^T`, so translation lives in the final column. It must not imply
PCB side, mirroring, screen coordinates, or any Altium/KiCad placement policy.

## Convenience Presets

Add named presets instead of making every caller remember edge flags.

Suggested presets:

- `assembly_outline`: fast poly HLR for visible sharp + visible outline.
- `visible_detail`: visible sharp, visible outline, visible smooth/sewn where
  supported.
- `visible_and_hidden`: visible and hidden sharp/outline categories.
- `exact_arcs`: exact HLR with native arcs where available.
- `debug_all_edges`: every supported edge category, intended for inspection.

Preset names should be the same in C++, Python, and JavaScript.

## C++ Friendly API

Keep the existing low-level status-based C++ implementation. Add a small
friendly layer above it.

Proposed C++ usage:

```cpp
#include "geometer/projection_api.h"

std::vector<unsigned char> step_bytes = read_file_bytes("part.step");

geometer::HlrRequest request;
request.views = {geometer::ProjectionView::top()};
request.options = geometer::HlrProjectionOptions::assembly_outline();

geometer::HlrProjectionResult result =
    geometer::project_step_hlr_or_throw(step_bytes, request);

const geometer::ProjectedModeGeometry& simple =
    result.view("top").geometry(geometer::ProjectionMode::Simple);

std::string json = result.to_json();
```

Proposed C++ API surface:

```cpp
namespace geometer {

enum class ProjectionMode { Simple, Detail };

struct ProjectionView {
    std::string id;
    std::array<double, 3> direction;
    std::array<double, 3> up;

    static ProjectionView top();
    static ProjectionView bottom();
    static ProjectionView front();
    static ProjectionView back();
    static ProjectionView right();
    static ProjectionView left();
    static ProjectionView camera(std::array<double, 3> direction,
                                 std::array<double, 3> up);
};

struct HlrProjectionOptions {
    static HlrProjectionOptions assembly_outline();
    static HlrProjectionOptions visible_detail();
    static HlrProjectionOptions visible_and_hidden();
    static HlrProjectionOptions exact_arcs();
    static HlrProjectionOptions debug_all_edges();

    // Existing option fields stay available for advanced callers.
};

struct HlrRequest {
    std::vector<ProjectionView> views;
    std::array<double, 16> model_transform;
    HlrProjectionOptions options;
};

int project_step_hlr(const unsigned char* step_data,
                     std::size_t step_size,
                     const HlrRequest& request,
                     HlrProjectionResult* result,
                     Status* status = nullptr);

HlrProjectionResult project_step_hlr_or_throw(
    const std::vector<unsigned char>& step_data,
    const HlrRequest& request = {}
);

std::string project_step_hlr_json_or_throw(
    const std::vector<unsigned char>& step_data,
    const HlrRequest& request = {}
);

} // namespace geometer
```

The non-throwing function should remain available for CLI and C ABI layers.
The throwing helpers are for friendly native app code and tests.

## Python Friendly API

Python should call the native C ABI through `ctypes`, but the public API should
look like normal Python.

Proposed Python usage:

```python
from geometer import HlrOptions, ProjectionView, project_step_hlr

step_bytes = Path("part.step").read_bytes()

projection = project_step_hlr(
    step_bytes,
    views=[ProjectionView.top()],
    options=HlrOptions.assembly_outline(),
)

simple = projection.view("top").simple
projection_json = projection.to_json()
```

Proposed Python API surface:

```python
@dataclass(frozen=True)
class ProjectionView:
    id: str
    direction: tuple[float, float, float]
    up: tuple[float, float, float]

    @staticmethod
    def top() -> "ProjectionView": ...
    @staticmethod
    def bottom() -> "ProjectionView": ...
    @staticmethod
    def front() -> "ProjectionView": ...
    @staticmethod
    def back() -> "ProjectionView": ...
    @staticmethod
    def right() -> "ProjectionView": ...
    @staticmethod
    def left() -> "ProjectionView": ...
    @staticmethod
    def camera(direction, up) -> "ProjectionView": ...

@dataclass
class HlrOptions:
    projection_algorithm: str = "poly"
    curve_mode: str = "polyline"
    samples_per_curve: int = 24
    round_digits: int = 3
    union_simple_polygons: bool = True
    mesh_linear_deflection: float = 0.01
    mesh_angular_deflection: float = 0.5
    mesh_relative: bool = False
    hlr_angle_tolerance: float = 0.0174533
    edge_v_sharp: bool = True
    edge_v_outline: bool = True
    edge_v_smooth: bool = False
    edge_v_sewn: bool = False
    edge_v_iso: bool = False
    edge_h_sharp: bool = False
    edge_h_outline: bool = False
    edge_h_smooth: bool = False
    edge_h_sewn: bool = False
    edge_h_iso: bool = False

    @classmethod
    def assembly_outline(cls) -> "HlrOptions": ...
    @classmethod
    def visible_detail(cls) -> "HlrOptions": ...
    @classmethod
    def visible_and_hidden(cls) -> "HlrOptions": ...
    @classmethod
    def exact_arcs(cls) -> "HlrOptions": ...
    @classmethod
    def debug_all_edges(cls) -> "HlrOptions": ...

def project_step_hlr(
    step: bytes | bytearray | memoryview | Path | str,
    *,
    views: Sequence[ProjectionView] | None = None,
    model_transform: Sequence[Sequence[float]] | None = None,
    options: HlrOptions | Mapping[str, Any] | None = None,
) -> HlrProjectionResult: ...

def project_step_hlr_json(
    step: bytes | bytearray | memoryview | Path | str,
    *,
    views: Sequence[ProjectionView] | None = None,
    model_transform: Sequence[Sequence[float]] | None = None,
    options: HlrOptions | Mapping[str, Any] | None = None,
) -> str: ...
```

The wrapper should accept bytes first. Accepting paths is a convenience that
reads the file in Python before calling the byte-based native ABI.

Errors should raise `GeometerError` with:

- native error code
- native error message
- function name
- version and ABI when available

## Python Viewer Work Product

Build a small Python viewer early, before migrating Altium/Viz. The viewer
should be a practical consumer of the Python wrapper, not a separate geometry
stack.

A Dear PyGui viewer is the first candidate:

- file picker for STEP/STP input;
- HLR projection canvas for `simple` and `detail` output;
- top, bottom, and custom `ProjectionView` controls;
- editable or preset `model_transform` controls for source-model normalization;
- projection option presets and timing/edge-count diagnostics;
- native error display with Geometer version and ABI;
- optional STEP geometry preview/export plumbing through `step_to_glb`.

This viewer gives the API an immediate acceptance test: if loading a part,
choosing a view, applying a transform, and inspecting the HLR result is awkward
in the viewer, the wrapper API is not friendly enough yet.

The first viewer does not need to solve all 3D rendering. If Dear PyGui cannot
comfortably display GLB/mesh geometry, start with a strong 2D HLR plot and keep
`step_to_glb` available for export or handoff. Add a direct `step_to_mesh`
Python API later if the viewer needs indexed triangles rather than GLB bytes.

## Browser JavaScript Friendly API

The browser API should wrap the current worker/module path.

Proposed JavaScript usage:

```js
import {
  createGeometerProjector,
  HlrOptions,
  ProjectionView,
} from "/dist/geometer-projector.js";

const projector = await createGeometerProjector({ backend: "/dist" });

const projection = await projector.projectStepHlr(stepBytes, {
  views: [ProjectionView.top()],
  options: HlrOptions.assemblyOutline(),
});

const simple = projection.view("top").simple;
```

Proposed JavaScript API surface:

```js
export const ProjectionView = {
  top(),
  bottom(),
  front(),
  back(),
  right(),
  left(),
  camera(direction, up),
};

export const HlrOptions = {
  assemblyOutline(overrides = {}),
  visibleDetail(overrides = {}),
  visibleAndHidden(overrides = {}),
  exactArcs(overrides = {}),
  debugAllEdges(overrides = {}),
};

export async function createGeometerProjector({
  backend = "/dist",
  workerUrl = "/tests/wasm/hlr_projection_worker.js",
} = {}) {
  return {
    projectStepHlr(stepBytes, { views, options } = {}) {},
    dispose() {},
  };
}
```

The worker can keep using the current Emscripten/C ABI calls internally. The
page should not need to know about `ccall`, `_malloc`, or returned string frees.

## Result Wrapper Shape

All friendly APIs should provide the same result conveniences:

- `schema`
- `units`
- `source_hash`
- `timings`
- `views`
- `view(id)`
- `geometry(view_id, mode)`
- `to_json()`

Python and JavaScript can wrap the parsed JSON. C++ can wrap
`HlrProjectionResult` directly and serialize with existing
`write_hlr_projection_json`.

Result geometry names should stay aligned with `geometry.projection.a0`:

- `simple.segments`
- `simple.arcs`
- `detail.segments`
- `detail.arcs`

Do not introduce PCB or viewer-specific names into the Geometer result.

## Implementation Phases

### Phase 1 - Documented Contract

- Add this plan.
- Update `INTERFACES.md` with a short "friendly APIs are planned" note after
  implementation begins.
- Add the early Python viewer target to Plan 005.

### Phase 2 - Transform-Capable Projection Request

- Add `model_transform` to native `HlrProjectionOptions`.
- Parse `model_transform` from C ABI JSON options.
- Apply the transform to the loaded STEP shape before projection.
- Document row-major 4x4 order and identity default.
- Add parser and projection smoke coverage.

### Phase 3 - Python Wrapper

- Implement the Python dataclasses and `ctypes` loader from Plan 005.
- Add `project_step_hlr`, `project_step_hlr_json`, and `step_to_glb`.
- Add smoke tests against a small STEP fixture.

### Phase 4 - Python Viewer

- Add a small Dear PyGui viewer example/tool.
- Load a STEP file, call the Python wrapper, and plot simple/detail HLR.
- Show projection timings, edge counts, version/ABI, and native errors.
- Exercise `model_transform` controls against known Altium/KiCad-style poses.

### Phase 5 - Altium Cruncher Pilot

- Replace the old `viz.altium_pcb_svg_assembly_projection` OCP path for a
  limited fixture set.
- Keep Altium body placement, screen mirroring, SVG Y-down conversion, and
  board-side policy outside Geometer.
- Compare old OCP output and new Geometer Python output on representative
  embedded STEP components.

### Phase 6 - Browser Wrapper

- Add a small browser wrapper module around `hlr_projection_worker.js`.
- Refactor the HTML test pages to use the wrapper.
- Keep direct worker messages available for tests.

### Phase 7 - C++ Convenience Layer

- Add `projection_api.h/.cpp` convenience wrappers.
- Keep existing `projection.h` value API stable.
- Add C++ tests for presets and view helpers.

### Phase 8 - Cross-Language Examples

Add equivalent examples that produce the same projection JSON:

- `examples/cpp/project_hlr.cpp`
- `examples/python/project_hlr.py`
- `examples/browser/project_hlr.html`

## Design Rules

- Keep Geometer generic.
- Keep C ABI as the binding boundary.
- Keep options byte/JSON serializable.
- Prefer presets for common use and raw flags for advanced use.
- Keep STEP input byte-oriented at the boundary.
- Do not expose Emscripten memory management in public browser APIs.
- Do not expose OCCT classes in Python or JavaScript APIs.
- Do not let downstream KiCad/Altium placement semantics leak into Geometer.

## Open Questions

1. Should friendly APIs return both `simple` and `detail` every time, or should
   callers be able to request only one mode for performance?
2. Should `mirrorX` stay outside Geometer as viewer policy, or become an
   optional view metadata field in the friendly browser wrapper only?
3. Should Python parse projection JSON into dataclasses, or return a dict-like
   wrapper over the JSON payload for v1?
4. Should C++ throwing helpers be included in the installed public header, or
   kept as examples/utilities?
5. Should `curve_mode` default to `polyline` in the core C++ value type too, or
   only in the friendly presets?
6. Should `model_transform` be one transform for the whole request, or should
   future APIs allow one transform per input body for batched projection?

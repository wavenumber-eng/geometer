# Geometer Interfaces

This file documents the current callable surface for Geometer. The C++ API is
the source-level API used inside this repository. The C ABI is the boundary used
for WASM and future non-C++ bindings.

## Interface Policy

Geometer interfaces must describe generic geometry operations. Keep downstream
application concepts such as PCB placement policy, Altium/KiCad names,
visualizer preferences, or Three.js scene behavior outside this repository.

New browser-capable APIs should normally have:

- a native C++ value API;
- a flat C ABI entry point for WASM and future non-C++ callers;
- byte-buffer inputs when the browser cannot rely on local files;
- documented ownership rules for returned strings, byte buffers, or mesh
  packets;
- options encoded in a stable JSON object when the option surface is expected
  to grow;
- version and ABI notes when the callable surface changes;
- at least one native test and one WASM/browser smoke path.

For STEP model rendering, prefer a backend-neutral mesh/tessellation packet if
it can stay compact and practical. Returning GLB bytes is acceptable when it
keeps the first integration simple, but it should not be the only long-term
geometry transport considered for browser tools.

## Header Entry Point

Use the umbrella header for native C++ callers:

```cpp
#include "geometer.h"
```

That includes the current public headers:

- `geometer/status.h`
- `geometer/version.h`
- `geometer/step_to_glb.h`
- `geometer/projection.h`
- `geometer/projection_options_json.h`
- `geometer/planar_contours.h`
- `geometer/planar_solve.h`

## Version

Defined in `src/cpp/lib/geometer/version.h`.

Geometer uses date-based release versions per ADR 006. The current package and
runtime version is `2026.5.23`, corresponding to release tag `v2026-05-23`.
The current C ABI generation is `20260523`. Consumers should check both the
project version and ABI generation at runtime.

```cpp
struct Version {
    int major = 0;
    int minor = 0;
    int patch = 0;
    int abi = 0;
    const char* string = "";
};

const Version& version();
const char* version_string();
int version_major();
int version_minor();
int version_patch();
int abi_version();
```

C ABI version functions:

```c
const char* geometer_version_string(void);
int geometer_version_major(void);
int geometer_version_minor(void);
int geometer_version_patch(void);
int geometer_abi_version(void);
```

WASM consumers can call the same C ABI exports:

```js
const version = module.ccall("geometer_version_string", "string", [], []);
const abi = module.ccall("geometer_abi_version", "number", [], []);
```

The returned version string is static storage and must not be freed.

## Status

Defined in `src/cpp/lib/geometer/status.h`.

```cpp
struct Status {
    int code = 0;
    std::string message;
    bool ok() const;
};
```

Most library functions return `0` on success. When a `Status*` parameter is
available and a failure occurs, `code` and `message` describe the error.

## STEP To GLB

Defined in `src/cpp/lib/geometer/step_to_glb.h`.

```cpp
struct StepToGlbOptions {
    double linear_deflection = 0.1;
    double angular_deflection = 0.5;
};

int step_to_glb(
    const std::string& step_path,
    const std::string& glb_path,
    const StepToGlbOptions& options = {}
);

int step_to_glb_from_bytes(
    const unsigned char* step_data,
    std::size_t step_size,
    const StepToGlbOptions& options,
    std::vector<unsigned char>* glb_bytes,
    Status* status = nullptr
);
```

`step_to_glb` is the file-based converter. `step_to_glb_from_bytes` accepts STEP
bytes and fills owned GLB bytes for browser/downstream consumers that cannot
depend on local files.

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

## HLR Projection

Defined in `src/cpp/lib/geometer/projection.h`.

```cpp
enum class ProjectionCurveMode {
    NativeArcs,
    Polyline
};

struct ProjectionViewSpec {
    std::string id = "top";
    std::array<double, 3> direction = {0.0, 0.0, 1.0};
    std::array<double, 3> up = {0.0, 1.0, 0.0};
};

struct HlrProjectionOptions {
    std::vector<ProjectionViewSpec> views;
    std::array<double, 16> model_transform = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    };
    ProjectionCurveMode curve_mode = ProjectionCurveMode::NativeArcs;
    int samples_per_curve = 24;
    int round_digits = 3;
    bool include_visible = true;
    bool include_outline = true;
    bool union_simple_polygons = true;
};
```

Projection output:

```cpp
struct ProjectedSegment {
    double x1 = 0.0;
    double y1 = 0.0;
    double x2 = 0.0;
    double y2 = 0.0;
};

struct ProjectedArc {
    std::array<double, 2> start = {0.0, 0.0};
    std::array<double, 2> end = {0.0, 0.0};
    std::array<double, 2> center = {0.0, 0.0};
    double radius = 0.0;
    double extent_rad = 0.0;
    bool ccw = true;
    bool full_circle = false;
};

struct ProjectedModeGeometry {
    std::vector<ProjectedSegment> segments;
    std::vector<ProjectedArc> arcs;
};

struct ProjectedViewGeometry {
    ProjectionViewSpec view;
    ProjectedModeGeometry simple;
    ProjectedModeGeometry detail;
};

struct HlrProjectionResult {
    std::string schema = "geometry.projection.a0";
    std::string units = "mm";
    std::string source_hash;
    std::vector<ProjectedViewGeometry> views;
};
```

Functions:

```cpp
int step_hlr_projection_from_bytes(
    const unsigned char* step_data,
    std::size_t step_size,
    const HlrProjectionOptions& options,
    HlrProjectionResult* result,
    Status* status = nullptr
);

int write_hlr_projection_json(
    const HlrProjectionResult& result,
    std::string* json,
    Status* status = nullptr
);

int write_hlr_projection_svg(
    const HlrProjectionResult& result,
    const std::string& view_id,
    const std::string& mode,
    std::string* svg,
    Status* status = nullptr
);
```

`step_hlr_projection_from_bytes` parses STEP bytes, runs OCCT HLR for each
requested view, and fills both `detail` and `simple` geometry. `simple` is built
from projected edge contours. `write_hlr_projection_json` emits
`geometry.projection.a0` JSON. `write_hlr_projection_svg` emits a quick
inspection SVG for one view and mode.

`model_transform` is an optional row-major 4x4 affine transform applied to the
loaded source shape before projection. Translation lives in the final column and
the final row must be `[0, 0, 0, 1]`. The transform is generic source-model
normalization; it does not imply PCB side, screen mirroring, or SVG/canvas
Y-down policy.

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
- `include_visible` or `includeVisible`.
- `include_outline` or `includeOutline`.
- `union_simple_polygons` or `unionPolygons`.

The browser test pages currently use the viz-compatible setting set:

```json
{
  "curve_mode": "polyline",
  "samples_per_curve": 24,
  "round_digits": 3,
  "include_visible": true,
  "include_outline": true,
  "union_simple_polygons": true
}
```

## Planar Contours

Defined in `src/cpp/lib/geometer/planar_contours.h`.

```cpp
struct PlanarContourPoint {
    double x = 0.0;
    double y = 0.0;
};

struct PlanarContourSegment {
    PlanarContourPoint start;
    PlanarContourPoint end;
};

struct PlanarContourRing {
    std::vector<PlanarContourPoint> points;
    bool hole = false;
    int nesting_depth = 0;
    double signed_area = 0.0;
};

struct PlanarContourOptions {
    int round_digits = 3;
    bool union_polygons = true;
    double area_epsilon = 1.0e-9;
};

struct PlanarContourResult {
    std::vector<PlanarContourRing> rings;
    std::vector<PlanarContourSegment> segments;
};

int build_planar_contours(
    const std::vector<PlanarContourSegment>& segments,
    const PlanarContourOptions& options,
    PlanarContourResult* result,
    Status* status = nullptr
);
```

This helper turns planar line segments into closed rings and output contour
segments. The HLR implementation uses it to generate simplified projection
geometry.

## Planar Batch Solve

Defined in `src/cpp/lib/geometer/planar_solve.h`.

The planar batch solve API performs generic 2D filled-geometry work:

1. starts with closed subject rings;
2. offsets open stroke paths into filled regions;
3. unions the subject and stroked fills;
4. subtracts local and optional common rings;
5. optionally clips each job to final clip rings;
6. returns outline/hole regions suitable for meshing or rendering.

It intentionally has no PCB, EDA, visualizer, or file-format policy.

```cpp
struct PlanarSolvePoint {
    double x = 0.0;
    double y = 0.0;
};

using PlanarSolvePath = std::vector<PlanarSolvePoint>;
using PlanarSolveRing = PlanarSolvePath;

struct PlanarSolveRegion {
    PlanarSolveRing outline;
    std::vector<PlanarSolveRing> holes;
};

enum class PlanarSolveJoinType { Miter, Round, Bevel, Square };
enum class PlanarSolveEndType { Round, Square, Butt, Joined };

struct PlanarSolveStrokeGroup {
    double radius_mm = 0.0;
    double miter_limit = 2.0;
    double arc_tolerance_mm = 0.0;
    PlanarSolveJoinType join_type = PlanarSolveJoinType::Miter;
    PlanarSolveEndType end_type = PlanarSolveEndType::Round;
    std::vector<PlanarSolvePath> paths;
};

struct PlanarSolveJob {
    std::vector<PlanarSolveRing> subject_rings;
    std::vector<PlanarSolveRing> subtract_rings;
    std::vector<PlanarSolveStrokeGroup> stroke_groups;
    bool subtract_common_rings = true;
    bool filter_common_subtract_by_bounds = true;
    bool clip_to_final_rings = true;
    double common_subtract_filter_margin_mm = 0.0;
};

struct PlanarBatchSolveOptions {
    int decimal_precision = 6;
    double cleanup_radius_mm = 0.0;
    double cleanup_miter_limit = 2.0;
    double cleanup_arc_tolerance_mm = 0.0;
};

struct PlanarSolveJobResult {
    std::vector<PlanarSolveRegion> regions;
    double area_mm2 = 0.0;
    std::uint32_t source_subject_ring_count = 0;
    std::uint32_t raw_subject_ring_count = 0;
    std::uint32_t stroke_path_count = 0;
    std::uint32_t stroke_region_count = 0;
    std::uint32_t local_subtract_ring_count = 0;
    std::uint32_t common_subtract_ring_count = 0;
};

struct PlanarBatchSolveInput {
    PlanarBatchSolveOptions options;
    std::vector<PlanarSolveRing> common_subtract_rings;
    std::vector<PlanarSolveRing> final_clip_rings;
    std::vector<PlanarSolveJob> jobs;
};

struct PlanarBatchSolveResult {
    std::vector<PlanarSolveJobResult> jobs;
};

int solve_planar_batch(
    const PlanarBatchSolveInput& input,
    PlanarBatchSolveResult* result,
    Status* status = nullptr
);

int solve_planar_batch_from_bytes(
    const unsigned char* request_data,
    std::size_t request_size,
    std::vector<unsigned char>* response_bytes,
    Status* status = nullptr
);
```

Closed rings should be supplied without a duplicate closing point. Output
outlines are oriented positive and holes are oriented negative. Inputs that
contain duplicate closing points are tolerated and cleaned.

### Planar Batch Byte Format

`solve_planar_batch_from_bytes` and the matching C ABI use a little-endian
binary packet. Version 2 request packets start with:

```text
bytes[8] magic = "GMPBRQ01"
u32 version = 2
u32 flags = 0
u32 decimal_precision
u32 job_count
f64 cleanup_radius_mm
f64 cleanup_miter_limit
f64 cleanup_arc_tolerance_mm
u32 common_subtract_ring_count
u32 final_clip_ring_count
u32 reserved0
u32 reserved1
```

Then all common subtract rings, then all final clip rings, then each job.

Ring and path encoding:

```text
u32 point_count
repeat point_count:
  f64 x
  f64 y
```

Job encoding:

```text
u32 flags
f64 common_subtract_filter_margin_mm
u32 subject_ring_count
u32 local_subtract_ring_count
u32 stroke_group_count
u32 reserved
subject rings...
local subtract rings...
stroke groups...
```

Job flags:

- `1`: subtract common rings.
- `2`: filter common subtract rings by expanded subject bounds.
- `4`: clip result to final clip rings.

Stroke group encoding:

```text
f64 radius_mm
f64 miter_limit
f64 arc_tolerance_mm
u32 join_type      // 0=miter, 1=round, 2=bevel, 3=square
u32 end_type       // 0=round, 1=square, 2=butt, 3=joined
u32 path_count
u32 reserved
open paths...
```

Response packets start with:

```text
bytes[8] magic = "GMPBRS01"
u32 version = 2
u32 job_count
u32 total_region_count
u32 total_ring_count
u32 total_point_count
u32 reserved
```

Each job then encodes:

```text
u32 region_count
u32 ring_count
u32 point_count
u32 source_subject_ring_count
f64 area_mm2
u32 raw_subject_ring_count
u32 stroke_path_count
u32 stroke_region_count
u32 local_subtract_ring_count
u32 common_subtract_ring_count
u32 reserved
regions...
```

Each region then encodes:

```text
u32 hole_count
u32 reserved
outline ring
hole rings...
```

## C ABI

Defined in `src/cpp/lib/geometer/c_api.h`.

```c
typedef struct GeometerBuffer {
    const unsigned char* data;
    size_t size;
} GeometerBuffer;

typedef struct GeometerStringResult {
    int code;
    char* value;
    char* error;
} GeometerStringResult;

typedef struct GeometerByteResult {
    int code;
    unsigned char* value;
    size_t size;
    char* error;
} GeometerByteResult;

GeometerStringResult geometer_step_hlr_projection_json(
    GeometerBuffer step_data,
    const char* options_json
);

int geometer_step_hlr_projection_json_bytes(
    const unsigned char* step_data,
    size_t step_size,
    const char* options_json,
    char** value,
    char** error
);

GeometerByteResult geometer_step_to_glb(
    GeometerBuffer step_data,
    const char* options_json
);

int geometer_step_to_glb_bytes(
    const unsigned char* step_data,
    size_t step_size,
    const char* options_json,
    unsigned char** value,
    size_t* value_size,
    char** error
);

GeometerByteResult geometer_planar_batch_solve(GeometerBuffer request_data);

int geometer_planar_batch_solve_bytes(
    const unsigned char* request_data,
    size_t request_size,
    unsigned char** value,
    size_t* value_size,
    char** error
);

const char* geometer_version_string(void);
int geometer_version_major(void);
int geometer_version_minor(void);
int geometer_version_patch(void);
int geometer_abi_version(void);

void geometer_free_string(char* value);
void geometer_free_bytes(unsigned char* value);
```

Returned `error` strings and projection JSON `value` strings are heap-allocated
and owned by the caller. Release them with `geometer_free_string`. Returned GLB
and planar batch byte buffers are heap-allocated and owned by the caller.
Release them with `geometer_free_bytes`. The version string is static storage
and does not use either free function.

## Python Interface

The source checkout includes a thin Python package under `python/geometer`. It
wraps the native C ABI and keeps the public API byte/path oriented:

```python
import geometer

version = geometer.version()
projection = geometer.project_step_hlr(
    "part.step",
    views=[geometer.ProjectionView.top()],
    options=geometer.HlrOptions.assembly_outline(),
)
json_text = geometer.hlr_projection_json("part.step")
glb_bytes = geometer.step_to_glb("part.step")
```

The Python wrapper looks for a loadable native library in this order:

- `GEOMETER_NATIVE_LIBRARY`;
- `GEOMETER_NATIVE_LIBRARY_DIR`;
- the package directory or `python/geometer/native`;
- source checkout `dist/`.

On Windows, Python uses direct `ctypes` calls when the native library is bundled
with shared OCCT runtime DLLs in the same directory. If only the older static
OCCT local build is available, OCCT-heavy HLR/GLB calls fall back to a small
worker subprocess to avoid static teardown crashes. Set
`GEOMETER_PYTHON_DIRECT=1` to force direct mode, or `GEOMETER_PYTHON_WORKER=1`
to force the worker bridge.

## WASM Interfaces

`scripts/build_wasm.py` builds three WASM targets into `dist/`.

Full Browser/Web Worker target:

- `dist/geometer.js`
- `dist/geometer.wasm`

This is the official application integration target. It is modularized with
the factory name `createGeometerModule` and includes the OCCT-backed STEP/HLR/GLB
APIs plus the planar byte APIs.

Node CLI parity/test target:

- `dist/geometer-node-test.js`
- `dist/geometer-node-test.wasm`

This target uses Node filesystem access for command-line parity and diagnostics.
Do not use it for browser integration.

Planar-only Browser/Web Worker target:

- `dist/geometer-planar-browser.js`
- `dist/geometer-planar-browser.wasm`

The full browser target exports:

- `_malloc`
- `_free`
- `_geometer_version_string`
- `_geometer_version_major`
- `_geometer_version_minor`
- `_geometer_version_patch`
- `_geometer_abi_version`
- `_geometer_step_hlr_projection_json`
- `_geometer_step_hlr_projection_json_bytes`
- `_geometer_step_to_glb`
- `_geometer_step_to_glb_bytes`
- `_geometer_planar_batch_solve`
- `_geometer_planar_batch_solve_bytes`
- `_geometer_free_string`
- `_geometer_free_bytes`

It also exports these Emscripten runtime helpers:

- `ccall`
- `cwrap`
- `UTF8ToString`
- `stringToUTF8`
- `lengthBytesUTF8`
- `getValue`

The planar-only browser target is modularized with the factory name
`createGeometerPlanarModule`. It exports only the version/free functions plus:

- `_malloc`
- `_free`
- `_geometer_planar_batch_solve`
- `_geometer_planar_batch_solve_bytes`

Use this target for browser workers that only need packed planar geometry
operations and should not pay the full OCCT/STEP WASM size, startup, and
worker-memory cost. The full browser target also exports these planar APIs, so
the planar-only target is an optimization, not a separate semantic API.

Minimal browser-worker shape:

```js
importScripts("/dist/geometer.js");

const module = await createGeometerModule({
  locateFile: (path) => path.endsWith(".wasm") ? `/dist/${path}` : path,
});

// Allocate STEP bytes and options JSON, then call:
module.ccall(
  "geometer_step_hlr_projection_json_bytes",
  "number",
  ["number", "number", "number", "number", "number"],
  [stepPtr, stepSize, optionsPtr, valueOutPtr, errorOutPtr],
);

module.ccall(
  "geometer_step_to_glb_bytes",
  "number",
  ["number", "number", "number", "number", "number", "number"],
  [stepPtr, stepSize, optionsPtr, valueOutPtr, valueSizeOutPtr, errorOutPtr],
);
```

The complete browser-worker example lives at
`tests/wasm/hlr_projection_worker.js`.

## CLI Interfaces

Native CLI:

```powershell
.\dist\geometer.exe --version
.\dist\geometer.exe step-to-glb input.step output.glb
.\dist\geometer.exe step-project-hlr input.step output.json
.\dist\geometer.exe step-project-svg input.step output.svg --mode simple --view top
.\dist\geometer.exe planar-batch-solve request.bin response.bin --warmup 1 --repeat 5 --metrics metrics.json
```

Node WASM CLI parity/test target:

```powershell
node dist\geometer-node-test.js step-to-glb input.step output.glb
```

Projection CLI options:

- `--view <id>`
- `--mode <simple|detail>`
- `--curve-mode <native-arcs|polyline>`
- `--samples <count>`
- `--round-digits <count>`

STEP-to-GLB CLI options:

- `--deflection <value>`
- `--angular <value>`

Planar batch solve CLI options:

- `--warmup <count>`: run unmeasured solves before benchmark repeats.
- `--repeat <count>`: measured solve repeats.
- `--metrics <path>`: write JSON metrics with request/response byte sizes and
  min/mean/max/last solve time.

`planar-batch-solve` uses the same packed request/response byte format as
`solve_planar_batch_from_bytes` and the browser C ABI. It is intended for
native-vs-WASM diagnostics and benchmark comparisons.

## Distribution Artifacts

The repository policy is to commit distributable outputs in `dist/` so another
project can clone Geometer and use the CLI/WASM artifacts without rebuilding.

Persist these when publishing interface changes:

- Native CLI: `dist/geometer.exe` or `dist/geometer`.
- Windows shared OCCT runtime DLLs: `dist/TK*.dll`.
- Native static library: `dist/geometer.lib` or `dist/libgeometer.a`.
- Full browser WASM C ABI: `dist/geometer.js` and `dist/geometer.wasm`.
- Node WASM CLI parity/test target: `dist/geometer-node-test.js` and
  `dist/geometer-node-test.wasm`.
- Planar-only browser WASM C ABI optimization:
  `dist/geometer-planar-browser.js` and `dist/geometer-planar-browser.wasm`.

Do not commit local generated build state:

- `.deps/`
- `build/`
- `build-wasm/`

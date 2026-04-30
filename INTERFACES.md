# Geometer Interfaces

This file documents the current callable surface for Geometer. The C++ API is
the source-level API used inside this repository. The C ABI is the boundary used
for WASM and future non-C++ bindings.

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

## Version

Defined in `src/cpp/lib/geometer/version.h`.

Geometer v0.1.0 uses C ABI version `1`. The project version follows semver.
While Geometer is under `0.x`, public interface changes may still happen, but
consumers should check both the project version and ABI version at runtime.

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
```

This is a file-based converter. It reads a STEP file and writes a GLB file.

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

const char* geometer_version_string(void);
int geometer_version_major(void);
int geometer_version_minor(void);
int geometer_version_patch(void);
int geometer_abi_version(void);

void geometer_free_string(char* value);
```

The returned `value` and `error` pointers are heap-allocated strings owned by
the caller. Release any non-null returned string with `geometer_free_string`.
The version string is static storage and does not use `geometer_free_string`.

## WASM Interfaces

`scripts/build_wasm.py` builds two WASM targets into `dist/`.

Node CLI target:

- `dist/geometer.js`
- `dist/geometer.wasm`

Browser/Web Worker target:

- `dist/geometer-browser.js`
- `dist/geometer-browser.wasm`

The browser target is modularized with the factory name
`createGeometerModule`. It exports:

- `_malloc`
- `_free`
- `_geometer_version_string`
- `_geometer_version_major`
- `_geometer_version_minor`
- `_geometer_version_patch`
- `_geometer_abi_version`
- `_geometer_step_hlr_projection_json`
- `_geometer_step_hlr_projection_json_bytes`
- `_geometer_free_string`

It also exports these Emscripten runtime helpers:

- `ccall`
- `cwrap`
- `UTF8ToString`
- `stringToUTF8`
- `lengthBytesUTF8`
- `getValue`

Minimal browser-worker shape:

```js
importScripts("/dist/geometer-browser.js");

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
```

Node WASM CLI:

```powershell
node dist\geometer.js step-to-glb input.step output.glb
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

## Distribution Artifacts

The repository policy is to commit distributable outputs in `dist/` so another
project can clone Geometer and use the CLI/WASM artifacts without rebuilding.

Persist these when publishing interface changes:

- Native CLI: `dist/geometer.exe` or `dist/geometer`.
- Native static library: `dist/geometer.lib` or `dist/libgeometer.a`.
- Node WASM CLI: `dist/geometer.js` and `dist/geometer.wasm`.
- Browser WASM C ABI: `dist/geometer-browser.js` and
  `dist/geometer-browser.wasm`.

Do not commit local generated build state:

- `.deps/`
- `build/`
- `build-wasm/`

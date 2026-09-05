# Native mesh illustration (development candidate)

## Availability and authority

The source branch provides `geometer::illustrate_mesh` in
`geometer/mesh_illustration.h`, linked through `geometer_lib`. It performs CPU
vector illustration without a JavaScript engine, browser, GPU or WASM runtime.
This is an **unreleased direct C++ API**. It is not yet an executable IPC
operation or a Rust/Python illustration method. Do not infer availability in
released binaries from the generated illustration DTOs.

Public input, style and result values come from the existing TypeSpec
[`mesh-illustration-a0.tsp`](../../src/tsp/geometer/operations/mesh-illustration-a0.tsp).
The C++ API uses those generated structures directly. Its private scene,
visibility graph and drawing commands are implementation details, not new wire
formats. The existing [browser API](hlr-projection-a0.md) remains supported.

## Calling the value API

```cpp
#include "geometer/mesh_illustration.h"
#include "geometer/model_tessellation.h"

geometer::contracts::MeshCollectionA0 collection;
geometer::Status status;
int code = geometer::model_tessellation_from_bytes(
    step_bytes.data(), step_bytes.size(), {}, &collection, &status);
if (code != 0) { /* report status.message and stop */ }

geometer::contracts::MeshIllustrationInputA0 input;
input.meshes = std::move(collection.meshes);
input.view.direction = {0, 0, 1};
input.view.up = {0, 1, 0};
geometer::contracts::MeshIllustrationResultA0 result;
code = geometer::illustrate_mesh(input, &result, &status);
if (code != 0) { /* report status.message and stop */ }
// result.svg is vector XML; result.stats and result.warnings are generated A0 values.
```

See [colored tessellation](model-tessellation-a0.md) for millimeter units,
placement, materials and STEP security policy. The illustration operation itself
accepts meshes, not files. It never reads local paths or fetches external assets.

## Rendering semantics

The native implementation follows the production TypeScript renderer:

- Orthographic direction/up basis, optional horizontal reflection and
  column-major mesh transforms; reflected winding and inverse-transpose normals.
- sRGB source materials, opacity, and presence-preserving A0 style defaults;
  unlit, flat, Lambert, banded and toon shading.
- Overlap-based visibility constraints, stable strongly connected component
  ordering, safe adjacent-surface fusion and coplanar material layers.
- Closed-mesh silhouettes and shared creases. Open triangle-soup edges do not
  automatically become outlines.
- Integer-grid SVG coordinates, six-percent padding, stable CSS classes,
  background/title options and chained line paths.

This pure API does not compute or accept optional CAD HLR layers yet. In
particular, `show_hlr_outline` and `show_hlr_detail` do not request extra HLR
computation. Native Fast HLR composition needs a separate governed wrapper;
the browser's `illustrated-hlr` composition is not silently substituted.

SVG text escapes XML delimiters and rejects XML 1.0 forbidden characters or
invalid Unicode. That hardening is shared with TypeScript. CSS color validation
matches ECMAScript whitespace/case-folding semantics and rejects unsafe CSS to
the established black fallback. Decimal output uses JavaScript-compatible
halfway rounding rather than C++ formatting's ties-to-even behavior.

## Errors and resource limits

The function returns zero on success; nonzero status clears the result,
including SVG. Invalid generated DTOs, invalid view bases, numeric overflow and
invalid XML text fail explicitly. Out-of-range or degenerate triangles retain
the browser warning-and-skip policy; warnings are capped at 256 entries.

The generated prepare option defaults to 750,000 source triangles and allows
at most 2,000,000. Native processing additionally rejects more than 20,000,000
candidate/simplification work items, 4,000,000 visibility constraints or stored
same-style overlaps, and complete SVG documents larger than 256 MiB. These
limits return status 102; there is no approximate-renderer fallback.

These are accepted-work/output limits, not a hard peak-memory or CPU sandbox.
Generated value validation serializes input; scene/graph/SVG assembly allocates
intermediate storage. Callers processing untrusted inputs should use the
managed executable boundary with process limits once its adapter is available.

## Focused conformance evidence

The native CTest smoke covers successful fusion, determinism, triangle caps,
invalid views, exact output-cap boundaries, decimal halfway rounding, XML text
and extreme finite inputs. `geometer_mesh_illustration_parity` compares the
entire generated result and SVG byte-for-byte with TypeScript, repeats native
calls for determinism, and parses every SVG with an XML parser.

Shared fixtures cover shading modes, views, colors, transforms, reflection,
normals, opacity, fusion, material inlays/holes, overlapping triangle soup,
warnings, Unicode CSS, title/viewport options and the colored SOT-23 STEP model.
Windows has local conformance evidence. macOS/Linux runtime qualification,
executable/Rust/Python integration and GUI acceptance remain open.

```powershell
cmake --build build --target geometer_mesh_illustration_test
ctest --test-dir build -R geometer_mesh_illustration --output-on-failure
```

The differential CTest is registered when Node is present. It uses CMake's
Python interpreter for XML parsing. The TypeScript Rack lane also runs it when
`GEOMETER_REQUIRE_NATIVE_TEST_SERVERS=1`; set `GEOMETER_ILLUSTRATION_TEST` for
non-default build locations. Contract tooling/Node must be installed for parity
qualification; passing the native-only smoke is not full conformance evidence.

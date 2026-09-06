# Native mesh illustration (development candidate)

## Availability and authority

The source branch provides `geometer::illustrate_mesh` in
`geometer/mesh_illustration.h`, linked through `geometer_lib`. It performs CPU
vector illustration without a JavaScript engine, browser, GPU or WASM runtime.
This is **unreleased development functionality**, also exposed by
`geometry.mesh_illustration.a0` through the generic operation ABI and native
`serve --stdio` dispatcher, with typed Rust/Python methods. Use matching feature
clients and executable; the previous released binary does not expose it.

Public input, style and result values come from the existing TypeSpec
[`mesh-illustration-a0.tsp`](../../src/tsp/geometer/operations/mesh-illustration-a0.tsp).
The C++ API uses those generated structures directly. Its private scene,
visibility graph and drawing commands are implementation details, not new wire
formats. The existing [browser API](hlr-projection-a0.md) remains supported.

The executable settings and attachment declaration come from
[`mesh-illustration-operation-a0.tsp`](../../src/tsp/geometer/operations/mesh-illustration-operation-a0.tsp).
It reuses the same view/prepare/style/SVG fields and requires one
`mesh_collection` attachment, media type
`application/vnd.wavenumber.geometer.mesh-collection+json`, containing UTF-8
`geometry.mesh_collection.a0` JSON (maximum 256 MiB). No private indexed-mesh
layout, base64 JSON or local file path is required. The result is the existing
`geometry.mesh_illustration.result.a0`, including inline SVG, not a relabeled
attachment descriptor. There are no output attachments.

An optional `hlr_projection` attachment contains the existing generated
`geometry.hlr_projection.result.a0` JSON, media type
`application/vnd.wavenumber.geometer.hlr-projection+json`, maximum 64 MiB.
Exactly one view must match the illustration's normalized direction/up basis;
units must be millimeters. Request `curve_mode: polyline`: arcs are rejected,
not silently approximated. All layers combined are limited to 1,000,000
segments, including disabled layers and bbox. The renderer uses the mesh
bounds/viewport and applies `mirror_x` to both fills and supplied linework.

Supply **visible-only** HLR from the same model, placement and transform as the
meshes. The projection contract does not bind its source hash to a mesh
collection or retain the producing options; composition cannot verify those
relationships or recover visibility from arbitrary 2D segments. The complete
STEP examples and demo use the same STEP bytes and stripped root placement,
with no additional model transform. Hidden HLR edges must remain disabled.

## Rust and Python executable clients

Both clients accept the existing generated `MeshIllustrationInputA0` and adapt
it to generated settings plus the mesh attachment. This keeps the public
illustration input shape consistent with the browser and direct C++ API.

Python also exposes public one-shot `geometer.model_tessellation(step_bytes)`
and `geometer.mesh_illustration(input, hlr_projection=hlr)` helpers. Each owns
and closes one maintained IPC client; no additional wire implementation exists.
They accept optional `executable=` and `timeout=` keywords. Prefer the persistent
client in the complete workflow below so all stages reuse one process.

The complete [Rust example](../../src/rust/geometer-client/examples/mesh_illustration.rs)
spawns the executable, tessellates STEP bytes, computes visible Fast HLR,
renders the combined illustration through the typed client,
closes the process and writes SVG:

```powershell
cargo run --manifest-path src/rust/geometer-client/Cargo.toml --example mesh_illustration -- PATH_TO_MATCHING_GEOMETER INPUT.step OUTPUT.svg
```

Equivalent Python:

```python
from pathlib import Path
import geometer

with geometer.GeometerClient(executable="PATH_TO_MATCHING_GEOMETER") as client:
    step = Path("INPUT.step").read_bytes()
    tessellated = client.model_tessellation(step)
    view = geometer.MeshIllustrationView(direction=(0.4, 0.7, 1.0), up=(0.0, 1.0, 0.0))
    hlr = client.model_hlr_projection(step, geometer.HlrProjectionOptionsA0(
        views=(geometer.HlrViewSpec(id="illustration", direction=view.direction, up=view.up),),
        projection_algorithm=geometer.HlrProjectionAlgorithm.FAST,
        outline_algorithm=geometer.HlrOutlineAlgorithm.FAST_MESH_SHADOW,
        curve_mode=geometer.HlrCurveMode.POLYLINE,
        strip_root_placement=True, output_outline=True, output_detail=True, output_bbox=False,
        fast=geometer.FastHlrOptionsA0(include_hidden=False),
    ))
    result = client.mesh_illustration(geometer.MeshIllustrationInputA0(
        schema="geometry.mesh_illustration.input.a0",
        meshes=tessellated.mesh_collection.meshes,
        view=view,
        style=geometer.MeshIllustrationStyleA0(
            shading=geometer.MeshIllustrationShading.TOON,
            show_outlines=False, show_creases=False,
            show_hlr_outline=True, show_hlr_detail=True),
    ), hlr_projection=hlr)
Path("OUTPUT.svg").write_text(result.svg, encoding="utf-8")
```

Rust remains asynchronous; Python is synchronous and accepts `timeout=`.
Both retain the existing negotiation, cancellation/process lifecycle and
protocol-failure behavior. A failed illustration is an operation error, not
automatic renderer fallback. Malformed results terminate the owned connection.

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
- Raw closed-mesh silhouettes and shared creases. These diagnostic strokes
  are drawn after surfaces and are **not occlusion-filtered**; rear edges can
  appear over foreground faces. Open triangle-soup edges do not automatically
  become outlines. The browser Lab disables these strokes and instead supplies
  filtered HLR linework; see the [Lab/native comparison](../developer/native-illustration-lab-parity.md).
- Integer-grid SVG coordinates, six-percent padding, stable CSS classes,
  background/title options and chained line paths.

The pure `illustrate_mesh(input, result, status)` / Rust `mesh_illustration(input)`
calls still do not compute HLR. For a finished layered SVG, use the C++ overload
`illustrate_mesh(input, hlr, result, status)`, Rust
`mesh_illustration_with_hlr(input, hlr)`, or Python's `hlr_projection=` keyword.
`show_hlr_detail` and `show_hlr_outline` select supplied lines; detail is drawn
before outline, above fills, exactly as in the web Lab. Consumers do not manage
SVG z-order. They may retain the original HLR result for independent layers.

### Fuse surfaces — enabled by default

`MeshIllustrationStyleA0.fuse_surfaces` defaults to **`true` when omitted** in
both the native C++ and TypeScript renderers. The Rust demo also explicitly
starts with it enabled. Leave it on for normal illustration and SVG export.

Fusion combines compatible adjacent projected triangles with matching rendered
fill color and opacity into larger drawing surfaces, subject to the renderer's
visibility-order constraints. This can reduce SVG paths, browser Canvas draw
commands and visible internal tessellation seams. Savings depend on the model,
view and shading; it does not guarantee a single path per source material.

This is an **illustration rendering option**, not CAD solid union, source-mesh
decimation, color-palette reduction, or the experimental analytic planar solver.
It does not modify the source STEP/mesh or independently computed HLR geometry.
Rust/Python clients pass the generated style option to the native renderer;
they do not perform a separate fusion pass.

Set `fuse_surfaces: false` for comparison or diagnostics. The renderer then
draws ordered individual triangles, usually producing more surface commands.
Fusion adds rendering work and remains subject to the documented resource
limits. `stats.surface_draws` and SVG byte size help compare a particular model;
`stats.triangles` still counts rendered source triangles, not fused surfaces.

`layer_coplanar_materials` is separate: it controls coplanar material layering
within the fusion path and has no effect when fusion is disabled. SVG CSS style
deduplication and same-style line chaining are automatic even without surface
fusion; they do not require a consumer pass.

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
managed executable boundary with process limits.

IPC still has an **8 MiB encoded JSON envelope limit**, including escaped SVG,
warnings and outcome metadata. A larger result returns
`geometer.transport.response_limit_exceeded`; the process stays usable. No
implicit truncation, oversized frame or hidden alternate result schema is used.

## Focused conformance evidence

The native CTest smoke covers successful fusion, determinism, triangle caps,
invalid views, exact output-cap boundaries, decimal halfway rounding, XML text
and extreme finite inputs. `geometer_mesh_illustration_parity` compares the
entire generated result and SVG byte-for-byte with TypeScript, repeats native
calls for determinism, and parses every SVG with an XML parser.

Shared fixtures cover shading modes, views, colors, transforms, reflection,
normals, opacity, fusion, material inlays/holes, overlapping triangle soup,
warnings, Unicode CSS, title/viewport options and the colored SOT-23 STEP model.
Windows has local conformance evidence, including typed Rust and Python
STEP-to-SVG calls and exact Rust IPC output versus the TypeScript oracle.
Malformed attachments, triangle limits and oversized inline SVG recover without
poisoning the process. Windows installed-wheel and clean packaged-Rust consumer
checks now pass, and the user accepted the Rust demo as proof of the workflow.
macOS/Linux runtime qualification and clean public-release gates remain open;
see the [API readiness handoff](../developer/native-api-readiness.md).
Native composition additionally has
24 exact TypeScript/IPC comparisons (three views, reflection and four line
toggle combinations), deterministic repeats, Python typed composition and
malformed-attachment checks, and C++ basis/finite-value/segment-cap regressions.

```powershell
cmake --build build --target geometer_mesh_illustration_test
ctest --test-dir build -R geometer_mesh_illustration --output-on-failure
```

The differential CTest is registered when Node is present. It uses CMake's
Python interpreter for XML parsing. The TypeScript Rack lane also runs it when
`GEOMETER_REQUIRE_NATIVE_TEST_SERVERS=1`; set `GEOMETER_ILLUSTRATION_TEST` for
non-default build locations. Contract tooling/Node must be installed for parity
qualification; passing the native-only smoke is not full conformance evidence.

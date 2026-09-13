# Model illustration A0

Model illustration completes import or analytic lowering, Fast vector linework,
technical shading, surface fusion, and output construction in one Geometer
operation. Consumers do not transfer an intermediate triangle-mesh JSON value.

The TypeSpec authority is
[`model-illustration-a0.tsp`](../../src/tsp/geometer/operations/model-illustration-a0.tsp).
The operation and attachment projections are declared in
[`model-illustration-operation-a0.tsp`](../../src/tsp/geometer/operations/model-illustration-operation-a0.tsp).
Generated schemas and styled contract pages are the field-level reference.

## Operations

`geometry.model_illustration.a0` returns an inline deterministic SVG result.
`geometry.model_illustration_geometry.a0` runs the same source preparation and
rendering policy, then returns the existing renderer-neutral
`geometry.mesh_illustration.geometry.a0` value in one governed JSON attachment.
Use the latter when a Canvas, PDF, editor, or application renderer owns final
drawing serialization.

Both operations accept one of two source variants:

- `kind: "model"` requires exactly one attachment named `model`, with media
  type `application/step` or `model/step`. STEP is the only model format in A0.
- `kind: "analytic"` accepts no attachment. Definitions contain reusable
  extrusion, vertical cylinder, and sphere primitives; occurrences place those
  definitions with column-major affine transforms.

File paths, PCB meaning, component placement, and format fallback remain in
application adapters. The operation never reads a path or resolves an external
model reference. External STEP references fail with
`geometer.contract.external_model_reference`.

## Model preparation

The model source reuses the colored tessellation controls. Root placement is
normalized first, followed by the optional source transform. Geometer validates
that transform as finite, affine, nonsingular, and adequately conditioned. It
applies inverse-transpose normal transformation and reverses triangle winding
for reflections. A material override, when present, replaces imported surface
materials after placement.

Partial tessellation defaults to enabled. Recoverable failed faces produce
warnings and usable faces continue. When OCCT reports an outdated triangulation,
Geometer retries once after cleaning the model; any faces still marked outdated
are omitted under the same partial-result policy. A model with no usable
prepared geometry fails the operation.

## Analytic 2.5D source

Each reached definition is validated and lowered once per request. Every
definition must be referenced, definition and occurrence IDs must be unique,
and an occurrence must reference a known definition. The native implementation
then copies the lowered local meshes and applies occurrence transforms. This
keeps authored geometry compact and leaves a clean definition/occurrence seam
for future persistent caching.

Extrusions use one or more planar regions. A region has an outer ring and
optional holes. Ring points and segments have equal counts; segment `i` joins
point `i` to the next point, including the closing segment. Lines remain exact.
Circular arcs use an explicit center and clockwise/counterclockwise sweep, then
sample against bounded linear and angular deflection. Caps use the maintained
planar triangulator and walls are emitted directly between `z_min_mm` and
`z_max_mm`.

Cylinders are vertical with a 2D center, radius, and Z interval. Spheres use a
3D center and radius. Both are sampled in native code from the same bounded
deflection settings. Their source contract remains analytic; sampled meshes are
private working data and never cross IPC.

Authored, generated, definition-local, occurrence-expanded, and drawing work
all have explicit limits. Limit failures return
`geometer.operation.resource_limit_exceeded` before an unbounded result is
allocated or serialized. The analytic broad phase also has a bounded topology
candidate-pair budget (`max_topology_candidate_pairs`) so complex region
validation cannot grow without a caller-visible limit.

## Illustration policy

The combined operation always uses Fast vector detail and Fast Mesh Shadow.
Hidden lines are not part of a finished illustration and `include_hidden: true`
is rejected. `show_hlr_outline` and `show_hlr_detail` select the visible Fast
layers; fill styling and same-color fusion reuse the mesh illustration A0
contract. One normalized view governs fills and linework.

Successful results include the prepared source bounds in millimeters. The raw
drawing geometry also carries physical line widths in projected millimeters;
SVG and custom renderers therefore receive the same lineweight intent without
recovering it from a normalized style fraction.

The source is imported or lowered once into an in-memory mesh collection. Fast
linework and illustration consume that same collection, although each engine
may derive private view-specific acceleration data. Before flattening source
faces for Fast linework, the implementation counts the complete vertex and
triangle totals and reserves each contiguous array once; cost therefore remains
linear for STEP files split into many face meshes. Phase timings therefore
report source preparation, linework, illustration, and raw-attachment encoding
without implying that OCCT import and tessellation are separately observable.

See [technical illustration algorithms](technical-illustration-algorithms.md)
for the pipeline and determinism rules and [illustration drawing geometry](mesh-illustration-geometry.md)
for custom-renderer semantics.

## Python

```python
from pathlib import Path
import geometer

request = geometer.ModelIllustrationRequestA0(
    schema="geometry.model_illustration.request.a0",
    source=geometer.ModelAttachmentIllustrationSourceA0(
        kind="model", attachment="model"
    ),
    view=geometer.MeshIllustrationView(
        direction=(0.4, 0.7, 1.0), up=(0.0, 1.0, 0.0)
    ),
    style=geometer.MeshIllustrationStyleA0(show_hlr_detail=True),
)

with geometer.GeometerClient() as client:
    result = client.model_illustration(request, Path("package.step").read_bytes())
Path("package.svg").write_text(result.svg, encoding="utf-8")
```

`geometer.model_illustration(...)` owns a process for one call. The persistent
client should be reused for batches. `model_illustration_geometry(...)` accepts
the generated geometry request and returns a `ModelIllustrationGeometry`
composite. Its `metadata` member preserves source summary, timings, statistics,
and warnings; its `geometry` member contains the decoded
`MeshIllustrationGeometryA0`. The helper validates the attachment media type,
length, digest, schema, statistics, and warnings before returning either value.

The asynchronous Rust client exposes methods with the same two names. Generated
TypeScript DTOs/codecs and operation metadata work with the generic native,
worker, or WASM operation transport. The generic C ABI accepts the same logical
request plus optional model attachment.

## Examples

- [`model_illustration_analytic.py`](../../examples/python/model_illustration_analytic.py)
  illustrates repeated analytic bodies, a cylinder, and a sphere.
- [`illustration_geometry_canvas.py`](../../examples/python/illustration_geometry_canvas.py)
  sends STEP once and draws returned raw geometry on Canvas.
- [`mesh_illustration.rs`](../../src/rust/geometer-client/examples/mesh_illustration.rs)
  sends STEP once and writes the returned SVG.

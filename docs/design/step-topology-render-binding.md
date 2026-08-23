# Direct STEP Topology Render Binding Research

Status: experimental native value API; not a GLB or wire contract

Date: 2026-08-22

## Outcome

Geometer can now tessellate the live XCAF session while retaining an exact
renderer-neutral relationship from an instance and mesh-local triangle ordinal
to one occurrence, body, and face target. This establishes the geometry
relationship before selecting a GLB metadata representation.

The implementation is `step_topology_render_binding.cpp`, exposed through the
experimental value types in `geometer/step_topology_session.h`. It produces:

- one indexed definition-local mesh for each simple shape definition;
- one primitive candidate and contiguous triangle span per selectable face;
- one render instance for every leaf root/component occurrence, with its fully
  accumulated transform;
- a bounded occurrence/primitive/triangle-span table; and
- reverse resolution from `(instance_index, triangle_index)` to live opaque
  occurrence, body, and face handles.

Repeated occurrences share one definition mesh but retain distinct occurrence
handles. Root placement is carried only on the instance. A render artifact is
valid only for its session and generation; cross-session and stale-generation
resolution fail closed.

Every artifact also carries a content SHA-256 and an opaque `gtr_` seal derived
from the session secret, generation, and content digest. Hit resolution
recomputes that seal and validates the complete occurrence-to-definition,
instance-to-mesh, primitive-to-span, body-to-face, and live target-kind chain.
It rejects modified provenance, re-stamping onto another public session record,
forged handles, reordered or duplicated bindings, gaps, and overlapping spans.
The value artifact is therefore safe to treat as caller-mutable research input.
The selected GLB work packet preserves equivalent checks with a second seal
over the exact GLB bytes and this render identity.

## Coordinates, Winding, And Normals

OCCT transfers source geometry into its normalized millimeter working unit.
Positions remain definition-local. The render options explicitly contain a
row-major signed-rigid 3x4 transform from that OCCT frame to the render frame;
scale and shear are rejected. The transform is left-multiplied with each
accumulated occurrence transform.

Reflections are supported and reported through `front_face_reversed`, allowing
a renderer to reverse its front-face convention for that instance. Face
orientation controls both emitted index winding and vertex-normal direction.
The proof checks every generated box triangle has winding consistent with its
average normal, checks a reflected coordinate frame, and checks the generated
slab's translated/rotated source root while its mesh remains at the modelling
origin. The reflection result proves the signed-rigid render-frame path; the
current STEP corpus does not contain an imported mirrored assembly placement,
so no such source-file claim is made yet.

Tessellation uses explicit absolute/relative linear deflection, angular
deflection, and serial/parallel settings. The baseline uses 0.1 mm absolute,
0.5 radians, and serial meshing. A cancellation progress range is passed to
OCCT and cancellation is also checked while copying every face and triangle.

## Bounds And Failure Behavior

Session limits independently bound render vertices, indices, primitives,
instances, bindings, effective instanced triangles, and the conservative
Geometer-owned render-artifact byte estimate. A face must currently belong to
exactly one body to be selectable;
ambiguous non-manifold ownership fails the render rather than choosing a body
silently. Failures and cancellation clear the output artifact.

The current table is deliberately internal and verbose. It duplicates the
three opaque handles for every occurrence/face span so its size can be measured
before choosing JSON, pagination, or a compact binary attachment. No node name,
Three.js object id, STEP entity number, XCAF label, or traversal index is used
as target identity.

## Corpus Measurement

The checked fixture report
`docs/reports/step-topology-fixture-baseline.json` now records mesh, instance,
vertex, index, primitive, binding, geometry-triangle, instanced-triangle, and
logical binding-table byte counts under fixed tessellation settings. The
largest current logical table is the repeated SOIC assembly: 312 spans and
73,632 bytes. Its 636 unique geometry triangles become 2,992 instanced
triangles, demonstrating why definition geometry and occurrence context must
remain separate.

These measurements favor a compact indexed table over repeated JSON handle
strings, but the encoding decision remains deferred until the GLB alternatives
and real Three.js raycast are tested.

## Validation

The focused native test proves shared definition geometry, distinct repeated
occurrences, exhaustive reverse resolution for every triangle, target-kind
resolution, indexed geometry, accumulated and root transforms, orientation,
normals, reflected coordinates, limits, cancellation, invalid coordinates,
out-of-range hits, artifact tampering/re-stamping, and stale/cross-session
rejection:

```powershell
cmake --build build --config Release --target `
  geometer_step_topology_render_binding_test
ctest --test-dir build -C Release --output-on-failure -R `
  geometer_step_topology_render_binding_test
```

This renderer-neutral slice does not itself emit or parse GLB and does not
define the later TypeSpec operation. The follow-on
[GLB work-packet research](step-topology-glb-binding.md) selects an encoding and
proves it with the real pinned Three.js `Raycaster` path.

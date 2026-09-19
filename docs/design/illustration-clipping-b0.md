# Illustration Half-Space Clipping B0

Geometer B0 illustration operations accept an ordered set of generic 3D
half-spaces. A point is retained when

```text
dot(normal, point) - distance_mm >= -tolerance_mm
```

Clipping runs after STEP root placement, authored analytic lowering, per-mesh
placement, and the operation's affine transform. It runs before bounds, Fast
HLR, outlines, visibility ordering, shading, and 2D projection. The same native
kernel is used by model illustration, composed-mesh illustration, and composed-
mesh HLR.

The B0 operation identities are:

- `geometry.model_illustration.b0`
- `geometry.model_illustration_geometry.b0`
- `geometry.mesh_illustration.b0`
- `geometry.mesh_illustration_geometry.b0`
- `geometry.mesh_hlr_projection.b0`

The executable IPC framing, generic C ABI, Worker protocol, and operation
catalog remain A0. The operation identity selects the exact A0 or B0 logical
request/outcome root. A0 illustration operations remain strict compatibility
adapters without clipping fields; maintained clients and examples use B0.
The public C++ value API likewise overloads `illustrate_mesh` and
`illustrate_mesh_geometry` for the B0 input/result roots. The A0 overloads are
retained unchanged as explicit compatibility entry points.

## Deterministic fragment contract

Planes are normalized and evaluated in authored order. New boundary vertices
and normals are interpolated, source material indices are preserved, duplicate
and degenerate output is removed deterministically, and unused geometry is
compacted. `cap_policy` is currently required to be `none`: cut edges are
ordinary mesh boundaries for HLR/outlines, and no synthetic section surface or
material is invented.

Each B0 result reports normalized clipping, input/output triangle counts, the
raw attachment digest when applicable, a complete fragment digest, and a
geometry-only digest used to prove that supplied HLR and shaded geometry came
from the same transformed and clipped fragment. Plane definitions, tolerance,
cap policy, transforms, and effective work limits therefore participate in
result/cache identity.

An entirely removed fragment is a successful result with `empty: true`, zero
output triangles, and absent bounds. Implementations must not fabricate
six-zero bounds. SVG operations return a valid empty illustration document;
geometry and HLR operations return empty collections for the requested view.

Clipping is bounded by 16 planes and explicit ceilings for output triangles,
generated vertices, intersections, and edge-plane tests. Non-finite values,
zero normals, unsupported cap policies, malformed geometry, and exceeded work
limits fail closed with governed diagnostics.

This facility is intentionally unaware of PCBs or board footprints. A
condition such as “outside a 2D footprint or beyond a viewed surface” is not a
half-space and requires a separate profile/occluder design.

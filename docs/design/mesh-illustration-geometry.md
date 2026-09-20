# Illustration drawing geometry

`geometry.mesh_illustration_geometry.b0` returns colored, ordered 2D drawing
geometry for applications that supply their own renderer. It shares native
preparation, shading, visibility ordering, fusion and optional HLR composition
with [SVG illustration](mesh-illustration-native.md), stopping before the SVG
writer. It returns neither original 3D tessellation nor XML/path strings.
The A0 geometry and SVG operations remain explicit compatibility surfaces.

## Contracts and calls

TypeSpec owns the reusable [A0 values](../../src/tsp/geometer/operations/mesh-illustration-geometry-a0.tsp)
and the canonical [B0 operation](../../src/tsp/geometer/operations/mesh-illustration-geometry-b0.tsp).
The generated DTOs, codecs, schemas and catalogs define their structure.

| Surface | Entry point |
| --- | --- |
| C++ owning value | `illustrate_mesh_geometry(input, result, status)`; overload accepts matching HLR |
| Native IPC / generic C ABI / full WASM | `geometry.mesh_illustration_geometry.b0` |
| Python persistent / one-shot | `client.mesh_illustration_geometry(input, hlr_projection=...)` / `geometer.mesh_illustration_geometry(...)` |
| Rust persistent | `mesh_illustration_geometry(input)` / `mesh_illustration_geometry_with_hlr(input, hlr)` |
| TypeScript local renderer | `illustrateMeshGeometry(input, linework)` / `createIllustrator(input).renderGeometry(style)` |

The compatibility direct input `geometry.mesh_illustration_geometry.input.a0`
and its B0 successor explicitly require
millimeter mesh positions and matrix translations. Convert other units first.
It has no SVG title/viewport options. The TypeScript prepared API's geometry
method likewise requires millimeter source meshes. Existing SVG/Canvas
conventions remain unchanged.

IPC reuses the `mesh_collection` JSON input attachment and optional
`hlr_projection`. HLR must be visible-only millimeter polylines from the same
model, placement and normalized view. Neither illustration operation computes
HLR or determines visibility for arbitrary supplied lines.

The small IPC result contains statistics, warnings and a descriptor with name,
schema, byte length and SHA-256. One required `illustration_geometry` attachment
contains UTF-8 `geometry.mesh_illustration.geometry.b0` JSON, media type
`application/vnd.wavenumber.geometer.illustration-geometry+json`, maximum 256 MiB.
This uses existing attachments, outside the 32 MiB inline envelope; platform
aggregate budgets still apply. Typed Python/Rust helpers validate integrity and
draw counts, then return the owning geometry DTO. Generic execute returns the
descriptor and bytes.

## Coordinates and painting

The view's orthographic basis gives `dot(world,right), dot(world,up)`, with
horizontal mirroring already applied. Y points upward; origin is world origin.
The result retains the requested view; do not mirror its points again. Geometry
retains floating-point precision rather than adopting the SVG integer grid.
Separate native/JavaScript floating-point evaluation and JSON decoding may differ
in their last binary digits; geometry output does not round them to force equality.

Viewport bounds include valid triangles before backface culling, exclude
supplied HLR and become `[-1,-1]..[1,1]` when all triangles skip. They are not a
tight visible-paint bound or clipping mask.

Draw surfaces in array order, and each surface's layers in array order. A
layer's rings form one implicitly closed even-odd fill, including holes.
Preserve underpaint/inlays and triangle/fused/layered kind. Then draw the ordered
lines: raw diagnostic strokes, HLR detail, HLR outline. Do not re-sort by an
inferred average Z or source material. Widths are millimeters, with round caps
and joins. Colors are sanitized CSS strings; color alpha multiplies layer
opacity. No new CSS-to-RGBA parser is involved.

Fusion does not retain comprehensive source-body provenance; this result does
not promise per-component picking IDs. `surface_draws` counts layers and
`commands` counts layers plus line segments, not SVG elements after chaining.
Warnings retain source order and the existing 256-entry cap.

## Presentation and limits

Presentation includes background/transparency, fill/stroke semantics, padding
and same-fill seam width. Consumers choose their viewport. Existing adapters
have distinct rules:

- SVG uses 6% padding, a default coordinate span of 1,000,000, Y inversion and
  ECMAScript rounding. Stroke/seam widths have a one-grid-unit minimum. Opacity
  at or above `.999` is opaque. Contiguous same-style lines are chained.
- Canvas retains layer opacity, uses a `.6`-pixel minimum line width and a
  `.7`-to-`1.5`-pixel seam stroke. SVG and Canvas are not pixel-identical.

SVG line grouping still compares original color spelling before sanitization;
the geometry DTO exposes safe paint values. Geometry consumers can reproduce
appearance without reproducing XML class names or exact path chaining.

Existing triangle, visibility-work and supplied-HLR limits remain. Geometry
adds aggregate limits of 2,000,000 surfaces/layers/rings, 6,000,000 ring points
and 1,000,000 lines, plus its attachment byte cap. Coordinates must be finite,
rings have at least three points and widths are nonnegative. These are accepted
work/output bounds, not a hard peak-memory sandbox.

Native failures clear the result; resource failures return status 102. IPC
remains reusable after recoverable operation errors. Typed clients terminate
their connection on malformed results. TypeScript retains its existing CPU/work
policies; shared DTO limits do not imply identical native/JS peak-memory bounds.
Planar-only WASM is unsupported: that target has no generic operation ABI.

The [Python-to-Canvas example](../../examples/python/illustration_geometry_canvas.py)
requests this operation and writes a standalone HTML preview and geometry JSON.
Its Canvas consumer uses the same painting conventions as the Illustration Lab.

The integer-formatting fast path accelerates shared native preparation/fusion
for both outputs. It does not accelerate unrelated STEP/HLR/GLB operations or
the separate TypeScript SVG/Canvas renderer.

# Technical illustration algorithms

Geometer technical illustration combines three maintained stages: Fast vector
linework, projected triangle visibility and shading, and drawing-surface
fusion. The combined model-illustration operation orchestrates them; the
independent HLR and mesh-illustration APIs remain available for consumers that
already own geometry or need separate intermediate products.

## Fast vector detail

Fast detail starts from indexed triangles. It welds vertices within the
configured tolerance, builds edge incidence, and classifies boundaries,
creases, silhouettes, and optional coplanar seams. View projection maps the 3D
segments into one orthographic 2D basis. A bounded spatial candidate pass tests
projected triangle coverage and depth, splitting or rejecting hidden fragments.
The algorithm emits polylines and source-face identities; it does not invoke
OCCT HLR.

The name `fast` distinguishes this triangle-based route from the older OCCT
exact and polygonal projection engines. Finished model illustrations always use
Fast detail and reject hidden output. The independent HLR APIs retain all older
algorithm choices.

## Fast Mesh Shadow

Fast Mesh Shadow derives the visible projected boundary from the same indexed
surface mesh. Candidate edges are classified from triangle incidence and
front/back orientation, then tested against projected occluders. Coplanar seam
suppression and tolerances come from the nested Fast options. This produces the
strong outside contour used over technical illustration fills.

Fast Mesh Shadow operates on source geometry and never unions CAD solids.
Separate component bodies therefore keep their own visible contours and
material shading while normal depth visibility resolves overlaps.

## Illustration preparation and visibility

The illustration renderer validates mesh positions, indices, normals,
materials, and transforms, then projects triangles into the requested view.
Normals use inverse-transpose transformation and reflections reverse winding.
Back-facing triangles are removed unless double-sided rendering is selected.

Projected bounding boxes drive a bounded broad phase. Significant overlaps
produce depth-order constraints; stable strongly connected components resolve
cycles without depending on hash iteration order. This painter order governs
all emitted fills. Supplied Fast linework is already visibility-filtered and is
composed over fills, with detail before outline.

Toon, banded, Lambert, flat, and unlit styles share the same scene. Source sRGB
materials, opacity, ambient/key lighting, light direction, rim amount, and
fallback color determine fill colors. The raw geometry and SVG operations use
the same style resolution.

## Same-color surface fusion

After shading and ordering, compatible adjacent triangles with the same final
fill and opacity can be fused into polygon rings. Fusion uses projected planar
topology and preserves holes, material layers, painter-order constraints, and
visible boundaries. It reduces drawing commands and SVG size after source mesh
preparation; it does not reduce model attachment size or create a portable mesh
cache.

Fusion is deterministic for identical validated input, view, and options.
Numeric SVG formatting uses locale-independent integer-grid and compact decimal
paths. Raw geometry retains millimeter coordinates and painter order so a
custom renderer can reproduce the same drawing.

## Limits and diagnostics

Every public input has structural count bounds. Runtime budgets additionally
cap triangles, Fast visibility candidate pairs, illustration overlap/fusion
comparisons, drawing commands, output attachment bytes, analytic sampling, and
analytic topology candidate pairs.
Budget exhaustion returns a stable resource-limit failure; it does not silently
switch algorithms or return a partial illustration. Partial STEP tessellation
is separate: Geometer cleans and retries outdated OCCT triangulations once, then
recoverable source faces may be omitted with warnings before the remaining
complete illustration is built.

Timings distinguish source preparation, Fast linework, illustration work, and
raw attachment encoding. These fields locate consumer-visible cost without
claiming a stable subdivision of private importer or renderer stages.

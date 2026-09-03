# WASM Examples

## Shared demo system

`geometer_demo.css` is the visual-system authority for new and migrated browser
demos. It derives the Wavenumber light palette, flat 0 px geometry, and
watermark treatment from `docs/design/styles.css`, and uses the locally
vendored OFL-1.1 JetBrains Mono 2.304 webfonts. Demo-specific layout selectors
belong in this single stylesheet under a demo root; HTML must not contain
visual values or inline style declarations. The analytic polygon-pour and
model-bounds pages are migrated consumers; HLR and planar-ring still require
selector/markup migration before this styling requirement is complete.

All portable demos use the same standalone packaging entry point:

```powershell
python scripts\build_standalone_demos.py analytic-polygon-pour
python scripts\build_standalone_demos.py all
```

The compiler bundles module graphs to classic IIFEs, embeds binary assets,
constructs Workers from Blob URLs, and produces directly openable HTML under
`dist/wasm/demos/`. Hosted directory builds remain separate because their CSP
and cache behavior differ from direct `file://` artifacts.

## Editor-tooling foundation

`demo-tooling/` contains runtime-dependency-free strict TypeScript building
blocks for future interactive examples: camera/grid math, normalized input and
intent resolution, pointer-capturing tools, scoped commands, named animation,
and transactional undo/redo. It is intentionally generic and is not yet wired
into the analytic demo. Its README records the clean-room provenance boundary.

## Live Analytic Polygon Pour

`analytic_polygon_pour_demo.html` is the first high-level TypeScript client for
the production-dispatched filtered solver behind the frozen A0 candidate. It
continuously solves a board-like copper region with circular clearances and a
draggable full-height keepout through a dedicated Worker. The canvas renders
the solver's canonical line and circular-arc fragments; JavaScript performs no
Boolean geometry.

The interaction uses one active solve plus one replaceable pending request, so
fast slider movement cannot accumulate stale work. The inspector reports the
Worker round-trip, result closure counts, surviving subtraction boundaries,
and the governed standalone-result digest.

`http://127.0.0.1:8123/examples/wasm/analytic_polygon_pour_demo.html`

Build the deterministic deploy-unchanged site after refreshing WASM and the
TypeScript examples:

```powershell
python scripts\build_analytic_polygon_pour_site.py
```

Serve `dist/wasm/demos/analytic-polygon-pour/` as a static directory or deploy
it unchanged to Cloudflare Pages. The directory contains its Worker, WASM,
generated client package, redistribution-safe JetBrains Mono fonts, `_headers` policy,
and a SHA-256 closure manifest; it has no service backend or network runtime
dependency.

Build the literal one-file version and open it directly from disk:

```powershell
python scripts\build_standalone_demos.py analytic-polygon-pour
```

`dist/wasm/demos/analytic_polygon_pour_demo.html` contains its stylesheet,
JetBrains Mono fonts, Wavenumber watermark, TypeScript client, Worker host,
Emscripten glue, and WASM bytes. It performs no runtime file or network loads.

## Interactive PCB Polygon Pour

`pcb_polygon_pour_demo.html` is an application-shaped example layered on the
generic analytic operation. It uses the generated TypeScript Worker client and
the clean-room `demo-tooling/` camera, input, command, tool-controller, and
history modules. Board semantics remain outside Geometer core.

The board-minus-pad job is composed with independent exact 0.55 mm capsule
clearance jobs for 0.15 mm traces under a 0.20 mm clearance rule, same-net via
clearance jobs, and exact via/spoke copper overlay jobs. The browser applies
the layers in deterministic subtract-then-add order and reports a SHA-256 over
the ordered job-ID/digest pairs; it does not claim one fused result topology.
Via placement/movement, deterministic
45-degree routing, polygon-vertex movement, pan/zoom, undo, and redo are
interactive. Chamfer, fillet, and shove routing are explicitly deferred.

Build the hosted offline directory and directly openable single HTML after
refreshing the TypeScript and browser-WASM artifacts:

```powershell
python scripts\build_pcb_polygon_pour_site.py
python scripts\build_standalone_demos.py pcb-polygon-pour
```

Outputs:

- `dist/wasm/demos/pcb-polygon-pour/index.html`
- `dist/wasm/demos/pcb_polygon_pour_demo.html`

## Generated Model Bounds Client

`model_bounds_demo.html` is the first TypeScript/generated-contract example.
It loads a STEP fixture through `@wavenumber/geometer/wasm`, computes bounds
through the generic ABI, and renders the authoritative result as a translucent
volume, wireframe, dimension lines, and an inspector around an interactive
Three.js model. The prepared GLB for the same fixture is a display companion;
the box and every reported number come from the STEP/OCCT result. The
TypeScript source contains no pointer or Emscripten heap management.

The page uses the shared light Wavenumber demo stylesheet, locally vendored
JetBrains Mono font, flat component geometry, and common watermark. Its Three.js
scene colors resolve from that stylesheet instead of maintaining another theme.
The hosted source page still pins the same Three.js 0.161.0 browser import used
by the existing embedded-model viewer, so serving that source page requires
network access to the pinned Three.js CDN module.

The maintained page runs the synchronous OCCT operation in a dedicated Worker.
`model_bounds_demo.ts` owns the Three.js UI and uses
`@wavenumber/geometer/worker`; `model_bounds_worker.ts` loads the classic
Emscripten factory and installs `@wavenumber/geometer/worker-host`. Both
JavaScript files are generated by `npm run generate:contracts`.

Build the package and example, serve the repository root, then open it:

```powershell
npm run generate:contracts
python -m http.server 8123 --bind 127.0.0.1
```

`http://127.0.0.1:8123/examples/wasm/model_bounds_demo.html`

HLR and Illustration Labs use the governed HLR operation and production
TypeScript package modules. Planar examples retain their existing surfaces.

## STEP Illustration Lab

**Status: production-package demo.** This page consumes the supported
`@wavenumber/geometer/mesh-illustration` and raster-HLR modules plus the
governed model HLR operation. The package owns mesh preparation, visibility
ordering, fusion, colorization, SVG/Canvas rendering, caching, and disposal;
the page owns only UI, camera, file, download, and presentation behavior.

`illustration_demo.html` and `illustration_demo.ts` form the reviewable consumer
of the production mesh-illustration package.
The page keeps the HLR Lab's side-by-side 3D/2D workflow, but the right pane renders
colorized triangle surfaces to either SVG or Canvas2D. It supports flat,
unquantized Lambert diffuse, quantized-band (up to 32 bands), and early toon shading,
live style changes, named or trackball
camera views, local STEP upload, and SVG/style downloads.

`@wavenumber/geometer/raster-hlr` adds a separate live GPU renderer to the same page. It builds
boundary/crease candidates once, uses the filled mesh as a depth occluder, and
depth-tests the retained edge fragments during camera motion. It deliberately
does not run OCCT HLR per frame. See
[`docs/design/fast-hlr-research.md`](../../docs/design/fast-hlr-research.md) for
the measured baseline and known quality gaps.

The Geometry section exposes Draft, Balanced, Fine, and Extra fine STEP mesh
presets plus custom linear deflection (millimetres), angular deflection
(degrees) and separately grouped HLR relative chord/angular tolerances. The demo
does not impose a triangle-count cap. Surface tolerance changes reconvert STEP
through Geometer WASM; HLR tolerances only reproject linework, and GLB-only
inputs retain their authored tessellation.
Linear deflection is the maximum positional gap between an exact STEP surface
and its triangle approximation; angular deflection limits directional change
while following curvature. Smaller values increase curved-surface density.
Remeshing the current model preserves the active orthographic camera pose,
target, zoom, and framing scale.

The Surface panel can fuse safe adjacent opaque triangles that resolve to the
same rendered color. SVG and Canvas use the same cached surface commands, and
the UI reports front-facing-triangle, resulting polygon-draw, and SVG byte
counts. Fusion runs after visibility ordering and may cross unrelated paint
commands only when the whole polygon can move without crossing an overlapping
different-style surface. Edge identity includes depth; transparent surfaces,
ambiguous projected folds or overlaps, non-manifold edges, and invalid boundary
loops fall back to individual ordered triangles. This is a conservative
output-size optimization rather than a change to the prepared mesh scene.

When `fuseSurfaces` and `layerCoplanarMaterials` are both enabled, the shared
renderer also recognizes opaque material partitions connected by complete
edges on the same geometric plane. It unions the partition into a continuous
footprint, underpaints that footprint with its largest-area rendered style,
then overpaints the other fused material regions. The whole layered group must
fit one common visibility-order interval; transparent, overlapping,
non-manifold, non-coplanar, or ambiguously ordered groups retain the ordinary
surface path. This generic treatment covers zero-height package markings,
logos, and similar mesh-authored inlays without introducing PCB-specific
policy. It does not reconstruct analytic STEP curves; marking contours still
reflect the selected surface tessellation.

SVG serialization uses shared style classes, chained compound HLR paths,
collinear boundary removal, and a normalized integer coordinate grid. The
default grid has 1,000,000 units across the larger unpadded artwork axis;
callers can override it through `MeshIllustrationSvgOptions.coordinateSpan`.
Canvas retains the original projected coordinates. Prepared scene data remains
an opaque package implementation detail and is not exported as a JSON contract.

The current polygon contraction does not yet clip away fully occluded triangle
fragments. A future maximum-reduction mode can use Geometer's Clipper2 byte
bridge to subtract accumulated front coverage and union the remaining opaque
regions by style. Transparent, gradient, and Gouraud surfaces must retain an
ordered mesh-capable fallback rather than using that opaque flattening path.

The illustration algorithm consumes generic indexed or non-indexed triangle
meshes with transforms, material colors, and vertex normals. STEP is only the
first adapter: the demo Worker uses the existing STEP-to-GLB compatibility
surface, then the same Three.js-to-generic-mesh adapter handles bundled and
uploaded models. A glTF/GLB loader or Viz-generated PCB mesh can feed the same
production package without STEP or OCCT.

Illustration A0 is a package contract rather than a native illustration
operation. STEP-backed models can enable Geometer's `fast-mesh-shadow` Outline
layer, which reconstructs CAD-face boundary loops when possible and falls back
to per-face triangle unions before combining the reduced contours into the clean
outer body trace, and its fast visible Detail
preset. Independent checkboxes composite those layers over the shared
SVG/Canvas surface scene and include them in SVG downloads. Generic mesh inputs
remain surface-only unless an adapter supplies comparable linework; the noisy
triangle-adjacency silhouette experiment is not exposed in the main UI.
Projected triangle overlaps are spatially indexed and depth-compared over their
actual intersection before vector paint order is chosen; this avoids the
body/lead and dense-assembly failures caused by average-depth sorting. Triangle
visibility order is cached per prepared scene and back-face mode so style-only
redraws do not rebuild the overlap graph. Triangle paint overlap suppresses
SVG/Canvas antialias seams. Coplanar material layering additionally ensures
that shallow marking boundaries are overpainted with the marking color rather
than a later-painted base color. The hidden, experimental adjacency-derived outline
and crease flags remain overlay linework rather than occlusion-clipped strokes;
STEP-backed lab output uses the visibility-resolved HLR outline instead.

The STEP adapter requests `strip_root_placement` so HLR and STEP-to-GLB use the
same definition-local frame, keeps the projection result in its documented
millimetres, and converts outline coordinates to glTF metres only when attaching
them to the illustration scene.

Build and review the offline site:

```powershell
node scripts\build-typescript-examples.mjs
python scripts\build_illustration_site.py
python -m http.server 8123 --bind 127.0.0.1 --directory dist\wasm\demos\illustration
```

Open `http://127.0.0.1:8123/`. The directly openable single-file artifact is
`dist/wasm/demos/illustration_demo.html`. Building does not publish either
artifact.

## Embedded Model Viewer

`embedded_model_viewer.html` and `embedded_model_viewer.js` are the browser
reference shell and application module for the high-level
Geometer workflow. Together they:

- load a prepared GLB model into an interactive Three.js 3D pane;
- run STEP HLR through the browser WASM worker;
- draw detail, outline, and optional bounding-box projection geometry in the adjacent SVG pane;
- expose view, algorithm, outline, mesh, color, and edge-category controls.

Run it from the repository root:

```powershell
python -m http.server 8123 --bind 127.0.0.1
```

`http://127.0.0.1:8123/examples/wasm/embedded_model_viewer.html`

Build or refresh the WASM outputs first when needed:

```powershell
python scripts\build_wasm.py
```

Release/demo review uses a standalone one-file copy:

```powershell
python scripts\build_self_contained_hlr_demo.py
```

Open `dist/wasm/demos/hlr_demo.html` directly from disk. The generated file
embeds the Geometer browser WASM, the demo model assets, the Worker body,
Three.js viewer runtime, and the watermark.

The page uses the embedded model fixtures under `tests/fixtures/`. It also accepts
local `.step` and `.stp` uploads. Uploaded bytes stay in the browser: the Worker
uses Geometer WASM to produce both the 3D GLB preview and the HLR projection.
`Export SVG` downloads the active view, display mode, bounding-box choice, and
colors as a standalone, model-unit-sized SVG. The 3D pane uses Three.js
TrackballControls: left-drag rotates freely, the wheel zooms, and right-drag pans.
Its lens defaults to orthographic so it matches Geometer's orthographic HLR;
the `3D lens` selector can switch to perspective for depth inspection.

The projection UI follows the terminology in ADR 008. `Detail` is raw linework
from the selected OCCT polygonal/exact engine or the Fast triangle HLR engine.
The `Fast detail HLR` checkbox provides the direct comparison switch. Its
candidate-category, degree-based crease-angle, weld, projection, and depth
controls live in a separate Fast-only panel; the OCCT engine, edge angle,
presets, and edge categories live in an OCCT-only panel.
`Outline` is Geometer's independent assembly silhouette, and `Both` displays
those layers without changing or merging either layer's color, width, or line
style. Mesh shadow remains the recommended silhouette source, with the
fast-mesh-shadow evaluation backend available for direct comparison. Raw OCCT
categories use `Detail edge set` rather than the overloaded term `Profile`.
The independently selectable outline source does not change when the Fast
detail checkbox changes.
Tessellation is either model-relative (controlled by `Quality coef`)
or absolute (controlled by `Linear tol (mm)`), so the page no longer presents
inactive linear/relative controls at the same time.
Line widths are capped at 5 px, and dash/dot lengths and gaps scale with each
layer's width so thick patterned lines remain legible in the page and SVG export.
Secondary display and geometry controls live in the resizable `Settings` dock,
which uses the dependency-free TypeScript panel system in `demo-tooling`. The
dock can be collapsed or hidden from its activity-rail tab. `Reset geometry
defaults` restores the polygonal engine, mesh-shadow silhouette, model-relative
tessellation coefficient, linear/angular tolerances, edge-angle tolerance, and
the Sharp + silhouettes Detail edge set before recomputing the current view.

`Top axis` and `Front axis` explicitly define the preset frame in model
coordinates; they must be perpendicular. Right/Left and the ISO presets are
derived as a right-handed frame. The default is `+Y` Top and `+Z` Front. Camera
fitting preserves the active named preset when the model changes; only the live
`Cam` view uses the fit camera's isometric direction. Query parameters
`topAxis` and `frontAxis` accept `+x`, `-x`,
`+y`, `-y`, `+z`, or `-z`.

The `3D` dock exposes material, shading, sidedness, wireframe, background, tone
mapping, exposure, and light controls. `Viz Lambert` is the default: it preserves
the imported colors/maps while converting the OCCT-generated glTF PBR materials
to matte `MeshLambertMaterial`. `Source glTF PBR` restores the original materials,
and `Unlit Basic` uses `MeshBasicMaterial`. The default Viz-style light rig is
ambient 0.2, fixed directional 0.5, and camera-following directional 0.75 with
explicit sRGB output. There is currently no environment map or shadow pass.

Build the deploy-unchanged hosted directory:

```powershell
python scripts\build_hlr_site.py
```

Serve `dist/wasm/demos/hlr/` locally for review or deploy it unchanged to
Cloudflare Pages after signoff. Its only runtime file is `index.html`: the
browser Worker, Geometer WASM, Three.js runtime and license, logo, styles, and
example models are all embedded in that one page. `_headers` and
`asset-manifest.json` are deployment/verification metadata, not runtime assets.
The reusable `scripts/package_single_html_site.py` tool supplies the CSP,
single-runtime-file manifest, deterministic staging, and Cloudflare headers for
this and future self-contained demos.

See [Browser demo packaging and UI](../../docs/design/browser-demos.md) for the
durable source/build/test layout and the checklist for adding another hosted
demo. Building these artifacts never publishes them.

Refresh those fixtures with:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\prepare_embedded_model_fixtures.ps1
```

Test-only browser pages remain under `tests/wasm/`, including
`hlr_benchmark.html` and `browser_hlr_validation.html`.

## Planar Ring Solver Demo

`planar_ring_solver_demo.html` demonstrates the planar batch solver JSON ring
output. It shows draggable subject/subtract rings, curved source shapes,
solver JSON, and the resulting SVG rings.

`http://127.0.0.1:8123/examples/wasm/planar_ring_solver_demo.html`

After building `dist/wasm/planar-browser`, bake a standalone demo with:

```powershell
python scripts\build_self_contained_planar_ring_solver_demo.py
```

Open `dist/wasm/demos/planar_ring_solver_demo.html` directly from disk. The
generated file embeds the planar WASM runtime and watermark.

+++
type = "plan"
id = "step-illustration-demo"
status = "active"
created = "2026-09-01"

[[steps]]
id = "synchronize-baseline"
title = "Start implementation from a clean checkout of the latest origin/main and re-audit the HLR Lab and topology render surfaces"
status = "done"

[[steps]]
id = "vector-visibility-spike"
title = "Prove the projected-triangle visibility and ordering strategy on representative STEP fixtures"
status = "active"
depends_on = ["synchronize-baseline"]

[[steps]]
id = "contract-and-adr"
title = "Specify the A0 illustration scene, operation request, renderer capabilities, and compatibility policy"
status = "pending"
depends_on = ["vector-visibility-spike"]

[[steps]]
id = "native-illustration-core"
title = "Implement the focused C++ illustration projection and scene-building value APIs"
status = "pending"
depends_on = ["contract-and-adr"]

[[steps]]
id = "native-svg-and-cli"
title = "Implement deterministic SVG serialization and native CLI JSON/SVG output"
status = "pending"
depends_on = ["native-illustration-core"]

[[steps]]
id = "cross-language-projections"
title = "Generate candidate C++, TypeScript, Rust, and Python projections and governed vectors"
status = "pending"
depends_on = ["contract-and-adr"]

[[steps]]
id = "operation-transports"
title = "Implement the candidate operation through generic C ABI, executable IPC, browser WASM, and Worker transports"
status = "pending"
depends_on = ["native-illustration-core", "cross-language-projections"]

[[steps]]
id = "browser-renderers"
title = "Implement SVG-DOM and Canvas2D consumers of the same illustration scene contract"
status = "pending"
depends_on = ["native-svg-and-cli", "operation-transports"]

[[steps]]
id = "illustration-lab"
title = "Build the HLR-Lab-style STEP Illustration Lab with preview, controls, caching, and downloads"
status = "pending"
depends_on = ["browser-renderers"]

[[steps]]
id = "package-and-validate"
title = "Package self-contained and hosted demos and add native, contract, static-site, and real-browser validation"
status = "pending"
depends_on = ["illustration-lab"]

[[steps]]
id = "promote-durable-docs"
title = "Promote settled behavior into ADRs, requirements, design docs, CLI docs, and generated contract references"
status = "pending"
depends_on = ["package-and-validate"]

[[steps]]
id = "design-doc-intent-audit"
title = "Audit design docs, ADRs, and requirements against implementation"
status = "pending"
depends_on = ["promote-durable-docs"]

[[steps]]
id = "test-runtime-impact-audit"
title = "Audit new test runtime impact and Rack stratum placement"
status = "pending"
depends_on = ["package-and-validate"]

[[steps]]
id = "external-review"
title = "Obtain independent implementation and promotion-evidence review"
status = "pending"
depends_on = ["design-doc-intent-audit", "test-runtime-impact-audit"]

[[steps]]
id = "promote-operation"
title = "Promote the operation only after every ADR-010 projection, transport, documentation, compatibility, and review gate passes"
status = "pending"
depends_on = ["external-review"]

[[exit_criteria]]
id = "native-output"
title = "The native CLI emits deterministic illustration JSON and SVG for arbitrary orthographic views"
status = "pending"

[[exit_criteria]]
id = "shared-contract"
title = "Native SVG, browser SVG, and Canvas use one versioned illustration scene contract"
status = "pending"

[[exit_criteria]]
id = "browser-demo"
title = "The Illustration Lab loads local STEP, renders off-main-thread, switches SVG/Canvas, and downloads SVG"
status = "pending"

[[exit_criteria]]
id = "portable-packaging"
title = "Self-contained and hosted demo artifacts pass offline/static-closure and real-browser validation"
status = "pending"

[[exit_criteria]]
id = "future-effects"
title = "The A0 contract preserves the data and pass semantics needed for toon shading, Gouraud shading, shadows, and transparency"
status = "pending"

[[exit_criteria]]
id = "promotion"
title = "The promotion manifest records complete C++, TypeScript, Rust, Python, native IPC, and browser WASM evidence"
status = "pending"

[[exit_criteria]]
id = "design-doc-intent-audit"
title = "Design docs, ADRs, and requirements match implementation"
status = "pending"

[[exit_criteria]]
id = "test-runtime-impact-audit"
title = "New tests are listed and runtime impact is reviewed"
status = "pending"

[[exit_criteria]]
id = "external-review"
title = "Independent external review is complete"
status = "pending"

[[exit_criteria]]
id = "signoff"
title = "Focused native, WASM, package, and browser signoff passes"
status = "pending"
+++

# STEP Illustration SVG/Canvas Contract and Browser Demo

## Purpose

Stand up a Geometer-owned illustration pipeline and an HLR-Lab-style browser
demo that turns a STEP model and a chosen camera into a colorized, shaded 2D
rendering. The same retained illustration scene must support deterministic SVG,
interactive Canvas rendering, native command-line document generation, and the
browser WASM workflow.

The first useful result is an orthographic technical-art rendering with source
part colors, opaque flat or discretely banded lighting, visible outlines, and
optional HLR detail strokes. The contract must deliberately preserve enough
information to add cel/toon shading, smooth Gouraud-like shading, cast shadows,
and transparency later without replacing the operation or inventing a second
scene model.

This work lives completely in Geometer. The core illustration contract and
SVG/Canvas renderers do not depend on Viz, DwgScene, an Altium data model,
WebGL, or an external rendering service. The demo intentionally uses Three.js/
WebGL only for its left-hand interactive 3D preview.

The projection and shading algorithm must consume a generic retained triangle
mesh scene: positions, normals, indices, transforms, materials, and optional
stable source identities. STEP/XCAF is the first importer, not a hard dependency
of the illustration core. This keeps a future GLB/glTF adapter or a Viz-generated
PCB/component mesh scene on the same rendering path.

## Baseline and execution prerequisite

Planning inspected the latest fetched `origin/main` on 2026-09-01, including
the current HLR Lab, browser demo packaging, generated TypeScript Worker client,
generic operation C ABI, color-aware STEP import, and experimental topology
render artifact. At that point `origin/main` was
`88ac1b194c7ada60f3ec4f4b7026bd796fb99db4` and the working branch was seven
commits behind with unrelated generated `dist/` changes.

Implementation must begin in a clean branch or worktree synchronized to the
then-current `origin/main`. Refresh the remote and repeat the narrow audit before
editing; do not merge, overwrite, or normalize the existing dirty `dist/`
artifacts as part of this feature.

References to reuse rather than reinvent:

- `examples/wasm/embedded_model_viewer.html` and `.js` for the HLR Lab workflow
  and interaction model;
- `docs/design/browser-demos.md` and ADR 014 for demo ownership and packaging;
- `scripts/build_self_contained_hlr_demo.py` and `scripts/build_hlr_site.py` for
  portable artifact construction;
- `src/tsp/geometer/operations/`, every required generated-language projection,
  and the Worker client for a candidate browser operation;
- `src/cpp/lib/step_topology_render_binding.cpp` for reusable occurrence,
  transform, triangulation, normal, and instance/face-association concepts;
- the existing XCAF and STEP-to-GLB paths as references for focused source-color
  resolution and precedence tests;
- the existing projection/HLR value APIs for camera conventions and analytic
  outline/detail overlays.

The topology render artifact is a source of implementation ideas, not the
public illustration contract. Its current neutral GLB material policy and
experimental status must not leak into this API.

## Independent plan review

An independent plan review on 2026-09-01 checked this proposal against the
latest generic operation limits, ADR-010/REQ-008 promotion rules, Worker
semantics, HLR Lab preview path, topology rendering code, and browser packaging.
The review found and this revision corrects:

- an invalid assumption that a useful tessellated scene could live inside the
  generic operation's 8 MiB outcome JSON rather than an output attachment;
- premature operation promotion before all generated-language, IPC, WASM,
  compatibility, documentation, and review evidence exists;
- the missing 3D-preview data path for uploaded STEP files;
- optional normals/geometry that could not actually preserve Gouraud, shadow,
  or transparency follow-ons;
- mixed ownership of lighting/style between the OCCT operation and renderers;
- an unsupported claim of cooperative Worker cancellation; and
- inaccurate topology color reuse and ambiguous session-derived identities.

The `external-review` execution step remains pending because ADR-010 also
requires an independent review of the eventual implementation and promotion
evidence; this completed review covers the plan only.

A focused closure pass approved the corrected plan with no remaining high- or
medium-severity findings. Its low-risk reproducibility and browser-test notes
are incorporated below.

## Product outcome

The deliverable is a new **STEP Illustration Lab** with essentially the same
shape and portability as the HLR Lab:

- a left 3D source preview constructed from the returned retained geometry and
  a right 2D illustration pane;
- bundled fixtures plus drag/drop and file-picker loading of local STEP files;
- orthographic view presets and "use current preview camera";
- meshing, camera, material, lighting, background, outline, and detail controls;
- SVG and Canvas display modes backed by the same returned scene;
- download of SVG, illustration-scene JSON, and illustration-style JSON, with
  optional Canvas PNG as a UI-only convenience;
- processing in a Worker so large STEP files do not block the page;
- self-contained HTML and hosted-directory outputs under
  `dist/wasm/demos/`, with no runtime network dependency.

Changing lighting, shading bands, background, line style, or another
renderer-side presentation control should redraw the retained scene without
rerunning OCCT. Changing geometry, tessellation, camera, or visibility inputs
should issue a new operation and cache it by a deterministic request key.

## First reviewable feasibility checkpoint

Before freezing the native/TypeSpec contract, land a browser-only prototype
that makes the rendering and visibility choices reviewable:

- a strict TypeScript generic-mesh input and projected-scene model;
- STEP-to-GLB only as a temporary compatibility adapter into that mesh input;
- shared SVG and Canvas2D rendering with source colors, flat/banded/toon
  lighting, named/current-camera views, and scene/style/SVG downloads;
- offline single-HTML and hosted-directory packaging plus generic-mesh, static
  closure, and real-Chrome tests;
- explicit experimental labeling for mesh-derived silhouette/crease linework
  until the visibility spike settles HLR overlay and arbitrary-mesh policy.

This checkpoint does not promote an operation and does not replace the planned
retained native scene attachment. Its evidence informs the visibility gate and
A0 contract.

## Architecture

```text
STEP bytes or another model/mesh adapter + projection options
            |
            v
OCCT import / XCAF colors / tessellation / HLR
or generic retained triangle meshes
            |
            v
geometry.model_illustration.a0 operation
            |
            v
small typed result + required illustration-scene JSON attachment
            |
            v
unlit geometry/projection scene + illustration.style.a0
       |             |             |              |
       v             v             v              v
 native SVG      browser SVG    Canvas2D     Three.js preview
       |             |             |
 docs / CI       SVG download    interactive preview
```

Keep implementation responsibilities separate:

- a small native projection/scene builder owns model import, camera transforms,
  face/triangle preparation, normals, focused XCAF source-color resolution,
  depth/visibility metadata, and HLR overlays;
- a renderer-neutral value model owns an unlit retained A0 geometry/projection
  scene, including every source surface triangle rather than only visible
  vector fragments;
- a separate versioned illustration-style model owns lights, shading bands,
  background, line styling, and requested render effects;
- a native SVG writer owns serialization only;
- TypeScript SVG and Canvas renderers consume the contract and contain no OCCT
  or STEP logic;
- the demo owns presets, controls, cache policy, downloads, and presentation.

## A0 illustration scene contract

Settle the exact names in the ADR/TypeSpec step, but use
`geometry.model_illustration.a0` for the operation,
`geometry.illustration_scene.a0` for the attached scene, and
`geometry.illustration_style.a0` for renderer-owned appearance unless the
existing naming audit finds a conflict.

The generic operation outcome JSON has an 8 MiB hard limit, so it must not
inline the tessellated scene. Return a small typed result descriptor containing
the scene schema identity, media type, byte count, digest, counts, and generated
capabilities. Carry the strict illustration-scene JSON in one required output
attachment, tentatively named `illustration_scene`, with a governed vendor media
type. Define operation-specific byte/count limits below the generic ABI's 256
MiB individual and wasm32 aggregate attachment ceilings, and test both the
8 MiB outcome boundary and every scene-attachment limit. Packed binary remains
a measured follow-on, not an excuse to exceed A0 limits.

The attached scene should be array-oriented, deterministic, easy to validate,
and straightforward to adapt into a future drawing-scene model. It should
include:

- contract/version, source digest, units, projection mode, camera basis, model
  transform, projected bounds, view box, and numeric precision;
- generated geometry capabilities plus sufficient provenance for deterministic
  regeneration and debugging, without timing data in canonical output;
- an unlit retained geometry layer containing all tessellated source surfaces,
  including surfaces back-face-culled or hidden in the opaque projection;
- model-space or otherwise reconstructible 3D positions, camera-space/projected
  coordinates where needed, indexed triangles, and required per-corner or
  per-vertex normals with an explicit smoothing and hard-seam policy;
- material records with source/fallback linear RGBA, alpha mode
  (`opaque`, `mask`, or `blend`), alpha cutoff, double-sided policy, and explicit
  precedence for occurrence, face, material, and optional corner colors;
- occurrence transforms, face/triangle associations, and deterministic
  scene-local IDs. Never serialize topology session handles, seals, or other
  process/session-local identities as canonical illustration IDs;
- a derived projection layer containing stable visible-surface ordering or
  clipped fragments plus path drawables for silhouettes, creases, HLR
  visible/hidden detail, and analytic arcs;
- reserved derived-layer kinds for future vector shadows and optional hybrid
  image/tiles, neither emitted by the MVP.

The separate style contract owns ambient/directional lights, background,
flat/banded/toon parameters, line colors and widths, pass visibility, and
requested render effects. It defines ordered semantic passes:
`background`, `cast_shadow`, `opaque_surface`, `transparent_surface`, `detail`,
and `outline`. Each renderer reports requested versus effective capabilities so
unsupported effects fail clearly or produce an explicit governed fallback.

Do not make normals, full source geometry, or material/base-color bindings
optional in A0. They are required to relight a cached scene, construct the 3D
preview, and preserve a credible path to toon/Gouraud shading, cast shadows,
and transparency.

The contracts must state coordinate orientation, winding, color space, alpha
semantics, depth direction, tie-breaking, pass ordering, numeric limits,
unknown-field behavior, and schema evolution rules. Add positive and negative
vectors and deterministic JSON Schema, C++, TypeScript, Rust, and Python
projections/codecs exactly as required by ADR-010 and REQ-008.

## Initial rendering profile

The MVP profile is intentionally constrained:

- orthographic projection; retain an extensible projection discriminator but do
  not promise perspective SVG in A0;
- XCAF/STEP occurrence and face colors with a caller-selected fallback color;
- ambient plus one or more directional diffuse lights;
- opaque materials only as an effective capability;
- flat per-face or per-triangle shading and simple configurable quantized
  diffuse bands; the complete named toon profile remains a follow-on;
- back-face culling where valid, deterministic visible-surface ordering, and
  HLR-derived silhouettes/detail paths;
- SVG paths with solid fills and strokes; Canvas2D draws the same meshes and
  paths;
- native analytic arcs retained for HLR paths when requested.

The first SVG renderer must favor portability: no scripts, filters, remote
assets, or browser-specific extensions. Group paths by semantic pass/material
where this reduces size without changing ordering or identity.

## Visibility feasibility gate

Before the public contract is frozen, implement a narrow spike using generated
fixtures and a few representative colored assemblies. Compare a stable
back-face-cull plus depth-sort approach with any needed polygon splitting or
partial-order strategy. Test overlapping occurrences, concave solids, internal
features, near-coplanar faces, and mirrored transforms.

The gate must answer:

- whether deterministic triangle ordering is visually correct for the accepted
  STEP fixture class;
- which degeneracy tolerance and stable tie-break keys are required;
- whether the MVP can remain vector-only or needs a documented complexity or
  model-validity limitation;
- how outline/detail strokes avoid z-fighting, gaps, and double-dark seams;
- the expected JSON and SVG size per triangle and the threshold for rejecting
  or simplifying overly dense output.

If correct vector visibility requires a materially different scene shape,
revise the A0 proposal before promotion. Do not hide a raster depth buffer
inside output advertised as editable vector artwork.

## Native API and CLI

Add direct C++17 value APIs under `src/cpp/lib/` and keep modules focused:
illustration options, scene/value types, scene construction, visibility/order,
JSON codec, and SVG writer should not become one catch-all translation unit.

Proposed native file-oriented commands, subject to the CLI naming audit:

```powershell
geometer model-project-illustration input.step output.illustration.json --format step --view iso
geometer illustration-to-svg input.illustration.json output.svg --style style.json
geometer model-project-illustration-svg input.step output.svg --format step --view iso
```

The STEP-to-scene command must accept arbitrary direction/up vectors in addition
to named views, model transforms, tessellation controls, source/fallback color
policy, outline/detail geometry modes, rounding, and output bounds or padding.
Options common with HLR projection should reuse the same parsing and validation
semantics. Lighting, background, shading bands, and line appearance belong to
the style passed to `illustration-to-svg`; they do not change the retained scene
or its OCCT cache key.

The JSON command is the automation and cache artifact for documentation
generators. The scene-to-SVG command proves that cached output can be restyled
without STEP or OCCT. The STEP-to-SVG command is a convenience composition of
the same scene builder and SVG renderer and must not run a separate projection
algorithm. Add compatible batch-operation support only if it follows naturally
from the current generic operation path; do not extend the legacy batch
vocabulary as a substitute for operation promotion.

## WASM and TypeScript surface

This is a new operation, so do not copy the HLR Lab's legacy handwritten
low-level C ABI call pattern. Inventory and implement it as a candidate TypeSpec
operation using the generic operation transports, required scene output
attachment, modular browser WASM build, generated contract types, generated
Worker protocol/client, and documented request limits. Generate and validate
the C++, TypeScript, Rust, and Python projections and executable IPC surface.
Do not mark the operation promoted until the final ADR-010 manifest gate after
the maintained demo, durable docs, full transport evidence, compatibility
checks, and independent implementation review are complete.

STEP bytes remain inside the browser. The Worker owns the WASM module and
operation lifecycle. Transfer large buffers where supported, surface structured
errors, debounce rapid changes, suppress stale results, and optionally
terminate/recreate the Worker when the user explicitly aborts a long operation.
Do not claim cooperative cancellation inside synchronous OCCT work. Native,
executable IPC, and WASM results should be semantically identical after
canonicalization; if floating-point byte identity is not realistic, document
and test the allowed tolerance.

The retained geometry layer is also the 3D-preview data path. Build the
Three.js preview from its model-space positions, normals, indices, occurrence
transforms, and material/base-color bindings. This keeps local uploaded STEP
files within one Worker operation and avoids depending on the legacy
`geometer_step_to_glb_bytes` symbol or separately promoting model-to-GLB. Test
the additional scene size, WASM memory, and Worker transfer cost explicitly.

Keep renderers as reusable TypeScript modules outside the demo page:

- `renderIllustrationSvg(scene, style, options)` returns an SVG element/string
  without reparsing STEP;
- `renderIllustrationCanvas(context, scene, style, options)` draws into a
  supplied Canvas2D context;
- both consume the same versioned style DTO and shared shading test vectors;
- renderer options may change display scale and diagnostic overlays but may not
  reinterpret contract units, style values, or capability fallbacks silently.

## Illustration Lab behavior

Reuse the maintained demo theme, shared controls, fixtures, local-file safety,
and build helpers. Match the HLR Lab's familiar two-pane layout but create a
separate demo entry rather than adding another mode to the HLR monolith.

Minimum controls:

- fixture/local STEP selection and reset;
- top/front/right/isometric presets plus current-preview-camera capture;
- orthographic fit, padding, and tessellation quality;
- source colors on/off and fallback/base color;
- flat versus banded shading, band count, ambient level, light direction, and
  light intensity;
- outline visibility/color/width and visible/hidden detail controls;
- SVG versus Canvas output and fit/actual-size zoom;
- timing, triangle/path counts, effective capabilities, warnings, and errors;
- download SVG, illustration-scene JSON, and illustration-style JSON; include
  scene/style digests in a small render manifest or the SVG metadata so the
  current rendering can be reproduced. Canvas PNG may be offered but is not a
  contract artifact.

Do not expose nonfunctional Gouraud, soft-shadow, or transparency controls in
the MVP UI. A compact capabilities readout can show those as reserved/not yet
implemented.

## Future appearance profiles preserved by A0

These are compatibility requirements, not MVP implementation acceptance:

### Cel/toon profile

Reserve a named `toon` profile suitable for a Wind Waker-inspired look without
copying game assets: source-color tinting, quantized diffuse bands, optional
highlight/rim bands, low or disabled specular response, bold silhouettes, and
selectable crease/detail strokes. The scene's required normals/material colors,
the style's lighting values, and the path layers must make this a renderer
addition rather than a new OCCT export.

An early follow-on can implement the profile consistently in native SVG and
Canvas by quantizing lighting per face/triangle. A higher-quality version may
subdivide at band boundaries so broad curved faces do not expose triangulation
patterns.

### Smooth/Gouraud shading

SVG 2 does not provide a portable arbitrary Gouraud triangle primitive, and
Canvas2D has no native one. Preserve per-vertex normals/colors so a later
renderer can choose WebGL/WebGPU, software-rasterized tiles, or adaptive vector
subdivision. Any hybrid raster pass must be declared in capabilities and remain
optional; SVG output should provide a documented flat/banded fallback.

### Cast shadows

Keep `cast_shadow` separate from surface shading. Future requests should name a
receiver plane, light, bias, opacity, and hard/soft mode. Hard shadows may be
projected vector silhouettes; soft shadows may require bands, filters, or a
hybrid image pass. Contact/drop shadows are distinct styles and must not be
silently labeled physically cast shadows.

### Transparency

Preserve alpha modes and a separate transparent pass now. Future implementation
must define depth sorting, self-overlap behavior, and SVG/Canvas parity before
advertising `blend` as effective. The MVP validates transparent source colors
but resolves them through an explicit opaque fallback and warning.

## Caching, determinism, and performance

Define a canonical cache key from Geometer version/ABI, source digest, model
format, model transform, camera, tessellation, visibility, and source/fallback
color-resolution inputs. Lighting, shading, background, and line style are not
part of the OCCT operation key. Cache rendered SVG/Canvas products separately
by scene digest, style digest, renderer identity/version, and output options.

Require stable ordering and stable numeric formatting in canonical JSON/SVG.
Exclude elapsed time and browser state from canonical artifacts. Record metrics
for import, mesh/HLR, scene construction, serialization, transfer, and paint so
future work can target the correct layer.

Establish fixture-based budgets during the visibility spike for:

- operation time and peak WASM memory;
- triangle/path counts, small outcome JSON size, attached scene JSON size, and
  rendered SVG size;
- Worker transfer and parsing time;
- Canvas redraw time and SVG DOM node count.

Prefer a clear size/complexity diagnostic over freezing or silently degrading a
large model. A packed binary scene transport can be a later optimization after
the JSON A0 contract is stable and measured; do not make it an MVP prerequisite.

## Validation plan

### Native and contract

- C++ unit tests for camera basis validation, projection orientation, winding,
  mirrored transforms, normals, color precedence, lighting bands, depth ties,
  scene bounds, pass ordering, and deterministic serialization.
- Generated colored STEP fixtures covering face colors, occurrence colors,
  repeated instances, multiple solids, concavity, near-coplanar faces, and
  curved surfaces.
- Structural SVG tests for view box, solid fills, analytic arcs, outline/detail
  layers, escaping, finite numbers, deterministic output, and no external
  resources.
- CLI smoke/golden tests for named and arbitrary views and JSON/SVG parity.
- Positive/negative contract vectors, freshness checks, unknown-field and limit
  validation, and C++/TypeScript/Rust/Python semantic parity.
- Boundary tests proving the typed outcome remains below the generic 8 MiB JSON
  ceiling and scene attachments respect declared per-operation, 256 MiB
  individual, and wasm32 aggregate ceilings.
- Native executable IPC round trips and Rust/Python client coverage, including
  attachment digest/media-type validation and governed failures.

### WASM and renderers

- Native-executable-versus-WASM scene parity on the same fixture and options.
- TypeScript renderer tests proving SVG and Canvas consume the same scene and
  honor pass/style switches without another operation.
- Pixel probes or small deterministic browser screenshots for source colors,
  light direction, band counts, outlines, and transparent-background output;
  do not rely only on snapshot markup.
- Worker tests for transfer, structured failure, debounce/stale suppression,
  immediate terminate/recreate behavior, and repeated requests without leaked
  model state; do not assert cooperative active-operation cancellation.

### Demo and packaging

- Static-closure validation for the self-contained HTML and hosted directory;
  no network requests are required at runtime.
- Real Chrome tests for bundled fixture load, local STEP upload, view changes,
  Canvas/SVG switching, SVG download, JSON download, cache hits, responsive
  layout, and absence of console/page errors.
- Assert that the retained-geometry Three.js preview is nonblank for bundled
  and uploaded STEP, and that current-camera capture changes and regenerates the
  2D projection rather than only updating UI state.
- Register every test in the appropriate Rack stratum and measure added runtime
  before closeout.
- Update build/package validation and the existing demo workflow. Publication
  to any external host remains a separate explicitly approved action.
- Complete the ADR-010 promotion manifest only after all generated projections,
  native executable and browser-WASM round trips, maintained demo migration,
  durable docs, required downstream compatibility snapshots, and independent
  implementation review agree.

## Expected repository areas

Exact filenames are settled during implementation, but expected areas are:

- `src/cpp/lib/geometer/` and focused `src/cpp/lib/` illustration modules;
- `src/tsp/geometer/operations/` plus generated contract artifacts;
- CLI dispatch/options, generic operation registry, executable IPC, and C
  ABI/WASM transport wiring;
- browser exports and `src/ts/geometer/` reusable renderers/client surface;
- `src/rust/geometer-client/` and the internal generated Python contract/client
  boundary required for promotion;
- `examples/wasm/` Illustration Lab source;
- `scripts/build-typescript-examples.mjs`, a self-contained illustration demo
  builder, hosted-site builder, and browser build/package attestation;
- C++, contract, TypeScript, Rust, Python, IPC, WASM/browser, fixture, and Rack
  metadata;
- ADR, requirement, browser demo, CLI, WASM, TypeScript client, and generated
  contract documentation.

## Explicit non-goals for the first delivery

- DwgScene conversion or any dependency on another Wavenumber application;
- perspective projection guarantees;
- physically based rendering, texture maps, environment maps, or photorealism;
- exact smooth Gouraud SVG, soft shadows, or correct blended transparency;
- a general-purpose 2D drawing editor;
- cooperative cancellation of an active synchronous OCCT operation;
- publishing the demo externally;
- replacing existing HLR JSON/SVG commands or the HLR Lab.

## Completion evidence

Closeout must record:

- the synchronized source commit used for implementation;
- example CLI commands and produced JSON/SVG fixture artifacts;
- native executable/IPC/WASM parity results and deterministic-output checks;
- C++, TypeScript, Rust, and Python projection/vector evidence plus the final
  promotion-manifest state;
- real-browser evidence for SVG and Canvas modes and SVG download;
- packaged artifact paths under `dist/wasm/demos/`;
- measured fixture performance/size budgets and all added test runtimes;
- the final requested/effective capability matrix, including explicit status of
  toon, Gouraud, cast-shadow, and transparency support;
- independent implementation/promotion review findings and disposition.

Once the operation is promoted and implementation ships, keep decisions and
behavior in the durable docs of record and close/remove this temporary plan
according to repository policy.

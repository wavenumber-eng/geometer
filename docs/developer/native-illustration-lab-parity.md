# Illustration Lab / native Rust comparison

Analysis of the feature-branch implementations, 2026-09-05. This compares the
browser **Lab workflow**, not just the underlying renderer. Native renderer
parity alone does not establish application parity.

## Why rear lines appeared in front

The [browser Lab](../../examples/wasm/illustration_demo.ts) `currentStyle()`
explicitly sets `showOutlines: false` and `showCreases: false`. It separately
calls `loadHlrLinework()`, then attaches `outlineSegments` and `detailSegments`
to its prepared illustration scene. The
[STEP worker](../../examples/wasm/illustration_step_worker.js) requests Fast
HLR detail and fast mesh-shadow outline, with hidden lines disabled.

The [TypeScript renderer](../../src/ts/geometer/mesh-illustration.ts)
`renderCommands()` places all surface commands first, then raw mesh strokes,
then supplied HLR lines. Its comment explicitly calls the generic mesh-derived
linework experimental and says the Lab hides it. `edgeKind()` classifies
silhouettes/creases from adjacent-face orientation and normals; it does not
test each edge against every foreground surface. Sorting strokes by depth
among themselves does not hide them behind surfaces already painted.

The [native core](../../src/cpp/lib/mesh_illustration.cpp) retains that policy;
the [native SVG writer](../../src/cpp/lib/mesh_illustration_svg.cpp) writes
surfaces before line paths. The first Rust demo enabled `show_outlines` and
`show_creases`, while its HLR output lived in separate tabs. That enabled the
very overlays the browser Lab deliberately avoids. It is not wgpu drawing
on top of the illustration: Rust displays the SVG returned by Geometer.

The native demo now uses the Lab's raw-line defaults (both off) and composes
visibility-filtered HLR through the native illustration operation. Diagnostic
raw toggles remain available with an explicit occlusion warning. The governed
optional `hlr_projection` attachment reuses `HlrProjectionResultA0`, with one
matching millimeter/polyline view; no private Rust SVG overlay is involved.
See the [native API boundary](../design/mesh-illustration-native.md) for matching
model/frame requirements and limits.

Like the browser, native composition draws supplied HLR detail then outline
after fills because those segments are already visibility filtered. It does
not re-occlude arbitrary 2D segments or enable hidden HLR edges. `show_hlr_*`
select supplied lines and affect the original exported SVG. The demo requests
only enabled HLR layers; with both off it skips HLR completely. Its separate
HLR tabs expose the computed geometry, rather than substituting for composition.

## Settings and native mapping

The [Rust settings module](../../examples/rust/native_viewer/src/settings.rs)
uses existing TypeSpec-generated values. The additive optional IPC attachment
is declared in TypeSpec and projected into every generated operation catalog.

| Lab setting | Native control / generated mapping | Current boundary |
| --- | --- | --- |
| Shading; bands; ambient; key; rim | `MeshIllustrationStyleA0.shading`, `bands`, `ambient`, `key_intensity`, `rim_amount` | Implemented; Lab defaults Toon / 3 / 0.28 / 0.9 / 0.12, fixed Lab light direction `[0.35, 0.8, 0.48]`. |
| Source colors; fallback | `source_colors`, `fallback_color` | Implemented; default fallback `#71a6a0`. |
| Surface fusion; coplanar markings; back faces | `fuse_surfaces`, `layer_coplanar_materials`, `double_sided` | Implemented; defaults true / true / false. |
| Line color; line width | `outline_color`, `outline_width`; linked `crease_color`, `crease_width` | Implemented; default `#17252c`, width 0.006 of span; detail width = 0.55 × outline width, as in Lab. Applies to composed SVG, native raw diagnostics and separate HLR previews. |
| Background; transparent background | `background`, `transparent_background` | Implemented for SVG and HLR display; 3D diagnostic preview remains separately styled and opaque. |
| Fast crease | `HlrProjectionOptionsA0.fast.crease_angle_rad` | Implemented; 1–80° UI converted to radians, default 25°. This is the Lab's crease slider. |
| Hide coplanar joins; seam angle; seam depth | `fast.suppress_coplanar_seams`, `coplanar_seam_angle_rad`, `coplanar_seam_depth_tolerance` | Implemented; experimental seam filtering, default false / 1° / 0.001 mm. Disabled in UI when detail is not Fast. |
| HLR outline/detail toggles | `show_hlr_outline`, `show_hlr_detail`; corresponding HLR `output_*` flags | Select lines in the illustration and exported SVG; disabled layers are not computed. Both off skips HLR and disables its JSON export. |
| HLR Lab named views / Top and Front axes | Generated direction/up/mirror values from signed model-axis presets | Six face views and four ISO views match the web helper. Default Top +Y / Front +Z; parallel axes excluded. GPU, composed SVG and HLR tabs share reflection; HLR display caps/joins are round. |
| STEP chord / angle | `ModelTessellationRequestA0.linear_deflection_mm`, `angular_deflection_rad` | Implemented on load or explicit Retessellate; uses the original STEP snapshot, retains camera, and labels unapplied changes. |
| HLR relative chord / angle | `mesh_deflection_mode = bbox-relative`, `mesh_deflection_coefficient`, `mesh_angular_deflection` | Implemented, auto recompute; default coefficient 0.004 and 0.5 rad. The first Rust demo used absolute 0.1 mm instead. |
| Draft / Balanced / Fine / Extra-fine | Same four surface/HLR parameter tuples as Lab | Implemented; custom edits remain possible. Native triangle/output/resource limits still apply; the browser Lab's uncapped triangle policy is not adopted. |
| Mesh crease angle | `MeshIllustrationStyleA0.crease_angle_degrees` | Diagnostic control, **not** Fast HLR crease. Lab fixes it at 42° while disabling raw mesh creases. |
| Experimental AO enable / strength / radius / samples / bands | No native A0 mapping | Browser-only extension, explicitly unavailable. Requires separate native implementation and governed contracts; no fake controls or silent fallback. |

This is not a claim of complete UI parity. Browser Canvas output, its model
picker/GLB workflow remain distinct from the native
STEP/wgpu demo. Compare the same mesh and explicit transformed direction/up
values, not similarly named camera buttons or different default views. The
browser converts GLB/Three model data and transforms; native colored meshes
are obtained directly through the governed STEP tessellation operation.

## Focused evidence and next work

The optional RUST_002 native-settings check uses the same SOT-23 generated input
through TypeScript and native illustration and compares the entire A0 result.
Raw mesh lines off produces zero outline/crease commands with nonempty surfaces;
turning them on produces line commands and a different SVG. The same test
verifies finer STEP settings increase triangle count, Fast crease 1° versus
80° changes returned detail geometry, and disabling both HLR layers returns
empty linework. Node is a test oracle only, never a GUI runtime dependency.

The Rust client composition test now compares the complete native A0 result
byte-for-byte with TypeScript `createIllustrator` for three SOT-23 views,
reflection and every outline/detail toggle combination (24 cases), with native
deterministic repeats. C++ checks equivalent scaled/skewed bases, invalid bases,
nonfinite linework and disabled-layer resource caps; Python checks typed
composition and malformed optional attachments. Native GUI smoke displays the
composed result. Node remains a test oracle, never a runtime dependency.

The result pane is white and fixed across status changes, with the SVG fitted
and centered in both axes and a direct Save SVG action. Surface coalescing is
the shared `fuse_surfaces` Geometer renderer option in TS and C++; automatic
SVG style deduplication is distinct from coplanar material layering.

Experimental AO, Mac runtime qualification and exhaustive human UI acceptance
remain separate work. This is feature-branch evidence, not a public release.

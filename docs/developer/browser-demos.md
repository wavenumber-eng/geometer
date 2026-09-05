# Browser Demo Packaging And UI

This document defines the maintained source, build, review, and static-hosting
shape for Geometer browser demos. ADR 014 governs the distribution decision;
REQ-009 defines the supported release surface.

## Directory Model

| Purpose | Location |
| --- | --- |
| Maintained HTML, JavaScript/TypeScript, Worker, and demo CSS | `examples/wasm/` |
| Generic dependency-free demo UI/interaction modules | `examples/wasm/demo-tooling/` |
| Full browser Geometer runtime | `dist/wasm/browser/` |
| Compiled TypeScript examples/tooling | `dist/wasm/demos/` |
| Self-contained HTML artifacts | `dist/wasm/demos/<demo>_demo.html` |
| Deploy-unchanged hosted directories | `dist/wasm/demos/<demo>/` |
| Static closure validation | `tests/typescript/` |
| Real-browser workflow validation | `tests/wasm/` |
| Generic packager unit tests | `tests/python/` |

Demo code is a consumer of Geometer. Product-specific UI behavior must not move
into `src/cpp/lib/`, the C ABI, or generated contracts merely to support a page.

Promoted operations use their governed operation identities and production
package APIs. The HLR and Illustration Lab Workers invoke
`geometry.model_hlr_projection.a0` through the generic C ABI adapter; the
illustration application imports the production illustration modules. The
remaining focused STEP-to-GLB call is a compatibility conversion
surface, not illustration or HLR policy.

The HLR and Illustration Labs use the same Fast HLR resource defaults as stable
API consumers. They do not carry demo-only limit overrides.

## Build Layers

A single-HTML hosted demo has two explicit build layers.

1. A demo-specific builder creates a literal standalone HTML file. It owns the
   application bundle and embeds all required Workers, WASM, models, images,
   fonts, third-party notices, and styles.
2. `scripts/package_single_html_site.py` verifies that file and stages the
   hosted directory atomically.

The generic packager does not know the application's assets or JavaScript
framework. It provides:

- rejection of external `src`, `href`, `poster`, and `data` references;
- rejection of remaining ESM/import-map runtime dependencies;
- CSP hashes for executable inline scripts;
- `worker-src blob:` and the minimum data/blob policies used by standalone
  browser/WASM pages;
- Cloudflare-compatible `_headers`;
- deterministic `wn.geometer.single_html_site.a0` closure metadata; and
- atomic replacement of the destination directory.

`asset-manifest.json` is verification metadata and is not fetched by the page.
Its `files` closure contains `_headers` and `index.html`; the manifest excludes
itself to avoid a recursive digest. `runtime_files` must be exactly
`["index.html"]`.

## HLR Lab

Build the current HLR artifact and hosted directory after the browser WASM
artifacts are available:

```powershell
python scripts\build_hlr_site.py
```

Outputs:

```text
dist/wasm/demos/hlr_demo.html
dist/wasm/demos/hlr/index.html
dist/wasm/demos/hlr/_headers
dist/wasm/demos/hlr/asset-manifest.json
```

Review it locally:

```powershell
python -m http.server 8123 --bind 127.0.0.1 --directory dist\wasm\demos\hlr
```

Then open `http://127.0.0.1:8123/`. Building and reviewing do not publish the
directory.

The HLR Lab embeds:

- the full OCCT-backed Geometer browser WASM and factory;
- a blob-backed Worker for STEP-to-GLB and HLR work;
- Three.js, TrackballControls, GLTFLoader, and the exact Three.js license;
- local STEP/GLB example fixtures and Wavenumber presentation assets; and
- all application and reusable-panel JavaScript/CSS.

Its maintainable shell and application module are
`examples/wasm/embedded_model_viewer.html` and
`examples/wasm/embedded_model_viewer.js`. The standalone builder inlines the
module after replacing its development-only imports and asset references.

Local `.step` and `.stp` files stay in the browser. The Worker converts their
bytes to the 3D preview and projection data; no upload endpoint exists.

## Reusable Panel System

`examples/wasm/demo-tooling/panels.ts` exports a typed `PanelManager` with:

- left, right, and bottom activity rails;
- hidden, collapsed, and open panel states;
- resizable side and bottom docks;
- deterministic content inset CSS variables;
- a small `mount`/`onStateChange`/`destroy` panel lifecycle; and
- no application state-manager or third-party runtime dependency.

`panels.css` supplies the generic presentation. Consumers may override the
`--gdm-panel-*` variables. Empty rails remain hidden so they do not obscure or
reserve viewport pixels.

Panels should contain secondary controls. Primary workflow actions and compact
view presets may stay in the page toolbar. Settings that affect geometry should
recompute automatically; typed numeric inputs should be debounced. Pure display
changes should redraw cached geometry instead of rerunning WASM.

## STEP Illustration Lab

The generic mesh-illustration engine is a production TypeScript package module
at `@wavenumber/geometer/mesh-illustration`. Its serialized input, style, and
SVG result start at A0. `illustrateMesh` is the one-shot entry point;
`createIllustrator` prepares once, supports repeated SVG or Canvas rendering,
and has explicit disposal. The Illustration Lab in
`examples/wasm/illustration_demo.*` consumes that package implementation and
retains only STEP/glTF adaptation, controls, and review presentation.

The production implementation has these boundaries:

- the core projector consumes generic mesh buffers, transforms, materials, and
  normals rather than STEP objects;
- the current STEP adapter converts browser-local bytes through the existing
  compatibility STEP-to-GLB symbol and calls the governed
  `geometry.model_hlr_projection.a0` operation for its hierarchical
  `fast-mesh-shadow` Outline and Fast Detail layers;
- SVG and Canvas2D consume one prepared projected scene and a separate style,
  so lighting or palette changes do not redo mesh preparation;
- the shared renderer offers unlit, flat, unquantized Lambert diffuse, banded (2-32),
  and toon surface shading;
- STEP-backed demo inputs can be remeshed through Draft, Balanced, Fine, Extra
  fine, or custom linear/angular deflection settings, with separately grouped
  HLR relative chord/angular tolerances and no demo-imposed triangle-count cap;
- remeshing the active STEP model preserves its orthographic camera pose,
  target, zoom, and framing scale;
- filled triangles use spatially culled overlap tests and depth-plane ordering
  rather than a global average-depth painter sort, and cache that geometry-only
  order across style-only SVG/Canvas redraws;
- an optional conservative fusion pass contracts opaque, depth-connected
  triangles with identical rendered fill into even-odd polygon regions after
  visibility ordering; mobility intervals allow fusion across unrelated paint
  commands without crossing an overlapping different-style surface, while
  projected folds, overlaps, non-manifold edges, and invalid boundaries retain
  their original triangles;
- when enabled with fusion, coplanar-material layering groups opaque,
  same-plane partitions connected by complete mesh edges, underpaints their
  union with the largest-area style, and overpaints the remaining fused styles;
  the group is accepted only when all members share a safe paint-order interval,
  otherwise the normal surface commands remain authoritative;
- SVG output uses shared palette/line classes, a configurable normalized integer
  coordinate grid, compact polygon paths, and chained compound HLR paths; Canvas
  consumes the same cached render-command preparation;
- the Three.js pane and glTF adapter are demo concerns, not dependencies of the
  generic illustration algorithm; and
- STEP-backed surfaces can composite that HLR outline in both SVG and Canvas;
  generic mesh-only sources remain surface-only unless an adapter supplies a
  compatible linework layer; and
- a separate HLR Detail checkbox composites fast visible boundary, crease, and
  silhouette segments beneath the independently computed fast mesh-shadow
  outline.

Build its hosted review directory with:

```powershell
python scripts\build_illustration_site.py
```

The output is `dist/wasm/demos/illustration/`; its only runtime file is
`index.html`. The single-file form is
`dist/wasm/demos/illustration_demo.html`. Neither builder publishes artifacts.

## Adding A Single-HTML Demo

1. Add the maintained page/application source under `examples/wasm/`.
2. Put generic behavior in `demo-tooling`; keep domain-specific controls with
   the demo.
3. Add the TypeScript entry/output to `examples/wasm/tsconfig.json` and
   `scripts/build-typescript-examples.mjs` when applicable.
4. Add `scripts/build_self_contained_<demo>.py`. Use the shared helpers in
   `scripts/standalone_html.py`, embed required third-party licenses, and call
   `assert_self_contained` before writing the artifact.
5. Add `scripts/build_<demo>_site.py` as the thin wrapper around the standalone
   builder and `package_single_html_site`.
6. Add a static validation script under `tests/typescript/`, a real-browser test
   under `tests/wasm/`, and the corresponding Rack stratum entries.
7. Add the builder and validation paths to `.github/workflows/wasm.yml`.
8. Build locally, inspect desktop and narrow screenshots, and obtain explicit
   publication approval before copying the output directory to a host.

## Validation

Focused HLR/demo-platform checks are:

```powershell
python scripts\build_hlr_site.py
python scripts\build_illustration_site.py
node tests\typescript\hlr_static_site_validation.mjs
node tests\typescript\illustration_static_site_validation.mjs
uv run pytest tests\python\test_package_single_html_site.py -q
uv run pytest tests\wasm\test_hlr_static_site.py -q
uv run pytest tests\wasm\test_illustration_static_site.py -q
npm run check:typescript
```

The Chrome test covers startup, panel lifecycle and resizing, selectable model
axes, model changes that preserve a named view, Trackball camera projection,
geometry auto-reprojection/reset, 3D material/light/shading controls, local STEP
conversion, independent projection appearances, and SVG export. It also rejects
uncaught exceptions and unexpected network requests.

## Publication Boundary

The repository intentionally has no automatic upload in these builders or
tests. A future publishing workflow must name the exact hosted directory,
preserve the checked closure, and require explicit authorization. DNS, Cloudflare
project selection, aliases, and cache invalidation are deployment concerns and
must not be inferred from a successful local build.

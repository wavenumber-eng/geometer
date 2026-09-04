# CLI Interfaces

The current implemented CLI is documented below. The additive persistent
`serve --stdio` mode follows [Executable IPC A0](executable-ipc-a0.md); normal
file-oriented commands remain unchanged.

Native CLI:

```powershell
.\dist\native\windows-x64\geometer.exe --version
.\dist\native\windows-x64\geometer.exe model-bounds input.step output.bounds.json --format step
.\dist\native\windows-x64\geometer.exe model-to-glb input.step output.glb --format step
.\dist\native\windows-x64\geometer.exe model-project-hlr input.step output.json --format step
.\dist\native\windows-x64\geometer.exe model-project-svg input.step output.svg --format step --mode outline --view top
.\dist\native\windows-x64\geometer.exe mesh-project-hlr input.mesh output.json --options fast-options.json
.\dist\native\windows-x64\geometer.exe step-to-glb input.step output.glb
.\dist\native\windows-x64\geometer.exe step-project-hlr input.step output.json
.\dist\native\windows-x64\geometer.exe step-project-svg input.step output.svg --mode outline --view top
.\dist\native\windows-x64\geometer.exe planar-step planar-step-request.json output.step
.\dist\native\windows-x64\geometer.exe init-request request.json --step input.step --operation step_hlr_projection_json --output output.json
.\dist\native\windows-x64\geometer.exe run request.json response.json
.\dist\native\windows-x64\geometer.exe planar-batch-solve request.bin response.bin --warmup 1 --repeat 5 --metrics metrics.json
.\dist\native\windows-x64\geometer.exe serve --stdio
```

`serve --stdio` is a machine protocol endpoint, not an interactive command.
stdin and stdout contain only binary `GMIPCA01` frames, while diagnostics and
logs use stderr. Use the generated Rust client rather than writing framed bytes
in application code. The initial live operation is
`geometry.model_bounds.a0` with a raw STEP attachment.

Root-level `dist/geometer(.exe)` artifacts are no longer produced. Source
checkout consumers should use `dist/native/<platform>/geometer(.exe)`.

Node WASM CLI parity/test target:

```powershell
node dist\wasm\node-test\geometer-node-test.js step-to-glb input.step output.glb
```

Projection CLI options:

- `--format <step>`
- `--view <id>`
- `--mode <outline|detail|bbox>`
- `--curve-mode <native-arcs|polyline>`
- `--samples <count>`
- `--round-digits <count>`
- `--projection-algorithm <poly|exact|fast>`
- `--outline-algorithm <hlr-close|mesh-shadow|fast-mesh-shadow>`
- `--deflection-mode <absolute|bbox-relative>`
- `--deflection-coefficient <value>`

`--projection-algorithm fast` selects Fast vector detail without changing the
default `poly` behavior. `--outline-algorithm fast-mesh-shadow` independently
selects the Fast outline implementation. They may be used together or with an
older compatible counterpart.

The direct projection commands expose the common selectors above. The `run`
request is the file-oriented route for the full canonical nested `fast` block,
including boundary, crease, silhouette, hidden-line, coplanar-seam, tolerance,
and resource-limit controls. Canonical angles such as `crease_angle_rad` are
radians; the browser demos convert their degree controls at the UI boundary.
See [HLR Projection A0](hlr-projection-a0.md#fast-controls) for every member,
default, unit, and compatibility alias.

`mesh-project-hlr` accepts the governed indexed-triangle-mesh A0 packet and
writes `geometry.hlr_projection.result.a0`. With no options file, it selects
Fast detail and Fast mesh-shadow, the only applicable mesh backends. Its
optional `--options` file is a strict canonical
`geometry.hlr_projection.options.a0` object. The equivalent batch operation is
`mesh_hlr_projection_json`, with `mesh_path`, `output_path`, and optional
canonical `options`; batch-level options are applied first.

STEP-to-GLB CLI options:

- `--deflection <value>`
- `--angular <value>`

Planar STEP CLI:

- `planar-step <request.json> <output.step>` reads a
  `geometry.planar_step.request.a0` request and writes a colored STEP file.

JSON batch CLI commands:

- `run <request.json> <response.json>`: run a
  `geometer.batch.request.a0` jobs array and write a
  `geometer.batch.response.a0` response.
- `init-request <request.json> --step <path>`: write a starter JSON request.
- `--operation <step_hlr_projection_json|step_hlr_projection_svg|step_to_glb>`:
  choose the starter request operation.
- `--output <path>`: choose the starter request output path.

Batch requests accept an optional top-level `options` object. Batch jobs accept
`operation`, `step_path`, `output_path`, and an optional job-level `options`
object. `planar_step` jobs instead accept `request_path` or an inline
`planar_step_request` object. Geometer parses top-level options first, then
parses job-level options on top, so callers can put shared settings such as `curve_mode`,
`samples_per_curve`, `round_digits`, `mesh_linear_deflection`, or
`mesh_angular_deflection` at the request level and only override the fields
that differ per job. HLR JSON/SVG jobs use HLR projection options; GLB jobs use
STEP-to-GLB options. The response includes Geometer version, ABI, top-level
`ok`, and per-job `id`, `operation`, `ok`, `code`, `elapsed_ms`, and optional
`output_path` or `message`.

For source-model jobs, prefer:

- `model_bounds_json`
- `model_hlr_projection_json`
- `model_hlr_projection_svg`
- `model_to_glb`

These operations currently accept only `format: "step"` and accept either
`model_path` or compatibility `step_path`. Existing `step_hlr_projection_json`,
`step_hlr_projection_svg`, and `step_to_glb` jobs remain supported.

Planar batch solve CLI options:

- `--warmup <count>`: run unmeasured solves before benchmark repeats.
- `--repeat <count>`: measured solve repeats.
- `--metrics <path>`: write JSON metrics with request/response byte sizes and
  min/mean/max/last solve time.
- `--format <binary|json>`: choose packed binary output or fused ring JSON.
- `--return-rings <true|false>`: compatibility alias for `--format json`.

`planar-batch-solve` uses the same packed request/response byte format as
`solve_planar_batch_from_bytes` and the browser C ABI. It is intended for
native-vs-WASM diagnostics and benchmark comparisons.

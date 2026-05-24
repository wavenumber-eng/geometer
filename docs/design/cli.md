# CLI Interfaces

Native CLI:

```powershell
.\dist\native\windows-x64\geometer.exe --version
.\dist\native\windows-x64\geometer.exe step-to-glb input.step output.glb
.\dist\native\windows-x64\geometer.exe step-project-hlr input.step output.json
.\dist\native\windows-x64\geometer.exe step-project-svg input.step output.svg --mode simple --view top
.\dist\native\windows-x64\geometer.exe planar-step planar-step-request.json output.step
.\dist\native\windows-x64\geometer.exe init-request request.json --step input.step --operation step_hlr_projection_json --output output.json
.\dist\native\windows-x64\geometer.exe run request.json response.json
.\dist\native\windows-x64\geometer.exe planar-batch-solve request.bin response.bin --warmup 1 --repeat 5 --metrics metrics.json
```

Root-level `dist/geometer(.exe)` artifacts are no longer produced. Source
checkout consumers should use `dist/native/<platform>/geometer(.exe)`.

Node WASM CLI parity/test target:

```powershell
node dist\wasm\node-test\geometer-node-test.js step-to-glb input.step output.glb
```

Projection CLI options:

- `--view <id>`
- `--mode <simple|detail>`
- `--curve-mode <native-arcs|polyline>`
- `--samples <count>`
- `--round-digits <count>`

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

Planar batch solve CLI options:

- `--warmup <count>`: run unmeasured solves before benchmark repeats.
- `--repeat <count>`: measured solve repeats.
- `--metrics <path>`: write JSON metrics with request/response byte sizes and
  min/mean/max/last solve time.

`planar-batch-solve` uses the same packed request/response byte format as
`solve_planar_batch_from_bytes` and the browser C ABI. It is intended for
native-vs-WASM diagnostics and benchmark comparisons.

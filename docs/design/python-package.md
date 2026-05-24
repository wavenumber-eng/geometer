# Python Package Interface

## Python Interface

The source checkout and PyPI wheel include a thin Python package named
`geometer`; the PyPI distribution name is `wn-geometer`. The package drives the
native CLI and keeps the public API byte/path oriented:

```python
import geometer

version = geometer.version()
projection = geometer.project_step_hlr(
    "part.step",
    views=[geometer.ProjectionView.top()],
    options=geometer.HlrOptions.assembly_outline(),
)
json_text = geometer.hlr_projection_json("part.step")
glb_bytes = geometer.step_to_glb("part.step")

runner = geometer.GeometerBatchRunner(max_workers=8, chunk_size=5)
runner_version = runner.version()
batch = runner.run(
    [
        {
            "id": "part-top",
            "operation": "step_hlr_projection_json",
            "step_path": "part.step",
            "output_path": "part.top.projection.json",
        }
    ],
    options={"curve_mode": "polyline"},
)
```

The Python package intentionally uses the executable backend only for now. It
looks for the Geometer CLI in this order:

- `GEOMETER_EXE`;
- the package directory;
- `geometer/native/<platform>` inside the installed package or source checkout;
- `geometer/native` inside the installed package or source checkout;
- source checkout `dist/native/<platform>`;
- `PATH`.

Published wheels are platform wheels because they bundle the native executable.
Linux wheels must be repaired/tagged with `auditwheel` before PyPI upload; the
wheel contents install under `platlib`, not `purelib`, so auditwheel can inspect
the bundled ELF executable.

The executable backend writes temporary STEP/request/output files, calls:

```powershell
geometer run request.json response.json
```

and returns the requested JSON text or GLB bytes to the Python caller.

`GEOMETER_BACKEND=exe` and `GEOMETER_BACKEND=cli` are accepted explicit names.
Other backend names are rejected by the public Python API.

The native CLI now has an initial JSON batch interface:

```powershell
geometer run request.json response.json
geometer init-request request.json --step U1.step --operation step_hlr_projection_json --output U1.projection.json
```

`request.json` contains a `geometer.batch.request.a0` jobs array so one process
can project or convert multiple STEP files before exiting. An optional top-level
`options` object supplies defaults for every job; each job's own `options`
object is parsed afterwards and overrides those defaults.
`version` and `abi` fields in a request are metadata written by `init-request`;
`run` only requires the `jobs` array.

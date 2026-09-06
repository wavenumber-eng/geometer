# Python Package Interface

## Python Interface

The source checkout and PyPI wheel include a thin Python package named
`geometer`; the PyPI distribution name is `wn-geometer`. The package drives the
native CLI and keeps the file-oriented convenience API byte/path oriented.

The persistent `GeometerClient` also exposes generated-value operations. Feature
builds include [colored STEP tessellation and native mesh illustration](mesh-illustration-native.md)
with a complete Python STEP-to-SVG example. Those new methods require a matching
feature executable; the previously released wheel does not provide them.

The public one-shot helpers `geometer.model_tessellation(step_bytes, ...)` and
`geometer.mesh_illustration(input, hlr_projection=..., ...)` reuse that same
client with automatic process cleanup. Both accept `executable=` for an explicit
override and `timeout=` for the local operation deadline; startup/shutdown have
the client's separate bounds. Prefer one `with geometer.GeometerClient() as
client:` block for a STEP/tessellation/HLR/illustration sequence or repeated work.
The optional HLR argument is already computed visible polyline geometry, not an
instruction to compute HLR or re-occlude arbitrary 2D lines.

```python
from pathlib import Path

import geometer

version = geometer.version()
model_bounds = geometer.model_bounds("part.step", format="step")
projection = geometer.project_step_hlr(
    "part.step",
    views=[geometer.ProjectionView.top()],
    options=geometer.HlrOptions.assembly_outline(),
)
outline_only = geometer.project_step_hlr(
    "part.step",
    views=[geometer.ProjectionView.top()],
    options=geometer.HlrOptions(
        outline_algorithm="mesh-shadow",
        output_outline=True,
        output_detail=False,
        output_bbox=False,
    ),
)
fast_outline = geometer.project_step_hlr(
    "part.step",
    views=[geometer.ProjectionView.top()],
    options=geometer.HlrOptions.fast_assembly_outline(),
)
fast_detail = geometer.project_step_hlr(
    "part.step",
    views=[geometer.ProjectionView.top()],
    options=geometer.HlrOptions(
        projection_algorithm="fast",
        fast={"crease_angle_rad": 0.4363323129985824},
        output_outline=False,
        output_detail=True,
        output_bbox=False,
    ),
)
generic_projection = geometer.project_model_hlr(
    "part.step",
    format="step",
    views=[geometer.ProjectionView.top()],
)
json_text = geometer.hlr_projection_json("part.step")
generic_json_text = geometer.model_hlr_projection_json("part.step", format="step")
glb_bytes = geometer.model_to_glb("part.step", format="step")
planar_step_request = {
    "schema": "geometry.planar_step.request.a0",
    "units": "mm",
    "bodies": [
        {
            "id": "copper",
            "thickness_mm": 0.035,
            "regions": [
                {
                    "outer": {
                        "points": [[0, 0], [10, 0], [10, 5], [0, 5]],
                        "segments": [{"kind": "line"}] * 4,
                    }
                }
            ],
        }
    ],
}
planar_step_bytes = geometer.planar_step(planar_step_request)
geometer.write_planar_step(planar_step_request, "layer.step")

planar_rings = geometer.planar_batch_solve("planar-batch-request.bin")
first_region = planar_rings.regions()[0]
outer_ring_mm = first_region.outer
hole_rings_mm = first_region.holes

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

The Python package intentionally uses the executable backend only for now.
Existing file-oriented helpers use the JSON batch CLI, while `GeometerClient`
holds one `geometer serve --stdio` process for typed model/mesh HLR and
analytic packed requests.
Both paths discover the Geometer executable in this order:

- `GEOMETER_EXE`;
- the package directory;
- `geometer/native/<platform>` inside the installed package or source checkout;
- `geometer/native` inside the installed package or source checkout;
- source checkout `dist/native/<platform>`;
- `PATH`.

## Generated contract boundary

The package contains generated dependency-free dataclasses, enums, and strict
JSON codecs under `geometer._generated.contracts`. They are the structural
authority. Most generated declarations remain an internal compatibility
boundary; the selected analytic request/result construction types are
re-exported from `geometer` for the typed packed client. The generator reads
the normalized TypeSpec catalog and is part of the ordinary freshness gate.

`model_bounds` is the first operation integrated through this boundary. The
public compatibility adapter continues to accept `model_format`,
`modelTransform`, uppercase `STEP`, nested 4-by-4 transforms, and ignored
mapping members, then normalizes them to the closed generated request. The
generated codec validates the canonical request before the executable call and
validates its result before the existing public `ModelBoundsResult` convenience
wrapper is constructed. Public names, signatures, return attributes,
executable discovery, and all other CLI-backed operations remain unchanged.

The persistent client exposes governed HLR without temporary files:

```python
import geometer

options = geometer.HlrProjectionOptionsA0(
    projection_algorithm=geometer.HlrProjectionAlgorithm.FAST,
    outline_algorithm=geometer.HlrOutlineAlgorithm.FAST_MESH_SHADOW,
    fast=geometer.FastHlrOptionsA0(crease_angle_rad=0.4363323129985824),
    output_detail=True,
)
with geometer.GeometerClient() as client:
    step_result = client.model_hlr_projection(Path("part.step").read_bytes(), options)
    mesh_result = client.mesh_hlr_projection(
        geometer.IndexedTriangleMeshA0(
            positions=(0.0, 0.0, 0.0, 10.0, 0.0, 0.0, 0.0, 10.0, 0.0),
            indices=(0, 1, 2),
            source_faces=(1,),
        )
    )
```

The analytic planar Boolean candidate is integrated through the same
executable-backed lane:

> This method is experimental and not production-ready. It may fail closed on
> valid inputs and is not the dependable path for whole-board or whole-layer
> copper union. Prefer the Clipper2-backed planar APIs when polygonized output
> is suitable.

```python
import geometer

request = geometer.AnalyticPlanarBooleanBatchRequestA0(jobs=(), relationship_queries=())
with geometer.GeometerClient() as client:
    result = client.analytic_planar_boolean_batch(request, timeout=10)
```

Consumers that need a governed artifact rather than an immediate solve may
call `encode_analytic_planar_boolean_batch_request_a0_packet(request)`. The
matching `decode_analytic_planar_boolean_batch_result_a0_packet(packet)`
strictly validates and projects a packed result. These are the same codec
functions used by `GeometerClient`; they do not create a second JSON or binary
authority.

`GeometerClient` performs the A0 handshake, validates the generated operation
catalog and negotiated limits, and sends raw named attachments over binary
frames. `_analytic_packet_a0.py` is the strict little-endian codec for the
logical analytic DTOs; no JSON geometry is placed on the hot path. It enforces
the canonical packet graph and the batch-wide 1,048,576 logical
source-reference expansion limit before materializing result DTOs. Protocol,
process, timeout, and typed operation failures have distinct public exception
classes. The client is synchronous and serialized, uses queue-only
cancellation after a local timeout, drains terminal responses before reuse,
and treats malformed negotiated responses as connection-fatal.

The runtime uses only the Python standard library. This adds no wheel runtime
dependency and supports the package's existing Python 3.10 floor. Generated
source is included by normal setuptools package discovery. Clean-wheel tests
prove public analytic DTO/client imports, empty and nontrivial packed analytic
IPC calls against the bundled executable, and a live public model-bounds round
trip.

Published wheels are platform wheels because they bundle the native executable.
Linux wheels must be repaired/tagged with `auditwheel` before PyPI upload; the
wheel contents install under `platlib`, not `purelib`, so auditwheel can inspect
the bundled ELF executable.

Published wheels also install a `geometer` console script. The script is a thin
Python launcher that resolves the packaged native executable and forwards
arguments to it, so `python -m pip install wn-geometer` makes the native CLI
available inside the active environment without editing a user's global `PATH`.

The planar STEP Python API is the supported package interface for Python
callers. It is executable-backed internally, like the HLR and GLB helpers, but
downstream packages should depend on `geometer.planar_step(...)` or
`geometer.write_planar_step(...)` rather than invoking `geometer.exe` directly.

For source-model operations, prefer the generic model names:

- `model_bounds(...)`
- `model_bounds_json(...)`
- `model_to_glb(...)`
- `project_model_hlr(...)`
- `model_hlr_projection_json(...)`

The only supported source model format is currently `format="step"`.
Compatibility wrappers remain available:

- `step_to_glb(...)`
- `project_step_hlr(...)`
- `hlr_projection_json(...)`

The file-oriented executable backend writes temporary STEP/request/output
files and calls:

```powershell
geometer run request.json response.json
```

and returns the requested JSON text or GLB bytes to the Python caller. The
analytic client instead uses the persistent binary `serve --stdio` protocol;
it does not write geometry packets to temporary files.

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

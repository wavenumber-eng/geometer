# Colored model tessellation A0

Introduced in 2026.9.5; see [release qualification](../developer/native-api-readiness.md).
`geometry.model_tessellation.a0` is a stateless STEP-to-colored-mesh operation.
Its TypeSpec definitions live in `src/tsp/geometer/operations/model-tessellation-a0.tsp`;
the catalog generates C++, Rust, Python, TypeScript, JSON Schema and HTML.
It does not change existing STEP-to-GLB, HLR or experimental topology behavior.

## Request and result

Send `geometry.model_tessellation.request.a0` plus a raw `model` attachment
(`application/step` or `model/step`, up to 256 MiB). Defaults: absolute linear
deflection 0.1 mm, angular deflection 0.5 radians, 750,000 output triangles.
`root_placement=strip` removes free-root placement, matching existing GLB;
`preserve` retains it. Neither mode removes component placement.
External STEP references are rejected before transfer; the operation will not
resolve other model files from the filesystem. Supply a self-contained STEP.

The result descriptor identifies one `mesh_collection` attachment with media
type `application/vnd.wavenumber.geometer.mesh-collection+json`. These are UTF-8
JSON bytes validated against `geometry.mesh_collection.a0`, not a private binary
format. The result contains the source and attachment SHA-256, attachment byte
length, mesh count and triangle count. Typed clients verify these before
returning a `ModelTessellation` with `metadata` and `mesh_collection`.

Positions are millimeters, independent of the caller's OCCT global unit setting.
Current tessellation output bakes occurrence/face transforms into positions and
normals and omits the optional matrix. Indices are zero-based; winding follows
face orientation and mirrored placement. Normals are supplied where OCCT can
provide them. Mesh IDs are deterministic within the result, not persistent
topological identifiers. Each mesh currently represents one triangulated face.

Materials reuse the illustration DTO: sRGB color, opacity and optional name.
XCAF document/face iterators resolve source occurrence and subshape styles;
colors are converted from OCCT linear channels to sRGB. Unassigned material uses
`[0.72, 0.74, 0.78]`, opacity 1. Invisible nodes/faces are omitted. Textures and
product-specific styling are not part of this boundary.

## Limits and failure

Maximum accepted output: 2,000,000 triangles (or the lower requested cap),
2,000,000 vertices across the collection, 65,536 meshes/leaf occurrences and
256 MiB serialized mesh JSON. Transport envelopes remain at 8 MiB; mesh payloads
use the existing bounded attachment path. Mesh failures/status flags reject the
call; no partial successful result is returned. Malformed STEP and limit errors
are operation failures, and the persistent process can service the next call.
While serving IPC, OCCT diagnostics go to stderr, never the binary stdout stream.

These are input/output acceptance limits, **not a hard peak-memory or CPU bound
inside OCCT**. STEP transfer and meshing precede output validation; generated
JSON serialization also has transient allocation costs. Do not use this
operation as an untrusted-model sandbox. Active OCCT work cannot be
queue-cancelled; use the existing client timeout/process-termination semantics
and OS-level resource isolation where required.

## Rust and Python

```rust,no_run
let result = client.model_tessellation(
    geometer_client::ModelTessellationRequest::step(step_bytes)
).await?;
let meshes = result.mesh_collection.meshes;
```

```python
import geometer

with geometer.GeometerClient() as client:
    result = client.model_tessellation(step_bytes)
    meshes = result.mesh_collection.meshes
```

Use a 2026.9.5 or later compatible executable matching the generated catalog. Source Rust
process tests accept `GEOMETER_EXECUTABLE`; Python uses `GEOMETER_EXE`. Released
2026.9.4 executables do not advertise this operation. A generated TypeScript DTO
does not prove that an older browser WASM binary supports the new operation.

This operation supplies the [Rust GPU preview](../../examples/rust/native_viewer/README.md)
and [native illustration](mesh-illustration-native.md). Tessellation itself does
not render SVG: call mesh illustration next, optionally supplying the visible
result of a separate Fast HLR request for native line composition.

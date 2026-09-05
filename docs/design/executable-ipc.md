# Calling The Geometer Executable

Use persistent IPC when an application wants to reuse one geometry process and
exchange model/mesh bytes without temporary files. Start `geometer serve --stdio`
through a maintained client. For one-off conversions, the [file CLI](cli.md)
and [Python convenience API](python-package.md) are simpler.

## Client And Executable Selection

| Consumer | Maintained boundary |
| --- | --- |
| Python | Public `geometer.GeometerClient`: synchronous HLR and experimental analytic convenience calls. The package resolves its bundled executable; `executable=` selects an explicit compatible binary. |
| Node/TypeScript | `GeometerNodeProcessA0` from `@wavenumber/geometer/node-process-a0`: process ownership plus typed generic IPC calls. |
| Rust | `geometer-client`: generated contracts and Tokio executable client; see [Rust client](rust-client.md). |
| Implementing a new transport/client | [A0 framing and lifecycle specification](executable-ipc-a0.md), not a hand-built application protocol. |

Use a client and executable generated from a compatible catalog. The clients
validate the welcome catalog/digest and effective limits; updating only a
binary while retaining stale generated clients can fail the handshake.
Source artifacts live at `dist/native/<platform>/geometer(.exe)`.

## Runnable Model-Bounds Example

From a source checkout with Node 24 and committed release artifacts:

```powershell
node examples/node/ipc-model-bounds.mjs dist/native/windows-x64/geometer.exe tests/fixtures/step/embedded_models/SOT-23.STEP
```

The [example source](../../examples/node/ipc-model-bounds.mjs) spawns the
process, discovers the operation, sends a typed options object with a raw STEP
attachment named `model`, checks the outcome, prints the result, and closes
the process. Substitute your platform executable path on Linux/macOS.
The source-checkout import points to the distributable package; installed
package consumers use its exported package specifier above.

## Mesh Attachment Example (Python)

The public client packs the mesh and sends a named binary attachment; JSON
contains HLR options, not base64 geometry:

```python
import geometer

mesh = geometer.IndexedTriangleMeshA0(
    positions=(0.0, 0.0, 0.0, 10.0, 0.0, 0.0, 0.0, 10.0, 0.0),
    indices=(0, 1, 2),
    source_faces=(1,),
)
with geometer.GeometerClient() as client:
    result = client.mesh_hlr_projection(mesh, timeout=30)
    print(result.schema)
```

For STEP HLR, call `client.model_hlr_projection(step_bytes, options)`.
Feature builds additionally expose typed `model_tessellation` in Rust and Python
for [colored model meshes](model-tessellation-a0.md); use its matching catalog
and executable, not the older released binary.
The file-oriented `geometer.model_bounds(...)` helper does not demonstrate
persistent IPC; use the Node example above for the generated bounds operation.

## Request Lifecycle And Failures

1. Client starts the executable with separate binary stdin/stdout and stderr.
2. `hello` / `welcome` negotiates A0, effective resource limits and operations.
3. Each request has a correlation ID, generated JSON DTO, and declared raw
   attachments. Names, media types, lengths and operation identity must agree.
4. The server executes serially and returns a typed outcome plus any output
   attachments. A valid failure outcome is distinct from a corrupt connection.
5. Graceful shutdown rejects queued work with ordinary failure responses
   (`geometer.transport.server_shutting_down`), allows only the active request
   to finish, then flushes responses and acknowledgement within one 30-second
   deadline. It does not execute all accepted queued requests.

Never decode stdout as text, merge stderr into stdout, or share the process
pipes with a second owner. Drain stderr so logging cannot block the process.
The maintained clients handle these details.

A local timeout does not stop the geometry solver. A0 cancellation is queue
only: active work is not interruptible. The Python client sends cancellation
after timeout and must drain the terminal response before reuse; an immediate
new call may report that the prior request is still draining. If a hard deadline
requires process termination, all outstanding calls fail and a fresh handshake
is required. Experimental topology session handles are process-local and must
not be reused after restart.

Distinguish typed operation diagnostics, local timeout, protocol violation,
and process exit. Python exposes `GeometerOperationError`,
`GeometerIpcTimeoutError`, `GeometerIpcProtocolError` and
`GeometerIpcProcessError`. Do not blindly replay state-mutating operations
after a lost response; the outcome may be unknown.

## What Is Callable?

The [generated operation reference](../generated/contracts/index.html) and
welcome catalog distinguish portable operations, additional native-only
experimental operations and structural-only declarations. Clipper2 packed
entry points and legacy conversion commands are not automatically IPC
operations. See [contract authority](../contracts/README.md).

Analytic planar Boolean is experimental and not production-ready even though
its packed interface is implemented. Prefer Clipper2-backed polygonized
operations for production visualization where suitable. Protocol generation,
release version, C ABI generation and packed-format versions are independent.

Byte offsets, frame-kind values, limits, control messages and connection-fatal
conditions belong to the [normative A0 reference](executable-ipc-a0.md).

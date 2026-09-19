+++
type = "adr"
id = "geometer-adr-011"
domain = "geometer"
status = "accepted"
title = "Use Generic Operation Transports"
created = "2026-08-18"
+++

# ADR-011: Generic Operation Transports

## Status

Accepted amendment on 2026-09-18. Independent review approved the exact
normative file content with SHA-256
`8aa8f3a79145695fa8715eb8e717c9b7ab5eb5651c02a36868b0f97cbf1d805b` at
review head `03e0c5c14ce29c38e51ba50b34c4a3020e5762d1`, with no remaining
blockers. Status and review-record edits do not change that normative content.

The original generic transport packet was accepted at review head
`b86a065c5926c35f1eee23a9ba1cef890689c7d7` on 2026-08-12, covering normative
remediation revision `529c768e559b4c88874264748d4186e775c8a4dd`. That evidence
continues to govern the implemented A0 C ABI and IPC framing. This amendment
reopens review because it changes the native C ABI's threading guarantee,
native capability projection, and supported embedding model. Work on the
original transport is tracked by [GitHub issue
#18](https://github.com/wavenumber-eng/geometer/issues/18); the amendment and
static SDK are tracked by [GitHub issue
#38](https://github.com/wavenumber-eng/geometer/issues/38).

## Context

ADR-010 makes TypeSpec and the normalized catalog the structural authority for
individually promoted operations. Generated browser and native clients still
need stable ways to invoke those operations.

The existing browser surface has one or more C functions per operation. It does
not expose model bounds, and adding every future operation would require
handwritten C declarations, C++ wrappers, Emscripten export edits, and direct
pointer code in each consumer. The executable has a file-based batch command
but no persistent binary-safe pipe suitable for a Rust client.

These transports must carry large model/result bytes separately from JSON,
preserve existing interfaces, bound untrusted sizes before allocation, and
avoid claiming concurrency or cancellation behavior that the geometry kernels
have not established.

## Decision

Add two transports over the same generated operation catalog:

1. an additive generic operation/attachment C ABI for native and browser WASM;
   and
2. executable IPC generation `a0`, a persistent framed stdin/stdout protocol.

The exact A0 specifications are:

- [Generic Operation C ABI](../../design/generic-operation-c-abi.md); and
- [Executable IPC A0](../../design/executable-ipc-a0.md).

Both carry generated JSON values plus named raw byte attachments. The C ABI
passes operation identity as a separate argument and accepts the
operation-specific request DTO directly. IPC wraps that same request DTO with
operation and correlation metadata in frame JSON. Both return the same generic
operation-outcome shape. Neither base64-encodes attachments. Operation identity
and allowed attachments come from the normalized catalog.

There is one catalog authority and multiple generated artifact projections.
Authored TypeSpec and its normalized catalog remain authoritative. The portable
projection contains operations compiled for both native and full browser WASM;
the native projection contains the portable operations plus implemented
native-only operations. Native generic C ABI and native executable IPC
artifacts from the same release advertise the identical native projection.
Direct WASM and the browser Worker advertise the identical portable projection.
An artifact must not advertise an operation it cannot dispatch.

The projection difference is capability data, not a second semantic
interface. Wherever an operation is advertised, its identity, request/result
DTOs, attachment declarations, governed diagnostics, and operation limits come
from the same normalized declaration.

### Execution policy

IPC A0 accepts multiple correlated requests but executes geometry operations
one at a time. The server has a bounded queue and may write control outcomes
independently, so clients correlate all frames by request identifier and do not
assume response order.

Cancellation removes queued requests only. An active operation is not
cancelled; the server returns `geometer.transport.not_cancellable` and the
original operation continues. Client timeout is local and does not imply
cancellation. A client may terminate the child as an explicit escalation, which
fails all outstanding requests.

The C ABI call remains synchronous. Native builds add a process-wide supported-
operation execution gate below the public transport adapters. Generic C ABI,
native executable IPC, the embedded stdio server, and maintained native C
compatibility adapters enter that same gate before geometry or topology
execution. At most one supported operation executes in one native process at a
time, even when foreign threads enter through different adapters. Lock
acquisition order across concurrent callers is unspecified; callers that need
mutation order must sequence their requests.

Catalog queries do not perform geometry and do not hold the execution gate.
Access and destruction of distinct completed result handles do not hold it.
Access or destruction of the same result handle must not race. Internal C++
value APIs retain their individual threading contracts and are not made
thread-safe by this decision.

The browser build remains single-lane per Emscripten module instance. The
browser Worker retains its explicit FIFO promise queue. Parallel browser work
uses separate Worker/module instances; parallel contained native work uses
separate processes. This decision does not qualify concurrent OCCT execution
inside one process.

A maintained direct Rust backend uses one bounded FIFO and one dedicated native
executor thread per process-wide runtime, rather than launching an unbounded
set of `spawn_blocking` calls. It may remove work only before execution begins.
Once a call is active, a Rust timeout is observational: the native operation
continues to own the lane until it returns. Multiple direct client objects in a
process share the execution lane and native topology session store. Dropping a
client neither isolates nor invalidates that process-global state. Consumers
requiring hard deadlines, crash containment, or client-private topology
lifetime use a process backend.

`geometer_serve_stdio()` is a native C bootstrap for the existing IPC adapter,
not another operation API and not an in-process background service. It may be
called only as the main action of a dedicated worker child, before unrelated
application initialization or threads, with exclusive ownership of stdin,
stdout, and stderr. Protocol-fatal and deadline paths may terminate that child
without stack unwinding. A parent application must not invoke it on a library
thread. A self-hosted application selects its hidden worker command before
normal application startup and then calls this bootstrap.

### Compatibility

Existing per-operation C ABI symbols, the file-based `run` CLI, command aliases,
and the executable-backed Python path remain supported. They become explicit
adapters to promoted contracts where practical and are retired only through a
separate compatibility decision.

The native generic C ABI changes from the portable capability projection to the
native capability projection when this amendment ships. That catalog and
threading change advances the date-based C ABI generation under ADR-006 and
regenerates the catalog digest. It is not backported under an existing C ABI
generation. Native static clients and native IPC clients validate the new
release, C ABI generation, projection, and catalog digest before use.

The executable's stdout is exclusively protocol frames in stdio server mode;
stderr is exclusively logs. Windows stdin and stdout are switched to binary
mode before the handshake.

### Review gate

The original independent review gate was satisfied on 2026-08-12 with no
blocking findings. The promotion manifest records the reviewed revision,
review head, reviewer identity, date, and artifact digests.

The 2026-09-18 amendment review approved the native/portable catalog
projections, common dispatcher gate, direct Rust executor lifecycle,
process-terminal embedded server contract, transport-local cancellation model,
and C ABI generation change. The reviewed file hash and review head are
recorded in Status. Any later normative change to this ADR or either transport
specification returns the gate to pending again.

## Consequences

- New catalog operations can become browser-callable without a bespoke C entry
  point.
- TypeScript and Rust clients can share operation identities, envelopes,
  attachments, diagnostics, and capability data.
- The opaque C result handle centralizes ownership and avoids exposing output
  allocation layouts to JavaScript.
- IPC request concurrency is a client/API concept in A0, not parallel geometry
  execution.
- A0 deliberately leaves active cancellation and parallel OCCT execution for a
  later evidenced protocol generation or capability.
- Executable IPC and a self-hosted process worker can cancel queued work. The
  browser Worker A0 protocol has no per-request cancellation message; hard
  termination rejects all of its outstanding work. Direct native execution
  cannot cancel an active call.
- Exact cross-transport parity applies to accepted, well-formed operation
  invocations and governed operation outcomes where the target projections
  overlap. Pointer, frame, queue, process, cancellation, and termination
  failures are transport-local. Effective framing, queue, aggregate-byte, and
  lifecycle limits may be stricter than the common operation declaration and
  must be reported honestly by each adapter.
- The transport implementations require adversarial limits, framing, race,
  shutdown, broken-pipe, and ownership tests in addition to operation tests.

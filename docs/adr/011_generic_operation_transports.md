# ADR-011: Generic Operation Transports

## Status

Accepted. Independent review approved the generic transport packet at review
head `b86a065c5926c35f1eee23a9ba1cef890689c7d7` on 2026-08-12, covering
normative remediation revision `529c768e559b4c88874264748d4186e775c8a4dd`.
Work is tracked by [GitHub issue
#18](https://github.com/wavenumber-eng/geometer/issues/18).

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

- [Generic Operation C ABI](../design/generic-operation-c-abi.md); and
- [Executable IPC A0](../design/executable-ipc-a0.md).

Both carry generated JSON values plus named raw byte attachments. The C ABI
passes operation identity as a separate argument and accepts the
operation-specific request DTO directly. IPC wraps that same request DTO with
operation and correlation metadata in frame JSON. Both return the same generic
operation-outcome shape. Neither base64-encodes attachments. Operation identity
and allowed attachments come from the normalized catalog.

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

The C ABI call is synchronous and is not specified as reentrant or thread-safe.
A host or generated adapter serializes catalog and execute calls until a later
concurrency decision says otherwise.

### Compatibility

Existing per-operation C ABI symbols, the file-based `run` CLI, command aliases,
and the executable-backed Python path remain supported. They become explicit
adapters to promoted contracts where practical and are retired only through a
separate compatibility decision.

The executable's stdout is exclusively protocol frames in stdio server mode;
stderr is exclusively logs. Windows stdin and stdout are switched to binary
mode before the handshake.

### Review gate

The independent review gate was satisfied on 2026-08-12 with no blocking
findings. The promotion manifest records the reviewed revision, review head,
reviewer identity, date, and artifact digests. Any later normative change to
this ADR or either transport specification returns the gate to pending before
implementation continues.

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
- The transport implementations require adversarial limits, framing, race,
  shutdown, broken-pipe, and ownership tests in addition to operation tests.

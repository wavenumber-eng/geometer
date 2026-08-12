# Executable IPC A0

## Status and command

This is the proposed first executable-pipe contract reviewed under ADR-011.
The server command is:

```powershell
geometer serve --stdio
```

IPC identity `a0` is independent of the release version, date-based C ABI
generation, TypeSpec contract identities, and packed planar format versions.

stdin and stdout carry frames only. stderr carries logs only. On Windows the
server switches stdin and stdout to binary mode before reading the first byte.

## Byte order and frame header

All integer fields are unsigned little-endian. Every frame begins with this
48-byte header:

| Offset | Size | Field | A0 value/meaning |
| ---: | ---: | --- | --- |
| 0 | 8 | magic | ASCII `GMIPCA01` |
| 8 | 2 | header size | `48` |
| 10 | 2 | protocol generation | `0` for `a0` |
| 12 | 2 | frame kind | table below |
| 14 | 2 | flags | `0` |
| 16 | 8 | request id | client-selected correlation id; `0` only where stated |
| 24 | 4 | JSON size | byte length of the UTF-8 JSON section |
| 28 | 4 | attachment count | number of following attachment sections |
| 32 | 8 | attachment bytes | total bytes of attachment headers, names, media types, and data |
| 40 | 4 | reserved 0 | `0` |
| 44 | 4 | reserved 1 | `0` |

The JSON section immediately follows the frame header. Attachment sections
follow the JSON section. No padding or trailing frame bytes are allowed.

Frame kinds are:

| Value | Kind | Direction | Request id |
| ---: | --- | --- | --- |
| 1 | `hello` | client to server | `0` |
| 2 | `welcome` | server to client | `0` |
| 3 | `request` | client to server | nonzero and not outstanding |
| 4 | `response` | server to client | matching request |
| 5 | `cancel` | client to server | target request |
| 6 | `cancelled` | server to client | target request |
| 7 | `cancel_rejected` | server to client | target request |
| 8 | `shutdown` | client to server | `0` |
| 9 | `shutdown_ack` | server to client | `0` |
| 10 | `protocol_error` | server to client when safely writable | offending id or `0` |

Only `request` and `response` frames may contain attachments in A0. Control
frames require `attachment_count == 0` and `attachment_bytes == 0`.

## Attachment section

Each attachment starts with a 16-byte header:

| Offset | Size | Field |
| ---: | ---: | --- |
| 0 | 2 | UTF-8 name size |
| 2 | 2 | UTF-8 media type size |
| 4 | 4 | flags, zero in A0 |
| 8 | 8 | data size |

The name bytes, media-type bytes, and raw data bytes follow immediately. Names
must be nonempty, unique within a frame, and declared for the operation.
Media types must be nonempty and match the catalog declaration. Names and media
types are not NUL terminated.

## Handshake

The client sends exactly one `hello` frame before any other frame. Its generated
JSON contains client name/version, supported protocol identities, and optional
client capabilities. It has request id zero and no attachments.

The server replies with exactly one `welcome` frame selecting `a0`. Its
generated JSON contains:

- Geometer release version and C ABI generation;
- selected IPC identity `a0`;
- normalized operation-catalog digest;
- supported operation identities and attachment declarations;
- effective size/queue limits; and
- server capabilities.

No common protocol, invalid first frame, or a second handshake is fatal. The
server writes a protocol error when it can do so safely, then closes stdout and
exits nonzero.

## Requests and responses

A request id is an unsigned 64-bit nonzero value selected by the client. It
must not already be outstanding. Reuse is allowed only after its terminal
response or cancellation has been received.

The request JSON is a generated generic envelope containing the operation
identity and its operation-specific request value. Raw attachment sections are
matched by name against the envelope and catalog. The response JSON contains
the same operation identity, `ok`, the operation-specific result when
successful, and governed diagnostics when unsuccessful. Response attachments
are declared by both the envelope and catalog.

Malformed strict JSON, an unknown operation, attachment mismatch, or operation
failure produces a correlated typed response when the frame itself was safely
decoded. Corrupt framing that prevents correlation produces `protocol_error`
when safe and otherwise closes the pipe.

Responses may arrive out of request order. A0 geometry execution is serialized,
so ordinary operation responses are normally in execution order, but clients
must route exclusively by request id.

## Limits and allocation

The server validates the fixed header before allocating or reading variable
payloads. A0 hard maxima are:

| Item | A0 maximum |
| --- | ---: |
| JSON section | 8 MiB |
| Attachments per frame | 16 |
| Attachment name | 128 bytes |
| Media type | 128 bytes |
| Individual attachment | 256 MiB |
| Complete frame | 512 MiB |
| Queued request count | 8 |
| Aggregate queued bytes | 512 MiB |

The `welcome` frame may advertise smaller effective values. The client obeys
the smaller values. All additions and conversions use overflow-safe arithmetic.

If a fixed header declares an impossible or oversized frame, the server does
not drain or allocate the claimed payload. It emits a protocol error if safe and
terminates, because resynchronization on an untrusted length is not reliable.
Truncation, unexpected EOF within a frame, invalid UTF-8, nonzero reserved
fields, unsupported flags, duplicate names, and trailing bytes are errors.

## Execution, queueing, and writes

One worker executes at most one geometry operation at a time. The reader may
validate and enqueue complete requests within the count and aggregate-byte
limits. Queue saturation returns a correlated retryable
`geometer.transport.queue_full` response; it never grows the queue.

One writer owns stdout. All server producers submit complete logical frames to
that writer. It writes each header, JSON, and attachments without interleaving
another frame, handles partial writes, and flushes after each complete frame.
This serialization is the A0 atomic-frame guarantee; it does not assume an
operating-system pipe write is atomically large enough for the frame.

Broken stdin stops acceptance and begins shutdown. Broken stdout or an
unrecoverable partial write is fatal because correlated outcomes can no longer
be delivered. stderr logging must never write protocol bytes.

## Cancellation and timeout

A `cancel` frame uses the target operation's request id and has a generated
control JSON body with an optional human reason.

- If the request is queued, the server removes it and emits `cancelled`. No
  later operation response is emitted.
- If the request is active, the server emits `cancel_rejected` with
  `geometer.transport.not_cancellable`; the original response still follows.
- If the id is unknown or already terminal, the server emits
  `cancel_rejected` with `geometer.transport.unknown_request`.

Queue removal and transition to active execution share one synchronization
boundary, so exactly one queued-cancel or active-rejection outcome wins the
race.

A client timeout is local. It may send `cancel`, continue draining the eventual
frame, or explicitly terminate the process. Timeout alone never asserts that
server execution stopped. Process termination fails all outstanding client
operations.

## Shutdown

After `shutdown`, valid stdin EOF, or parent-requested graceful termination, the
server:

1. stops accepting operation requests;
2. emits terminal shutdown diagnostics for queued requests;
3. lets the active request finish for a default 30-second grace period;
4. emits its terminal response and `shutdown_ack`, then flushes and exits zero;
   or
5. if grace expires, logs the condition and terminates the process nonzero,
   closing the pipe so clients fail all outstanding work.

An explicit forced child termination is a client escalation, not an A0 control
frame. The Rust client first attempts graceful shutdown and exposes a separate
forced termination helper.

## Diagnostics and logging

Protocol and request diagnostics follow `contract-semantics.md`. Exact message
text is nonbinding. Codes, category, retryability, JSON Pointer path, operation,
and request id are governed fields.

Logs include request ids and operation identities where available, but never
raw model bytes, complete request JSON, secrets, or protocol frames.


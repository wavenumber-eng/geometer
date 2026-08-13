# Rust Contracts And Executable IPC Client

## Current status

`src/rust/geometer-client` is the Tokio-based `geometer-client` crate. Its
operation DTOs and strict Serde codecs are generated from the normalized
TypeSpec catalog. The first live vertical spawns one persistent native
`geometer(.exe) serve --stdio` process and executes
`geometry.model_bounds.a0` with raw STEP bytes.

The executable implementation preserves every file-oriented CLI command. Its
stdin and stdout are switched to binary mode on Windows; stdout is reserved for
complete A0 frames and stderr is captured separately by the client.

The TypeSpec source now owns the strict hello, welcome, request, reason,
cancelled, cancel-rejected, protocol-error, shutdown-ack, and embedded
operation-catalog shapes. Generated C++ and Rust codecs are used by both ends
of the live connection. The broader plan step remains open for independent
review and hosted Windows, Linux x64, Linux ARM64, and macOS ARM64 evidence.
The implemented pilot is additive and reviewable; it is not yet a release
claim.

The crate carries a local `wn-dev-std` 2026.8.12 Rust profile. Its stable
toolchain components, denied Rust/Clippy lints, declared Cargo signoff commands,
and strict Tree-sitter hygiene policy are enforced from the repository Rust
Rack stratum.

## Generated contracts

`scripts/generate-rust-contracts.mjs` reads the same normalized catalog as the
C++ and TypeScript generators and writes
`src/rust/geometer-client/src/generated`. Generated operation and IPC structs use
`deny_unknown_fields`; direct deserialization rejects duplicate keys and
trailing data; validation rejects non-finite/range-invalid numbers and contract
literals. Optional fields use a presence-aware deserializer so an absent value
maps to `None` while explicit JSON `null` is rejected.

All 20 governed contract vectors replay through this projection. Generation is
part of `npm run generate:contracts` and freshness is part of
`npm run check:contracts`.

## Client lifecycle

`GeometerClient::spawn()` starts an explicit executable path, sends `hello`,
and requires a `welcome` selecting `a0`, the exact normalized-catalog digest,
all required capabilities, and effective limits no larger than the A0 maxima.
`discover_and_spawn()` additionally checks `GEOMETER_EXECUTABLE`, an executable
sibling, and the source checkout's grouped native distribution path.

Calls receive monotonically increasing nonzero `u64` request identifiers. One
reader task routes responses exclusively by identifier and verifies that the
generated outcome operation matches the pending operation. Unknown,
completed, malformed, or wrong-direction frames fail the connection and every
pending call.

`OperationCall::cancel()` requests queue-only cancellation. `wait_timeout()` is
a local timeout: it sends a cancellation request and reports whether the server
removed queued work; it never claims an active OCCT operation stopped. The
client continues draining any active terminal response. `close()` performs the
30-second graceful protocol shutdown, while `terminate()` is the explicit
forced-child escalation. Captured stderr is available through `stderr_text()`.

## Initial wire projection

The frame layout and limits are exactly those in
[Executable IPC A0](executable-ipc-a0.md). The pilot uses these strict JSON
objects inside frames:

| Frame | JSON projection |
| --- | --- |
| `hello` | `client_name`, `client_version`, `protocols`, optional `capabilities` |
| `welcome` | release/C ABI/IPC identities, catalog digest and operation catalog, effective limits, capabilities |
| `request` | `operation` and the operation-specific `request` object |
| `response` | generated `OperationOutcomeA0` |
| `cancel`, `shutdown` | optional `reason` |
| `cancelled` | `status: "cancelled"` |
| `cancel_rejected`, `protocol_error` | status plus a transport diagnostic |
| `shutdown_ack` | the three fields governed by the A0 design |

Raw attachments follow the JSON section with their governed name, media type,
and data lengths. They are never base64 encoded.

## Verification

The Rust Rack stratum checks formatting, Clippy with warnings denied, all
contract vectors, exact shutdown bytes in both C++ and Rust, binary attachment
round trips, fixed-header rejection before payload allocation, strict malformed
request recovery, unknown and queued cancellation, local timeout behavior,
duplicate-attachment correlation, wrong-direction fatal handling,
close/request race resolution, repeated real STEP work through one child,
graceful shutdown, and compilation from a clean packaged-crate consumer.
The failure-path matrix additionally covers incompatible negotiation,
oversized fixed headers before payload reads, broken stdout, and explicit
forced termination with pending requests. The latter closed a reader/terminator
race that could otherwise leave a pending call unresolved. Native and release
CI run these live Rust tests on every supported native platform after building
that platform's executable.

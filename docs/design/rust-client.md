# Rust Contracts And Executable IPC Client

## Current status

`src/rust/geometer-client` is the Tokio-based `geometer-client` crate. Its
operation DTOs and strict Serde codecs are generated from the normalized
TypeSpec catalog. The live client spawns one persistent native
`geometer(.exe) serve --stdio` process and executes model bounds, model/mesh
HLR, and analytic planar Boolean through their governed attachments.

Feature builds additionally provide typed `model_tessellation` and
`mesh_illustration` methods using generated A0 values. The complete
[native STEP-to-SVG example](mesh-illustration-native.md) needs no JS/WASM runtime
or handwritten subprocess protocol. These additions require a matching feature
executable and are not available in the previous release.

The analytic operation is experimental and not production-ready. It may fail
closed on valid inputs and is not the dependable path for whole-board or
whole-layer copper union. Prefer the Clipper2-backed planar APIs when
polygonized output is suitable.

The executable implementation preserves every file-oriented CLI command. Its
stdin and stdout are switched to binary mode on Windows; stdout is reserved for
complete A0 frames and stderr is captured separately by the client.

The TypeSpec source owns the strict hello, welcome, request, reason,
cancelled, cancel-rejected, protocol-error, shutdown-ack, and embedded
operation-catalog shapes. Generated C++ and Rust codecs are used by both ends
of the live connection, including the production request envelope. Model and
mesh HLR are implemented typed surfaces. Release promotion remains separately
gated on independent review, demo acceptance, hosted Windows, Linux x64, Linux
ARM64, and macOS ARM64 evidence.

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

All governed contract vectors replay through this projection. Generation is
part of `npm run generate:contracts` and freshness is part of
`npm run check:contracts`.

The generated operation catalog is also the Rust client's negotiation
authority. Packed request/result roots are deliberately excluded from generated
JSON codecs: their logical DTOs remain generated, while
`analytic_packet_a0` owns the strict little-endian wire codec. The public
encoder accepts `AnalyticPlanarBooleanBatchRequestA0`; the public decoder
returns `AnalyticPlanarBooleanBatchResultA0` and computes each job's SHA-256
over its exact rebased standalone packet closure.

Logical request decoding and request/result identity matching are generated in
`generated/dispatch.rs` from the TypeSpec IPC unions and catalog roots. The
generic client selects the exact negotiated request contract before decoding;
it does not guess from union order (bounds and HLR can both accept `{}`). This
also allows generic `execute` calls for advertised native topology operations.
Their experimental lifecycle is unchanged; generated support is not production
promotion. New logical operations must join the authored TypeSpec unions and
catalog, then regenerate all bindings, rather than add handwritten Rust
dispatch cases. Attachment and runtime-availability checks still use the
negotiated operation declaration; packed packet dispatch stays separate.

The feature-build `model_tessellation()` facade returns colored millimeter
meshes through a generated attachment contract. See
[model tessellation A0](model-tessellation-a0.md) for defaults, limits and current
release-qualification status. It requires a matching feature executable; it is
not present in the released 2026.9.4 binary.

## Client lifecycle

`GeometerClient::find_executable()` resolves the same local path as
`discover_and_spawn()` without launching it, so desktop consumers can show or
override the selected executable. Windows spawn uses `CREATE_NO_WINDOW` while
retaining piped stdin/stdout/stderr. These process-support helpers are not new
TypeSpec geometry operations. The optional
[Rust/wgpu demo](../../examples/rust/native_viewer/README.md) demonstrates the
typed native APIs without adding GUI dependencies to this crate.

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

`GeometerClient::analytic_planar_boolean_batch()` derives attachment names,
media types, schema identities, and packet format from the negotiated generated
catalog. Generic request and response validation rejects missing, duplicate,
undeclared, oversized, or media-incompatible attachments and projection
metadata drift. Fatal response ID/kind/JSON/attachment violations poison the
connection and resolve every pending call. Negotiated response frame limits are
checked before payload allocation.

`GeometerClient::model_hlr_projection()` accepts STEP bytes and canonical HLR
options. `mesh_hlr_projection()` accepts an indexed-mesh A0 packet;
`MeshHlrProjectionRequest::from_mesh()` encodes a structured mesh. The model
operation retains the `poly` default, while omitted mesh selectors choose the
only applicable Fast detail and Fast mesh-shadow paths.

`OperationCall::cancel()` requests queue-only cancellation. `wait_timeout()` is
a local timeout: it sends a cancellation request and reports whether the server
removed queued work; it never claims an active OCCT operation stopped. The
client continues draining any active terminal response. `close()` performs the
30-second graceful protocol shutdown, while `terminate()` is the explicit
forced-child escalation. Captured stderr is available through `stderr_text()`.

## Wire projection

The frame layout and limits are exactly those in
[Executable IPC A0](executable-ipc-a0.md). The client uses these strict JSON
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
repeated nonempty typed analytic work through one child, native-produced packet
corpus parity, mutation rejection, normative standalone job digests, graceful
shutdown, and empty plus nontrivial friendly analytic IPC calls from a clean
packaged-crate consumer against the platform executable in
`dist/native/<platform>/`.
The failure-path matrix additionally covers incompatible negotiation,
oversized fixed headers before payload reads, broken stdout, explicit forced
termination with pending requests, the server shutdown deadline, and an
independently unexpected child exit. The forced-termination case closed a
reader/terminator race that could otherwise leave a pending call unresolved.
The deadline and unexpected-exit cases use non-distributed native test servers
with deterministic timing/failure injection; the production server retains its
governed 30-second deadline. Native and release CI run these live Rust tests on
every supported native platform after building that platform's executable.

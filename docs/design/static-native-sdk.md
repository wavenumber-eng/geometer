# Static Native SDK

## Status

Proposed for the release governed by Geometer issue 38. Build-tree static
archives are not a supported SDK until the package, external-consumer,
attestation, release-candidate, and downstream qualification gates in this
document pass.

## Supported boundary

The SDK exposes the Geometer C ABI only. Its operation-growth surface consists
of:

- release and date-based C ABI generation queries;
- the normalized operation catalog query;
- generic operation execution using request JSON and named byte attachments;
- opaque operation-result access and destruction;
- Geometer-owned string destruction; and
- the native worker-process `geometer_serve_stdio()` bootstrap.

The focused C functions already declared in `c_api.h` retain their existing
compatibility status, but new operations are added only through the generic
operation ABI. Installing that header does not promote the C++ or OCCT ABI.

Geometer C++ APIs and OCCT headers, classes, symbols, allocation, and ABI are
implementation details. Static archives necessarily contain implementation
symbols, but their presence does not make them supported consumer APIs.

## One operation model

TypeSpec and the normalized operation catalog remain the structural authority.
Native C ABI, native executable IPC, direct browser WASM, and browser Worker
adapters share operation identities, request/result DTOs, attachment
declarations, and governed operation outcomes wherever an operation is
advertised.

The catalog has generated artifact projections:

- native static C ABI and native IPC advertise the identical native operation
  projection and canonical digest; and
- direct WASM and browser Worker advertise the portable projection.

Transport limits and local failures are not operation contracts. Invalid
pointers, malformed frames, queue saturation, process exit, Worker termination,
and cancellation may differ by adapter. Cross-transport equality applies to
well-formed accepted operation requests and their governed outcomes.

## Concurrency and lifecycle

One native process owns one supported-operation execution lane. The common
native dispatcher serializes geometry and topology execution below every
supported native adapter. Catalog reads and access to distinct completed result
handles do not acquire the execution gate. Acquisition order between concurrent
callers is unspecified. Retained focused native C operations acquire the same
lane at their operation boundary; metadata/version queries and owned-result
destruction remain outside it.

Direct C ABI execution is synchronous and cannot be force-cancelled. The Rust
direct backend uses one dedicated executor thread and a bounded FIFO queue.
Queue saturation fails locally. Dropping a client or timing out a future does
not remove queued work; timeout and future abandonment are observational, and
the executor continues until it can release all result/error resources.

`geometer_serve_stdio()` is a worker-process main entry point. It requires a
dedicated child with exclusive stdin/stdout/stderr ownership and must run before
unrelated application initialization or threads. Protocol-fatal and deadline
paths may terminate the child without unwinding. It is not an in-process
background-service API.

Its production process-status contract is `0` for graceful completion, `2` for
bootstrap, protocol, I/O, or caught-exception failure, and `124` for the hard
shutdown deadline. Any other status or signal describes parent or operating-
system termination, not a status returned by Geometer.

Parallel native geometry uses a process pool in the first SDK generation.
Parallel browser geometry uses separate Worker/module instances. Parallel OCCT
execution inside one runtime instance is not supported.

## Platform profiles

Each SDK declares exactly one compatible ABI profile:

| Platform | Profile |
| --- | --- |
| Windows x64 | `x86_64-pc-windows-msvc`, MSVC v143, Release, `/MT`; Rust requires `crt-static` |
| macOS ARM64 | Apple Clang and libc++, ARM64, macOS 11.0 minimum |
| Linux x64 | Qualified GCC major/libstdc++ ABI, x86-64, glibc 2.35 minimum |
| Linux ARM64 | Qualified GCC major/libstdc++ ABI, AArch64, glibc 2.35 minimum |

Windows SDK OCCT objects use a separate build/install/cache profile from the
normal `/MD` executable and wheel build. Target, architecture, compiler family,
runtime, or platform-baseline mismatch is a hard consumer error.

## Package layout

The logical archive layout is:

```text
include/geometer/c_api.h
lib/geometer.lib                 # Windows
lib/libgeometer.a                # Unix
lib/occt/<required archives>
lib/cmake/Geometer/GeometerConfig.cmake
lib/cmake/Geometer/GeometerConfigVersion.cmake
lib/cmake/Geometer/GeometerTargets.cmake
rust/geometer-sys/Cargo.toml
rust/geometer-sys/build.rs
rust/geometer-sys/src/lib.rs
share/geometer/geometer-sdk.json
share/geometer/geometer-sdk.schema.json
share/geometer/geometer-sdk-payload.json
share/geometer/geometer-sdk-payload.schema.json
share/geometer/geometer-sdk-attestation.json
licenses/...
```

The CMake package exports the relocatable `Geometer::c_api_static` target. OCCT
and other implementation include directories remain private. Package-relative
link-only imported targets carry the exact static dependency closure without
requiring OCCT headers in a C ABI consumer.

The packaged `geometer-sys` crate is the only reviewed Rust unsafe FFI owner.
Its build script requires an explicit `GEOMETER_SDK_DIR`, validates the Cargo
target and SDK release/profile, requires `crt-static` for Windows, and emits the
ordered archive/system-library/framework closure from the SDK manifest. It
does not download dependencies or SDK artifacts. Its safe ownership helpers
copy output bytes into Rust values and release every Geometer-owned handle;
`geometer-client`'s optional `direct-static` feature builds the shared
catalog/contract validation and bounded executor on top. It also re-exports a
safe `serve_stdio()` bootstrap so a downstream executable can use itself as
the ordinary IPC worker and retain process-pool parallelism while shipping one
file.

Release qualification also builds the Rust
[`direct_static_illustration`](../../src/rust/geometer-client/examples/direct_static_illustration.rs)
sample against the relocated SDK, copies only its application binary into an
unrelated package directory, clears Geometer executable discovery and the
normal executable search path, and runs STEP-to-SVG illustration. The produced
app must have no dynamic Geometer or OCCT imports and no bundled
`geometer(.exe)` fallback.

The versioned SDK JSON schema records target/profile identity, ordered archive
entries, rescan/group boundaries, system libraries, Apple frameworks, and the
minimum platform baseline. Cargo consumers validate this manifest using
Cargo's `TARGET` and applicable target features/runtime profile before emitting
link directives. Windows also requires `CARGO_CFG_TARGET_FEATURE` to contain
`crt-static`. Cargo build scripts never download the SDK.

## Integrity and provenance

The internal canonical payload inventory lists every payload member name, size,
and SHA-256 except the inventory itself and binds the release, C ABI generation,
catalog digest, toolchain, runtime, dependency profile, schemas, and link
manifest. The archive member set must equal the inventory member plus exactly
its declared payload. Undeclared members fail validation.

An external release sidecar binds the final SDK archive SHA-256, Git tag, source
revision, workflow/run identity, and internal-manifest digest. GitHub build
provenance and detached checksums accompany the sidecar. An unsigned checksum
alone is not provenance.

SDK packing uses sorted paths and canonical metadata. Two clean builds of the
same source/toolchain profile must produce identical archive digests. Producers
must normalize or eliminate nondeterministic inputs; run-specific provenance
stays outside the deterministic archive.

## Static-link compliance

The SDK includes Geometer and third-party licenses and notices, exact OCCT
source identity and checksum, modification status, build scripts, and durable
corresponding-source access or a compliant source offer. The reviewed release
posture also documents downstream static-link/relink obligations and any
commercial-licensing alternative. Build metadata does not determine legal
compliance by itself.

## Release lifecycle

SDK archives are external GitHub Release assets and are not committed under
`dist/`. Runtime archives continue to reject `.lib` and `.a` members.

1. The four-platform matrix builds deterministic candidate archives and their
   provenance.
2. Clean relocated CMake and Cargo consumers validate the exact candidate
   bytes, including a direct operation, the embedded stdio server, and the
   internal direct-static clipping qualification sample. The sample is not a
   release asset; the existing browser and egui Labs are the maintained demos.
3. KiCad Cruncher consumes those same bytes for Windows x64 and macOS ARM64
   one/four-worker Toon qualification with no Geometer sidecar.
4. Full review and release signoff approve an exact expected asset inventory.
5. The candidate bytes are published immutably without `--clobber`.
6. CI downloads the public assets and repeats inventory, provenance, and clean
   consumer verification.

Issue 38 closes only after the downloaded public SDK—not a build-tree archive
or rebuilt candidate—passes these gates.

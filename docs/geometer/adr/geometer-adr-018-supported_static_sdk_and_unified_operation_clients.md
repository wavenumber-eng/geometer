+++
type = "adr"
id = "geometer-adr-018"
domain = "geometer"
status = "accepted"
title = "Ship A Supported Static SDK And Unified Operation Clients"
created = "2026-09-18"
+++

# ADR-018: Ship A Supported Static SDK And Unified Operation Clients

## Status

Accepted on 2026-09-18. Independent review approved the exact normative file
content with SHA-256
`96078714432f4cfbc60fa821d07106acdc0f14cb39146e0d0f359ee76722305d` at
review head `03e0c5c14ce29c38e51ba50b34c4a3020e5762d1`, with no remaining
blockers. Status and review-record edits do not change that normative content.

This decision implements the future-SDK conditions recorded by ADR-015 and
depends on the accepted 2026-09-18 ADR-011 amendment.
Work is tracked by [GitHub issue
#38](https://github.com/wavenumber-eng/geometer/issues/38).

## Context

Geometer already builds a static C++ library, but that build-tree archive is not
a supported SDK. It has no installed public-header set, relocatable CMake
target, non-CMake native link manifest, complete static OCCT closure, consumer
toolchain profile, SDK attestation, or external-consumer release test. ADR-015
therefore correctly classifies native static libraries as generated build
outputs and excludes them from native runtime archives.

Rust applications currently use the persistent executable IPC client. That
boundary provides bounded queues, hard process replacement, crash containment,
and process-level parallelism, but packaging a second executable is undesirable
for desktop applications. A statically linked application should be able to
use the same Geometer operations directly or spawn copies of itself as
contained stdio workers without exposing OCCT or creating a second typed API.

## Decision

### Supported boundary

Ship a separately named and versioned native static **C ABI SDK** in every
supported native release. The SDK supports the generic catalog-governed
operation ABI. It does not support the Geometer C++ ABI or the OCCT ABI.

The semantic interface remains operation identity plus generated request JSON,
named byte attachments, a governed outcome, and named result attachments. The
native static C ABI, browser WASM C ABI, browser Worker, executable IPC, and
self-hosted IPC are transport adapters over that interface. Generated Rust,
TypeScript, and Python conveniences must share operation declarations and
codecs rather than fork operation semantics.

The SDK installs only Geometer-owned public C headers. OCCT headers, objects,
handles, and ownership rules do not cross the supported boundary. Static OCCT
archives remain private implementation dependencies included only to complete
the final native link.

### SDK contents and profiles

Each SDK artifact contains:

- the supported public C headers;
- the Geometer static archive and exact required static OCCT archive closure;
- a relocatable exported `Geometer::c_api_static` CMake target;
- a generated, ordered native-link manifest for Cargo and other non-CMake
  consumers, including system libraries, frameworks, link groups, and options;
- a canonical SDK manifest and attestation covering every packaged file,
  catalog digest, release, date-based C ABI generation, source revision, target
  triple, architecture, compiler family/version, C and C++ runtime profile,
  deployment baseline, dependency profile, and license; and
- all required Geometer and third-party notices and licenses.

The supported matrix and initial ABI profiles are:

- Windows x64: `x86_64-pc-windows-msvc`, MSVC v143 Release, static
  `MultiThreaded` (`/MT`) CRT, and Rust `crt-static`;
- Linux x64: `x86_64-unknown-linux-gnu`, the release-qualified GCC/libstdc++
  profile, and a glibc 2.35 minimum;
- Linux ARM64: `aarch64-unknown-linux-gnu`, the release-qualified
  GCC/libstdc++ profile, and a glibc 2.35 minimum; and
- macOS ARM64: `aarch64-apple-darwin`, the release-qualified Apple
  Clang/libc++ profile, and a macOS 11.0 minimum deployment target.

Each target publishes one explicitly named and attested supported profile per
release. A different compiler ABI, Windows CRT selection, architecture,
deployment baseline, or OCCT profile is a different SDK profile and is
unsupported unless separately built, attested, and qualified. The Windows
static-CRT OCCT build has a distinct recipe, install path, marker, and cache key
and must never restore `/MD` objects. Intel macOS and Universal 2 remain new
targets.

The SDK does not claim that a final executable has no dynamic dependencies.
Normal operating-system libraries and platform frameworks remain dynamic. On
Windows, the selected MSVC CRT profile must match every Rust, Geometer, OCCT,
and other native object in the final link; the release attestation makes that
selection explicit rather than inferring it from an archive filename.

### Rust client and process embedding

Rust exposes one maintained typed operation facade. Process-backed and direct
static execution are internal backend choices with transport capabilities, not
separate generated operation clients. Unsafe declarations and raw ownership
live in one narrow `geometer-sys` crate; safe clients retain their unsafe-code
prohibition. Its build script consumes an explicit verified SDK location and
does not download release assets during `cargo build`.

The direct backend follows ADR-011's bounded dedicated executor and process-
wide serialization contract. It does not promise active cancellation, crash
containment, memory containment, or independent topology-session lifetime.

The SDK also exports the process-terminal `geometer_serve_stdio()` bootstrap
specified by ADR-011. A statically linked application may expose a hidden
worker command and spawn its own executable. The child then hosts the unchanged
executable IPC protocol, preserving process pools, queue-only cancellation,
hard replacement, and crash isolation while allowing distribution of one
application binary. The bootstrap is not a second operation interface.

### Release-candidate qualification and promotion

Static SDK archives are GitHub Release assets, not committed repository files
and not members of native runtime archives. Runtime archives continue to reject
`.lib` and `.a` files. SDK asset names distinguish release, target, and profile
from runtime assets.

A release candidate is promotable only when all of the following pass from the
downloaded candidate assets, outside the Geometer source and build trees:

1. Every supported target has an SDK archive, checksum, canonical manifest,
   internal attestation, external archive-binding sidecar, build provenance,
   and complete license set.
2. The archive relocates and builds clean external C, C++, and Cargo consumers.
3. Consumers query and validate the expected catalog, execute representative
   byte-in/byte-out operations, and start the embedded stdio server.
4. The native static catalog and executable IPC welcome catalog have the same
   canonical native projection and digest.
5. Platform inspection finds no Geometer or OCCT shared-library dependency and
   confirms the declared CRT, system-library, architecture, and deployment
   profile.
6. Windows x64 and macOS ARM64 downstream trials statically link KiCad Cruncher,
   execute representative Toon work with one and four self-hosted workers,
   preserve failure replacement and output parity, and package no sibling
   Geometer executable.
7. Performance, memory, final-binary size, and SDK-size evidence satisfies the
   reviewed promotion budgets.
8. Static-link licensing evidence identifies the exact OCCT corresponding
   source, modifications and build scripts, provides durable source access or
   the required offer, and documents downstream relink obligations.

Promotion publishes the already-qualified bytes to the dated GitHub Release;
the release job does not rebuild different SDK bytes after qualification. The
release fails closed when an asset, checksum, attestation, profile, or
downloaded-asset validation is missing. Large SDK and OCCT archives remain
generated state locally and are never an authoritative Git cache.

## Non-goals

- Do not expose or support Geometer C++ or OCCT headers, types, handles, or ABI.
- Do not create per-operation Rust FFI functions or a second contract catalog.
- Do not create separate typed operation method sets for direct and process
  clients.
- Do not qualify concurrent OCCT execution inside one native process.
- Do not claim hard cancellation, crash containment, or hard memory containment
  for direct in-process calls.
- Do not replace executable IPC where process isolation, hard deadlines, or
  parallel worker pools are required.
- Do not merge SDK archives into runtime archives or restore committed static
  libraries under `dist/`.
- Do not promise a universally dependency-free executable.
- Do not add KiCad-, Altium-, board-, renderer-, or application-specific policy
  to Geometer.

## Consequences

- Every supported native release has both a runtime distribution and a
  separately governed static SDK distribution.
- Applications can ship one binary while retaining the proven executable IPC
  worker model by spawning themselves.
- Direct calls avoid framing and pipe copies but retain JSON/attachment contract
  validation and sacrifice process containment.
- Process-level pools remain the supported native parallelism mechanism until a
  separate OCCT concurrency qualification is accepted.
- Static consumers inherit a larger and more toolchain-sensitive link closure;
  manifests, profiles, attestations, and external-consumer tests become release
  obligations.
- ADR-015 remains correct for ordinary build outputs and runtime archives; this
  ADR establishes the separately named supported SDK that ADR-015 anticipated.

## Review gate

Independent review approved this ADR together with the ADR-011 amendment on
2026-09-18. The reviewed file hash and review head are recorded in Status; the
review reported no remaining blockers. Any normative change to the supported
interface, concurrency boundary, SDK profile model, or release-candidate
promotion rules returns this gate to pending.

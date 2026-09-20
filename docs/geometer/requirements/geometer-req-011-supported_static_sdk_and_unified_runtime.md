+++
type = "requirement"
id = "geometer-req-011"
domain = "geometer"
status = "active"
title = "Supported Static SDK And Unified Runtime"
created = "2026-09-18"
issue_refs = ["wavenumber-eng/geometer#25", "wavenumber-eng/geometer#38"]
plan_refs = ["geometer-static-sdk-unified-transports"]
adr_refs = ["geometer-adr-011", "geometer-adr-015", "geometer-adr-018"]
verification_status = "unverified"
+++

# REQ-011: Supported Static SDK And Unified Runtime

## Summary

Every native Geometer release provides a supported static C ABI SDK without
creating a second operation interface. Native static callers, browser WASM,
Web Workers, executable IPC, self-hosted application workers, and generated
language clients use the same catalog-governed operations, JSON values, and
named byte attachments. OCCT remains a private implementation dependency.

## Requirements

1. Treat authored TypeSpec, the normalized operation catalog, and the governed
   attachment model as the single semantic operation interface. Transports and
   language packages may adapt invocation and ownership, but must not fork
   operation identities, operation-declared validation, diagnostics, limits,
   or governed outcomes. An adapter may impose stricter pointer, framing,
   queue, resident-memory, aggregate-byte, process, or lifecycle limits and
   must report those effective limits honestly.
2. Support only Geometer's C ABI from the native static SDK. Do not publish or
   support Geometer's C++ ABI, OCCT headers, OCCT types, OCCT ownership, or the
   OCCT ABI as downstream interfaces.
3. Keep the generic operation boundary byte-oriented: operation identity and
   request JSON enter as bounded bytes, model and binary inputs use named byte
   attachments, and governed outcome JSON and named byte attachments leave
   through an opaque Geometer-owned result handle.
4. Route the native generic C ABI and executable IPC through one target-aware
   dispatcher. The full browser WASM target must use the same dispatcher for
   its compiled operation subset. Every artifact's catalog must omit operations
   that the artifact cannot execute.
5. Keep focused legacy C functions and CLI commands as compatibility adapters.
   Do not add a per-operation Rust FFI surface or a second static-library
   catalog.
6. Provide `geometer_serve_stdio()` as a native C ABI bootstrap for the
   existing executable IPC server. It is a transport entry point, not a new
   operation interface, and must catch C++ exceptions before they cross the C
   boundary. It is process-terminal: call it only as the main path of a
   dedicated child, before unrelated application initialization or threads,
   with exclusive ownership of standard input, output, and error. Existing
   fatal protocol and deadline paths may terminate the child through `_Exit`
   without stack unwinding. The production bootstrap's process statuses are
   stable: `0` means graceful completion; `2` means bootstrap, protocol, I/O,
   or caught-exception failure; and `124` means the hard shutdown deadline
   expired. Other statuses or signals represent parent/operating-system
   termination, not a returned Geometer status. Tests must assert this mapping.
7. Enforce one serialized geometry-execution lane per loaded Geometer runtime
   instance. Put the gate in the common native dispatcher below the generic C
   ABI, executable IPC, embedded server, and every maintained native operation
   adapter so foreign-thread calls through different boundaries cannot overlap
   geometry dispatch within one process.
8. Keep direct native calls synchronous. A direct-call timeout is observation
   only and must not claim that the active native call was cancelled. Hard
   cancellation and crash containment require terminating and replacing a
   worker process.
9. Support geometry parallelism through multiple process or Worker instances
   until an independently reviewed qualification explicitly permits concurrent
   OCCT execution within one runtime instance.
10. Report serialization, queued cancellation, hard active-operation
    termination, process replacement, and topology-session invalidation as
    transport capabilities. A platform must not advertise hard containment
    that it has not proven, including macOS containment tracked by issue 25.
11. Implement the direct Rust backend as one process-wide bounded FIFO and one
    dedicated executor thread shared by every direct client. Permit removal
    only while a request remains queued. An active timeout or abandoned future
    is observational; execution continues and must free every native result and
    error buffer. Closing or dropping clients must have explicit bounded
    behavior while queued or active work remains, and must not imply an
    independent topology-session lifetime.
12. Advance the date-based C ABI generation when the native catalog projection,
    execution-threading guarantee, or `geometer_serve_stdio()` surface ships;
    regenerate catalog digests and every generated binding/reference together.
13. Publish the SDK separately from native runtime archives. Do not commit SDK
    archives, OCCT archives, or candidate SDK output to Git or place them in
    `dist/native/<platform>/`.
14. Include the public C header, Geometer static archive, exact private OCCT
    archive closure, relocatable CMake package, ordered Cargo native-link
    manifest, SDK manifest and schema, licenses, notices, and internal SDK
    attestation in each platform SDK archive.
15. Export a relocatable `Geometer::c_api_static` CMake target and exact-version
    package configuration. Installed files must contain no source-tree,
    build-tree, `.deps`, runner-home, or producer-specific absolute paths.
16. Make all OCCT, RapidJSON, and Clipper include directories private to the
    build. C ABI consumers must compile without OCCT or other implementation
    headers.
17. Generate the Cargo link manifest from the authoritative CMake link graph.
    It must preserve archive order, rescan/group boundaries, system libraries,
    Apple frameworks, C++ runtime, target triple, compiler/runtime profile, and
    deployment baseline without a handwritten duplicate closure.
18. Provide a narrow `geometer-sys` crate as the sole Rust unsafe FFI owner.
    Its build script must consume an explicit verified SDK location, reject a
    target or runtime mismatch, emit the governed link closure, and never
    download an SDK during `cargo build`.
19. Keep generated Rust contracts, codecs, and ergonomic operation methods in
    one safe layer shared by process and static backends. Safe crates retain
    their unsafe-code prohibition; the exact `geometer-sys` FFI boundary must
    have a reviewed, value-bounded standards exception.
20. Support these four native ABI profiles:
    - `windows-x64`: `x86_64-pc-windows-msvc`, MSVC v143, Release, static
      `MultiThreaded` (`/MT`) CRT, and Rust `crt-static`;
    - `linux-x64`: `x86_64-unknown-linux-gnu`, the release-pinned GCC/libstdc++
      profile, and glibc 2.35 minimum;
    - `linux-arm64`: `aarch64-unknown-linux-gnu`, the release-pinned
      GCC/libstdc++ profile, and glibc 2.35 minimum; and
    - `macos-arm64`: `aarch64-apple-darwin`, the release-pinned Apple
      Clang/libc++ profile, and macOS 11.0 minimum deployment target.
21. Give the Windows static-CRT OCCT build a distinct semantic recipe, install
    path, cache key, and attested profile. Never restore `/MD` OCCT objects into
    the `/MT` SDK, and make Rust SDK consumption fail when `crt-static` is not
    enabled.
22. Record the exact compiler version, C++ runtime and ABI mode, architecture,
    target triple, deployment baseline, OCCT identity, Geometer release and C
    ABI generation in each SDK. Do not claim that ordinary operating-system
    libraries and platform frameworks are statically linked.
23. Make SDK archives deterministic and relocatable. A clean external CMake
    consumer and a clean external Cargo consumer must build and run after the
    SDK is moved to an unrelated path containing spaces on every supported
    platform.
24. Exercise catalog lookup, a direct model operation, model illustration
    geometry, and the embedded stdio server from external consumers. Inspect
    the final executable on each platform and reject shared Geometer or OCCT
    dependencies, a mismatched Windows CRT, or a deployment requirement newer
    than the declared baseline.
25. Put a canonical payload inventory in the SDK archive that binds every
    payload member name, size and SHA-256 except the inventory itself, plus the
    catalog, ABI, link/profile manifest, schema set, toolchain profile, license
    set, and stable build-recipe/tool identity. Workflow run identity remains
    external. Require the archive member set to equal the one inventory member
    plus exactly its declared payload; reject undeclared, duplicate, escaping,
    or non-canonical archive members.
26. Publish an external sidecar that binds the complete final archive SHA-256, internal
    manifest digest, release tag, source revision, and workflow identity.
    Produce and verify GitHub build provenance for each final SDK archive;
    detached checksums alone are not provenance.
27. Treat static-link licensing as a release gate. Ship the exact Geometer and
    third-party licenses, prominent OCCT-use notice, exact OCCT corresponding
    source identity and durable source access or offer, modification status,
    and build scripts. Document downstream LGPL static-link/relink obligations
    and record a reviewed compliance disposition before promotion.
28. Build a release-candidate SDK matrix outside Git and before public release
    promotion. Candidate bytes are temporary CI or draft-release evidence and
    must not become authoritative merely because a build job succeeded.
29. When the SDK interface, supported toolchain profile, packaging, or embedded
    runtime behavior changes, gate promotion on a clean KiCad Cruncher trial
    using the candidate SDK, not Geometer build-tree paths. On Windows x64 and
    macOS ARM64, statically link Geometer, invoke `geometer_serve_stdio()` from
    a hidden self-hosted command, preserve the serial-worker process pool, and
    run representative Toon work with one and four workers. Unrelated releases
    may reference the latest applicable downstream evidence rather than repeat
    the trial.
30. When required by item 29, run the downstream trial with `GEOMETER_EXE` unset, no Geometer on `PATH`,
    and no sibling Geometer executable or shared Geometer/OCCT library. Prove
    that child processes execute the relocated Cruncher binary, compare output
    with the sibling-Geometer baseline, inject one worker failure, and preserve
    required licenses and notices in the application release.
31. Promote exactly the candidate archives that passed external-consumer and
    downstream trials. Verify the complete four-platform archive, checksum,
    sidecar, provenance, license, and source-offer inventory before making the
    dated GitHub Release immutable. Do not repair a final release by silently
    overwriting assets.
32. Download the promoted GitHub Release assets into fresh jobs and repeat
    manifest, checksum, provenance, relocation, link, runtime, capability, and
    dependency inspection. Validation confined to the producer staging tree is
    insufficient.
33. Fail every native release closed when any required SDK profile or evidence
    item is missing. Runtime archives must continue rejecting `.lib` and `.a`
    files; GitHub Releases are the authoritative SDK distribution channel.
34. Keep SDK candidates, release assets, attestations, source-offer material,
    performance evidence, and downstream trial evidence governed as external,
    noncommitted artifacts with bounded retention and explicit ownership.

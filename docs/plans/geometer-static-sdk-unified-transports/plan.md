+++
type = "plan"
id = "geometer-static-sdk-unified-transports"
status = "active"
created = "2026-09-18"

[[steps]]
id = "dev-std-upgrade"
title = "Pin and verify wn-dev-std 2026.9.8 across repository documentation and CI"
status = "done"

[[steps]]
id = "architecture-contract"
title = "Draft the unified operation, SDK, concurrency, licensing, and release architecture"
status = "done"
depends_on = ["dev-std-upgrade"]

[[steps]]
id = "transport-architecture-review"
title = "Obtain independent pre-implementation review of the normative transport changes"
status = "done"
depends_on = ["architecture-contract"]

[[steps]]
id = "performance-baseline"
title = "Record the existing sibling-process performance baseline and approve budgets"
status = "done"
depends_on = ["transport-architecture-review"]

[[steps]]
id = "macos-containment-spike"
title = "Prove a feasible hard macOS worker-containment primitive"
status = "pending"
depends_on = ["transport-architecture-review"]

[[steps]]
id = "dispatcher-unification"
title = "Route supported native, WASM, and executable operations through one governed dispatcher"
status = "active"
depends_on = ["transport-architecture-review", "performance-baseline"]

[[steps]]
id = "concurrency-gate"
title = "Enforce and test one serialized execution lane per Geometer runtime instance"
status = "pending"
depends_on = ["dispatcher-unification"]

[[steps]]
id = "static-sdk-packaging"
title = "Build and validate the supported relocatable static C ABI SDK"
status = "pending"
depends_on = ["dispatcher-unification", "concurrency-gate"]

[[steps]]
id = "static-license-compliance"
title = "Approve and implement the OCCT static-link licensing and corresponding-source posture"
status = "pending"
depends_on = ["transport-architecture-review"]

[[steps]]
id = "rust-client"
title = "Add the static Rust backend without duplicating generated operation methods"
status = "pending"
depends_on = ["concurrency-gate", "static-sdk-packaging"]

[[steps]]
id = "embedded-worker"
title = "Add the self-hosted stdio worker entry point and reusable process supervision"
status = "pending"
depends_on = ["concurrency-gate", "static-sdk-packaging"]

[[steps]]
id = "macos-containment-25"
title = "Complete and qualify hard macOS worker containment for issue 25"
status = "pending"
depends_on = ["macos-containment-spike"]

[[steps]]
id = "performance-qualification"
title = "Benchmark direct, sibling-process, and self-hosted execution"
status = "pending"
depends_on = ["performance-baseline", "rust-client", "embedded-worker"]

[[steps]]
id = "sdk-release-candidate"
title = "Build and verify immutable attested SDK release-candidate artifacts"
status = "pending"
depends_on = ["static-sdk-packaging", "static-license-compliance", "performance-qualification"]

[[steps]]
id = "kicad-trial"
title = "Prove the SDK and self-hosted worker in KiCad Cruncher on Windows and macOS"
status = "pending"
depends_on = ["embedded-worker", "sdk-release-candidate"]

[[steps]]
id = "projection-doc-31"
title = "Document projection view-plane reconstruction and close issue 31"
status = "pending"

[[steps]]
id = "consumer-docs"
title = "Update SDK, concurrency, transport, release, and downstream-consumer documentation"
status = "pending"
depends_on = ["kicad-trial", "macos-containment-25", "projection-doc-31"]

[[steps]]
id = "design-doc-intent-audit"
title = "Audit design docs, ADRs, and requirements against implementation"
status = "pending"
depends_on = ["consumer-docs"]

[[steps]]
id = "test-runtime-impact-audit"
title = "Audit new test runtime impact"
status = "pending"
depends_on = ["performance-qualification", "kicad-trial"]

[[steps]]
id = "external-review"
title = "Obtain independent implementation review"
status = "pending"
depends_on = ["design-doc-intent-audit", "test-runtime-impact-audit"]

[[steps]]
id = "release-signoff"
title = "Run full release signoff against the exact qualified candidate artifacts"
status = "pending"
depends_on = ["external-review", "sdk-release-candidate", "kicad-trial"]

[[steps]]
id = "release-promotion"
title = "Publish the immutable SDK asset matrix and download-verify the public release"
status = "pending"
depends_on = ["release-signoff"]

[[exit_criteria]]
id = "dev-std-upgrade"
title = "Repository and CI use and pass wn-dev-std 2026.9.8"
status = "met"

[[exit_criteria]]
id = "canonical-interface"
title = "All maintained boundaries use one catalog-governed operation and attachment model"
status = "pending"

[[exit_criteria]]
id = "concurrency-contract"
title = "Concurrency, cancellation, containment, and parallelism guarantees are enforced and documented"
status = "pending"

[[exit_criteria]]
id = "static-sdk"
title = "Relocatable, attested static SDKs pass clean external-consumer tests on every supported native platform"
status = "pending"

[[exit_criteria]]
id = "github-release-assets"
title = "Immutable GitHub Releases fail closed unless every required SDK asset, sidecar, provenance attestation, and checksum is present"
status = "pending"

[[exit_criteria]]
id = "kicad-trial"
title = "KiCad Cruncher Toon ships and runs from a single binary on Windows x64 and macOS ARM64"
status = "pending"

[[exit_criteria]]
id = "issue-25"
title = "macOS hard worker containment is proven and Geometer issue 25 is closed"
status = "pending"

[[exit_criteria]]
id = "issue-31"
title = "Projection view-plane documentation is generated, verified, and Geometer issue 31 is closed"
status = "pending"

[[exit_criteria]]
id = "issue-38"
title = "The supported static SDK and unified Rust client are released and Geometer issue 38 is closed"
status = "pending"

[[exit_criteria]]
id = "performance"
title = "Measured direct and self-hosted performance meets the recorded acceptance budgets"
status = "pending"

[[exit_criteria]]
id = "signoff"
title = "Focused and full release signoff passes"
status = "pending"

[[exit_criteria]]
id = "design-doc-intent-audit"
title = "Design docs, ADRs, and requirements match implementation"
status = "pending"

[[exit_criteria]]
id = "test-runtime-impact-audit"
title = "New tests are listed and runtime impact is reviewed"
status = "pending"

[[exit_criteria]]
id = "external-review"
title = "Independent pre-implementation design review and final implementation review are complete"
status = "pending"
+++

# Supported Static SDK and Unified Geometer Operation Transports

This working plan covers:

- [Geometer issue 38](https://github.com/wavenumber-eng/geometer/issues/38), the supported static native SDK and in-process Rust client;
- [Geometer issue 31](https://github.com/wavenumber-eng/geometer/issues/31), projection view-plane and bottom-view reconstruction documentation;
- [Geometer issue 25](https://github.com/wavenumber-eng/geometer/issues/25), hard topology-worker memory containment on macOS; and
- a downstream proof in KiCad Cruncher using the newly completed native Toon workflow.

The durable record after implementation is the accepted ADRs, requirements,
design documents, generated API references, code, tests, release attestations,
and downstream evidence. Retire this plan after all exit criteria are met.

## Independent plan review disposition

Three independent reviews were completed before implementation: interface and
concurrency, SDK/release portability, and governance/testing. Their blocking
findings were accepted and remediated in this revision:

- the process-wide execution gate moved below all supported native adapters;
- Browser Worker cancellation and the process-terminal stdio bootstrap were
  corrected;
- the direct Rust backend gained one bounded executor and explicit lifecycle;
- the native and portable catalog projections and parity scope were made
  precise;
- ADR-011 gained a required pre-implementation review step;
- existing performance baselining moved before code changes;
- release-candidate staging, downstream qualification, immutable promotion,
  and post-publication verification were separated;
- Windows `/MT`, OCCT cache identity, two-level attestation, static-link license
  compliance, and durable downstream evidence became explicit; and
- issue 25 was retained as an overall exit criterion but decoupled from the
  unrelated KiCad Toon critical path.

## Goal

Make a supported static Geometer library an invariant of every native release
while preserving one semantic operation interface across native C ABI, browser
WASM, Web Workers, the executable protocol, Rust, Python, and downstream
applications.

The static SDK must let a Rust application link Geometer and private OCCT
implementation code into its own binary. A downstream application may either:

1. invoke the generic byte-oriented operation ABI directly; or
2. spawn copies of its own executable as contained `serve --stdio` workers.

Both paths use the same operation identities, generated request/result
contracts, attachment declarations, diagnostics, limits, and catalog
negotiation. Neither path exposes OCCT headers, types, object ownership, or ABI.

## Non-goals

- Do not publish or support the Geometer C++ ABI.
- Do not expose OCCT as a downstream API.
- Do not add KiCad-, Altium-, board-, renderer-, or application-specific policy
  to Geometer.
- Do not create per-operation native Rust FFI entry points.
- Do not create a second contract catalog for the static library.
- Do not claim parallel OCCT execution inside one runtime instance without a
  separate evidence-backed architecture decision.
- Do not describe a platform binary as having no dynamic dependencies. System
  libraries and platform frameworks remain normal dependencies.
- Do not place SDK archives in the native runtime archive or restore the old
  root-level/static-library `dist` layout prohibited by ADR-015.

## Interface authority

There is one semantic interface and several transport adapters.

1. Authored TypeSpec and the normalized operation catalog define operation
   identity, request/result contracts, attachment names and media types,
   projections, limits, and generated language models.
2. The generic C ABI accepts an operation identifier, request JSON bytes, and
   named attachment byte views. It returns an opaque result containing governed
   outcome JSON and named attachment bytes.
3. Native static callers and full browser WASM call that C ABI directly.
4. Executable IPC wraps the same invocation in correlated bounded frames.
5. The browser Worker protocol wraps the same invocation in transferable
   messages.
6. Typed Rust, TypeScript, and Python methods are generated or shared facades
   over the same operation declarations. A transport must not fork their
   operation semantics.

`geometer_serve_stdio()` is a native bootstrap function, not a new operation
interface. Its only role is to host the executable IPC adapter from a binary
that statically links Geometer. It is a worker-process main entry point, not an
ordinary background-service call: it owns process stdio, must run before
unrelated application initialization or threads, and protocol-fatal paths may
terminate the child process without stack unwinding.

Legacy focused C functions and CLI commands remain compatibility adapters. No
new operation should require a bespoke public C symbol unless separately
approved for a non-generic use case.

## Current concurrency inventory

### Generic C ABI

`geometer_operation_execute` is synchronous. ADR-011 currently makes no
reentrancy or thread-safety guarantee and requires the host to serialize catalog
and execute calls. The implementation has no public execution context and no
internal concurrency gate. It dispatches portable operations through
`execute_operation`.

### Direct browser WASM

One `GeometerWasmClient` owns one Emscripten module instance. Calls enter the
synchronous generic C ABI on the JavaScript thread. One module instance is one
execution lane. Parallel geometry requires separate module/Worker instances;
WASM threads are not part of the supported contract.

### Browser Worker

The Worker client may hold multiple correlated promises, but `worker-host.ts`
chains requests through one `Promise` queue. One Worker therefore executes one
operation at a time. Transferable `ArrayBuffer` attachments avoid structured
clone copies where possible. Parallel browser work requires a Worker pool, with
one WASM module instance per Worker.

### Executable IPC

`geometer serve --stdio` has separate reader, geometry-worker, and writer
activities. It accepts and bounds multiple correlated requests, queues at most
the negotiated limits, and executes exactly one geometry request at a time.
Only queued requests can be cancelled. Terminating the process is the hard
fallback for an active operation.

The IPC server currently calls `execute_native_operation`; native topology
operations use a process-global session store and all other operations fall
back to `execute_operation`.

### Rust executable client

`GeometerClient` accepts concurrent async requests, serializes writes to stdin,
tracks pending correlations, and routes independently arriving responses. This
is concurrent client request management, not parallel geometry execution in one
server. Current downstream parallelism comes from several client/server
process pairs.

### Native topology sessions

The topology value API is explicitly serialized-worker infrastructure, not a
thread-safe shared document service. The process boundary supplies hard
deadline/cancellation replacement semantics and contains OCCT allocation that
Geometer's accounting cannot fully observe. Windows has Job Object containment;
Linux has `RLIMIT_AS`; the experimental Python supervisor rejects macOS pending
issue 25.

### KiCad Cruncher Toon

The current Toon implementation creates a pool of persistent Geometer child
processes, defaulting to four workers. Each worker is serial; throughput comes
from process-level parallelism. Replacing it with an unqualified multithreaded
in-process implementation could reduce throughput or remove crash containment.

## Target concurrency contract

The common rule is **one serialized execution lane per Geometer runtime
instance**.

| Boundary | Runtime instance | Requests accepted | Geometry execution | Supported parallelism |
| --- | --- | --- | --- | --- |
| Native generic C ABI | One loaded Geometer library in a process | Calls from any host thread after the new gate | Internally serialized, synchronous | Multiple processes |
| Direct browser WASM | One Emscripten module | Synchronous calls | Serialized by the owning JS thread | Multiple module instances |
| Browser Worker | One Worker plus one module | Multiple correlated messages | Explicit FIFO queue, one active | Worker pool |
| Executable IPC | One server process | Multiple correlated frames within bounds | One active worker | Process pool |
| Self-hosted static app | One child copy of the application | Same as executable IPC | One active worker | Self-process pool |
| Rust direct backend | One statically linked process runtime | Bounded FIFO requests | One dedicated executor thread plus the native execution gate | Multiple processes until separately qualified |

One native process owns one process-wide supported-operation execution lane.
The gate lives in the common native dispatcher below the generic C ABI,
executable IPC, embedded server, and supported legacy operation adapters so
those adapters cannot bypass one another. Catalog reads and access/free of
distinct completed result handles do not acquire the gate. Acquisition order
between concurrent callers is unspecified; callers that require mutation order
must sequence requests. Internal focused C++ value APIs retain their individual
threading contracts and must not be mixed concurrently with the supported
operation runtime unless their documentation explicitly permits it.

The first supported generation does not add opaque multi-instance C runtime
handles. Such handles would create new lifecycle, topology-session,
cancellation, and allocator contracts. Consider them only after profiling shows
that process/Worker pools are insufficient and a dedicated OCCT concurrency
qualification proves independent in-process instances safe.

### Cancellation and containment

- A direct C ABI call is synchronous and cannot be force-cancelled safely.
- A Rust timeout around a direct call is local observation only; the blocking
  native call continues until it returns.
- Executable IPC and self-hosted process workers may remove work that remains
  in their bounded server queue.
- Browser Worker A0 has no request-cancellation message. Graceful shutdown is
  queued after accepted work; `Worker.terminate()` is its only hard stop and
  rejects every outstanding request.
- Hard cancellation of active native work means terminating and replacing the
  contained worker process.
- A killed topology worker invalidates all of that process generation's
  sessions and handles.
- These transport capabilities must be reported honestly without changing the
  operation request/result contracts.

## Target execution architecture

Refactor toward one target-aware internal dispatcher:

```text
generated catalog + operation contracts
                  |
       target-aware operation dispatcher
          /                         \
 portable operations       native-only operations
          \                         /
           governed outcome + attachments
                  |
       +----------+-----------+
       |          |           |
    C ABI       IPC A0    Worker adapter
  native/WASM   executable    browser
```

The native C ABI and IPC server must call the same native dispatcher and
advertise the identical native operation projection. Direct WASM and browser
Worker advertise the portable projection generated from the same normalized
catalog authority. Catalog capability output must advertise only operations
executable by that artifact. A transport-specific unknown-operation result
where the artifact advertised support is a release-blocking defect.

Operation identities, request/result DTOs, attachment declarations, and
governed operation outcomes are shared wherever an operation is advertised.
Artifact capability projections and transport limits may differ. Exact
cross-transport parity applies to accepted well-formed operation requests and
typed operation failures; pointer, frame, queue, process, cancellation, and
termination failures remain adapter-local and must be reported honestly.

## Workstreams

### 1. Architecture and governance

- Add an ADR that approves the separately versioned static SDK anticipated by
  ADR-015 and records the one-lane concurrency contract.
- Return ADR-011's normative review gate to pending and amend it rather than
  creating a competing generic transport decision. Obtain independent review
  of the amendment, new SDK/concurrency ADR, native capability projection,
  process-terminal stdio bootstrap, and C ABI generation before implementation.
- Define the Windows SDK's sole initial standalone profile as MSVC v143 Release
  `/MT`, with Rust `target-feature=+crt-static`. Give it a distinct OCCT build,
  install, recipe, marker, and cache key; never restore the normal `/MD` OCCT
  profile into it. The normal executable/wheel build may remain `/MD`.
- Add a focused static-SDK/concurrency requirement with verification references
  and update REQ-001, REQ-005, REQ-006, REQ-008, and REQ-010 only where their
  existing supported boundaries change.
- Increment the date-based C ABI generation for the native capability and
  bootstrap additions and regenerate catalog digests and generated references.
- Register external SDK artifacts and attestations in the artifact-governance
  catalog and register a separate GitHub SDK release channel.
- Record a reviewed OCCT static-link legal/compliance disposition, corresponding
  source/source-offer strategy, relinking guidance, and downstream obligations.
- Preserve ADR-015's distinction between runtime archives and SDK archives.

### 1a. Development-standard upgrade

- Pin repository CI, release, setup, and nested Rust validation to
  `wn-dev-std==2026.9.8`; do not install moving Git `main`.
- Update stale source documentation and Rack stratum text that still names
  2026.8.12, then regenerate derived documentation.
- Run the latest full repository, nested Rust, plan, and release-mode audits.
- At final closeout, use the current dev-std plan-close operation to remove this
  temporary plan only after its durable records and logs are complete.

### 2. Unified dispatcher and capability truth

- Replace the portable/native split at transport call sites with one
  target-aware dispatcher.
- Make the native generic C ABI reach every operation advertised by the native
  catalog, including native topology operations when their lifecycle contract
  is satisfied.
- Keep unsupported WASM operations absent from the WASM runtime capability
  view rather than advertising calls that can only fail as unknown.
- Reuse operation request validation, response validation, governed diagnostics,
  and result ownership across direct and framed transports while retaining
  honest transport-specific limits and local failures.
- Require canonical native C ABI and IPC welcome capability projections to have
  identical operation identities and catalog digest.
- Add cross-transport parity vectors for every promoted operation used in the
  downstream trial.

### 3. Native concurrency gate

- Add a small responsibility-focused native execution gate in the common native
  dispatcher below every supported operation adapter. Catalog queries remain
  outside the geometry gate.
- Prove simultaneous calls from foreign threads never overlap geometry
  dispatch in one process.
- Prove simultaneous calls through different adapters cannot overlap.
- Prove the lock is released on successful outcomes, contract failures,
  allocation failures, and caught exceptions.
- Do not hold the execution gate while a caller reads or frees an already
  returned result. Access/free of the same result handle must not race.
- Preserve the IPC server's bounded reader/one-worker/one-writer design and the
  browser Worker's explicit queue.
- Audit legacy exported C geometry functions and either route each supported
  operation adapter through the gate or document it as outside the supported
  operation-runtime concurrency guarantee.

### 4. Supported static SDK

Create a separately named SDK archive, not a runtime archive. Its logical layout
is:

```text
include/geometer/c_api.h
lib/geometer.lib                 # Windows
lib/libgeometer.a                # Unix
lib/occt/<required archives>
lib/cmake/Geometer/GeometerConfig.cmake
lib/cmake/Geometer/GeometerConfigVersion.cmake
lib/cmake/Geometer/GeometerTargets.cmake
share/geometer/geometer-sdk.json
share/geometer/geometer-sdk-attestation.json
licenses/...
```

Requirements:

- Install/export a relocatable `Geometer::c_api_static` target.
- Use build/install include interfaces and package-relative imported dependency
  targets. Installed CMake/JSON files must contain no source, build, `.deps`,
  runner-home, or drive-specific paths.
- Package only the exact OCCT archive closure required by that target.
- Do not require OCCT headers for C ABI consumers.
- Keep OCCT, RapidJSON, and Clipper include paths private and propagate the
  archive closure with link-only metadata.
- Generate a machine-readable ordered native-link manifest for Cargo and other
  non-CMake consumers, including system libraries/frameworks and required link
  groups/options.
- Define a versioned JSON Schema for the link/SDK manifest with archive paths,
  logical names, ordering/group boundaries, system libraries, Apple frameworks,
  target triple, compiler/runtime profile, and minimum platform baseline.
- Put a canonical internal manifest in the archive that binds every member name,
  size and SHA-256 plus catalog/toolchain/link identity and rejects undeclared
  members.
- Publish an external sidecar that binds the final archive SHA-256, tag, source
  revision, workflow/run identity, and internal-manifest digest. Also publish and
  verify GitHub build provenance plus detached checksums; an unsigned checksum
  alone is not provenance.
- Validate archive paths, checksums, canonical metadata, and absence of shared
  Geometer/OCCT dependencies.
- Build a clean external CMake consumer and a clean external Cargo consumer
  outside the source/build trees.
- Relocate the installed SDK into an unrelated path containing spaces before
  both external-consumer tests.
- Pack deterministically and compare archive digests from two clean builds of
  the same source and toolchain profile.

The supported release matrix is the current native matrix: Windows x64, Linux
x64, Linux ARM64, and macOS ARM64. Windows x64 and macOS ARM64 are mandatory
downstream application trials. Intel macOS and Universal 2 are new targets and
remain out of scope until separately approved.

Supported ABI profiles are Windows x64/MSVC v143 `/MT`/Rust `crt-static`,
macOS ARM64/Apple Clang/libc++/macOS 11.0, and Linux x64 or ARM64 with the
qualified GCC/libstdc++ ABI and glibc 2.35 baseline. Consumers fail closed on a
target, compiler family, runtime, architecture, or baseline mismatch.

### 4a. Static-link licensing

- Include exact OCCT source identity, upstream archive checksum, modification
  status, build scripts, licenses, and prominent use notice.
- Provide durable corresponding-source access or a compliant source offer and
  document the relinkable-object/source alternative for downstream static
  executable distributors, plus any available commercial-licensing path.
- Preserve required notices in single-executable downstream packages; license
  and README files do not violate the single-executable runtime requirement.
- Treat the reviewed legal/compliance disposition as a promotion prerequisite,
  not a conclusion inferred from build metadata.

### 5. Rust client without a second typed API

- Add a narrow `geometer-sys` crate that owns all `unsafe extern "C"`
  declarations and declares `links = "geometer"`.
- Have its build script consume the SDK manifest from an explicit, verified SDK
  location. It must not download artifacts during `cargo build`.
- Use Cargo's `TARGET`, validate target/toolchain/runtime compatibility before
  emitting link directives, emit complete rerun metadata, preserve Linux rescan
  groups without default whole-archive linking, and link required C++ runtimes
  and platform libraries explicitly. Reject the Windows SDK unless Rust
  `crt-static` matches its `/MT` profile.
- Keep generated contracts/codecs and ergonomic operation methods in one shared
  layer.
- Refactor `GeometerClient` around an internal backend boundary rather than
  copying operation methods into process and static client types.
- Retain the existing executable/process-adoption backend.
- Add a static backend with one dedicated executor thread and a bounded FIFO
  queue shared by direct clients in the process. Do not occupy multiple Tokio
  blocking-pool threads while waiting on the native gate.
- Permit cancellation only while a direct request remains queued. Once active,
  timeout or future abandonment is observational: execution continues, owns the
  process lane until completion, and must still free every result/error buffer.
- Define close and final-drop behavior while queued or active work remains.
- Document that direct clients share the process-wide execution lane and
  topology session store; dropping one client neither isolates nor invalidates
  process-global sessions.
- Require the process backend for untrusted data, hard deadlines, crash
  isolation, or client-private topology lifecycle.
- Report backend capabilities for queued cancellation, hard active-operation
  termination, topology-session replacement, and execution serialization.
- Keep unsafe code isolated in `geometer-sys`; safe client crates retain their
  existing unsafe-code prohibition.

### 6. Embedded/self-hosted server

- Add `GEOMETER_C_API int geometer_serve_stdio(void)` as a native-only C
  bootstrap around the existing server.
- Catch ordinary C++ exceptions and return stable process exit codes, while
  documenting that existing protocol-fatal/deadline paths may call `_Exit`.
- Require invocation only as the main path of a dedicated child process, before
  normal application startup or unrelated threads, with exclusive stdio
  ownership. Never expose it as an in-process background service.
- Add a reusable client construction path that can spawn a caller-selected
  executable and argument vector without weakening containment ownership.
- Let downstream binaries expose a hidden `serve --stdio` or equivalently
  governed worker command that detects worker mode and invokes the bootstrap
  before configuration, logging, runtime, or UI initialization.
- Keep stdout protocol-only and stderr log-only.
- Preserve exact welcome/catalog negotiation, bounds, shutdown, queue
  cancellation, and forced-termination behavior.

### 7. macOS hard containment for issue 25

- Run a focused macOS ARM64 spike before selecting a mechanism. Test the limit
  before Python, OCCT, and large application mappings are established.
- Prefer a minimal native pre-execution/bootstrap path or another documented
  OS-native primitive that produces a real enforced ceiling, not polling-based
  best effort.
- Distinguish setup failure, launcher failure, ordinary worker failure, and
  allocation rejection in tests and diagnostics.
- Prove deadline and explicit cancellation kill the worker and descendants.
- Preserve private temporary-directory cleanup and process-generation/session
  invalidation.
- Exercise the mechanism with signed/hardened runtime builds used by the macOS
  release path.
- Reuse the containment primitive from the self-hosted Rust worker controller
  where feasible; do not change public topology operation contracts to expose
  process mechanics.
- Enable and run the existing Python `TopologyWorkerSupervisor` containment
  tests on macOS ARM64 only after the hard allocation ceiling is proven, and
  change its documented reject-macOS support boundary in the same change.
- Keep macOS fail-closed until the allocation ceiling and descendant kill are
  proven. Advisory memory reporting is not sufficient to close issue 25.
- Keep this process-backend guarantee separate from direct static execution,
  which cannot inherit hard memory containment. Issue 25 remains an overall
  plan exit criterion but does not block the KiCad Toon trial unless that
  downstream product explicitly adopts the same hard-memory guarantee.

### 8. Performance qualification

Build one benchmark harness around identical requests and fixtures. Compare:

- direct native C ABI through the Rust static backend;
- current sibling `geometer serve --stdio`;
- self-hosted application `serve --stdio`;
- direct browser WASM; and
- browser Worker WASM where the operation is supported.

Use both a native STEP/model-illustration corpus and a canonical portable
indexed-mesh illustration request. The latter must replay identical request and
attachment bytes through executable IPC, native C ABI, direct WASM, and browser
Worker WASM so portable transports are measured rather than skipped.

Record on Windows x64 and macOS ARM64:

- cold construction/startup latency;
- first operation latency;
- warm p50/p95 latency;
- sustained throughput with one worker and four workers;
- input and output attachment bytes;
- peak resident memory per process and for the pool;
- final binary and SDK sizes; and
- output digests or governed toleranced parity.

Use representative small, medium, and large STEP/Toon fixtures. Persistent
workers must be warmed before steady-state comparison. Before implementation,
record the current sibling-process baseline and set reviewed numeric regression
budgets. Use enough steady-state samples for meaningful p50/p95 statistics and
at least three independent benchmark runs for median comparison. Sustained
one-worker and four-worker batches must run for at least five seconds, with one
serial host loop per runtime process and barrier-synchronized starts. Timing gates
belong in controlled qualification jobs, not local correctness tests.
Self-hosting is expected to remain within benchmark noise of the sibling executable because it
uses the same protocol; any regression outside the approved budget blocks
promotion. Do not choose direct execution as the default merely because a
microbenchmark is faster if four-worker application throughput or containment
is worse.

### 9. GitHub Release integration

- Split staging from promotion. The four-platform matrix first produces
  deterministic immutable release-candidate SDK archives as workflow artifacts
  or draft-release assets.
- Run clean external consumers and the KiCad trial against those exact staged
  bytes, never against rebuilt archives.
- After qualification, download the candidate artifacts, verify an explicit
  expected inventory, checksums, internal manifests, external sidecars, and
  GitHub provenance, and then publish those same bytes.
- Name SDK assets with release and platform identity so runtime and SDK archives
  cannot be confused.
- Replace arbitrary file discovery and `--clobber` promotion with a generated
  four-target inventory that rejects missing, extra, duplicate, or changed
  assets. Published release assets are immutable.
- Keep Node 24-capable GitHub action majors required by repository policy.
- Make final GitHub and PyPI publication depend on the same qualified inventory
  when the release is intended to be atomic.
- Download the public release after promotion and repeat inventory, provenance,
  and clean-consumer verification.
- Run `wn-dev-std audit . --mode release` against the staged promotion payload.
- Add SDK asset inventory to release notes and `docs/design/distribution.md`.
- Continue rejecting `.lib` and `.a` files from native runtime archives.
- Do not commit the large SDK/OCCT archives to Git. GitHub Releases are their
  authoritative distribution channel; local build output remains generated
  state.

### 10. KiCad Cruncher trial

Use a clean downstream branch/worktree and a release-candidate SDK, not Geometer
build-tree paths.

- Remove Geometer from `PATH`, unset `GEOMETER_EXE`, extract the candidate into
  an unrelated relocatable path, and prove all worker child executable paths
  resolve to the Cruncher binary.
- Pin SDK release, source revision, C ABI generation, catalog digest, target,
  and archive checksum.
- Link Geometer and its private OCCT closure into the Cruncher executable.
- Have the hidden worker command call `geometer_serve_stdio()`.
- Change native Toon worker discovery to spawn the current Cruncher executable.
- Preserve the existing default four-worker pool and process failure behavior.
- Run Toon with one and four workers on Windows x64 and macOS ARM64.
- Run one direct static `geometry.model_illustration_geometry.a0` operation even
  if the shipping backend remains self-hosted workers.
- Compare results against the current sibling-Geometer implementation.
- Force one worker to fail and prove the parent reports/replaces it without
  corrupting other jobs.
- Package and test with no `geometer.exe`/`geometer` sibling and no Geometer or
  OCCT shared libraries.
- Inspect Windows imports and macOS `otool -L`, deployment target, signature,
  hardened runtime, and notarization evidence.
- Test the signed, notarized, quarantined macOS artifact rather than only an
  unsigned CI binary.
- Measure the same performance corpus used by Geometer qualification.

Define “single binary” as one canonical executable required at runtime. Optional
Cruncher command aliases and required licenses/readmes may remain in an archive,
but there is no Geometer executable or Geometer/OCCT shared-library sidecar.

Commit or attach a durable trial record containing the SDK archive/manifest
digests, Geometer tag/source/C ABI/catalog identity, KiCad Monkey commit,
workflow URL/run ID and toolchains, final executable digest, dependency scans,
one/four-worker output hashes and performance, child executable paths, and the
forced-worker-failure outcome.

The trial is an SDK promotion gate. Findings that require downstream knowledge
must be fixed in the application; findings about linking, manifests,
attestation, contracts, or transport behavior must be fixed in Geometer before
the SDK is declared supported.

### 11. Projection documentation for issue 31

As part of the overall public-documentation update:

- Update `docs/design/hlr-projection-a0.md` and the authored TypeSpec
  documentation for `HlrViewSpec`.
- State that output XY is in the requested view plane, not model/board XY.
- Define the basis exactly: normalized direction is Z, orthogonalized normalized
  up is Y, and X is `Y cross Z`; define the visible/front-face sign precisely.
- State the projection origin and that `model_transform` is applied before view
  projection without recentering to an object bounding box.
- Include the axial examples for up `[0,1,0]`: positive Z maps `(x,y)` to
  `(x,y)` and negative Z maps it to `(-x,y)`.
- Document reconstruction as `board_point = occurrence_transform *
  view_to_model * view_point`, including bottom `diag(-1,1)` in 2D.
- Apply the frame statement to silhouette, visible detail, arcs, bounds, and
  related projection outputs; preserve arc orientation through the affine.
- Distinguish view selection from coordinate reconstruction and model-local
  placement from already board-placed geometry.
- Regenerate and verify public API/reference documentation.
- Add the requested small asymmetric example without changing geometry or
  release behavior.

Close issue 31 only after authored and generated documentation agree with all
existing projection engines.

## Test and validation matrix

### Focused native tests

- Concurrent foreign-thread calls prove maximum active dispatch count is one.
- Cross-adapter stress proves generic C ABI and embedded/IPC entry cannot
  overlap in the same native process; acquisition order remains unspecified.
- Exception and allocation-failure injection proves lock release and stable C
  output states.
- Native C ABI and IPC execute the same portable operation corpus.
- Native-only topology capability advertisement matches actual dispatch.
- Result ownership/free tests cover success and every local failure class.
- Embedded server handshake, request, cancellation, shutdown, broken-pipe, and
  forced-exit tests reuse existing IPC vectors.
- Direct Rust stress covers queue saturation, queued cancellation, active
  timeout/future abandonment, close during active execution, multiple clients,
  cleanup after abandonment, and at most one executor/native dispatch.

### SDK consumer tests

- Install into a temporary prefix and relocate it before consumption.
- Build C and C++ consumers through the exported CMake target.
- Build Rust through the SDK link manifest with no source-tree paths.
- Link and run a catalog query, model bounds, model illustration geometry, and
  the embedded stdio server.
- Detect missing/reordered transitive libraries and accidental shared OCCT or
  Geometer dependencies.
- Verify Windows CRT selection and macOS minimum deployment target.
- On Windows reject MSVC runtime DLL imports; on macOS inspect architecture,
  `LC_BUILD_VERSION`, and `otool -L`; on Linux inspect `NEEDED` and required
  GLIBC/GLIBCXX versions and execute on the claimed minimum host.

### Cross-transport tests

- Replay governed requests through direct native, native IPC, direct WASM, and
  Worker WASM wherever catalog capabilities overlap.
- Require exact shared operation/outcome/attachment metadata parity and the
  existing exact or toleranced geometry comparison policy for well-formed
  accepted requests. Keep adapter-local pointer/frame/queue/process failures
  outside that parity claim.
- Verify unsupported target operations are absent from advertised capability
  catalogs.

### Release tests

- Validate all four SDK archives and attestations.
- Download the GitHub Release assets and repeat clean consumer smoke tests.
- Run `uv run --group dev rack run --all` and the L99 release stratum.
- Run the canonical Ruff, Pyright, lock, clang-format, native, package, WASM,
  Rust, documentation, and standards gates before tagging.

Before implementation, assign tests to fast CTest/Rack, Rust stratum, platform
CI, release-only four-platform qualification, or promotion/manual lanes. Keep
network downloads and timing assertions out of L99/local correctness tests and
record measured runtime deltas in the closeout log and test-strategy docs.

## Documentation and durable-record updates

Expected records include:

- a new SDK/concurrency ADR plus any reviewed amendment to ADR-011;
- ADR-015 cross-reference to the implemented SDK distribution;
- updated REQ-001, REQ-005, REQ-006, REQ-008, and REQ-010;
- `docs/design/generic-operation-c-abi.md`;
- `docs/design/executable-ipc-a0.md` and its consumer guide;
- `docs/design/distribution.md`;
- native interface, WASM, Rust-client, and developer build/release guides;
- the projection documents and generated references required by issue 31;
- macOS containment support documentation required by issue 25;
- SDK manifests, attestation schema, and release notes; and
- artifact/release governance catalogs and test-strategy lane assignments;
- KiCad Cruncher downstream design/release documentation and durable trial
  evidence; and
- current wn-dev-std version references and pinned CI/setup commands.

Do not leave implementation decisions only in this plan or its logs.

## Risks and review questions

1. **Windows CRT:** the SDK uses `/MT`; prove OCCT, Geometer, and Rust
   `crt-static` match and that the separately keyed profile cannot restore `/MD`
   objects.
2. **Archive link ordering:** Cargo, Apple `ld`, GNU linkers, and MSVC differ.
   Generate ordered metadata from the authoritative CMake graph and validate
   it externally rather than maintaining a handwritten list.
3. **SDK size:** the raw OCCT closure is large. Correctness and relocatability
   come before merged-archive optimization. Consider a combined archive only
   after the multi-archive SDK is qualified on every platform.
4. **Capability drift:** native C ABI currently dispatches a smaller set than
   native IPC. Unification must not silently expose unsupported or incompletely
   lifecycle-managed operations.
5. **Static process state:** topology sessions and the execution gate are
   process-global in the first generation. Document lifetime and fork/process
   replacement consequences.
6. **Direct-call cancellation:** no safe hard cancellation exists inside a
   process. Keep this transport limitation visible and retain workers for
   untrusted or deadline-bound inputs.
7. **macOS memory ceiling:** issue 25 requires empirical proof on signed ARM64
   release builds. A parent polling loop is detection, not hard containment.
8. **Interface proliferation:** implementation crates, link manifests, and
   transport adapters must not grow separate generated operation facades.
9. **Static-link compliance:** technical packaging cannot decide legal
   sufficiency. Promotion requires the recorded reviewed disposition and
   corresponding-source/relink path.
10. **Release mutation:** a workflow triggered only after public release and
    uploading with `--clobber` cannot be the promotion gate. Candidate bytes
    must be qualified before immutable publication.

## Closure order

1. Complete and verify the wn-dev-std upgrade; issue 31 documentation may run
   in parallel.
2. Draft the ADR/requirements, obtain independent pre-implementation review,
   and record the existing process performance baseline.
3. Run the macOS containment feasibility spike in parallel with architecture
   work, without coupling the KiCad trial to issue 25.
4. Implement common dispatch and its process-wide gate, then the static SDK,
   Rust backend, and process-terminal embedded worker.
5. Complete focused tests, performance qualification, licensing disposition,
   and immutable four-platform candidate artifacts.
6. Run clean external consumers and the Windows/macOS KiCad Cruncher trial
   against the exact candidate bytes; complete issue 25 qualification in
   parallel.
7. Complete durable docs, governance catalogs, test-runtime audit, and
   independent implementation review.
8. Run full release signoff, publish the exact qualified SDK asset matrix, then
   download and verify the public release.
9. Close issues 25 and 31 when their own evidence is complete. Close issue 38
   only after public released-asset verification—not merely a build-tree or
   staged library—passes.
10. Use the current dev-std close operation to retire this temporary plan after
    all outcomes are represented in the docs of record.

+++
type = "plan"
id = "geometer-typespec-contracts"
status = "active"
created = "2026-08-12"

[[steps]]
id = "authority-and-inventory"
title = "Accept the contract authority boundary and freeze the current interface inventory"
status = "done"

[[steps]]
id = "contract-semantics-and-transport"
title = "Ratify generic ABI, option presence, IPC execution, diagnostics, and conformance rules"
status = "done"
depends_on = ["authority-and-inventory"]

[[steps]]
id = "transport-design-review"
title = "Obtain independent review of the generic C ABI and IPC A0 design"
status = "done"
depends_on = ["contract-semantics-and-transport"]

[[steps]]
id = "typespec-foundation"
title = "Establish the TypeSpec source, normalized catalog, schemas, and generation gates"
status = "done"
depends_on = ["transport-design-review"]

[[steps]]
id = "pilot-contracts"
title = "Model pilot primitives, diagnostics, model bounds, and conformance vectors"
status = "done"
depends_on = ["typespec-foundation"]

[[steps]]
id = "documentation-asset-licensing"
title = "Record font redistribution authority or select and license an open-font substitute"
status = "done"
depends_on = ["authority-and-inventory"]

[[steps]]
id = "generated-html-reference"
title = "Generate the styled HTML contract reference and verify it offline"
status = "done"
depends_on = ["pilot-contracts", "documentation-asset-licensing"]

[[steps]]
id = "cpp-server"
title = "Generate C++ contract code and integrate the native operation server"
status = "done"
depends_on = ["generated-html-reference"]

[[steps]]
id = "typescript-wasm"
title = "Generate the TypeScript contracts and browser/WASM reference client"
status = "done"
depends_on = ["cpp-server"]

[[steps]]
id = "rust-exe-ipc"
title = "Generate the Rust contracts and geometer executable pipe client"
status = "pending"
depends_on = ["cpp-server"]

[[steps]]
id = "python-public-contracts"
title = "Integrate generated contracts behind the compatible public Python API"
status = "pending"
depends_on = ["cpp-server"]

[[steps]]
id = "model-bounds-promotion"
title = "Promote model bounds as the first complete generated-contract vertical"
status = "pending"
depends_on = ["typescript-wasm", "rust-exe-ipc", "python-public-contracts"]

[[steps]]
id = "geom-contract-alignment"
title = "Freeze the Geom a0 compatibility input and propose Geometer-owned reusable planar contracts"
status = "pending"
depends_on = ["model-bounds-promotion"]

[[steps]]
id = "analytic-planar-boolean-design"
title = "Design and prototype the analytic planar Boolean contract, solver, and packed format"
status = "pending"
depends_on = ["geom-contract-alignment"]

[[steps]]
id = "analytic-synthetic-correctness"
title = "Establish the analytic Boolean synthetic correctness and mutation program"
status = "pending"
depends_on = ["analytic-planar-boolean-design"]

[[steps]]
id = "occt-8-0-1-qualification"
title = "Qualify OCCT V8_0_1 against V8_0_0 on native, WASM, and existing regressions"
status = "pending"
depends_on = ["analytic-synthetic-correctness"]

[[steps]]
id = "analytic-planar-boolean-promotion"
title = "Promote analytic planar Boolean batch and ship its focused additive release"
status = "pending"
depends_on = ["occt-8-0-1-qualification"]

[[steps]]
id = "remaining-operation-promotion"
title = "Promote the remaining inventoried operations and retire displaced handwritten contract code"
status = "pending"
depends_on = ["analytic-planar-boolean-promotion"]

[[steps]]
id = "typescript-demo-closure"
title = "Verify every maintained browser demo migrated with its owning operation"
status = "pending"
depends_on = ["remaining-operation-promotion"]

[[steps]]
id = "viz-compatibility"
title = "Prove Viz 2026.6.10 compatibility and publish its TypeScript migration path"
status = "pending"
depends_on = ["remaining-operation-promotion"]

[[steps]]
id = "design-doc-intent-audit"
title = "Update and audit docs of record, guides, generated references, and compatibility notes"
status = "pending"
depends_on = ["typescript-demo-closure", "viz-compatibility"]

[[steps]]
id = "test-runtime-impact-audit"
title = "Audit new test runtime, packaging, and release impact"
status = "pending"
depends_on = ["typescript-demo-closure", "viz-compatibility"]

[[steps]]
id = "external-review"
title = "Obtain independent review of the implemented contracts, transports, and clients"
status = "pending"
depends_on = ["design-doc-intent-audit", "test-runtime-impact-audit"]

[[exit_criteria]]
id = "authority"
title = "The accepted ADR and promotion manifest identify one structural authority per promoted contract"
status = "pending"

[[exit_criteria]]
id = "generic-operation-abi"
title = "The additive generic operation and attachment C ABI serves the model bounds pilot in native and browser WASM"
status = "pending"

[[exit_criteria]]
id = "presence-and-defaults"
title = "Wire presence, option patches, normalized options, and encoder default omission have distinct tested semantics"
status = "pending"

[[exit_criteria]]
id = "ipc-execution-policy"
title = "The stdio protocol enforces serialized execution, queue-only cancellation, bounded framing, and safe shutdown"
status = "pending"

[[exit_criteria]]
id = "transport-design-review"
title = "Independent review approves the generic C ABI and IPC A0 specifications before implementation"
status = "met"

[[exit_criteria]]
id = "conformance-oracles"
title = "Every vector declares an assertion lane and exact, structural, or toleranced comparison policy"
status = "pending"

[[exit_criteria]]
id = "deterministic-generation"
title = "All generated artifacts are deterministic, complete, and clean under check mode"
status = "pending"

[[exit_criteria]]
id = "documentation-asset-licensing"
title = "Every vendored font has repository-safe redistribution authority and committed license evidence"
status = "met"

[[exit_criteria]]
id = "generated-html-reference"
title = "Generated HTML contract references use the approved vendored Wavenumber visual system and pass freshness, link, and offline browser checks"
status = "pending"

[[exit_criteria]]
id = "cross-language-conformance"
title = "C++, TypeScript, Rust, and Python pass the same governed request, response, and diagnostic vectors"
status = "pending"

[[exit_criteria]]
id = "typescript-wasm"
title = "A packaged TypeScript consumer completes supported operations through browser WASM without direct pointer management"
status = "met"

[[exit_criteria]]
id = "typescript-demos"
title = "The maintained browser demos are TypeScript consumers of the generated contracts and WASM client"
status = "pending"

[[exit_criteria]]
id = "rust-exe-ipc"
title = "A packaged Rust consumer completes supported operations through a persistent geometer executable pipe"
status = "pending"

[[exit_criteria]]
id = "python-generated-contracts"
title = "The compatible public Python API uses generated contract models and strict codecs at its executable boundary"
status = "pending"

[[exit_criteria]]
id = "compatibility"
title = "Existing public surfaces pass compatibility tests or have an approved versioned replacement with migration notes"
status = "pending"

[[exit_criteria]]
id = "viz-compatibility"
title = "Viz 2026.6.10 remains compatible or has completed its generated TypeScript client migration with replacement evidence"
status = "pending"

[[exit_criteria]]
id = "analytic-planar-boolean"
title = "The analytic planar Boolean operation preserves exact topology and passes native, WASM, IPC, TypeScript, Rust, Python, and MATZ integration gates"
status = "pending"

[[exit_criteria]]
id = "analytic-synthetic-correctness"
title = "Closed-form, invariant, metamorphic, degeneracy, exhaustive, property, lineage, cross-runtime, and mutation suites protect the analytic mathematics"
status = "pending"

[[exit_criteria]]
id = "occt-8-0-1-qualification"
title = "OCCT V8_0_1 is accepted with complete native/WASM evidence or V8_0_0 retention has a governed rejecting fixture and decision"
status = "pending"

[[exit_criteria]]
id = "matz-compatibility"
title = "MATZ has reviewed the released contract, supplied representative fixtures, pinned the tagged Geometer release, and passed solved-copper integration"
status = "pending"

[[exit_criteria]]
id = "documentation-closure"
title = "ADRs, requirements, design docs, guides, generated references, compatibility notes, and release notes are current"
status = "pending"

[[exit_criteria]]
id = "issue-traceability"
title = "GitHub issue 18 is closed only after completion or linked follow-up issues account for every accepted remainder"
status = "pending"

[[exit_criteria]]
id = "release-signoff"
title = "Native, WASM, package, Rack, and L99 release gates pass"
status = "pending"

[[exit_criteria]]
id = "design-doc-intent-audit"
title = "Docs of record and consumer/developer guides match the shipped interfaces and compatibility posture"
status = "pending"

[[exit_criteria]]
id = "test-runtime-impact-audit"
title = "New tests are represented in Rack strata and their runtime impact is reviewed"
status = "pending"

[[exit_criteria]]
id = "external-review"
title = "Independent implementation review is complete"
status = "pending"
+++

# TypeSpec Contract Authority And Generated Clients

## Active Status

This plan is active under ADR-010. Authority, inventory, and independent review
of the generic C ABI and IPC A0 design packet are complete; ADR-011 is Accepted
and transport implementation is allowed. The TypeSpec/catalog foundation is
implemented. The pilot contracts, generated HTML reference, generated C++
projection, and approved generic C ABI/model-bounds server slice are complete.
No operation is promoted and no compatibility reader or published interface
may be removed merely because the plan is active.

The deterministic styled HTML reference and its offline resource/navigation
tests now include real-Chrome desktop and emulated 390 px verification. That
browser pass found and closed a narrow generated-identifier overflow, with the
revised stylesheet digest recorded in the promotion manifest.

The TypeScript/WASM slice is complete for the model-bounds pilot. Generated
DTOs, strict codecs, typed operation metadata, the deterministic
`@wavenumber/geometer` ESM artifact, direct and correlated Worker clients, a
clean packed consumer, a real Worker/WASM test, the model-bounds TypeScript
browser example, and the Viz operation-by-operation migration guide are
implemented. Other browser demos migrate only when their owning operations are
promoted.

Generated Python is intentional mandatory scope based on project-owner
direction that public Python should use generated contract code while
maintaining compatibility. It is not merely a consequence of review feedback.
Every operation promotion therefore requires its Python projection and public
compatibility evidence. Removing that requirement would be a material plan
scope change requiring explicit approval.

The plan also accepts analytic planar Boolean batch as the first additive
capability after the `model_bounds` infrastructure pilot. The MATZ packet is a
frozen consumer-requirements input; Geometer still owns and reviews the generic
contract, solver, packed format, and release shape.

The plan adopts lessons from the ALX TypeSpec work in
`C:/eli/wn-hw/appz/data_models`, while keeping Geometer generic and independently
buildable. ALX models and application policy must not become Geometer
dependencies.

## Tracking

GitHub issue [#18](https://github.com/wavenumber-eng/geometer/issues/18) is the
durable program tracker. Implementation commits and pull requests must reference
it. Any accepted work that remains when the main implementation closes must be
moved to a linked follow-up issue before issue #18 is closed.

wn-dev-std plan logs are the active work, handoff, and review-request record.
Issue comments are historical context and are not authoritative review packets.
A transport approval must cite the exact requested revision and digests from
the current `transport-design-review` log and promotion manifest, regardless of
an earlier issue comment.

## Goal

Establish one authored definition for Geometer operation contracts and use it
to make new interfaces inexpensive and consistent across the native library,
browser WASM, native executable IPC, and language clients.

The required production projections are:

- generated JSON Schemas and styled HTML contract documentation;
- generated C++ value types and structural codecs used by Geometer itself;
- generated TypeScript types, strict codecs, and a browser/Web Worker WASM
  reference client;
- generated Rust types, strict codecs, and client helper code for a persistent
  `geometer` executable pipe;
- generated Python contract models and strict codecs integrated behind the
  compatible public executable-backed API; and
- shared raw-byte conformance vectors replayed by C++, TypeScript, Rust, and
  Python.

## Proposed Authority Boundary

After an individual contract is promoted, authority flows in one direction:

```text
authored Geometer TypeSpec
  -> normalized Geometer contract and operation catalog
  -> generated JSON Schema and documentation
  -> generated C++, TypeScript, Rust, and Python structural projections
  -> transport-specific clients and dispatch adapters
  -> shared cross-language conformance evidence
```

TypeSpec owns serialized structure: field names, requiredness, defaults,
unions, closure, scalar constraints, operation parameter/result types,
diagnostic payloads, and attachment descriptors.

Defaults in this statement mean declared canonical default intent. Generated
wire decoders must preserve whether a field was absent and must not eagerly
materialize defaults into partial option-patch models.

The C++ geometry implementation remains behavioral authority for model loading,
OCCT operations, planar solving, tessellation, projection, and other geometry
semantics. Generated models must not create a second geometry implementation.

Transport mechanics remain explicit and separately governed:

- C ABI symbol names, pointer ownership, allocation/free rules, and WASM export
  sets;
- executable process lifecycle, framing, request correlation, cancellation,
  logging, and attachment transfer; and
- packed planar packet magic, endianness, fixed-width fields, flags, reserved
  words, and layout versions.

The normalized catalog may carry reviewed Geometer-specific transport metadata,
and generators may consume it, but ordinary JSON Schema generation must not be
treated as sufficient authority for these mechanics.

Generated C++ contract types are wire DTOs. They map explicitly to and from the
existing focused public C++ value APIs. A generated nested JSON shape must not
silently replace a public semantic value such as `ModelBoundsResult`; changing
those public C++ values requires its own compatibility decision.

## Resolved Design Choices

These decisions are incorporated in the active plan. ADR-010 accepts the
authority decisions. ADR-011 and the concrete transport specifications passed
their independent review gate at the revision recorded in the manifest.

### Generic Browser And Native Operation ABI

New generated clients will use an additive generic operation/attachment C ABI,
not a new handwritten C symbol for every operation. The ABI will accept:

- a stable operation identity;
- one generated operation-specific JSON request DTO;
- a counted array of named byte attachments; and
- explicit output holders for one generated generic operation outcome and a
  counted array of named byte attachments.

The ABI design must define fixed-width fields, struct sizes or generations,
ownership, allocation/free functions, limits, duplicate attachment-name
behavior, and error cleanup. The operation catalog will generate or verify the
operation identities and attachment declarations. Existing per-operation C ABI
symbols remain compatible adapters until separately retired.

`model_bounds` will be the first operation on this ABI. Its model bytes are an
input attachment and its structured bounds are the successful result inside the
generic operation outcome. The full browser WASM target must export the generic
entry point before the pilot can claim a browser round trip.

### Presence, Defaults, And Layered Options

The contract system will distinguish four concepts:

1. Wire input models preserve presence with an unset state distinct from JSON
   `null` and from an explicitly supplied default-valued field.
2. Operation-specific option-patch DTOs contain only present fields and do not
   materialize defaults while decoding.
3. Compatibility and batch layers merge patches in order. Existing semantics
   remain defaults, then top-level batch patch, then job patch.
4. Focused C++ option values are normalized only after all patches are applied;
   every canonical option then has an effective value.

Canonical request encoders omit absent optional fields. Patch encoders preserve
explicit presence, including fields explicitly set to their default value.
Generated encoders may omit default-valued fields only for models whose
contract declares that policy; omission must never be used for patch DTOs.
JSON `null` is rejected unless the specific field explicitly admits it.
Compatibility aliases normalize into canonical patch fields before merging and
do not enter the generated canonical DTOs.

### Executable IPC A0 Execution Policy

IPC contract version `a0` is the first authored executable-pipe contract. It is
independent of the date-based C ABI generation and the numeric versions used by
existing packed planar formats.

The A0 stdio server executes one geometry request at a time. It may queue a
bounded number of parsed request descriptors, but it must not overlap OCCT or
planar operation execution until a later concurrency-safety decision provides
evidence for a bounded worker pool or process isolation.

Frames carry request identifiers and the protocol permits out-of-order
responses so concurrency can be added compatibly. The A0 serialized server
normally responds in completion order, which is execution order. Clients must
correlate by identifier rather than rely on ordering.

Cancellation in A0 applies only to queued requests. An active request is not
cooperatively cancelled; the server returns a typed `not_cancellable` outcome.
A client timeout does not imply server cancellation. Terminating the child is a
separate client escalation that fails all outstanding requests.

Graceful shutdown stops accepting requests, rejects or cancels queued work,
allows the active request to finish within the documented grace policy, flushes
its terminal frame, and exits. Forced shutdown may terminate the process and
must fail all outstanding client futures.

The transport implementation must additionally:

- put Windows stdin and stdout into binary mode;
- reserve stdout for frames and stderr for logs;
- validate header magic, generation, counts, and bounded lengths before any
  payload allocation;
- cap request, attachment, queue, and aggregate in-flight sizes;
- read exact lengths and reject truncation or trailing frame data;
- serialize complete frame writes through one writer so frames cannot
  interleave;
- flush each complete response/control frame; and
- define behavior for broken pipes and partial writes.

### Diagnostic Contract

Wire diagnostics use stable namespaced string codes rather than exposing the
current local integer statuses as a cross-operation taxonomy. Each diagnostic
defines:

- category: `transport`, `contract`, or `operation`;
- stable string code;
- machine-readable path using one selected syntax;
- retryability;
- operation and request identity when available; and
- a human message whose exact prose is not a compatibility contract.

Wire diagnostic paths use RFC 6901 JSON Pointer. An empty pointer identifies
the whole document; operation failures without a meaningful document location
omit the path. Existing C ABI integer return values remain through adapters and
map to diagnostics where possible. They do not become stable wire codes by
numeric conversion.

### Conformance Comparison Oracles

Every vector manifest entry declares one assertion lane and comparison policy:

- `strict_json`: raw UTF-8 bytes with exact accept/reject behavior, including
  duplicate keys and malformed numbers;
- `schema`: structural accept/reject against the generated root schema;
- `semantic`: decoded result comparison after a declared projection;
- `diagnostic`: exact category, string code, path, and retryability, with
  message text nonbinding unless explicitly requested;
- `canonical_serialization`: exact bytes under RFC 8785 JCS for only the
  vectors that require a canonical wire digest; or
- `transport_framing`: exact header and payload bytes.

Normal generated encoder output is not assumed byte-identical across languages.
Semantic result vectors compare strings, enums, booleans, integer counts, and
topology structurally; floating-point fields declare absolute and relative
tolerances. Semantic comparison normalizes negative zero only when the vector
declares it. Nondeterministic fields such as operation timings are excluded by
an explicit projection listed in the vector, never silently ignored by a
global comparator.

## Intended Consumer Experience

TypeScript browser consumers should call a typed client and never manually use
`ccall`, `_malloc`, `_free`, pointer-out parameters, or copies of the planar
binary encoders. The generated client should be transport-neutral above a small
adapter boundary so the same operation API can support full browser WASM,
planar-only browser WASM, and a Web Worker.

The maintained HLR/model viewer and planar ring solver demos must be authored
as TypeScript consumers of this generated client. Standalone HTML under
`dist/wasm/demos` may remain generated distribution output, but its behavior
must be built from the same typed sources. Demos are production-facing proof of
the intended integration path, not alternate handwritten WASM bindings.

The model-bounds pilot will add a small TypeScript browser example that uses the
generated client. Existing HLR and planar demos are not gates for that pilot;
they migrate when their respective operation contracts are promoted. Final
plan exit still requires every maintained browser demo to use the generated
interfaces.

Rust native consumers should receive generated Serde-compatible models and a
client that can spawn or connect to `geometer`, negotiate protocol
capabilities, submit correlated requests through the bounded serialized A0
server, transfer raw binary attachments, and report typed transport or
operation failures. The client API may allow multiple outstanding requests,
but it must not imply parallel geometry execution or active-request
cancellation. Consumers should not build JSON property trees or parse ad hoc
stdout text.

Python consumers must retain the documented `geometer` import and
executable-backed behavior. Generated contract models and codecs should own
request/result structure internally and should back public result objects where
compatibility permits. Existing convenience classes, mappings, and call
signatures may remain as thin adapters, but they must not continue as an
independent handwritten structural authority.

## Initial Contract Inventory

The first manifest must inventory every existing public contract before any
promotion claim:

- release version, C ABI generation, and future IPC protocol generation;
- common status and diagnostic information;
- model format, transform, and source metadata;
- model-bounds options and `geometry.model_bounds.a0` result;
- HLR projection options and `geometry.projection.b0` result;
- STEP-to-GLB options and GLB attachment result;
- `geometry.planar_step.request.a0` and STEP attachment result;
- `geometry.planar_batch_solve.a0` semantic input and result;
- planar triangulate semantic input and result;
- Clipper2 boolean and inflate-open semantic inputs and results;
- `geometer.batch.request.a0` and `geometer.batch.response.a0`;
- current C ABI functions and browser export sets; and
- all packed byte formats, including planar batch, triangulation, boolean, and
  inflate-open requests and responses.

The manifest also records
`geometry.analytic_planar_boolean_batch.a0` as a design candidate requested by
a named downstream consumer. A candidate is not an implemented operation, a
frozen request/result schema, or a claim that its attachment layout exists.

The inventory must identify canonical fields separately from current aliases
such as camelCase spellings, STEP-specific operation names, and transitional
planar contour forms. Compatibility behavior remains active until its owning
contract is deliberately promoted or versioned.

## Work Slices

### 1. Contract Authority And Inventory

- Write a Geometer ADR that defines TypeSpec structural authority, behavioral
  authority, generated-artifact policy, transport authority, and mechanical
  promotion rules.
- Add a requirement for generated TypeScript/WASM and Rust/executable reference
  clients, generated Python contract integration, and typed demos as
  first-class release surfaces.
- Create a promotion manifest with every contract root, current authority,
  compatibility posture, required projections, and evidence status.
- Freeze an implementation-backed inventory of C++ types, JSON keys and
  aliases, schemas, operations, C ABI symbols, WASM exports, error codes,
  attachment types, and binary layouts.
- Decide stable TypeSpec namespace, schema identity convention, generated
  package names, and output roots before generation begins.
- Ratify the generic operation C ABI, presence/default rules, serialized stdio
  policy, diagnostic taxonomy, and conformance oracle defined above.
- Write concrete generic C ABI and IPC A0 specifications, including C layout,
  ownership, framing examples, limits, state transitions, cancellation,
  shutdown, and malformed-input behavior.
- Obtain independent transport-design review of those specifications before
  TypeSpec infrastructure or server implementation begins. Resolve blocking
  findings in the specifications and record the reviewed revision.

Approval gate: accept the authority ADR, initial manifest, canonical-versus-
compatibility classification, package identities, and independent transport-
design review.

### 2. TypeSpec And Catalog Foundation

- Add a pinned Node/TypeSpec toolchain and lockfile without making it a runtime
  dependency of Geometer.
- Author TypeSpec only under a dedicated source root such as
  `src/tsp/geometer`.
- Adapt the ALX normalized-catalog idea into a generic, in-repository Geometer
  emitter. Do not depend on the ALX domain or sibling Alexandria checkout.
- Make roots and operations catalog-discovered; language generators must not
  maintain smaller handwritten root lists.
- Fail generation on unsupported or lossy constructs, missing schema
  identities, duplicate identities, or unconsumed governed declarations.
- Add generate and `--check` commands plus a stale/unexpected generated-file
  gate.
- Generate committed HTML indexes and per-contract/per-operation references
  under `docs/generated/contracts/` from the normalized catalog.
- Vendor the reviewed `appz/data_models` Wavenumber stylesheet and watermark
  into Geometer with recorded source digests. Use the selected OFL-1.1 Cousine
  font instead of the internally licensed Berkeley Mono files; lock its source,
  license, and digests, and vendor the license alongside the font. Generated
  pages use the same page structure and relative offline links without reading
  the sibling checkout.
- Test deterministic HTML, complete navigation, source/schema links, generated
  authority warnings, and desktop/narrow browser rendering.
- Integrate generation checks with existing CMake, Rack, and L99 policy without
  forcing Node at Geometer runtime.

### 3. Pilot Models And Conformance Vectors

- Complete the inventory for all public contracts, but author only the shared
  primitives, diagnostics, attachments, registry infrastructure, model-bounds
  option patch, and model-bounds result needed by the first pilot.
- Make the operation registry extensible without requiring unimplemented
  operation variants in the pilot union or generated clients.
- Add other operation DTOs and union variants only when their individual
  promotion slice begins.
- Separate release version, C ABI generation, contract generation, packed
  format version, and executable IPC protocol generation.
- Define closed canonical objects by default. Deliberately open extension data
  requires an annotated reason.
- Establish strict JSON policy, including UTF-8, duplicate-key, unknown-field,
  nullability, non-finite-number, numeric-range, and trailing-data behavior.
- Add governed raw-byte vectors with independent expectations for framing,
  strict JSON, schema structure, and operation semantics.
- Generate a nonblocking difference report against current handwritten parsers
  before selecting any compatibility cutover.
- Give every vector a manifest-declared assertion lane, comparison policy,
  nondeterministic-field projection, and numeric tolerance where applicable.

### 3A. Geom A0 Contract Alignment

- Treat the MATZ requirements packet as consumer input, not as a prescribed
  Geometer API or wire schema. Geometer owns the final generic operation,
  TypeSpec shapes, packed attachment layout, normalization rules, provenance,
  diagnostics, and implementation.
- Freeze the reviewed `geom_a0` schema, accepted Geom ADRs 0003 through 0005,
  and MATZ requirements packet in an in-repository compatibility snapshot with
  their source revision and SHA-256 digests at
  `docs/contracts/compatibility/data-models-geom-a0-2026-08-12.toml`. Geometer
  generation, builds, and tests must not read the sibling checkout.
- Compare the Geom topology-first vocabulary for points, line/circular-arc
  paths and rings, planar regions, disks, annuli, capsules, and swept paths
  against the smallest reusable Geometer TypeSpec vocabulary. Record explicit
  compatible mappings, deliberate differences, unit ownership, validation
  rules, and migration implications before freezing Geometer's shapes.
- Make the promoted Geometer TypeSpec types the structural authority for the
  operation. `data_models` may migrate to a released generated projection or
  maintain an explicit adapter afterward; it does not remain a second
  structural authority inside Geometer.
- Keep PCB features, copper semantics, layer policy, source-tool policy, and
  MATZ materialization behavior outside Geometer.

Approval gate: Geometer publishes a proposed logical shape and mapping report;
MATZ confirms that it can represent its required inputs and authoritative
analytic outputs and supplies representative fixtures. Approval freezes the
Geometer proposal, not the original consumer packet.

### 3B. Analytic Planar Boolean Design And Feasibility

- Use the stable operation identity
  `geometry.analytic_planar_boolean_batch.a0`. Generated clients expose friendly
  language names such as `analyticPlanarBooleanBatch` and
  `analytic_planar_boolean_batch` without leaking the wire suffix into every
  public helper name.
- Specify an ordered batch of union and difference stages, including normative
  add-subtract-add behavior, analytic line/circular-arc topology, holes and
  islands, deterministic winding and ordering, normalization to the governed
  coordinate grid, a maximum permitted normalization error, and fail-closed
  topology diagnostics.
- Preserve surviving analytic curve fragments, intersections, topology, and
  source provenance. Define provenance multiplicity for coincident or merged
  geometry and stable opaque identifiers for jobs, stages, regions, rings, and
  source segments.
- Govern the logical TypeSpec request/result separately from a versioned packed
  binary attachment format. Specify magic, generation, endianness, numeric
  bounds, record counts, offsets, allocation limits, malformed-input behavior,
  and deterministic canonical bytes. Raw packets travel as named attachments
  through the generic operation C ABI and executable IPC; do not add a dedicated
  `geometer_analytic_planar_boolean_batch_bytes` symbol.
- Prototype an analytic solver path before committing to production structure.
  Start with an OCCT feasibility spike covering line/arc intersections,
  ordered Boolean stages, holes/islands, arc preservation, normalization, and
  provenance recovery. Clipper2 may serve as a sampled area/topology oracle,
  but its polygonized output cannot be authoritative. Record an ADR-level
  solver decision or stop the promotion if the spike cannot meet the invariants.
- Decide whether the existing planar-only browser artifact will export the
  generic operation ABI or whether this operation requires the full browser
  artifact. If that choice changes the digest-locked generic transport packet,
  reopen its independent design-review gate before implementation.
- Define assertion lanes for exact canonical bytes, structural topology,
  toleranced geometry where expressly allowed, diagnostics, transport framing,
  and native/WASM semantic parity. Include fixtures for line-only and arc-heavy
  shapes, disks, annuli, capsules, swept paths, nested holes/islands, tangent and
  coincident boundaries, normalization collisions, malformed batches,
  add-subtract-add ordering, provenance multiplicity, and large-board stress.
- Set measurable batch latency, memory, and payload limits from representative
  MATZ fixtures without embedding MATZ policy in the library.

Design gate: MATZ reviews the proposed contract and packet description against
its behavioral requirements, while Geometer independently accepts the solver,
determinism, diagnostics, bounds, and transport design. No consumer review can
waive Geometer's genericity or release gates.

### 3C. Analytic Synthetic Correctness And OCCT Qualification

- Build a closed-form corpus for line/arc rings, rectangles, disks, annuli,
  capsules, swept paths, nested holes/islands, tangency, coincidence, complete
  subtraction, and disconnected results. Expected topology and normalized
  analytic geometry must come from mathematics independent of OCCT output.
- Check structural invariants for shared endpoints, replayable arc
  endpoint/radius/direction/branch data, winding, containment, Euler
  characteristic, absence of dangling/self-intersecting topology, and
  independently certified analytic area/perimeter of the published normalized
  replay geometry.
- Add metamorphic transformations for same-stage operand permutation, geometry
  projection under id renaming, integer translation, exact 90-degree rotation,
  reflection with direction/winding recanonicalization, valid integer scaling,
  identity/self operations, and standalone-versus-mixed-batch equivalence.
  Stage splitting compares geometry only when mathematically equivalent and
  may legitimately change lineage. Scaling applies only while the transformed
  case remains valid under the fixed 1-nm/error rules.
- Sweep parameters below, at, and above tangency, coincidence, half-grid ties,
  normalization collisions, vanishing notches/holes, collinearity, 0/180/360
  degree arc boundaries, and permitted swept-area self-overlap. Each case has
  an exact result or exact fail-closed diagnostic, never a tolerance-based
  disposition.
- Exhaustively enumerate a bounded small integer domain and run larger
  reproducibly seeded property tests. Every failure records its seed and is
  minimized into a committed regression vector.
- Test lineage independently: absorbed/coincident positives, partial and
  complete later removal, surviving and overwritten subtraction, refill, and
  many-to-many operand/result associations.
- Use closed-form solutions, the selected exact algebraic predicates, and
  independently implemented certified line/arc area formulas as authoritative
  oracles. Secondary geometry engines and dense raster/sampling comparisons are
  differential warnings only.
- Run critical mutation sentinels that reverse arcs, change tie policy,
  normalize between stages, omit lineage, change operand order, corrupt
  hierarchy, or mishandle tangency, and require the suite to detect each.
- Pull requests run the closed-form corpus, minimized regressions,
  deterministic seed subset, and critical mutations. Nightly runs bounded
  exhaustive enumeration, larger seeds, and differential warnings. Release
  runs all frozen vectors through C++, native/WASM, IPC, TypeScript, Rust, and
  Python with separate semantic, diagnostic, and canonical-byte assertions.
- Compare id-renamed cases with a geometry-only projection or an explicit
  renaming map because provenance bytes and digests may change. Empty-operation
  properties use the governed zero-operand-stage semantics.
- Freeze the canonical exact expression-DAG node set, child ordering,
  integer/algebraic payload encoding, structural interning, and byte encoding
  before exact-backend implementation can satisfy its feasibility gate. Assert
  byte-identical expression keys natively and under Emscripten.
- Include canonical provenance vectors whose interned source sets overlap,
  including `{A, B}` and `{A, C}`, and prove that source-reference-index
  indirection produces unique bytes and round-trips every set.
- Include deterministic IPC shutdown vectors for every
  `activeRequestCompleted` case: no active request, completion winning before
  draining, and an active-at-transition request completing during grace.
- Qualify exact upstream OCCT tags `V8_0_0` and `V8_0_1` side-by-side before
  production solver work or golden freeze. Include bounded timeout regressions
  for fixed Boolean hangs plus all existing STEP, HLR, GLB, planar, CLI,
  Python, native, and WASM suites. Retest the local Emscripten install-rule
  patch. Accept and cache V8_0_1 only with reviewed evidence; otherwise retain
  V8_0_0 with the rejecting fixture. Never substitute upstream master.

### 4. Generated C++ And Native Server Boundary

- Generate C++17 value types and strict structural codecs from the normalized
  catalog, integrated into Geometer's existing CMake targets.
- Treat generated C++ values as wire DTOs and map them explicitly into focused
  native geometry value APIs; do not replace public value structs, move
  geometry behavior into generated code, or grow a catch-all module.
- Introduce one typed operation registry shared by executable dispatch and
  client metadata generation.
- Add the generic operation/attachment C ABI and use it for the model-bounds
  native and browser pilot.
- Generate or verify the C ABI declaration/export manifest and ownership
  documentation from reviewed operation metadata.
- Remove the duplicated full/planar C ABI support implementation by extracting
  a shared transport support module where practical.
- Preserve existing exported symbols until a separately approved C ABI break.

### 5. TypeScript And Browser/WASM Client

- Generate documented TypeScript types and strict JSON codecs for every
  promoted root and operation.
- Generate typed attachment descriptors for STEP input, GLB/STEP output, and
  planar byte payloads without base64 encoding.
- Implement a small WASM transport adapter that owns allocation, copying,
  pointer-out handling, error-string decoding, and freeing.
- Expose a typed high-level client shared by direct browser WASM and Web Worker
  integrations, with capability negotiation and actionable version errors.
- Generate the Emscripten exported-function list or verify it mechanically
  against the operation catalog and C header.
- Package the generated contracts and client for normal TypeScript projects;
  finalize the package name and module formats in the authority slice.
- Prove use from a clean external TypeScript package and browser smoke test.
- Publish a Viz migration guide mapping its current factories, manifest
  capabilities, direct memory calls, STEP-to-GLB worker, and packed planar
  helpers to `@wavenumber/geometer`. Do not require Viz to migrate before the
  generated client is ready.
- Add a small model-bounds TypeScript browser example as the pilot consumer of
  the generic operation ABI and generated high-level client.
- Establish the pinned, deterministic TypeScript build used by later demo
  migrations, and inventory every maintained WASM demo with its owning
  operation.
- Keep focused low-level ABI validation tests where they add coverage, but do
  not use low-level tests as justification for direct C ABI calls in demos.

### 6. Rust And Executable Pipe Client

- Generate Rust models and strict Serde-compatible codecs for every promoted
  root and operation.
- Add a persistent executable mode, provisionally `geometer serve --stdio`,
  with stdout reserved exclusively for protocol frames and logs on stderr.
- Use binary-safe framed messages rather than newline-delimited JSON. The
  framing design must define magic, protocol generation, frame kind, request
  identifier, lengths, limits, endianness, and attachment correlation.
- Carry TypeSpec-generated JSON envelopes and raw STEP, GLB, STEP-output, or
  planar attachments in separate length-delimited frame sections rather than
  base64.
- Add a startup handshake reporting release version, ABI generation, IPC
  protocol generation, supported operation identities, and capabilities.
- Implement the serialized execution, bounded queue, queue-only cancellation,
  binary-mode I/O, allocation limits, atomic frame writes, flushing, and
  shutdown rules from the approved IPC A0 policy.
- Generate a Rust client facade plus executable discovery, spawn, handshake,
  request correlation, timeout, queue-cancellation, shutdown, stderr capture,
  child-termination escalation, and unexpected-exit helpers.
- Start with one async implementation if Tokio is accepted as the client
  runtime dependency. Add a synchronous facade only when a named consumer
  requires it; do not maintain two transports speculatively.
- Prove the packaged crate from a clean external Rust consumer against the
  platform executable in `dist/native/<platform>/`.

### 7. Generated Python Contract Integration

- Audit every public Python function, class, accepted mapping form, return
  wrapper, exception, and executable-discovery behavior before selecting the
  generated integration boundary.
- Generate Python models and strict codecs from the complete normalized
  catalog. The generator and runtime library choice require an explicit
  dependency, package-size, Python-version, and wheel-install analysis.
- Use generated models/codecs to construct executable requests and decode
  executable responses. Public APIs must not decode promoted results directly
  into unvalidated `dict[str, Any]` payloads.
- Preserve public names and call signatures through aliases, wrappers, or
  conversion adapters where compatible. Record unavoidable behavior or type
  changes before promotion rather than silently changing them.
- Decide per public result whether the generated class is exposed directly or
  wrapped by the existing convenience class. Either choice must retain one
  structural authority and a documented migration path for typing consumers.
- Allow legacy mapping inputs only at documented compatibility boundaries;
  normalize and validate them through generated contracts before IPC.
- Include generated models, codecs, schemas, and typing metadata in the wheel,
  and prove imports plus round trips from a clean installed environment.
- Keep the executable as the supported backend unless a separate ADR changes
  that decision.

### 8. Model-Bounds Pilot, Analytic Boolean, And Remaining Promotions

Promote one thin vertical operation first. `model_bounds` is the preferred
pilot because it exercises model bytes, options, a structured result, errors,
WASM, and executable IPC without GLB output complexity.

For each operation:

1. accept its canonical intent and compatibility posture;
2. generate all required projections;
3. pass shared C++, TypeScript, Rust, and Python vectors;
4. pass a real native executable and browser WASM round trip;
5. migrate the examples and demos that exercise that operation to TypeScript
   and the generated high-level client;
6. regenerate any affected standalone `dist/wasm/demos` artifact and verify it
   remains self-contained;
7. switch production dispatch and clients to the generated contract;
8. replay named downstream compatibility snapshots affected by the operation;
9. delete displaced handwritten structural parsing and serialization; and
10. mark the operation promoted in the manifest with digest-checked evidence.

The first new capability after the complete `model_bounds` infrastructure pilot
is `geometry.analytic_planar_boolean_batch.a0`. Its promotion must prove the
separately governed packed format, generated TypeScript, Rust, and Python
consumption, native/browser canonical parity, executable IPC behavior, and the
frozen MATZ fixtures. It may then ship in a focused additive tagged Geometer
release without waiting for legacy HLR, GLB, or sampled planar contracts and
demos to migrate.
The existing sampled planar solver, triangulation, and Clipper2 APIs remain
supported and unchanged.

The Geometer/MATZ release handshake is:

1. Geometer proposes the TypeSpec operation, analytic topology, packed format,
   normalization, provenance, and diagnostics.
2. MATZ reviews that proposal against its frozen requirements and supplies
   representative fixtures; it does not dictate the wire schema.
3. Geometer proves canonical native/WASM parity and generated TypeScript, Rust,
   and Python consumption, plus executable IPC behavior.
4. Geometer publishes an additive tagged release after the focused native,
   WASM, package, documentation, Rack, and L99 gates pass.
5. MATZ pins the tagged release, replaces its solved-copper Boolean replay with
   the generated client, and records a released compatibility snapshot.
6. MATZ's solved-copper promotion remains blocked until that released
   integration passes. Candidate artifacts may be used for pre-release tests,
   but not for its production/default switch.

Suggested order after the focused analytic Boolean release:

1. HLR projection;
2. STEP-to-GLB;
3. planar batch solve;
4. triangulation;
5. Clipper2 boolean and inflate-open; and
6. planar STEP.

The embedded model viewer and HLR worker migrate with HLR projection. The
planar ring solver migrates with planar batch solve. If the demo inventory
finds another maintained demo, its owning operation must be recorded before
promotion begins. A final demo-closure audit verifies that no maintained demo
still contains inline application JavaScript, a handwritten `.js` worker, or
direct C ABI pointer marshalling.

The existing file-based `run` command and Python wrapper remain supported
during this work. Their relationship to the typed operation registry must be
implemented through generated contract models and explicit compatibility
adapters or a separately versioned replacement, never two ungoverned
authorities.

Viz is a named downstream compatibility consumer. Until its TypeScript upgrade
lands, Geometer must keep the frozen 2026.6.10 full/planar factory names,
Emscripten runtime helpers, required per-operation C ABI exports, ownership
functions, vendor-manifest capability meanings, and packed planar versions
working. Geometer-side snapshot tests run on every affected change; final exit
also requires an integration run using a candidate Geometer artifact in a
temporary Viz test workspace. Viz migrates later to `@wavenumber/geometer`
without forcing legacy pointer code into the generated client.

MATZ is a named requirements and adoption consumer for analytic planar Boolean,
not a Geometer build dependency. Geometer protects the frozen Geom/MATZ input
snapshot during design, then replaces it with a released-integration snapshot
only after MATZ pins and passes the tagged release.

## Documentation And Compatibility Closure

Documentation is part of the shipped interface and is required for each
operation promotion as well as final plan exit.

For each promoted operation:

- update the authority ADR, promotion manifest, requirements, and relevant
  maintained files under `docs/design`;
- update C++, C ABI, WASM, CLI, executable IPC, binary-format, and JSON-format
  documentation when that surface is affected;
- update developer setup and generation/check commands in
  `docs/developer/README.md` and any focused toolchain guide;
- regenerate contract schemas, catalogs, API references, and package
  documentation from their authored sources;
- regenerate the styled HTML contract site and verify its vendored stylesheet,
  font/logo assets, navigation, generated markers, and offline links;
- update the applicable TypeScript/WASM, Rust/executable, Python, and native
  consumer guides and runnable examples;
- record the compatibility result for existing symbols, commands, Python
  signatures/types, accepted wire inputs, and persisted artifacts;
- document aliases and adapters that remain, deprecations introduced, removed
  behavior, and the supported migration path;
- update `CHANGELOG.md`, release notes, and version/ABI/IPC-generation notes
  when the change affects a published surface; and
- verify all links, commands, paths, examples, schema identities, operation
  identities, and version examples against the artifacts that actually ship.

The final design-doc intent audit must find no stale handwritten contract table
that conflicts with TypeSpec or generated references, no undocumented public
compatibility change, and no generated artifact presented as an authored source
of truth.

## Verification Matrix

Each promoted operation requires:

- TypeSpec compile with warnings as errors;
- generated catalog/schema/document freshness;
- generated HTML deterministic-output, complete-navigation, relative-link,
  shared-style digest, and offline desktop/narrow browser checks;
- exact root and operation inventory checks;
- strict positive and adversarial JSON vectors with manifest-declared lanes and
  comparison oracles;
- generated C++ compile and CTest coverage;
- TypeScript typecheck, codec tests, package-consumer compile, and browser or
  Web Worker WASM smoke;
- TypeScript compilation and browser smoke for the examples and demos
  applicable to the operation being promoted, plus a check that their source
  does not reproduce C ABI pointer marshalling;
- Rust format, lint, unit/conformance tests, crate-consumer compile, and live
  executable-pipe smoke;
- Python generation freshness, strict codec vectors, public API compatibility
  tests, static typing, installed-wheel import, and executable round trips;
- native versus WASM versus executable result-equivalence checks where output
  determinism permits them;
- malformed/truncated/oversized frame and attachment tests;
- Windows binary-mode, bounded-allocation, atomic-write, and flush tests;
- serialized execution, queue saturation, queue-only cancellation, graceful
  and forced shutdown tests;
- process crash, timeout, unsupported-operation, and protocol-version tests;
- documentation generation/freshness, link, command, example, and
  compatibility-note checks;
- `scripts/validate_native.py`, `scripts/validate_python_package.py`, CTest,
  Rack, WASM validations, and `tests/L99_release`; and
- regenerated committed `dist/` artifacts whenever the shipped interfaces or
  runtime change;
- the frozen Viz compatibility snapshot on every C ABI, Emscripten export,
  packed planar format, WASM artifact, or ownership change; and
- a candidate-artifact Viz vendoring and targeted browser/integration smoke
  before final compatibility signoff;
- analytic planar Boolean exact-topology, ordered-stage, winding, provenance,
  normalization-error, fail-closed diagnostic, and canonical-byte vectors on
  native, full or planar browser WASM as selected, and executable IPC; and
- candidate-artifact MATZ integration followed by a tagged-release pin and
  solved-copper integration gate before the MATZ compatibility snapshot is
  marked released.

Final plan exit additionally requires TypeScript compilation and browser smoke
for every maintained demo in the frozen inventory.

New Python tests must be registered in `tests/python/STRATUM.toml`. New
TypeSpec, TypeScript, Rust, IPC, and cross-language suites must receive explicit
Rack strata rather than hiding inside a general release test.

## Non-Goals

- Adding Altium, board, visualizer, or other application policy to Geometer.
- Making `appz/data_models` or `alexandria` a Geometer runtime or build
  dependency.
- Making `appz/viz` a Geometer build dependency; Viz compatibility is verified
  from a frozen in-repository snapshot plus an explicit downstream integration
  run.
- Making `appz/data_models` or MATZ a Geometer build dependency, copying PCB
  policy into Geometer, or treating the MATZ requirements packet as a frozen
  request/result wire schema.
- Reimplementing geometry algorithms in generated code.
- Encoding large model and result blobs as base64 JSON.
- Replacing efficient packed planar transport solely to make it resemble JSON.
- Breaking the current C ABI, CLI, Python package, or browser exports without a
  separate version and migration decision.
- Generating languages beyond C++, TypeScript, Rust, and Python before a named
  consumer requires them.
- Treating generated types alone as proof of strict wire conformance.

## Accepted Direction And Remaining Decisions

ADR-010 accepts the authority boundary and required projections. ADR-011 and
its transport specifications have passed the independent review gate. Package,
generator, compatibility, and packed-layout choices below must be resolved no
later than their owning implementation slice.

1. Approve TypeSpec plus the normalized catalog as structural authority for
   individually promoted Geometer contracts.
2. Approve TypeScript/WASM, Rust/executable, and Python structural code as
   required production projections.
3. Approve closed canonical contracts and explicit compatibility adapters as
   the default posture.
4. Approve the additive generic operation/attachment C ABI for generated
   native and browser clients while retaining existing per-operation symbols.
5. Approve the presence-aware option-patch and post-merge defaulting semantics.
6. Approve the serialized, bounded, binary-safe stdio A0 execution and
   queue-only cancellation policy while retaining file-based CLI compatibility.
7. Approve the diagnostic categories and RFC 6901 JSON Pointer wire path
   syntax.
8. Approve the vector assertion lanes and exact, structural, and toleranced
   comparison rules.
9. Accept the independent early transport-design review and reviewed spec
   revision before implementation begins.
10. Explicitly approve generated Python and public-Python compatibility work as
   mandatory promotion scope.
11. Select the TypeScript package identity/module formats, Rust crate
   identity/runtime dependency policy, and Python generator/runtime library.
12. Approve the Python compatibility and generated-model exposure analysis as a
   gate before changing public Python types.
13. Approve the TypeScript demo source/build layout and the exact maintained
   demo inventory.
14. Approve `model_bounds` as the first vertical promotion pilot using the
    generic operation ABI.
15. Keep packed layouts separately governed from TypeSpec logical models. This
    plan includes the analytic planar Boolean packet specification and may add
    deterministic layout generation only after its design proves that generated
    output preserves the independently reviewed binary contract.
16. Preserve the accepted Viz 2026.6.10 compatibility snapshot and replace it
    only after its TypeScript client migration passes.
17. Use the accepted digest-tracked Wavenumber documentation stylesheet,
    OFL-1.1 Cousine font assets, and watermark for generated Geometer HTML
    references. Do not vendor the internally licensed Berkeley Mono binaries.
18. Accept `geometry.analytic_planar_boolean_batch.a0` as the first new
    operation after the model-bounds infrastructure pilot, with authoritative
    line/circular-arc topology and no early polygonization.
19. Treat the frozen Geom A0 and MATZ packet as compatibility and requirements
    inputs. Geometer owns the final generic TypeSpec and packet shape; MATZ
    reviews it, provides fixtures, and migrates only after an additive tagged
    release.
20. Carry analytic request/result packets through named generic attachments;
    do not add an operation-specific C ABI bytes symbol.

## Completion And Plan Hygiene

This plan remains temporary working state. When implementation ships, durable
decisions and interface truth must live in code, generated contracts, ADRs,
requirements, design documentation, and conformance evidence. Delete this plan
after all accepted work is complete; do not retain it as a substitute for the
docs of record. Close GitHub issue #18 only after its acceptance criteria pass
or every explicitly deferred item is represented by a linked follow-up issue.

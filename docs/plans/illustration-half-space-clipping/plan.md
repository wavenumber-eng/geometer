+++
type = "plan"
id = "illustration-half-space-clipping"
status = "active"
created = "2026-09-19"

[[steps]]
id = "contract-semantics"
title = "Specify normalized half-space, empty-fragment, no-cap, limit, and identity semantics"
status = "done"

[[steps]]
id = "native-clip-kernel"
title = "Implement deterministic material-preserving triangle clipping as a focused native module"
status = "done"
depends_on = ["contract-semantics"]

[[steps]]
id = "model-pipeline"
title = "Clip transformed STEP and analytic model sources before bounds, HLR, and illustration"
status = "done"
depends_on = ["native-clip-kernel"]

[[steps]]
id = "composed-mesh-pipeline"
title = "Apply the same clipping semantics to composed mesh illustration, geometry, and HLR"
status = "done"
depends_on = ["native-clip-kernel"]

[[steps]]
id = "generated-surfaces"
title = "Regenerate and verify C++, Rust, Python, TypeScript, schemas, catalog, and docs"
status = "done"
depends_on = ["model-pipeline", "composed-mesh-pipeline"]

[[steps]]
id = "visible-demo"
title = "Expose clipping in the existing browser and egui Illustration Labs"
status = "done"
depends_on = ["generated-surfaces"]

[[steps]]
id = "first-party-consumer-migration"
title = "Move maintained apps, web pages, examples, benchmarks, and primary tests to B0"
status = "done"
depends_on = ["generated-surfaces"]

[[steps]]
id = "design-doc-intent-audit"
title = "Update and audit durable design and consumer documentation against implementation"
status = "done"
depends_on = ["generated-surfaces", "visible-demo", "first-party-consumer-migration"]

[[steps]]
id = "test-runtime-impact-audit"
title = "Audit clipping test placement and runtime impact"
status = "done"
depends_on = ["generated-surfaces", "visible-demo"]

[[steps]]
id = "external-review"
title = "Obtain independent implementation and contract review"
status = "done"
depends_on = ["design-doc-intent-audit", "test-runtime-impact-audit"]

[[steps]]
id = "qualification"
title = "Pass focused native, transport, SDK, WASM, Rack, and release validation"
status = "active"
depends_on = ["external-review"]

[[steps]]
id = "shared-release-handoff"
title = "Hand the qualified issue 40 implementation to the unified release candidate"
status = "pending"
depends_on = ["qualification"]

[[exit_criteria]]
id = "shared-contract"
title = "One generated clipping contract governs model, analytic, composed-mesh, and HLR paths"
status = "met"

[[exit_criteria]]
id = "geometry-correctness"
title = "Clipping is deterministic, bounded, material-preserving, and transform-correct"
status = "met"

[[exit_criteria]]
id = "explicit-empty-state"
title = "Empty fragments succeed with an explicit empty state and no fabricated bounds"
status = "met"

[[exit_criteria]]
id = "transport-parity"
title = "Supported native, IPC, static SDK, Rust, Python, and WASM paths agree"
status = "pending"

[[exit_criteria]]
id = "visible-demo"
title = "The browser and egui Illustration Labs visibly exercise an adjustable clipping plane"
status = "met"

[[exit_criteria]]
id = "first-party-consumer-migration"
title = "Every maintained first-party illustration consumer uses B0 except explicit A0 compatibility tests"
status = "met"

[[exit_criteria]]
id = "design-doc-intent-audit"
title = "Durable design and consumer documentation matches the implemented behavior"
status = "met"

[[exit_criteria]]
id = "test-runtime-impact-audit"
title = "New clipping tests are assigned to appropriate strata and runtime impact is reviewed"
status = "met"

[[exit_criteria]]
id = "external-review"
title = "Independent contract and implementation review is complete"
status = "met"

[[exit_criteria]]
id = "issue-40"
title = "The capability is released and Geometer issue 40 is closed"
status = "pending"
+++

# Generic Illustration Half-Space Clipping

This plan implements [Geometer issue 40](https://github.com/wavenumber-eng/geometer/issues/40).
It is a release-signoff dependency of the
[supported static SDK and unified-transports plan](../geometer-static-sdk-unified-transports/plan.md).
The downstream coordination record is the Altium Cruncher issue 67 Toon plan;
Geometer remains unaware of boards, mounting sides, footprints, and application
cache policy.

The durable record after completion is the authored contract, generated
bindings and schemas, focused native implementation, tests, design documents,
demo artifacts, release notes, and released package. Retire this temporary plan
after those records are complete and issue 40 is closed.

## Independent plan review disposition

An independent pre-implementation review initially required changes. The
review's three blocking findings are incorporated here:

- strict A0 contracts remain unchanged and clipping uses generated B0 successor
  operations with explicit compatibility vectors;
- composed mesh HLR B0 consumes the same mesh collection as illustration and
  correlates a separate canonical geometry-only digest while preserving raw
  attachment identity; and
- issue 40 qualification feeds the unified release candidate rather than
  requiring an earlier release that would form a dependency cycle.

The post-implementation independent review approved the current tree after the
B0 HLR/digest documentation, six-archive public attestation loop, and canonical
unqualified B0 mesh-HLR client migration were corrected. Its independent checks
covered generated contracts, focused Python and native suites, browser WASM,
Rust validation, formatting, and diff hygiene.

Implementation and Windows x64 SDK/demo qualification are complete. Focused
geometry remains in CTest, contract/client checks remain in their existing Rack
strata, and no network or SDK rebuild was added to a fast unit stratum. The only
issue-40 qualification still open is the macOS ARM64 relocated SDK/demo build,
governed digest parity, archive validation, and executable smoke run. Release
promotion and issue closure remain pending until that external matrix leg and
the shared release gates pass.

The review also required exact canonical numeric rules, inverse-transpose normal
and mirrored-winding behavior, a per-result empty-state matrix, Fast-only B0
mesh HLR scope, explicit attribute/compaction semantics, independent clipping
work budgets, cross-platform identity vectors, and a definite two-platform demo
release decision. Those requirements are included below. A follow-up review
approved this revision for implementation with no remaining blocking or
high-priority findings.

## Goal

Clip placed three-dimensional illustration geometry against an ordered
intersection of world-space half-spaces before any bounds, HLR, outline,
visibility, shading, SVG, or renderer-neutral geometry result is produced.
Every artifact returned for one request must describe the same surviving
fragment.

The kept side of each plane is:

```text
dot(normal, point) - distance_mm >= -tolerance_mm
```

Plane coordinates are millimeters in the world frame after STEP root placement
and every supplied source, occurrence, or mesh affine transform.

## Non-goals

- Do not add PCB-, board-, Altium-, KiCad-, Toon-, or side-selection policy.
- Do not reconstruct or clip meshes in Python, Rust, or a downstream application.
- Do not implement profile-prism occlusion or the union of an XY footprint test
  with a half-space test.
- Do not synthesize section faces in the first version.
- Do not expose OCCT types or create transport-specific clipping APIs.
- Do not accept supplied linework that identifies an unclipped or differently
  clipped fragment.

## Contract

Preserve every strict A0 request, result, operation identity, codec, and behavior
unchanged. A0 decoders reject unknown fields, so adding required empty/fragment
metadata or optional bounds to A0 would break both old clients reading new
results and new clients reading old results.

Treat issue 40 as one coordinated B0 logical-contract generation. Author one
shared TypeSpec clipping model and B0 successors for:

- `geometry.model_illustration.b0`;
- `geometry.model_illustration_geometry.b0`;
- `geometry.mesh_illustration.b0`;
- `geometry.mesh_illustration_geometry.b0`; and
- `geometry.mesh_hlr_projection.b0`.

Also version the carried renderer-neutral and linework payloads as
`geometry.mesh_illustration.geometry.b0` and
`geometry.hlr_projection.result.b0`. B0 geometry attachment descriptors name
the B0 geometry schema, and direct-value B0 geometry returns that same payload.
Its required `empty`, clipping/fragment metadata, optional projected bounds,
stats, and warnings must agree exactly with the outer B0 operation result. B0
mesh HLR attachments carry the B0 HLR result. A0 model and indexed-mesh HLR
results and A0 illustration geometry payloads remain unchanged.

Apply the generation only to serialized contract roots: complete operation
requests/results, generic outcome envelopes, attachment payloads, and binary
formats when their bytes change. Plane, clipping-options, limits,
normalized-plane, fragment-metadata, and empty-state models are ordinary nested
members of the B0 request/result namespace. They receive no independent schema
identity or generation merely because code generators emit named language
types. Do not create an A1 family or a separately versioned leaf-type graph.

The generated clients expose the B0 methods alongside A0; this is contract
versioning, not a transport-specific or handwritten parallel API. Each B0
request has an optional `clipping` object. Absence selects unclipped legacy
geometry semantics while still returning the unambiguous B0 result shape.
`cap_policy` is required only inside a present clipping object.

### Version and negotiation boundary

Advance every serialized logical root whose closed shape gains issue 40
variants. Add `geometer.operation.outcome.b0` for B0 operation responses and a
B0 `geometer.ipc.request.b0` executable-IPC envelope for the B0 request union.
Retain `geometer.operation.outcome.a0`, `IpcRequestA0`, and their exact codecs
and vectors unchanged. The generic dispatcher selects the request and outcome
codec from the complete operation identity; it never attempts to decode a B0
request or result as A0.

The complete changed-root inventory is:

| Public surface | Operation/interface identity | Request or input root | Result or payload root |
| --- | --- | --- | --- |
| Model SVG operation | `geometry.model_illustration.b0` | `geometry.model_illustration.request.b0` | `geometry.model_illustration.result.b0` |
| Model geometry operation | `geometry.model_illustration_geometry.b0` | `geometry.model_illustration_geometry.request.b0` | `geometry.model_illustration_geometry.result.b0`; attachment `geometry.mesh_illustration.geometry.b0` |
| Composed-mesh SVG operation | `geometry.mesh_illustration.b0` | `geometry.mesh_illustration.request.b0` | `geometry.mesh_illustration.result.b0` |
| Composed-mesh geometry operation | `geometry.mesh_illustration_geometry.b0` | `geometry.mesh_illustration_geometry.request.b0` | `geometry.mesh_illustration_geometry.result.b0`; attachment `geometry.mesh_illustration.geometry.b0` |
| Composed-mesh Fast HLR operation | `geometry.mesh_hlr_projection.b0` | new dedicated `geometry.mesh_hlr_projection.request.b0` | `geometry.hlr_projection.result.b0` |
| Direct-value mesh SVG interface | no dispatched operation | `geometry.mesh_illustration.input.b0` | `geometry.mesh_illustration.result.b0` |
| Direct-value mesh geometry interface | no dispatched operation | `geometry.mesh_illustration_geometry.input.b0` | `geometry.mesh_illustration.geometry.b0` |
| Generic operation result JSON | generic dispatcher interface | selected operation request root above | `geometer.operation.outcome.b0` |
| Executable IPC request JSON | executable dispatcher interface | `geometer.ipc.request.b0` | selected `geometer.operation.outcome.b0` |

The dedicated mesh-HLR B0 request embeds the needed Fast HLR/view settings and
clipping object; it does not add clipping to or widen the shared
`geometry.hlr_projection.options.a0`. Direct-value B0 inputs embed the unchanged
mesh value plus clipping and other illustration settings rather than changing
`geometry.mesh_illustration.input.a0` or
`geometry.mesh_illustration_geometry.input.a0`.

| Unchanged surface | Decision |
| --- | --- |
| Plane, clipping, limit, and fragment metadata models | Nested in B0; no independent identity |
| Existing illustration style/view/preparation models | Reuse their unchanged current identities |
| `geometry.mesh_collection.a0` composed-mesh attachment | Remains A0 |
| Executable IPC binary framing and Worker messages | Remain A0; carry operation-selected opaque JSON |
| Operation-catalog format and generic operation ABI | Remain A0; catalog digest and declarations change |
| Existing packed formats | Retain their current identities |
| Package and C ABI release identity | Advance date version/generation |

Do not bump a containing interface merely because its opaque request/result JSON
can carry a newly versioned operation. Executable IPC keeps the existing A0
binary frame format, hello/welcome lifecycle, bounds, attachment sections, and
single-worker semantics. The Worker message protocol also remains A0 because it
already carries an operation identity plus opaque request/outcome JSON strings.
B0-aware executable, direct-WASM, Worker, Rust, Python, and static clients choose
the B0 codec from the operation identity. A0 operation calls continue to use A0
request and outcome roots.

The normalized operation-catalog file format and generic operation ABI remain
A0 because their schemas and bytes do not change. Their exact catalog digest and
operation declarations advertise the added B0 operation and request/result
identities. Existing mesh-collection input and packed attachment formats keep
their current identities when their bytes are unchanged. The release advances
the date-based package version and C ABI generation, regenerates every client,
and makes clients verify the catalog digest, operation request/result contract
identities, and C ABI generation before invoking B0. No client may infer B0
support solely from a package version or silently fall back to A0.

Executable IPC performs deterministic two-phase request decoding without
probing contract generations:

1. boundedly inspect the strict request object to extract exactly one valid
   `operation` member while rejecting malformed JSON, invalid UTF-8, duplicate
   keys, missing/empty/oversized operation names, and trailing bytes;
2. resolve that identity through the verified catalog declaration and compiled
   operation mapping;
3. select exactly `geometer.ipc.request.a0` or `geometer.ipc.request.b0`;
4. strictly decode the entire selected root and verify its embedded operation
   equals the identity used for selection; and
5. reject on any mismatch without trial-decoding or fallback.

Executable and Worker clients select `geometer.operation.outcome.a0` or
`geometer.operation.outcome.b0` from the outstanding request's complete
operation identity and verified catalog declaration before strict result
decoding. They reject an outcome whose embedded operation or catalog-declared
result identity disagrees with that outstanding request.

### A0 compatibility adapters and shared execution

Do not maintain separate A0 and B0 geometry, HLR, or illustration algorithms.
After strict root decoding, use generation-specific preparation adapters. The
A0 adapter preserves its frozen preprocessing order, defaults, presence
semantics, and numerical behavior; it must not pass through B0 canonicalization
merely to reduce code. The B0 adapter performs its defined affine lowering,
canonicalization, clipping, identity, and empty-state work. Once each adapter
has produced the generation's prepared geometry, share the existing Fast HLR,
visibility, shading, and rendering algorithms. Project the result through the
selected generation's result adapter: A0 calls emit the exact unchanged A0
root, while B0 calls emit the B0 root with required empty and fragment metadata.

This is an internal compatibility conversion, not wire-level coercion. Never
accept B0-only members in an A0 root, relabel A0 JSON as B0, return a B0 outcome
for an A0 operation identity, or silently retry a failed decode as another
generation. A B0 request with `clipping` absent uses legacy geometry semantics
but still returns the B0 result shape.

Where carriers differ, decode them strictly before normalization. In particular,
the A0 mesh-HLR indexed packet and the B0 colored `mesh_collection` request have
separate input adapters but converge on the same internal Fast HLR structures
and solver. The B0 adapter preserves material and identity data needed by
illustration correlation; the A0 adapter preserves its established packet
semantics.

Preserving A0 operations does not promise that an already released client can
connect to a newer executable or static library. Existing clients verify an
exact catalog digest and C ABI generation and may reject the new release before
sending a request. The supported guarantee is that generated clients from the
new release expose both unchanged A0 operations and their B0 successors; release
documentation must state the paired-client requirement explicitly.

Treat the A0 adapters as a controlled migration surface. Ship A0 and B0 together
in the issue 40 release, then update every first-party and named downstream
consumer to the released B0 operation identities and exact package/catalog
version promptly. Track that migration in the consumer compatibility inventory,
including Altium Cruncher issue 67 and every repository represented by a named
snapshot. Once the inventory contains no required A0 illustration consumer,
make deprecation or removal of those A0 operations a separate reviewed
compatibility decision. Do not remove A0 in the issue 40 release or let the
temporary adapter justify a duplicate solver/rendering implementation.

### First-party consumer migration

Make B0 the canonical unqualified illustration API in each maintained generated
or handwritten client. Generation-explicit A0 methods/codecs remain available
only as compatibility surfaces; examples and application code must not select
them accidentally. Where source-language overloads cannot expose both clearly,
use an explicit `*_a0` compatibility name and reserve the ordinary method name
for B0.

Audit every repository-owned caller of the five affected A0 operation identities
and the direct-value A0 illustration inputs. Migrate all maintained production,
demonstration, qualification, and documentation callers to B0, including:

- the browser Illustration Lab, its Worker, renderer adapters, static-site
  checks, exported metadata, and any HLR Lab path that calls affected mesh HLR;
- the native illustration lab and other maintained test applications;
- Rust executable and direct-static examples, especially
  `direct_static_illustration.rs` and `mesh_illustration.rs`;
- Python and TypeScript examples, package READMEs, quick starts, and executable-
  IPC examples;
- release SDK consumers, relocated-link smoke programs, WASM/Worker smoke pages,
  and the packaged Windows/macOS clipping demo;
- performance benchmarks, parity corpora, promotion manifests, and primary
  end-to-end illustration tests; and
- authored documentation snippets and regenerated contract/reference pages.

Retain A0 calls only in clearly named compatibility fixtures that prove frozen
A0 decoding, serialization, adapter equivalence, and result bytes. Add a
repository gate that scans non-generated maintained application/example/doc
sources for the affected A0 identities and fails unless each occurrence is in a
small reviewed allowlist of compatibility tests or historical release/research
records. Generated A0 bindings and generated A0 reference pages are expected and
are not evidence that a live consumer still selects A0.

For each migrated consumer, test or inspect the executed operation identity—not
only the method name—so an unchanged wrapper cannot silently continue dispatching
A0. Release signoff requires browser, native, Rust, Python, executable IPC,
direct static, and WASM/Worker smoke evidence that the applicable illustration
call resolved to B0 and decoded a B0 result or payload root.

The request contains:

- one to sixteen planes in request order;
- a finite, nonzero three-component normal for each plane;
- finite `distance_mm` and nonnegative finite `tolerance_mm` values;
- required `cap_policy: "none"`; and
- `max_output_triangles`, range `1..2,000,000`, default/hard ceiling 2,000,000;
- `max_generated_vertices`, range `1..6,000,000`, default/hard ceiling 6,000,000;
- `max_intersections`, range `1..8,000,000`, default/hard ceiling 8,000,000; and
- `max_edge_plane_tests`, range `1..32,000,000`, default/hard ceiling
  32,000,000.

TypeSpec encodes those inclusive ranges. Semantic validation resolves defaults
and rejects every invalid plane, count, or budget before capacity calculation,
allocation, transform, or clipping work begins. Callers may lower but never
raise the hard ceilings.

Geometer divides each normal and distance by the normal magnitude before any
classification. Tolerance is already a physical millimeter distance and is not
scaled. Canonicalize every normalized component, normalized distance,
tolerance, emitted position, and emitted normal to a `1e-12` grid using
`sign(x) * floor(abs(x) * 1e12 + 0.5) / 1e12`, and rewrite negative zero to
positive zero. Geometry evaluation and identity both use these canonical
values. Apply the complete affine transform and canonicalize every resulting
world-space source position before the first plane classification; do not defer
canonicalization until an intersection or final emission. Reject finite inputs
whose canonicalization would overflow. Positive scalings are equivalent only
when this normalization and canonicalization produce the same values; do not
promise equivalence for arbitrary floating inputs. Reversing a normal changes
the kept half-space.

For one canonical plane compute `s(point) = dot(normal, point) - distance_mm +
tolerance_mm`. Keep `s >= 0`; therefore the actual crossing surface is the
normalized signed distance `-tolerance_mm`. Retain coplanar vertices. For an
edge whose endpoints have opposite classifications, compute
`t = s0 / (s0 - s1)`, clamp `t` to `[0,1]`, and interpolate from endpoint zero.
Canonicalize the result after interpolation. Process edge endpoints in source
polygon order; stable canonicalization, not unordered-map traversal, resolves
ties.

The normalized planes, order, tolerances, cap policy, and work limits are
returned in fragment metadata. Encode identity with an explicit version prefix,
fixed-width little-endian integers, IEEE-754 binary64 canonical values, length-
prefixed UTF-8 strings, and positive-zero normalization.

Every B0 result exposes a required top-level `empty` boolean whose meaning is
source-fragment emptiness after clipping and degenerate removal, before
view-dependent back-face culling or paint suppression. It is not a statement
that a particular view produced no drawing commands.

| B0 result | Empty representation |
| --- | --- |
| Model SVG | `empty=true`; `bounds_mm` member absent; zero stats; valid canonical empty SVG |
| Model geometry | `empty=true`; `bounds_mm` absent; required geometry attachment with empty arrays |
| Mesh SVG | `empty=true`; zero stats; valid canonical empty SVG |
| Mesh geometry | `empty=true`; projected `bounds` absent; required attachment/value with empty arrays |
| Mesh HLR | `empty=true`; every requested view and mode remains present with empty arrays and absent bounds |

When `empty=false`, model 3D bounds and renderer-neutral projected bounds are
present whenever their existing nonempty contract requires them. Optional JSON
members are omitted, never encoded as `null`. Never encode six zero bounds for
an empty fragment. Define the canonical empty SVG as an exact governed vector:
it retains schema/title/background/presentation metadata and the requested
coordinate-span viewBox but contains no surface or line elements. Empty
clipping is expected control flow and emits no warning.

Fragment metadata includes the normalized clipping request, input/output
triangle counts, a deterministic full `fragment_sha256`, and a distinct
`linework_geometry_sha256`. The full digest covers canonical clipping semantics
and compacted world-space meshes including surviving IDs, normals, complete
material tables, triangle material indices, and double-sided state. The
linework digest covers the same clipping semantics and canonical positions,
topology, source-mesh order, and source-triangle lineage but excludes paint and
normal attributes. Timings, diagnostics, and view settings are excluded.

Preserve the raw input attachment SHA-256 separately from both fragment
digests. Operation request bytes plus raw attachments remain transport-cache
inputs; normalized fragment metadata gives semantic result identity.

`cap_policy: "none"` means Geometer creates no filled section triangles and
invents no section material. The exposed cut is an ordinary mesh boundary, so
Fast HLR and illustration outlines treat it exactly like other open boundaries.
A later closed-cap policy requires a separately reviewed triangulation,
orientation, UV/normal, and deterministic material-selection contract.

## Native clipping kernel

Add a small responsibility-focused C++ module operating on generated mesh
values. Do not grow `geometer.cpp`, the operation registry, or the renderer
preparation module into a clipping implementation.

For each source triangle, apply Sutherland-Hodgman clipping against planes in
request order. Preserve source mesh order and triangle order. Triangulate each
surviving convex polygon with a deterministic fan rooted at its first retained
vertex. Preserve the source triangle's material index and double-sided policy.
Interpolate new positions and available per-vertex normals with the exact edge
parameter used for the boundary intersection; normalize a nonzero interpolated
normal after interpolation. If interpolation cancels a normal to the geometry
epsilon, store a canonical zero normal so existing shading falls back to the
geometric face normal. Preserve absence of normals rather than inventing an
attribute array.

Validate affine matrices through the existing nonsingularity/conditioning
rules. Bake each mesh matrix into the clipped world-space vertices and clear
the matrix. Transform normals with the inverse transpose. Correct winding for
negative determinants for indexed and nonindexed input before clipping.

Omit fully clipped meshes. Preserve every surviving mesh ID and relative order,
retain its complete material table without compaction/reindexing, and preserve
triangle material indices. Emit compact indexed geometry with no unused
vertices, but never merge vertices across source triangle, normal, or material
seams. Canonicalized intersection positions from adjacent triangles must still
weld by position during Fast HLR preparation.

For one mesh define `scale_mm = max(1, max(abs(world_coordinate)))` and
`geometry_epsilon_mm = max(1e-12, 32 * binary64_epsilon * scale_mm)`. Collapse
consecutive polygon vertices whose squared distance is at most
`geometry_epsilon_mm^2`, including the closing pair. Remove a triangle when the
magnitude of its edge cross product is at most
`geometry_epsilon_mm^2`. This epsilon is independent of clip tolerance, view
scale, SVG pixels, line width, and other visual heuristics.

Clipping a convex source triangle by `p` half-spaces yields at most `3 + p`
polygon vertices and therefore at most `1 + p` fan triangles. Enforce the
maximum plane count before work begins; use that bound for overflow-safe
capacity preflight without requiring the conservative maximum allocation to
fit. Separately debit every generated polygon vertex, boundary intersection,
edge-plane test, and emitted triangle from the request limits before append.
Return the typed resource-limit failure on exhaustion and never return partial
geometry.

## Pipeline integration

### Model and analytic illustration

1. Import STEP and apply root-placement policy, or lower the analytic scene.
2. Apply the complete requested source/occurrence affine transform.
3. Run the shared clipping kernel.
4. Retain source-summary mesh/triangle counts as input/pre-clipping facts, then
   compute explicit empty state, optional bounds, fragment input/output counts,
   and fragment identity from the clipped collection.
5. Build Fast HLR from the same clipped collection.
6. Run illustration preparation, visibility, fusion, SVG, or geometry output
   without clipping a second time.

Material overrides happen before clipping so every surviving triangle retains
the effective source material. Multi-body inputs clip every mesh independently
and retain surviving mesh order. A completely clipped source is successful and
does not enter HLR or renderer preparation with fabricated geometry.

### Composed mesh illustration and HLR

Direct value and attachment-backed mesh illustration must invoke the same
kernel before renderer preparation. Preserve the A0 indexed-packet mesh HLR
operation unchanged. The B0 mesh HLR operation instead consumes the exact
governed colored `mesh_collection` attachment also accepted by B0 composed
illustration, applies mesh matrices and clipping once, and then projects the
result through Fast HLR. It preserves the raw collection-attachment digest and
returns the reproducible geometry-only linework digest.

When a mesh illustration request supplies an HLR attachment, require its source
identity to equal `linework_geometry_sha256`. Reject A0/legacy HLR, missing B0
identity, or any mismatch when B0 clipping is requested. Document the supported
consumer sequence: submit the same mesh collection and clipping object to B0
mesh HLR, then attach that result to B0 mesh illustration. Never combine clipped
surfaces with linework computed from another carrier or unmodified mesh.

Clipping is not added to standalone A0 model HLR. B0 mesh HLR is Fast-only and
rejects `poly` or `exact` selection rather than silently changing algorithms.
B0 model illustration continues to build its internal Fast HLR from the clipped
collection, so STEP and analytic requests remain one-pass operations.

## Verification

Fast native tests cover:

- one plane retaining, splitting, or removing a triangle;
- multiple planes and request-order determinism;
- nonunit normal normalization and equivalent-plane identity;
- positive tolerance at coplanar/noisy boundaries;
- transformed meshes, mirrored transforms, and analytic occurrences;
- indexed and nonindexed inputs;
- interpolated normals, missing normals, materials, opacity, and double-sided
  state;
- inverse-transpose normals and winding for positive, negative, singular, and
  ill-conditioned affine matrices on indexed and nonindexed meshes;
- duplicate boundary vertices, slivers, degenerates, compaction, and stable
  repeated output;
- adjacent triangles whose cut vertices weld without merging attribute seams,
  whose fan diagonals do not appear in HLR, and whose section boundary is
  continuous;
- invalid/nonfinite planes, too many planes, and triangle-growth limits;
- explicit empty success with absent bounds; and
- no generated cap triangles plus ordinary HLR/outline section boundaries.

Operation and language tests cover generated codec strictness, canonical
fragment metadata, attachment correlation, mismatched supplied HLR rejection,
STEP/analytic parity, direct/composed mesh parity, generic C ABI, executable
IPC, direct Rust static use, Python, direct browser WASM, and Worker WASM where
the catalog advertises the operation.

Add exact compatibility vectors proving all A0 requests/results, operation
outcomes, IPC frames, and Worker messages retain their previous strict bytes and
reject B0-only fields or variants. Add shared B0 catalog-selection, root-codec,
and identity vectors
that must match on Windows x64, macOS ARM64, and direct browser WASM.
Include negative vectors for B0 operation plus A0 request root, A0 operation plus
B0 request root, duplicate `operation`, embedded-operation mismatch, and an
incorrect catalog request or result identity.
For every retained A0 operation, add a compatibility vector proving strict A0
decoding and frozen preprocessing remain unchanged, while the corresponding B0
request with `clipping` absent reaches the same renderer algorithms without
changing A0 numerics or bytes. Exercise the shared HLR/renderer layers so tests
fail if a duplicate solver or renderer is introduced; do not require A0 to use
the B0 preparation adapter.

Keep focused geometry cases in CTest and contract/binding checks in their
existing Rack strata. Do not add network access or full SDK rebuilds to a fast
unit stratum. Record any material test-runtime increase in the current
unified-transports plan's runtime-impact audit.

## Visible demo

Extend the existing browser Illustration Lab and egui Native Rust API Lab with
X/Y/Z/camera-facing plane selection, retained-side selection, and an adjustable
plane position. Both submit the governed B0 request and keep their 3D panes as
the unclipped source reference. Camera-facing clipping follows orbit changes.

Keep `src/rust/geometer-client/examples/direct_static_illustration.rs` only as
an internal relocated-SDK qualification sample. It must use
`GeometerDirectClient`, run with no executable discovery or sibling
`geometer(.exe)`, and exercise unclipped, partial, and empty results. It is not
a separately packaged demo or release asset.

## Documentation and release

Update the authored illustration, model-illustration, HLR, transport, Rust,
Python, WASM, static SDK, and distribution documents. Regenerate the offline
contract site and public-surface review lock. Add issue 40 and the supported
no-cap/empty-state behavior to the release notes.

Before handing issue 40 to the shared release candidate, pass native CTest, all
Rack strata, contract generation checks, Rust formatting/lint/tests, browser
and egui Lab smoke tests, real relocated static SDK qualification, WASM
validation, L99, and release-mode `wn-dev-std audit`.

The clipping plan does not publish an earlier independent release. Its
qualification step satisfies the unified plan's `illustration-clipping-40`
dependency; the unified release signoff then qualifies one candidate containing
the SDK, B0 contracts, and negotiated clients. Unified promotion publishes those
same bytes. Close issue 40 only after public redownload/attestation verification,
then let the downstream Cruncher plan update its exact `wn-geometer` pin.

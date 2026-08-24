# STEP Topology Contract Slice C

Status: unpromoted experimental candidate; native exact-journal restore subset available

Date: 2026-08-23

## Result

TypeSpec Slice C projects the stabilized persistence and recovery research
shapes into JSON Schema, C++17, TypeScript, Rust, Python, and offline HTML. It
adds candidate save, restore, and recovery-analysis operation shapes. The
hierarchy operation and its behavior are governed as a Slice B extension; Slice
C only carries hierarchy state as part of persisted outcomes.

These are generated experimental structures. The operation catalog records
them as `experimental_candidate` and keeps portable `runtime_available = false`.
The native catalog advertises only exact edit-journal restore; general save and
recovery analysis remain structural-only. No C ABI, CLI alias, or browser/WASM
entry point is implied. Focused native C++ remains behavioral authority.

## Hierarchy Boundary

`apply_hierarchy` represents products sourced from complete definitions or
independent bodies, source-free assemblies, and authored occurrences with
explicit 3-by-4 transforms. Its closed command union covers create product,
create assembly, create occurrence, reparent occurrence, rename node, erase
occurrence, and erase node.

The generated TypeScript semantic helper checks research namespaces, duplicate
creates, published node/occurrence uniqueness, product-versus-assembly source
rules, parent and child references, assembly-only parents, and cycles. The
native value API additionally owns transaction revision, source ownership,
transform, work-budget, aliasing, and atomic-publication behavior. Arbitrary
face subsets are not hierarchy sources; a fused slab uses logical groups unless
a separately approved geometry operation creates independent shapes.

## Persistence Boundary

Save is closed over four explicitly different carriers:

- binary XCAF/XBF storage version 12;
- XML XCAF storage version 12;
- STEP AP242 managed-model-based 3D engineering; and
- a namespaced JSON sidecar.

Restore additionally accepts the bounded edit-journal artifact. Journal replay
is not a generic file import: it requires exact source, normalized B-rep,
target-inventory, OCCT-version, and transaction-count preconditions and rejects
a mismatched source before replay.

Save validation binds the requested carrier to the emitted artifact carrier.
Restore accepts both declared STEP media types, binds exact attachment
descriptors to bytes, and validates the reported source and replayed transaction
count against the request preconditions.

Every variant fixes the attachment name, media type, format string, byte limit,
and SHA-256 descriptor. A capability record reports save, restore, authored
payload, and topology-link posture independently as supported, experimental,
or unsupported. This prevents successful file creation from being interpreted
as semantic survival.

The structural restore request deliberately requires both exact source STEP
bytes and a state artifact. That conservative rule supports digest validation,
journal replay, and fail-closed recovery across all carriers, even when XBF is
self-contained. A future implementation may introduce a separately reviewed
XBF-only shortcut; the candidate does not imply one.

## Recovery Boundary

The wire result preserves the native policy model's independent dimensions:
resolution state, method, topology comparison, confidence, evidence, member
counts, and group completeness. Candidate evidence records provenance,
explicit tolerances, carrier validation flags, optional geometry/adjacency
fingerprints, rejected alternatives, and one result per requested body or face
member.

The TypeScript semantic helper enforces unique groups, members, and candidate
handles; bounded groups, members, and at most 16 candidates per member;
candidate/member kind agreement; lowercase digests, finite ordered
fingerprints, verified authored-id and locator evidence requirements; lineage preconditions;
resolved-handle/method/confidence consistency; exact accepted-plus-rejected
candidate accounting with no accepted/rejected handle overlap; aggregate
member counts; and group state/completeness rules.

The current result has one optional resolved target handle per source member.
A `split` or `merged` topology comparison therefore reports verified lineage
for the selected durable successor, not a complete successor set or an
automatic repair. A future repair contract must add and prove a bounded
descendant set before promising one-to-many edits.

## Governed Evidence

Twenty-nine Slice C vectors cover:

- a carrier-specific save result, exact attachment relation, media tamper, and
  capability and requested-carrier mismatch;
- a strict restore request, exact two-attachment relation, and rejected replay
  source mismatch, including both accepted STEP media types and invalid media;
- a restore result matching its request plus rejected source, replay-count, and
  nested recovery contradictions;
- a valid recovery evidence request;
- exact and over-limit candidate fan-in;
- rejected missing authored id, missing validated locator, uppercase digest,
  and reversed fingerprint bounds;
- an explicitly partial two-member recovery result; and
- an explicit ambiguous recovery result;
- a native-valid authored-id recovery whose topology comparison is unavailable
  because fingerprints are absent, plus an empty-group structural rejection;
- a resolved target whose topology is still reported changed; and
- rejected recovery aggregate-count and candidate-evidence-count mismatches,
  including a balanced count whose selected target is also listed as rejected.

The promotion manifest keeps the slice unpromoted and records all six generated
projection families. The legacy Appz glTF enrichment model is not reused.

## Deferred Runtime Work

- general XBF/XML/AP242/sidecar save and restore adapters;
- carrier-to-recovery evidence extraction rather than caller-supplied evidence;
- a complete split/merge descendant model;
- recovery-analysis process routing; and
- the later Appz Annotation Lab application.

The native executable implements the strict edit-journal restore subset. Its
machine-readable native catalog advertises only the edit-journal
`state_artifact` media type and 64 MiB ceiling, even though the structural
request union retains future carrier variants. It requires exact source and
journal attachments, validates byte counts, SHA-256, media types, and explicit
replay preconditions before store admission, issues a new session identity, and
reports any admission evictions. The real Node flow then mutates the restored
authored group to prove replayed state without reusing old topology handles.

No Appz files are changed by this slice.

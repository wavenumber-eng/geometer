# STEP Topology Contract Slice B

Status: unpromoted experimental candidate; native mutation/checkpoint subset available

Date: 2026-08-23

## Purpose And Authority

Slice B projects the stabilized native logical-group, neutral metadata-probe,
synthetic hierarchy, and exact-source checkpoint shapes into generated TypeSpec
contracts. It adds four candidate operations: apply logical groups, apply
metadata probes, apply hierarchy, and checkpoint the edit journal. It does not
expose a callable C ABI or browser WASM route; the native executable routes the
three non-hierarchy operations through the bounded session store.

TypeSpec owns the candidate JSON structure. The native C++ session remains the
behavioral authority for atomic mutation, generation and revision checks,
target resolution, accounting, cancellation, source binding, replay, and the
binary journal encoding. All eight roots and four operations remain
`experimental_candidate` and portable `runtime_available: false`. The native
catalog additionally advertises apply logical groups, apply metadata probes,
and checkpoint edit journal; hierarchy remains structural-only.

This is carrier research, not the future Appz annotation contract. The names
`wn.geometer.research.group.*`, `wn.geometer.research.probe.*`, and
`wn.geometer.research.probe.key.*` intentionally prevent these temporary
records from being mistaken for durable application semantics. No legacy GLTF
enrichment vocabulary is reused.

## Mutation Shapes

Logical-group commands form a closed discriminated union:

- `create` carries an authored id, name, and nonempty body/face handle list;
- `rename` carries an authored id, expected revision, and name;
- `replace_members` carries an authored id, expected revision, and nonempty
  handle list; and
- `erase` carries only the authored id and expected revision.

Metadata-probe commands are likewise closed. `attach` carries a target, key,
and value; `replace` adds an expected revision; and `erase` carries only the
authored id and expected revision. Probe targets are a second discriminated
union covering document, definition, root occurrence, component occurrence,
body, face, and logical group. The document target has no locator, topology
targets have exactly one generation-scoped handle, and a logical-group target
has exactly one authored group id. This makes the native canonical erase rule
structural rather than relying on ignored placeholder fields.

Successful results publish the refreshed session/generation, edit-journal
revision, bounded accounting, and the complete applicable group/probe state.
Target handles in results belong only to that refreshed generation. The native
adapter pre-encodes that exact candidate outcome and applies the executable
IPC 8 MiB JSON limit before commit; rejection rolls back generation, journal,
handles, and authored state.

Generated codecs enforce the closed union variants, required fields, bounded
arrays and strings, positive `uint32` revisions, and numeric resource ceilings.
The shared TypeScript semantic helpers additionally enforce the ASCII research
namespaces, exact `gtt_` target-handle shape for group members, duplicate
member/create rejection with sequential erase/recreate semantics,
probe-key/group-target namespaces, and the 100,000 aggregate group-member
limit for both transaction payloads and published results. The same aggregate
transaction limit is native policy rather than a wire-only restriction.
Live-state preconditions and target existence remain native responsibilities.

## Checkpoint Attachment

Checkpoint returns exact source SHA-256, normalized B-rep SHA-256, ordered
target-inventory SHA-256, OCCT version, transaction count, and one required
`edit_journal` descriptor. The separately transported attachment is limited to
64 MiB and uses media type
`application/vnd.wavenumber.geometer.step-topology-edit-journal` with format
`geometer.step_topology_edit_journal.a0`.

The semantic attachment validator requires exactly one declared attachment,
matching name, media type, byte count, and SHA-256. It also requires the
reported transaction count to equal the session edit-journal revision. The
binary payload layout, internal checksum, replay limits, and exact-source
preflight remain governed by the native edit-journal design and tests.

## Hierarchy Extension

The hierarchy command union covers create product, create assembly, create
occurrence, reparent occurrence, rename node, erase occurrence, and erase node.
Products, assemblies, and occurrences use their native kind-specific,
lowercase-suffix research namespaces. The semantic validators accept
erase/recreate lifecycle sequences for both product and occurrence ids,
enforce the full `uint32` revision domain, require products to bind definition
or body sources, require source-free assemblies and assembly parents, and use
an iterative cycle check so maximum-size valid graphs do not recurse.

## Validation

Forty Slice B vectors are recorded in the promotion manifest: eleven
strict/schema vectors and twenty-nine semantic vectors. They cover canonical
round-trip and accepted create/rename/replace/erase/recreate group sequencing;
accepted attach/replace/erase/re-attach probe sequencing over every target
variant; both mutation result roots; the checkpoint request; maximum `uint32`
group revision; unknown fields; wrong command variants; malformed target
handles; duplicate members/creates/attaches; invalid group, probe, key, and
group-target namespaces; exact-limit acceptance and aggregate request/result
group-member exhaustion across multiple individually valid collections;
canonical probe erase; the 64 MiB checkpoint ceiling; and accepted plus
name/media/byte/count/digest/revision-mismatched journal relationships.
They also govern the intended casing split: logical-group and metadata-probe
names retain the native case-permitting research syntax, while hierarchy ids
remain lowercase-only. They cover all seven hierarchy commands in one
lifecycle, the exact
`uint32` revision maximum and overflow rejection, a valid published hierarchy,
and a rejected assembly cycle.

The generated JSON Schema, C++, TypeScript, Rust, Python, and HTML projections
are deterministic. TypeScript and the vector harness own the extra semantic
checks; Python replays the generated structural projection. Native focused
tests remain the behavioral evidence.

## Native Runtime And Deferred Surface

- hierarchy process routing and all browser session routing;
- changed-source, changed-OCCT, partial, or ambiguous recovery;
- logical-group/journal projection into the now-proven standard XBF/XML carrier;
- STEP/AP242 projection and survival evidence;
- sidecar and GLB transport comparison; and
- the carrier-neutral Appz annotation contract and Annotation Lab implementation.

The native executable routes atomic logical-group and metadata-probe
transactions plus edit-journal checkpoints. The real Node integration verifies
that every mutation advances the generation and remints topology handles before
checkpointing.

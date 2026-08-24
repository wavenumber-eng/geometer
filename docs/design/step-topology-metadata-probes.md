# STEP Topology Metadata Probes

Status: experimental carrier-research surface

Date: 2026-08-23

## Purpose And Boundary

Metadata probes are deliberately small namespaced key/value records used to
measure what XCAF, XBF/XML, STEP/AP242, GLB, and sidecars can carry. They are
not annotations, semantic features, presentation models, or the future
`appz/data_models` contract.

Probe authored ids use `wn.geometer.research.probe.<suffix>` and keys use
`wn.geometer.research.probe.key.<suffix>`. Values are bounded strings. A probe
can target one document, definition, root occurrence, component occurrence,
body, face, or authored logical group.

## Transactions

Attach, replace, and erase commands carry an expected session generation;
replace and erase also carry the expected probe revision. All target locators,
payload shapes, namespaces, revisions, group references, cardinality, strings,
journal growth, and resident storage are validated before publication.
Erase has one canonical discriminated shape: the target sentinel is
`document`, and target/payload strings are empty. Alternate discriminants are
rejected even when their apparent effect would be identical.

Success advances the session generation and journal revision, invalidates old
runtime/render handles, and returns all probes plus logical groups with fresh
handles. A logical group with an attached probe cannot be erased. Any command,
refresh, remap, allocation, or publication failure restores the prior probes,
snapshot, handle registry, generation, journal, and accounting and clears the
result. Native IPC encodes and validates the exact generated success outcome
against the 8 MiB JSON ceiling before committing, so an oversized complete-state
response cannot turn a successful mutation into a transport failure.

## Identity And Replay

The live record stores a source-bound target class, ordinal, and evidence
digest rather than a runtime handle. Publication resolves that record to a
fresh handle only for the current generation. Logical-group targets use the
group's authored research id.

The edit journal records the same bounded locator evidence. Exact-source replay
validates the STEP digest, normalized B-rep digest, complete ordered target
inventory and occurrence parent paths, OCCT version, target class, ordinal,
and evidence before reconstructing a command. This is strict restart replay,
not cross-version or geometry-changing recovery.

## Evidence

`geometer_step_topology_logical_groups_test` proves:

- one atomic batch attaches probes to every supported target class;
- old target handles are not published after mutation;
- forged targets fail without mutation;
- attach, replace, and erase revisions replay in order;
- group deletion is rejected while a probe references the group and succeeds
  after that probe is removed;
- group and probe commands share one deterministic checkpoint; and
- noncanonical erase commands fail both through the direct API and after
  checksum-valid journal decoding; and
- the final replayed group/probe state and checkpoint bytes match the original
  while the B-rep digest remains unchanged.

## Deferred

- physical XCAF attribute placement;
- XBF/XML storage-driver behavior;
- STEP/AP242 entity and relationship projection;
- sidecar and GLB projection comparison;
- cross-version/member-level recovery; and
- any carrier-neutral annotation meaning.

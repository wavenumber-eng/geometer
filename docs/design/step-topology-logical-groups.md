# Native STEP Topology Logical-Group Transactions

Status: experimental native transaction, neutral metadata probe, and
exact-source restart foundation

Date: 2026-08-22

## Purpose And Boundary

The native value API can create, rename, replace the members of, and erase
research-only logical groups over live body and face targets. A logical group
does not split, sew, fuse, copy, or otherwise change the B-rep. It is not an
XCAF product, assembly, layer, material, or final annotation contract.

Authored ids are explicitly namespaced as
`wn.geometer.research.group.<suffix>`. They are distinct from opaque session
handles and generation-scoped body/face handles. The live session records every
committed transaction in a bounded source-bound journal. See
`step-topology-edit-journal.md` for the deliberately strict
same-source/same-OCCT restart claim and its recovery limitations.

## Transaction Semantics

`StepTopologyGroupTransaction` carries the expected live generation and one or
more commands. Existing-group commands also carry the expected group revision.
All commands are applied to an authored-id-indexed staged copy. Body/face
handles and duplicate member shapes are also resolved through bounded hash
indexes rather than repeated scans. Validation rejects stale
generations or revisions, duplicate authored ids, duplicate members, empty
member sets, non-body/face targets, malformed command shapes, and configured
group/member/string/resource limits before publication. Command payloads have
the same 100,000 aggregate member-reference ceiling as published group state,
so repeated per-command arrays cannot bypass transaction admission bounds.
The exact encoded journal growth is preflighted as part of validation, so a
successful transaction cannot leave the session in a state that is too large
to checkpoint.

On success, Geometer rebuilds the normalized snapshot at generation + 1,
invalidating prior topology handles and render artifacts. Stored member shapes
must resolve uniquely to new body/face handles before the staged groups are
committed. Publication is built into a separate result before a no-throw swap.
For native IPC, that exact generated success outcome is encoded and validated
against the 8 MiB response ceiling while the old state is still available for
rollback. Any command, allocation/OCCT exception, refresh, remapping, or
response-publication failure clears the output and restores the prior snapshot,
handle registry, generation, journal, accounting, handle counter, and group
state. The result contains all groups in authored-id order with refreshed
member handles.

Logical-group dynamic storage participates in the conservative session
resident-byte estimate. Group ids and names obey both the per-string limit and
the same aggregate string budget as normalized inspection data. The
session-store adapter updates aggregate accounting and evicts a mutated session
if it no longer fits the configured store limit.

Each normalized snapshot also contains a SHA-256 digest over bounded canonical
B-rep evidence: validity, topology cardinalities, orientations, analytic
surface/curve kinds, tolerances, bounds, mass properties, centroids, and vertex
coordinates. It deliberately excludes tessellation so rendering cannot create
a false geometry change. This digest is geometry-preservation and recovery
evidence, not durable semantic identity.
The snapshot records the complete B-rep digest traversal and source-transfer
work counts used by the bounded restart-replay meter.

## Evidence

`geometer_step_topology_logical_groups_test` uses the generated fused-slab
fixture and proves:

- two faces can be grouped without changing the normalized B-rep digest or
  body/face geometric properties;
- successful create, rename, replace-members, and erase operations advance the
  session generation and applicable group revision;
- old target handles and render artifacts fail after a successful mutation;
- returned group members use the refreshed generation;
- stale transactions and a failing command later in a batch clear output and
  do not partially mutate the session;
- group strings consume per-string and session-wide budgets;
- resident accounting grows with group state; and
- a mutation that crosses the store byte limit evicts the session and clears
  its unpublished result;
- an injected publication rejection observes the complete candidate result but
  leaves generation and journal state unchanged; and
- definition targets are rejected because this first grouping surface accepts
  only bodies and faces.

## Deferred In This Active Work Package

- cross-version, changed-topology, ambiguous, and partial member recovery
  beyond the implemented exact-source replay path;
- browser/WASM runtime exposure of the unpromoted TypeSpec Slice B; and
- XBF/XML or AP242 persistence.

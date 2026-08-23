# Native STEP Topology Logical-Group Transactions

Status: experimental native transaction foundation; edit-journal and metadata
probe work remains active

Date: 2026-08-22

## Purpose And Boundary

The native value API can create, rename, replace the members of, and erase
research-only logical groups over live body and face targets. A logical group
does not split, sew, fuse, copy, or otherwise change the B-rep. It is not an
XCAF product, assembly, layer, material, or final annotation contract.

Authored ids are explicitly namespaced as
`wn.geometer.research.group.<suffix>`. They are distinct from opaque session
handles and generation-scoped body/face handles. This slice stores groups in
the live native session only. It does not yet claim restart safety; that claim
is gated on the strict journal/replay work in the next sub-slice.

## Transaction Semantics

`StepTopologyGroupTransaction` carries the expected live generation and one or
more commands. Existing-group commands also carry the expected group revision.
All commands are applied to a staged copy. Validation rejects stale
generations or revisions, duplicate authored ids, duplicate members, empty
member sets, non-body/face targets, malformed command shapes, and configured
group/member/string/resource limits before publication.

On success, Geometer rebuilds the normalized snapshot at generation + 1,
invalidating prior topology handles and render artifacts. Stored member shapes
must resolve uniquely to new body/face handles before the staged groups are
committed. Any command, refresh, or remapping failure restores the prior
snapshot, handle registry, generation, and group state. The result contains all
groups in authored-id order with refreshed member handles.

Logical-group dynamic storage participates in the conservative session
resident-byte estimate. The session-store adapter updates aggregate accounting
and evicts a mutated session if it no longer fits the configured store limit.

## Evidence

`geometer_step_topology_logical_groups_test` uses the generated fused-slab
fixture and proves:

- two faces can be grouped without changing body/face geometric properties;
- successful create, rename, replace-members, and erase operations advance the
  session generation and applicable group revision;
- old target handles and render artifacts fail after a successful mutation;
- returned group members use the refreshed generation;
- stale transactions and a failing command later in a batch do not partially
  mutate the session; and
- definition targets are rejected because this first grouping surface accepts
  only bodies and faces.

## Deferred In This Active Work Package

- strict source-bound edit-journal encoding, checkpointing, and replay;
- member recovery evidence after reopening the original STEP;
- namespaced metadata probes on non-group targets;
- TypeSpec Slice B and process/TypeScript exposure; and
- XBF/XML or AP242 persistence.

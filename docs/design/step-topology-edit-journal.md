# STEP Topology Edit-Journal Checkpoint

Status: experimental exact-source restart checkpoint

Date: 2026-08-23

## Purpose

The native STEP topology session records every successfully committed logical
group or neutral metadata-probe transaction in one bounded edit journal. A
caller can checkpoint that journal as deterministic binary bytes and later
reopen the exact original STEP source plus the checkpoint. Replay reconstructs
authored group and probe state while issuing entirely new session, generation,
and target handles.

This is the first restart-safe authoring mechanism. It is not the later
cross-version or geometry-changing recovery model, and it does not make a
runtime handle, STEP entity number, XCAF label, topology ordinal, or geometry
fingerprint into durable semantic identity.

## Source Binding

A checkpoint is accepted only when all of these match the newly opened native
session:

- SHA-256 of the exact submitted STEP bytes;
- normalized B-rep evidence digest; and
- SHA-256 of the complete ordered definition, occurrence hierarchy, body,
  shell, face, membership, transform, label, and geometric target inventory;
  and
- exact OCCT build version.

Each body or face member is encoded as a bounded source-local ordinal plus a
definition ordinal and target evidence digest. Replay validates all three
before translating the member into a fresh runtime handle. These locators are
strict replay preconditions for an identical source, not portable identifiers.
The first [multidimensional recovery policy](step-topology-recovery.md) now
reports ambiguity, topology changes, and member-level outcomes instead of
using this strict path. Candidate extraction from a changed STEP/XBF/GLB
artifact remains later work.

## Encoding And Integrity

`geometer.step_topology_edit_journal.a0` is a deterministic little-endian
binary checkpoint containing:

- format magic and version;
- exact source, B-rep, ordered-target-inventory, and OCCT evidence;
- ordered transaction sequence numbers;
- ordered create, rename, replace-members, and erase commands;
- authored group ids, expected revisions, and names; and
- validated body/face source locators and evidence digests.

The ordered transaction union also carries metadata-probe attach, replace, and
canonical erase commands, including their authored ids, revisions, target
locators, keys, and values.

The final 32 bytes are SHA-256 over the preceding payload. The public
checkpoint additionally reports SHA-256 over the complete artifact. Decode
rejects an invalid digest, unsupported version, malformed lengths or enums,
out-of-order sequence, configured byte/transaction/member limits, and trailing
payload.

Double evidence uses the classic C++ locale and authored-id validation is
explicitly ASCII. Thus a process locale change cannot alter checkpoint bytes
or turn non-ASCII bytes into valid namespace characters.

The format is an experimental Geometer attachment, not a final annotation
contract. TypeSpec Slice B describes the operation and attachment relationship;
the binary journal stays owned by the native behavior and is not hand-edited by
web consumers.

## Atomicity And Accounting

A candidate journal entry is created, source-locator evidence is validated,
and its exact projected binary checkpoint size is admitted before session
mutation begins. A source cannot open when even its empty checkpoint exceeds
`max_edit_journal_bytes`. The entry is appended before snapshot rebuild so the
same per-string, aggregate-string, session-resident, and store-resident limits
cover both group state and journal history. Any failed rebuild, remap,
publication, allocation, or OCCT exception removes the candidate journal entry
and restores the prior session state.

The session reports `edit_journal_revision` and aggregate accounted string
bytes. Those fields are included in render and GLB seals. Transaction count,
encoded bytes, and a conservative replay-work meter bound restore cost;
restore also accepts cooperative cancellation during decode, preflight,
command/member reconstruction, transaction validation, locator construction,
snapshot rebuild, and result publication. The work meter charges every
transaction for repeated topology work, cumulative journal commands and
members, and a monotonic upper bound on retained authored group/probe state
before any replay mutation begins. Erased or replaced records remain charged.
The topology charge uses the measured source-transfer work plus the complete
B-rep digest traversal count, including compounds, solids, shells, wires,
edges, vertices, and the additional per-face/per-edge membership walks; it is
not inferred from face count alone.
This deliberately conservative bound contains the current repeated-snapshot
replay implementation until a future batched replay engine replaces it.

## Evidence

`geometer_step_topology_logical_groups_test` proves that:

- create and rename transactions advance the journal revision;
- a deterministic checkpoint replays to the same authored groups and group
  revisions with fresh runtime handles;
- replay produces the same canonical checkpoint bytes;
- the normalized B-rep digest is unchanged;
- tampered bytes and a different STEP source fail without publishing a
  session; and
- a validly checksummed but mismatched ordered inventory fails closed;
- exact empty and one-transaction byte boundaries remain checkpointable while
  one-byte overflow is rejected before mutation;
- large replay histories obey the explicit work budget and cancellation; and
- a deterministic replay-apply entry barrier proves cancellation after
  replay preflight stops without publishing a restored session; and
- journal exhaustion rejects the next transaction without changing the live
  generation or publishing partial output.

## Deferred

- browser runtime exposure and changed-source recovery routing;
- XBF/XML autosave as an alternative checkpoint;
- cross-version and geometry-changing candidate extraction;
- applying partial member recovery or repair workflows; and
- AP242 projection.

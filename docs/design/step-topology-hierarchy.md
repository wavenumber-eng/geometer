# STEP Topology Synthetic Product Hierarchy

Status: experimental native value model; XCAF/AP242 projection deferred

Date: 2026-08-23

## Purpose And Boundary

The native hierarchy policy builds a synthetic product/assembly overlay over a
generation-bound topology snapshot. It gives flat STEP bodies an authored
product structure without changing, splitting, sewing, fusing, duplicating, or
relocating the source B-rep.

The overlay intentionally accepts geometry ownership only from:

- a complete inspected definition; or
- a complete inspected body.

Faces, shells, arbitrary face subsets, and logical groups are not product
sources. A semantic subset of one fused body remains a logical group unless a
separately approved geometric decomposition produces independent valid shapes.

This slice is the safe mutation model, not yet a rewritten XCAF document or
STEP file. Persistence expansion will project the independently reviewed value
state into XCAF and AP242 and then re-run geometry and occurrence evidence.

## State Model

`StepTopologyHierarchyState` is bound to the session handle, topology
generation, and normalized B-rep digest that created it. It contains two
distinct concepts:

- nodes are authored products or synthetic assemblies; product nodes own one
  complete definition/body source, while assemblies own no geometry; and
- occurrences place one product or assembly under a parent assembly with a
  row-major signed-rigid 3x4 transform.

Product definitions and their occurrences are never collapsed. Authored ids
use separate `wn.geometer.research.product.*`, `.assembly.*`, and
`.occurrence.*` namespaces. Nodes and occurrences have independent optimistic
revisions, while the state has one transaction revision.

## Atomic Commands

One transaction can create a product, create an assembly, create an
occurrence, reparent an occurrence, rename a node, erase an occurrence, or
erase an unreferenced node. Commands apply to a candidate copy; the final state
is published only when the whole transaction validates.
Passing the same state object as both current input and result output is
supported: failure preserves it, and success replaces it only at final
publication.

Validation rejects:

- stale topology or hierarchy revisions;
- stale node/occurrence revisions;
- duplicate or malformed authored ids;
- unknown or unsupported geometry sources;
- assigning the same source twice;
- assigning a complete definition and any of its bodies independently;
- geometry-bearing assembly nodes;
- missing children or non-assembly parents;
- assembly cycles;
- non-finite, scaled, sheared, or otherwise non-signed-rigid transforms;
- erasing referenced nodes; and
- command, work, topology-cardinality, occurrence-cardinality, per-string, or
  aggregate-string limit exhaustion.

The work meter conservatively charges five projected-record touches per
command for dual lookups plus the most expensive reference scan and vector
shift/reallocation path. It separately charges current-state validation and
copy, final-state validation, cycle traversal, and source inventory.
`max_hierarchy_transaction_commands` and
`max_hierarchy_transaction_work_items` are explicit native limits. The test
executes erase-node with an unrelated live occurrence at the exact boundary.
Every string field in every command is preflighted, including fields ignored by
that command kind. This prevents transient or unused input from bypassing the
per-string and aggregate transaction limits.

## Evidence

`geometer_step_topology_hierarchy_test` uses the curated generated fixtures and
proves:

- the flat multi-solid STEP definition's two independent bodies become two
  products placed through a root assembly and subassembly;
- rename and reparent update independent revisions and preserve placement;
- cycles, stale revisions, ownership overlap, and non-rigid transforms fail
  without output or changes to the prior state;
- erasing occurrences and then nodes reverses the hierarchy and releases
  ownership for a complete-definition product;
- the live session generation and B-rep digest remain unchanged;
- a face of the fused slab cannot become a product, while its complete fused
  body can; and
- exact command/work limits accept while one-over limits fail before
  publication.

## Deferred

- mapping pre-existing STEP assembly occurrences into editable authored state;
- journal encoding and restart replay for hierarchy commands;
- XCAF binary/XML projection and reload;
- AP242 product/assembly projection and relationship checks;
- occurrence-specific annotations and recovery; and
- TypeSpec Slice B hierarchy command additions and Slice C persistence results.

The legacy Appz glTF enrichment model was not used. The later Annotation Lab
should consume generated contracts after projection and recovery evidence
stabilize, not copy this C++ research structure by hand.

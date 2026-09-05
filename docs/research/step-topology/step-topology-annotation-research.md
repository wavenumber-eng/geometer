# STEP Topology Annotation Research Synthesis

Status: durable research synthesis; native experimental substrate implemented,
Appz Annotation Lab not started

Date: 2026-08-23

Scope: Geometer STEP/AP242, OCCT/XCAF/BRepGraph, browser selection handoff,
logical grouping, persistence evidence, and recovery policy

Tracking: wavenumber-eng/geometer#15, #16, #17

## Goal And Outcome

This research asked whether a student-facing tool can use OCCT through Geometer
to inspect STEP structure, select bodies or faces in a web renderer, author
logical groups and small metadata probes, preserve restart-safe state, and
eventually project application-owned annotations into suitable CAD carriers.

The answer is qualified yes:

- OCCT/XCAF can retain the source document, distinguish definitions from
  occurrences, enumerate selectable topology, and resolve transient opaque
  handles without exposing pointers or label paths.
- Geometer can tessellate that topology into a bounded GLB work packet, preserve
  occurrence/body/face ownership for every triangle, and resolve a real
  Three.js ray hit back to the live native target.
- Geometer can apply geometry-neutral logical face/body groups and namespaced
  metadata probes atomically, checkpoint an edit journal, replace the native
  process, and replay against the exact original STEP under strict
  preconditions.
- XCAF binary/XML and AP242 carrier experiments show useful persistence paths,
  but their evidence is narrower than the live edit-journal workflow and does
  not establish third-party CAD survival.

The maintained student reference is
[`examples/node/step_topology_annotation_reference.ts`](../../../examples/node/step_topology_annotation_reference.ts).
The measured fixture baseline is
[step-topology-fixture-baseline.md](step-topology-fixture-baseline.md), and the
recommended later Appz slice is
[step-topology-appz-annotation-lab-handoff.md](step-topology-appz-annotation-lab-handoff.md).

The legacy Appz glTF enrichment model was not used.

## Implemented Native Research Surface

The native executable catalog advertises these experimental operations:

- open, inspect, render, resolve hit, and close;
- apply logical groups and apply metadata probes;
- checkpoint edit journal; and
- exact-source edit-journal restore.

The portable C ABI and browser WASM catalogs do not advertise these native
session operations. Synthetic hierarchy, general save/export, changed-source
recovery analysis, XBF/XML restore, and AP242 annotation export remain
structural candidates or focused native evidence only. Catalog availability and
promotion-manifest declarations are tested in lockstep.

Session handles, topology handles, artifact handles, occurrence handles, and
GLB primitive/triangle locators are transient. They are scoped to a native
process, session generation, or render artifact. Only authored ids, source and
artifact digests, carrier evidence, fingerprints, and application-owned
semantics are candidates for durable state.

## Browser And GLB Finding

The selected render proof uses deterministic Geometer tessellation with one
primitive per face and namespaced GLB `extras` for bounded occurrence/body/face
binding. Before Three.js parses or raycasts the bytes, the generated TypeScript
helper verifies attachment names, media types, byte counts, and SHA-256
digests. The native resolver independently checks the retained artifact,
generation, occurrence, primitive, triangle range, and claimed target handles.

GLB is therefore a useful self-describing work packet. It is not authoritative
authoring state and cannot reconstruct the source B-rep, XCAF document, or
annotation meaning. Unknown-`extras` preservation by downstream glTF tools is
not assumed.

## Grouping And Hierarchy Finding

A semantic group of faces on one fused body does not create a new body or STEP
product. The live transaction engine supports create, rename, replace-members,
and erase for logical groups, plus attach, replace, and erase for metadata
probes. Tests compare B-rep digests and geometric properties across those
operations.

The separate hierarchy proof establishes a safe future boundary: complete
definitions and independent bodies may participate in synthetic product
structure, while arbitrary face subsets remain logical groups. Hierarchy
mutation is not advertised by the native operation catalog.

## Persistence Finding

Same-version OCCT 8.0.1 XBF and XML XCAF tests identify which standard
attributes and experimental authored records survive. A minimal namespaced
custom attribute proves that registered binary/XML drivers can persist custom
state and that missing-driver behavior must fail explicitly. These formats are
Geometer/OCCT caches, not vendor-neutral interchange.

The AP242 test constructs and reloads controlled product/body/face relationship
graphs using OCCT model/work-session APIs. It proves OCCT self-round-trip for
the tested `SHAPE_ASPECT`, property, and
`GEOMETRIC_ITEM_SPECIFIC_USAGE` paths, including a research multi-face
convention. It does not prove general PMI, an AP242-standard logical-group
vocabulary, third-party CAD survival, or a live selected group/probe-to-AP242
export operation.

The student flow consequently uses the original STEP plus a validated edit
journal as its current restart carrier. General save/export remains deferred.

## Recovery Finding

Recovery must keep independent dimensions for resolution state, resolution
method, topology comparison, confidence, evidence, and group completeness.
STEP entity numbers, XCAF label paths, traversal indexes, GLB ids, and
BRepGraph UIDs are not presumed durable. Ambiguous and partial results fail
closed rather than using ordering as identity.

Focused native recovery tests cover precedence and reporting. Changed-source
recovery analysis is not yet exposed through the native operation catalog, so
the student reference reports exact replay, not a changed-topology recovery
claim.

## BRepGraph Finding

The isolated OCCT 8.0.1 BRepGraph probe confirms that the API is available and
can be evaluated without making it a prerequisite for the XCAF session model.
The current public-looking identity design remains authored ids plus transient
opaque handles and evidence-backed recovery; it does not adopt BRepGraph UIDs
as durable annotation identity.

## Evidence Index

- [Fixture and runtime-impact baseline](step-topology-fixture-baseline.md)
- [Native inspection and lifecycle](../../design/step-topology-native-inspection.md)
- [GLB topology/render binding](../../design/step-topology-glb-binding.md)
- [Logical groups](../../design/step-topology-logical-groups.md)
- [Metadata probes](../../design/step-topology-metadata-probes.md)
- [Edit-journal checkpoint and replay](../../design/step-topology-edit-journal.md)
- [Synthetic hierarchy proof](../../design/step-topology-hierarchy.md)
- [Multidimensional recovery](../../design/step-topology-recovery.md)
- [XCAF persistence](step-topology-xcaf-persistence.md)
- [AP242 persistence](step-topology-ap242-persistence.md)
- [Experimental contract Slice B](../../design/step-topology-contract-slice-b.md)
- [Experimental contract Slice C](../../design/step-topology-contract-slice-c.md)
- [Appz Annotation Lab handoff](step-topology-appz-annotation-lab-handoff.md)

## Next Authorized Boundary

No Appz file, issue state, hosted sandbox, or public artifact is changed by this
research. The next separately approved work should establish greenfield Appz
TypeSpec domains and a loopback-only Annotation Lab around this generated
Geometer client. It should start with a sidecar as semantic authority, use GLB
only as a validated projection, and retain save/export, external CAD survival,
and changed-source recovery as visible follow-on work.

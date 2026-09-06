# STEP Topology Research Handoff To Appz Annotation Lab

Status: durable research handoff; Appz implementation not started

Date: 2026-08-23

## Purpose

This document maps Geometer's OCCT topology, render-binding, persistence, and
recovery research to the Appz Annotation Lab issue family. It is the durable
bridge between generic Geometer evidence and a later, separately approved Appz
sandbox. It does not define the final `data_models.annotation` or
`data_models.three_d` contracts, and it does not reuse the legacy Appz glTF
enrichment model.

The architecture boundary remains:

- Appz `data_models.annotation` owns annotation meaning;
- Appz `data_models.three_d` owns carrier-neutral 3D structure and target
  semantics;
- Geometer owns OCCT/XCAF traversal, transient topology handles,
  topology-to-render binding, carrier I/O, and evidence-backed recovery;
- Viz owns reusable viewer/runtime components; and
- `appz/sandbox/annotation_lab` composes those parts as a disposable learning
  application.

## Durable Findings

1. STEP/XCAF, an XBF checkpoint, or the original STEP plus a validated edit
   journal is authoritative authoring state. A GLB is a render/work projection
   and cannot reconstruct the source B-rep.
2. Session target handles and GLB node/mesh/primitive/triangle locators are
   transient. They are generation- and artifact-scoped and must never become
   semantic join keys or durable annotation targets.
3. A stable authored research id can name a logical group or metadata probe,
   but carrier linkage and topology survival are independent evidence. A
   matching id does not prove unchanged geometry.
4. GLB `extras` is useful for bounded, namespaced projection metadata and a
   topology/render binding. It is not the first authoritative annotation
   store. The initial Appz loop should persist semantic state and recovery
   evidence in a sidecar, then regenerate GLB projection data.
5. Multiple faces of one fused body can form a semantic logical group without
   becoming a new B-rep body or product. Synthetic product hierarchy is valid
   only for complete definitions and independent bodies.
6. Binary and XML XCAF preserve the tested standard and authored research
   state in the same-version OCCT matrix. They are useful Geometer caches, not
   vendor-neutral interchange. Custom attributes require registered drivers
   and explicit version compatibility.
7. OCCT can construct and reload the tested AP242 relationship paths,
   including face-linked `GEOMETRIC_ITEM_SPECIFIC_USAGE` evidence and the
   research multi-face convention. This is OCCT self-round-trip evidence, not
   proof of generic PMI, a standard AP242 logical-group abstraction, or
   third-party CAD survival.
8. Recovery is multidimensional: resolution state, method, topology change,
   confidence, evidence, and group completeness remain separate. Partial and
   ambiguous recovery fail closed; input order never repairs identity.

## Appz Issue Mapping

| Appz issue | Geometer evidence now available | Remaining Appz or external work |
| --- | --- | --- |
| [#90](https://github.com/wavenumber-eng/appz/issues/90) carrier-neutral roadmap | Identity separation, normalized topology, render binding, carrier comparisons, and recovery policy | Own the semantic domains and promotion decisions |
| [#139](https://github.com/wavenumber-eng/appz/issues/139) sandbox/domain foundation | Generic boundary and terminology above | Establish Appz sandbox governance and greenfield domain shells |
| [#140](https://github.com/wavenumber-eng/appz/issues/140) STEP corpus/baseline | Partial support: reproducible generated fixtures, content hashes, structure/topology summaries, and focused XCAF/AP242 matrices | Add source/license-or-usage/redistribution fields for the selected corpus and record before/after unmodified write/reload evidence; copy only redistributable fixtures or reproducible generators |
| [#141](https://github.com/wavenumber-eng/appz/issues/141) sidecar authoring loop | Generated experimental contracts, Three.js hit proof, logical groups, probes, checkpoints, and strict replay/recovery rules | Build the loopback-only Appz UI/service and lab-authority sidecar model after approval |
| [#142](https://github.com/wavenumber-eng/appz/issues/142) external CAD survival | Exact OCCT-side AP242 evidence and limitations | Run recorded Altium and other CAD workflows; report field-level survival |
| [#143](https://github.com/wavenumber-eng/appz/issues/143) GLB extras comparison | Real GLTFLoader/Raycaster binding proof and bounded work-packet recommendation | Compare Appz semantic sidecar projection against Three.js load/save behavior |
| [#144](https://github.com/wavenumber-eng/appz/issues/144) fused-face grouping | Atomic face groups, unchanged B-rep evidence, member-level recovery, changed-topology reporting | Define carrier-neutral feature/group meaning and repair UX in the lab |
| [#146](https://github.com/wavenumber-eng/appz/issues/146) PCB-net-to-terminal trace | Geometer can supply selected occurrence/body/face targets and render highlights | Appz domain ids and relationships must be the semantic join graph |

## Recommended First Annotation Lab Slice

The first approved Appz slice should implement #141 and exercise #144 without
claiming a production contract:

1. open one corpus STEP file through a supervised native Geometer process;
2. inspect definitions, occurrences, bodies, shells, and faces;
3. render GLB and resolve a real Three.js ray hit to one transient Geometer
   target;
4. create a lab-local semantic face group and attach a label or note whose
   meaning is owned by Appz, not Geometer;
5. save a greenfield sidecar containing semantic ids, source digest,
   carrier-locator evidence, fingerprints, versions, and command provenance,
   but no runtime handles;
6. checkpoint the Geometer research edit journal separately;
7. replace the worker, reopen the exact source, replay the journal under strict
   preconditions, regenerate GLB, and print the exact-replay report; and
8. keep save/export and changed-source recovery visibly disabled until their
   structural candidates gain separately reviewed native implementations.

The maintained reference covers open, inspect, validated render attachments, a
real ray hit, group/probe mutation, checkpoint, process replacement, exact
restore, and post-restore mutation. It does not currently cover general
save/export or changed-source recovery analysis. The focused AP242, XCAF,
hierarchy, and recovery tests are research evidence for later runtime slices,
not student-callable operations today.

The lab sidecar should be authored from new TypeSpec models in Appz after the
domain composition and identity decisions are approved. Geometer's generated
experimental DTOs are transport/research inputs, not models to copy into the
Appz semantic domains.

## Student Guardrails

The student should work only with generated TypeScript types/codecs, ordinary
Three.js objects, named binary attachments, and a small loopback client. The
student should not traverse XCAF labels, mutate raw STEP entities, decode OCCT
pointers, invent durable meaning from `gtt_`/`gts_` values, or persist Three.js
object ids and triangle indexes.

The maintained starting point is
[`examples/node/step_topology_annotation_reference.ts`](../../../examples/node/step_topology_annotation_reference.ts).
It is compiled and bundled to
`dist/native/examples/step-topology-annotation-reference.mjs`, runs against the
real native executable, and is exercised by the TypeScript Rack stratum. The
example deliberately restarts the native process between checkpoint and
restore so a student cannot accidentally design around retained in-process
OCCT objects.

Start with generated redistributable fixtures. Add user-provided or private
models only through a provenance manifest that records source hash, license or
usage basis, redistribution permission, Geometer/OCCT versions, and baseline
structure/topology summaries. Retain failing cases as evidence.

## Evidence Index

- [Fixture baseline](step-topology-fixture-baseline.md)
- [Native inspection and session model](../../design/step-topology-native-inspection.md)
- [GLB binding](../../design/step-topology-glb-binding.md)
- [Logical groups](../../design/step-topology-logical-groups.md)
- [Metadata probes](../../design/step-topology-metadata-probes.md)
- [Edit journal](../../design/step-topology-edit-journal.md)
- [Hierarchy](../../design/step-topology-hierarchy.md)
- [Recovery](../../design/step-topology-recovery.md)
- [XCAF persistence](step-topology-xcaf-persistence.md)
- [AP242 persistence](step-topology-ap242-persistence.md)
- [Experimental contract Slice B](../../design/step-topology-contract-slice-b.md)
- [Experimental contract Slice C](../../design/step-topology-contract-slice-c.md)

## Authorization Boundary

No Appz file, issue state, public artifact, or hosted sandbox is changed by this
handoff. Appz setup, data-model authoring, and Annotation Lab implementation
require explicit user approval after the Geometer student reference surface and
final evidence review are complete.

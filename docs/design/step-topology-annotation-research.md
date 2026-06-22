# STEP Topology Annotation Research

Status: pre-design research plan  
Date: 2026-06-22  
Scope: Geometer STEP/AP242, OCCT/XCAF/BRepGraph, browser annotation handoff

## Purpose

This note defines the next Geometer investigation for WN package-model
annotations. The target is not a finished data model contract. The target is to
learn how much STEP, XCAF, TopoDS, and BRepGraph identity Geometer can expose
and round-trip so a later student-facing tool can annotate component terminals,
pin-1 indicators, and connector egress geometry.

The first practical workflow is:

```text
STEP AP203/AP214/AP242 bytes
-> Geometer topology/render view
-> browser UI selects bodies/faces/groups
-> browser sends annotation edits using Geometer ids
-> Geometer writes annotated AP242
-> Geometer reloads AP242 and reconstructs annotations
```

Altium is treated as an opaque carrier for this slice. The working assumption
from prior prototype work is that Altium preserves embedded STEP payload bytes.
Geometer therefore owns reading, writing, and interpreting the WN annotations.

## Current Geometer Baseline

Current public browser-facing APIs are task APIs, not an exposed OCCT binding:

- `step_to_glb` uses `STEPCAFControl_Reader`, an XCAF `TDocStd_Document`,
  names, colors, layers, `XCAFDoc_ShapeTool`, and `RWGltf_CafWriter`.
- HLR and model bounds use `STEPControl_Reader`, `TransferRoots()`, and
  `OneShape()`, which flattens the model into a `TopoDS_Shape`.
- The C ABI/WASM surface returns byte buffers or JSON for current operations;
  it does not expose XCAF labels, product structure, face identity, or STEP
  metadata.
- `scripts/dependency_versions.py` pins OCCT to `8.0.0`, so BRepGraph can be
  investigated in this repository once the local OCCT build exposes the headers.

Relevant files:

- `src/cpp/lib/step_to_glb.cpp`
- `src/cpp/lib/hlr_projection.cpp`
- `src/cpp/lib/model_bounds.cpp`
- `src/cpp/lib/geometer/c_api.h`
- `docs/design/wasm.md`

## Boundaries

Geometer should own:

- STEP/AP203/AP214/AP242 import.
- XCAF assembly/product/component inspection.
- TopoDS/BRep topology enumeration.
- BRepGraph feasibility testing.
- Tessellation with body/face/triangle identity.
- Writing annotated AP242 and reloading it.

The browser app should own:

- Selection UX.
- Grouping bodies/faces into semantic targets.
- Assigning small annotation payloads such as `terminal = "1"` or
  `kind = "pin1_indicator"`.

Viz and shared data_models should not own this spike. Once Geometer proves the
stable concepts, the durable annotation vocabulary can move into `data_models`.

## Core Design Rule

Do not make raw STEP entity numbers, glTF node names, or Three.js object ids the
authoritative identity.

Use Geometer-issued topology ids at the authoring boundary. Geometer may attach
provenance and repair hints to those ids:

```json
{
  "id": "face:body:3:7",
  "kind": "face",
  "body_id": "body:3",
  "face_index": 7,
  "step_ref": "#456",
  "fingerprint": {
    "area_mm2": 1.234,
    "centroid_mm": [1.0, 2.0, 0.0],
    "normal_hint": [0.0, 0.0, 1.0],
    "bbox_mm": [[0.5, 1.5, -0.01], [1.5, 2.5, 0.01]],
    "surface_kind": "plane"
  }
}
```

Resolution order on reload should be:

1. WN topology annotation link inside AP242.
2. WN persistent annotation id.
3. STEP entity reference, if still meaningful.
4. Body/face index for unchanged Geometer output.
5. Geometry fingerprint fallback.

## Initial Annotation Vocabulary

Keep the A0 payload intentionally small:

```json
{
  "schema": "wn.step242.package_annotation.a0",
  "kind": "terminal",
  "terminal": "1",
  "role": "electrical_pin"
}
```

Use a distinct kind for orientation marks:

```json
{
  "schema": "wn.step242.package_annotation.a0",
  "kind": "pin1_indicator",
  "terminal": "1",
  "role": "orientation_marker"
}
```

Terminology:

- `terminal`: physical conductor/contact geometry that maps to a footprint
  pad/pin and can carry a net.
- `pin1_indicator`: package mark/chamfer/notch/dot that indicates orientation
  but is not electrical geometry.

## AP242 Embedding Strategy To Test

Test two layers together:

1. A product-level WN summary block for quick scan and repair data.
2. Topology-linked AP242 entities for durable face/body targets.

Candidate topology-linked AP242 constructs:

- `SHAPE_ASPECT`
- `COMPOSITE_SHAPE_ASPECT`
- `SHAPE_ASPECT_RELATIONSHIP`
- `GEOMETRIC_ITEM_SPECIFIC_USAGE`
- `PROPERTY_DEFINITION`
- `PROPERTY_DEFINITION_REPRESENTATION`
- `DESCRIPTIVE_REPRESENTATION_ITEM`

For a single-face terminal, a simple `SHAPE_ASPECT` is enough. Use
`COMPOSITE_SHAPE_ASPECT` only for groups with at least two component
relationships.

## Browser Render Handoff

The browser should receive a renderable model plus selection identity. Two
routes should be compared:

### Route A: GLB With Geometer Extras

Emit GLB nodes/meshes/primitives arranged for picking. Put Geometer topology
ids and selection metadata in glTF `extras`.

Pros:

- Easy Three.js integration.
- Easy body-level picking.
- Useful for quick student-facing UI.

Cons:

- Face-level picking may require one primitive per face or per selectable face
  group.
- Large or complex models may produce many primitives.

### Route B: GLB Plus Topology/Pick Sidecar

Emit compact GLB and a JSON sidecar mapping body/face ids to mesh primitive or
triangle ranges.

Pros:

- Better long-term render efficiency.
- Keeps semantic data explicit and testable.

Cons:

- More custom browser picking/highlighting code.

The spike may start with Route A if it shortens the path to a working browser
selection UI. The persistent STEP annotation contract must not depend on GLB.

## BRepGraph Relevance

OCCT 8.0 introduces BRepGraph as a graph layer for B-Rep storage, assembly
traversal, topology references, owner-scoped geometry/mesh records, persistent
layers, and runtime caches.

Useful concepts for this project:

- `ProductDef`, `OccurrenceDef`, and `OccurrenceRef` align with assembly and
  component occurrences.
- `FaceDef`, `EdgeDef`, `CoEdgeDef`, `ShellDef`, and `SolidDef` align with
  annotation targets.
- Stable node/ref/item UIDs could become future Geometer topology ids.
- Persistent layers are a strong conceptual fit for WN metadata attached to
  topology graph identity.
- Persistent face triangulation and owner-scoped mesh records could simplify
  face-to-web-mesh highlight mapping.

Primary sources:

- https://dev.opencascade.org/content/brepgraph-new-topology-geometry-graph-coming-occt-80
- https://github.com/Open-Cascade-SAS/OCCT/discussions/1170

BRepGraph should not block the first proof. The first proof can use XCAF and
TopoDS. The BRepGraph task is to determine whether Geometer can align its new
topology ids with BRepGraph UIDs now or later.

## Investigation Questions

### STEP/XCAF Import

- For representative AP203/AP214/AP242 package models, what free shapes,
  assemblies, components, names, colors, layers, and sub-shapes does XCAF expose?
- Do common generated models expose pins as separate solids, component
  occurrences, named sub-shapes, or only faces in one fused solid?
- What model tree should Geometer normalize from XCAF for browser selection?

### STEP Entity Mapping

- Can Geometer recover source STEP entities for imported solids/faces through
  OCCT transfer reader/work-session APIs?
- If yes, which entity kinds map reliably: `MANIFOLD_SOLID_BREP`,
  `CLOSED_SHELL`, `ADVANCED_FACE`, mapped items, component occurrences?
- Does the mapping survive after writing AP242 and reloading it?
- Are STEP `#NNN` ids useful only as provenance, or can they be part of a
  durable repair path?

### TopoDS Identity

- What body/face traversal order is deterministic for unchanged input?
- What minimal fingerprints should be computed for repair matching?
- How should Geometer represent user-created groups of faces and bodies?

### BRepGraph

- Do OCCT 8.0 headers in the Geometer dependency build expose BRepGraph?
- Can a `TopoDS_Shape` imported from STEP be added to BRepGraph and rebuilt
  without losing topology?
- Can BRepGraph preserve product/occurrence structure, or does conversion from
  a flattened `TopoDS_Shape` already lose that information?
- Are BRepGraph UID and layer APIs mature enough to influence the A0 Geometer
  topology id design?

### AP242 Writing

- Can OCCT/XCAF write product-level custom properties without text injection?
- Can OCCT write topology-linked `SHAPE_ASPECT` /
  `GEOMETRIC_ITEM_SPECIFIC_USAGE` records for selected faces/bodies?
- If exact topology-linked writing is not practical through OCCT APIs, what is
  the smallest controlled Part 21 append/injection approach that validates on
  reload?

## Proposed Geometer Experimental APIs

Names are placeholders for the spike.

```text
step_annotation_view(step_bytes, options_json)
  -> package bytes:
       render.glb or mesh packet
       topology_view.json
       annotations.json
```

```text
step_apply_annotations(step_bytes, annotation_edit_json)
  -> annotated_step_bytes
  -> report.json
```

Example edit:

```json
{
  "schema": "geometer.step_annotation_edit.a0",
  "source_hash": "sha256:...",
  "edits": [
    {
      "op": "set_annotation",
      "targets": ["face:body:3:7", "face:body:3:8"],
      "annotation": {
        "schema": "wn.step242.package_annotation.a0",
        "kind": "terminal",
        "terminal": "1",
        "role": "electrical_pin"
      }
    }
  ]
}
```

Example topology view:

```json
{
  "schema": "geometer.step_topology_view.a0",
  "source": {
    "format": "step",
    "ap": "AP214",
    "hash": "sha256:..."
  },
  "occurrences": [],
  "bodies": [],
  "faces": [],
  "groups": [],
  "annotations": []
}
```

## Experiment Plan

### 1. Inventory Existing Fixtures

Use the existing `tests/fixtures/step/embedded_models` corpus plus a few known
Altium IPC wizard outputs. Produce a JSON inventory for each file:

- AP schema from STEP header.
- XCAF free shape count.
- Assembly/component count.
- Body/solid count.
- Face count.
- Names/colors/layers discovered.
- Whether pins appear as separate bodies or only as face regions.

### 2. XCAF Topology View Prototype

Add a native-only diagnostic command or test helper that reads STEP through
`STEPCAFControl_Reader` and emits `geometer.step_topology_view.a0` JSON.

Keep this behind tests or an experimental CLI flag until the shape of the API is
proven.

### 3. Face Tessellation Mapping

Tessellate solids with `BRepMesh_IncrementalMesh` and prove Geometer can map:

```text
body id -> face id -> triangle ranges
```

This can be independent of GLB at first. A direct JSON/mesh packet is acceptable
for the spike.

### 4. GLB Selection Prototype

Test whether `RWGltf_CafWriter` can preserve enough node/mesh structure and
`extras` for body or face selection. If not, test a custom GLB emission path or
sidecar mapping.

### 5. Product-Level Metadata Round Trip

Write a simple AP242 file with a WN product-level summary block. Reload it and
extract the same payload. This is the minimum survival proof.

### 6. Topology-Linked Metadata Round Trip

Attach terminal metadata to one selected body or face group, write AP242, reload,
and resolve `terminal = "1"` back to the selected topology.

### 7. BRepGraph Feasibility Spike

Build a small native-only test that:

- Imports a STEP fixture to `TopoDS_Shape`.
- Builds a BRepGraph from the shape.
- Emits counts and UIDs for products, occurrences, solids, shells, faces, and
  mesh records if available.
- Rebuilds a `TopoDS_Shape` from the graph and verifies bounds and counts.

Also test whether XCAF assembly/product information can be represented in
BRepGraph without flattening.

## Acceptance Criteria For This Research Slice

- A report/test artifact shows what structure is available from at least:
  - one clean connector with separate pin bodies;
  - one Altium/IPC wizard-generated package;
  - one slab/fused package;
  - one AP203 or AP214 vendor model;
  - one AP242 model if available.
- Geometer can emit a browser-usable topology/render view with stable ids for
  body-level selection.
- Geometer can emit at least one face-level selectable id and map it to render
  triangles or a GLB primitive.
- Geometer can write and reload a WN product-level metadata block.
- Geometer can write and reload one topology-linked terminal annotation, or the
  report documents exactly why the first implementation must use summary plus
  repair fingerprints.
- The report recommends whether the student tool should start from body
  selection, face selection, or both.
- The report recommends whether BRepGraph should influence the first public
  Geometer topology id contract.

## Deferred Work

- Automatic pin detection.
- Unibody splitting.
- Hitbox inference.
- KiCad/Altium rebake beyond byte-preservation regression tests.
- Full `pcbent` or `3d_part_a0` format.
- Shared `data_models` schema hardening.
- Viz production integration.

## Handoff Summary

The next session should start by adding native diagnostics in Geometer, not by
building a polished browser app. The browser app becomes useful once Geometer
can answer three questions:

1. What did the STEP file contain structurally?
2. What exactly did the user select?
3. Can that selection be written to AP242 and recovered after reload?

Only after those answers are backed by fixtures should the student-facing tool
be scoped.

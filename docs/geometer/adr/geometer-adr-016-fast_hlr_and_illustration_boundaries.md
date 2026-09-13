+++
type = "adr"
id = "geometer-adr-016"
domain = "geometer"
status = "accepted"
title = "Separate Fast Vector HLR And Mesh Illustration"
created = "2026-09-03"
+++

# ADR 016: Fast HLR And Illustration Boundaries

## Status

Accepted.

## Context

Geometer already exposes OCCT exact and polygonal hidden-line projection for
STEP models. Documentation generators need a lower-latency vector alternative,
while browser visualization also benefits from styled mesh illustration and a
depth-buffer raster view. Those products share source geometry but do not have
the same output or portability contract.

Synthesized assemblies, including extruded analytic PCB layers, may already be
tessellated and should not need a STEP round trip merely to use Fast HLR. At the
same time, OCCT remains the appropriate boundary for STEP import and
tessellation, and existing exact/poly callers must not silently change
algorithms or option meanings.

## Decision

Geometer publishes two separate products:

1. Fast vector HLR is generic C++ geometry. It accepts prepared triangle data,
   provides prepare-once and one-shot value APIs, and returns renderer-neutral
   projected segments through `geometry.hlr_projection.result.a0`.
2. Mesh illustration is a TypeScript browser/Node package. It owns preparation,
   visibility ordering, safe fusion, styling, colorization, SVG/Canvas
   rendering, caching, and disposal. Its serialized contracts begin at A0.
The evaluated browser raster-HLR prototype is not published as a production
module or contract. A future raster renderer would remain a presentation
product rather than a vector-projection backend and requires its own decision.

The governed portable operations are:

- `geometry.model_hlr_projection.a0`, with a STEP/model attachment; and
- `geometry.mesh_hlr_projection.a0`, with an indexed-triangle-mesh A0 packet.

Both use `geometry.hlr_projection.options.a0`. Omitted selectors choose `fast`
detail and `fast-mesh-shadow` for model/STEP and indexed-mesh projection.
The existing `poly`, `exact`, `hlr-close`, and `mesh-shadow` selections remain
available explicitly for compatible model sources. Fast-only controls are
nested under `fast`; they do not reinterpret OCCT edge-category options.

The TypeScript package may offer a convenience composition that combines Fast
linework and illustration, but the underlying HLR and illustration contracts
remain independently callable. Application-specific PCB, documentation, and
visualizer styling stays outside Geometer.

Existing STEP-specific C ABI, Python, CLI, JSON, and WASM entry points remain
compatibility surfaces under ADR 007. Promotion evidence and release signoff
remain governed by ADR 010 and are not implied merely by accepting this
architectural boundary.

## Consequences

### Native illustration extension (2026-09-11)

The original TypeScript-first boundary now also has a C++ illustration renderer
and an executable SVG operation. The additive
`geometry.mesh_illustration_geometry.a0` operation exposes its shared ordered
drawing data for custom renderers. Geometry output stops before SVG writing;
direct C++ returns owning values, while IPC carries a governed JSON attachment.
TypeScript exposes the same drawing contract using its existing prepared scene.
The [drawing contract](../../design/mesh-illustration-geometry.md) records units,
paint order, holes, opacity, resource limits and target availability. HLR remains
independently callable, and application placement/provenance remains outside it.

Consumers with synthesized geometry can encode one bounded indexed mesh and
use Fast HLR without constructing STEP. STEP consumers continue to use OCCT
for import and tessellation before the triangle visibility engine runs.

Documentation engines can use deterministic vector linework, and illustration
consumers do not need to copy algorithms from a demo. The separation permits
the illustration renderer to evolve without changing the portable HLR result.

Perspective vector guarantees, multithreaded WASM visibility, stable serialized
prepared models, and changing the default HLR
backend require later decisions.

### Combined model illustration extension (2026-09-13)

Geometer also publishes a combined model-illustration boundary for consumers
that need a completed technical illustration rather than an intermediate mesh.
It accepts either one attached model or a bounded generic analytic 2.5D scene,
imports or lowers that source once into an in-memory mesh collection, and feeds
the same collection to Fast vector detail, Fast Mesh Shadow, visibility,
shading and same-color surface fusion. Each engine may derive its own private
view-specific preparation from that collection. Its two portable operations
return either deterministic SVG or the existing renderer-neutral
illustration-geometry attachment.

The combined boundary does not replace the independent model tessellation,
Fast HLR, mesh HLR or mesh illustration operations. Consumers that own meshes,
need interactive triangle data, select older projection algorithms, or compose
multiple model attachments continue to use those surfaces. The A0 combined
boundary accepts at most one model attachment; its analytic alternative uses
definition-local extrusion, cylinder and sphere geometry with occurrence
transforms. Application adapters resolve file paths, component composition and
PCB-specific meaning before calling Geometer.

STEP is the only attached model media type in A0. Analytic input stays analytic
on the public boundary even when native code lowers it to a private prepared
representation. Neither path serializes an intermediate mesh collection.
TypeSpec owns the new request/result structures and operation declarations after
their complete vertical is promoted under ADR 010.

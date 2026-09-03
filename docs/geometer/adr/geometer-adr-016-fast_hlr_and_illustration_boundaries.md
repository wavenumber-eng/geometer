+++
type = "adr"
id = "geometer-adr-016"
domain = "geometer"
status = "accepted"
title = "Separate Fast Vector HLR, Mesh Illustration, And Raster HLR"
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

Geometer publishes three separate products:

1. Fast vector HLR is generic C++ geometry. It accepts prepared triangle data,
   provides prepare-once and one-shot value APIs, and returns renderer-neutral
   projected segments through `geometry.hlr_projection.result.a0`.
2. Mesh illustration is a TypeScript browser/Node package. It owns preparation,
   visibility ordering, safe fusion, styling, colorization, SVG/Canvas
   rendering, caching, and disposal. Its serialized contracts begin at A0.
3. Raster HLR is an explicitly browser-only pixel renderer. It is not a vector
   projection backend and does not define documentation geometry.

The governed portable operations are:

- `geometry.model_hlr_projection.a0`, with a STEP/model attachment; and
- `geometry.mesh_hlr_projection.a0`, with an indexed-triangle-mesh A0 packet.

Both use `geometry.hlr_projection.options.a0`. `projection_algorithm` adds
`fast`; model/STEP projection retains the `poly` default, while indexed-mesh
projection defaults to `fast`. `outline_algorithm` adds `fast-mesh-shadow` as
the indexed-mesh default without changing the model defaults or existing
`hlr-close` and `mesh-shadow` selections. Fast-only controls are nested under
`fast`; they do not reinterpret OCCT edge-category options.

The TypeScript package may offer a convenience composition that combines Fast
linework and illustration, but the underlying HLR, illustration, and raster
contracts remain independently callable. Application-specific PCB,
documentation, and visualizer styling stays outside Geometer.

Existing STEP-specific C ABI, Python, CLI, JSON, and WASM entry points remain
compatibility surfaces under ADR 007. Promotion evidence and release signoff
remain governed by ADR 010 and are not implied merely by accepting this
architectural boundary.

## Consequences

Consumers with synthesized geometry can encode one bounded indexed mesh and
use Fast HLR without constructing STEP. STEP consumers continue to use OCCT
for import and tessellation before the triangle visibility engine runs.

Documentation engines can use deterministic vector linework, interactive
applications can opt into raster output, and illustration consumers do not
need to copy algorithms from a demo. The separation also permits the
illustration renderer and raster implementation to evolve without changing the
portable HLR result.

Perspective vector guarantees, multithreaded WASM visibility, stable serialized
prepared models, native illustration rendering, and changing the default HLR
backend require later decisions.

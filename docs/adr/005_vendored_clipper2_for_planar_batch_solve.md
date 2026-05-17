# ADR 005: Vendored Clipper2 For Planar Batch Solve

## Status

Accepted

## Context

Browser PCB and CAD viewers need a generic way to solve many planar filled
geometry jobs without round-tripping each small boolean operation through a
JavaScript adapter. The first high-value case is filled copper-like geometry:
closed rings, open strokes that need round/miter offsets, subtract rings, and a
final clipping region. The operation itself is not PCB-specific, so it belongs
in Geometer as a generic planar batch solve.

Geometer already vendors small source dependencies when that materially improves
fresh-clone build reliability. RapidJSON follows this policy for OCCT GLB export.

## Decision

Geometer vendors Clipper2 2.0.1 under `third_party/clipper2/` and uses it for
the v0.3 planar batch solve API.

The vendored copy includes only:

- `CPP/Clipper2Lib/include`
- `CPP/Clipper2Lib/src`
- upstream `LICENSE`
- `README.geometer.md`

The public Geometer API remains generic. It accepts rings, stroke groups,
subtract rings, final clip rings, and per-job flags. It does not expose
downstream concepts such as boards, nets, pads, components, or visualizer
policy.

## Consequences

Fresh Geometer clones can build the planar solve API without another git clone
or package manager step.

The browser C ABI can process a packed batch of planar jobs in one WASM call,
which gives downstream apps a path to remove repeated JavaScript-to-WASM
marshalling around offset/boolean/extract phases.

Geometer must preserve the upstream BSL-1.0 license notice in source
distributions.

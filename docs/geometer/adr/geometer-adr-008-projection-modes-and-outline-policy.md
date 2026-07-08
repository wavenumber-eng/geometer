+++
type = "adr"
id = "geometer-adr-008"
domain = "geometer"
status = "accepted"
title = "ADR 008: Projection Modes And Outline Policy"
created = "2026-07-07"
+++

# ADR 008: Projection Modes And Outline Policy

## Status

Accepted.

## Context

Assembly projection consumers need a clear distinction between:

- `outline`: an assembly silhouette intended to overlay on pads and show part
  presence without interior drafting line art.
- `detail`: configured HLR detail edges for connectors, mechanical features,
  and inspection views.
- `bbox`: the projected 3D shape bounding box for occasional coarse placement
  and diagnostics.

The older public name `simple` caused confusion with application-level
assembly projection profiles and with HLR profile/edge-category terminology.

## Decision

The 2026-06-09 release removes the public `simple` mode and ships projection
schema `geometry.projection.b0` with `modes.outline`, `modes.detail`, and
`modes.bbox`.

`outline` may be computed by mesh shadow or by closed HLR edges. Mesh shadow is
the preferred default for assembly projection. `detail` remains configurable
through HLR edge-category options. `bbox` is cheap projected bounds geometry and
does not replace exact model bounds APIs.

## Consequences

Downstream callers must upgrade from `simple` to `outline`. Documentation and
examples must avoid using `profile` for the projection mode because that word is
reserved for application-level assembly projection policy.

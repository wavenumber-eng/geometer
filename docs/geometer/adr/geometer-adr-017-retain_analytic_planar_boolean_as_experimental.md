+++
type = "adr"
id = "geometer-adr-017"
domain = "geometer"
status = "accepted"
title = "Retain Analytic Planar Boolean As Experimental"
created = "2026-09-05"
+++

# ADR-017: Retain Analytic Planar Boolean As Experimental

## Status

Accepted on 2026-09-05. This decision changes the support posture of
`geometry.analytic_planar_boolean_batch.a0`; it does not remove the
implementation or change its A0 logical and packed contracts.

## Context

The analytic planar Boolean solver was developed for a MATZ A0 direction that
would combine large amounts of PCB copper, including whole-layer inputs, into
one analytic line/arc entity. That integration direction was abandoned after
the visualization requirements were clarified. Preserving analytic arcs adds
substantial topology, numeric-certification, normalization, provenance, and
resource-accounting complexity that is not needed when a sampled polygon result
is sufficient.

The filtered solver is useful and passes its governed fixtures, but it can fail
closed on otherwise valid inputs when numeric, topology-resolution, carrier, or
resource limits are reached. The existing evidence does not establish the
reliability or scalability required for production whole-board or whole-layer
copper aggregation.

Geometer already provides Clipper2-backed planar Boolean and offset paths. They
are the preferred basis for visualization workflows that can consume
polygonized geometry.

## Decision

- Retain the analytic implementation, public operation identity, generated
  DTOs, packet codecs, tests, fixtures, demos, and research/qualification tools.
- Classify the analytic solver as **experimental and not production-ready**.
  A frozen packet or callable client surface does not imply production support.
- Do not recommend the analytic solver as the default PCB visualization path or
  for combining an entire board or copper layer into one analytic entity.
- Document that an analytic job may fail closed on inputs accepted by its
  contract. Callers must handle job diagnostics and must not depend on
  successful completion for production workflows.
- Prefer the Clipper2-backed planar operations for production visualization
  when sampled polygon output meets the consumer's requirements.
- Treat the production-oriented language and gates in ADR-013 and the original
  MATZ design packet as historical implementation goals, not a statement of
  current maturity. Any future production promotion requires a new explicit
  decision and representative reliability evidence.

## Consequences

- No solver code, contract identity, packet bytes, or generated projection
  changes are required by this decision.
- Existing experimental users can continue evaluating the analytic surface and
  its exact-oracle tooling.
- Consumer documentation must distinguish API availability and wire stability
  from solver maturity.
- The abandoned whole-layer MATZ direction no longer acts as a production gate
  or recommendation for Geometer consumers.

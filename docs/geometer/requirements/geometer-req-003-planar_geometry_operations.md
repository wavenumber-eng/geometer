+++
type = "requirement"
id = "geometer-req-003"
domain = "geometer"
status = "implemented"
title = "Planar Geometry Operations"
created = "2026-08-18"

[[verification_refs]]
kind = "local_file"
target = "tests/L0_cpp_foundation/STRATUM.toml"
+++

# REQ-003: Planar Geometry Operations

## Summary

Geometer provides generic 2D contour, boolean, offset, and triangulation
operations used by projection simplification and downstream browser tools.
The Clipper2-backed operations are the supported path for production planar
visualization. The retained analytic line/arc solver is experimental and not
production-ready; its implemented status records code availability, not a
reliability guarantee. See
[ADR-017](../adr/geometer-adr-017-retain_analytic_planar_boolean_as_experimental.md).

## Requirements

1. Convert projected line segments into closed planar contour rings.
2. Solve planar filled geometry batches from packed little-endian binary
   request packets.
3. Support subject rings, local subtract rings, common subtract rings, open
   stroke offsets, and optional final clipping rings.
4. Return outlines, holes, counters, and area in deterministic packed binary
   response packets.
5. Keep packed binary packet versions explicit and documented.
6. Make planar byte APIs available through native C++, C ABI, WASM, and CLI
   diagnostics.
7. Retain the experimental analytic planar Boolean operation and preserve line
   and circular-arc result fragments when a job completes; polygonization is a
   consumer projection, not that operation's authoritative result.
8. Use a speed-first filtered numeric solver as the normal experimental
   analytic path. Arbitrary-precision real-algebraic processing is not a
   required hot-path dependency.
9. Keep the public coordinate unit and output grid at one integer nanometer,
   while applying a fixed 50 nm topology-resolution envelope. Geometry at or
   below that envelope may merge, bridge, or collapse deterministically;
   topology separated by more than 50 nm must be preserved.
10. Keep every published analytic boundary within 50 nm Hausdorff distance of
    the resolved ideal boundary and produce equivalent decisions natively and
    under WASM.
11. Prune curve pairs with a spatial broad phase before intersection testing,
    govern the remaining candidate-pair count per job, and exercise dense
    real-board inputs in performance qualification.
12. Retain exact algebraic machinery only as an isolated conformance oracle,
    diagnostic tool, or measured bounded fallback. Exceeding a fallback budget
    fails the affected job and never triggers unbounded symbolic work.
13. Do not treat the analytic operation as a dependable whole-board or
    whole-layer copper union path. Valid jobs may fail closed at documented
    numeric, topology, carrier, or resource boundaries.
14. Prefer the Clipper2-backed planar operations for production visualization
    when sampled polygon output satisfies the consumer contract.

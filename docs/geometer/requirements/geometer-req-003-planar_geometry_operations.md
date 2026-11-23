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
The Clipper2-backed operations are the supported path for production planarvisualization.

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

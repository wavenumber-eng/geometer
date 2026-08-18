+++
type = "adr"
id = "geometer-adr-012"
domain = "geometer"
status = "accepted"
title = "Use Exact Arrangement For Analytic Planar Booleans"
created = "2026-08-18"
+++

# ADR-012: Exact Arrangement For Analytic Planar Booleans

## Status

Superseded for production implementation by
[ADR-013](geometer-adr-013-filtered_resolution_bounded_planar_boolean.md) on 2026-08-15.
This ADR remains the historical record of the exact-first feasibility work.

Independent technical review approved the analytic design at head
`b86a065c5926c35f1eee23a9ba1cef890689c7d7`, covering normative remediation
revision `529c768e559b4c88874264748d4186e775c8a4dd`. Focused review later accepted
the isolated TypeSpec and packed-projection reconciliation at
`f4b6a9b87bf16f57ef29dae22150b16f2a742b64`. Exact-backend feasibility and
OCCT qualification were the gates under this decision. ADR-013 replaces those
production-architecture requirements while retaining the exact implementation
as a non-primary conformance oracle and optional bounded fallback.

## Context

`geometry.analytic_planar_boolean_batch.a0` must preserve line and circular-arc
topology, produce identical canonical results natively and under Emscripten,
normalize once to the integer-nanometer grid, and retain stage-aware material
lineage. OCCT demonstrates useful analytic Boolean feasibility, but its
binary64 tolerance decisions cannot prove that candidate topology is complete
or make exact half-grid and coincidence decisions. Clipper2 is robust for
sampled integer polygons but cannot be the authority for analytic arc output.

## Decision

Geometer owns an authoritative exact line/circle half-edge arrangement for
this operation. It enumerates every boundary occurrence and every curve pair
not proven disjoint, constructs and classifies all arrangement faces, applies
the ordered regularized set operations, and carries material lineage through
those exact cells.

Real-algebraic scalars use a primitive square-free integer defining polynomial,
a unique dyadic isolating interval, and a signed-subresultant Thom encoding.
Exact arithmetic and comparisons use resultant, polynomial-GCD,
square-free-factorization, signed-subresultant, and root-isolation algorithms
over `boost::multiprecision::cpp_int`. Equality and identically-zero decisions
are exact; interval refinement is acceleration, not the equality oracle.

Every algebraic degree, coefficient size, arrangement count, provenance count,
predicate-work count, and memory allocation is charged against the governed A0
limits. Exceeding a limit fails the isolated job before an approximate decision
is used.

OCCT remains an independently audited candidate-topology engine. It may supply
adjacency and traversal hints, but cannot suppress an exact pair, vertex,
half-edge, face, or classification. Every stage requires a bidirectional map
between OCCT candidate boundaries and exact material boundaries; mismatch
fails closed. Clipper2 remains a sampled differential oracle only.

The portable exact backend and every decision algorithm must pass a focused
native/Emscripten feasibility target before production solver implementation.
Separately, exact tags `V8_0_0` and `V8_0_1` are qualified side-by-side; no
upstream-master OCCT pin is permitted.

## Consequences

- Canonical geometry, topology, normalization, and diagnostics no longer
  depend on OCCT tolerance or traversal behavior.
- The solver is substantially more work than wrapping OCCT Boolean output and
  requires explicit combinatorial and arithmetic resource limits.
- Native and WASM share one C++17 exact implementation and fail identically at
  governed limits.
- OCCT upgrades can improve candidate behavior or performance without silently
  changing the authoritative mathematical result.
- Production work stops if the exact backend cannot satisfy the native/WASM
  feasibility, performance, or mutation-test gates.

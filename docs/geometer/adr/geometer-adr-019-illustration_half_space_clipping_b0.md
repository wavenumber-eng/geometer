+++
type = "adr"
id = "geometer-adr-019"
domain = "geometer"
status = "accepted"
title = "Version Illustration Clipping As B0 Operations"
created = "2026-09-19"
+++

# ADR 019: Version Illustration Clipping As B0 Operations

## Decision

Post-transform illustration clipping is a semantic pipeline change, so the
five affected serialized operations move to B0 as complete operation
interfaces. Nested vectors, planes, clipping limits, and fragment metadata are
ordinary members of those interfaces rather than independently negotiated
protocol generations.

The A0 operations remain accepted for compatibility and adapt to the common
native preparation pipeline with no clipping. Generic ABI and IPC framing stay
A0 and dispatch the logical generation from the exact operation identity.
Maintained package helpers, examples, demos, and release validation use B0.

The initial cap policy is only `none`. Empty fragments are successful and omit
bounds. Fragment and linework digests bind transforms, normalized clipping,
limits, and prepared geometry so independently supplied HLR cannot be composed
with a different shaded fragment.

## Consequences

Consumers get an explicit migration boundary and can reject unfamiliar B0
fields instead of silently applying A0 semantics. The implementation retains
one clipping kernel and one transport framing path; compatibility does not
create a second renderer. Future cap generation or footprint-aware occlusion
requires a new contract decision.

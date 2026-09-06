+++
type = "adr"
id = "geometer-adr-010"
domain = "geometer"
status = "accepted"
title = "Use TypeSpec As Contract Authority"
created = "2026-08-18"
+++

# ADR-010: TypeSpec Contract Authority And Promotion

## Status

Accepted on 2026-08-12. Implementation is tracked by
[GitHub issue #18](https://github.com/wavenumber-eng/geometer/issues/18).

## Context

Geometer currently expresses its callable structures in several handwritten
forms: C++ value types, JSON readers and writers, C ABI declarations,
Emscripten export lists, Python dataclasses and mappings, JavaScript examples,
CLI dispatch, and prose design documents. These surfaces implement useful
contracts, but a new operation must be reproduced in several languages and can
drift between native, browser, and executable-backed consumers.

The ALX domain in `appz/data_models` demonstrated a useful pattern: author
portable structure in TypeSpec, lower it into a normalized catalog, and drive
language projections and conformance evidence from that catalog. Geometer
needs the same mechanics without acquiring an ALX, application, or sibling
repository dependency.

TypeSpec cannot own geometry behavior, C allocation rules, Emscripten exports,
executable framing, or packed binary layouts by itself. Authority must remain
explicit for those concerns. Existing public interfaces also cannot change
merely because a generated representation differs from a handwritten one.

## Decision

Geometer will use authored TypeSpec plus an in-repository normalized catalog as
the structural authority for each contract after that contract is individually
promoted.

Before promotion, the existing implementation remains authority. The promotion
manifest under `docs/contracts/` records the current authority, target
authority, compatibility posture, required projections, and evidence state for
every known contract. A contract is promoted only when the manifest and all
required evidence agree; adding TypeSpec source alone does not transfer
authority.

### Authority boundaries

For a promoted contract, TypeSpec owns:

- serialized field names and types;
- required, optional, nullable, and default intent;
- closed objects, discriminated unions, and scalar constraints;
- operation request and response envelope structure;
- diagnostic payload structure; and
- attachment descriptors and their relationship to an operation.

The normalized catalog is a deterministic, loss-checked lowering of the
TypeSpec program. JSON Schema, generated reference documentation, C++ wire
DTOs/codecs, TypeScript, Rust, and Python projections consume that catalog.
Generators must reject unsupported or lossy constructs and must not maintain a
smaller handwritten root list.

C++ remains behavioral authority for model loading, OCCT work, planar solving,
projection, tessellation, topology, and all other geometry semantics. Generated
C++ values are wire DTOs mapped explicitly to focused public C++ value APIs;
they do not replace public semantic value types without a separate
compatibility decision.

The following remain separately governed transport authorities:

- C ABI names, fixed-width layouts, generations, ownership, allocation, free
  functions, and Emscripten exports;
- executable process lifecycle, framing, limits, correlation, cancellation,
  logging, and shutdown; and
- packed planar magic values, versions, endianness, flags, and layouts.

Reviewed Geometer-specific metadata may enter the normalized catalog so that
generators can verify these surfaces. Ordinary JSON Schema is not sufficient
authority for them.

### Presence, defaults, and compatibility

Wire presence is distinct from an effective operation value. Generated input
decoders must preserve absent versus present fields. Partial option-patch DTOs
must not eagerly materialize defaults. Compatibility and batch layers merge
defaults, top-level patches, and job patches before mapping to canonical C++
options.

Canonical contracts are closed by default. Compatibility aliases and legacy
input shapes stay in explicit adapters and normalize to canonical DTOs; they do
not enter generated canonical types. Existing C++, C ABI, CLI, Python, JSON,
binary, and browser surfaces remain supported until their manifest entry has an
approved compatibility disposition and migration evidence.

### Required projections

The production projection set is:

- JSON Schema and generated HTML contract reference documentation using the
  shared Wavenumber visual system;
- C++17 wire DTOs and strict codecs;
- TypeScript types, strict codecs, and browser/Web Worker WASM client helpers;
- Rust types, strict codecs, and executable-pipe client helpers; and
- Python types and strict codecs integrated behind the compatible public
  executable-backed package.

Generated Python is required promotion scope, not an optional follow-up. Public
Python names and behavior remain behind wrappers or adapters where direct
generated-class exposure would be incompatible.

### Package identities and runtime posture

The TypeScript package identity is `@wavenumber/geometer`. It is an ESM package
with generated declarations and explicit exports for the high-level client,
contracts/codecs, direct WASM transport, and Web Worker transport. Standalone
browser demo output may be bundled from that source, but is not a second package
authority.

`appz/viz` is a named compatibility consumer. Its vendored Geometer 2026.6.10
browser artifacts, factories, runtime helpers, per-operation C ABI symbols, and
packed planar formats remain supported while Viz is JavaScript-based. Its
planned TypeScript upgrade targets `@wavenumber/geometer`; compatibility does
not transfer until Viz passes its own integration suite on the generated client
and the promotion manifest records a replacement snapshot.

The Rust client crate identity is `geometer-client`. Its first supported client
is asynchronous and uses Tokio for pipe I/O and its convenient default process
launcher. A caller may instead transfer already-contained async streams and a
generic lifecycle controller before negotiation; this changes process policy,
not the governed stdio protocol. Generated wire models use Serde; a synchronous
facade is deferred until a named consumer requires it.

The existing PyPI distribution and import identities remain `wn-geometer` and
`geometer`. Generated Python lives in an internal package namespace and depends
on a small in-package strict-codec runtime using the Python standard library.
No new validation framework becomes a wheel runtime dependency unless the
Python compatibility analysis demonstrates that the internal runtime cannot
meet the governed constraints.

These names were unclaimed in the public npm and crates.io registry queries
performed on 2026-08-12. Registry availability is not a substitute for
Wavenumber publication authorization.

### Promotion rule

Promotion is operation-by-operation. Each operation must:

1. have an approved canonical identity and compatibility posture;
2. be present in TypeSpec and the normalized catalog;
3. generate every required projection deterministically;
4. pass shared strict, schema, semantic, diagnostic, and transport vectors as
   applicable;
5. complete native executable and browser WASM round trips;
6. migrate its maintained examples and demos to the generated TypeScript
   client;
7. update docs of record, guides, compatibility notes, and release notes; and
8. pass named downstream compatibility snapshots, including Viz while its
   legacy lane remains active; and
9. remove displaced handwritten structural authority or retain it only as a
   named compatibility adapter.

`model_bounds` is the first vertical pilot. Inventorying another operation does
not promote it or require its full TypeSpec graph in the pilot.

## Consequences

- A new logical interface can be projected consistently into the languages
  used by Geometer consumers.
- Browser applications and Rust executable clients gain supported high-level
  paths instead of reproducing allocation, JSON, or framing details.
- The Python package participates in the same structural authority while
  retaining its documented executable backend and compatibility surface.
- TypeSpec and generator tooling become build-time development dependencies,
  but never Geometer runtime dependencies.
- Generated HTML is a deterministic review projection. Geometer vendors the
  approved Wavenumber stylesheet/assets and never reads them from a sibling
  checkout during normal generation.
- Contract work carries an explicit conformance, documentation, compatibility,
  and release burden before promotion.
- Packed binary formats and transport ABIs continue to require focused design
  review rather than being inferred from a schema generator.

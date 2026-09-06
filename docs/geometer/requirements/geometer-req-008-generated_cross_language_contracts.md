+++
type = "requirement"
id = "geometer-req-008"
domain = "geometer"
status = "implemented"
title = "Generated Cross-Language Contracts"
created = "2026-08-18"

[[verification_refs]]
kind = "local_file"
target = "tests/typespec/STRATUM.toml"
+++

# REQ-008: Generated Cross-Language Contracts

## Summary

Geometer authors promoted operation structures in TypeSpec and ships generated,
compatible interfaces for native, browser/WASM, executable IPC, Rust,
TypeScript, and Python consumers.

## Requirements

1. Keep authored Geometer TypeSpec and its emitter in this repository. Do not
   depend on ALX, application domains, or a sibling checkout at build or
   runtime.
2. Lower TypeSpec into one deterministic normalized catalog and fail generation
   when a governed declaration is unsupported, lossy, missing an identity, or
   omitted from a required projection.
3. Generate JSON Schema and reference documentation, C++17 wire DTOs/codecs,
   TypeScript types/codecs, Rust types/codecs, and Python types/codecs for every
   promoted contract.
4. Keep generated C++ wire DTOs separate from focused public geometry value
   APIs. C++ remains the authority for geometry behavior.
5. Provide a packaged TypeScript client for the full and planar-only browser
   WASM targets and Web Workers. Consumers must not need direct pointer
   allocation, output-pointer decoding, or free calls for supported operations.
6. Provide a packaged Rust client for the framed `geometer` executable pipe,
   including handshake, request correlation, typed failures, raw attachments,
   timeout, queue cancellation, shutdown, unexpected-exit handling, and an
   additive process-adoption boundary for callers that apply platform
   containment before negotiation. While the async runtime remains available,
   every construction, close, failure and final-handle-drop path must terminate
   and reap or prove the supplied containment unit empty within one bounded
   deadline; a production controller's `Drop` is the synchronous platform
   backstop when the runtime itself is unavailable.
7. Use generated contract models and strict codecs at the public Python
   package's executable boundary while preserving documented names, call
   signatures, result conveniences, accepted aliases, and error behavior unless
   a versioned replacement is separately approved.
8. Transfer structural authority one operation at a time. TypeSpec source or a
   generated artifact alone must not claim promotion.
9. Test promoted contracts against shared governed vectors in C++, TypeScript,
   Rust, and Python. Each vector must declare its strict JSON, schema, semantic,
   diagnostic, canonical serialization, or transport-framing oracle and its
   exact, structural, or toleranced comparison policy.
10. Keep canonical contracts closed by default. Preserve legacy aliases and
    alternate shapes only in explicit compatibility adapters with a documented
    migration and retirement posture.
11. Migrate each maintained browser demo to TypeScript and the generated WASM
    client when its owning operation is promoted. All maintained demos must use
    generated interfaces before the contract program closes.
12. Update ADRs, requirements, design documents, developer and consumer guides,
    generated references, compatibility notes, examples, change history, and
    release/version notes for every affected published surface.
13. Keep release version, date-based C ABI generation, executable IPC
    generation, contract identities, and packed binary format versions separate
    and explicitly documented.
14. Require an independent design review of the generic C ABI and executable
    IPC A0 specifications before implementing those transports, plus an
    independent implementation review before program closure.
15. Preserve the frozen Viz 2026.6.10 browser/WASM compatibility snapshot until
    Viz explicitly migrates to the generated TypeScript client and supplies
    replacement integration evidence. Existing factory names, runtime helpers,
    required C ABI symbols, and packed format versions must remain usable.
16. Generate committed, deterministic HTML contract reference pages using the
    Wavenumber stylesheet, watermark, page idioms, an approved redistributable
    font, and offline relative-link behavior established from
    `appz/data_models`. Vendor reviewed assets and their required license
    evidence in Geometer; do not require a sibling checkout during generation
    or viewing.
17. Treat the HLR option/result family, model/mesh HLR operations, and mesh
    illustration serialized DTOs as A0 identities; keep their ergonomic package
    function names unversioned.

+++
type = "adr"
id = "geometer-adr-020"
domain = "geometer"
status = "accepted"
title = "Standard STEP Media Types"
created = "2026-10-14"
+++

# ADR-020: Standard STEP Media Types

## Context

Geometer accepts raw STEP attachments through native C ABI, executable IPC,
Rust, Python, TypeScript, browser WASM and Worker clients. Exact MIME-string
checks have rejected valid STEP inputs carrying auxiliary Source parameters.
Existing clients also default to the local `application/step` spelling.

The [IANA registration](https://www.iana.org/assignments/media-types/model/step)
defines `model/step` for STEP product models, with no required or optional
parameters. [RFC 2045 section 5.1](https://www.rfc-editor.org/rfc/rfc2045#section-5.1)
defines case-insensitive media type/subtype names and parameter syntax; unknown
parameters are ignored. [RFC 2077](https://www.rfc-editor.org/rfc/rfc2077) adds
the `model` top-level type. Source editing categories do not describe STEP
payload encoding, units, geometry or schema.

## Decision

New Geometer STEP request defaults and produced STEP descriptors use the bare
registered token `model/step`. `application/step` remains an input compatibility
spelling and remains readable in existing saved descriptors. It is not claimed
to be the registered STEP MIME type.

Each maintained language boundary uses a focused, bounded MIME-value parser
with a common fixture corpus. Input type/subtype tokens are case-insensitive;
spaces/tabs, comments and token or escaped quoted parameter values are parsed.
The whole value must be consumed. Valid unknown STEP parameters are ignored.
Substring matching and acceptance of malformed parameter tails are forbidden.
The framed field remains limited to 128 ASCII bytes. NUL, CR/LF and other
control characters are rejected; this is a MIME value in a binary attachment,
not an email header with line folding. Nested comments are bounded by that
field limit.

This equivalence applies only to input slots whose catalog declaration admits
STEP. Other vendor/packed media types retain their exact identity checks. The
existing dual STEP input declarations and ordering are preserved. Clients may
normalize accepted STEP input to `model/step` before sending it, including to
older native binaries that already admit that token. Direct native admission
still accepts valid parameterized values independently of client normalization.

Actual STEP bytes are parsed by the geometry implementation. Attachment size,
byte digest, ownership, placement and other operation checks remain unchanged.
Parameters never select compression, units, STEP schema or source transforms.

Output validation remains exact. New illustration source summaries use
`model/step`; their existing legacy enum member stays decodable. The structural
AP242 persistence descriptor is widened to decode both bare spellings, while
new owning producers must emit `model/step`. This metadata correction does not
implement or promote topology save or alter operation availability or maturity.

## Compatibility And Ownership

TypeSpec is the authored structural source. Catalog, schema and language
projections are regenerated together under ADR-010. Changed catalog digests
must be paired with matching generated clients and runtimes; catalog checks
are not weakened to admit arbitrary mixed builds.

No generic C ABI structure, executable frame, packed geometry packet or PCB
carrier layout changes. A C ABI generation bump is not implied by this MIME
correction. Actual release identities follow ADR-006. Existing public package
distribution is outside this internal development change.

CAD importers retain source-only editing evidence under their owning policies.
Geometer neither understands nor interprets that evidence. Moving Source
category evidence out of a saved MIME string is a separate carrier decision.

## Verification

Shared positive and negative fixtures cover both spellings, mixed case,
comments, quoted semicolons/escapes, malformed values and unrelated types.
Native direct admission and maintained client/IPC/WASM boundaries are tested.
Output tests assert bare `model/step`, legacy descriptor decoding and exact
rejection of undeclared output spellings. Geometry bytes/digests/options and
unrelated attachment rules remain equivalent. Real host consumers verify the
same model resources render without changing their saved Source evidence.

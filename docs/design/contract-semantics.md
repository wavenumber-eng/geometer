# Contract Semantics

ADR-010 defines structural authority and promotion. This document defines the
cross-language semantics that generated models and conformance tests must
preserve.

## Presence and null

An input field has three possible wire states:

1. absent;
2. present with a non-null value, including the declared default value; or
3. present with `null` only when TypeSpec explicitly declares the field
   nullable.

Generated decoders preserve these states. They do not turn an absent optional
field into a present default. `null` is rejected for non-nullable fields.

Generated patch models represent presence separately from the field's value.
This is required even when a language's ordinary optional type normally
combines absent and null.

## Defaults and layered option patches

TypeSpec records canonical default intent. Effective C++ operation options are
created only after all input layers are merged in this order:

```text
focused C++ defaults
  <- top-level batch option patch
  <- job-level option patch
```

Each later present field replaces the earlier value. An absent field has no
effect. A present field whose value equals the default still replaces the
earlier value. Compatibility aliases normalize to canonical patch fields before
the merge. Two aliases that supply the same canonical field in one object are a
contract error unless that compatibility adapter defines and tests an explicit
precedence rule.

Canonical request encoders omit absent optional fields. Patch encoders preserve
all present fields, including present default-valued fields. An encoder may omit
a present default only for a non-patch model whose generated metadata explicitly
declares that emission policy.

## Object and number policy

Canonical objects are closed unless a reviewed annotation identifies an open
extension bucket, its owner, interpretation rule, and promotion condition.
Strict decoders reject unknown fields, duplicate object keys, malformed UTF-8,
trailing JSON data, non-finite numbers, and values outside declared ranges.

Compatibility readers may accept named aliases or legacy shapes, but must emit
a canonical model or a path-specific diagnostic. They must not silently retain
unknown fields in canonical output.

## Diagnostics

Wire diagnostics contain:

- `category`: `transport`, `contract`, or `operation`;
- a stable namespaced string `code`;
- a human-readable `message`, which is not exact compatibility text;
- `retryable`;
- optional `path` as RFC 6901 JSON Pointer;
- optional operation identity; and
- optional request identifier.

An empty JSON Pointer identifies the whole JSON document. A diagnostic without
a meaningful document location omits `path`.

Code namespaces are:

- `geometer.transport.*` for framing, lifecycle, capability, and local ABI
  failures;
- `geometer.contract.*` for strict decoding and structural validation; and
- `geometer.operation.<operation-name>.*` for geometry execution outcomes.

Existing integer `Status.code` and C ABI return codes stay local adapter
statuses. They may map to a wire diagnostic but are never converted into a
stable wire code merely by formatting the integer.

## Conformance assertion lanes

Every governed vector declares exactly one primary assertion lane:

| Lane | Oracle |
| --- | --- |
| `strict_json` | Exact UTF-8 accept/reject behavior before model construction |
| `schema` | Accept/reject against the generated root JSON Schema |
| `semantic` | Structural decoded comparison after a declared projection |
| `diagnostic` | Exact category, code, path, and retryability |
| `canonical_serialization` | Exact RFC 8785 JCS bytes where a digest is required |
| `transport_framing` | Exact frame/header/attachment bytes or exact rejection |

Ordinary encoder output is not assumed byte-identical across languages.
Semantic vectors compare strings, enums, booleans, counts, and topology
structurally. Floating-point fields declare absolute and relative tolerances.
Negative zero is normalized only when the vector says so.

Every vector lists fields excluded from semantic comparison. Nondeterministic
timings such as `model_read_ms`, `bounds_ms`, and `elapsed_ms` are excluded by
that explicit projection, never by a global ignore rule.


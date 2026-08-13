# Model-Bounds Contract Compatibility

## Status and purpose

This is the required nonblocking difference report between the generated
canonical `geometry.model_bounds.options.a0` / `geometry.model_bounds.a0`
contracts and the current handwritten implementation. It records adapter work
before cutover; it does not promote the contracts or remove accepted inputs.

TypeSpec owns the candidate canonical structure. The generated C++ DTO/codec
and generic operation registry now use that strict structure. Existing parsing
behavior in `src/cpp/lib/model_bounds_options_json.cpp`, CLI layering, retained
per-operation C ABI functions, and the public Python wrapper remain active
until their individual migration gates pass.

## Option-input differences

| Input behavior | Current handwritten reader | Canonical generated contract | Required cutover behavior |
| --- | --- | --- | --- |
| Empty byte string | Accepts as an empty patch | Rejects before JSON construction | Keep only in the compatibility adapter |
| JSON `null` root | Accepts as an empty patch | Rejects; root is a closed object | Keep only in the compatibility adapter |
| Unknown fields | Currently ignored | Rejected | Adapter must not carry unknown fields into the canonical DTO |
| Duplicate keys | Not explicitly rejected before RapidJSON member lookup | Rejected by the strict parser | Reject before alias normalization |
| `model_format` | Accepted alias for `format` | Rejected unknown field | Normalize in the compatibility adapter |
| `modelTransform` | Accepted alias for `model_transform` | Rejected unknown field | Normalize in the compatibility adapter |
| Uppercase `STEP` | Accepted as `step` | Rejected enum value | Normalize in the compatibility adapter |
| Nested 4-by-4 transform | Accepted | Rejected; canonical form is a flat row-major array of 16 numbers | Flatten in the compatibility adapter |
| Absent option field | Leaves the inherited value unchanged | Preserved as absent | Generated patch DTO must retain presence |
| Explicit `"format":"step"` | Replaces the inherited value | Preserved as present even though it equals the default | Patch encoder must emit the present field |
| Non-affine final transform row | Rejected by operation validation | Structurally admitted as 16 numbers | Map the operation diagnostic after structural decoding; do not imply schema-only validity is executable validity |

Batch option behavior remains defaults, then top-level patch, then job patch.
Generated decoding must not materialize TypeSpec default intent before those
layers merge.

## Result differences and comparison

The generated result preserves the current emitted nesting and field names:
`schema`, `units`, `source`, `bounds`, and `timings`. The current writer always
emits both timing fields. They remain required wire fields but are explicitly
excluded from deterministic semantic comparison because their values are
nondeterministic.

JSON object member order and floating-point spelling from the current writer
are not canonical-byte authority. Ordinary cross-language result checks compare
the decoded structure with declared numeric tolerances and the explicit timing
projection. Exact bytes are reserved for separately identified canonical
serialization vectors.

The existing focused public C++ `ModelBoundsResult` and public Python
`ModelBoundsResult` convenience surface are not replaced by this structural
candidate. The generic registry maps the generated C++ request DTO into the
focused C++ value API and maps the focused result back to the generated wire
DTO. A later Python slice must use the same boundary while proving existing
call signatures and attributes.

## Evidence and remaining gate

The raw pilot vectors under `tests/contracts/vectors/` freeze strict parsing,
closed-schema behavior, absent-versus-present option fields, and success/failure
operation outcomes. Native C++ tests prove generated decoding, compatibility
separation, local-versus-typed C ABI failures, catalog discovery, attachment
ownership, and a live STEP `model_bounds` round trip. Later pilot work must add
complete generated C++ vector replay, TypeScript/Rust/Python replay,
non-affine-transform operation vectors, tolerant result vectors, and live
WASM/IPC round trips before promotion.

# Model-Bounds Contract Compatibility

## Status and purpose

This is the required difference report and promotion evidence index for the generated
canonical `geometry.model_bounds.options.a0` / `geometry.model_bounds.a0`
contracts and the retained compatibility implementation. Independent review
accepted the complete vertical, and the promotion manifest now records
TypeSpec plus the normalized catalog as its structural authority.

The structural authority is authored TypeSpec lowered through the normalized
catalog. Generated C++ DTOs/codecs and the generic operation registry use that
strict structure. Generated TypeScript, Rust, and Python projections consume
the same catalog. Existing parsing in
`src/cpp/lib/model_bounds_options_json.cpp`, CLI layering, retained
per-operation C ABI functions, and public Python convenience types remain as
explicit compatibility adapters; promotion does not remove those inputs or
surfaces.

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
contract. The generic registry maps the generated C++ request DTO into the
focused C++ value API and maps the focused result back to the generated wire
DTO. The Python package validates its canonical request/result boundary with
generated codecs while preserving the established call signatures,
attributes, executable discovery, aliases, and legacy mapping inputs.

## Promotion evidence and remaining gate

The raw pilot vectors under `tests/contracts/vectors/` freeze strict parsing,
closed-schema behavior, absent-versus-present option fields, and success/failure
operation outcomes. C++, TypeScript, Rust, and Python each replay all 20
structural manifest entries, including raw invalid UTF-8 and presence
projections. Two additional manifest-declared operation vectors now govern a
real SOT-23 result and a non-affine transform rejection. The success vector
excludes only the two timing leaves, compares strings and topology exactly, and
compares geometry with an absolute `1e-9` and relative `1e-12` tolerance. The
same expected projection is replayed through the native generic C ABI, direct
browser WASM client, persistent executable IPC Rust client, and compatible
Python boundary. The source hash is not excluded: each runtime independently
recomputes its exact FNV-1a value from the raw attachment bytes. This keeps the
oracle correct even when a text STEP fixture has platform-specific checkout
newlines. The diagnostic vector matches code, category, path presence, and
retryability exactly through the native C ABI, WASM, and executable IPC; only
human message prose is excluded.

Native C++ tests additionally prove compatibility separation,
local-versus-typed C ABI failures, catalog discovery, attachment ownership,
operation failures, and a live STEP round trip. Browser Worker tests remain a
separate correlation/lifecycle proof. Rust tests execute repeated and
concurrent correlated calls over one persistent native child, while Python
clean-wheel tests execute the compatible public boundary.

Exact remediated candidate revision
`e035bcd348cf0d8aa3db1812d907c19a36690ea0` passed the Windows x64, Linux x64,
Linux ARM64, and macOS ARM64 native/Rust/Python/wheel jobs in hosted workflow
run `31741067434`. Its standards job passed every check except the intentional
active-plan hygiene sentinel. The promotion manifest digest-locks the catalog,
vector manifest, and full-browser JavaScript/WASM artifacts used by this
evidence.

Independent review rejected the first promotion packet because it lacked these
operation-level vectors. The remediated corpus now has local and exact-commit
hosted evidence. Focused re-review accepted it in packet
`reviewer-019ffcd8-e5bb-7531-934f-b9e50e73d2b2ab7`; the manifest labels the
diagnostic, model-bounds request/result, operation outcome, and operation as
promoted. Compatibility readers and focused geometry APIs remain supported.

The generic result boundary also validates response-side JSON and attachment
limits even though `model_bounds` currently emits no output attachment. This is
deliberate preparation for later packed-result operations, not a change to the
model-bounds result shape.

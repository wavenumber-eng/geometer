# Geometer Contract Conformance Vectors

`manifest.json` is the language-neutral index for governed raw fixtures. Raw
files are authoritative for strict parsing, so duplicate keys and trailing data
must be tested before parsing into an ordinary map.

Every vector declares exactly one primary lane, an exact/structural/toleranced
comparison policy, excluded nondeterministic fields, and numeric tolerance.
The codec corpus covers:

- `strict_json`: UTF-8 decoding, duplicate-key rejection, and trailing-data
  rejection. A `.hex` case with `strict_parser_hex` represents exact invalid
  raw bytes without committing an editor-hostile non-UTF-8 source file;
- `schema`: Draft 2020-12 validation against the generated closed-object
  schemas; and
- `semantic`: presence projections that keep absent option-patch fields
  distinct from explicitly supplied default-valued fields.

The operation corpus adds:

- `operation_semantic`: a real STEP attachment executed through the native C
  ABI, browser WASM, executable IPC, and compatible Python boundary. All
  deterministic fields share one expected projection; only timing leaves are
  excluded, every numeric geometry value declares absolute and relative
  tolerance, and the source hash is recomputed exactly from the raw attachment
  bytes so platform checkout newline policy cannot masquerade as geometry
  drift; and
- `diagnostic`: a structurally valid but non-affine transform whose governed
  code, category, path presence, and retryability match exactly through the
  native C ABI, browser WASM, and executable IPC. Human message prose remains
  nonbinding.

Compatibility aliases such as `model_format` and `modelTransform` are rejected
by canonical schemas. Existing handwritten readers may continue to accept them
only at an explicit compatibility boundary. Canonical serialization and
transport-framing vectors remain separately governed when a promoted surface
requires exact bytes; they do not change the meaning of these frozen cases.

The [`analytic/`](analytic/) subdirectory is that separately governed binary
corpus for analytic planar Boolean A0. Its native-produced lowercase-hex files
pin canonical request/result bytes and standalone job digests independently of
the logical JSON-vector manifest in this directory.

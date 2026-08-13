# Geometer Contract Conformance Vectors

`manifest.json` is the language-neutral index for governed raw fixtures. Raw
files are authoritative for strict parsing, so duplicate keys and trailing data
must be tested before parsing into an ordinary map.

Every vector declares exactly one primary lane, an exact/structural/toleranced
comparison policy, excluded nondeterministic fields, and numeric tolerance.
The initial pilot covers:

- `strict_json`: UTF-8 decoding, duplicate-key rejection, and trailing-data
  rejection;
- `schema`: Draft 2020-12 validation against the generated closed-object
  schemas; and
- `semantic`: presence projections that keep absent option-patch fields
  distinct from explicitly supplied default-valued fields.

Compatibility aliases such as `model_format` and `modelTransform` are rejected
by canonical schemas. Existing handwritten readers may continue to accept them
only at an explicit compatibility boundary. Later projection/runtime slices
replay this same manifest and add diagnostic, canonical-serialization,
transport-framing, and operation-semantic lanes without changing the meaning of
these frozen cases.

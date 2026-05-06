# Third-Party Dependencies

This directory holds small vendored dependencies that should be available from a
fresh Geometer clone.

Generated or built dependencies stay in `.deps/` and are not committed.

## RapidJSON

`rapidjson/` is a vendored copy of RapidJSON v1.1.0, used by OCCT's GLB export
support. It includes the upstream license and one local compatibility patch
documented in `docs/adr/004_vendored_rapidjson_for_occt_gltf.md`.

# Third-Party Dependencies

This directory holds small vendored dependencies that should be available from a
fresh Geometer clone.

Generated or built dependencies stay in `.deps/` and are not committed.

## RapidJSON

`rapidjson/` is a vendored copy of RapidJSON v1.1.0, used by OCCT's GLB export
support. It includes the upstream license and one local compatibility patch
documented in `docs/geometer/adr/geometer-adr-004-vendored_rapidjson_for_occt_gltf.md`.

## Clipper2

`clipper2/` is a vendored copy of the Clipper2 2.0.1 C++ library, used for
generic planar polygon boolean and offset operations. It includes the upstream
Boost Software License 1.0 text and a Geometer vendoring note.

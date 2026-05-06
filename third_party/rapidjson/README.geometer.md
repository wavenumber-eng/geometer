# Geometer RapidJSON Notes

This is RapidJSON v1.1.0, vendored for OCCT's `RWGltf_CafWriter` GLB export
path.

Geometer carries one local compatibility patch:

- `include/rapidjson/document.h`
- `GenericStringRef& operator=(const GenericStringRef& rhs)` is deleted instead
  of assigning to const members.

The native and WASM build scripts verify that this patch is present before
configuring OCCT.

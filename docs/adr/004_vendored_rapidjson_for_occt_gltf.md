# ADR-004: Vendor RapidJSON for OCCT glTF export

## Status

Accepted

## Context

OCCT's `RWGltf_CafWriter` requires RapidJSON when `USE_RAPIDJSON=ON`. The
previous build flow cloned RapidJSON v1.1.0 into `.deps/rapidjson-src` and then
patched one header after clone so OCCT would compile with modern Clang and
Emscripten.

That worked, but it left a generated third-party git checkout in `.deps/`. The
local patch branch could not be pushed to our Geometer remote, and fresh-clone
behavior was less obvious than it needed to be.

## Decision

Geometer vendors RapidJSON v1.1.0 under `third_party/rapidjson`.

The vendored copy includes:

- `include/rapidjson/`
- `license.txt`
- upstream `readme.md`

It intentionally includes Geometer's one compatibility patch in
`include/rapidjson/document.h`: `GenericStringRef::operator=` is deleted. That
matches the type's const member semantics and avoids modern Clang/Emscripten
compile errors.

The native and WASM build scripts verify that the vendored header exists and
contains the compatibility patch, then pass `third_party/rapidjson` to OCCT via
`3RDPARTY_RAPIDJSON_DIR`.

## Consequences

- A fresh clone no longer needs a RapidJSON network clone.
- `.deps/` no longer contains RapidJSON state.
- RapidJSON updates are explicit source updates under `third_party/rapidjson`.
- The RapidJSON license stays checked in with the vendored source.

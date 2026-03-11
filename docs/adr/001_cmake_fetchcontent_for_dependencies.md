# ADR-001: Use CMake FetchContent for dependencies

## Status

Accepted

## Context

Geometer depends on OCCT, a large C++ library with its own CMake build. We need a dependency strategy that works across PC/Mac/Linux and eventually Emscripten (WASM).

Options considered:
- Git submodules
- Zephyr West manifest
- vcpkg / Conan
- CMake FetchContent

## Decision

Use CMake FetchContent with `GIT_SHALLOW TRUE` and a shared `FETCHCONTENT_BASE_DIR` configured via CMakePresets.json.

## Rationale

- OCCT is a CMake project. FetchContent integrates natively with no extra tooling.
- No additional tools to install (unlike West, vcpkg, or Conan).
- `FETCHCONTENT_BASE_DIR` prevents re-cloning across build directory wipes and across multiple build configurations (desktop, WASM).
- `GIT_SHALLOW` keeps the download to ~80MB instead of 500MB+.
- The same CMakeLists.txt works for desktop and Emscripten builds via toolchain file swap.

## Consequences

- First configure requires network access to clone OCCT.
- OCCT version is pinned by `GIT_TAG` in the top-level CMakeLists.txt.

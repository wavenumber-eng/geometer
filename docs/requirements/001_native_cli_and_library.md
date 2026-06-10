# REQ-001: Native CLI And Library

## Summary

Geometer provides a native C++17 library and a command-line executable for
generic CAD geometry operations built on OCCT.

## Requirements

1. Build on Windows, Linux/WSL2, and macOS with CMake, Ninja, Python 3, and a
   C++17 toolchain.
2. Restore or build OCCT as local generated dependency state under `.deps/`, not
   as committed source.
3. Keep application policy out of core geometry APIs. Geometer must not embed
   Altium, KiCad, PCB placement, viewer styling, or downstream cache policy.
4. Copy native runtime artifacts only under `dist/native/<platform>/`.
5. Do not produce or require root-level `dist/geometer(.exe)` artifacts.
6. Return exit code `0` on CLI success and nonzero on failure with a useful
   diagnostic.
7. Report release version and C ABI generation from native, C ABI, WASM, and
   Python surfaces.
8. Treat remote OCCT binary archives as optional generated dependency cache
   inputs. Validate them before use and retain source builds as the fallback.

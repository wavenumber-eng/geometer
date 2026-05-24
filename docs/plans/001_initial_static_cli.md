# Plan 001: Initial static CLI

Current status: historical plan, completed. The current CLI supports
STEP-to-GLB, STEP-to-HLR JSON/SVG, JSON batch requests, planar batch
diagnostics, and version reporting. Current native source-checkout artifacts are
moving to `dist/native/<platform>/` with flat `dist/` compatibility aliases.

## Goal

Get a working `geometer step-to-glb` command that statically links OCCT and
produces correct GLB output.

## Steps

1. Scaffold repo structure, CMakeLists.txt, FetchContent for OCCT.
2. Implement minimal `step_to_glb()` in libgeometer.
3. Implement CLI argument parsing in `main.cpp`.
4. Build on Windows first, validate against known STEP test files.
5. Verify GLB output loads correctly in a three.js viewer or glTF validator.
6. Set up rack test strata for C++ build verification and CLI integration
   tests.
7. Cross-build on macOS and Linux.

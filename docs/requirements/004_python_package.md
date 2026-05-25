# REQ-004: Python Package

## Summary

The PyPI distribution `wn-geometer` exposes the import package `geometer` and
uses a bundled platform executable backend.

## Requirements

1. Build a platform wheel for every released native platform.
2. Include the platform executable under `geometer/native/<platform>/` inside
   the wheel.
3. Resolve the executable from `GEOMETER_EXE`, the installed package, source
   checkout `dist/native/<platform>/`, or `PATH`.
4. Expose a `geometer` console script that forwards to the bundled platform
   executable.
5. Do not require users to load OCCT shared libraries directly from Python.
6. Provide public Python APIs for version, STEP HLR projection, projection JSON,
   STEP-to-GLB bytes, and batch execution.
7. Validate each wheel by installing it into a clean temporary environment and
   running a no-GUI package example.
8. Downstream users must be able to install from PyPI without local path
   overrides.

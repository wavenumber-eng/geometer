+++
type = "requirement"
id = "geometer-req-004"
domain = "geometer"
status = "implemented"
title = "Python Package"
created = "2026-08-18"

[[verification_refs]]
kind = "local_file"
target = "tests/python/STRATUM.toml"
+++

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
5. Publish macOS arm64 wheels with a deployment target that supports current
   hosted Apple Silicon CI runners. The default release target is macOS 11.0.
6. Do not require users to load OCCT shared libraries directly from Python.
7. Provide public Python APIs for version, STEP HLR projection, projection JSON,
   STEP-to-GLB bytes, and batch execution.
8. Export the governed HLR option/result types and typed persistent-client
   helpers for model and indexed-mesh HLR without removing the executable-backed
   compatibility wrappers.
9. Validate each wheel by installing it into a clean temporary environment and
   running a no-GUI package example.
10. Downstream users must be able to install from PyPI without local path
   overrides.

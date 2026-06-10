# REQ-006: Release Quality Gates

## Summary

A tagged Geometer release must pass deterministic native, Python, package, and
hygiene checks before publication.

## Requirements

1. Run `git diff --check`.
2. Run Ruff on Python source, examples, tests, and scripts.
3. Run Pyright on the public Python package, scripts, and Python tests.
4. Run `uv lock --check`.
5. Run clang-format checking on C++ source, examples, and tests.
6. Run the L99 code-hygiene test stratum before tagging.
7. Run native validation for the release platform.
8. Run Python package validation against an installed wheel.
9. For Linux releases, run validation from WSL2 or Linux and check that the
   executable does not dynamically depend on OCCT `libTK*` libraries.
10. After PyPI publication, install from PyPI in WSL2 and validate downstream
   use through Altium Cruncher or the headless package example.
11. When OCCT is restored from the binary dependency cache, validate the cache
    manifest and archive SHA-256 before using the restored install tree.

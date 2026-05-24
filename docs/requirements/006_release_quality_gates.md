# REQ-006: Release Quality Gates

## Summary

A tagged Geometer release must pass deterministic native, Python, package, and
hygiene checks before publication.

## Requirements

1. Run `git diff --check`.
2. Run Ruff on Python source, examples, tests, and scripts.
3. Run clang-format checking on C++ source, examples, and tests.
4. Run the L99 code-hygiene test stratum before tagging.
5. Run native validation for the release platform.
6. Run Python package validation against an installed wheel.
7. For Linux releases, run validation from WSL2 or Linux and check that the
   executable does not dynamically depend on OCCT `libTK*` libraries.
8. After PyPI publication, install from PyPI in WSL2 and validate downstream
   use through Altium Cruncher or the headless package example.

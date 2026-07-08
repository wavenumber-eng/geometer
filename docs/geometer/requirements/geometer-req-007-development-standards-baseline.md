+++
type = "requirement"
id = "geometer-req-007"
domain = "geometer"
status = "draft"
title = "REQ-007: Development Standards Baseline"
created = "2026-07-07"
+++

# REQ-007: Development Standards Baseline

## Summary

Geometer follows the Wavenumber mixed-mode project standard for Python, C++,
and WASM work. Because Geometer predates the standard, strict Python typing and
expanded Ruff rule families are tracked as a standards ratchet rather than a
single mechanical cleanup in this change.

## Requirements

1. The repository must declare `tool.wn_dev_std.profile = "python-native-wasm"`.
2. Root hygiene files must include `.gitattributes`, `.clang-format`,
   `.clang-tidy`, `LICENSE`, `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, and
   `SECURITY.md`.
3. CMake presets must use Ninja and set `CMAKE_EXPORT_COMPILE_COMMANDS=ON`.
4. Release signoff must run Ruff, Pyright, clang-format, Lizard complexity
   checking, code hygiene, and `uv lock --check`.
5. Wheel builds must write to `out/wheelhouse/` or another disposable output
   directory, not root `dist/`, because Geometer's `dist/` stores committed
   runtime artifacts.
6. The current Python typing gate uses Pyright basic mode. Strict mode is the
   target and should be enabled after public Python API and script annotations
   are ratcheted clean.
7. The current Ruff gate preserves the existing `E` and `F` baseline with
   existing long-line cleanup deferred through `E501`. Import, upgrade,
   bugbear, simplify, formatting, and line-length ratchets should be enabled in
   focused follow-up changes.

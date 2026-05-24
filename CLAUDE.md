# Geometer Claude Notes

Use [AGENTS.md](AGENTS.md) for repository working rules,
[DEVELOPMENT.md](DEVELOPMENT.md) for setup/build/test commands, and
[INTERFACES.md](INTERFACES.md) for the current C++, C ABI, Python, WASM, and
CLI surface.

## Commit Messages

- State what changed and why.
- Keep messages factual.
- Do not attribute commits to Claude or any AI tool.

## Current Shape

- CMake builds the native C++ library, CLI, optional shared C ABI target, and
  examples.
- OCCT is built as a standalone dependency under `.deps/` by
  `scripts/build_occt.py`.
- WASM builds use `scripts/build_wasm.py` and write grouped artifacts under
  `dist/wasm/<target>/`.
- Native source-checkout artifacts live under `dist/native/<platform>/`, with
  flat `dist/` compatibility aliases.
- The PyPI distribution is `wn-geometer`; Python imports `geometer` and uses the
  executable-backed CLI path.

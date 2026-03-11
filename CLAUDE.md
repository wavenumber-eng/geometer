# Geometer

Focused C++ geometry library and CLI built on OpenCASCADE Technology (OCCT).

## Commit messages

- To the point. State what changed and why.
- No emojis. No filler. No "production ready" or similar marketing language.
- Do not attribute commits to Claude or any AI tool in the message body.
- Factual only. If you are unsure about a claim, do not make it.

## Build system

- CMake is the build system. OCCT is pulled via FetchContent.
- Target: static linkage for a single distributable binary.
- CMakePresets.json defines the default configure preset including FETCHCONTENT_BASE_DIR.

## Project structure

- `src/cpp/lib/` — libgeometer, the reusable C++ core.
- `src/cpp/cli/` — geometer CLI executable (thin wrapper over libgeometer).
- `src/js/` — JavaScript/WASM code (viewers, browser tooling).
- `tests/` — rack-based test system with stratified test layers.
- `docs/adr/` — architecture decision records, numbered.
- `docs/requirements/` — requirements, numbered.
- `docs/plans/` — work plans, numbered.
- `scripts/` — Python tooling managed by uv.

## C++ guidelines

- C++17. Use `.clang-format` (Allman braces, 4-space indent, 100 col limit).
- Write readable code. If a construct is "write only" — hard to read back later — do not use it.
- No Boost. No heavy template metaprogramming. No SFINAE puzzles. No expression templates.
- OCCT is the one large dependency. Everything else should be small or written in-tree.
- For simple utilities (string helpers, file I/O wrappers, etc.), write our own. Keep them plain.
- Prefer concrete types over deep inheritance hierarchies.
- Prefer functions over classes when there is no state to manage.
- Use `std::string`, `std::vector`, `std::optional`, `std::variant` — the straightforward parts of the standard library. Avoid abusing `std::enable_if`, `std::invoke`, or type-trait gymnastics.
- Raw loops are fine. Not everything needs an algorithm or range pipeline.
- Error handling: return codes or simple error structs from library functions. Exceptions only at boundaries (e.g., CLI top-level catch). No custom exception hierarchies.
- No unnecessary comments or docstrings on self-evident code.

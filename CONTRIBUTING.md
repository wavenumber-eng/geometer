# Contributing

Geometer uses the Wavenumber mixed-mode project standard for Python, C++, and
WASM work.

Before opening a release-facing change:

```bash
uv sync --group dev
uv run pytest tests/L99_release -q
python scripts/validate_native.py
python scripts/validate_python_package.py
```

Native and WASM outputs that are part of the release contract live under
`dist/native/<platform>/` and `dist/wasm/<target>/`. Local build state such as
`.deps/`, `build/`, `build-wasm/`, `out/`, `.venv/`, and Rack results must not
be committed.

Use CMake presets with Ninja for native builds. Format owned C++ with the
committed `.clang-format` file and keep Python code passing Ruff and Pyright.

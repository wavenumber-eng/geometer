# Geometer

Focused C++ geometry library and CLI built on OCCT. Provides a small, controllable interface for operations like STEP-to-GLB conversion, HLR projection, and polygon extrusion/meshing.

## Build

```bash
cmake --preset default
cmake --build build --config Release
```

On first configure, CMake automatically builds OCCT from source (~10-15 min one-time cost). Subsequent configures are instant.

To manually rebuild OCCT (e.g., after version bump or to clean):

```bash
python scripts/build_occt.py --clean
python scripts/build_occt.py
```

## Usage

```bash
geometer step-to-glb input.step output.glb
geometer step-to-glb input.step output.glb --deflection 0.05 --angular 0.3
```

## Dependencies

- [OpenCASCADE Technology](https://dev.opencascade.org/) (V7_8_1) — built from source automatically.
- [RapidJSON](https://github.com/Tencent/rapidjson) (v1.1.0, header-only) — required by OCCT for glTF export.
- Python 3 — needed by `scripts/build_occt.py` (invoked by CMake on first configure).

## Project structure

- `src/cpp/lib/` — libgeometer, the reusable C++ core.
- `src/cpp/cli/` — geometer CLI executable.
- `src/js/` — JavaScript/WASM code.
- `tests/` — rack-based stratified test system.
- `docs/adr/` — architecture decision records.
- `docs/requirements/` — requirements.
- `docs/plans/` — work plans.
- `scripts/` — build and tooling scripts (uv-managed Python).

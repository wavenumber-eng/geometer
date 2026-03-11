# Geometer

Focused C++ geometry library and CLI built on OCCT. Provides a small, controllable interface for operations like STEP-to-GLB conversion, HLR projection, and polygon extrusion/meshing.

## Build

```bash
# 1. Build OCCT (one-time, ~10-15 min)
python scripts/build_occt.py

# 2. Build geometer
cmake --preset default
cmake --build build --config Release
```

## Usage

```bash
geometer step-to-glb input.step output.glb
```

## Dependencies

- [OpenCASCADE Technology](https://dev.opencascade.org/) — built from source via `scripts/build_occt.py`.

# Geometer

Focused C++ geometry library and CLI built on OCCT. Provides a small, controllable interface for operations like STEP-to-GLB conversion, HLR projection, and polygon extrusion/meshing.

## Build

```bash
cmake --preset default
cmake --build build --config Release
```

## Usage

```bash
geometer step-to-glb input.step output.glb
```

## Dependencies

- [OpenCASCADE Technology](https://dev.opencascade.org/) — pulled automatically via CMake FetchContent.

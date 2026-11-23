# Geometer Examples

This folder contains user-facing examples that exercise Geometer outside the
test harness.

| Environment | Examples |
| --- | --- |
| Browser | [Model bounds](wasm/model_bounds_demo.html), [HLR Lab](wasm/embedded_model_viewer.html), [Illustration Lab](wasm/illustration_demo.html), [Planar ring solver](wasm/planar_ring_solver_demo.html) |
| Python | [STEP HLR and SVG](python/step_hlr_svg.py), [Drawing geometry to Canvas](python/illustration_geometry_canvas.py), [Analytic model illustration](python/model_illustration_analytic.py), [PyVista HLR viewer](python/pyvista_hlr_viewer.py) |
| Native C++ | [HLR preview source](cpp/hlr_preview.cpp) and [build guide](cpp/README.md) |
| Node | [Model-bounds IPC quick start](node/ipc-model-bounds.mjs), [Experimental STEP topology](node/step_topology_annotation_reference.ts) |
| Rust | [Native API Lab source](rust/native_viewer/src/main.rs) and [build guide](rust/native_viewer/README.md) |

Test-only browser validation and benchmark harnesses live under `tests/wasm/`.
The maintained build, packaging, panel, and publication boundaries are in
[`docs/developer/browser-demos.md`](../docs/developer/browser-demos.md).

The WASM examples include a retained
[production-package Illustration Lab](wasm/README.md#step-illustration-lab).
It is available for design evaluation and regression testing, but is not a
production renderer, supported public API, or commitment that Geometer will own
the eventual illustration architecture.

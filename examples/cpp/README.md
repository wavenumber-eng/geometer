# C++ Examples

## Geometer HLR Preview

`geometer_hlr_preview` is a native Dear ImGui + SDL3 + OpenGL3 example. It
loads a STEP file, builds a GLB preview through Geometer, draws a lightweight 3D
preview pane, projects HLR from the current camera, and draws the projection in
an adjacent pane. It also computes model bounds through Geometer and can overlay
the projected bounds rectangle in the HLR pane. The UI shows the Geometer version
string and C ABI generation at the top of the window. The app requests a
high-pixel-density SDL window and scales Dear ImGui's font/style from
`SDL_GetWindowDisplayScale()` at startup.

Build from the repository root:

```sh
cmake -S . -B build -DGEOMETER_BUILD_EXAMPLES=ON
cmake --build build --target geometer_hlr_preview --config Release
```

When using the MSVC toolchain, run those commands from a Visual Studio
Developer PowerShell/Command Prompt so SDL's C configure step can find the
Windows SDK tools.

Run:

```sh
./build/examples/cpp/geometer_hlr_preview tests/fixtures/step/embedded_models/SOT-23.STEP
```

On Windows, run:

```powershell
.\build\examples\cpp\geometer_hlr_preview.exe tests\fixtures\step\embedded_models\SOT-23.STEP
```

macOS builds request an OpenGL 3.2 forward-compatible context and use GLSL
`#version 150`, matching Apple's supported core-profile OpenGL path. Other
desktop builds use OpenGL 3.0 and GLSL `#version 130`.

The example dependencies are optional and pinned in `CMakeLists.txt`:

- Dear ImGui `v1.92.8`, commit `8936b58fe26e8c3da834b8f60b06511d537b4c63`
- SDL `release-3.2.30`, commit `f5e5f6588921eed3d7d048ce43d9eb1ff0da0ffc`

Both are downloaded by CMake from exact source archives with SHA-256 checks.

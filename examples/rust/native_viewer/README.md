# Native Rust API Lab

Optional development demo of the maintained `geometer-client`, with an
eframe/egui + wgpu depth-buffered 3D viewport. It calls a separate native
`geometer(.exe)` process; it does not link OCCT/Geometer or embed JS/WASM.
All operation values/codecs come from Geometer's TypeSpec-generated contracts.
The independent C++ demo continues to demonstrate direct linking.

## Build and launch

From the repository root, with Rust 1.95.0 and a native linker:

```powershell
cargo build --manifest-path examples/rust/native_viewer/Cargo.toml --locked
examples/rust/native_viewer/target/debug/geometer-native-viewer.exe --geometer out/docs-cleanup/native-build/src/cpp/cli/geometer.exe --step tests/fixtures/step/embedded_models/SOT-23.STEP
```

The executable shown above is a local feature build, not a packaged dependency.
Use an executable built from the same revision/catalog. The previous 2026.9.4
release is incompatible with these additive operations even though development
builds have not yet changed the release version. Negotiation rejects mismatches;
the demo never downloads an executable or silently switches renderers.

`--geometer PATH` overrides maintained-client discovery: `GEOMETER_EXECUTABLE`,
an executable beside the app, then the working directory's
`dist/native/<platform>/geometer(.exe)`. If unresolved, browse and Connect in
the controls panel. The panel shows the selected path, release and catalog;
Windows child processes are launched without a console window.

Open STEP, orbit with left drag, pan with right/middle drag and zoom with the
wheel. Fit is explicit; orbit does not change zoom or automatically refit.
Drag the controls/result dividers to resize; dock controls left or right.
Controls scroll vertically in short windows. The 3D diagnostic preview is
**opaque**: it retains material color but intentionally does not render source
transparency. The native illustration and original SVG preserve source opacity.

## Native outputs and job behavior

- Colored mesh: `geometry.model_tessellation.a0`, millimeters, stripped root
  placement, generated mesh-collection attachment.
- Illustration: `geometry.mesh_illustration.a0`, original A0 SVG result, five
  shading choices, material/fusion/crease/background controls.
- Independent linework: `geometry.model_hlr_projection.a0`, Fast detail and
  fast mesh shadow, same view/root-placement convention. Controls also offer
  the existing poly/exact and mesh-shadow/HLR-close selectors; the default
  remains Fast detail plus fast mesh shadow. Separate shadow/detail tabs
  display the returned polylines; they do not claim combined illustration/HLR
  SVG support. That native composition API remains separate planned work.
- Export original SVG, generated illustration result/style JSON, HLR geometry
  JSON, or colored mesh-collection JSON. These are completed snapshots, not
  screenshots. Stale illustration/HLR exports are disabled until recomputation
  succeeds. The mesh export is the current loaded model, independent of view.

File reading, native requests, conversion, SVG rasterization and file writing
run off the UI thread. The panel shows phases and elapsed time, not a fabricated
percentage. One computation runs at a time; view changes replace the pending
view after a short debounce. All messages carry a process epoch, and solved
views carry a revision. Stop invalidates old messages and terminates the owned
process; reconnect to resume. Native operation cancellation itself is queue-only.
Closing the window terminates its owned process; it does not affect other agents'
Geometer processes. GPU uploads remain a UI/device step and can briefly stall
on large models; the transport limits are not a peak-memory sandbox.

SVG/HLR display rasterization is capped at 1600 pixels and external SVG images
are disabled. SVG exports retain original vectors. GPU targets are capped at
4096 pixels/device limits. STEP input is bounded at 256 MiB before native limits
and validation; larger/unsupported results report explicit errors.

## Focused validation

```powershell
cargo fmt --manifest-path examples/rust/native_viewer/Cargo.toml -- --check
cargo clippy --manifest-path examples/rust/native_viewer/Cargo.toml --all-targets --locked -- -D warnings
cargo test --manifest-path examples/rust/native_viewer/Cargo.toml --locked
examples/rust/native_viewer/target/debug/geometer-native-viewer.exe --geometer PATH_TO_FEATURE_EXE --step tests/fixtures/step/embedded_models/SOT-23.STEP --smoke-screenshot out/viewer.png
```

The screenshot parent directory must exist. `--smoke` opens a real window,
executes STEP/illustration/Fast HLR and exits; failure/timeout returns nonzero.
`--smoke-screenshot PNG` additionally reads back this app's rendered window.
The completion marker establishes native outputs and GPU submission; visual
inspection of the captured image is separate evidence, not exhaustive depth
or interaction certification. Rack entry RUST_002 is opt-in through
`GEOMETER_TEST_NATIVE_VIEWER=1`; GPU checks also require
`GEOMETER_TEST_NATIVE_VIEWER_GPU=1` and `GEOMETER_EXECUTABLE`.

## macOS arm64 handoff

This crate uses wgpu's native Metal path on macOS; no platform-specific renderer
or JS assets need to be generated. On an actual Apple Silicon Mac, install Rust
1.95.0 and Xcode command-line tools, build the matching native Geometer using
the [cached native build guide](../../../docs/developer/README.md), then:

```bash
cargo build --manifest-path examples/rust/native_viewer/Cargo.toml --release --locked
examples/rust/native_viewer/target/release/geometer-native-viewer \
  --geometer "$PWD/dist/native/macos-arm64/geometer" \
  --step tests/fixtures/step/embedded_models/SOT-23.STEP --smoke
```

Repeat without `--smoke` for Retina sizing, trackpad orbit/pan/zoom, dialogs,
docking, resize, rapid recompute, Stop/reconnect and close-during-load checks.
Record OS/GPU, Git revision, both executable hashes, command output and app
screenshots. A Windows build does not qualify macOS: Mac build and interactive
acceptance remain **not run**. The GUI's minimum macOS version is not yet
qualified; do not infer it from the Geometer wheel's deployment floor.

The [demo audit](../../../docs/developer/demo-status.md) records evidence and
remaining human checks. Release packaging, signing, bundled kernel distribution
and GitHub release inclusion are deferred; this crate is `publish = false`.

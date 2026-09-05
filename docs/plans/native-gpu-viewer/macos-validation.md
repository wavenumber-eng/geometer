+++
type = "plan_log"
id = "geometer-native-gpu-viewer-macos-handoff"
plan_id = "geometer-native-gpu-viewer"
step_id = "macos-agent-validation"
created = "2026-09-05"
+++

# Mac Agent Validation Handoff

Prepared handoff template, not evidence of a build or completed test. The user
authorized Windows-first development and a separate agent's Mac testing. Assign
this bounded task to an agent with an actual Mac after artifacts are available;
another Windows agent cannot certify Metal execution. Do not dispatch builds or
modify other repositories merely to draft this plan.

## Inputs The Implementer Must Supply

- Exact feature-complete commit, macOS arm64 build artifact and SHA-256 manifest.
- OS/deployment minimum, architecture, SDL/ImGui pins and Metal shader formats.
- Reproducible CMake preset/commands, required build tools and dependency-cache
  instructions; packaged shader/helper assets, licenses and launcher paths.
- If illustration uses a helper, its pinned runtime/module versions and startup
  protocol, with no user-global Node/browser installation assumption.
- Matching Windows results and SOT-23, one larger existing STEP fixture and the
  colored-material illustration fixture, with exact paths and source hashes.
- Geometry JSON, linework SVG, illustrated SVG and style JSON reference outputs.

## Agent Task And Evidence

Build on macOS arm64, or verify the supplied artifact's exact identity before
running it. Report compilation success independently from interactive success.
Launch from outside the checkout and verify asset resolution, Metal device
selection, no source-tree dependency and understandable startup failures.

Check Retina scaling, left/right docking, narrow controls, restored/reset
layout, minimize/restore, resize and native file/save dialogs. Exercise trackpad
and mouse orbit/pan/zoom, Fit and named views. Confirm rotation does not change
world-space camera extent or reset on a style/algorithm change.

Inspect body/pin occlusion from the previously failing angle and another view.
Load a larger STEP while interacting with controls; verify phase/elapsed/busy
indicators remain live. Rapidly change camera/model and confirm stale results
cannot replace newer work. Exercise failure/retry and closing during a job
according to the documented cancellation/shutdown limits.

Select fast detail/fast shadow plus legacy modes. Compare matching geometry and
SVG exports with Windows/web references, including units, orientation, layers,
colors and style. Open SVG in an independent viewer. Record geometry equality
separately from GPU raster appearance; do not require identical Metal/D3D pixels.

Return OS/hardware/GPU, commit/artifact hashes, commands, logs, screenshots and
per-check pass/fail/not-run results. Do not repair unrelated code or mark missing
hardware/tests as passes. Send reproducible failures back to the implementer;
test fixes at a new explicitly identified commit. No public release or signing
authorization is implied by this validation handoff.

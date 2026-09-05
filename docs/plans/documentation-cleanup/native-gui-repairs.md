+++
type = "plan_log"
id = "geometer-documentation-cleanup-native-gui-repairs"
plan_id = "geometer-documentation-cleanup"
step_id = "native-gui-repairs"
created = "2026-09-05"
+++

# Native GUI Repair Follow-up

After inspecting both native GUIs, the user reported black body triangles
occluding nearer gray pins at some C++ preview angles and requested the existing
fast detail/shadow algorithms in both demos. This explicitly extends the
documentation-only work to these demo repairs, not solver changes or pruning.

## Changes

- Replaced C++ average-depth triangle sorting with an opaque orthographic
  per-pixel depth buffer. Its OpenGL texture is cached across unchanged frames;
  model, camera, lighting and viewport changes invalidate it. Raster size is
  bounded to a 2048-pixel longest dimension. This is not transparency support.
- Added independent detail (`fast`, `poly`, `exact`) and outline
  (`fast-mesh-shadow`, `mesh-shadow`, `hlr-close`) selectors in both GUIs.
  Fast modes are the demo defaults; existing kernel defaults remain unchanged.
- Updated demo instructions, audit status and test strategy. Added a small
  dependency-free CTest regression, registered through the existing Rack
  native CTest boundary, without adding GUI startup to automated unit tests.

## Verification

Built only the C++ demo and its new regression target in an isolated CMake/Ninja
tree, reusing cached pinned OCCT/Boost/SDL/ImGui sources. This compiled unchanged
kernel objects required by the demo but did not rebuild OCCT or publish a new
kernel executable/WASM artifact. The updated Windows demo executable accompanies
the source changes; other distributables remain untouched.

Depth regression passed in 0.01 seconds: overlapping sloped faces, order,
winding, clipping and degeneracy. An initial test accidentally asserted a unique
winner at exact depth ties; the fixture was corrected to avoid that ambiguity.

The Python Qt control smoke used the existing GUI environment and this worktree's
Python source/executable. Fast/fast-mesh-shadow, poly/mesh-shadow and exact/hlr-close
all returned nonempty detail and outline geometry on SOT-23; returning to fast
produced 129 detail edges and 57 outline edges. The Qt offscreen platform could
not initialize VTK OpenGL on this host; the normal desktop platform with a shown
window passed cleanly. A Qt widget screenshot verifies controls/HLR only, not
the embedded native OpenGL surface. Both updated GUIs were launched for the user.

Ruff, clang-format, documentation freshness and whitespace checks passed.
Independent agent `/root/independent_plan_review` approved the scoped source
changes without blocking findings, checking depth direction, cache invalidation,
GL texture lifetime/state restoration, selector wiring and Rack registration.
That was a read-only code review, not interactive visual acceptance.

The user should recheck the previously failing C++ angle before the demo runtime
audit and overall cleanup plan are closed. No public push or release is implied.

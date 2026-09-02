# Fast Raster HLR Research Baseline

## Result

The HOOPS-style direction is viable for interactive Geometer viewers. The
retained Illustration Lab now has a clean-room `GPU HLR` experiment that does
no OCCT projection work while the camera moves:

1. STEP is tessellated to GLB once.
2. Boundary and crease candidates are extracted once into retained line
   buffers.
3. Opaque faces fill the normal hardware depth buffer.
4. Candidate edge fragments render afterward with depth testing enabled.
5. A configurable polygon offset prevents a face from rejecting its own edge.

This is the same broad raster visibility architecture as HOOPS
`FastHiddenLine`, not a reconstruction of HOOPS source or its complete internal
pass set. Public HOOPS documentation describes FastHiddenLine as an O(n),
multi-pass z-buffer algorithm intended for interactive display, while its more
expensive HiddenLine mode is intended for accurate hardcopy output:

- <https://staging.docs.techsoft3d.com/hoops/visualize-desktop/api_ref/cpp/class_h_p_s_1_1_subwindow.html>
- <https://docs.techsoft3d.com/hoops/visualize-3df/prog_guide/3dgs/06_1_rendering_hidden_surfaces.html>

Geometer should retain the same product split: raster HLR for interaction and
the existing OCCT HLR path for deterministic projected curves and SVG export.

## Prototype boundary

`examples/wasm/fast_hlr.ts` is a browser-renderer experiment, not a public
Geometer API. It clones only the Three.js scene graph and shares the source GLB
geometry. It owns the generated edge buffers and two simple materials.

The current candidate builder uses Three.js `EdgesGeometry`. It joins endpoints
by quantized position within each mesh, which is useful because Geometer's GLB
can retain face-local duplicate vertices. It retains boundaries and adjoining
faces whose normal angle exceeds the configured threshold. The public behavior
is documented at <https://threejs.org/docs/pages/EdgesGeometry.html>.

The current line primitive is intentionally minimal. WebGL renders
`LineBasicMaterial` at one pixel regardless of its requested width, so a
production renderer needs screen-space line quads for stable width, joins, and
dashes. See <https://threejs.org/docs/pages/LineBasicMaterial.html>.

## Measured fixture baseline

The hosted Chrome gate switches into GPU HLR, waits for retained frames, checks
that the framebuffer is nonblank, and verifies that the projection generation
does not change. The following values were observed on 2026-09-02 in headless
Chrome with the gate's `--disable-gpu` configuration:

| Fixture | Triangles | Candidate edges | One-time edge build | Mean CPU submit | Mean frame interval |
|---|---:|---:|---:|---:|---:|
| SOT-23 | 444 | 353 | 2.6-3.3 ms | 1.32-1.43 ms | 15.7 ms (about 64 fps) |
| BGA90 | 24,150 | 7,250 | 16.7-23.7 ms | 1.92-1.95 ms | 16.7 ms (about 60 fps) |

These are feasibility numbers, not a graphics benchmark. The frame interval is
refresh-scheduled, CPU submit time is not a GPU timestamp, and the headless
configuration may use a software WebGL implementation. The useful result is
that camera motion performs no STEP read, tessellation, OCCT HLR, projected
triangle sorting, or SVG generation, and the 24k-triangle fixture sustains the
test cadence.

An opt-in hardware run on the same machine identified ANGLE/D3D11 on an AMD
Radeon RX 7600 XT. Asynchronous disjoint-timer queries measured 0.14 ms GPU for
SOT-23 and 0.27 ms GPU for BGA90. CPU submission was 0.97 ms and 3.24 ms,
respectively. Headless requestAnimationFrame cadence stayed near 30 fps while
the page rendered both its source and HLR canvases, so that cadence is not a
GPU saturation result. The GPU timings and low submission cost are the more
useful evidence.

Reproduce the diagnostic report with:

```powershell
$env:GEOMETER_GPU_HLR_REPORT = '1'
$env:GEOMETER_GPU_HLR_HARDWARE = '1'
uv run pytest tests/wasm/test_illustration_static_site.py -q -s
```

The BGA fixture still produces 574 draw calls because the source scene retains
many mesh nodes. Edge and surface batching is therefore the first scaling
experiment for larger assemblies.

## Known gaps

- Smooth-surface silhouettes are not generated dynamically. A cylinder can
  lose its tangent outline when adjacent facet angles stay below the crease
  threshold.
- Position hashing is a demo heuristic. It can confuse coincident surfaces and
  does not carry CAD edge, face, body, or occurrence provenance.
- Polygon offset is a coarse bias. Near-coplanar bodies, clipping planes, and
  very different depth ranges need a screen/depth-derived policy.
- Transparent and alpha-masked faces have no governed occlusion policy.
- Lines are one pixel wide and have no screen-space joins or dash distance.
- Timer queries are optional browser extensions, so the gate accepts platforms
  that can report only CPU submission and display cadence.

## Next engineering evidence

The next useful slice is a topology-aware retained edge graph built while OCCT
face/edge relationships are available. It should carry tessellated CAD edge
polylines, adjacent face identities/normals, and occurrence provenance without
changing the face-local render mesh. That enables view-dependent silhouettes
and avoids global positional welding.

After that, batch surface and edge draws by compatible state, replace native
lines with screen-space quads, and add image regressions for cylinders,
coincident instances, near-plane clipping, mirrored instances, and
transparency. The existing optional GPU timer queries should be exercised over
a hardware/browser matrix and a large repeated-assembly corpus before setting
a production frame-time gate.

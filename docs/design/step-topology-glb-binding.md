# STEP Topology GLB Work-Packet Research

Status: selected experimental encoding; available through unpromoted native executable operations

Date: 2026-08-22

## Outcome

Geometer now emits a deterministic GLB 2.0 work packet from the sealed native
topology render artifact. A real pinned Three.js 0.161 `GLTFLoader` and
`Raycaster` test proves that every rendered primitive across the checked corpus
and a reflected case resolves to the exact native occurrence, body, and face
handles.

The native executable's experimental render operation returns the GLB as a
bounded attachment and retains only the authoritative sealed render artifact,
not a second copy of the GLB. The retained artifact, including its outer
identity wrapper, is memory-accounted,
session/generation-bound, replaced by the next render, and destroyed on close,
eviction, expiry, mutation, or process replacement. Resolve-hit validates the
browser's returned indices and target claims against that retained artifact.
The portable C ABI and browser/WASM runtime do not advertise these operations.

The retained artifact is trusted immutable store state. Resolve-hit therefore
uses direct instance, mesh, and primitive indexing and performs one bounded
lookup plus exact target-claim checks; it does not rehash or rescan the whole
artifact for every click. The public C++ value API keeps its full validation
path for caller-supplied mutable artifacts.

The selected A0 experiment uses:

- one shared glTF mesh for each simple-shape definition;
- one glTF node for each leaf occurrence, with its accumulated transform;
- one indexed glTF primitive per selectable face;
- `asset.extras.wn_geometer` for source, session, generation, artifact, content
  digest, coordinate, unit, and layout identity;
- mesh extras for the definition handle;
- node extras for occurrence and instance identity; and
- primitive extras for body/face handles and the exact triangle span.

The asset repeats the opaque artifact handle and complete content SHA-256 from
the native render artifact. The containing work packet additionally carries an
exact GLB-byte SHA-256 and a session-secret `gtg_` seal over that digest and the
sealed render identity. This avoids an impossible self-referential digest
inside the GLB JSON. Consumers validate the downloaded bytes against the
out-of-band work-packet digest before accepting a hit, then return both GLB
identities, indices, and claimed target handles to the native validator.
Handles remain live-session and generation-scoped; GLB metadata is not durable
annotation identity.

## Emission-Route Comparison

The OCCT 8.0.1 writer exposes face merge control and protected virtual
`writePrimArray` and `writeExtrasAttributes` hooks. The experiments and source
surface review produced these results:

| Route | What works | Blocking limitation for exact selection |
| --- | --- | --- |
| XCAF `TDataStd_NamedData` through `writeExtrasAttributes` | OCCT projects supported scalar and array values into node `extras` | The output is flat and node-only. Definition and occurrence attributes can collide, and it cannot express a nested, sealed asset/mesh/primitive binding table. |
| Focused `RWGltf_CafWriter` subclass | The protected hooks can change primitive and extras emission | `writePrimArray` receives an internal glTF-face object, while node/occurrence traversal context is owned by the writer. Joining both safely would couple Geometer to private traversal/order behavior or duplicate substantial writer logic. |
| Deterministic JSON-chunk postprocessing of an OCCT GLB | Nested asset and primitive extras can be inserted without rewriting binary buffers | The completed GLB does not carry enough authoritative OCCT face/occurrence identity to prove which primitive belongs to which live target. Inferring it by order would make ordering identity. Existing corpus output also duplicates mesh objects per occurrence even where accessors are shared. |
| Deterministic GLB encoding from Geometer's sealed OCCT tessellation | Exact occurrence/body/face provenance is already present, definition geometry remains shared, output is bounded, and metadata is placed at the level Three.js returns | Selected for the research work packet. It is intentionally a small geometry/binding encoder, not a replacement for OCCT's production material-aware STEP-to-GLB path. |

The direct route remains OCCT-backed: OCCT owns STEP transfer, XCAF structure,
tessellation, face orientation, normals, and occurrence locations. Geometer
only serializes the already-validated value artifact. No face association is
reconstructed from glTF names, array order, or geometric coincidence.

## Coordinates, Units, And Materials

Positions are written in meters, as required by glTF convention. Node matrices
contain the accumulated occurrence transform and the explicit signed-rigid
source-to-render conversion. The GLB default converts Geometer's OCCT frame
(+Y forward, +Z up) to glTF (+Y up, -Z forward). Matrices are emitted in glTF
column-major form, and translations are converted from millimeters to meters.

This research packet currently emits normals and one explicit neutral,
double-sided PBR material, but it does not preserve source materials, textures,
colors, or lights. Every face primitive references that material so the real
loader test covers glTF primitive/material handling. The asset declares
`single-neutral-research-material` so a caller cannot mistake it for the
existing presentation-oriented `step_to_glb` output. A later student sandbox
may replace the neutral presentation, but production material parity is
outside this topology proof.

## Face Layout And Size Evidence

One primitive per face gives the simplest exact Three.js result: each loaded
`Mesh` carries one primitive's extras, and `faceIndex` is local to that face.
Its cost is a draw call for every occurrence/face pair. A merged primitive
would reduce the projected draw calls to one per leaf occurrence, but requires
a separately validated triangle-range lookup. A compact binary table is the
preferred future layout if larger models make repeated JSON handles or draw
calls unacceptable.

The checked A2 fixture report records actual work-packet sizes and per-face
draw-call counts. `projected_merged_draw_calls` is a cardinality projection,
not a measured GPU timing:

| Case | Existing presentation GLB | Binding GLB | JSON | Binary | Per-face calls | Projected merged calls |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| miniature test point | 72,512 | 87,388 | 25,150 | 62,208 | 50 | 1 |
| SOT-23 | 41,048 | 44,984 | 26,427 | 18,528 | 53 | 1 |
| RESC1608X06L | 13,448 | 17,072 | 14,116 | 2,928 | 32 | 4 |
| SOIC-20-300 | 66,184 | 55,576 | 31,787 | 23,760 | 312 | 21 |
| ABM3B | 131,336 | 142,532 | 86,197 | 56,304 | 185 | 7 |
| generated repeated assembly | n/a | 6,504 | 5,753 | 720 | 24 | 4 |
| generated fused slab | n/a | 9,976 | 8,268 | 1,680 | 14 | 1 |
| generated flat multi-solid | n/a | 8,816 | 7,345 | 1,440 | 12 | 1 |

The existing and binding GLBs differ in tessellation, materials, and purpose,
so their byte counts are contextual rather than a codec benchmark. The SOIC
case already shows the key tradeoff: exact per-face selection uses 312 logical
draw calls versus a projected 21 for merged occurrence meshes. Slice A should
therefore keep GLB bindings compact and leave room for a later binary range
attachment without changing authored target identity.

## Validation And Failure Behavior

Native encoding is bounded independently for GLB bytes and inherits all render
vertex, index, primitive, instance, binding, instanced-triangle, and memory
limits. Byte/cancel checks run while copying vertices, normals, indices, JSON
records, final chunks, and SHA-256 chunks rather than only after a complete
allocation. Cancellation or any encoding failure clears the entire output
packet. POSITION bounds are computed from the exact finite FLOAT values stored
in the BIN chunk.

Store rendering also supplies an aggregate transient budget equal to the
remaining store allowance while the prior retained artifact stays live.
Render records are charged progressively, and GLB encoding receives only the
budget left after the candidate render plus a distinct 256 MiB wire ceiling.
The encoder uses one shared budget-enforcing allocator for BIN, layout tables,
and both RapidJSON stacks. Vector growth and RapidJSON reallocation charge the
simultaneously live old-plus-new allocations before calling the system
allocator. The destination GLB reserves once from empty only after its final
size and coexistence with all live source storage are preflighted. Regression
tests prove both boundaries: source growth is rejected before the denied
allocation, and a final GLB which fits the wire ceiling is still rejected when
source and destination cannot coexist under the transient ceiling. Failure
keeps the prior retained artifact intact.

The native structural test checks GLB framing, metadata, shared mesh/instance
counts, exact topology extras, coordinate conversion, cancellation, the GLB
byte ceiling, and the sealed return-path validator. It rejects modified BIN
bytes and mismatched primitive/target claims before resolving the live target.

The integration test runs every fixture in the checked corpus plus a reflected
coordinate variant. It validates buffer/accessor ranges, FLOAT accessor
bounds, indices, node matrices, millimeter-to-meter translation, explicit
material use, and exact GLB SHA-256 before loading with repository-pinned
Three.js. It raycasts the first triangle of every primitive instance and
compares the loaded occurrence, body, face, and reconstructed mesh-local native
triangle ordinal to an independently emitted native expectation. The emitter
also sends every such descriptor through the native validator. Stripped or
reordered extras, overlapping ranges, modified BIN bytes, detached digests,
and reflected-front-face mismatches fail closed.

```powershell
cmake --build build --config Release --target `
  geometer_step_topology_render_binding_test `
  geometer_step_topology_glb_fixture_emitter
ctest --test-dir build -C Release --output-on-failure -R `
  geometer_step_topology_render_binding_test
uv run pytest tests\python\test_topology_glb_raycast.py -q
```

This test proves a browser-compatible render hit and exact native target
agreement. It does not make GLB resumable authoring state, prove persistence in
STEP, or define the future annotation contract.

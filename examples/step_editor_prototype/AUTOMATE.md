# AUTOMATE — training a tensor system to condition STEP files for us

The end state we are building toward: drop ANY vendor STEP file in, get a
conditioned AP242 out — seated, oriented, pins found, numbered, netted,
hitboxed — with no human in the loop except review. This document explains
how a learned ("tensor") system gets us there, what it should and should not
be trusted with, and how the editor we already built is quietly generating
its training data.

## 1. The key insight: every session you run IS a training example

Each `Write AP242` embeds two things into the conditioned STEP:

- **Labels** — every B-rep face that belongs to a pin, that pin's
  designator, role (SMT/THR vs CON/HEAD), net grouping, function, and
  hitbox. This is exactly a per-face segmentation + per-pin attribute
  ground truth, authored by a human expert (you) against the datasheet.
- **The journal** — the declarative op sequence that produced those labels
  (`zsit`, `detect_pins`, `separate`, `hitboxes`, ...), replayable headlessly
  via `--apply`.

So the corpus is `(raw STEP, conditioned STEP)` pairs. No annotation tool
needs to be built — the editor *is* the annotation tool, and conditioning
parts you needed anyway keeps growing the dataset for free. The
`benchmarks/` folder is the seed.

## 2. What representation to feed the network

Do NOT feed raw point clouds or screenshots. CAD parts come to us as exact
B-rep, and the editor already computes the ideal graph form of it:

- **Nodes = B-rep faces**, with features we already extract per face:
  area, surface type (plane/cylinder/spline...), normal direction
  statistics, bbox extents, centroid height above the seat plane,
  triangle-count, color if present.
- **Edges = face adjacencies**, with the features our edge-flow detection
  already uses: dihedral angle at the shared edge (smooth vs sharp),
  shared-edge length, convex vs concave.

This is the UV-Net / BRepNet family of representations (graph neural
network over the face-adjacency graph, optionally with sampled UV-grid
patches per face for local shape). Our `face_smooth_adjacency`,
`mesh_face_areas`, `_edge_face_table` and friends produce these features
today — the dataset writer is mostly plumbing.

Why this beats "a massive amount of 3D files": generic 3D foundation models
learn what objects look like; our task is a precise per-face labeling on
engineering geometry with exact adjacency. A few THOUSAND labeled
connectors/chips beats a billion unlabeled meshes for this, because the
graph structure carries most of the signal (pins are repeated, small,
high-curvature subgraphs hanging off one big body — on every part family).

## 3. The tasks, ordered by how learnable they are

| Task | Output head | Difficulty | Fallback when unsure |
|---|---|---|---|
| Pin face segmentation | per-face: pin / body / shield-mount | easy | edge-flow detectors (today's) |
| Pin role | per-pin: SMT/THR vs CON/HEAD | easy | Z-band heuristic |
| Seating orientation | quaternion or 24-class axis pick | easy | Auto Z-Sit scorer (geometric, already shipped) |
| Pin-1 quadrant | 4-class | medium | polarization-feature heuristics + user click |
| Numbering order | per-pin ordinal (learned ordering) | medium | serpentine / grid solvers (today's) |
| Functions from datasheet | text task, not geometry | — | LLM with the datasheet PDF, not the tensor net |

Train one backbone with multiple heads; the segmentation head alone already
automates the part of the workflow that costs the most clicks.

Note which tasks are NOT for the geometry network: pin NAMES and FUNCTIONS
come from the datasheet, not the shape. That stage is an LLM-with-documents
problem (parse the pinout table, align to the predicted numbering), layered
on top.

## 4. Training pipeline, concretely

1. **Dataset writer** (a `--dataset` CLI on the editor): for every
   conditioned STEP in a folder, load the RAW input, build the face graph +
   features, read the embedded metadata, and emit one `.npz`/JSON graph
   sample with per-face labels. The journal's zsit matrix is the
   orientation label; `detect_pins`/`separate` results are the segmentation
   labels.
2. **Augmentation** — random rigid rotations (so seating must be inferred,
   not memorized), unit scaling, face-order shuffling, optional tessellation
   jitter. CAD graphs augment cheaply.
3. **Model** — 3–6 layer graph network (message passing over face
   adjacency), ~1M params is plenty to start. Heads per Section 3.
   Class imbalance (1195 housing faces vs 8-face pins on the MTSS connector)
   wants focal loss or pin-face upweighting.
4. **Metrics that matter** — per-PIN recall (a pin found with 7 of its 8
   faces is found; a missed pin is a failure), orientation top-1, and
   end-to-end: replay the predicted ops and diff the resulting metadata
   against the human session (`--apply` makes this a one-liner). The
   MTSSD03 benchmark journal is regression test #1.
5. **Scale expectation** — useful signal at ~100 labeled parts (families
   repeat), production confidence at low thousands. Until then the
   heuristics carry the load; the net only has to beat them family by
   family.

## 5. Inference: the net proposes, geometry disposes

The integration contract that keeps this safe:

- The network only ever emits the SAME declarative ops a human emits —
  a zsit matrix, pin face-sets, an ordering. They run through the exact
  journal/replay machinery, so a model-driven session is auditable and
  diffable like any other.
- Every deterministic guard stays in the loop downstream: seal volume
  conservation, cap-area and bounds guards, hitbox containment, metadata
  read-back. A wrong prediction degrades to "this pin did not separate /
  please review", never to corrupt geometry. The verifiers are cheap and
  exact; the net only needs to be right most of the time.
- Confidence gates: low-confidence faces fall back to the geometric
  detectors; conflicts surface in the editor as staged (orange/blue)
  proposals the human approves — the same stage-then-Apply flow every tool
  already has. Reviewed sessions feed back into the dataset (active
  learning for free).

## 6. The REF convention — how the corpus accumulates

A gold-standard sample is a hand-conditioned export saved as
`<part>_AP242_conditioned_REF.step` next to the raw input. Everything the
trainer needs is inside it (labels + journal); nothing else to maintain.

- Condition the part fully by hand (or fix up an Auto pass — corrections
  are the most valuable labels), Write AP242, rename with the `_REF`
  suffix so later sessions can't overwrite it.
- The journal is extracted into `benchmarks/<part>_REF.json` so the
  geometric solvers can be scored against it (`--apply` replays it; the
  auto chain's output diffs against its metadata).
- First entry: `benchmarks/mtssd03_connector_REF.json` — 26 ops, 67 nets,
  134 pins (67 SMT/THR + 67 CON/HEAD, all hitboxed, INHERIT propagated).
  Measured against it, today's coded `--auto` chain gets the seat to
  0.04 mm but misses the mouth pins entirely, keeps 4 false pins the
  human deleted, and guesses pin 1 a quarter-turn off — exactly the three
  judgment layers the learned stages are for.

## 7. The ladder (where we are)

1. ✅ Journaled manual tools — every session replayable (`--apply`).
2. ✅ Deterministic automation where geometry suffices — Auto Z-Sit
   (support-level scorer matches the hand-picked frame on MTSSD03 to
   0.00° / 0.08 mm), context-plane + seed detection, edge-flow separation,
   auto hitboxes, serpentine/grid numbering solvers.
3. ⬜ Dataset writer + ~100-part corpus (condition parts as usual; the
   corpus accumulates itself).
4. ⬜ Graph-net segmentation + orientation heads; ship behind a staged
   "Suggest" button, exactly like Auto Z-Sit.
5. ⬜ LLM datasheet reader for names/functions, aligned to the predicted
   numbering.
6. ⬜ Headless conditioner: `step_editor.py --auto input.step` emits a
   journal, replays it, validates, and flags anything below confidence for
   human review in the GUI.

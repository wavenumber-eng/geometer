# `model_3d_a0` — a 3D-model data model (draft)

A versioned data model for the metadata a conditioned 3D model (AP242 STEP)
carries, drafted in the **same style as `toolz/data_models`** (`dev` branch,
`data_models/src/py/data_models/pcb`). This is the **MODEL leg** of the PCB-entity
trio (footprint + schematic + model); it is the contract the STEP editor must
produce and that downstream tools read back.

> Status: **scaffold / draft.** The structure and goal are the point right now —
> how we actually derive pad/drill centres from the B-rep is deferred.

## Why this exists (two problems it fixes)

1. **No geometry linkage.** Today's embedded blob (`wn3d.step_conditioning.a0`)
   links a pin to geometry only by positional *index* (body N, face M) + centroid.
   Nothing ties it to the STEP file's own entity graph — a `CLOSED_SHELL` has no
   idea it is a pin, and a hitbox has no anchor in the B-rep. → `EntityRef`.
2. **Authoring data in the shipped file.** The operation journal (how the model
   was conditioned) is currently embedded in the final STEP. That is an authoring
   artifact, not part of the cleaned model. → the root `model_3d_a0` has **no
   journal field**; journals live in a separate schema / sidecar and are stripped
   from the final export.

## Conventions (inherited from `data_models`)

- **One class per file**, snake_case `model_3d_<noun>.py`; classes `Model3d…`.
  (The schema family is `model_3d` rather than `3d_model` because a Python module
  cannot begin with a digit; `model_3d_a0` keeps file prefix == schema family,
  matching `pcb_*` ↔ `pcb_a0`.)
- Plain `@dataclass`; `to_json()` omits empty fields; `from_json()` is defensive
  with defaults. Every object is self-describing via a `"type"` string.
- **Integer nanometres** for all lengths (`*_nm`), exact mm/mil/inch conversion;
  canonical frame is **X-right, Y-up, Z-out, seated on z = 0**.
- **Linking is by id**: an object holds another's `id` in a `*_ref` field. Geometry
  links reach the STEP entity graph through an `EntityRef`.
- `metadata: dict` on every object as a forward-compatible escape hatch.

## The linking model (the important part)

```
model_3d_a0 (root)
├── bodies[]      Model3dBody      geometry_ref → #MANIFOLD_SOLID_BREP / #CLOSED_SHELL
│                                  role=pin → pin_ref ─────────────┐
├── pins[]        Model3dPin   ◄───────────────────────────────────┘
│                                  geometry_ref → the pin's #CLOSED_SHELL / #faces
│                                  hitbox_ref ──► hitboxes[].id
│                                  zplane_feature_ref ──► zplane_features[].id
│                                  net_ref ──► nets[].id
├── hitboxes[]    Model3dHitbox    owner_ref → pin.id ; geometry_ref → #faces
├── zplane_features[]  Model3dZPlaneFeature  owner_ref → pin.id (drill hole / pad @ z=0)
└── nets[]        Model3dNet       pin_refs[] → pin.id  (designator = one node)
```

`EntityRef` carries **both** the literal STEP ids (`solid_id`, `shell_id`,
`face_ids` = `#NNN`) *and* an index fallback (`body_index`, `face_indices`, OCCT
map order) that survives a rigid re-export. The editor can populate the index
layer today; resolving/injecting the `#` ids is a later export step.

## Primitives (this draft)

| File | Class | `type` | Role |
|------|-------|--------|------|
| `coordinate_3d.py` | `Coordinate3D` | — | int-nm X/Y/Z point |
| `entity_ref.py` | `EntityRef` | — | link to STEP `#` entities (+ index fallback) |
| `obb.py` | `Obb` | — | oriented box (centre + half-extents + Z-rot) |
| `model_3d.py` | `Model3d` | `model_3d_a0` | root: frame, bodies, pins, nets, hitboxes, z-features |
| `model_3d_body.py` | `Model3dBody` | `model_3d_a0_body` | a solid/CLOSED_SHELL + its identity |
| `model_3d_pin.py` | `Model3dPin` | `model_3d_a0_{thr,smt,con,head}_pin` | a geometric contact (kind → type) |
| `model_3d_hitbox.py` | `Model3dHitbox` | `model_3d_a0_hitbox` | connection volume (box / cavity / cube / hull) |
| `model_3d_zplane_feature.py` | `Model3dZPlaneFeature` | `model_3d_a0_zplane_feature` | footprint @ z=0 (drill hole / pad) |
| `model_3d_net.py` | `Model3dNet` | `model_3d_a0_net` | pins sharing a designator = one node |

## Candidate primitives to add (open for discussion)

- **`model_3d_a0_frame`** — make the conditioning result explicit (origin, up
  axis, front direction, seat z, standoff) instead of leaving it implicit on the
  root. The Z-Sit + Redefine-Front output, as data.
- **`model_3d_a0_courtyard` / envelope** — the part's overall footprint outline
  (silhouette / bounding contour at z=0); the IPC courtyard analog.
- **`model_3d_a0_keepout`** — no-go volumes (under a connector mouth, beneath a
  shield) for mechanical/clearance checks.
- **`model_3d_a0_mating_axis`** (connectors) — insertion direction + board-parallel
  vs board-perpendicular mating, so CON/HEAD pins know where they connect.
- **`model_3d_a0_logo` / marking** — applied logos/text, for traceability.
- **`model_3d_a0_contact_path`** — the curved spine tail→mouth through a contact
  (the dotted-spine the Separate tool already draws), for true net topology.

## Open questions

1. **Pin variants**: one `Model3dPin` with a `kind` (current) vs four separate
   per-kind classes. Split only if a kind grows kind-specific fields.
2. **`EntityRef` strength**: do we commit to injecting real `#` ids (durable but
   needs the STEP-text injector), or ship index+centroid first and add `#` ids
   when the injector lands?
3. **Journal home**: separate `*_journal_a0` schema + sidecar, vs a build-only
   blob never embedded. (Either way it leaves the shipped model.)
4. Which of the candidate primitives above are in scope for `a0` vs a later `a1`.

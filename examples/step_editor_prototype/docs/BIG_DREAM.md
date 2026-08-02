# THE BIG DREAM — the PCB Entity

*Where the STEP conditioning editor is headed. Written 2026-06-12, after the
first hand-conditioned part (INA226) survived the full KiCad and Altium
round trips with its metadata intact.*

---

## The Big Three

Every electronic part, in every PCB construction program ever written, is
described by the same three things:

1. **FOOTPRINT** — where the copper is
2. **SCHEMATIC** — what the pins mean
3. **MODEL** — what the part physically is

Today these live in three separate files, in three separate frames, joined
by nothing stronger than a string: pin 3 on the symbol maps to pad 3 on the
footprint because someone typed "3" in two places. The model, when present
at all, is decorative — a shape for screenshots, carrying zero electrical
meaning. The triangle has one weak edge and two missing ones, and every
library bug ever soldered into a board at bring-up (mirrored footprints,
transposed pin numbers, symbol/footprint drift) lives in that gap.

## The model is the keystone

This prototype builds the MODEL leg — but the model leg is not just one
corner of the triangle. A **conditioned model** carries both bridge
vocabularies, grounded in geometry:

- **Model ↔ Footprint** — hitboxes + designators. The hitbox cross-section
  at the seating plane (Z=0) *is* the IPC-style land pattern. A pad is not
  a string; it is a volume in space you can intersect against.
- **Model ↔ Schematic** — designators + pin functions. The net groups with
  their function lists (VBUS, IN±, SCL/SDA, Alert…) *are* a symbol's pin
  table, already keyed and typed.

So the model leg **closes the triangle**, with two consequences:

1. **Derivability.** From one conditioned AP242 alone, the other two legs
   are generatable: slice the hitboxes at Z=0 → footprint candidate;
   emit the net/function table → symbol stub. A part enters the database
   as one conditioned model and the trio materializes from it.
2. **Auditability.** For existing footprints and symbols, the model is the
   referee. Does pad 3's copper actually fall inside pin 3's hitbox? Does
   the symbol's pin list match the model's net groups? Triangle consistency
   becomes a *checkable property* instead of a convention enforced by
   soldering irons.

## What a conditioned model is

The editor takes any STEP file (AP203/AP214/AP242, any vendor, any mess)
and normalizes it into the canonical frame:

- **Z-up**, seated at **Z = 0**
- **Pin 1 in the X+ Y+ quadrant** — every part in the database agrees on
  orientation, always
- Pins detected, numbered geometrically, named, functioned
- **Hitboxes**: per-pin contact volumes — male pins boxed, female pins
  boxed where the mating pin sits, BGA balls cubed, pads slabbed
- **Nets**: pins sharing a designator are the SAME electrical node
- Colors, watermark, and a declarative **operation journal** so the entire
  conditioning is replayable headlessly

All of it is embedded *inside* the AP242 as legal STEP entities
(`wn3d.step_conditioning.a0`) — readable by any STEP parser, queryable from
the 3D file alone, and proven to survive embedding in vendor containers
byte-for-byte.

## {Name}.pcbent — the PCB Entity file

The destination format. One file per part — `INA226.pcbent` — that bakes in
the big three and **is** the part:

```
INA226.pcbent  (container: zip-with-manifest, in the 3MF/glTF tradition)
├── manifest.json        pcbent_a0 schema tag, designator keying,
│                        per-leg provenance (derived-from-model |
│                        imported-and-audited | authored), journal
├── model/
│   └── INA226.step      conditioned AP242, canonical frame,
│                        wn3d.step_conditioning.a0 embedded
├── footprint/
│   └── footprint.json   pcb_a0-style contract: pads, copper, courtyard
│                        (nm integer coordinates, X-right/Y-up)
└── schematic/
    └── symbol.json      pin table: net groups + functions
```

**The validity rule that no other format has:** because the three legs
share one canonical frame and one designator/hitbox vocabulary, internal
consistency is enforceable at the file level. A `.pcbent` where pad 3 does
not intersect pin 3's hitbox is *invalid* — not merely wrong-looking.

## Projections — "transforms into exactly what every program needs"

The entity stays whole; tool-specific files become disposable
**projections** of it, each with a fidelity record saying exactly what the
target dialect could not carry:

| Projection | Machinery | Status |
|---|---|---|
| `.pcbent → .kicad_mod` | kicad_embed bake (zstd-15 + MMH3, byte-parity with KiCad's own writer) | **proven** |
| `.pcbent → .PcbLib` | altium_bake (zlib + native checksum, GUID/placement preserved) | **proven** |
| `.pcbent → AP242` | the model leg, verbatim | **proven** |
| `.pcbent → pcb_a0 / IPC-2581` | data_models converter suite | **working today** |
| `.pcbent → .kicad_sym / .SchLib` | sch_a0 leg | awaiting the schematic rethink |

This is why the discrepancies we see today (a conditioned model sitting 90°
off a legacy footprint's pads) are not bugs to patch — they are artifacts
of the three legs living in separate files with separate frames. Inside the
entity the discrepancy cannot exist by construction.

## Where it lives in the ecosystem

The other two legs are already under construction in `toolz/data_models`:

- `design_a0` is the umbrella — netlist + schematic + PCB payloads, keyed
  by designator exactly the way our net metadata is
- `pcb_a0` (footprint leg) is mature: Altium/KiCad/IPC-2581 converters work
- `sch_a0` (schematic leg) is paused for a deliberate semantic rethink
- `PcbEmbedded3DModel` has an extensible `metadata` dict — the docking port
  for `wn3d.step_conditioning.a0`
- Its conventions are the ones `pcbent_a0` should be drafted against:
  nanometer integer coordinates (1 mm = 1,000,000 nm exact), X-right/Y-up
  (ADR-0041), a0/a1 schema versioning (ADR-0037), and transformation
  fidelity/provenance tracking (ADR-0038)
- data_models' old tessellated-GLTF 3D goal was abandoned and the 3D
  contract redesign is pending — the conditioned AP242 is positioned to
  **be** the model leg, not to compete with one

This prototype is the **factory** that turns the world's messy vendor STEP
files into model legs: open anything (`.step`, `.kicad_mod`, `.PcbLib`),
condition by hand or fully automatically (`--auto`), export with the truth
embedded.

## What this unlocks (from DESIGN_INTENT.md, now within reach)

- Multiboard systems that track NETS across actual 3D space between boards
- Simulation of chips using real geometric pin outputs across stackups
- Full definition of a FOOTPRINT and SCHEMATIC within the 3D file metadata
- DuPont connectors on PCBs with actual utility — connections made by
  hitbox intersection, not by faith
- Complex board-to-board connections resolved geometrically
- Every 3D model useful and coherent — and every library bug of the
  "mirrored footprint" family caught by validation instead of bring-up

## Where we are, and the road ahead

**Done (all five phases of the Additive Design Intent, 2026-06-11):**

- The conditioning editor: Z-sit, pin-1 quadrant, detect pins, hitboxes,
  pin functions/nets, colors, separate-unibody, LOGO, metadata embedding,
  journal + headless replay, fully automatic conditioning
- Our own KiCad bake (byte-parity with KiCad's writer) and Altium bake
  (extract / split / replace, everything-else byte-untouched)
- The `[1]→[5]` round-trip identity chains as permanent selftests, and a
  110-case regression suite freezing every law above
- First real part conditioned end-to-end by hand and verified in KiCad 10
  and Altium Designer

**Ahead:**

1. **The first `.pcbent`, built by hand** — bundle INA226's three legs
   (model as-is, footprint *derived from hitboxes*, symbol table from net
   metadata) into a zip with a draft `pcbent_a0` manifest, and prove one
   projection (`→ .kicad_mod`) reproduces what the editor already exports.
   This surfaces every real design question before anything hardens.
2. **The audit demo** — overlay the derived land pattern against the
   actual pads in an existing `.kicad_mod`: triangle-consistency checking,
   live.
3. **`pcbent_a0` contract** — drafted against the data_models ADRs, with
   the capability matrix saying what each projection preserves.
4. **Docking** — `wn3d.step_conditioning.a0` flows into
   `PcbEmbedded3DModel.metadata`; the entity becomes a first-class citizen
   of `design_a0`; the schematic leg joins when its rethink lands.

---

*The big vision is consistency: every part, one entity, one canonical
frame, pin 1 always in X+ Y+ — and everything every PCB program needs,
projected from it.*

# Design intent — Separate Unibody (the manual split toolkit)

*Authored 2026-06-16 from the user's spoken design intent. This is the source
of truth for the Separate Unibody tool; the implementation must satisfy every
clause below. Linked from [DESIGN_INTENT.md](DESIGN_INTENT.md).*

## Why this tool matters most

Every other tool in the editor already lets a user **fully hand-condition** a
model. Unibody splitting is the one remaining hinder on whether a model can be
split apart at all. The goal is to **fully close the manual route**: any
unibody, no matter how awkward its geometry, must be separable by hand.

Once the manual route is closed, a user can (quite boringly) spend ~8 hours
hand-conditioning models to produce **reference files** — which either train a
machine-learning ranker or become ground-truth targets for the deterministic
algorithms. Manual completeness is therefore the whole point; elegance is
secondary to "you can always get there by hand."

## One tool, two sub-modes (flathead vs phillips)

Separate Unibody is a single tool with two interchangeable ways to apply it —
like a flathead and a phillips screwdriver: same tool, slightly different
application. **The two sub-modes are mutually exclusive in the UI.**

1. **Body-Face Cutoff** (the existing edge-flow growth): grow each pin from its
   seed faces by edge flow until the flow reaches the body; tune with the
   cutoff factor; paint/regrow. Best when pins are clean protrusions.
2. **Box Cut** (the universal manual fallback): define box-bounded boolean
   cuts numerically. Slices straight *through* faces, so pins whose junction
   with the body runs mid-face — the cases the edge-flow/seal path cannot
   handle — still separate cleanly.

**When Box Cut is active, the Body-Face Cutoff controls (cutoff factor,
Regrow, Auto, Paint Regions) are hidden** so the user can focus on the cut, and
vice versa.

## Box Cut sub-mode — exact behaviour

### Box-bounded cut (no cut plane)
A cut is just an **axis-aligned bounding box** — there is no cut plane:
- The box (X/Y/Z limits) carves out the chunk of geometry inside it as the pin.
  A boolean **Common** makes the pin, a complementary **Cut** removes that chunk
  from the body, so the two pieces conserve the original volume. The boolean
  uses a small outward pad + fuzzy tolerance so faces coplanar with the box are
  included (not dropped) and an embedded box still produces both pieces.
- The cut only affects geometry inside the box, so one box isolates a single
  local pin without disturbing the rest of the model.
- Several boxes can be banked (each carves its own pin) and applied together.

> History: an earlier version added an optional oblique cut plane (normal +
> location). It produced surprising angled cuts, so it was removed — a plain box
> is what users want.

### Inputs — text fields and vertex picks only (NO draggable widget)
The user must **not** drag anything in the 3D view; that disrupts the workflow.
Inputs are:
- **Text fields** typed directly: X / Y / Z limits (min/max).
- **⊙ Box from 2 points** — click two model vertices (opposite corners); their
  axis-aligned bounding box becomes the limits.
- **Per-axis ⊙ 2 pts** — click two vertices to set one axis's min/max.
- All picks **snap to the nearest vertex** and show a **preview dot** at the
  snap target (yellow for the box pick, X-red/Y-green/Z-blue for axis picks).
- Pressing **Box Cut** turns the model **solid** so the pin is easy to see.

### Live WYSIWYG preview
While defining cuts, the main 3D view **automatically previews the pastel
colours of every resulting separate body** (the same pastel split-preview the
tool already uses). The preview updates live as the fields change or faces are
clicked. Pressing **Apply Separate** then does **exactly** what the preview
shows — no surprises.

### Picking the box from the model (QOL)
Each axis (X, Y, Z) has a **vertex-snap pick button** next to its min/max
fields: click it, then click two model vertices — the clicks snap to the
nearest vertex and set that axis's min/max from their coordinates. While an
axis is being picked, that axis is drawn as a **bold, on-front coloured guide**
(X red / Y green / Z blue, same always-visible style as the 3D browser's
reference axes) so the user sees which axis they're setting.

### Detect-in-cut → find exact features (the reference-factory multiplier)
A **Find matching** button detects the feature inside the current cut box and
stages a matching box cut on **every identical feature** in the model
(rotation/mirror-invariant, via `find_similar_regions`). The seed plus all
matches preview in pastel; Apply carves them all. Define one pin's box →
condition a whole repeated row/array in two clicks.

### Mirror to matching geometries (the reference-factory multiplier)
For repeated structure, the user defines the cut for one pin, then — for more
nuanced control — **clicks a separated body and presses a special button that
mirrors that same separation onto every exact-same geometry elsewhere in the
model** (rotation/mirror-invariant match via `find_similar_regions`). Define one
pin → condition a whole repeated row/array in a couple of actions.

## Body-Face Cutoff sub-mode — manual selection power-tools

To close the manual route here too, face selection gains:
- **Propagation modes**: grow a selection across **tangent** (smooth-continuous)
  edges only, or across a **bend** (the next sharp edge) — not just the existing
  any-edge ring.
- **Select All** faces of a body into the active bundle.
- **Mirror / Select Matching**: paint one pin, then auto-stage every
  geometrically identical region as its own bundle.

## Invariants (must hold)
- Splitting only reorganises in-memory geometry — every action is **undoable**.
- Every applied separation is **journaled** declaratively and **replays
  headlessly** (so hand-made references reproduce exactly). Box cuts journal
  their X/Y/Z limits; replay re-cuts and re-links identically.
- Pins link to their carved body by identity/proximity, never by guessing; a
  pin that doesn't separate stays a valid face-region pin.
- Volume is conserved across a split (the pieces sum to the original).

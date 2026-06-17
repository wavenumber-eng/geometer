"""Build the model_3d_a0 data model from the editor's document + pins, with
each body/pin linked to its real STEP entities.

Two parts:
  • resolve_entity_refs(step_path, document) — parse a freshly-written STEP and
    map each body (in body-table order) to its #MANIFOLD_SOLID_BREP /
    #CLOSED_SHELL / [#ADVANCED_FACE...]. Position is the primary key, verified
    by a face-count fingerprint; if the writer ever reorders, it falls back to
    matching unused solids by fingerprint.
  • build_model_3d(...) — assemble the Model3d (bodies, pins, nets, hitboxes),
    package = the largest-volume body, with the CLOSED_SHELL ⇄ pin ⇄ hitbox
    links wired by id. No journal — that authoring artifact stays out.

See ../mock_data_model/README.md for the contract.
"""

from __future__ import annotations

import re
from pathlib import Path

import numpy as np

from mock_data_model import (
    Coordinate3D,
    EntityRef,
    Model3d,
    Model3dBody,
    Model3dHitbox,
    Model3dNet,
    Model3dPin,
    Obb,
)

from .document import shape_volume
from .metadata import GENERATOR

_NM = 1_000_000  # mm -> nm


def _mm_to_nm(value: float) -> int:
    return int(round(float(value) * _NM))


def _face_count(solid) -> int:
    from OCP.TopAbs import TopAbs_FACE
    from OCP.TopExp import TopExp
    from OCP.TopTools import TopTools_IndexedMapOfShape

    face_map = TopTools_IndexedMapOfShape()
    TopExp.MapShapes_s(solid, TopAbs_FACE, face_map)
    return face_map.Extent()


def _parse_solids_and_shells(text: str):
    """(solids in file order = [(solid#, shell#)], {shell# -> [face#...]})."""
    solids: list[tuple[int, int]] = []
    shells: dict[int, list[int]] = {}
    for entity in text.split(";"):
        msb = re.search(r"#(\d+)\s*=\s*MANIFOLD_SOLID_BREP\([^,]*,\s*#(\d+)", entity)
        if msb:
            solids.append((int(msb.group(1)), int(msb.group(2))))
            continue
        shell = re.search(r"#(\d+)\s*=\s*CLOSED_SHELL\(", entity)
        if shell:
            refs = [int(r) for r in re.findall(r"#(\d+)", entity)]
            shells[int(shell.group(1))] = refs[1:]  # drop the shell's own id
    return solids, shells


def resolve_entity_refs(step_path: Path, document) -> list[EntityRef]:
    """One EntityRef per body (body-table order), linking to its STEP entities."""
    text = Path(step_path).read_text(encoding="utf-8", errors="replace")
    solids, shells = _parse_solids_and_shells(text)
    order = [(sid, shid, len(shells.get(shid, []))) for sid, shid in solids]

    refs: list[EntityRef] = []
    used: set[int] = set()
    for index, body in enumerate(document.bodies):
        faces = _face_count(body.solid)
        pick = None
        if index < len(order) and index not in used and order[index][2] == faces:
            pick = index                                   # position match (verified)
        else:                                              # fingerprint fallback
            for j, (_s, _sh, f) in enumerate(order):
                if j not in used and f == faces:
                    pick = j
                    break
        if pick is None:
            refs.append(EntityRef(body_index=index))
            continue
        used.add(pick)
        solid_id, shell_id, _ = order[pick]
        refs.append(EntityRef(
            solid_id=f"#{solid_id}",
            shell_id=f"#{shell_id}",
            face_ids=[f"#{f}" for f in shells.get(shell_id, [])],
            body_index=index,
        ))
    return refs


def _hitbox_to_model(hitbox: dict, hitbox_id: str, owner_ref: str) -> Model3dHitbox:
    kind = str(hitbox.get("type", "") or "")
    if kind == "obb":
        center = hitbox.get("center", [0, 0, 0])
        half = hitbox.get("half_extents", [0, 0, 0])
        return Model3dHitbox(
            id=hitbox_id, owner_ref=owner_ref, kind="male_box",
            obb=Obb(
                center=Coordinate3D(_mm_to_nm(center[0]), _mm_to_nm(center[1]),
                                    _mm_to_nm(center[2])),
                half_x_nm=_mm_to_nm(half[0]), half_y_nm=_mm_to_nm(half[1]),
                half_z_nm=_mm_to_nm(half[2]),
                rotation_z_deg=float(hitbox.get("rotation_z_deg", 0.0) or 0.0)))
    if kind == "convex_hull":
        return Model3dHitbox(
            id=hitbox_id, owner_ref=owner_ref, kind="convex_hull",
            hull_points=[[_mm_to_nm(c) for c in v]
                         for v in hitbox.get("vertices", [])])
    return Model3dHitbox(id=hitbox_id, owner_ref=owner_ref, kind="male_box",
                         metadata={"raw": hitbox})


def _pin_kind(pin) -> str:
    # The editor distinguishes primary (PCB tail) vs mouth (mating contact); the
    # finer THR-vs-SMT / CON-vs-HEAD split needs z-plane drill/pad detection
    # (deferred), so map conservatively for now.
    return "CON" if getattr(pin, "role", "") == "mouth" else "SMT"


def build_model_3d(document, pins, refs: list[EntityRef], source_name=None) -> Model3d:
    """Assemble the model_3d_a0 object from the document + pin registry + the
    resolved per-body EntityRefs."""
    volumes = [abs(shape_volume(b.solid) or 0.0) for b in document.bodies]
    package_index = int(np.argmax(volumes)) if volumes else -1

    bodies = [
        Model3dBody(
            id=f"body_{i}",
            name=body.name,
            role="package" if i == package_index else "pin",
            color=list(body.color) if body.color else None,
            geometry_ref=refs[i] if i < len(refs) else None,
        )
        for i, body in enumerate(document.bodies)
    ]

    model_pins: list[Model3dPin] = []
    hitboxes: list[Model3dHitbox] = []
    nets: dict[str, Model3dNet] = {}
    registry_pins = list(pins.pins) if pins is not None else []
    for index, pin in enumerate(registry_pins):
        pid = f"pin_{index}"
        designator = pin.name or str(pin.number)
        net_id = f"net_{designator}"

        geometry_ref = None
        if pin.body_ids:
            body_index = pin.body_ids[0]
            if 0 <= body_index < len(refs):
                base = refs[body_index]
                geometry_ref = EntityRef(
                    solid_id=base.solid_id, shell_id=base.shell_id,
                    face_ids=list(base.face_ids), body_index=body_index)
                bodies[body_index].role = "pin"
                bodies[body_index].pin_ref = pid
        elif pin.face_ids:
            body_index = pin.face_ids[0][0]
            shell_id = refs[body_index].shell_id if body_index < len(refs) else ""
            geometry_ref = EntityRef(
                shell_id=shell_id, body_index=body_index,
                face_indices=[f for (b, f) in pin.face_ids if b == body_index])

        hitbox_ref = ""
        if pin.hitbox:
            hitbox_ref = f"hb_{index}"
            hitboxes.append(_hitbox_to_model(pin.hitbox, hitbox_ref, pid))

        model_pins.append(Model3dPin(
            id=pid, number=pin.number, designator=designator, kind=_pin_kind(pin),
            function=pin.function or "", net_ref=net_id, net_name=designator,
            centroid=Coordinate3D(*[_mm_to_nm(c) for c in pin.centroid]),
            geometry_ref=geometry_ref, hitbox_ref=hitbox_ref))

        net = nets.setdefault(designator, Model3dNet(id=net_id, designator=designator))
        net.pin_refs.append(pid)
        if (pin.function and pin.function != "INHERIT"
                and pin.function not in net.functions):
            net.functions.append(pin.function)

    bounds = document.bounds()
    return Model3d(
        id=Path(document.path).stem,
        units="mm",
        source_file=source_name or Path(document.path).name,
        generator=GENERATOR,
        bbox_min=[_mm_to_nm(bounds[0]), _mm_to_nm(bounds[2]), _mm_to_nm(bounds[4])],
        bbox_max=[_mm_to_nm(bounds[1]), _mm_to_nm(bounds[3]), _mm_to_nm(bounds[5])],
        bodies=bodies,
        pins=model_pins,
        nets=list(nets.values()),
        hitboxes=hitboxes,
    )

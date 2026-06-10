"""Headless milestone selftests — no Qt, no interactive window. Each test
exercises the pure document/tool layer and exits non-zero on failure so this
can gate CI later. Usage: step_editor.py --selftest m0 [--fixture <path>]"""

from __future__ import annotations

import tempfile
from pathlib import Path

import geometer

ROOT = Path(__file__).resolve().parents[3]
FIXTURES = ROOT / "tests" / "fixtures" / "step" / "embedded_models"


def _check(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def selftest_m0(fixture: Path | None = None) -> None:
    """Load → tessellate → face maps consistent → export AP242 → re-read."""
    from .document import EditorDocument
    from .export_ap242 import export_ap242

    path = fixture or FIXTURES / "ct-sot-23-5.stp"
    document = EditorDocument.load(path)
    print(f"loaded {path.name}: {len(document.bodies)} bodies")
    _check(len(document.bodies) >= 1, "no bodies loaded")

    for index, body in enumerate(document.bodies):
        mesh = body.mesh
        _check(mesh is not None, f"body {index} has no mesh")
        _check(len(mesh.tris) > 0, f"body {index} has no triangles")
        _check(
            len(mesh.tri_face_ids) == len(mesh.tris),
            f"body {index}: face-id array length mismatch",
        )
        _check(mesh.face_count > 0, f"body {index} has no faces")
        _check(
            int(mesh.tri_face_ids.max()) <= mesh.face_count,
            f"body {index}: face id exceeds face count",
        )
        _check(
            int(mesh.tris.max()) < len(mesh.points),
            f"body {index}: triangle index out of range",
        )
    total_tris = sum(len(b.mesh.tris) for b in document.bodies)
    total_faces = sum(b.mesh.face_count for b in document.bodies)
    print(f"meshed: {total_faces} faces, {total_tris} triangles")

    if fixture is None:
        # Known fixture: ct-sot-23-5 is a unibody part.
        _check(len(document.bodies) == 1, "ct-sot-23-5 should be a single body")

    with tempfile.TemporaryDirectory(prefix="step_editor_m0_") as temp:
        out_path = Path(temp) / f"{path.stem}_AP242_conditioned.step"
        report = export_ap242(document, out_path)
        print(report.summary())
        _check(report.ok, "export validation failed")
        head = out_path.read_text(encoding="utf-8", errors="replace")[:4096]
        _check("10303-242" in head or "AP242" in head.upper(), "output is not AP242")
        bounds = geometer.model_bounds(out_path)
        print(f"geometer reads exported file: size={bounds.bounds.get('size')}")


def selftest_m1(fixture: Path | None = None) -> None:
    """Scramble a part with a known transform, Z-sit it back from picked
    points, and assert it sits exactly on Z=0 with its original height."""
    import numpy as np

    from .document import EditorDocument
    from .export_ap242 import export_ap242
    from .tools.zsit import compute_zsit_matrix

    path = fixture or FIXTURES / "sot223.stp"
    document = EditorDocument.load(path)
    xmin, xmax, ymin, ymax, zmin, zmax = document.bounds()
    original_height = zmax - zmin
    print(f"original z=[{zmin:.4f}, {zmax:.4f}] height={original_height:.4f}")

    # Known scramble: rotate 37 deg about X, 21 deg about Y, translate.
    rx, ry = np.radians(37.0), np.radians(21.0)
    rot_x = np.array(
        [[1, 0, 0], [0, np.cos(rx), -np.sin(rx)], [0, np.sin(rx), np.cos(rx)]]
    )
    rot_y = np.array(
        [[np.cos(ry), 0, np.sin(ry)], [0, 1, 0], [-np.sin(ry), 0, np.cos(ry)]]
    )
    scramble = np.eye(4)
    scramble[:3, :3] = rot_y @ rot_x
    scramble[:3, 3] = (3.0, -4.0, 5.0)
    document.apply_trsf(scramble)
    sb = document.bounds()
    print(f"scrambled z=[{sb[4]:.4f}, {sb[5]:.4f}]")
    _check(abs(sb[4]) > 1.0e-3 or abs((sb[5] - sb[4]) - original_height) > 1.0e-3,
           "scramble did not change the pose")

    # The user's picks: three points on the original seating plane (z=zmin)
    # and one above it, all carried through the scramble.
    def scrambled(p):
        v = scramble @ np.array([p[0], p[1], p[2], 1.0])
        return tuple(v[:3])

    picks = [
        scrambled((xmin + 0.2, ymin + 0.2, zmin)),
        scrambled((xmax - 0.2, ymin + 0.3, zmin)),
        scrambled((xmin + 0.4, ymax - 0.2, zmin)),
        scrambled(((xmin + xmax) / 2, (ymin + ymax) / 2, zmax)),
    ]
    matrix = compute_zsit_matrix(*picks, origin_rule="centroid")
    document.apply_trsf(matrix)
    nb = document.bounds()
    print(f"re-sat    z=[{nb[4]:.4f}, {nb[5]:.4f}]")
    _check(abs(nb[4]) < 1.0e-4, f"z-min should be 0, got {nb[4]}")
    _check(
        abs((nb[5] - nb[4]) - original_height) < 1.0e-4,
        "height changed — transform is not rigid",
    )

    with tempfile.TemporaryDirectory(prefix="step_editor_m1_") as temp:
        out_path = Path(temp) / "resat.step"
        report = export_ap242(document, out_path)
        _check(report.ok, "export validation failed")
        bounds = geometer.model_bounds(out_path)
        exported_zmin = float(bounds.bounds["min"][2])
        print(f"geometer exported z-min = {exported_zmin:.6f}")
        # geometer's bounds are tessellation-based, so allow mesh slack here;
        # the exact assertion is the OCC Bnd_Box check above.
        _check(abs(exported_zmin) < 1.0e-2, "exported file does not sit on Z=0")


def selftest_m2(fixture: Path | None = None) -> None:
    """Pin-1 swing math: every quadrant lands in +X+Y in both modes; a real
    document rotation is rigid (bounds-diagonal preserved)."""
    import math

    import numpy as np

    from .document import EditorDocument
    from .tools.pin1_quadrant import compute_pin1_angle, rotation_z_matrix

    samples = [(4.2, 3.1), (-2.0, 5.0), (-3.3, -0.7), (1.5, -6.0)]
    for mode in ("quarter", "exact-bisector"):
        for x, y in samples:
            angle = compute_pin1_angle(x, y, mode=mode)
            rotated = rotation_z_matrix(angle) @ np.array([x, y, 0.0, 1.0])
            _check(
                rotated[0] >= -1.0e-9 and rotated[1] >= -1.0e-9,
                f"{mode}: ({x},{y}) -> ({rotated[0]:.3f},{rotated[1]:.3f}) not in +X+Y",
            )
            if mode == "quarter":
                _check(
                    abs(math.degrees(angle) % 90.0) < 1.0e-9,
                    f"quarter mode produced non-90-degree angle {math.degrees(angle)}",
                )
    print("swing math OK for all quadrants in both modes")

    path = fixture or FIXTURES / "SOIC-20-300.STEP"
    document = EditorDocument.load(path)
    before = document.bounds()
    diag_before = np.linalg.norm(
        [before[1] - before[0], before[3] - before[2], before[5] - before[4]]
    )
    document.apply_trsf(rotation_z_matrix(compute_pin1_angle(-3.0, -2.0)))
    after = document.bounds()
    diag_after = np.linalg.norm(
        [after[1] - after[0], after[3] - after[2], after[5] - after[4]]
    )
    _check(
        abs(diag_before - diag_after) < 1.0e-6,
        "bounds diagonal changed — rotation is not rigid",
    )
    print(f"rigid rotation OK on {path.name} (diag {diag_before:.4f})")


def selftest_m3(fixture: Path | None = None) -> None:
    """Pin detection: SOIC-20-300 (pins as bodies) must yield exactly 20 pins
    with 10 per row and serpentine numbering; unibody segmentation on
    ct-sot-23-5 is logged (best-effort by design)."""
    import numpy as np

    from .document import EditorDocument
    from .pins import Band, PinRegistry, detect_pins_multibody, detect_pins_unibody, order_pins

    path = fixture or FIXTURES / "SOIC-20-300.STEP"
    document = EditorDocument.load(path)
    if fixture is None:
        # The fixture ships lying on its side (pin rows split along Z) —
        # condition it the way the workflow would: stand it up first.
        rot_x90 = np.array(
            [[1, 0, 0, 0], [0, 0, -1, 0], [0, 1, 0, 0], [0, 0, 0, 1]],
            dtype=np.float64,
        )
        document.apply_trsf(rot_x90)
    xmin, xmax, ymin, ymax, _zmin, _zmax = document.bounds()
    pad = 1.0
    band = Band(xmin - pad, ymin - pad, xmax + pad, ymax + pad)

    pins = detect_pins_multibody(document, band, exclude_largest=True)
    print(f"multibody candidates in band: {len(pins)}")
    _check(len(pins) == 20, f"expected 20 pins on SOIC-20, got {len(pins)}")

    registry = PinRegistry()
    registry.set_pins([pins[i] for i in order_pins(pins, mode="serpentine")])
    centroids = np.array([p.centroid for p in registry.pins])
    y_mid = (centroids[:, 1].min() + centroids[:, 1].max()) * 0.5
    row1 = centroids[:10]
    row2 = centroids[10:]
    _check(
        np.all(row1[:, 1] < y_mid) and np.all(row2[:, 1] >= y_mid),
        "serpentine rows are mixed",
    )
    _check(
        np.all(np.diff(row1[:, 0]) > 0) and np.all(np.diff(row2[:, 0]) < 0),
        "serpentine X ordering is not monotone",
    )
    print("serpentine ordering OK: 10 ascending-X + 10 descending-X")

    # Unibody hard case: SOT-23-5 is one solid; banding each lead row (the
    # band stops short of the package wall, as a user would draw it) must
    # find exactly the 3+2 leads.
    unibody_path = FIXTURES / "ct-sot-23-5.stp"
    unibody = EditorDocument.load(unibody_path)
    rot_x90 = np.array(
        [[1, 0, 0, 0], [0, 0, -1, 0], [0, 1, 0, 0], [0, 0, 0, 1]], dtype=np.float64
    )
    unibody.apply_trsf(rot_x90)
    bx = unibody.bounds()
    span = bx[3] - bx[2]
    south = Band(bx[0] - 1, bx[2] - 1, bx[1] + 1, bx[2] + span * 0.15)
    north = Band(bx[0] - 1, bx[3] - span * 0.15, bx[1] + 1, bx[3] + 1)
    south_pins = detect_pins_unibody(unibody, 0, south)
    north_pins = detect_pins_unibody(unibody, 0, north)
    print(f"unibody {unibody_path.name}: south={len(south_pins)} north={len(north_pins)}")
    _check(len(south_pins) == 3, f"expected 3 south leads, got {len(south_pins)}")
    _check(len(north_pins) == 2, f"expected 2 north leads, got {len(north_pins)}")


def selftest_m4(fixture: Path | None = None) -> None:
    """Hitboxes: auto-boxes and 3-click boxes must contain every mesh vertex
    of their pin; the convex hull variant likewise."""
    import numpy as np

    from .document import EditorDocument
    from .hitbox import (
        convex_hull_hitbox,
        obb_contains,
        obb_from_points,
        obb_from_three_clicks,
    )
    from .pins import Band, detect_pins_multibody, pin_mesh_points

    path = fixture or FIXTURES / "SOIC-20-300.STEP"
    document = EditorDocument.load(path)
    rot_x90 = np.array(
        [[1, 0, 0, 0], [0, 0, -1, 0], [0, 1, 0, 0], [0, 0, 0, 1]], dtype=np.float64
    )
    document.apply_trsf(rot_x90)
    b = document.bounds()
    pins = detect_pins_multibody(document, Band(b[0] - 1, b[2] - 1, b[1] + 1, b[3] + 1))
    _check(len(pins) == 20, f"expected 20 pins, got {len(pins)}")

    margin = 0.05
    for pin in pins:
        points = pin_mesh_points(document, pin)
        _check(len(points) > 0, "pin without mesh points")
        box = obb_from_points(points, margin=margin)
        _check(bool(obb_contains(box, points).all()), "auto-box does not contain its pin")
    print("auto-box containment OK for 20 pins")

    # Synthetic 3-click box around the first pin's XY bounds.
    points = pin_mesh_points(document, pins[0])
    low, high = points.min(axis=0), points.max(axis=0)
    box = obb_from_three_clicks(
        (low[0] - margin, low[1] - margin, low[2]),
        (high[0] + margin, low[1] - margin, low[2]),
        (high[0] + margin, high[1] + margin, low[2]),
        z_range=(low[2], high[2]),
        margin=margin,
    )
    _check(bool(obb_contains(box, points).all()), "3-click box does not contain the pin")
    angled = obb_from_three_clicks((0, 0, 0), (1, 1, 0), (0, 2, 0), z_range=(0, 1))
    _check(abs(angled["rotation_z_deg"] - 45.0) < 1e-9, "3-click rotation wrong")
    print("3-click OBB OK (incl. rotated base edge)")

    import trimesh

    hull = convex_hull_hitbox(points, margin=margin)
    hull_mesh = trimesh.PointCloud(np.asarray(hull["vertices"])).convex_hull
    inside = hull_mesh.contains(points)
    _check(bool(inside.all()), "convex hull does not contain the pin")
    _check(len(hull["vertices"]) <= 64, "hull vertex budget exceeded")
    print(f"convex hull containment OK ({len(hull['vertices'])} vertices)")


def selftest_m6(fixture: Path | None = None) -> None:
    """Colors: assign known colours to bodies, export, re-read with the XCAF
    colour tool — RGB must round-trip within 1/255."""
    from .document import EditorDocument
    from .export_ap242 import export_ap242

    path = fixture or FIXTURES / "RESC3216X07L.step"
    document = EditorDocument.load(path)
    _check(len(document.bodies) >= 2, "fixture should be multi-body")

    assigned = [
        (212 / 255, 175 / 255, 55 / 255),   # gold
        (32 / 255, 36 / 255, 40 / 255),     # epoxy black
    ]
    for index, body in enumerate(document.bodies):
        body.color = assigned[index % len(assigned)]
        if body.mesh is not None:
            body.mesh.face_colors = {}

    with tempfile.TemporaryDirectory(prefix="step_editor_m6_") as temp:
        out_path = Path(temp) / "recolored.step"
        report = export_ap242(document, out_path)
        _check(report.ok, "export validation failed")
        reread = EditorDocument.load(out_path)
        _check(len(reread.bodies) == len(document.bodies), "body count changed")
        for index, body in enumerate(reread.bodies):
            expected = assigned[index % len(assigned)]
            _check(body.color is not None, f"body {index} lost its colour")
            for a, b in zip(body.color, expected):
                _check(abs(a - b) <= 1.0 / 255.0 + 1e-9,
                       f"body {index} colour drifted: {body.color} vs {expected}")
    print(f"colour round-trip OK for {len(document.bodies)} bodies")


SELFTESTS = {
    "m0": selftest_m0,
    "m1": selftest_m1,
    "m2": selftest_m2,
    "m3": selftest_m3,
    "m4": selftest_m4,
    "m6": selftest_m6,
}


def run(name: str, fixture: Path | None = None) -> int:
    names = list(SELFTESTS) if name == "all" else [name]
    failures = 0
    for test_name in names:
        test = SELFTESTS.get(test_name)
        if test is None:
            print(f"FAIL {test_name}: unknown selftest (have: {', '.join(SELFTESTS)})")
            failures += 1
            continue
        try:
            print(f"--- selftest {test_name} ---")
            test(fixture)
            print(f"PASS {test_name}")
        except Exception as exc:
            print(f"FAIL {test_name}: {exc}")
            failures += 1
    return 1 if failures else 0

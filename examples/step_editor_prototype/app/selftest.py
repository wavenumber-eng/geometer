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
    matrix = compute_zsit_matrix(picks[:3], picks[3], origin_rule="centroid")
    document.apply_trsf(matrix)
    nb = document.bounds()
    print(f"re-sat    z=[{nb[4]:.4f}, {nb[5]:.4f}]")
    _check(abs(nb[4]) < 1.0e-4, f"z-min should be 0, got {nb[4]}")

    # 4-point mode: best-fit plane through four (noisy) seating picks
    import numpy as _np

    four = [scrambled := None]  # placeholder to keep flake quiet
    four = [
        ((-1.0, -1.0, 0.001), (1.0, -1.0, -0.001), (1.0, 1.0, 0.001),
         (-1.0, 1.0, -0.001)),
    ][0]
    m4 = compute_zsit_matrix(four, (0.0, 0.0, 5.0), origin_rule="rect-center")
    origin_world = m4 @ _np.array([0.0, 0.0, 0.0, 1.0])
    _check(abs(origin_world[2]) < 0.01, "4-point plane fit off")
    print("4-point best-fit plane OK")
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
    """Pin-1 swing math: every quadrant lands in +X+Y using exact 90-degree
    rotations only; a manual rotation followed by its reset restores the
    document pose."""
    import math

    import numpy as np

    from .document import EditorDocument
    from .tools.pin1_quadrant import compute_pin1_angle, rotation_z_matrix

    samples = [(4.2, 3.1), (-2.0, 5.0), (-3.3, -0.7), (1.5, -6.0)]
    for x, y in samples:
        angle = compute_pin1_angle(x, y)
        rotated = rotation_z_matrix(angle) @ np.array([x, y, 0.0, 1.0])
        _check(
            rotated[0] >= -1.0e-9 and rotated[1] >= -1.0e-9,
            f"({x},{y}) -> ({rotated[0]:.3f},{rotated[1]:.3f}) not in +X+Y",
        )
        _check(
            abs(math.degrees(angle) % 90.0) < 1.0e-9,
            f"non-90-degree swing angle {math.degrees(angle)}",
        )
    print("swing math OK: all quadrants, exact 90-degree steps")

    path = fixture or FIXTURES / "SOIC-20-300.STEP"
    document = EditorDocument.load(path)
    before = np.array(document.bounds())
    # Manual rotation (arbitrary angle) then reset must restore the pose.
    document.apply_trsf(rotation_z_matrix(math.radians(33.0)))
    rotated_bounds = np.array(document.bounds())
    _check(
        bool(np.any(np.abs(rotated_bounds - before) > 1.0e-6)),
        "manual rotation did not change the pose",
    )
    document.apply_trsf(rotation_z_matrix(math.radians(-33.0)))
    after = np.array(document.bounds())
    _check(
        bool(np.all(np.abs(after - before) < 1.0e-6)),
        "reset did not restore the original pose",
    )
    print(f"manual rotate + reset round-trip OK on {path.name}")


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

    # Mouth-pin similarity search: a smooth region seeded on one lead (the
    # same growth Mouth Seed uses) must find matching regions on the other
    # (identical) leads of the SOT-23.
    from .pins import (
        Pin,
        find_similar_regions,
        grow_smooth_region,
        join_mouth_pins,
        mesh_face_areas,
        mesh_region_centroid,
    )

    areas = mesh_face_areas(unibody.bodies[0].mesh)
    seed_face = max(
        (face for _body, face in south_pins[0].face_ids),
        key=lambda face: areas.get(face, 0.0),
    )
    seed = grow_smooth_region(unibody, 0, seed_face)
    matches = find_similar_regions(unibody, 0, seed)
    known = np.array([p.centroid for p in (south_pins[1:] + north_pins)])
    hits = 0
    for match in matches:
        centroid = mesh_region_centroid(
            unibody.bodies[0].mesh, sorted(face for _b, face in match)
        )
        if centroid is not None and np.min(
            np.linalg.norm(known - np.asarray(centroid), axis=1)
        ) < 0.6:
            hits += 1
    _check(hits >= 3, f"similarity search found only {hits}/4 other leads")
    print(f"similarity search OK ({len(matches)} matches, {hits} on real leads)")

    # Single-face seed (Mouth Seed is normal face picking): matching faces on
    # the other leads must be found without any region growth.
    single = find_similar_regions(unibody, 0, {seed_face})
    _check(
        all(len(match) == 1 for match in single),
        "single-face seed returned grown regions",
    )
    _check(len(single) >= 3, f"single-face similarity found only {len(single)}")
    print(f"single-face similarity OK ({len(single)} matching faces)")

    # Joining: mouth contacts inherit the designator of the tail pin they
    # line up with along the connector row, regardless of staging order.
    primaries = [
        Pin(number=i + 1, centroid=(float(i), 0.0, 0.0), name=str(i + 1))
        for i in range(4)
    ]
    mouths = [
        Pin(number=0, centroid=(float(i), 2.0, 1.5), role="mouth")
        for i in (2, 0, 3, 1)  # shuffled
    ]
    assigned, conflicts = join_mouth_pins(primaries, mouths)
    _check(not conflicts, f"join reported {len(conflicts)} conflicts")
    for index, primary in assigned.items():
        _check(
            float(mouths[index].centroid[0]) == float(primary.centroid[0]),
            f"mouth at x={mouths[index].centroid[0]} joined to "
            f"primary at x={primary.centroid[0]}",
        )
    print("mouth join OK (4 contacts inherited the in-line designator)")

    # BGA grid naming + anchored prediction on a synthetic 4x4 ball grid.
    from .pins import Pin, predict_grid_names, predict_serpentine_order

    grid = [
        Pin(number=0, centroid=(float(x), float(y), 0.0))
        for y in (3, 2, 1, 0)
        for x in (0, 1, 2, 3)
    ]
    default_names = predict_grid_names(grid, {})
    _check(default_names[0] == "A1", f"default A1 corner wrong: {default_names[0]}")
    _check(default_names[15] == "D4", f"default D4 corner wrong: {default_names[15]}")
    # Anchor flips the row direction: the ball at (0, y=3) is declared D1.
    flipped = predict_grid_names(grid, {0: "D1"})
    _check(flipped[0] == "D1" and flipped[15] == "A4",
           f"anchored prediction wrong: {flipped[0]}, {flipped[15]}")
    print("grid prediction OK (default + anchored row flip)")

    # Depopulated channel (DDR-style): two 2-column banks 4 pitches apart
    # must number across the gap (1, 2, 6, 7), not consecutively.
    channel = [
        Pin(number=0, centroid=(float(x), float(y), 0.0))
        for y in (1, 0)
        for x in (0, 1, 5, 6)
    ]
    channel_names = predict_grid_names(channel, {})
    columns = sorted({int(name[1:]) for name in channel_names.values()})
    _check(columns == [1, 2, 6, 7], f"channel columns wrong: {columns}")
    print("grid prediction OK (depopulated channel)")

    # Swapped-axis grid: letters run along X (model not re-oriented). Two
    # anchors with different letters at the same Y must flip the axis
    # assignment instead of failing.
    swapped = [
        Pin(number=0, centroid=(float(x), float(y), 0.0))
        for x in (0, 1, 2)
        for y in (0, 1, 2)
    ]
    # letters along X: A at x=0; numbers along Y: 1 at y=0
    swapped_names = predict_grid_names(swapped, {0: "A1", 3: "B1"})
    _check(swapped_names[1] == "A2" and swapped_names[8] == "C3",
           f"axis-swap prediction wrong: {swapped_names}")
    print("grid prediction OK (swapped letter axis)")

    # Numeric anchors pick the matching serpentine variant.
    two_row = [
        Pin(number=0, centroid=(float(x), float(y), 0.0))
        for y in (0, 3)
        for x in (0, 1, 2)
    ]
    order = predict_serpentine_order(two_row, {3: 1})  # top-left ball is pin 1
    _check(order is not None and order[0] == 3, "serpentine prediction failed")
    print("serpentine prediction OK (anchored)")

    # Edge-connector layout: pins run in two COLUMNS (split along X, ordered
    # by Y) — pin 1 top-left going down, then up the right column.
    connector = [
        Pin(number=0, centroid=(float(x), float(y), 0.0))
        for x in (0, 5)
        for y in (4, 3, 2, 1, 0)
    ]
    col_order = predict_serpentine_order(connector, {0: 1, 9: 6})
    _check(col_order == [0, 1, 2, 3, 4, 9, 8, 7, 6, 5],
           f"column serpentine wrong: {col_order}")
    print("serpentine prediction OK (two-column connector)")


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

    # Paintbrush-style per-face colour on body 0, face 1.
    face_red = (200 / 255, 30 / 255, 30 / 255)
    document.bodies[0].mesh.face_colors[1] = face_red

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
        reread_face = reread.bodies[0].mesh.face_colors.get(1)
        _check(reread_face is not None, "face colour lost on export")
        for a, b in zip(reread_face, face_red):
            _check(abs(a - b) <= 1.0 / 255.0 + 1e-9,
                   f"face colour drifted: {reread_face} vs {face_red}")

        # Viewers (wn3d browser) only colour single-free-shape files: the
        # export must be ONE assembly, not N top-level solids.
        from OCP.IFSelect import IFSelect_RetDone
        from OCP.STEPCAFControl import STEPCAFControl_Reader
        from OCP.TCollection import TCollection_ExtendedString
        from OCP.TDF import TDF_LabelSequence
        from OCP.TDocStd import TDocStd_Document
        from OCP.XCAFApp import XCAFApp_Application
        from OCP.XCAFDoc import XCAFDoc_DocumentTool

        doc2 = TDocStd_Document(TCollection_ExtendedString("MDTV-XCAF"))
        XCAFApp_Application.GetApplication_s().InitDocument(doc2)
        reader = STEPCAFControl_Reader()
        reader.SetColorMode(True)
        _check(reader.ReadFile(str(out_path)) == IFSelect_RetDone, "re-read failed")
        reader.Transfer(doc2)
        labels = TDF_LabelSequence()
        XCAFDoc_DocumentTool.ShapeTool_s(doc2.Main()).GetFreeShapes(labels)
        _check(labels.Length() == 1,
               f"export should be 1 assembly, got {labels.Length()} free shapes")
    print(f"colour round-trip OK for {len(document.bodies)} bodies + 1 painted face")


def selftest_sep(fixture: Path | None = None) -> None:
    """Separate Unibody by edge flow: section-detect the SOT-23-5 lead feet
    (the seeds), grow each pin until the flow reaches the BODY, then split at
    the pin/body junctions. Pin bodies must be WHOLE leads (they extend far
    above the section plane), volume is conserved, and the result exports."""
    import numpy as np

    from .document import EditorDocument, shape_volume
    from .pins import detect_pins_by_section, grow_pin_regions

    path = fixture or FIXTURES / "ct-sot-23-5.stp"
    document = EditorDocument.load(path)
    rot_x90 = np.array(
        [[1, 0, 0, 0], [0, 0, -1, 0], [0, 1, 0, 0], [0, 0, 0, 1]], dtype=np.float64
    )
    document.apply_trsf(rot_x90)
    bounds = document.bounds()
    z = bounds[4] + (bounds[5] - bounds[4]) * 0.06  # through the lead feet
    point, normal = (0.0, 0.0, z), (0.0, 0.0, -1.0)

    pins = detect_pins_by_section(document, point, normal)
    print(f"section at z={z:.3f}: {len(pins)} closed shapes")
    _check(len(pins) == 5, f"expected 5 lead feet, got {len(pins)}")

    grown = grow_pin_regions(document, pins, area_factor=4.0)
    seed_count = sum(len(pin.face_ids) for pin in pins)
    grown_count = sum(len(region) for region in grown if region)
    print(f"edge flow grew {seed_count} seed faces to {grown_count}")
    _check(grown_count > seed_count, "edge flow did not grow past the seeds")

    # A mouth pin (face not in any cut region) must survive the split: its
    # (body, face) reference dies with the old body table and is remapped by
    # centroid onto the rebuilt one.
    from .pins import (
        Pin,
        capture_pin_face_anchors,
        mesh_face_areas,
        mesh_region_centroid,
        remap_pin_faces,
    )

    cut_faces = {face for region in grown if region for _b, face in region}
    mesh0 = document.bodies[0].mesh
    mouth_face = max(
        (f for f in mesh_face_areas(mesh0) if f not in cut_faces),
        key=lambda f: mesh_face_areas(mesh0)[f],
    )
    mouth_centroid = mesh_region_centroid(mesh0, [mouth_face])
    mouth = Pin(number=99, centroid=mouth_centroid,
                face_ids=[(0, mouth_face)], role="mouth")
    anchors = capture_pin_face_anchors(document, [mouth])

    volume_before = sum(shape_volume(b.solid) or 0.0 for b in document.bodies)
    pin_indices = document.split_by_face_regions([r for r in grown if r])
    print(f"split: {len(document.bodies)} bodies, {len(pin_indices)} pin solids")
    _check(len(pin_indices) == 5, f"expected 5 pin bodies, got {len(pin_indices)}")
    _check(len(document.bodies) >= 6, "package body missing after split")

    # Whole-lead check: a plane split would cap the pins exactly at the
    # section plane; edge-flow pins must rise well above it.
    rise = (bounds[5] - bounds[4]) * 0.15
    for body_index in pin_indices:
        top = float(document.bodies[body_index].mesh.points[:, 2].max())
        _check(top > z + rise, f"pin body capped too low: top={top:.3f} vs plane z={z:.3f}")
    print(f"pin bodies are whole leads (tops > {z + rise:.3f})")

    volume_after = sum(shape_volume(b.solid) or 0.0 for b in document.bodies)
    _check(
        abs(volume_after - volume_before) <= max(volume_before, 1e-9) * 1e-3,
        f"volume not conserved: {volume_before} -> {volume_after}",
    )
    print(f"volume conserved: {volume_before:.4f} -> {volume_after:.4f}")

    diag = float(np.linalg.norm([
        bounds[1] - bounds[0], bounds[3] - bounds[2], bounds[5] - bounds[4],
    ]))
    remapped = remap_pin_faces(document, anchors, diag * 5.0e-3)
    _check(remapped == 1, "mouth pin was not remapped across the split")
    new_body, new_face = mouth.face_ids[0]
    new_centroid = mesh_region_centroid(
        document.bodies[new_body].mesh, [new_face]
    )
    drift = float(np.linalg.norm(
        np.asarray(new_centroid) - np.asarray(mouth_centroid)
    ))
    _check(drift <= diag * 5.0e-3,
           f"remapped mouth face drifted {drift:.4f}")
    print(f"mouth pin remapped across the split (drift {drift:.2e})")

    from .export_ap242 import export_ap242

    with tempfile.TemporaryDirectory(prefix="step_editor_sep_") as temp:
        report = export_ap242(document, Path(temp) / "separated.step")
        _check(report.ok, "separated model failed export validation")
    print("separated model exports and re-reads OK")


def selftest_m5(fixture: Path | None = None) -> None:
    """Pin functions: bulk-assignment parsing and storage on the registry."""
    from .pins import Pin, PinRegistry
    from .tools.pin_functions import parse_function_assignments

    parsed = parse_function_assignments(" 1:GND, 2 : VCC ; 5:SDA, bad, 7:")
    _check(parsed == {1: "GND", 2: "VCC", 5: "SDA", 7: ""},
           f"bulk parse wrong: {parsed}")

    registry = PinRegistry()
    registry.set_pins([Pin(number=0, centroid=(float(i), 0.0, 0.0)) for i in range(4)])
    by_number = {pin.number: pin for pin in registry.pins}
    for number, function in parse_function_assignments("1:GND,2:PWR,4:SDA").items():
        by_number[number].function = function
    functions = [pin.function for pin in registry.pins]
    _check(functions == ["GND", "PWR", "", "SDA"], f"functions wrong: {functions}")
    print("bulk parse + registry function assignment OK")


def selftest_m7(fixture: Path | None = None) -> None:
    """Apply LOGO: emboss WN3D.dxf into the top face of a resistor via
    geometer.planar_step + boolean cut. Volume must drop by a plausible
    amount, the result must stay one valid solid, and the engraved faces
    must carry the chosen logo colour."""
    import numpy as np

    from .document import EditorDocument, shape_volume
    from .dxf_loader import emboss_logo

    here = Path(__file__).resolve().parents[1]
    dxf = here / "WN3D.dxf"
    _check(dxf.is_file(), f"missing {dxf}")

    path = fixture or FIXTURES / "RESC3216X07L.step"
    document = EditorDocument.load(path)

    # largest body, topmost planar face (the resistor's top)
    volumes = [shape_volume(b.solid) or 0.0 for b in document.bodies]
    body_index = int(np.argmax(volumes))
    mesh = document.bodies[body_index].mesh
    best_face, best_z = None, -np.inf
    for fid in np.unique(mesh.tri_face_ids):
        frame = document.face_plane(body_index, int(fid))
        if frame is None or frame["normal"][2] < 0.9:
            continue
        if frame["origin"][2] > best_z:
            best_face, best_z = int(fid), float(frame["origin"][2])
    _check(best_face is not None, "no upward planar face found")

    before = shape_volume(document.bodies[body_index].solid)
    width, depth = 2.0, 0.05
    delta = emboss_logo(
        document, body_index, best_face, dxf,
        width_mm=width, depth_mm=depth, logo_rgb=(1.0, 0.84, 0.0),
    )
    print(f"engrave volume delta: {delta:.6f} (body was {before:.4f})")
    _check(delta < -1e-6, "engrave did not remove material")
    _check(abs(delta) < width * width * depth, "removed more than the logo bbox")

    from OCP.BRepCheck import BRepCheck_Analyzer

    _check(BRepCheck_Analyzer(document.bodies[body_index].solid).IsValid(),
           "engraved solid is invalid")
    mesh2 = document.bodies[body_index].mesh
    gold = [fid for fid, rgb in mesh2.face_colors.items()
            if abs(rgb[0] - 1.0) < 0.02 and abs(rgb[1] - 0.84) < 0.02]
    print(f"logo-coloured faces: {len(gold)}")
    _check(len(gold) > 0, "no engraved faces were coloured")
    # the logo colour belongs ONLY to faces strictly below the cut surface —
    # the face the logo cuts through keeps its own colour
    tri_centers = mesh2.points[mesh2.tris].mean(axis=1)
    for fid in gold:
        z = float(tri_centers[mesh2.tri_face_ids == fid].mean(axis=0)[2])
        _check(z < best_z - 1e-4,
               f"logo colour leaked onto a surface-level face (z={z:.4f})")
    print("logo colour confined below the surface plane")

    from .export_ap242 import export_ap242

    with tempfile.TemporaryDirectory(prefix="step_editor_m7_") as temp:
        report = export_ap242(document, Path(temp) / "logo.step")
        _check(report.ok, "engraved model failed export validation")
    print("engraved model exports and re-reads OK")


def selftest_m8(fixture: Path | None = None) -> None:
    """End-to-end conditioning with metadata + journal replay: scramble ->
    Z-sit -> detect -> name -> functions -> hitboxes -> export. The embedded
    JSON must round-trip, pins sharing a designator must group into one net,
    and replaying the journal headlessly must reproduce the same nets."""
    import json

    import numpy as np

    from .document import EditorDocument
    from .export_ap242 import export_ap242, extract_metadata
    from .hitbox import obb_from_points
    from .journal import Journal
    from .pins import Band, PinRegistry, detect_pins_multibody, order_pins, pin_mesh_points
    from .replay import replay
    from .tools.zsit import compute_zsit_matrix

    path = fixture or FIXTURES / "SOIC-20-300.STEP"
    document = EditorDocument.load(path)
    journal = Journal()

    rot_x90 = np.array(
        [[1, 0, 0, 0], [0, 0, -1, 0], [0, 1, 0, 0], [0, 0, 0, 1]], dtype=np.float64
    )
    document.apply_trsf(rot_x90)
    journal.record("zsit", {"origin_rule": "rect-center"}, {"points": []},
                   {"matrix": rot_x90.tolist()})

    b = document.bounds()
    band = [b[0] - 1, b[2] - 1, b[1] + 1, b[3] + 1]
    pins = detect_pins_multibody(document, Band(*band), exclude_largest=True)
    registry = PinRegistry()
    registry.set_pins(pins)
    registry.reorder(order_pins(registry.pins, mode="serpentine"))
    journal.record("detect_pins", {"exclude_largest": True,
                                   "ordering": "serpentine"},
                   {"bands": [band]}, {"added": len(pins)})
    _check(len(registry.pins) == 20, f"expected 20 pins, got {len(registry.pins)}")

    # bridged pair: pins 1 and 2 share designator SW_A -> ONE net
    registry.pins[0].name = "SW_A"
    registry.pins[1].name = "SW_A"
    registry.pins[0].function = "NO"
    registry.pins[1].function = "INHERIT"  # flag: takes the net's function
    journal.record("pin_functions", {}, {"pin_number": 1}, {"function": "NO"})
    journal.record("pin_functions", {}, {"pin_number": 2}, {"function": "INHERIT"})

    applied = []
    for pin in registry.pins:
        pin.hitbox = obb_from_points(pin_mesh_points(document, pin), margin=0.05)
        applied.append({"pin": pin.number, "variant": "auto", "hitbox": pin.hitbox})
    journal.record("hitboxes", {}, {}, {"applied": applied})

    with tempfile.TemporaryDirectory(prefix="step_editor_m8_") as temp:
        out_path = Path(temp) / "conditioned.step"
        report = export_ap242(document, out_path, pins=registry, journal=journal)
        _check(report.ok, "export validation failed")

        payload = extract_metadata(out_path)
        _check(payload is not None, "embedded metadata not found in the STEP")
        _check(payload["schema"] == "wn3d.step_conditioning.a0", payload["schema"])
        nets = {net["designator"]: net for net in payload["nets"]}
        _check(len(nets["SW_A"]["pins"]) == 2,
               "same-designator pins did not group into one net")
        _check(nets["SW_A"]["functions"] == ["NO"],
               f"INHERIT leaked into net functions: {nets['SW_A']['functions']}")
        inherit_pin = next(p for p in nets["SW_A"]["pins"] if p["number"] == 2)
        _check(inherit_pin["function"] == "INHERIT",
               "INHERIT flag lost from the pin")
        _check(len(payload["nets"]) == 19, f"net count {len(payload['nets'])}")
        _check(all(p["hitbox"] for net in payload["nets"] for p in net["pins"]),
               "a pin lost its hitbox in the metadata")
        _check(len(payload["journal"]) == len(journal.operations),
               "journal not embedded completely")
        _check(not out_path.with_suffix(".metadata.json").exists(),
               "sidecar written — metadata must live only inside the AP242")
        print(f"metadata embedded + extracted: {len(payload['nets'])} nets, "
              f"{len(payload['journal'])} journal ops")

        # headless replay of the same journal reproduces the same nets
        fresh = EditorDocument.load(path)
        replayed = replay(fresh, journal)
        _check(len(replayed.pins) == 20, f"replay pins {len(replayed.pins)}")
        zb = fresh.bounds()
        _check(abs(zb[4] - b[4]) < 1e-6, "replayed transform differs")
        functions = sorted(p.function for p in replayed.pins if p.function)
        _check(functions == ["INHERIT", "NO"], f"replayed functions {functions}")
        boxed = sum(1 for p in replayed.pins if p.hitbox)
        _check(boxed == 20, f"replayed hitboxes {boxed}")
        print("journal replay reproduces the conditioning")

        # A real session journal easily exceeds the ~16k-char string limit of
        # OCCT's STEP parser — the injected blob must be chunked across
        # entities or the conditioned file becomes unreadable.
        for i in range(400):
            journal.record("seed_pin", {"angle": None},
                           {"point": [float(i), 0.0, 0.0]},
                           {"faces": list(range(40)),
                            "centroid": [float(i), 0.0, 0.0]})
        big_path = Path(temp) / "conditioned_big.step"
        report = export_ap242(document, big_path, pins=registry, journal=journal)
        _check(report.ok, "large-journal export not re-readable (chunking)")
        big_payload = extract_metadata(big_path)
        _check(big_payload is not None
               and len(big_payload["journal"]) == len(journal.operations),
               "large journal did not round-trip")
        print(f"large metadata blob OK "
              f"({len(json.dumps(big_payload, separators=(',', ':')))} chars, "
              f"chunked)")


SELFTESTS = {
    "m0": selftest_m0,
    "m5": selftest_m5,
    "m7": selftest_m7,
    "m8": selftest_m8,
    "m1": selftest_m1,
    "m2": selftest_m2,
    "m3": selftest_m3,
    "m4": selftest_m4,
    "m6": selftest_m6,
    "sep": selftest_sep,
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

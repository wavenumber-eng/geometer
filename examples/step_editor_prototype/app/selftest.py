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


SELFTESTS = {
    "m0": selftest_m0,
    "m1": selftest_m1,
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

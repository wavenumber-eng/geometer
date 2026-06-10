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


SELFTESTS = {
    "m0": selftest_m0,
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

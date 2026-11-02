"""Split by color: a same-colour region holding a tail and its mouth is ONE
contact; a region holding several tails is shared between them; the body's
largest colour region (housing / shell) is never a pin."""

from pathlib import Path

from OCP.BRepPrimAPI import BRepPrimAPI_MakeBox

from app.document import BodyRecord, EditorDocument
from app.pins import Pin, color_region_bundles

GOLD, BLACK = (0.9, 0.7, 0.2), (0.05, 0.05, 0.05)


def _box(gold_faces: set) -> EditorDocument:
    """A long box (one big black face area) with some faces painted gold."""
    doc = EditorDocument(Path("box.step"), [BodyRecord(solid=BRepPrimAPI_MakeBox(10, 1, 1).Shape(),
                                                       name="box", color=BLACK)], "TEST")
    doc.remesh_all()
    mesh = doc.bodies[0].mesh
    mesh.face_colors = {f: (GOLD if f in gold_faces else BLACK) for f in range(1, mesh.face_count + 1)}
    return doc


def _pin(face: int, role: str = "primary") -> Pin:
    return Pin(number=0, centroid=(0, 0, 0), face_ids=[(0, face)], role=role)


def _gold_strip(doc) -> list[int]:
    """Two adjacent long side faces (one gold region, smaller than the black rest)."""
    mesh = doc.bodies[0].mesh
    adjacency = doc.face_smooth_adjacency(0, None)
    long_faces = sorted(range(1, mesh.face_count + 1), key=lambda f: -(mesh.tri_face_ids == f).sum())[:4]
    first = long_faces[0]
    second = next(f for f in long_faces[1:] if f in adjacency.get(first, ()))
    return [first, second]


def test_tail_and_mouth_in_one_region_are_one_contact():
    doc = _box(set())
    a, b = _gold_strip(doc)
    doc = _box({a, b})
    bundles = color_region_bundles(doc, [_pin(a), _pin(b, role="mouth")])
    assert set(bundles) == {0} and bundles[0] == {(0, a), (0, b)}


def test_two_tails_in_one_region_share_it():
    doc = _box(set())
    a, b = _gold_strip(doc)
    doc = _box({a, b})
    bundles = color_region_bundles(doc, [_pin(a), _pin(b)])
    assert bundles == {0: {(0, a)}, 1: {(0, b)}}


def test_housing_colour_is_never_a_pin():
    doc = _box(set())
    a, b = _gold_strip(doc)
    doc = _box({a, b})
    black = next(f for f in range(1, doc.bodies[0].mesh.face_count + 1) if f not in (a, b))
    assert color_region_bundles(doc, [_pin(black)]) == {}


def test_growth_follows_colour_and_joins_the_touching_mouth():
    """Growth on a coloured body takes the seed's whole colour region, and a
    mouth whose region touches the tail's joins it and takes its name."""
    from app.pins import grow_pin_regions

    doc = _box(set())
    a, b = _gold_strip(doc)
    doc = _box({a, b})
    tail, mouth = _pin(a), _pin(b, role="mouth")
    tail.name, mouth.name = "1", "7"
    grown = grow_pin_regions(doc, [tail, mouth])
    assert set(grown[0]) == {(0, a), (0, b)} and grown[1] is None
    assert mouth.name == "1"

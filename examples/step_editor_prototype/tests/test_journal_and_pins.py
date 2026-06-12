"""Pure-logic invariants from the original milestones: the operation journal
(the seed of the fully-automatic conditioner — every tool execution must be
replayable from declarative JSON) and pin ordering."""

from __future__ import annotations

from app.journal import Journal
from app.pins import Pin, order_pins


class TestJournalContract:
    def test_record_roundtrips_through_json_file(self, tmp_path):
        journal = Journal()
        journal.record("zsit", {"tol": 0.01}, {"points": [[0, 0, 0]]},
                       {"matrix": [[1, 0], [0, 1]]})
        journal.record("pins", {}, {}, {"count": 20})
        path = tmp_path / "ops.json"
        journal.save(path)
        loaded = Journal.load(path)
        assert loaded.to_jsonable() == journal.to_jsonable()
        assert [op.tool for op in loaded.operations] == ["zsit", "pins"]

    def test_load_tolerates_missing_optional_keys(self, tmp_path):
        path = tmp_path / "ops.json"
        path.write_text('[{"tool": "zsit"}]', encoding="utf-8")
        loaded = Journal.load(path)
        assert loaded.operations[0].params == {}
        assert loaded.operations[0].inputs == {}
        assert loaded.operations[0].result == {}

    def test_jsonable_shape_is_stable(self):
        # Replay and the embedded metadata both consume this exact shape.
        journal = Journal()
        journal.record("logo", {"depth": 0.05}, {"face": 3}, {})
        assert journal.to_jsonable() == [
            {"tool": "logo", "params": {"depth": 0.05},
             "inputs": {"face": 3}, "result": {}}
        ]


class TestBodyPerPinContract:
    """The body-split / pin-designation contract: every body that IS a pin
    carries the pin's identity in its name and role, however it became one."""

    def _doc(self, n_bodies):
        from types import SimpleNamespace

        return SimpleNamespace(bodies=[
            SimpleNamespace(name=f"vendor.{i}", role="body") for i in range(n_bodies)
        ])

    def test_single_owner_named_pin_n(self):
        from app.pins import apply_pin_body_names

        doc = self._doc(3)
        pins = [Pin(number=1, centroid=(0, 0, 0), body_ids=[2])]
        assert apply_pin_body_names(doc, pins) == 1
        assert doc.bodies[2].name == "PIN_1"
        assert doc.bodies[2].role == "pin"
        assert doc.bodies[0].name == "vendor.0"  # non-pin bodies untouched

    def test_designator_wins_over_number(self):
        from app.pins import apply_pin_body_names

        doc = self._doc(2)
        pins = [Pin(number=3, centroid=(0, 0, 0), body_ids=[1], name="SW_A")]
        apply_pin_body_names(doc, pins)
        assert doc.bodies[1].name == "SW_A"

    def test_mouth_pins_get_head_suffix(self):
        from app.pins import apply_pin_body_names

        doc = self._doc(2)
        pins = [Pin(number=2, centroid=(0, 0, 0), body_ids=[0], role="mouth")]
        apply_pin_body_names(doc, pins)
        assert doc.bodies[0].name == "PIN_2_HEAD"

    def test_shared_body_names_all_owners(self):
        from app.pins import apply_pin_body_names

        doc = self._doc(1)
        pins = [Pin(number=2, centroid=(0, 0, 0), body_ids=[0]),
                Pin(number=1, centroid=(1, 0, 0), body_ids=[0])]
        apply_pin_body_names(doc, pins)
        assert doc.bodies[0].name == "PINS_1_2"  # bridged contacts, sorted

    def test_face_region_pins_do_not_rename(self):
        from app.pins import apply_pin_body_names

        doc = self._doc(1)
        pins = [Pin(number=1, centroid=(0, 0, 0), face_ids=[(0, 5)])]
        assert apply_pin_body_names(doc, pins) == 0
        assert doc.bodies[0].name == "vendor.0"


class TestJoinMouthPins:
    """CON/HEAD designator joining. The DDR5 regression (2026-06-12): a DIMM
    socket has TWO parallel rows of tails and contacts — 1-D projection onto
    the dominant axis collapses the rows together and scrambles designators
    across them. The join must be row-aware."""

    def _pin(self, x, y, number, role="primary", name="", source=""):
        pin = Pin(number=number, centroid=(x, y, 0.0), role=role, name=name)
        pin.name_source = source
        return pin

    def test_single_row_joins_by_position(self):
        from app.pins import join_mouth_pins

        primaries = [self._pin(i * 1.0, 0.0, i + 1) for i in range(8)]
        mouths = [self._pin(i * 1.0, 5.0, 0, role="mouth") for i in range(8)]
        assigned, conflicts = join_mouth_pins(primaries, mouths)
        assert not conflicts
        assert all(assigned[m].number == m + 1 for m in range(8))

    def test_two_row_connector_keeps_rows_apart(self):
        # DDR5-style: row A tails at y=-3 with mouths at y=-1, row B tails at
        # y=+3 with mouths at y=+1. Same X positions in both rows — the 1-D
        # join ties every cross-row pair; row-aware must keep sides separate.
        from app.pins import join_mouth_pins

        pitch = 0.85
        primaries, mouths = [], []
        for i in range(20):
            primaries.append(self._pin(i * pitch, -3.0, i + 1))           # row A
            primaries.append(self._pin(i * pitch, +3.0, i + 101))         # row B
            mouths.append(self._pin(i * pitch, -1.0, 0, role="mouth"))    # row A
            mouths.append(self._pin(i * pitch, +1.0, 0, role="mouth"))    # row B
        assigned, conflicts = join_mouth_pins(primaries, mouths)
        assert not conflicts
        for m in range(0, 40, 2):   # row A mouths -> row A numbers (1..20)
            assert assigned[m].number == m // 2 + 1
        for m in range(1, 40, 2):   # row B mouths -> row B numbers (101..120)
            assert assigned[m].number == m // 2 + 101

    def test_two_row_anchor_overrides_row_proximity(self):
        # An anchored mouth names a primary on the FAR row: the anchor's row
        # vote must win over perpendicular proximity for its whole row.
        from app.pins import join_mouth_pins

        primaries = [self._pin(i * 1.0, -3.0, i + 1) for i in range(12)]
        primaries += [self._pin(i * 1.0, +3.0, i + 101) for i in range(12)]
        mouths = [self._pin(i * 1.0, -1.0, 0, role="mouth") for i in range(12)]
        mouths[0].name = "101"
        mouths[0].name_source = "anchor"
        assigned, _conflicts = join_mouth_pins(primaries, mouths)
        assert assigned[0].number == 101
        assert all(assigned[m].number == m + 101 for m in range(1, 12))

    def test_mirrored_row_detected_by_two_anchors(self):
        from app.pins import join_mouth_pins

        primaries = [self._pin(i * 1.0, 0.0, i + 1) for i in range(6)]
        mouths = [self._pin((5 - i) * 1.0, 4.0, 0, role="mouth") for i in range(6)]
        mouths[0].name, mouths[0].name_source = "1", "anchor"
        mouths[5].name, mouths[5].name_source = "6", "anchor"
        assigned, conflicts = join_mouth_pins(primaries, mouths)
        assert not conflicts
        assert all(assigned[m].number == m + 1 for m in range(6))

    def test_ddr5_staggered_tails_one_contact_row_per_side(self):
        # Real DIMM-socket geometry: each side's SMT tails stagger into two
        # sub-rows (near/far), while the contacts form one row per side
        # inside the slot. Sub-rows must stay grouped with their side — a
        # per-sub-row split would orphan half the designators.
        from app.pins import join_mouth_pins

        pitch = 0.85
        primaries, mouths = [], []
        for i in range(30):
            stagger = -0.6 if i % 2 else -1.6   # side A staggered tails
            primaries.append(self._pin(i * pitch, stagger - 2.0, i + 1))
            stagger_b = 0.6 if i % 2 else 1.6   # side B staggered tails
            primaries.append(self._pin(i * pitch, stagger_b + 2.0, i + 145))
            mouths.append(self._pin(i * pitch, -1.0, 0, role="mouth"))  # side A
            mouths.append(self._pin(i * pitch, +1.0, 0, role="mouth"))  # side B
        assigned, conflicts = join_mouth_pins(primaries, mouths)
        assert not conflicts
        for m in range(0, 60, 2):
            assert assigned[m].number == m // 2 + 1
        for m in range(1, 60, 2):
            assert assigned[m].number == m // 2 + 145

    def test_misaligned_mouth_conflicts(self):
        from app.pins import join_mouth_pins

        primaries = [self._pin(i * 1.0, 0.0, i + 1) for i in range(4)]
        mouths = [self._pin(10.0, 2.0, 0, role="mouth")]  # far past the row end
        _assigned, conflicts = join_mouth_pins(primaries, mouths)
        assert conflicts == [0]

    def test_mouth_designators_always_from_primary_set(self):
        # User invariant (2026-06-12): CON/HEAD pins are numbered exactly as
        # their SMT/THR counterparts — a mouth designator outside the primary
        # set is impossible by construction. Every join result must reference
        # a primary OBJECT, never a fabricated number.
        from app.pins import join_mouth_pins

        pitch = 0.85
        primaries = [self._pin(i * pitch, -3.0, i + 1) for i in range(40)]
        primaries += [self._pin(i * pitch, +3.0, i + 145) for i in range(40)]
        mouths = [self._pin(i * pitch, -1.0, 0, role="mouth") for i in range(40)]
        mouths += [self._pin(i * pitch, +1.0, 0, role="mouth") for i in range(40)]
        assigned, _conflicts = join_mouth_pins(primaries, mouths)
        primary_ids = {id(p) for p in primaries}
        assert assigned and all(id(p) in primary_ids for p in assigned.values())


class TestCurvedFaceLogoFrame:
    def test_tangent_frame_on_non_planar_face(self, fixtures_dir):
        # User request (2026-06-12): logo must apply on curved/drafted faces
        # too — as a tangent-plane stamp at the picked point.
        import numpy as np
        from app.document import EditorDocument

        doc = EditorDocument.load(fixtures_dir / "SOIC-20-300.STEP")
        found = None
        for body_index, body in enumerate(doc.bodies):
            mesh = body.mesh
            if mesh is None or not len(mesh.tris):
                continue
            for face_id in np.unique(mesh.tri_face_ids)[:60]:
                if doc.face_plane(body_index, int(face_id)) is None:
                    mask = mesh.tri_face_ids == face_id
                    point = mesh.points[np.unique(mesh.tris[mask])].mean(axis=0)
                    found = (body_index, int(face_id), point)
                    break
            if found:
                break
        assert found, "fixture has no curved face to test against"
        body_index, face_id, point = found
        frame = doc.face_frame_at(body_index, face_id, point)
        assert frame is not None, "curved face must yield a tangent frame"
        n, u = np.asarray(frame["normal"]), np.asarray(frame["u"])
        assert abs(np.linalg.norm(n) - 1.0) < 1e-6
        assert abs(np.linalg.norm(u) - 1.0) < 1e-6
        assert abs(float(n @ u)) < 1e-6  # orthonormal
        assert np.linalg.norm(np.asarray(frame["origin"]) - point) < 1.0

    def test_planar_face_unchanged(self, fixtures_dir):
        import numpy as np
        from app.document import EditorDocument

        doc = EditorDocument.load(fixtures_dir / "SOIC-20-300.STEP")
        for body_index, body in enumerate(doc.bodies):
            mesh = body.mesh
            if mesh is None or not len(mesh.tris):
                continue
            for face_id in np.unique(mesh.tri_face_ids)[:60]:
                plane = doc.face_plane(body_index, int(face_id))
                if plane is not None:
                    mask = mesh.tri_face_ids == face_id
                    point = mesh.points[np.unique(mesh.tris[mask])].mean(axis=0)
                    frame = doc.face_frame_at(body_index, int(face_id), point)
                    assert np.allclose(frame["normal"], plane["normal"])
                    return
        raise AssertionError("no planar face found")


class TestPinOrdering:
    def _grid(self):
        # two rows of five, like a SOIC-10 footprint
        pins = []
        for i in range(5):
            pins.append(Pin(number=0, centroid=(float(i), -3.0, 0.0)))
        for i in range(5):
            pins.append(Pin(number=0, centroid=(float(i), 3.0, 0.0)))
        return pins

    def test_serpentine_returns_permutation(self):
        pins = self._grid()
        order = list(order_pins(pins, mode="serpentine"))
        assert sorted(order) == list(range(len(pins)))

    def test_ordering_is_deterministic(self):
        pins = self._grid()
        assert list(order_pins(pins, mode="serpentine")) == list(
            order_pins(pins, mode="serpentine")
        )

    def test_rows_stay_contiguous(self):
        # DESIGN_INTENT: ordering follows geometric centers — a row is
        # walked fully before the order jumps to the other row.
        pins = self._grid()
        order = list(order_pins(pins, mode="serpentine"))
        row_of = [0 if pins[i].centroid[1] < 0 else 1 for i in order]
        transitions = sum(1 for a, b in zip(row_of, row_of[1:]) if a != b)
        assert transitions == 1

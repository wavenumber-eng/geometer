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

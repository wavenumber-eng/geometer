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

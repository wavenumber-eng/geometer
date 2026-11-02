"""Replayed pin naming goes by place, not by detection order (2026-11-01).

Replay can detect the same pins in another order than the live session did
(ADXL1001 and BTN8982TA came back with every name on the wrong pad, because
the recorded names were applied by list position). With the recorded
centroids each name goes to the pin at its place; old journals without
centroids keep the old behaviour."""

from app.journal import Operation
from app.pins import Pin, PinRegistry
from app.replay import _detect_predict, _ReplayState


def _state(xs):
    registry = PinRegistry()
    registry.set_pins([Pin(i + 1, (x, 0.0, 0.0)) for i, x in enumerate(xs)])
    return _ReplayState(None, registry)


def test_names_follow_the_recorded_places():
    state = _state([3.0, 1.0, 2.0])  # replay found them right to left-ish
    op = Operation("detect_pins", {"action": "number_from_footprint"}, {},
                   {"names": ["1", "2", "3"], "order": [1, 2, 3],
                    "centroids": [[1.0, 0, 0], [2.0, 0, 0], [3.0, 0, 0]]})
    _detect_predict(state, op)
    assert [(p.name, p.centroid[0], p.number) for p in state.registry.pins] == \
        [("1", 1.0, 1), ("2", 2.0, 2), ("3", 3.0, 3)]


def test_old_journals_without_centroids_keep_the_order_rule():
    state = _state([3.0, 1.0, 2.0])
    op = Operation("detect_pins", {"action": "predict"}, {}, {"names": ["A", "B", "C"], "order": [1, 2, 3]})
    _detect_predict(state, op)
    assert [p.name for p in state.registry.pins] == ["A", "B", "C"]
    assert [p.centroid[0] for p in state.registry.pins] == [3.0, 1.0, 2.0]

"""BGA: names follow JEDEC from the A1 corner (the pin-1 mark, a hint, or the
top-left default), typed names are the only ground truth, and lands are round
at 80% of the ball."""

import numpy as np

from app.pins import Pin, a1_anchor, predict_grid_names
from app.tools.footprint_embed import BGA_LAND_RATIO, _bga_land


def _grid(n=6, pitch=1.0):
    half = (n - 1) * pitch / 2
    return [Pin(number=i + 1, centroid=(x, y, 0.26), kind="bga")
            for i, (x, y) in enumerate((x, y) for y in np.linspace(half, -half, n) for x in np.linspace(-half, half, n))]


def _where(pins, names, name):
    return tuple(round(v, 2) for v in pins[next(i for i, n in names.items() if n == name)].centroid[:2])


def test_a1_top_left_numbers_rightward_letters_downward():
    pins = _grid()
    names = predict_grid_names(pins, a1_anchor(pins, None))
    assert _where(pins, names, "A1") == (-2.5, 2.5)
    assert _where(pins, names, "A6") == (2.5, 2.5)
    assert _where(pins, names, "F1") == (-2.5, -2.5)


def test_a1_follows_the_pin1_mark_corner():
    pins = _grid()
    names = predict_grid_names(pins, a1_anchor(pins, (2.5, -2.5)))
    assert _where(pins, names, "A1") == (2.5, -2.5)
    assert len(set(names.values())) == len(pins)


def test_bga_land_is_round_at_the_ratio_of_the_ball():
    land = _bga_land({"pin": "A1", "centre_xy": [0.0, 0.0], "size_xy": [0.6, 0.6]})
    assert land["shape"] == "circle" and land["w"] == land["h"] == round(BGA_LAND_RATIO * 0.6, 4)

"""Holes from the peg / lead outline at the board: a round GUIDE peg gets a
round unplated hole, a rectangular one a slot; a square plated lead gets a
round hole that clears its diagonal (IPC-2222)."""

import math

from app.tools.footprint_embed import NPTH_ALLOWANCE, THT_HOLE_ALLOWANCE, _drill_text, _hole_land


def _pad(kind, sx, sy, area):
    return {"pin": "1", "kind": kind, "section_z_mm": -0.05, "centre_xy": [0.0, 0.0],
            "size_xy": [sx, sy], "area_mm2": area}


def test_round_guide_peg_gets_a_round_unplated_hole():
    land = _hole_land(_pad("guide", 0.9, 0.9, math.pi * 0.45 ** 2))
    assert land["type"] == "np_thru_hole" and land["shape"] == "circle"
    assert abs(land["drill"][0] - (0.9 + NPTH_ALLOWANCE)) < 1e-9


def test_rectangular_guide_peg_gets_a_slot():
    land = _hole_land(_pad("guide", 1.6, 0.6, 1.6 * 0.6))
    assert land["shape"] == "oval" and land["drill"][0] > land["drill"][1]
    assert "oval" in _drill_text(land["drill"])


def test_square_lead_hole_clears_its_diagonal():
    land = _hole_land(_pad("pin", 0.7, 0.7, 0.49))
    assert land["type"] == "thru_hole"
    assert abs(land["drill"][0] - (math.hypot(0.7, 0.7) + THT_HOLE_ALLOWANCE)) < 1e-3

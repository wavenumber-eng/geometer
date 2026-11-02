"""USB-C style lands from the model's own outlines: a flat rectangular
ANCHOR tab gets a plated oval slot (not a round hole off its diagonal), and
every contact of a long row runs out along its own long axis, end pins too
(they sit further out in X than in Y but their leads still run in Y)."""

import numpy as np

from app.tools.footprint_embed import THT_HOLE_ALLOWANCE, _hole_land, _hole_shortfall, _smd_land


def _geo(kind, cx, cy, sx, sy, z=-0.05):
    return {"pin": "1", "kind": kind, "section_z_mm": z, "centre_xy": [cx, cy],
            "size_xy": [sx, sy], "area_mm2": sx * sy}


def test_rectangular_anchor_tab_gets_a_plated_slot():
    tab = _geo("anchor", 4.1, 1.48, 0.9, 0.4)
    land = _hole_land(tab)
    assert land["type"] == "thru_hole" and land["shape"] == "oval"
    assert np.allclose(land["drill"], [0.9 + THT_HOLE_ALLOWANCE, 0.4 + THT_HOLE_ALLOWANCE])
    assert land["w"] > land["drill"][0] and land["h"] > land["drill"][1]
    assert _hole_shortfall(land, tab) is None


def test_slot_too_small_for_its_lead_is_reported():
    land = {"drill": [0.8, 0.3]}
    assert _hole_shortfall(land, _geo("anchor", 0, 0, 0.9, 0.4)) is not None


def test_end_pins_of_a_row_run_along_their_lead():
    centre, body = np.array([0.0, 0.0]), [-4.5, 4.5, -3.0, 3.0]
    lands = [_smd_land(_geo("pin", x, 1.74, 0.2, 0.565, z=0.01), centre, body) for x in (2.75, 0.25, -2.75)]
    for land in lands:
        assert land["h"] > land["w"], land  # long along Y, like the lead
    widths = {round(land["w"], 4) for land in lands}
    assert len(widths) == 1 and widths.pop() < 0.5  # no land bridges to its 0.5 mm-pitch neighbour


def test_squarish_contacts_keep_their_row_pitch():
    """87832-1420: 0.5 x 0.6 contacts on a 2 mm pitch in two rows. Lands run
    out across the row (Y) for every pin, so the pitch stays exactly 2 mm."""
    from app.tools.footprint_embed import _row_axes
    contacts = [_geo("pin", x, y, 0.5, 0.6, z=0.01) for x in (-6, -4, -2, 0, 2, 4, 6) for y in (1.75, -1.75)]
    rows = _row_axes(contacts)
    centre, body = np.array([0.0, 0.0]), [-8.0, 8.0, -3.0, 3.0]
    lands = [_smd_land(c, centre, body, rows.get(id(c))) for c in contacts]
    assert sorted({round(land["x"], 6) for land in lands}) == [-6, -4, -2, 0, 2, 4, 6]
    assert all(land["h"] > land["w"] for land in lands)


def test_tiny_rectangular_lead_gets_a_round_hole():
    """ADDR0136 (DDR4): 0.3 x 0.2 leads are drilled round, clearing the
    diagonal; only a real tab (>= 0.8 mm long) is routed as a slot."""
    land = _hole_land(_geo("pin", 0.0, 1.1, 0.3, 0.2))
    assert land["shape"] == "circle" and land["drill"][0] == land["drill"][1]
    assert land["drill"][0] >= np.hypot(0.3, 0.2) + THT_HOLE_ALLOWANCE - 1e-9


def _dimm_pins():
    """Tails on both sides at the same X (the back side a micron off), and a
    mouth contact over each."""
    from app.pins import Pin
    xs = np.arange(-10, 10) * 0.85
    tails, mouths = [], []
    for side, nudge in ((1, 0.0), (-1, 1e-6)):
        for x in xs:
            tails.append(Pin(number=len(tails) + 1, name=str(len(tails) + 1),
                             centroid=np.array([x + nudge, side * 1.1, -2.75])))
            mouths.append(Pin(number=0, centroid=np.array([x, side * 0.47, 4.0]), role="mouth"))
    return tails, mouths


def test_join_pitch_ignores_micron_offsets_between_sides():
    """DDR4: front and back tails share X to within a micron; the join must
    still see the 0.85 mm pitch and pair every contact with its own tail."""
    from app.pins import join_mouth_pins
    tails, mouths = _dimm_pins()
    assigned, conflicts = join_mouth_pins(tails, mouths)
    assert not conflicts and len(assigned) == len(mouths)
    offsets = [abs(tail.centroid[:2] - mouths[i].centroid[:2] * [1, 1.1 / 0.47]).max() for i, tail in assigned.items()]
    assert max(offsets) < 1e-3


def test_surface_anchor_tab_is_smd_not_a_slot():
    """BM08B-SRSS-TBT (JST SH): the side fixing tabs sit ON the board (sliced
    above the seat). They are unnumbered SMD lands, never plated slots."""
    from types import SimpleNamespace

    from app.tools.footprint_embed import _hole_kind, ipc_land_pattern
    doc = SimpleNamespace(bounds=lambda: (-5.0, 5.0, -1.8, 1.2, 0.0, 2.9))
    pins = [_geo("pin", x, -1.4558, 0.2, 0.8, z=0.01) | {"pin": str(i + 1)}
            for i, x in enumerate((-3.5, -2.5, -1.5, -0.5, 0.5, 1.5, 2.5, 3.5))]
    tabs = [_geo("anchor", x, 0.9942, 0.2, 1.5, z=0.01) | {"pin": "ANCHOR"} for x in (4.6, -4.6)]
    lands = ipc_land_pattern({"pads": pins + tabs}, doc)
    for land in lands[-2:]:
        assert land["type"] == "smd" and land["drill"] is None and land["number"] == ""
        assert (land["w"], land["h"]) == (1.2, 1.8)  # JST's recommended SH mounting pad
    assert all(land["type"] == "smd" for land in lands[:8])
    assert _hole_kind("anchor", 0.01) == "" and _hole_kind("anchor", -0.05) == "plated"
    assert _hole_kind("guide", 0.01) == "non-plated"


def test_through_board_anchor_keeps_its_plated_slot():
    from types import SimpleNamespace

    from app.tools.footprint_embed import ipc_land_pattern
    doc = SimpleNamespace(bounds=lambda: (-5.0, 5.0, -3.0, 3.0, -1.0, 3.0))
    land = ipc_land_pattern({"pads": [_geo("anchor", 4.1, 1.48, 0.9, 0.4) | {"pin": "ANCHOR"}]}, doc)[0]
    assert land["type"] == "thru_hole" and land["drill"] is not None

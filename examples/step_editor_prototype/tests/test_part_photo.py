"""Reference photo -> palette -> proposed recolour (app/part_photo.py)."""

from types import SimpleNamespace

import numpy as np

from app.part_photo import (colour_groups, extract_palette, foreground, is_metal, plan_mapping,
                            suggest_roles)


def _chip_photo():
    """A white product-photo backdrop with a black QFN body and tin pads."""
    img = np.full((200, 300, 3), 250, np.uint8)
    img[60:140, 90:210] = (20, 20, 22)  # black epoxy body (9,600 px)
    for x in range(95, 205, 12):  # tin pads along both long edges (1,800 px)
        img[50:60, x:x + 6] = (190, 190, 186)
        img[140:150, x:x + 6] = (190, 190, 186)
    return img


def test_background_is_removed():
    px = foreground(_chip_photo())
    assert len(px) == 9600 + 2 * 10 * 6 * 10
    assert not (px == 250).all(axis=1).any()


def test_palette_finds_body_and_tin():
    palette = extract_palette(_chip_photo(), colours=4)
    hexes = {p["hex"]: p for p in palette}
    assert len(palette) == 2  # duplicate clusters merge
    body = max(palette, key=lambda p: p["share"])
    assert body["hex"] == "#141416" and not body["metal"]
    tin = next(p for p in palette if p is not body)
    assert tin["metal"] and abs(tin["share"] - 1200 / 10800) < 0.01
    assert set(hexes) == {"#141416", "#bebeba"}


def test_roles():
    palette = extract_palette(_chip_photo())
    assert suggest_roles(palette) == {"body": "#141416", "metal": "#c0c0c0"}  # tin -> the Sn swatch
    assert suggest_roles([]) == {"body": None, "metal": None}
    # every colour metal-looking: the largest is still the body
    only_metal = [{"hex": "#cccccc", "metal": True, "share": 0.7}, {"hex": "#aaaaaa", "metal": True, "share": 0.3}]
    assert suggest_roles(only_metal) == {"body": "#cccccc", "metal": "#aaaaaa"}


def test_photo_record_is_plain_json():
    import json
    from app.part_photo import photo_record
    palette = extract_palette(_chip_photo())
    record = photo_record(b"png bytes", "chip.png", "test", palette, 4, 40)
    assert json.loads(json.dumps(record))["palette"][1]["metal"] is True


def test_metal_rule():
    assert is_metal((192, 192, 192)) and is_metal((200, 195, 180))
    assert not is_metal((20, 20, 20))  # black is not metal
    assert not is_metal((60, 150, 80))  # green housing is not metal


def _mesh(face_ids, colours):
    """Two triangles per face, unit squares: each face has area 1."""
    pts, tris, ids = [], [], []
    for f in face_ids:
        base = len(pts)
        pts += [(f, 0, 0), (f + 1, 0, 0), (f + 1, 1, 0), (f, 1, 0)]
        tris += [(base, base + 1, base + 2), (base, base + 2, base + 3)]
        ids += [f, f]
    return SimpleNamespace(points=np.array(pts, float), tris=np.array(tris), tri_face_ids=np.array(ids),
                           face_colors=dict(colours))


def test_groups_and_mapping():
    body = SimpleNamespace(mesh=_mesh([0, 1, 2], {}), color=(0.1, 0.1, 0.1), role="body")
    pin = SimpleNamespace(mesh=_mesh([0], {}), color=(0.8, 0.8, 0.8), role="pin")
    doc = SimpleNamespace(bodies=[body, pin])
    groups = colour_groups(doc, SimpleNamespace(pins=[]))
    assert [round(g["share"], 2) for g in groups] == [0.75, 0.25]
    assert groups[0]["on_pins"] == 0 and groups[1]["on_pins"] == 1
    assert groups[1]["keys"] == [(1, 0)]
    plan = plan_mapping(groups, {"body": "#141416", "metal": "#bebeba"})
    assert plan == ["#141416", "#bebeba"]
    # no metal found in the photo: the pins keep their colour
    assert plan_mapping(groups, {"body": "#141416", "metal": None}) == ["#141416", None]


def test_no_pins_known_metal_colour_counts_as_pins():
    body = SimpleNamespace(mesh=_mesh([0, 1, 2], {2: (0.6, 0.6, 0.6)}), color=(0.03, 0.03, 0.03), role="body")
    groups = colour_groups(SimpleNamespace(bodies=[body]), None)
    assert [g["on_pins"] for g in groups] == [0.0, 1.0]
    assert plan_mapping(groups, {"body": "#141416", "metal": "#bebeba"}) == ["#141416", "#bebeba"]


def test_registry_pin_faces_count_as_pins():
    body = SimpleNamespace(mesh=_mesh([0, 1, 2, 3], {3: (0.7, 0.7, 0.7)}), color=(0.1, 0.1, 0.1), role="body")
    registry = SimpleNamespace(pins=[SimpleNamespace(body_ids=[], face_ids=[(0, 3)])])
    groups = colour_groups(SimpleNamespace(bodies=[body]), registry)
    pin_group = next(g for g in groups if g["keys"] == [(0, 3)])
    assert pin_group["on_pins"] == 1


# ----------------------------------------------------- the Colours-page panel

def _panel(tmp_path, monkeypatch):
    import os
    os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
    from PySide6.QtWidgets import QApplication
    from app.tools import colors_photo

    QApplication.instance() or QApplication([])
    monkeypatch.setattr(colors_photo, "CACHE", tmp_path / "photo_cache")
    body = SimpleNamespace(mesh=_mesh([0, 1, 2], {}), color=(0.1, 0.1, 0.1), role="body")
    pin = SimpleNamespace(mesh=_mesh([0], {}), color=(0.8, 0.8, 0.8), role="pin")
    journal = []
    mutated = []

    class Tool:
        id = "colors"
        ctx = SimpleNamespace(
            document=SimpleNamespace(bodies=[body, pin]),
            window=SimpleNamespace(pins=SimpleNamespace(pins=[]), document_mutated=lambda: mutated.append(1)),
            scene=SimpleNamespace(plotter=SimpleNamespace(render=lambda: None)),
            journal=SimpleNamespace(record=lambda **kw: journal.append(kw)))
        current = None

        def status(self, _text):
            pass

        def set_color(self, hex_color):
            self.current = hex_color

        def _paint_many(self, keys, hex_color):
            from PySide6.QtGui import QColor
            c = QColor(hex_color)
            for b, f in keys:
                self.ctx.document.bodies[b].mesh.face_colors[f] = (c.redF(), c.greenF(), c.blueF())

    tool = Tool()
    return colors_photo, colors_photo.PhotoPanel(tool), tool, journal, mutated


def _qimage(rgb):
    from PySide6.QtGui import QImage
    h, w, _ = rgb.shape
    return QImage(np.ascontiguousarray(rgb).data, w, h, 3 * w, QImage.Format.Format_RGB888).copy()


def _loaded_panel(tmp_path, monkeypatch):
    colors_photo, panel, tool, journal, mutated = _panel(tmp_path, monkeypatch)
    image = _qimage(_chip_photo())
    panel.set_image(image, colors_photo.png_bytes(image), "chip.png", "test")
    return panel, tool, journal, mutated


def test_panel_loads_photo(tmp_path, monkeypatch):
    colors_photo, panel, tool, journal, mutated = _panel(tmp_path, monkeypatch)
    image = _qimage(_chip_photo())
    assert np.array_equal(colors_photo.qimage_to_rgb(image), _chip_photo())
    panel.set_image(image, colors_photo.png_bytes(image), "chip.png", "test")
    assert [p["hex"] for p in panel.palette] == ["#141416", "#bebeba"]
    assert (tmp_path / "photo_cache").exists() and len(list((tmp_path / "photo_cache").iterdir())) == 1
    doc = tool.ctx.document
    assert doc.reference_photo["palette"][0]["hex"] == "#141416"


def test_panel_preview_cancel_apply(tmp_path, monkeypatch):
    panel, tool, journal, mutated = _loaded_panel(tmp_path, monkeypatch)
    doc = tool.ctx.document
    body_before = dict(doc.bodies[0].mesh.face_colors)

    panel.match()  # preview: body faces -> photo body, pin -> photo metal
    assert [c.currentData() for c in panel.combos] == ["#141416", "#c0c0c0"]
    assert doc.bodies[0].mesh.face_colors[0] != body_before.get(0)
    panel.cancel()  # everything back, nothing journaled
    assert doc.bodies[0].mesh.face_colors == body_before and not journal

    panel.match()
    panel.combos[1].setCurrentIndex(0)  # the person says: keep the pin colour
    panel.apply()
    assert [e["result"]["color"] for e in journal] == ["#141416"]
    assert journal[0]["inputs"]["keys"] == [[0, 0], [0, 1], [0, 2]]
    assert doc.reference_photo["applied"] == ["#141416"] and mutated
    assert doc.bodies[1].mesh.face_colors == {}  # pin kept


def test_image_search_url():
    from app.tools.colors_photo import image_search_url, search_query
    doc = SimpleNamespace(identity={"mpn": "1984714", "manufacturer": "Phoenix Contact"}, path="x/raw.STEP")
    assert search_query(doc) == "Phoenix Contact 1984714"
    assert image_search_url(doc) == "https://www.google.com/search?tbm=isch&q=Phoenix%20Contact%201984714"
    assert search_query(SimpleNamespace(identity={}, path="w/LAN8741A-EN.STEP")) == "LAN8741A-EN"
    assert image_search_url(None) == ""


def test_metal_snaps_to_plating_swatch_by_its_lit_side():
    """1984714: tin pins photographed against a green housing come out a
    green-grey whose MEAN is as dark as nickel; the lit side reads as tin."""
    img = np.full((200, 300, 3), 250, np.uint8)
    img[40:160, 40:260] = (103, 144, 124)  # mint housing
    shade = np.linspace(212, 140, 30)[:, None, None]  # tin: lit edge fading into shade
    img[160:190, 60:240] = (shade * np.array([0.97, 1.0, 0.97])).astype(np.uint8)  # green-grey, mean ~ Ni
    palette = extract_palette(img, colours=2)
    metal = next(p for p in palette if p["metal"])
    assert metal["swatch"] == "Sn" and metal["swatch_hex"] == "#c0c0c0"
    assert suggest_roles(palette)["metal"] == "#c0c0c0"


def test_dark_metal_snaps_to_zinc():
    from app.part_photo import snap_metal
    assert snap_metal(110)[0] == "ZN" and snap_metal(160)[0] == "Ni" and snap_metal(230)[0] == "Ag"


def test_metal_swatches_match_the_colours_page():
    import os
    os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
    from app.part_photo import METAL_SWATCHES
    from app.tools.colors import SWATCHES
    page = {label: hex_color for hex_color, label, _m in SWATCHES}
    assert all(page[name] == hex_color for name, hex_color, _m in METAL_SWATCHES)


def test_match_options_offer_every_plating_swatch():
    import os
    os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
    from app.tools.colors_photo import match_options
    options = match_options(extract_palette(_chip_photo()))
    values = [v for _t, v in options]
    assert values[0] is None and "#141416" in values
    assert {"#c0c0c0", "#e1e1e1", "#9da1a6", "#6e7378"} <= set(values)
    assert len(values) == len(set(values))  # the snapped Sn is not listed twice


def test_white_plastic_housing_is_not_metal():
    """JST JWPF: a white housing (bright, unsaturated: what the metal rule
    alone calls silver) in light and shadow, plus an orange seal. Mostly
    neutral = plastic: the white is the body, its shading points at it,
    nothing is metal."""
    img = np.full((240, 320, 3), 255, np.uint8)
    img[20:220, 20:300] = (225, 209, 210)    # white housing, lit
    img[150:220, 20:300] = (183, 161, 158)   # its shadowed side (same pinkish tint)
    img[60:120, 120:220] = (228, 103, 12)    # orange seal
    palette = extract_palette(img, colours=4)
    assert not any(p["metal"] for p in palette)
    assert suggest_roles(palette) == {"body": "#e1d1d2", "metal": None}
    shade = next(p for p in palette if p["hex"] == "#b7a19e")
    assert shade["shade_of"] == "#e1d1d2"


def test_tin_of_another_tint_on_a_white_housing_stays_metal():
    """The plastic rule keeps a neutral of ANOTHER tint as metal: cool grey
    tin pins on a warm white housing."""
    img = np.full((240, 320, 3), 255, np.uint8)
    img[20:190, 20:300] = (232, 214, 204)    # warm white housing
    shade = np.linspace(205, 150, 50)[:, None]
    img[190:240, 40:280] = np.stack([shade, shade + 4, shade + 10], axis=-1).astype(np.uint8)  # cool tin
    palette = extract_palette(img, colours=2)
    metal = [p for p in palette if p["metal"]]
    assert len(metal) == 1 and metal[0]["swatch"] in ("Sn", "Ni")
    body = suggest_roles(palette)["body"]
    assert np.abs(np.array([int(body[i:i + 2], 16) for i in (1, 3, 5)]) - (232, 214, 204)).max() <= 3

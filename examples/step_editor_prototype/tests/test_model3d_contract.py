"""model_3d_a0 contract: bodies/pins link to real STEP entities, the
CLOSED_SHELL ⇄ pin loop is closed, and the embedded block round-trips with no
journal. Exercises resolve_entity_refs + build_model_3d + the export wiring.
"""

from __future__ import annotations

import numpy as np
import pytest

from app.document import EditorDocument, shape_volume
from app.export_ap242 import extract_metadata, export_ap242
from app.model3d_export import resolve_entity_refs
from app.pins import Pin, PinRegistry
from mock_data_model import Model3d

FIXTURE = "SOIC-20-300.STEP"


@pytest.fixture
def document(fixtures_dir):
    path = fixtures_dir / FIXTURE
    if not path.is_file():
        pytest.skip(f"fixture {FIXTURE} not present")
    return EditorDocument.load(path)


def _registry_from_small_bodies(document) -> PinRegistry:
    """Every body except the largest (the package) becomes a whole-body pin."""
    volumes = [abs(shape_volume(b.solid) or 0.0) for b in document.bodies]
    package = int(np.argmax(volumes))
    registry = PinRegistry()
    number = 1
    for index, body in enumerate(document.bodies):
        if index == package:
            continue
        centroid = tuple(float(v) for v in body.mesh.points.mean(axis=0))
        registry.pins.append(Pin(number=number, centroid=centroid, body_ids=[index]))
        number += 1
    return registry


def test_entity_refs_resolve_to_shells(document, tmp_path):
    out = tmp_path / "soic.step"
    from app.export_ap242 import write_step

    write_step(document, out)
    refs = resolve_entity_refs(out, document)
    assert len(refs) == len(document.bodies)
    for ref in refs:
        assert ref.shell_id.startswith("#")
        assert ref.solid_id.startswith("#")
        assert ref.face_ids and all(f.startswith("#") for f in ref.face_ids)


def test_model3d_block_links_and_roundtrips(document, tmp_path):
    registry = _registry_from_small_bodies(document)
    out = tmp_path / "soic_conditioned.step"
    report = export_ap242(document, out, pins=registry)
    assert report.ok

    payload = extract_metadata(out, tag="WN3D_MODEL_3D")
    assert payload is not None
    assert payload["type"] == "model_3d_a0"
    assert "journal" not in payload  # the cleaned model never carries the journal

    # exactly one package, the rest pins; every pin links to a real CLOSED_SHELL
    roles = [b["role"] for b in payload["bodies"]]
    assert roles.count("package") == 1
    assert len(payload["pins"]) == len(registry.pins)
    for pin in payload["pins"]:
        assert pin["geometry_ref"]["shell_id"].startswith("#")

    # CLOSED_SHELL ⇄ pin loop: each pin's body carries a pin_ref back to it
    pin_ids = {p["id"] for p in payload["pins"]}
    back_links = {b["pin_ref"] for b in payload["bodies"] if b.get("pin_ref")}
    assert back_links == pin_ids

    # the data model parses its own output losslessly
    assert Model3d.from_json(payload).to_json() == payload

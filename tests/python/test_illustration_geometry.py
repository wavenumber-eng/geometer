from __future__ import annotations

from dataclasses import replace
from pathlib import Path
from hashlib import sha256

import pytest
import geometer
from geometer._generated.contracts.codecs import (
    encode_mesh_collection_a0_json,
    encode_mesh_illustration_geometry_b0_json,
)
from geometer._ipc_a0 import Attachment
from geometer._generated.contracts.models import OperationSuccessB0
from geometer._illustration_geometry import _decode_response

ROOT = Path(__file__).resolve().parents[2]


@pytest.fixture(scope="module")
def illustration_input():
    model = (ROOT / "tests/fixtures/step/embedded_models/SOT-23.STEP").read_bytes()
    with geometer.GeometerClient() as client:
        meshes = client.model_tessellation(model).mesh_collection.meshes
    return geometer.MeshIllustrationGeometryInputB0(
        schema="geometry.mesh_illustration_geometry.input.b0",
        length_unit="millimeter",
        meshes=meshes,
        view=geometer.MeshIllustrationView(direction=(0.4, 0.7, 1), up=(0, 1, 0)),
        style=geometer.MeshIllustrationStyleA0(
            show_outlines=False, show_creases=False, show_hlr_detail=True, show_hlr_outline=True
        ),
    )


@pytest.mark.parametrize("mirror", [False, True])
def test_geometry_matches_svg_with_hlr_and_survives_operation_errors(illustration_input, mirror):
    input = replace(illustration_input, view=replace(illustration_input.view, mirror_x=mirror))
    with geometer.GeometerClient() as client:
        collection = geometer.MeshCollectionA0(
            schema="geometry.mesh_collection.a0", length_unit="millimeter", meshes=input.meshes
        )
        hlr = client.mesh_hlr_projection(
            collection,
            geometer.MeshHlrProjectionRequestB0(
                schema="geometry.mesh_hlr_projection.request.b0",
                views=(geometer.HlrViewSpec(id="test", direction=input.view.direction, up=input.view.up),),
                output_detail=True,
                output_outline=True,
                output_bbox=False,
                fast=geometer.FastHlrOptionsA0(include_hidden=False),
            ),
        )
        geometry = client.mesh_illustration_geometry(input, hlr_projection=hlr)
        svg = client.mesh_illustration(
            geometer.MeshIllustrationInputB0(
                schema="geometry.mesh_illustration.input.b0",
                meshes=input.meshes,
                view=input.view,
                style=input.style,
            ),
            hlr_projection=hlr,
        )
        assert not hasattr(geometry, "svg")
        assert geometry.stats == svg.stats and geometry.warnings == svg.warnings
        assert geometry == client.mesh_illustration_geometry(input, hlr_projection=hlr)
        segments = (*hlr.views[0].modes.detail.segments, *hlr.views[0].modes.outline.segments)
        assert len(geometry.lines) == len(segments)
        for line, segment in zip(geometry.lines, segments):
            assert line.start == pytest.approx(((-1 if mirror else 1) * segment[0], segment[1]))
            assert line.end == pytest.approx(((-1 if mirror else 1) * segment[2], segment[3]))
        with pytest.raises(geometer.GeometerOperationError):
            client.mesh_illustration_geometry(
                replace(input, prepare=geometer.MeshIllustrationPrepareOptions(max_triangles=1))
            )
        with pytest.raises(geometer.GeometerOperationError):
            client.mesh_illustration_geometry(
                input, hlr_projection=replace(hlr, views=(replace(hlr.views[0], up=(1, 0, 0)),))
            )
        assert client.mesh_illustration_geometry(input, hlr_projection=hlr) == geometry


def test_geometry_attachment_integrity_and_semantic_validation(illustration_input):
    input = illustration_input
    attachment = Attachment(
        name="mesh_collection",
        media_type="application/vnd.wavenumber.geometer.mesh-collection+json",
        data=encode_mesh_collection_a0_json(
            geometer.MeshCollectionA0(
                schema="geometry.mesh_collection.a0", length_unit="millimeter", meshes=input.meshes
            )
        ),
    )
    request = geometer.MeshIllustrationGeometryRequestB0(
        schema="geometry.mesh_illustration_geometry.request.b0", view=input.view, style=input.style
    )
    with geometer.GeometerClient() as client:
        response = client.execute("geometry.mesh_illustration_geometry.b0", request, (attachment,))
        geometry = _decode_response(response)
        assert isinstance(response.outcome, OperationSuccessB0)
        metadata = response.outcome.result
        assert isinstance(metadata, geometer.MeshIllustrationGeometryResultB0)
        assert geometry.bounds is not None
        output = response.attachments[0]
        for changed in (
            replace(output, name="wrong"),
            replace(output, media_type="application/json"),
            replace(output, data=output.data + b" "),
        ):
            with pytest.raises(ValueError):
                _decode_response(replace(response, attachments=(changed,)))
        for altered in (
            replace(
                geometry,
                bounds=replace(geometry.bounds, min=geometry.bounds.max, max=geometry.bounds.min),
            ),
            replace(geometry, stats=replace(geometry.stats, commands=geometry.stats.commands + 1)),
        ):
            encoded = encode_mesh_illustration_geometry_b0_json(altered)
            new_metadata = replace(
                metadata,
                stats=altered.stats,
                geometry=replace(metadata.geometry, byte_length=len(encoded), sha256=sha256(encoded).hexdigest()),
            )
            with pytest.raises(ValueError):
                _decode_response(
                    replace(
                        response,
                        outcome=replace(response.outcome, result=new_metadata),
                        attachments=(replace(output, data=encoded),),
                    )
                )
        bad = client.execute("geometry.mesh_illustration_geometry.b0", request, (replace(attachment, data=b"{}"),))
        assert not bad.outcome.ok
        assert client.mesh_illustration_geometry(input) == geometry


def test_geometry_one_shot_and_malformed_result_terminate(illustration_input, monkeypatch):
    with geometer.GeometerClient() as client:
        expected = client.mesh_illustration_geometry(illustration_input)
    assert geometer.mesh_illustration_geometry(illustration_input) == expected
    with geometer.GeometerClient() as client:
        original = client.execute
        terminated = []
        terminate = client._terminate

        def record_terminate():
            terminated.append(True)
            terminate()

        def corrupt(*args, **kwargs):
            response = original(*args, **kwargs)
            return replace(response, attachments=())

        monkeypatch.setattr(client, "execute", corrupt)
        monkeypatch.setattr(client, "_terminate", record_terminate)
        with pytest.raises(geometer.GeometerIpcProtocolError):
            client.mesh_illustration_geometry(illustration_input)
        assert terminated

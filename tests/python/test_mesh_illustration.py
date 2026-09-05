from __future__ import annotations

from dataclasses import replace
from pathlib import Path
import xml.etree.ElementTree as ET

import pytest

import geometer
from geometer._generated.contracts.codecs import (
    decode_mesh_illustration_result_a0_json,
    encode_mesh_illustration_result_a0_json,
)
from geometer._ipc_a0 import Attachment

ROOT = Path(__file__).resolve().parents[2]


def test_native_hlr_composition_and_optional_attachment_validation() -> None:
    model = (ROOT / "tests/fixtures/step/embedded_models/SOT-23.STEP").read_bytes()
    with geometer.GeometerClient() as client:
        meshes = client.model_tessellation(model).mesh_collection.meshes
        view = geometer.MeshIllustrationView(direction=(0.4, 0.7, 1.0), up=(0.0, 1.0, 0.0))
        hlr = client.model_hlr_projection(
            model,
            geometer.HlrProjectionOptionsA0(
                views=(geometer.HlrViewSpec(id="preview", direction=view.direction, up=view.up),),
                projection_algorithm=geometer.HlrProjectionAlgorithm.FAST,
                outline_algorithm=geometer.HlrOutlineAlgorithm.FAST_MESH_SHADOW,
                curve_mode=geometer.HlrCurveMode.POLYLINE,
                strip_root_placement=True,
                output_outline=True,
                output_detail=True,
                output_bbox=False,
                fast=geometer.FastHlrOptionsA0(include_hidden=False),
            ),
        )
        input = geometer.MeshIllustrationInputA0(
            schema="geometry.mesh_illustration.input.a0",
            meshes=meshes,
            view=view,
            style=geometer.MeshIllustrationStyleA0(show_outlines=False, show_creases=False),
        )
        assert input.style is not None
        for outline, detail in [(False, False), (True, False), (False, True), (True, True)]:
            styled = replace(input, style=replace(input.style, show_hlr_outline=outline, show_hlr_detail=detail))
            result = client.mesh_illustration(styled, hlr_projection=hlr)
            assert result == client.mesh_illustration(styled, hlr_projection=hlr)
            assert result.stats.outlines == (len(hlr.views[0].modes.outline.segments) if outline else 0)
            assert result.stats.details == (len(hlr.views[0].modes.detail.segments) if detail else 0)
            assert ET.fromstring(result.svg).tag == "{http://www.w3.org/2000/svg}svg"
            assert decode_mesh_illustration_result_a0_json(encode_mesh_illustration_result_a0_json(result)) == result
        with pytest.raises(geometer.GeometerOperationError):
            client.mesh_illustration(
                input, hlr_projection=replace(hlr, views=(replace(hlr.views[0], up=(1.0, 0.0, 0.0)),))
            )
        from geometer._generated.contracts.codecs import encode_mesh_collection_a0_json

        attachments = (
            Attachment(
                name="mesh_collection",
                media_type="application/vnd.wavenumber.geometer.mesh-collection+json",
                data=encode_mesh_collection_a0_json(
                    geometer.MeshCollectionA0(
                        schema="geometry.mesh_collection.a0", length_unit="millimeter", meshes=meshes
                    )
                ),
            ),
        )
        request = geometer.MeshIllustrationRequestA0(schema="geometry.mesh_illustration.request.a0", view=view)
        bad = client.execute(
            "geometry.mesh_illustration.a0",
            request,
            (
                *attachments,
                Attachment(
                    name="hlr_projection",
                    media_type="application/vnd.wavenumber.geometer.hlr-projection+json",
                    data=b"{}",
                ),
            ),
        )
        assert not bad.outcome.ok
        with pytest.raises(geometer.GeometerIpcProtocolError):
            client.execute(
                "geometry.mesh_illustration.a0",
                request,
                (
                    *attachments,
                    Attachment(
                        name="hlr_projection",
                        media_type="application/octet-stream",
                        data=b"{}",
                    ),
                ),
            )
        assert client.mesh_illustration(input, hlr_projection=hlr).stats.outlines > 0


def test_native_step_illustration_is_typed_deterministic_and_recoverable() -> None:
    model = (ROOT / "tests/fixtures/step/embedded_models/SOT-23.STEP").read_bytes()
    with geometer.GeometerClient() as client:
        meshes = client.model_tessellation(model).mesh_collection.meshes
        input = geometer.MeshIllustrationInputA0(
            schema="geometry.mesh_illustration.input.a0",
            meshes=meshes,
            view=geometer.MeshIllustrationView(direction=(0.4, 0.7, 1.0), up=(0.0, 1.0, 0.0)),
            svg=geometer.MeshIllustrationSvgOptions(title="Native STEP illustration"),
        )
        result = client.mesh_illustration(input)
        assert result == client.mesh_illustration(input)
        assert result.stats.triangles > 0 and result.stats.surface_draws > 0
        assert ET.fromstring(result.svg).tag == "{http://www.w3.org/2000/svg}svg"
        assert decode_mesh_illustration_result_a0_json(encode_mesh_illustration_result_a0_json(result)) == result
        with pytest.raises(geometer.GeometerOperationError) as limited:
            client.mesh_illustration(replace(input, prepare=geometer.MeshIllustrationPrepareOptions(max_triangles=1)))
        assert limited.value.diagnostics[0].code == "geometer.operation.resource_limit_exceeded"
        with pytest.raises(geometer.GeometerOperationError):
            client.mesh_illustration(replace(input, view=replace(input.view, direction=(0.0, 1.0, 0.0))))
        bad = client.execute(
            "geometry.mesh_illustration.a0",
            geometer.MeshIllustrationRequestA0(schema="geometry.mesh_illustration.request.a0", view=input.view),
            (
                Attachment(
                    name="mesh_collection",
                    media_type="application/vnd.wavenumber.geometer.mesh-collection+json",
                    data=b"{}",
                ),
            ),
        )
        assert not bad.outcome.ok
        for attachments in [
            (),
            (Attachment(name="mesh_collection", media_type="application/octet-stream", data=b"{}"),),
        ]:
            with pytest.raises(geometer.GeometerIpcProtocolError):
                client.execute(
                    "geometry.mesh_illustration.a0",
                    geometer.MeshIllustrationRequestA0(schema="geometry.mesh_illustration.request.a0", view=input.view),
                    attachments,
                )
        assert client.mesh_illustration(input) == result


def test_oversized_inline_svg_returns_failure_without_killing_the_server() -> None:
    # Nonoverlapping triangles keep geometry work linear while exercising the
    # actual encoded response cap, not the separate triangle/complexity limits.
    positions = tuple(
        value
        for index in range(128000)
        for value in (
            float(index % 320),
            float(index // 320),
            0.0,
            index % 320 + 0.4,
            float(index // 320),
            0.0,
            float(index % 320),
            index // 320 + 0.4,
            0.0,
        )
    )
    input = geometer.MeshIllustrationInputA0(
        schema="geometry.mesh_illustration.input.a0",
        meshes=(
            geometer.MeshIllustrationMesh(
                id="grid", positions=positions, materials=(geometer.MeshIllustrationMaterial(color=(0.5, 0.5, 0.5)),)
            ),
        ),
        view=geometer.MeshIllustrationView(direction=(0.0, 0.0, 1.0), up=(0.0, 1.0, 0.0)),
        style=geometer.MeshIllustrationStyleA0(fuse_surfaces=False, show_outlines=False, show_creases=False),
    )
    with geometer.GeometerClient() as client:
        with pytest.raises(geometer.GeometerOperationError) as oversized:
            client.mesh_illustration(input, timeout=60)
        assert oversized.value.diagnostics[0].code == "geometer.transport.response_limit_exceeded"
        small = replace(input, meshes=(replace(input.meshes[0], positions=positions[:9]),))
        assert client.mesh_illustration(small).stats.triangles == 1

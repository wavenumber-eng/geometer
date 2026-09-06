"""Headless public-API checks run by the installed-wheel validation lane."""

from __future__ import annotations

import argparse
from dataclasses import replace
from pathlib import Path
import xml.etree.ElementTree as ET

import geometer


def validate(step: Path, output: Path) -> None:
    package = Path(geometer.__file__).resolve().parent
    executable = geometer.executable_path().resolve()
    assert package in executable.parents, "illustration must use the installed wheel's executable"
    model = step.read_bytes()
    view = geometer.MeshIllustrationView(direction=(0.4, 0.7, 1.0), up=(0.0, 1.0, 0.0))
    style = geometer.MeshIllustrationStyleA0(
        show_outlines=False,
        show_creases=False,
        show_hlr_outline=True,
        show_hlr_detail=True,
    )  # Omitted fuse_surfaces uses the governed renderer default: true.
    with geometer.GeometerClient() as client:
        operations = {item.identity for item in client.welcome.operation_catalog.operations}
        assert {"geometry.model_tessellation.a0", "geometry.mesh_illustration.a0"} <= operations
        meshes = client.model_tessellation(model)
        input = geometer.MeshIllustrationInputA0(
            schema="geometry.mesh_illustration.input.a0",
            meshes=meshes.mesh_collection.meshes,
            view=view,
            style=style,
        )
        pure = client.mesh_illustration(input)
        assert pure.stats.surface_draws > 0 and pure.stats.outlines == 0 and pure.stats.details == 0
        assert pure == client.mesh_illustration(replace(input, style=replace(style, fuse_surfaces=True)))
        hlr = client.model_hlr_projection(
            model,
            geometer.HlrProjectionOptionsA0(
                views=(geometer.HlrViewSpec(id="illustration", direction=view.direction, up=view.up),),
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
        composed = client.mesh_illustration(input, hlr_projection=hlr)
        assert composed == client.mesh_illustration(input, hlr_projection=hlr)
        assert composed.stats.outlines > 0 and composed.stats.details > 0
        assert isinstance(composed, geometer.MeshIllustrationResultA0)
        assert composed.schema == "geometry.mesh_illustration.result.a0"
        assert ET.fromstring(composed.svg).tag == "{http://www.w3.org/2000/svg}svg"
        try:
            client.mesh_illustration(replace(input, prepare=geometer.MeshIllustrationPrepareOptions(max_triangles=1)))
        except geometer.GeometerOperationError as error:
            assert error.diagnostics[0].code == "geometer.operation.resource_limit_exceeded"
        else:
            raise AssertionError("native illustration ignored its triangle limit")
        assert client.mesh_illustration(input) == pure
    # Both explicit override and automatic bundled discovery, with one-shot cleanup.
    assert geometer.model_tessellation(model, executable=executable) == meshes
    assert geometer.mesh_illustration(input) == pure
    assert geometer.mesh_illustration(input, executable=executable, hlr_projection=hlr) == composed
    output.mkdir(parents=True, exist_ok=True)
    (output / "native-illustration.svg").write_text(composed.svg, encoding="utf-8")
    print(f"Installed illustration APIs passed: {composed.stats.triangles} triangles; {len(composed.svg)} SVG bytes")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--step", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    args = parser.parse_args()
    validate(args.step, args.out_dir)

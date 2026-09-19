"""Render a repeated analytic 2.5D definition through the model-illustration API."""

from __future__ import annotations

import argparse
from pathlib import Path

import geometer


def rectangle(width: float, height: float) -> geometer.IllustrationProfileRingA0:
    line = geometer.IllustrationProfileLineA0(kind="line")
    return geometer.IllustrationProfileRingA0(
        points_mm=((0, 0), (width, 0), (width, height), (0, height)),
        segments=(line, line, line, line),
    )


def request() -> geometer.ModelIllustrationRequestB0:
    body = geometer.MeshIllustrationMaterial(color=(0.15, 0.42, 0.68), name="body")
    metal = geometer.MeshIllustrationMaterial(color=(0.78, 0.8, 0.82), name="metal")
    marker = geometer.MeshIllustrationMaterial(color=(0.88, 0.18, 0.16), name="marker")
    definition = geometer.AnalyticDefinitionA0(
        id="demo-package",
        primitives=(
            geometer.AnalyticExtrusionA0(
                kind="extrusion",
                id="lower-body",
                regions=(geometer.IllustrationProfileRegionA0(outer=rectangle(9, 6)),),
                z_min_mm=0,
                z_max_mm=1.6,
                material=body,
            ),
            geometer.AnalyticExtrusionA0(
                kind="extrusion",
                id="raised-body",
                regions=(geometer.IllustrationProfileRegionA0(outer=rectangle(5, 3)),),
                z_min_mm=1.6,
                z_max_mm=2.3,
                material=marker,
            ),
            geometer.AnalyticCylinderA0(
                kind="cylinder",
                id="post",
                center_mm=(7.2, 3),
                radius_mm=0.65,
                z_min_mm=1.6,
                z_max_mm=3.2,
                material=metal,
            ),
            geometer.AnalyticSphereA0(
                kind="sphere",
                id="indicator",
                center_mm=(2.2, 4.2, 2.5),
                radius_mm=0.7,
                material=marker,
            ),
        ),
    )
    source = geometer.AnalyticIllustrationSourceA0(
        kind="analytic",
        scene=geometer.AnalyticSceneA0(
            definitions=(definition,),
            occurrences=(
                geometer.AnalyticOccurrenceA0(id="U1", definition_id=definition.id),
                geometer.AnalyticOccurrenceA0(
                    id="U2",
                    definition_id=definition.id,
                    transform=(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 12, 0, 0, 1),
                ),
            ),
        ),
    )
    return geometer.ModelIllustrationRequestB0(
        schema="geometry.model_illustration.request.b0",
        source=source,
        view=geometer.MeshIllustrationView(direction=(0.45, 0.65, 1), up=(0, 1, 0)),
        style=geometer.MeshIllustrationStyleA0(
            show_outlines=False,
            show_creases=False,
            show_hlr_outline=True,
            show_hlr_detail=True,
            transparent_background=True,
        ),
        svg=geometer.MeshIllustrationSvgOptions(title="Repeated analytic package"),
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", type=Path)
    parser.add_argument("--output", type=Path, default=Path("out/examples/analytic-model-illustration.svg"))
    args = parser.parse_args()
    result = geometer.model_illustration(request(), executable=args.executable)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(result.svg, encoding="utf-8")
    print(f"Wrote {args.output.resolve()} ({result.stats.commands} drawing commands)")


if __name__ == "__main__":
    main()

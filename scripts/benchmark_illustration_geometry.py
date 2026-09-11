"""Opt-in SVG/drawing comparison and large attachment transport qualification.

IPC timings include input adaptation, native work, transport and typed decoding.
Output re-encoding is measured separately, outside the IPC timer. This is not a
kernel-only benchmark; preparation/fusion costs are shared by both operations.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import statistics
import time
from pathlib import Path

from benchmark_mesh_illustration import ROOT, geometer, synthetic_grid
from geometer._generated.contracts.codecs import (
    decode_mesh_illustration_geometry_input_a0_json,
    decode_mesh_illustration_input_a0_json,
    encode_mesh_illustration_geometry_a0_json,
    encode_mesh_illustration_result_a0_json,
)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", type=Path, required=True)
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--large", action="store_true", help="Also verify an output attachment over 8 MiB")
    parser.add_argument("--output", type=Path, default=ROOT / "out/illustration-performance/geometry.json")
    args = parser.parse_args()
    if args.repeats < 1:
        parser.error("repeats must be positive")
    report = {"binary_sha256": hashlib.sha256(args.executable.read_bytes()).hexdigest(), "samples": [], "summary": []}
    with geometer.GeometerClient(executable=args.executable) as client:
        collection = client.model_tessellation((ROOT / "tests/fixtures/step/embedded_models/SOT-23.STEP").read_bytes())
        small = geometer.MeshIllustrationInputA0(
            schema="geometry.mesh_illustration.input.a0",
            meshes=collection.mesh_collection.meshes,
            view=geometer.MeshIllustrationView(direction=(0.4, 0.7, 1), up=(0, 1, 0)),
        )
        inputs = {
            "grid": decode_mesh_illustration_input_a0_json(json.dumps(synthetic_grid(48)).encode()),
            "sot23": small,
        }
        for name, svg_input in inputs.items():
            geometry_input = geometer.MeshIllustrationGeometryInputA0(
                schema="geometry.mesh_illustration_geometry.input.a0",
                length_unit="millimeter",
                meshes=svg_input.meshes,
                view=svg_input.view,
                style=svg_input.style,
                prepare=svg_input.prepare,
            )
            reference = None
            for repeat in range(args.repeats):
                for mode in ("svg", "geometry") if repeat % 2 == 0 else ("geometry", "svg"):
                    start = time.perf_counter()
                    result = (
                        client.mesh_illustration(svg_input)
                        if mode == "svg"
                        else client.mesh_illustration_geometry(geometry_input)
                    )
                    elapsed = time.perf_counter() - start
                    start = time.perf_counter()
                    encoded = (
                        encode_mesh_illustration_result_a0_json(result)
                        if isinstance(result, geometer.MeshIllustrationResultA0)
                        else encode_mesh_illustration_geometry_a0_json(result)
                    )
                    encoding = time.perf_counter() - start
                    signature = (result.stats, result.warnings)
                    if reference is None:
                        reference = signature
                    assert signature == reference
                    report["samples"].append(
                        dict(
                            fixture=name,
                            mode=mode,
                            repeat=repeat + 1,
                            ipc_seconds=elapsed,
                            reencode_seconds=encoding,
                            output_json_bytes=len(encoded),
                        )
                    )
            for mode in ("svg", "geometry"):
                rows = [row for row in report["samples"] if (row["fixture"], row["mode"]) == (name, mode)]
                report["summary"].append(
                    dict(
                        fixture=name,
                        mode=mode,
                        ipc_median_seconds=statistics.median(row["ipc_seconds"] for row in rows),
                        reencode_median_seconds=statistics.median(row["reencode_seconds"] for row in rows),
                        output_json_bytes=rows[0]["output_json_bytes"],
                    )
                )
        if args.large:
            fixture = synthetic_grid(190)
            fixture.update(schema="geometry.mesh_illustration_geometry.input.a0", length_unit="millimeter")
            fixture["style"].update(fuse_surfaces=False, layer_coplanar_materials=False)
            typed = decode_mesh_illustration_geometry_input_a0_json(json.dumps(fixture).encode())
            start = time.perf_counter()
            drawing = client.mesh_illustration_geometry(typed, timeout=180)
            elapsed = time.perf_counter() - start
            size = len(encode_mesh_illustration_geometry_a0_json(drawing))
            assert size > 8 * 1024 * 1024 and drawing.stats.surface_draws == 190 * 190 * 2
            assert client.mesh_illustration(small).stats.triangles > 0
            report["large_attachment"] = dict(
                triangles=drawing.stats.triangles, geometry_bytes=size, ipc_seconds=elapsed, reusable=True
            )
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()

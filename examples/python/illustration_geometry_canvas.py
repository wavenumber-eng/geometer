"""Request native illustration geometry and draw it with a small custom Canvas consumer."""

from __future__ import annotations

import argparse
from html import escape
from pathlib import Path

import geometer
from geometer._generated.contracts.codecs import encode_mesh_illustration_geometry_b0_json

ROOT = Path(__file__).resolve().parents[2]

# This consumer receives only the governed geometry DTO. It never reads SVG.
CANVAS = r"""
const geometry = JSON.parse(document.getElementById('geometry').textContent);
const canvas = document.querySelector('canvas'), ctx = canvas.getContext('2d');
const lo = geometry.bounds.min, hi = geometry.bounds.max;
const width = Math.max(hi[0]-lo[0], 1e-9), height = Math.max(hi[1]-lo[1], 1e-9);
const pad = geometry.presentation.padding;
const scale = Math.min(canvas.width/(width+2*pad), canvas.height/(height+2*pad));
const ox = (canvas.width-width*scale)/2-lo[0]*scale;
const oy = (canvas.height-height*scale)/2+hi[1]*scale;
const map = p => [ox+p[0]*scale, oy-p[1]*scale];
if (!geometry.presentation.transparent_background) {
  ctx.fillStyle = geometry.presentation.background;
  ctx.fillRect(0,0,canvas.width,canvas.height);
}
ctx.lineCap = geometry.presentation.line_cap;
ctx.lineJoin = geometry.presentation.line_join;
for (const surface of geometry.surfaces) for (const layer of surface.layers) {
  ctx.beginPath();
  for (const ring of layer.rings) {
    ctx.moveTo(...map(ring.points[0]));
    for (const point of ring.points.slice(1)) ctx.lineTo(...map(point));
    ctx.closePath();
  }
  ctx.globalAlpha = layer.opacity;
  ctx.fillStyle = ctx.strokeStyle = layer.fill;
  ctx.fill(geometry.presentation.fill_rule);
  ctx.lineWidth = Math.max(.7, Math.min(1.5, scale*.0008));
  ctx.stroke();
}
ctx.globalAlpha = 1;
for (const line of geometry.lines) {
  ctx.beginPath(); ctx.moveTo(...map(line.start)); ctx.lineTo(...map(line.end));
  ctx.strokeStyle = line.color; ctx.lineWidth = Math.max(.6, line.width*scale); ctx.stroke();
}
document.getElementById('counts').textContent =
  `${geometry.stats.triangles} triangles → ${geometry.stats.surface_draws} filled layers; ${geometry.lines.length} lines`;
document.documentElement.dataset.rendered = 'true';
"""


def write_canvas(geometry: geometer.MeshIllustrationGeometryB0, output: Path, title: str) -> None:
    if geometry.bounds is None:
        raise ValueError("cannot draw empty illustration geometry")
    encoded = encode_mesh_illustration_geometry_b0_json(geometry)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.with_suffix(".geometry.json").write_bytes(encoded)
    safe_json = encoded.decode().replace("<", "\\u003c").replace("&", "\\u0026")
    output.write_text(
        '<!doctype html><html lang="en"><meta charset="utf-8">'
        '<meta name="viewport" content="width=device-width,initial-scale=1">'
        f"<title>{escape(title)} — illustration geometry</title>"
        "<style>body{font:16px system-ui;margin:24px;background:#eef1f3;color:#17252c}"
        "main{max-width:1100px;margin:auto}canvas{width:100%;height:auto;background:white;"
        "border:1px solid #bbc5c8}p{line-height:1.5}</style><main>"
        f"<h1>{escape(title)}</h1><p>Native illustration geometry drawn directly on Canvas. "
        'No SVG was generated.</p><p id="counts"></p>'
        '<canvas width="1400" height="1050"></canvas>'
        f'<p><a href="{escape(output.with_suffix(".geometry.json").name, quote=True)}">Geometry JSON</a></p>'
        f'<script id="geometry" type="application/json">{safe_json}</script>'
        f"<script>{CANVAS}</script></main></html>",
        encoding="utf-8",
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("step", nargs="?", type=Path, default=ROOT / "tests/fixtures/step/embedded_models/SOT-23.STEP")
    parser.add_argument("--executable", type=Path)
    parser.add_argument("--output", type=Path, default=ROOT / "out/examples/illustration-geometry.html")
    args = parser.parse_args()
    view = geometer.MeshIllustrationView(direction=(0.4, 0.7, 1), up=(0, 1, 0))
    with geometer.GeometerClient(executable=args.executable) as client:
        model = args.step.read_bytes()
        print("Requesting one-pass STEP illustration geometry (no SVG)...", flush=True)
        geometry_response = client.model_illustration_geometry(
            geometer.ModelIllustrationGeometryRequestB0(
                schema="geometry.model_illustration_geometry.request.b0",
                source=geometer.ModelAttachmentIllustrationSourceA0(kind="model", attachment="model"),
                view=view,
                style=geometer.MeshIllustrationStyleA0(
                    show_outlines=False, show_creases=False, show_hlr_outline=True, show_hlr_detail=True
                ),
            ),
            model,
        )
        geometry = geometry_response.geometry
    write_canvas(geometry, args.output.resolve(), args.step.stem)
    print(args.output.resolve())


if __name__ == "__main__":
    main()

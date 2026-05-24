from __future__ import annotations

import argparse
from pathlib import Path

import geometer


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_STEP = ROOT / "tests" / "fixtures" / "step" / "embedded_models" / "SOT-23.STEP"
DEFAULT_OUT_DIR = ROOT / "out" / "examples"

VIEW_FACTORIES = {
    "top": geometer.ProjectionView.top,
    "bottom": geometer.ProjectionView.bottom,
    "front": geometer.ProjectionView.front,
    "back": geometer.ProjectionView.back,
    "right": geometer.ProjectionView.right,
    "left": geometer.ProjectionView.left,
}


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Generate Geometer HLR projection JSON, SVG, and GLB files from a STEP model."
    )
    parser.add_argument("step", nargs="?", type=Path, default=DEFAULT_STEP)
    parser.add_argument("--out-dir", type=Path, default=DEFAULT_OUT_DIR)
    parser.add_argument("--view", choices=sorted(VIEW_FACTORIES), default="top")
    parser.add_argument("--mode", choices=["simple", "detail"], default="simple")
    parser.add_argument("--curve-mode", choices=["polyline", "native_arcs"], default="polyline")
    parser.add_argument("--skip-glb", action="store_true")
    args = parser.parse_args()

    step_path = args.step.resolve()
    if not step_path.exists():
        raise FileNotFoundError(step_path)

    output_dir = args.out_dir.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    stem = step_path.stem
    projection_path = output_dir / f"{stem}.projection.json"
    svg_path = output_dir / f"{stem}.{args.view}.{args.mode}.svg"
    glb_path = output_dir / f"{stem}.glb"

    view = VIEW_FACTORIES[args.view]()
    options = {
        "curve_mode": args.curve_mode,
        "views": [view.to_json_value()],
    }

    projection_text = geometer.hlr_projection_json(step_path, views=[view], options=options)
    projection_path.write_text(projection_text, encoding="utf-8")

    response = geometer.run_batch(
        [
            {
                "id": "svg",
                "operation": "step_hlr_projection_svg",
                "step_path": str(step_path),
                "output_path": str(svg_path),
                "mode": args.mode,
                "view": args.view,
            }
        ],
        options=options,
        work_dir=output_dir,
    )
    if not response.get("ok"):
        raise RuntimeError(f"Geometer SVG batch failed: {response!r}")

    if not args.skip_glb:
        glb_path.write_bytes(geometer.step_to_glb(step_path))

    version = geometer.version()
    print(f"geometer {version.string} abi {version.abi}")
    print(f"executable: {geometer.executable_path()}")
    print(f"projection: {projection_path}")
    print(f"svg: {svg_path}")
    if not args.skip_glb:
        print(f"glb: {glb_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""Build one or all supported single-HTML Geometer demos."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BUILDERS = {
    "analytic-polygon-pour": "build_self_contained_analytic_polygon_pour_demo.py",
    "hlr": "build_self_contained_hlr_demo.py",
    "pcb-polygon-pour": "build_self_contained_pcb_polygon_pour_demo.py",
    "planar-ring": "build_self_contained_planar_ring_solver_demo.py",
}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("demo", choices=[*BUILDERS, "all"])
    args = parser.parse_args()
    selected = list(BUILDERS) if args.demo == "all" else [args.demo]
    for demo in selected:
        script = ROOT / "scripts" / BUILDERS[demo]
        print(f"Building standalone demo: {demo}", flush=True)
        subprocess.run([sys.executable, str(script)], cwd=ROOT, check=True)


if __name__ == "__main__":
    main()

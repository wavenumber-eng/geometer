"""Opt-in paired illustration replay; no STEP/HLR work in measured render calls.

Native-process samples include process startup and JSON I/O. Persistent-IPC
samples include Python adaptation and transport. Neither is a kernel-only timer.
Run from a source checkout with baseline and candidate binaries built alike.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import platform
import statistics
import subprocess
import sys
import time
from contextlib import ExitStack
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "python"))

import geometer  # noqa: E402
from geometer._generated.contracts.codecs import (  # noqa: E402
    decode_mesh_illustration_input_b0_json,
    encode_mesh_illustration_result_b0_json,
)


def synthetic_grid(size: int) -> dict:
    """Two coplanar materials on a tilted, shared-vertex triangle grid."""
    positions, indices, materials = [], [], []
    for row in range(size + 1):
        for col in range(size + 1):
            x, y = (col - size / 2) * 0.7134567, (row - size / 2) * 0.8123456
            positions.extend((x, y, 0.09 * x - 0.06 * y - 0.23123))
    for row in range(size):
        for col in range(size):
            a = row * (size + 1) + col
            indices.extend((a, a + 1, a + size + 2, a, a + size + 2, a + size + 1))
            materials.extend((int(col >= size // 2),) * 2)
    return {
        "schema": "geometry.mesh_illustration.input.b0",
        "meshes": [
            {
                "id": "tilted-grid",
                "positions": positions,
                "indices": indices,
                "materials": [{"color": [0.2, 0.7, 0.62]}, {"color": [0.8, 0.35, 0.1]}],
                "triangle_material_indices": materials,
            }
        ],
        "view": {"direction": [0.4, 0.7, 1], "up": [0, 1, 0]},
        "style": {"show_outlines": False, "show_creases": False},
    }


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline-test", required=True, type=Path)
    parser.add_argument("--candidate-test", required=True, type=Path)
    parser.add_argument("--baseline-exe", required=True, type=Path)
    parser.add_argument("--candidate-exe", required=True, type=Path)
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--grid", type=int, default=48)
    parser.add_argument("--output", type=Path, default=ROOT / "out/illustration-performance/paired.json")
    args = parser.parse_args()
    if args.repeats < 1 or not 1 <= args.grid <= 128:
        parser.error("repeats must be positive and grid must be between 1 and 128")
    binaries = {
        "baseline": (args.baseline_test.resolve(), args.baseline_exe.resolve()),
        "candidate": (args.candidate_test.resolve(), args.candidate_exe.resolve()),
    }
    output = args.output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    collection = json.loads(
        subprocess.run(
            [str(binaries["baseline"][0]), "--step", str(ROOT / "tests/fixtures/step/embedded_models/SOT-23.STEP")],
            check=True,
            capture_output=True,
            timeout=120,
        ).stdout
    )
    fixtures = {
        "grid": synthetic_grid(args.grid),
        "sot23": {
            "schema": "geometry.mesh_illustration.input.b0",
            "meshes": collection["meshes"],
            "view": {"direction": [0.4, 0.7, 1], "up": [0, 1, 0]},
        },
    }
    report = {
        "platform": platform.platform(),
        "python": platform.python_version(),
        "binaries": {
            mode: [{"path": str(p), "sha256": sha256(p.read_bytes())} for p in paths]
            for mode, paths in binaries.items()
        },
        "fixtures": {},
        "samples": [],
        "summary": [],
    }
    with ExitStack() as stack:
        clients = {
            mode: stack.enter_context(geometer.GeometerClient(executable=paths[1])) for mode, paths in binaries.items()
        }
        for name, fixture in fixtures.items():
            data = json.dumps(fixture, separators=(",", ":")).encode()
            path = output.parent / f"{name}.input.json"
            path.write_bytes(data)
            typed = decode_mesh_illustration_input_b0_json(data)
            report["fixtures"][name] = {"sha256": sha256(data), "path": str(path)}
            reference = None
            for repeat in range(args.repeats):
                modes = ("baseline", "candidate") if repeat % 2 == 0 else ("candidate", "baseline")
                for mode in modes:
                    for lane in ("native-process", "persistent-ipc"):
                        started = time.perf_counter()
                        if lane == "native-process":
                            encoded = subprocess.run(
                                [str(binaries[mode][0]), str(path)], check=True, capture_output=True, timeout=120
                            ).stdout
                            elapsed = time.perf_counter() - started
                        else:
                            result = clients[mode].mesh_illustration(typed, timeout=120)
                            elapsed = time.perf_counter() - started
                            encoded = encode_mesh_illustration_result_b0_json(result)
                        result = json.loads(encoded)
                        if reference is None:
                            reference = result
                        if result != reference:
                            raise RuntimeError(f"Illustration parity failed: {name}/{mode}/{lane}")
                        report["samples"].append(
                            {
                                "fixture": name,
                                "mode": mode,
                                "lane": lane,
                                "repeat": repeat + 1,
                                "seconds": elapsed,
                                "svg_sha256": sha256(result["svg"].encode()),
                                "svg_bytes": len(result["svg"].encode()),
                                "stats": result["stats"],
                                "warnings": result["warnings"],
                                "exact": True,
                            }
                        )
                        output.write_text(json.dumps(report, indent=2), encoding="utf-8")
                        print(f"{name} {mode} {lane}: {elapsed:.4f}s exact", flush=True)
    for name in fixtures:
        for lane in ("native-process", "persistent-ipc"):
            medians = {
                mode: statistics.median(
                    row["seconds"]
                    for row in report["samples"]
                    if (row["fixture"], row["lane"], row["mode"]) == (name, lane, mode)
                )
                for mode in binaries
            }
            report["summary"].append(
                {
                    "fixture": name,
                    "lane": lane,
                    "median_seconds": medians,
                    "speedup": medians["baseline"] / medians["candidate"],
                }
            )
    output.write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(json.dumps(report["summary"], indent=2))


if __name__ == "__main__":
    main()

"""Benchmark exact, poly, fast-detail, and mesh-shadow projection backends.

The report can compare the native executable with the Node-hosted WASM CLI.
Wall time deliberately measures the existing one-shot projection surface and
therefore includes runtime startup, STEP import, projection, and serialization;
the projection JSON's phase timings are preserved separately. Layer selection
can isolate outline or detail work while combined rows preserve normal output.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import platform
import shutil
import subprocess
import tempfile
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parent.parent
DEFAULT_MANIFEST = ROOT / "tests/fixtures/embedded_models_manifest.json"
DEFAULT_OUTPUT = ROOT / ".bench-tmp/fast-hlr-baseline.json"
DEFAULT_WASM_CLI = ROOT / "dist/wasm/node-test/geometer-node-test.js"
DEFAULT_MODELS = (
    "SOT-23.STEP",
    "SOIC-8-W.step",
    "sot223.stp",
    "TSOT-23-5.STEP",
    "BGA90-8X13mm.step",
)
PHASE_NAMES = ("step_read_ms", "mesh_ms", "hlr_ms", "extract_ms")


def percentile(values: list[float], fraction: float) -> float:
    if not values:
        raise ValueError("percentile requires at least one value")
    if not 0.0 <= fraction <= 1.0:
        raise ValueError("percentile fraction must be between zero and one")
    ordered = sorted(values)
    index = max(0, math.ceil(fraction * len(ordered)) - 1)
    return ordered[index]


def summarize(values: list[float]) -> dict[str, float | int]:
    if not values:
        raise ValueError("summary requires at least one value")
    return {
        "count": len(values),
        "min": min(values),
        "mean": sum(values) / len(values),
        "p50": percentile(values, 0.50),
        "p95": percentile(values, 0.95),
        "max": max(values),
    }


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        while chunk := source.read(1024 * 1024):
            digest.update(chunk)
    return digest.hexdigest()


def resolve_executable(explicit: Path | None) -> Path:
    if explicit is not None:
        candidates = [explicit]
    elif os.name == "nt":
        candidates = [
            ROOT / "build/src/cpp/cli/geometer.exe",
            ROOT / "dist/native/windows-x64/geometer.exe",
        ]
    else:
        candidates = [
            ROOT / "build/src/cpp/cli/geometer",
            ROOT / "dist/native/linux-x64/geometer",
            ROOT / "dist/native/macos-arm64/geometer",
        ]
    for candidate in candidates:
        resolved = candidate.resolve()
        if resolved.is_file():
            return resolved
    rendered = ", ".join(str(path) for path in candidates)
    raise FileNotFoundError(f"Geometer executable not found; checked: {rendered}")


def resolve_wasm_cli(explicit: Path | None) -> Path:
    candidates = [explicit] if explicit is not None else [DEFAULT_WASM_CLI]
    for candidate in candidates:
        resolved = candidate.resolve()
        if resolved.is_file():
            return resolved
    rendered = ", ".join(str(path) for path in candidates)
    raise FileNotFoundError(f"Geometer Node WASM CLI not found; checked: {rendered}")


def resolve_node(explicit: Path | None) -> Path:
    if explicit is not None:
        resolved = explicit.resolve()
        if resolved.is_file():
            return resolved
        raise FileNotFoundError(f"Node executable not found: {resolved}")
    found = shutil.which("node")
    if found:
        return Path(found).resolve()
    raise FileNotFoundError("Node executable was not found on PATH")


def runtime_target(
    name: str,
    executable: Path | None = None,
    wasm_cli: Path | None = None,
    node: Path | None = None,
) -> dict[str, Any]:
    if name == "native":
        artifact = resolve_executable(executable)
        return {"name": name, "command": [str(artifact)], "artifact": artifact}
    if name == "wasm":
        artifact = resolve_wasm_cli(wasm_cli)
        node_executable = resolve_node(node)
        return {
            "name": name,
            "command": [str(node_executable), str(artifact)],
            "artifact": artifact,
            "host": node_executable,
        }
    raise ValueError(f"unsupported runtime: {name}")


def load_workloads(manifest_path: Path, requested_names: list[str]) -> list[dict[str, Any]]:
    value = json.loads(manifest_path.read_text(encoding="utf-8-sig"))
    if not isinstance(value, list):
        raise ValueError(f"fixture manifest must contain an array: {manifest_path}")
    by_name = {str(item.get("name")): item for item in value if isinstance(item, dict)}
    names = requested_names or list(DEFAULT_MODELS)
    missing = [name for name in names if name not in by_name]
    if missing:
        raise ValueError(f"fixture manifest is missing requested models: {', '.join(missing)}")

    workloads: list[dict[str, Any]] = []
    for name in names:
        item = by_name[name]
        step_path = (ROOT / str(item["step"])).resolve()
        if not step_path.is_file():
            raise FileNotFoundError(f"STEP fixture is missing: {step_path}")
        workloads.append(
            {
                "name": name,
                "step_path": step_path,
                "step_bytes": step_path.stat().st_size,
            }
        )
    return workloads


def _projection_counts(projection: dict[str, Any], view_id: str) -> dict[str, int]:
    views = projection.get("views")
    if not isinstance(views, list):
        raise RuntimeError("projection result is missing views")
    view = next(
        (item for item in views if isinstance(item, dict) and item.get("id") == view_id),
        views[0] if views else None,
    )
    if not isinstance(view, dict) or not isinstance(view.get("modes"), dict):
        raise RuntimeError(f"projection result is missing view {view_id!r}")
    modes = view["modes"]

    def count(mode_name: str, primitive_name: str) -> int:
        mode = modes.get(mode_name)
        values = mode.get(primitive_name) if isinstance(mode, dict) else None
        return len(values) if isinstance(values, list) else 0

    return {
        "outline_segments": count("outline", "segments"),
        "outline_arcs": count("outline", "arcs"),
        "detail_segments": count("detail", "segments"),
        "detail_arcs": count("detail", "arcs"),
    }


def _geometry_digest(projection: dict[str, Any]) -> str:
    geometry = {"schema": projection.get("schema"), "views": projection.get("views")}
    encoded = json.dumps(geometry, sort_keys=True, separators=(",", ":")).encode()
    return hashlib.sha256(encoded).hexdigest()


def run_projection(
    target: dict[str, Any],
    workload: dict[str, Any],
    algorithm: str,
    outline: str,
    layer: str,
    view_id: str,
    timeout_seconds: float,
) -> dict[str, Any]:
    with tempfile.TemporaryDirectory(prefix="geometer-fast-hlr-benchmark-") as directory_text:
        directory = Path(directory_text)
        request_path = directory / "request.json"
        response_path = directory / "response.json"
        projection_path = directory / "projection.json"
        request = {
            "schema": "geometer.batch.request.a0",
            "jobs": [
                {
                    "id": "baseline",
                    "operation": "step_hlr_projection_json",
                    "step_path": str(workload["step_path"]),
                    "output_path": str(projection_path),
                    "options": {
                        "views": [
                            {
                                "id": view_id,
                                "direction": [0.0, 0.0, 1.0],
                                "up": [0.0, 1.0, 0.0],
                            }
                        ],
                        "curve_mode": "polyline",
                        "projection_algorithm": algorithm,
                        "outline_algorithm": outline,
                        "output_outline": layer in {"outline", "both"},
                        "output_detail": layer in {"detail", "both"},
                        "output_bbox": layer == "both",
                    },
                }
            ],
        }
        request_path.write_text(json.dumps(request, separators=(",", ":")), encoding="utf-8")
        started = time.perf_counter()
        completed = subprocess.run(
            [*target["command"], "run", str(request_path), str(response_path)],
            cwd=ROOT,
            capture_output=True,
            check=False,
            text=True,
            timeout=timeout_seconds,
        )
        wall_ms = (time.perf_counter() - started) * 1000.0
        if completed.returncode != 0 or not projection_path.is_file():
            detail = completed.stderr.strip() or completed.stdout.strip() or "no projection output"
            raise RuntimeError(
                f"{target['name']} {workload['name']} {algorithm}/{outline}/{layer} failed "
                f"with exit code {completed.returncode}: {detail}"
            )
        projection = json.loads(projection_path.read_text(encoding="utf-8"))
        timings = projection.get("timings")
        if not isinstance(timings, dict):
            raise RuntimeError("projection result is missing timings")
        phases = {name: float(timings[name]) for name in PHASE_NAMES}
        return {
            "wall_ms": wall_ms,
            "phases_ms": phases,
            "counts": _projection_counts(projection, view_id),
            "geometry_sha256": _geometry_digest(projection),
        }


def benchmark_case(
    target: dict[str, Any],
    workload: dict[str, Any],
    algorithm: str,
    outline: str,
    layer: str,
    view_id: str,
    warmup: int,
    repeat: int,
    timeout_seconds: float,
) -> dict[str, Any]:
    for _ in range(warmup):
        run_projection(target, workload, algorithm, outline, layer, view_id, timeout_seconds)
    samples = [
        run_projection(target, workload, algorithm, outline, layer, view_id, timeout_seconds) for _ in range(repeat)
    ]
    counts = samples[0]["counts"]
    if any(sample["counts"] != counts for sample in samples[1:]):
        raise RuntimeError(f"nondeterministic output counts for {workload['name']}")
    geometry_sha256 = samples[0]["geometry_sha256"]
    if any(sample["geometry_sha256"] != geometry_sha256 for sample in samples[1:]):
        raise RuntimeError(f"nondeterministic output geometry for {workload['name']}")
    summary = {"wall_ms": summarize([sample["wall_ms"] for sample in samples])}
    for phase_name in PHASE_NAMES:
        summary[phase_name] = summarize([sample["phases_ms"][phase_name] for sample in samples])
    return {
        "model": workload["name"],
        "runtime": target["name"],
        "step": str(Path(workload["step_path"]).relative_to(ROOT)).replace("\\", "/"),
        "step_bytes": workload["step_bytes"],
        "algorithm": algorithm,
        "outline_algorithm": outline,
        "layer": layer,
        "view": view_id,
        "counts": counts,
        "geometry_sha256": geometry_sha256,
        "samples": samples,
        "summary": summary,
        "note": (
            "combined mesh-shadow timing includes the selected detail backend; use "
            "--layer outline to isolate outline work"
            if outline == "mesh-shadow" and layer == "both"
            else None
        ),
    }


def _git_value(*arguments: str) -> str | None:
    completed = subprocess.run(["git", *arguments], cwd=ROOT, capture_output=True, check=False, text=True)
    value = completed.stdout.strip()
    return value if completed.returncode == 0 and value else None


def runtime_comparisons(cases: list[dict[str, Any]]) -> list[dict[str, Any]]:
    keys = ("model", "algorithm", "outline_algorithm", "layer", "view")
    grouped: dict[tuple[Any, ...], dict[str, dict[str, Any]]] = {}
    for case in cases:
        key = tuple(case[field] for field in keys)
        grouped.setdefault(key, {})[case["runtime"]] = case
    comparisons = []
    for key, runtimes in grouped.items():
        native = runtimes.get("native")
        wasm = runtimes.get("wasm")
        if native is None or wasm is None:
            continue
        ratios = {}
        for metric in ("wall_ms", *PHASE_NAMES):
            ratios[metric] = {
                statistic: (
                    wasm["summary"][metric][statistic] / native["summary"][metric][statistic]
                    if native["summary"][metric][statistic] > 0
                    else None
                )
                for statistic in ("mean", "p50", "p95")
            }
        comparisons.append(
            {
                **dict(zip(keys, key, strict=True)),
                "wasm_over_native": ratios,
                "counts_equivalent": native["counts"] == wasm["counts"],
                "geometry_equivalent": native["geometry_sha256"] == wasm["geometry_sha256"],
            }
        )
    return comparisons


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--model", action="append", default=[], help="fixture name; repeatable")
    parser.add_argument("--algorithm", action="append", choices=("poly", "exact", "fast"), default=[])
    parser.add_argument(
        "--outline",
        action="append",
        choices=("hlr-close", "mesh-shadow", "fast-mesh-shadow"),
        default=[],
    )
    parser.add_argument("--layer", action="append", choices=("outline", "detail", "both"), default=[])
    parser.add_argument("--view", default="top")
    parser.add_argument("--warmup", type=int, default=1)
    parser.add_argument("--repeat", type=int, default=5)
    parser.add_argument("--timeout-seconds", type=float, default=120.0)
    parser.add_argument(
        "--runtime",
        action="append",
        choices=("native", "wasm"),
        default=[],
        help="execution runtime; repeatable (default: native)",
    )
    parser.add_argument("--executable", type=Path)
    parser.add_argument("--wasm-cli", type=Path)
    parser.add_argument("--node", type=Path)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    return parser


def main() -> int:
    args = build_parser().parse_args()
    if args.warmup < 0 or args.repeat < 1:
        raise ValueError("warmup must be nonnegative and repeat must be positive")
    runtime_names = args.runtime or ["native"]
    targets = [runtime_target(name, args.executable, args.wasm_cli, args.node) for name in runtime_names]
    workloads = load_workloads(args.manifest.resolve(), args.model)
    algorithms = args.algorithm or ["poly", "exact"]
    outlines = args.outline or ["hlr-close", "mesh-shadow"]
    layers = args.layer or ["both"]

    cases: list[dict[str, Any]] = []
    total = len(targets) * len(workloads) * len(algorithms) * len(outlines) * len(layers)
    current = 0
    for target in targets:
        for workload in workloads:
            for algorithm in algorithms:
                for outline in outlines:
                    for layer in layers:
                        current += 1
                        label = f"{target['name']} {workload['name']} {algorithm}/{outline}/{layer}"
                        print(f"[{current}/{total}] {label}", flush=True)
                        case = benchmark_case(
                            target,
                            workload,
                            algorithm,
                            outline,
                            layer,
                            args.view,
                            args.warmup,
                            args.repeat,
                            args.timeout_seconds,
                        )
                        cases.append(case)
                        wall = case["summary"]["wall_ms"]
                        print(
                            f"  wall p50={wall['p50']:.2f} ms p95={wall['p95']:.2f} ms; "
                            f"detail={case['counts']['detail_segments'] + case['counts']['detail_arcs']} "
                            f"outline={case['counts']['outline_segments'] + case['counts']['outline_arcs']}",
                            flush=True,
                        )

    dirty = _git_value("status", "--porcelain")
    report = {
        "schema": "geometer.fast_hlr.baseline.a0",
        "generated_utc": datetime.now(timezone.utc).isoformat(),
        "environment": {
            "platform": platform.platform(),
            "machine": platform.machine(),
            "processor": platform.processor(),
            "python": platform.python_version(),
            "runtimes": [
                {
                    "name": target["name"],
                    "command": target["command"],
                    "artifact": str(target["artifact"]),
                    "artifact_sha256": sha256_file(target["artifact"]),
                    **(
                        {
                            "host": str(target["host"]),
                            "host_sha256": sha256_file(target["host"]),
                        }
                        if "host" in target
                        else {}
                    ),
                }
                for target in targets
            ],
            "git_commit": _git_value("rev-parse", "HEAD"),
            "git_dirty": bool(dirty),
            "warmup": args.warmup,
            "repeat": args.repeat,
        },
        "measurement_scope": {
            "wall_ms": "process startup through completed projection output",
            "phase_ms": "timings reported by geometry.projection.b0",
            "prepared_view": False,
            "gpu": False,
        },
        "cases": cases,
        "runtime_comparisons": runtime_comparisons(cases),
    }
    output = args.output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

"""Compare governed V8_0_0 and V8_0_1 qualification evidence."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parent.parent
TAGS = ("V8_0_0", "V8_0_1")


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def report_for(root: Path, tag: str) -> dict[str, Any]:
    path = root / tag / "qualification-report.json"
    report = json.loads(path.read_text(encoding="utf-8"))
    if report.get("tag") != tag or report.get("status") != "passed":
        raise RuntimeError(f"qualification report is not a passing {tag} report: {path}")
    return report


def normalized_projection(path: Path) -> bytes:
    value = json.loads(path.read_text(encoding="utf-8"))
    timings = value.pop("timings", None)
    if not isinstance(timings, dict) or not timings:
        raise RuntimeError(f"projection is missing excluded runtime timings: {path}")
    return json.dumps(value, sort_keys=True, separators=(",", ":")).encode("utf-8")


def normalized_step(path: Path) -> bytes:
    text = path.read_text(encoding="utf-8").replace("\r\n", "\n")
    normalized, count = re.subn(
        r"(FILE_NAME\('Open CASCADE Shape Model',')[^']+(')",
        r"\1<timestamp>\2",
        text,
        count=1,
    )
    if count != 1:
        raise RuntimeError(f"planar STEP is missing its governed timestamp field: {path}")
    return normalized.encode("utf-8")


def require_equal(label: str, first: Any, second: Any) -> None:
    if first != second:
        raise RuntimeError(f"{label} differs: V8_0_0={first!r}, V8_0_1={second!r}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--qualification-root",
        type=Path,
        default=ROOT / "out/occt-qualification",
    )
    parser.add_argument("--output", type=Path, default=None)
    args = parser.parse_args()

    qualification_root = args.qualification_root.resolve()
    reports = {tag: report_for(qualification_root, tag) for tag in TAGS}
    baseline = reports["V8_0_0"]
    candidate = reports["V8_0_1"]
    require_equal(
        "Geometer revision", baseline["geometer_revision"], candidate["geometer_revision"]
    )
    require_equal(
        "feasibility digest",
        baseline["lanes"]["native"]["feasibility_sha256"],
        candidate["lanes"]["native"]["feasibility_sha256"],
    )
    for report in reports.values():
        if not report.get("feasibility_native_wasm_equal"):
            raise RuntimeError(f"{report['tag']} native/WASM feasibility output differs")

    native_roots = {
        tag: qualification_root / tag / "native-validation" for tag in TAGS
    }
    exact_native_outputs = (
        "SOT-23.glb",
        "SOT-23.top.outline.svg",
        "planar-step-request.json",
    )
    native_digests: dict[str, str] = {}
    for name in exact_native_outputs:
        values = {tag: (native_roots[tag] / name).read_bytes() for tag in TAGS}
        require_equal(f"native output {name}", values["V8_0_0"], values["V8_0_1"])
        native_digests[name] = sha256(values["V8_0_0"])

    projections = {
        tag: normalized_projection(native_roots[tag] / "SOT-23.projection.json") for tag in TAGS
    }
    require_equal("normalized HLR projection", projections["V8_0_0"], projections["V8_0_1"])
    native_digests["SOT-23.projection.normalized.json"] = sha256(projections["V8_0_0"])

    planar_steps = {tag: normalized_step(native_roots[tag] / "planar.step") for tag in TAGS}
    require_equal("normalized planar STEP", planar_steps["V8_0_0"], planar_steps["V8_0_1"])
    native_digests["planar.normalized.step"] = sha256(planar_steps["V8_0_0"])

    wasm_runtime = {
        tag: reports[tag]["lanes"]["wasm"]["consumer_validation"][
            "step_glb_runtime_memory"
        ]
        for tag in TAGS
    }
    require_equal("WASM STEP-to-GLB runtime/memory", wasm_runtime["V8_0_0"], wasm_runtime["V8_0_1"])
    browser_results = {
        tag: reports[tag]["lanes"]["wasm"]["consumer_validation"]["full_browser_hlr"]
        for tag in TAGS
    }
    require_equal("full-browser HLR", browser_results["V8_0_0"], browser_results["V8_0_1"])

    result = {
        "schema": "geometer.occt_qualification_comparison.a0",
        "status": "passed",
        "geometer_revision": baseline["geometer_revision"],
        "baseline": {
            "tag": baseline["tag"],
            "occt_revision": baseline["occt_revision"],
        },
        "candidate": {
            "tag": candidate["tag"],
            "occt_revision": candidate["occt_revision"],
            "occt_tag_object": candidate["occt_tag_object"],
        },
        "feasibility_sha256": baseline["lanes"]["native"]["feasibility_sha256"],
        "native_output_sha256": native_digests,
        "wasm_step_glb_runtime_memory": wasm_runtime["V8_0_0"],
        "full_browser_hlr": browser_results["V8_0_0"],
        "size_deltas": {
            "native_occt_install_bytes": (
                candidate["lanes"]["native"]["install_bytes"]
                - baseline["lanes"]["native"]["install_bytes"]
            ),
            "wasm_occt_install_bytes": (
                candidate["lanes"]["wasm"]["install_bytes"]
                - baseline["lanes"]["wasm"]["install_bytes"]
            ),
            "native_executable_bytes": (
                candidate["lanes"]["native"]["consumer_validation"]["executable"]["bytes"]
                - baseline["lanes"]["native"]["consumer_validation"]["executable"]["bytes"]
            ),
            "browser_wasm_bytes": (
                candidate["lanes"]["wasm"]["consumer_validation"]["browser_wasm_artifact"][
                    "bytes"
                ]
                - baseline["lanes"]["wasm"]["consumer_validation"]["browser_wasm_artifact"][
                    "bytes"
                ]
            ),
        },
        "production_pin_unchanged": True,
        "production_solver_allowed": False,
    }
    output = args.output or qualification_root / "comparison.json"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps(result, sort_keys=True, separators=(",", ":")))
    print(f"qualification comparison: {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

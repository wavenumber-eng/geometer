"""Qualify analytic GMABRQ01 packets through production executable IPC."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PYTHON_ROOT = ROOT / "python"
if str(PYTHON_ROOT) not in sys.path:
    sys.path.insert(0, str(PYTHON_ROOT))

from analytic_qualification.corpus import QualificationError, load_corpus  # noqa: E402
from analytic_qualification.runner import qualify  # noqa: E402
from analytic_qualification.telemetry import resolve_telemetry_helper  # noqa: E402
from geometer._ipc_client import GeometerIpcError  # noqa: E402
from geometer._paths import executable_path  # noqa: E402


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--corpus", type=Path, help="A0 corpus JSON; defaults to the governed synthetic request")
    parser.add_argument("--executable", type=Path, help="production Geometer executable")
    parser.add_argument("--telemetry-helper", type=Path, help="native internal solver telemetry helper")
    parser.add_argument("--output", type=Path, default=ROOT / "out/analytic-qualification/qualification-report.json")
    parser.add_argument("--warmup-count", type=int, default=1)
    parser.add_argument("--repeat-count", type=int, default=3)
    parser.add_argument("--power-mode", default="unrecorded")
    parser.add_argument(
        "--require-target",
        action="store_true",
        help="fail when the conservative 1 s/512 MiB process-envelope observation is missed",
    )
    parser.add_argument(
        "--require-solver-telemetry",
        action="store_true",
        help="fail closed unless production-equivalent internal solver telemetry passes",
    )
    parser.add_argument(
        "--require-promotion-attested",
        action="store_true",
        help="fail closed unless executable, clean source, toolchain, and OCCT provenance are attested",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    args = _parser().parse_args(argv)
    try:
        cases = load_corpus(None if args.corpus is None else args.corpus.resolve())
        executable = executable_path() if args.executable is None else args.executable.resolve()
        if not executable.is_file():
            raise QualificationError(f"production executable does not exist: {executable}")
        require_solver_telemetry = args.require_solver_telemetry or any(
            case.classification == "external_real_board" for case in cases
        )
        telemetry_helper = resolve_telemetry_helper(
            args.telemetry_helper, required=require_solver_telemetry
        )
        report = qualify(
            cases,
            executable,
            warmup_count=args.warmup_count,
            repeat_count=args.repeat_count,
            power_mode=args.power_mode,
            require_target=args.require_target,
            telemetry_helper=telemetry_helper,
            require_solver_telemetry=require_solver_telemetry,
        )
        output = args.output.resolve()
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print(f"analytic qualification report: {output}")
        print(f"qualification status: {report['qualification']['status']}")
        if args.require_promotion_attested:
            qualification = report["qualification"]
            if qualification["build_provenance_gate"] != "pass":
                print("analytic qualification failed: promotion-attested build provenance is required")
                return 1
            if qualification["real_board_evidence_present"] and (
                qualification["real_board_promotion_gate"] != "pass"
            ):
                print("analytic qualification failed: portable real-board promotion gates are required")
                return 1
        return 0 if report["qualification"]["status"] == "pass" else 1
    except (QualificationError, GeometerIpcError) as error:
        print(f"analytic qualification failed: {error}")
        return 2


if __name__ == "__main__":
    raise SystemExit(main())

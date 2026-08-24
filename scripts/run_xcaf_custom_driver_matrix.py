"""Run the isolated OCCT XCAF custom-attribute compatibility matrix."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import re
import shutil
import subprocess
import time
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
PROBE_SOURCE = ROOT / "tests" / "cpp" / "step_topology_xcaf_custom_driver_test.cpp"
PROBE_CMAKE_SOURCE = ROOT / "tests" / "cpp" / "occt_xcaf_custom_driver_matrix"
DEFAULT_OLD_OCCT = ROOT / ".deps" / "occt-qualification" / "7.9.3" / "native" / "windows-x64" / "occt-install" / "cmake"
DEFAULT_NEW_OCCT = ROOT / ".deps" / "native" / "windows-x64" / "occt-install" / "cmake"
DEFAULT_WORK_ROOT = ROOT / ".deps" / "xcaf-matrix" / "automated"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def display_path(path: Path) -> str:
    resolved = path.resolve()
    try:
        return resolved.relative_to(ROOT).as_posix()
    except ValueError:
        return f"<external>/{resolved.name}"


def display_argument(value: str) -> str:
    prefix = "-DOpenCASCADE_DIR="
    if value.startswith(prefix):
        return prefix + display_path(Path(value[len(prefix) :]))
    path = Path(value)
    return display_path(path) if path.is_absolute() else value


def run(command: list[str], *, capture: bool = False) -> tuple[str, float]:
    started = time.perf_counter()
    completed = subprocess.run(
        command,
        cwd=ROOT,
        check=True,
        text=True,
        stdout=subprocess.PIPE if capture else None,
        stderr=subprocess.STDOUT if capture else None,
    )
    elapsed_ms = (time.perf_counter() - started) * 1000.0
    return (completed.stdout.strip() if capture else "", elapsed_ms)


def cmake_generator_arguments() -> tuple[list[str], bool]:
    if os.name == "nt" and shutil.which("cl") is None:
        return ["-G", "Visual Studio 17 2022", "-A", "x64"], True
    if shutil.which("ninja") is not None:
        return ["-G", "Ninja"], False
    return [], os.name == "nt"


def compiler_identity(build_dir: Path) -> dict[str, str]:
    candidates = list((build_dir / "CMakeFiles").glob("*/CMakeCXXCompiler.cmake"))
    if len(candidates) != 1:
        raise RuntimeError(f"could not identify compiler metadata under {display_path(build_dir)}")
    text = candidates[0].read_text(encoding="utf-8")

    def cmake_value(name: str) -> str:
        match = re.search(rf'^set\({name} "([^"]+)"\)', text, re.MULTILINE)
        return match.group(1) if match else "unknown"

    return {
        "id": cmake_value("CMAKE_CXX_COMPILER_ID"),
        "version": cmake_value("CMAKE_CXX_COMPILER_VERSION"),
        "path": display_path(Path(cmake_value("CMAKE_CXX_COMPILER"))),
    }


def build_probe(label: str, occt_cmake_dir: Path, work_root: Path) -> tuple[Path, list[list[str]], dict[str, str]]:
    config = occt_cmake_dir / "OpenCASCADEConfig.cmake"
    if not config.is_file():
        raise FileNotFoundError(f"OCCT CMake package is missing: {display_path(config)}")
    build_dir = work_root / f"build-{label}"
    generator_args, multi_config = cmake_generator_arguments()
    configure = [
        "cmake",
        "-S",
        str(PROBE_CMAKE_SOURCE),
        "-B",
        str(build_dir),
        *generator_args,
        f"-DOpenCASCADE_DIR={occt_cmake_dir}",
        *([] if multi_config else ["-DCMAKE_BUILD_TYPE=Release"]),
    ]
    build = ["cmake", "--build", str(build_dir), "--config", "Release"]
    run(configure)
    run(build)
    executable = (
        build_dir
        / ("Release" if multi_config else "")
        / ("geometer_xcaf_custom_driver_matrix.exe" if os.name == "nt" else "geometer_xcaf_custom_driver_matrix")
    )
    if not executable.is_file():
        raise FileNotFoundError(f"matrix probe executable is missing: {display_path(executable)}")
    return executable, [configure, build], compiler_identity(build_dir)


def artifact_records(directory: Path) -> list[dict[str, object]]:
    records: list[dict[str, object]] = []
    for carrier, name in (
        ("xbf", "GeometerBinXCAFProbe.wnxbf"),
        ("xml-xcaf", "GeometerXmlXCAFProbe.wnxml"),
    ):
        path = directory / name
        records.append(
            {
                "carrier": carrier,
                "path": display_path(path),
                "bytes": path.stat().st_size,
                "sha256": sha256(path),
                "ocaf_storage_version": 12,
            }
        )
    return records


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Build identical probes against two OCCT installs and run both XBF/XML directions."
    )
    parser.add_argument("--old-occt-cmake", type=Path, default=DEFAULT_OLD_OCCT)
    parser.add_argument("--new-occt-cmake", type=Path, default=DEFAULT_NEW_OCCT)
    parser.add_argument("--work-root", type=Path, default=DEFAULT_WORK_ROOT)
    parser.add_argument("--report", type=Path, default=None)
    args = parser.parse_args()

    work_root = args.work_root.resolve()
    work_root.mkdir(parents=True, exist_ok=True)
    report_path = args.report.resolve() if args.report else work_root / "matrix-report.json"
    report_path.parent.mkdir(parents=True, exist_ok=True)

    old_probe, old_build_commands, old_compiler = build_probe("7.9.3", args.old_occt_cmake.resolve(), work_root)
    new_probe, new_build_commands, new_compiler = build_probe("8.0.1", args.new_occt_cmake.resolve(), work_root)
    old_version, _ = run([str(old_probe), "--version"], capture=True)
    new_version, _ = run([str(new_probe), "--version"], capture=True)
    if old_version != "7.9.3" or new_version != "8.0.1":
        raise RuntimeError(f"unexpected OCCT matrix versions: old={old_version!r}, new={new_version!r}")

    probes = {old_version: old_probe, new_version: new_probe}
    artifacts: dict[str, list[dict[str, object]]] = {}
    results: list[dict[str, object]] = []
    command_provenance = [*old_build_commands, *new_build_commands]
    missing_driver_results: list[dict[str, object]] = []
    for version in (old_version, new_version):
        command = [str(probes[version])]
        output, elapsed_ms = run(command, capture=True)
        command_provenance.append(command)
        if "custom binary/XML attribute driver probe passed" not in output:
            raise RuntimeError(f"OCCT {version} missing-driver probe did not report success")
        missing_driver_results.append(
            {
                "occt": version,
                "carriers": ["xbf", "xml-xcaf"],
                "document_open": "success",
                "custom_attribute": "omitted",
                "result": "expected-readable-loss",
                "wall_time_milliseconds": round(elapsed_ms, 3),
            }
        )
    for writer_version in (old_version, new_version):
        artifact_directory = work_root / "artifacts" / f"{writer_version}-write"
        artifact_directory.mkdir(parents=True, exist_ok=True)
        write_command = [str(probes[writer_version]), "--write-matrix", str(artifact_directory)]
        _, write_ms = run(write_command, capture=True)
        command_provenance.append(write_command)
        artifacts[writer_version] = artifact_records(artifact_directory)
        for reader_version in (writer_version, new_version if writer_version == old_version else old_version):
            read_command = [str(probes[reader_version]), "--read-matrix", str(artifact_directory)]
            _, read_ms = run(read_command, capture=True)
            command_provenance.append(read_command)
            results.append(
                {
                    "writer_occt": writer_version,
                    "reader_occt": reader_version,
                    "carriers": ["xbf", "xml-xcaf"],
                    "result": "pass",
                    "write_wall_time_milliseconds": round(write_ms, 3),
                    "read_wall_time_milliseconds": round(read_ms, 3),
                }
            )

    report = {
        "research_format": "geometer.xcaf-custom-driver-matrix.a0",
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "host": {"system": platform.system(), "machine": platform.machine()},
        "observed_build_environment": {
            "note": "Compiler identity and host architecture are measured; linkage and runtime-library posture are not inferred.",
            "old": old_compiler,
            "new": new_compiler,
        },
        "probe_source": display_path(PROBE_SOURCE),
        "probe_source_sha256": sha256(PROBE_SOURCE),
        "attribute_guid": "eacfa1c8-42b2-4b9e-9d69-e3f050eaf8a1",
        "xml_namespace": "https://wavenumber.com/ns/geometer/research/ocaf/a0",
        "versions": {
            "old": {
                "observed_runtime_version": old_version,
                "expected_source_identity": {
                    "tag": "V7_9_3",
                    "commit": "a016080bf6738d6aeae020badee4e888ad1540a5",
                    "validated_by_harness": False,
                },
            },
            "new": {
                "observed_runtime_version": new_version,
                "expected_source_identity": {
                    "tag": "V8_0_1",
                    "commit": "b8f597c677811d1f9f4d8a97f5ae2825c0353a42",
                    "validated_by_harness": False,
                },
            },
        },
        "artifacts": artifacts,
        "results": results,
        "missing_retrieval_driver_results": missing_driver_results,
        "command_provenance": [[display_argument(value) for value in command] for command in command_provenance],
    }
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"XCAF custom-driver compatibility matrix passed: {display_path(report_path)}")


if __name__ == "__main__":
    main()

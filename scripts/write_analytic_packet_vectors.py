from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
VECTOR_ROOT = ROOT / "tests" / "contracts" / "vectors" / "analytic"
CROSS_TRANSPORT_REQUEST = "cross-transport-primitive-family-self-request.hex"
CROSS_TRANSPORT_RESULT = "cross-transport-primitive-family-self-result.hex"


def _arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Write the governed analytic A0 packet vectors")
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build")
    parser.add_argument("--check", action="store_true")
    return parser.parse_args()


def _producer(build_dir: Path, stem: str) -> Path:
    suffix = ".exe" if sys.platform == "win32" else ""
    path = build_dir / "tests" / "cpp" / f"{stem}{suffix}"
    if not path.is_file():
        raise FileNotFoundError(f"analytic vector producer is missing: {path}")
    return path


def _run(path: Path) -> dict[str, str]:
    completed = subprocess.run(
        [str(path)],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
        timeout=30,
    )
    values: dict[str, str] = {}
    for line in completed.stdout.splitlines():
        if "=" not in line:
            continue
        name, value = line.split("=", 1)
        values[name] = value.strip()
    return values


def _decode_hex(value: str, label: str) -> bytes:
    if not value or len(value) % 2 or value.lower() != value:
        raise ValueError(f"{label} is not canonical lowercase even-length hex")
    try:
        return bytes.fromhex(value)
    except ValueError as error:
        raise ValueError(f"{label} is not valid hex") from error


def _read_hex_vector(filename: str) -> bytes:
    path = VECTOR_ROOT / filename
    if not path.is_file():
        raise FileNotFoundError(f"analytic cross-transport vector is missing: {path}")
    canonical = "".join(path.read_text(encoding="ascii").split())
    return _decode_hex(canonical, filename)


def _cross_transport_parity() -> dict[str, object]:
    sys.path.insert(0, str(ROOT / "python"))
    from geometer._analytic_packet_a0 import (  # noqa: PLC0415
        decode_analytic_planar_boolean_batch_result_a0_packet,
    )

    request = _read_hex_vector(CROSS_TRANSPORT_REQUEST)
    result = _read_hex_vector(CROSS_TRANSPORT_RESULT)
    if request[:8] != b"GMABRQ01" or result[:8] != b"GMABRS01":
        raise ValueError("analytic cross-transport vectors use unexpected packet magic")
    decoded = decode_analytic_planar_boolean_batch_result_a0_packet(result)
    if len(decoded.job_results) != 1 or len(decoded.relationship_results) != 1:
        raise ValueError("analytic cross-transport result must contain one job and one relationship")
    job = decoded.job_results[0]
    relationship = decoded.relationship_results[0]
    if job.job_id != 3 or relationship.query_id != 30001:
        raise ValueError("analytic cross-transport result identity drifted")
    return {
        "id": "analytic_primitive_family_self_query",
        "vector_count": 2,
        "source_fixture": "tests/fixtures/analytic_planar_boolean/matz_observations_a0.json",
        "source_fixture_id": "analytic_primitive_family",
        "comparison": "exact_bytes",
        "runtimes": ["native_executable_ipc", "browser_wasm_generic_c_abi"],
        "request_file": CROSS_TRANSPORT_REQUEST,
        "request_bytes": len(request),
        "request_sha256": hashlib.sha256(request).hexdigest(),
        "result_file": CROSS_TRANSPORT_RESULT,
        "result_bytes": len(result),
        "result_sha256": hashlib.sha256(result).hexdigest(),
        "job_id": job.job_id,
        "standalone_job_result_sha256": job.digest_sha256,
        "self_query_id": relationship.query_id,
        "self_query_expected_status": relationship.status.value,
        "self_query_expected_dimension": relationship.aggregate_dimension.value,
        "validator": "tests/wasm/test_analytic_cross_transport_parity.py",
        "wasm_runner": "tests/wasm/analytic_cross_transport_runner.js",
    }


def _render(build_dir: Path) -> dict[Path, bytes]:
    request = _run(_producer(build_dir, "geometer_analytic_request_packet_test"))
    result = _run(_producer(build_dir, "geometer_analytic_result_packet_records_test"))
    definitions = [
        ("request.empty", request, "ANALYTIC_REQUEST_PACKET_EMPTY_VECTOR", "request-empty.hex"),
        (
            "request.exemplar",
            request,
            "ANALYTIC_REQUEST_PACKET_EXEMPLAR_VECTOR",
            "request-exemplar.hex",
        ),
        (
            "result.canonical-mixed",
            result,
            "ANALYTIC_RESULT_PACKET_CANONICAL_VECTOR",
            "result-canonical-mixed.hex",
        ),
        (
            "result.success-standalone",
            result,
            "ANALYTIC_RESULT_PACKET_STANDALONE_VECTOR",
            "result-success-standalone.hex",
        ),
        (
            "result.mixed-success-standalone",
            result,
            "ANALYTIC_RESULT_PACKET_MIXED_SUCCESS_STANDALONE_VECTOR",
            "result-mixed-success-standalone.hex",
        ),
        (
            "result.failure-standalone",
            result,
            "ANALYTIC_RESULT_PACKET_FAILED_STANDALONE_VECTOR",
            "result-failure-standalone.hex",
        ),
    ]
    files: dict[Path, bytes] = {}
    vectors: list[dict[str, object]] = []
    for identity, source, key, filename in definitions:
        if key not in source:
            raise RuntimeError(f"{key} is absent from its native vector producer")
        data = _decode_hex(source[key], key)
        files[VECTOR_ROOT / filename] = (data.hex() + "\n").encode("ascii")
        vectors.append(
            {
                "id": identity,
                "file": filename,
                "bytes": len(data),
                "sha256": hashlib.sha256(data).hexdigest(),
                "expected": "accept",
                "comparison": "exact_bytes",
            }
        )
    manifest = {
        "manifest_identity": "wn.geometer.analytic_packet_vectors",
        "generation": "a0",
        "encoding": "lowercase_hex",
        "vectors": vectors,
        "job_digests": {
            "success_standalone": result["ANALYTIC_RESULT_PACKET_STANDALONE_DIGEST"],
            "success_in_mixed_batch": result["ANALYTIC_RESULT_PACKET_MIXED_SUCCESS_STANDALONE_DIGEST"],
            "failure_standalone": result["ANALYTIC_RESULT_PACKET_FAILED_STANDALONE_DIGEST"],
        },
        "cross_transport_parity": _cross_transport_parity(),
        "mutation_requirements": [
            "bad_magic",
            "truncated_payload",
            "nonzero_reserved_or_padding",
            "duplicate_job_id",
            "noncanonical_fragment_order",
            "invalid_authored_source_occurrence",
        ],
    }
    files[VECTOR_ROOT / "manifest.json"] = (json.dumps(manifest, indent=2, sort_keys=False) + "\n").encode("utf-8")
    return files


def main() -> int:
    args = _arguments()
    rendered = _render(args.build_dir.resolve())
    stale: list[Path] = []
    for path, expected in rendered.items():
        if args.check:
            if not path.is_file() or path.read_bytes() != expected:
                stale.append(path)
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(expected)
    if stale:
        names = ", ".join(str(path.relative_to(ROOT)) for path in stale)
        raise SystemExit(f"analytic packet vectors are stale: {names}")
    print(f"{'Checked' if args.check else 'Wrote'} {len(rendered) - 1} analytic packet vectors.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

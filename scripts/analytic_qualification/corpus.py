"""Strict qualification-corpus loading and GMABRQ01 accounting."""

from __future__ import annotations

import hashlib
import json
import re
import struct
from dataclasses import dataclass
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
CORPUS_SCHEMA = "wn.geometer.analytic_planar_boolean_qualification_corpus.a0"
DEFAULT_CASE_ID = "cross_transport_primitive_family_self_query"
DEFAULT_REQUEST = ROOT / "tests/contracts/vectors/analytic/cross-transport-primitive-family-self-request.hex"
DEFAULT_RESULT_SHA256 = "7947d3850dafcd6d897aba2f4f6353d5afb61892349247c436a6e44f370ba183"
CLASSIFICATIONS = frozenset({"synthetic", "external_real_board"})
PROVENANCE_FIELDS = frozenset(
    {
        "source_identity",
        "source_sha256",
        "exporter_identity",
        "exporter_revision",
        "redistribution_authorization",
        "license_scope",
    }
)
HEX_64 = re.compile(r"[0-9a-f]{64}\Z")


class QualificationError(RuntimeError):
    """The qualification input or production replay is invalid."""


@dataclass(frozen=True, slots=True)
class QualificationCase:
    case_id: str
    request_path: Path
    classification: str
    expected_result_sha256: str | None
    source_provenance: dict[str, str] | None


def canonical_bytes(value: Any) -> bytes:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode("utf-8")


def identity_sha256(value: Any) -> str:
    return hashlib.sha256(canonical_bytes(value)).hexdigest()


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def load_hex_packet(path: Path) -> bytes:
    if path.suffix != ".hex":
        raise QualificationError(f"lowercase-hex request packet must use .hex: {path}")
    try:
        text = "".join(path.read_text(encoding="ascii").split())
        packet = bytes.fromhex(text)
    except (OSError, UnicodeError, ValueError) as error:
        raise QualificationError(f"could not read lowercase-hex packet {path}: {error}") from error
    if text != text.lower() or any(character not in "0123456789abcdef" for character in text):
        raise QualificationError(f"packet {path} must use canonical lowercase hex")
    return packet


def load_raw_packet(path: Path) -> bytes:
    if path.suffix != ".gmabrq01":
        raise QualificationError(f"raw request packet must use .gmabrq01: {path}")
    try:
        return path.read_bytes()
    except OSError as error:
        raise QualificationError(f"could not read raw request packet {path}: {error}") from error


def load_request_packet(path: Path) -> bytes:
    if path.suffix == ".hex":
        packet = load_hex_packet(path)
    elif path.suffix == ".gmabrq01":
        packet = load_raw_packet(path)
    else:
        raise QualificationError(f"request packet {path} must use canonical .hex or raw .gmabrq01")
    if not packet.startswith(b"GMABRQ01"):
        raise QualificationError(f"request packet {path} does not begin with GMABRQ01")
    request_accounting(packet)
    return packet


def request_accounting(packet: bytes) -> dict[str, int]:
    sizes = (24, 32, 24, 32, 4, 32, 24, 40, 32, 40, 48, 32, 24)
    if len(packet) < 64 + len(sizes) * 32 or packet[:8] != b"GMABRQ01":
        raise QualificationError("request does not have a complete GMABRQ01 header and directory")
    generation, header_bytes, flags, total, directory, entries, jobs, relationships, reserved, payload, reserved64 = (
        struct.unpack_from("<HHIQQIIIIQQ", packet, 8)
    )
    if (generation, header_bytes, flags, total, directory, entries, reserved, reserved64) != (
        1,
        64,
        0,
        len(packet),
        64,
        13,
        0,
        0,
    ):
        raise QualificationError("request header is not canonical A0")
    counts: list[int] = []
    payload_actual = 0
    cursor = (64 + len(sizes) * 32 + 7) & ~7
    for index, record_size in enumerate(sizes):
        kind, version, encoded_size, offset, byte_length, count = struct.unpack_from("<HHIQQQ", packet, 64 + index * 32)
        if (
            kind != index + 1
            or version != 1
            or encoded_size != record_size
            or byte_length != count * record_size
            or offset != cursor
            or offset + byte_length > len(packet)
        ):
            raise QualificationError("request table directory is not canonical A0")
        counts.append(count)
        payload_actual += byte_length
        end = offset + byte_length
        cursor = end if index + 1 == len(sizes) else (end + 7) & ~7
        if any(packet[end:cursor]):
            raise QualificationError("request table alignment padding is nonzero")
    if cursor != len(packet) or payload != payload_actual or counts[0] != jobs or counts[12] != relationships:
        raise QualificationError("request packet accounting is inconsistent")
    return {
        "job_count": jobs,
        "stage_count": counts[1],
        "operand_count": counts[2],
        "planar_region_count": counts[3],
        "ring_count": counts[5],
        "input_segment_count": counts[7],
        "disk_count": counts[8],
        "annulus_count": counts[9],
        "capsule_count": counts[10],
        "swept_path_count": counts[11],
        "relationship_query_count": relationships,
    }


def _required_text(provenance: dict[str, Any], field: str, case_id: str) -> str:
    value = provenance.get(field)
    if not isinstance(value, str) or not value.strip():
        raise QualificationError(f"external real-board case {case_id!r} requires nonempty {field}")
    return value


def _real_board_provenance(value: Any, case_id: str) -> dict[str, str]:
    if not isinstance(value, dict) or set(value) != PROVENANCE_FIELDS:
        raise QualificationError(
            f"external real-board case {case_id!r} requires the exact source/exporter/authorization provenance fields"
        )
    provenance = {field: _required_text(value, field, case_id) for field in PROVENANCE_FIELDS}
    if HEX_64.fullmatch(provenance["source_sha256"]) is None:
        raise QualificationError(f"external real-board case {case_id!r} requires a lowercase source_sha256")
    if provenance["redistribution_authorization"] != "authorized_for_qualification":
        raise QualificationError(
            f"external real-board case {case_id!r} must explicitly authorize redistribution for qualification"
        )
    return provenance


def _case_from_json(base: Path, value: Any) -> QualificationCase:
    if not isinstance(value, dict):
        raise QualificationError("each corpus case must be an object")
    allowed = {"id", "request", "classification", "expected_result_sha256", "source_provenance"}
    if set(value) - allowed:
        raise QualificationError("corpus case has unknown fields")
    case_id = value.get("id")
    request = value.get("request")
    classification = value.get("classification")
    expected = value.get("expected_result_sha256")
    if not isinstance(case_id, str) or not case_id or not isinstance(request, str) or not request:
        raise QualificationError("corpus case id and request must be nonempty strings")
    if classification not in CLASSIFICATIONS:
        raise QualificationError(f"corpus case {case_id!r} has an unsupported classification")
    if expected is not None and (not isinstance(expected, str) or HEX_64.fullmatch(expected) is None):
        raise QualificationError(f"corpus case {case_id!r} has an invalid expected result SHA-256")
    raw_provenance = value.get("source_provenance")
    provenance = _real_board_provenance(raw_provenance, case_id) if classification == "external_real_board" else None
    if classification == "synthetic" and raw_provenance is not None:
        raise QualificationError(f"synthetic case {case_id!r} must not claim external source provenance")
    request_path = Path(request)
    if request_path.is_absolute():
        raise QualificationError(f"corpus request must be relative to its corpus: {request}")
    path = (base / request_path).resolve()
    try:
        path.relative_to(base.resolve())
    except ValueError as error:
        raise QualificationError(f"corpus request escapes its corpus directory: {request}") from error
    if path.suffix not in {".hex", ".gmabrq01"}:
        raise QualificationError(f"corpus request must use canonical .hex or raw .gmabrq01: {request}")
    if not path.is_file():
        raise QualificationError(f"corpus request does not exist: {path}")
    return QualificationCase(case_id, path, classification, expected, provenance)


def load_corpus(path: Path | None) -> list[QualificationCase]:
    if path is None:
        return [QualificationCase(DEFAULT_CASE_ID, DEFAULT_REQUEST, "synthetic", DEFAULT_RESULT_SHA256, None)]
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise QualificationError(f"could not read corpus manifest {path}: {error}") from error
    if not isinstance(value, dict) or set(value) != {"schema", "cases"} or value.get("schema") != CORPUS_SCHEMA:
        raise QualificationError("corpus manifest does not have the exact A0 envelope")
    raw_cases = value.get("cases")
    if not isinstance(raw_cases, list) or not raw_cases:
        raise QualificationError("corpus manifest must contain at least one case")
    cases = [_case_from_json(path.parent, item) for item in raw_cases]
    if len({case.case_id for case in cases}) != len(cases):
        raise QualificationError("corpus case ids must be unique")
    return cases

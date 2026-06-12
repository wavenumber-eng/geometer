"""Shared fixtures for the design-intent regression suite.

Run from the prototype folder:
    uv run pytest tests/ -q
"""

from __future__ import annotations

import sys
from pathlib import Path

import pytest

PROTO = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PROTO))


@pytest.fixture(scope="session")
def proto() -> Path:
    return PROTO


@pytest.fixture(scope="session")
def fixtures_dir() -> Path:
    return PROTO.parents[1] / "tests" / "fixtures" / "step" / "embedded_models"


def make_fake_step(payload_hint: str = "fake") -> str:
    """A minimal STEP text satisfying inject_metadata()'s anchors:
    a PRODUCT_DEFINITION, a GEOMETRIC_REPRESENTATION_CONTEXT, and ENDSEC."""
    return (
        "ISO-10303-21;\n"
        "HEADER;\n"
        f"FILE_DESCRIPTION(('{payload_hint}'),'2;1');\n"
        "FILE_NAME('part.step','2026-01-01T00:00:00',(''),(''),'','','');\n"
        "FILE_SCHEMA(('AP242_MANAGED_MODEL_BASED_3D_ENGINEERING_MIM_LF'));\n"
        "ENDSEC;\n"
        "DATA;\n"
        "#1=PRODUCT('P','part','',(#5));\n"
        "#2=PRODUCT_DEFINITION('design','',#3,#4);\n"
        "#6=(GEOMETRIC_REPRESENTATION_CONTEXT(3)GLOBAL_UNCERTAINTY_ASSIGNED_CONTEXT"
        "((#7))GLOBAL_UNIT_ASSIGNED_CONTEXT((#8,#9))REPRESENTATION_CONTEXT('',''));\n"
        "#10=SHAPE_REPRESENTATION('',(#11),#6);\n"
        "ENDSEC;\n"
        "END-ISO-10303-21;\n"
    )


@pytest.fixture
def fake_step_path(tmp_path: Path) -> Path:
    path = tmp_path / "part.step"
    path.write_text(make_fake_step(), encoding="utf-8")
    return path


def make_synthetic_payload(tag: bytes, size: int = 4000) -> bytes:
    """Deterministic STEP-looking bytes for chain-identity tests (the bake
    layer is payload-agnostic; vendor open only sniffs the header)."""
    body = (tag * (size // len(tag) + 1))[:size]
    return b"ISO-10303-21;\nHEADER;\n" + body + b"\nEND-ISO-10303-21;\n"

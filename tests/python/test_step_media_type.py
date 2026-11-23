from __future__ import annotations

import json
from pathlib import Path

import pytest

from geometer._generated.contracts.models import IpcAttachmentDeclarationA0
from geometer._ipc_a0 import Attachment
from geometer._ipc_client import (
    GeometerIpcProtocolError,
    _normalize_step_inputs,
    _validate_declared_attachments,
)
from geometer._step_media_type import canonical_step_media_type

CASES = json.loads((Path(__file__).resolve().parents[1] / "fixtures/step_media_types.json").read_text())["cases"]


@pytest.mark.parametrize("case", CASES, ids=[case["value"] for case in CASES])
def test_shared_complete_step_mime_fields(case: dict[str, object]) -> None:
    assert canonical_step_media_type(str(case["value"])) == ("model/step" if case["accepted"] else None)


def declaration(name: str, media: tuple[str, ...]) -> IpcAttachmentDeclarationA0:
    return IpcAttachmentDeclarationA0(name=name, media_types=media, required=True, max_bytes=256)


def test_step_input_normalization_preserves_bytes_and_caller_category() -> None:
    data = b"ISO-10303-21;\x00\xff"
    original = Attachment("model", '(source) MODEL/STEP; kicad-type="other"', data)
    declared = (declaration("model", ("application/step", "model/step")),)
    normalized = _normalize_step_inputs((original,), declared)
    _validate_declared_attachments(normalized, declared, "request")
    assert normalized[0].media_type == "model/step"
    assert normalized[0].data is data
    assert original.media_type == '(source) MODEL/STEP; kicad-type="other"'
    with pytest.raises(GeometerIpcProtocolError, match="invalid media"):
        _normalize_step_inputs((Attachment("model", 'model/step; name="unterminated', data),), declared)


def test_output_and_non_step_vendor_types_keep_exact_matching() -> None:
    step = (declaration("model", ("application/step", "model/step")),)
    with pytest.raises(GeometerIpcProtocolError, match="incompatible media"):
        _validate_declared_attachments(
            (Attachment("model", "MODEL/STEP; category=other", b"a"),), step, "response"
        )
    vendor = (declaration("mesh", ("application/vnd.wavenumber.geometer.mesh-collection+json",)),)
    original = Attachment(
        "mesh", "application/vnd.wavenumber.geometer.mesh-collection+json; extra=yes", b"b"
    )
    assert _normalize_step_inputs((original,), vendor)[0] is original
    with pytest.raises(GeometerIpcProtocolError, match="incompatible media"):
        _validate_declared_attachments((original,), vendor, "request")


def test_field_byte_limit_and_frame_controls_remain_enforced() -> None:
    prefix = 'model/step; x="'
    full = prefix + "x" * (128 - len(prefix) - 1) + '"'
    assert len(full) == 128
    assert canonical_step_media_type(full) == "model/step"
    assert canonical_step_media_type(full + " ") is None
    assert canonical_step_media_type("model/step\x7f") is None


def test_mixed_step_input_keeps_only_exact_vendor_alternative() -> None:
    vendor = "application/vnd.opencascade.xbf"
    declared = (declaration("input", ("application/step", "model/step", vendor)),)
    original = Attachment("input", vendor, b"exact vendor bytes")
    assert _normalize_step_inputs((original,), declared)[0] is original
    _validate_declared_attachments((original,), declared, "request")
    for altered in [vendor.upper(), vendor + "; category=other", "application/octet-stream"]:
        with pytest.raises(GeometerIpcProtocolError, match="invalid media"):
            _normalize_step_inputs((Attachment("input", altered, original.data),), declared)
    (step,) = _normalize_step_inputs(
        (Attachment("input", "Application/Step; category=other", b"step"),), declared
    )
    assert step.media_type == "model/step"


def test_legacy_only_request_descriptor_accepts_canonical_step_but_responses_stay_exact() -> None:
    declared = (declaration("model", ("application/step",)),)
    original = Attachment("model", 'MODEL/STEP; name="old;peer.step"', b"unchanged bytes")
    normalized = _normalize_step_inputs((original,), declared)
    assert normalized[0].media_type == "model/step"
    assert normalized[0].data is original.data
    _validate_declared_attachments(normalized, declared, "request")
    _validate_declared_attachments((original,), declared, "request")
    with pytest.raises(GeometerIpcProtocolError, match="incompatible media"):
        _validate_declared_attachments(normalized, declared, "response")
    _validate_declared_attachments((Attachment("model", "application/step", original.data),), declared, "response")


def test_model_attachment_producer_uses_canonical_mime() -> None:
    from geometer._generated.contracts.models import ModelAttachmentIllustrationSourceA0
    from geometer._model_illustration import _attachments

    data = b"STEP bytes"
    (attachment,) = _attachments(ModelAttachmentIllustrationSourceA0(kind="model", attachment="model"), data)
    assert attachment.media_type == "model/step"
    assert attachment.data is data

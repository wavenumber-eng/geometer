"""Synchronous executable IPC A0 frame encoding and decoding."""

from __future__ import annotations

import struct
from dataclasses import dataclass
from enum import IntEnum
from typing import BinaryIO


HEADER_SIZE = 48
MAX_JSON_BYTES = 8 * 1024 * 1024
MAX_ATTACHMENT_COUNT = 16
MAX_ATTACHMENT_TEXT_BYTES = 128
MAX_ATTACHMENT_BYTES = 256 * 1024 * 1024
MAX_FRAME_BYTES = 512 * 1024 * 1024

_MAGIC = b"GMIPCA01"
_HEADER = struct.Struct("<8sHHHHQIIQII")
_ATTACHMENT_HEADER = struct.Struct("<HHIQ")


class FrameKind(IntEnum):
    HELLO = 1
    WELCOME = 2
    REQUEST = 3
    RESPONSE = 4
    CANCEL = 5
    CANCELLED = 6
    CANCEL_REJECTED = 7
    SHUTDOWN = 8
    SHUTDOWN_ACK = 9
    PROTOCOL_ERROR = 10


@dataclass(frozen=True, slots=True)
class Attachment:
    name: str
    media_type: str
    data: bytes


@dataclass(frozen=True, slots=True)
class Frame:
    kind: FrameKind
    request_id: int
    json: bytes
    attachments: tuple[Attachment, ...] = ()


class IpcFrameError(RuntimeError):
    """An executable IPC frame violates the governed A0 format."""


def encoded_size(frame: Frame) -> int:
    attachment_bytes = sum(_attachment_size(item) for item in frame.attachments)
    return HEADER_SIZE + len(frame.json) + attachment_bytes


def write_frame(stream: BinaryIO, frame: Frame) -> None:
    """Validate, write, and flush one complete IPC frame."""

    encoded_attachments = _validate_outgoing(frame)
    attachment_bytes = sum(
        _ATTACHMENT_HEADER.size + len(name) + len(media_type) + len(attachment.data)
        for attachment, name, media_type in encoded_attachments
    )
    header = _HEADER.pack(
        _MAGIC,
        HEADER_SIZE,
        0,
        int(frame.kind),
        0,
        frame.request_id,
        len(frame.json),
        len(frame.attachments),
        attachment_bytes,
        0,
        0,
    )
    _write_all(stream, header)
    _write_all(stream, frame.json)
    for attachment, name, media_type in encoded_attachments:
        _write_all(
            stream,
            _ATTACHMENT_HEADER.pack(len(name), len(media_type), 0, len(attachment.data)),
        )
        _write_all(stream, name)
        _write_all(stream, media_type)
        _write_all(stream, attachment.data)
    stream.flush()


def read_frame(
    stream: BinaryIO,
    *,
    max_json_bytes: int = MAX_JSON_BYTES,
    max_attachment_count: int = MAX_ATTACHMENT_COUNT,
    max_attachment_name_bytes: int = MAX_ATTACHMENT_TEXT_BYTES,
    max_attachment_media_type_bytes: int = MAX_ATTACHMENT_TEXT_BYTES,
    max_attachment_bytes: int = MAX_ATTACHMENT_BYTES,
    max_frame_bytes: int = MAX_FRAME_BYTES,
) -> Frame | None:
    """Read one complete IPC frame, returning ``None`` only at a clean EOF."""

    first = stream.read(1)
    if first == b"":
        return None
    if first is None:
        raise IpcFrameError("IPC stream returned no data without reaching EOF")
    header = first + _read_exact(stream, HEADER_SIZE - 1, "frame header")
    (
        magic,
        header_size,
        generation,
        raw_kind,
        flags,
        request_id,
        json_size,
        attachment_count,
        attachment_bytes,
        reserved0,
        reserved1,
    ) = _HEADER.unpack(header)
    if magic != _MAGIC:
        raise IpcFrameError("IPC frame magic is invalid")
    if header_size != HEADER_SIZE or generation != 0 or flags != 0 or reserved0 != 0 or reserved1 != 0:
        raise IpcFrameError("IPC frame header contains an unsupported or reserved value")
    try:
        kind = FrameKind(raw_kind)
    except ValueError as error:
        raise IpcFrameError("IPC frame kind is unknown") from error
    if json_size == 0 or json_size > max_json_bytes:
        raise IpcFrameError("IPC JSON section exceeds the A0 limit")
    if attachment_count > max_attachment_count or attachment_bytes > max_frame_bytes:
        raise IpcFrameError("IPC attachment section exceeds the A0 limit")
    complete_size = HEADER_SIZE + json_size + attachment_bytes
    if complete_size > max_frame_bytes:
        raise IpcFrameError("IPC complete frame exceeds the A0 limit")
    _validate_kind_and_request_id(kind, request_id)
    if kind not in {FrameKind.REQUEST, FrameKind.RESPONSE} and (attachment_count or attachment_bytes):
        raise IpcFrameError("IPC control frames cannot carry attachments")

    payload = _read_exact(stream, json_size + attachment_bytes, "frame payload")
    json_bytes = payload[:json_size]
    offset = json_size
    names: set[str] = set()
    attachments: list[Attachment] = []
    for _ in range(attachment_count):
        if len(payload) - offset < _ATTACHMENT_HEADER.size:
            raise IpcFrameError("IPC attachment header is truncated")
        name_size, media_type_size, attachment_flags, data_size = _ATTACHMENT_HEADER.unpack_from(payload, offset)
        offset += _ATTACHMENT_HEADER.size
        if (
            name_size > max_attachment_name_bytes
            or media_type_size > max_attachment_media_type_bytes
            or data_size > max_attachment_bytes
            or attachment_flags != 0
        ):
            raise IpcFrameError("IPC attachment header exceeds an A0 limit")
        section_size = name_size + media_type_size + data_size
        if section_size > len(payload) - offset:
            raise IpcFrameError("IPC attachment exceeds its declared section")
        name = _decode_text(payload[offset : offset + name_size], "attachment name")
        offset += name_size
        media_type = _decode_text(payload[offset : offset + media_type_size], "attachment media type")
        offset += media_type_size
        if not name or not media_type or name in names:
            raise IpcFrameError("IPC attachment metadata is empty or duplicated")
        names.add(name)
        data = bytes(payload[offset : offset + data_size])
        offset += data_size
        attachments.append(Attachment(name=name, media_type=media_type, data=data))
    if offset != len(payload) or offset - json_size != attachment_bytes:
        raise IpcFrameError("IPC attachment byte total does not match its sections")
    return Frame(kind=kind, request_id=request_id, json=json_bytes, attachments=tuple(attachments))


def _validate_outgoing(frame: Frame) -> list[tuple[Attachment, bytes, bytes]]:
    if not isinstance(frame.kind, FrameKind):
        raise IpcFrameError("IPC frame kind is invalid")
    if not isinstance(frame.request_id, int) or isinstance(frame.request_id, bool):
        raise IpcFrameError("IPC request id must be an unsigned integer")
    if not 0 <= frame.request_id <= (1 << 64) - 1:
        raise IpcFrameError("IPC request id exceeds uint64")
    if not isinstance(frame.json, bytes) or not frame.json or len(frame.json) > MAX_JSON_BYTES:
        raise IpcFrameError("IPC JSON section exceeds the A0 limit")
    if len(frame.attachments) > MAX_ATTACHMENT_COUNT:
        raise IpcFrameError("IPC attachment count exceeds the A0 limit")
    _validate_kind_and_request_id(frame.kind, frame.request_id)
    if frame.kind not in {FrameKind.REQUEST, FrameKind.RESPONSE} and frame.attachments:
        raise IpcFrameError("IPC control frames cannot carry attachments")

    names: set[str] = set()
    encoded: list[tuple[Attachment, bytes, bytes]] = []
    for attachment in frame.attachments:
        if not isinstance(attachment.data, bytes):
            raise IpcFrameError("IPC attachment data must be bytes")
        name = _encode_text(attachment.name, "attachment name")
        media_type = _encode_text(attachment.media_type, "attachment media type")
        if not name or not media_type or attachment.name in names:
            raise IpcFrameError("IPC attachment metadata is empty or duplicated")
        if (
            len(name) > MAX_ATTACHMENT_TEXT_BYTES
            or len(media_type) > MAX_ATTACHMENT_TEXT_BYTES
            or len(attachment.data) > MAX_ATTACHMENT_BYTES
        ):
            raise IpcFrameError("IPC attachment exceeds an A0 limit")
        names.add(attachment.name)
        encoded.append((attachment, name, media_type))
    if encoded_size(frame) > MAX_FRAME_BYTES:
        raise IpcFrameError("IPC complete frame exceeds the A0 limit")
    return encoded


def _validate_kind_and_request_id(kind: FrameKind, request_id: int) -> None:
    zero_id_kinds = {
        FrameKind.HELLO,
        FrameKind.WELCOME,
        FrameKind.SHUTDOWN,
        FrameKind.SHUTDOWN_ACK,
    }
    nonzero_id_kinds = {
        FrameKind.REQUEST,
        FrameKind.RESPONSE,
        FrameKind.CANCEL,
        FrameKind.CANCELLED,
        FrameKind.CANCEL_REJECTED,
    }
    if kind in zero_id_kinds and request_id != 0:
        raise IpcFrameError(f"IPC {kind.name.lower()} frame requires request id zero")
    if kind in nonzero_id_kinds and request_id == 0:
        raise IpcFrameError(f"IPC {kind.name.lower()} frame requires a nonzero request id")


def _attachment_size(attachment: Attachment) -> int:
    return (
        _ATTACHMENT_HEADER.size
        + len(attachment.name.encode())
        + len(attachment.media_type.encode())
        + len(attachment.data)
    )


def _read_exact(stream: BinaryIO, size: int, label: str) -> bytes:
    chunks = bytearray()
    while len(chunks) < size:
        chunk = stream.read(size - len(chunks))
        if not chunk:
            raise IpcFrameError(f"unexpected EOF within IPC {label}")
        chunks.extend(chunk)
    return bytes(chunks)


def _write_all(stream: BinaryIO, data: bytes) -> None:
    view = memoryview(data)
    while view:
        written = stream.write(view)
        if written is None or written <= 0:
            raise IpcFrameError("failed to write a complete IPC frame")
        view = view[written:]


def _encode_text(value: str, label: str) -> bytes:
    if not isinstance(value, str):
        raise IpcFrameError(f"IPC {label} must be text")
    try:
        return value.encode("utf-8", errors="strict")
    except UnicodeEncodeError as error:
        raise IpcFrameError(f"IPC {label} is not valid UTF-8") from error


def _decode_text(value: bytes, label: str) -> str:
    try:
        return value.decode("utf-8", errors="strict")
    except UnicodeDecodeError as error:
        raise IpcFrameError(f"IPC {label} is not valid UTF-8") from error

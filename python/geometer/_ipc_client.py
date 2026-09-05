"""Synchronous persistent client for ``geometer serve --stdio``."""

from __future__ import annotations

import math
import subprocess
import threading
import time
from dataclasses import dataclass, replace
from pathlib import Path
from queue import Empty, Queue
from typing import TYPE_CHECKING, BinaryIO, cast

from ._generated.contracts.codecs import (
    decode_ipc_cancel_rejected_a0_json,
    decode_ipc_cancelled_a0_json,
    decode_ipc_protocol_error_a0_json,
    decode_ipc_shutdown_ack_a0_json,
    decode_ipc_welcome_a0_json,
    decode_operation_outcome_a0_json,
    encode_ipc_hello_a0_json,
    encode_ipc_reason_a0_json,
    encode_ipc_request_a0_json,
)
from ._generated.contracts.models import (
    NORMALIZED_CATALOG_SHA256,
    DiagnosticA0,
    IpcAttachmentDeclarationA0,
    IpcEffectiveLimitsA0,
    IpcHelloA0,
    IpcOperationDeclarationA0,
    IpcReasonA0,
    IpcRequestA0,
    IpcRequestValueA0,
    IpcRuntimeDispatchA0,
    IpcWelcomeA0,
    HlrProjectionOptionsA0,
    HlrProjectionResultA0,
    OperationFailureA0,
    OperationOutcomeA0,
    OperationSuccessA0,
    PackedAttachmentProjectionA0,
    PackedAttachmentReferenceA0,
)
from ._ipc_a0 import (
    MAX_ATTACHMENT_BYTES,
    MAX_ATTACHMENT_COUNT,
    MAX_ATTACHMENT_TEXT_BYTES,
    MAX_FRAME_BYTES,
    MAX_JSON_BYTES,
    Attachment,
    Frame,
    FrameKind,
    IpcFrameError,
    encoded_size,
    read_frame,
    write_frame,
)
from ._generated.contracts.operations import expected_operation_catalog
from ._paths import executable_path

if TYPE_CHECKING:
    from ._tessellation import ModelTessellation
    from ._generated.contracts.models import ModelTessellationRequestA0
    from ._indexed_mesh_packet_a0 import IndexedTriangleMeshA0
    from ._generated.contracts.models import (
        AnalyticPlanarBooleanBatchRequestA0,
        AnalyticPlanarBooleanBatchResultA0,
    )


IPC_IDENTITY = "a0"
ANALYTIC_OPERATION = "geometry.analytic_planar_boolean_batch.a0"
ANALYTIC_REQUEST_CONTRACT = "geometry.analytic_planar_boolean_batch.request.a0"
ANALYTIC_RESULT_CONTRACT = "geometry.analytic_planar_boolean_batch.result.a0"
ANALYTIC_PACKET_FORMAT = "geometry.analytic_planar_boolean.packet.a0"
ANALYTIC_REQUEST_ATTACHMENT = "analytic_planar_boolean_request"
ANALYTIC_RESULT_ATTACHMENT = "analytic_planar_boolean_result"
ANALYTIC_REQUEST_MEDIA_TYPE = "application/vnd.wavenumber.geometer.analytic-planar-boolean-request"
ANALYTIC_RESULT_MEDIA_TYPE = "application/vnd.wavenumber.geometer.analytic-planar-boolean-result"
MODEL_HLR_OPERATION = "geometry.model_hlr_projection.a0"
MESH_HLR_OPERATION = "geometry.mesh_hlr_projection.a0"
INDEXED_MESH_MEDIA_TYPE = "application/vnd.wavenumber.geometer.indexed-triangle-mesh"

_REQUIRED_CAPABILITIES = frozenset({"serialized_execution", "queue_only_cancellation", "raw_attachments"})
_STDERR_CAPTURE_LIMIT = 1024 * 1024
_STARTUP_TIMEOUT_SECONDS = 10.0
_SHUTDOWN_TIMEOUT_SECONDS = 35.0


class GeometerIpcError(RuntimeError):
    """Base error for the persistent executable client."""


class GeometerIpcProtocolError(GeometerIpcError):
    """The executable violated the negotiated IPC contract."""


class GeometerIpcProcessError(GeometerIpcError):
    """The executable could not be started or exited unexpectedly."""


class GeometerIpcTimeoutError(GeometerIpcError):
    """A local IPC deadline elapsed; server interruption is not implied."""

    def __init__(self, phase: str, request_id: int | None = None) -> None:
        request = "" if request_id is None else f" for request {request_id}"
        super().__init__(
            f"local timeout while waiting for {phase}{request}; "
            "any requested cancellation is queue-only and does not prove interruption"
        )
        self.phase = phase
        self.request_id = request_id


class GeometerOperationError(GeometerIpcError):
    """A typed operation response reported failure."""

    def __init__(self, operation: str, diagnostics: tuple[DiagnosticA0, ...]) -> None:
        super().__init__(f"Geometer operation {operation} failed")
        self.operation = operation
        self.diagnostics = diagnostics


@dataclass(frozen=True, slots=True)
class OperationResponse:
    outcome: OperationOutcomeA0
    attachments: tuple[Attachment, ...]


@dataclass(slots=True)
class _PendingRequest:
    request_id: int
    operation: str
    declaration: IpcOperationDeclarationA0
    cancel_rejected: bool = False


@dataclass(frozen=True, slots=True)
class _ReadFailure:
    error: BaseException


class _ReadDeadlineExpired(Exception):
    pass


class _GeometerIpcSession:
    """One-request-at-a-time client for a persistent Geometer executable."""

    def __init__(
        self,
        executable: str | Path | None = None,
        *,
        client_name: str = "wn-geometer-python",
        client_version: str = "source",
        startup_timeout: float = _STARTUP_TIMEOUT_SECONDS,
        shutdown_timeout: float = _SHUTDOWN_TIMEOUT_SECONDS,
    ) -> None:
        client = cast("GeometerIpcClient", self)
        _validate_timeout(startup_timeout, "startup_timeout")
        _validate_timeout(shutdown_timeout, "shutdown_timeout")
        command = Path(executable) if executable is not None else executable_path()
        try:
            self._process = subprocess.Popen(
                [str(command), "serve", "--stdio"],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                bufsize=0,
            )
        except OSError as error:
            raise GeometerIpcProcessError(f"could not start {command}: {error}") from error
        self._lock = threading.RLock()
        self._closed = False
        self._shutdown_timeout = shutdown_timeout
        self._next_request_id = 1
        self._pending: _PendingRequest | None = None
        self._late_cancel_ids: set[int] = set()
        self._stdout_queue: Queue[Frame | _ReadFailure | None] = Queue()
        self._welcome_ready = threading.Event()
        self._stderr = bytearray()
        self._stderr_lock = threading.Lock()
        self._stderr_thread = threading.Thread(
            target=client._capture_stderr,
            name="geometer-python-ipc-stderr",
            daemon=True,
        )
        self._stderr_thread.start()
        self._stdout_thread = threading.Thread(
            target=client._read_stdout,
            name="geometer-python-ipc-stdout",
            daemon=True,
        )
        self._stdout_thread.start()
        try:
            hello = IpcHelloA0(
                client_name=client_name,
                client_version=client_version,
                protocols=(IPC_IDENTITY,),
                capabilities=("raw_attachments",),
            )
            client._write(
                Frame(
                    kind=FrameKind.HELLO,
                    request_id=0,
                    json=encode_ipc_hello_a0_json(hello),
                )
            )
            welcome_frame = client._read_required("welcome", time.monotonic() + startup_timeout)
            if (
                welcome_frame.kind is not FrameKind.WELCOME
                or welcome_frame.request_id != 0
                or welcome_frame.attachments
            ):
                raise GeometerIpcProtocolError("server did not return a valid welcome frame")
            try:
                self._welcome = decode_ipc_welcome_a0_json(welcome_frame.json)
            except Exception as error:
                raise GeometerIpcProtocolError("server returned invalid generated welcome JSON") from error
            _validate_welcome(self._welcome)
            self._welcome_ready.set()
        except _ReadDeadlineExpired as error:
            self._welcome_ready.set()
            client._terminate()
            raise GeometerIpcTimeoutError("startup welcome") from error
        except BaseException:
            self._welcome_ready.set()
            client._terminate()
            raise

    @property
    def welcome(self) -> IpcWelcomeA0:
        return self._welcome

    @property
    def process_id(self) -> int:
        """Operating-system process id of this client's owned server."""

        return self._process.pid

    @property
    def stderr_text(self) -> str:
        with self._stderr_lock:
            return bytes(self._stderr).decode("utf-8", errors="replace")

    def __enter__(self) -> GeometerIpcClient:
        return cast("GeometerIpcClient", self)

    def __exit__(self, exc_type: object, exc: object, traceback: object) -> None:
        client = cast("GeometerIpcClient", self)
        if exc_type is None:
            client.close()
            return
        try:
            client.close()
        except GeometerIpcError:
            client._terminate()

    def close(self, *, reason: str | None = None, timeout: float | None = None) -> None:
        """Request graceful A0 shutdown and wait for its final acknowledgment."""

        client = cast("GeometerIpcClient", self)
        with self._lock:
            if self._closed:
                return
            close_timeout = self._shutdown_timeout if timeout is None else timeout
            _validate_timeout(close_timeout, "timeout")
            deadline = time.monotonic() + close_timeout
            had_pending = self._pending is not None
            try:
                client._write(
                    Frame(
                        kind=FrameKind.SHUTDOWN,
                        request_id=0,
                        json=encode_ipc_reason_a0_json(IpcReasonA0(reason=reason)),
                    )
                )
                frame, active_request_eligible = client._read_shutdown_ack(deadline)
                try:
                    acknowledgment = decode_ipc_shutdown_ack_a0_json(frame.json)
                except Exception as error:
                    raise GeometerIpcProtocolError("server returned invalid shutdown_ack JSON") from error
                if (
                    acknowledgment.status != "complete"
                    or acknowledgment.rejected_queued_request_count != 0
                    or (not had_pending and acknowledgment.active_request_completed)
                    or (acknowledgment.active_request_completed and not active_request_eligible)
                ):
                    raise GeometerIpcProtocolError("shutdown acknowledgment contradicts observed client state")
                if self._process.stdin is not None:
                    self._process.stdin.close()
                try:
                    return_code = self._process.wait(timeout=max(0.0, deadline - time.monotonic()))
                except subprocess.TimeoutExpired as error:
                    raise GeometerIpcTimeoutError("server exit after shutdown acknowledgment") from error
                if return_code != 0:
                    raise GeometerIpcProcessError(
                        f"server exited with {return_code} after shutdown_ack: {self.stderr_text}"
                    )
                self._closed = True
                client._close_pipes()
            except _ReadDeadlineExpired as error:
                client._terminate()
                raise GeometerIpcTimeoutError(
                    "shutdown terminal and acknowledgment",
                    None if self._pending is None else self._pending.request_id,
                ) from error
            except BaseException:
                client._terminate()
                raise


class _GeometerIpcExecution(_GeometerIpcSession):
    def execute(
        self,
        operation: str,
        request: IpcRequestValueA0,
        attachments: tuple[Attachment, ...] = (),
        *,
        timeout: float | None = None,
    ) -> OperationResponse:
        """Execute one generated request over the serialized connection."""

        client = cast("GeometerIpcClient", self)
        with self._lock:
            client._require_open()
            if timeout is not None:
                _validate_timeout(timeout, "timeout")
            try:
                client._refresh_timed_out_request()
            except BaseException:
                client._terminate()
                raise
            if self._pending is not None:
                raise GeometerIpcError(f"request {self._pending.request_id} timed out locally and is still draining")
            declaration = _operation(self._welcome, operation)
            _validate_request_value(declaration, request)
            _validate_declared_attachments(attachments, declaration.input_attachments, "request")
            request_id = client._take_request_id()
            frame = Frame(
                kind=FrameKind.REQUEST,
                request_id=request_id,
                json=encode_ipc_request_a0_json(IpcRequestA0(operation=operation, request=request)),
                attachments=attachments,
            )
            _validate_effective_frame(frame, self._welcome.limits)
            try:
                client._write(frame)
            except BaseException:
                client._terminate()
                raise
            self._pending = _PendingRequest(request_id, operation, declaration)
            deadline = None if timeout is None else time.monotonic() + timeout
            try:
                response = client._read_operation_response(deadline)
            except _ReadDeadlineExpired as error:
                try:
                    client._write(
                        Frame(
                            kind=FrameKind.CANCEL,
                            request_id=request_id,
                            json=encode_ipc_reason_a0_json(IpcReasonA0(reason="local client timeout")),
                        )
                    )
                except BaseException:
                    client._terminate()
                    raise
                raise GeometerIpcTimeoutError("operation response", request_id) from error
            except BaseException:
                client._terminate()
                raise
            self._pending = None
            return response

    def analytic_planar_boolean_batch(
        self,
        request: AnalyticPlanarBooleanBatchRequestA0,
        *,
        timeout: float | None = None,
    ) -> AnalyticPlanarBooleanBatchResultA0:
        """Execute analytic planar Boolean A0 with its governed packed projection."""

        client = cast("GeometerIpcClient", self)
        from ._analytic_packet_a0 import (
            decode_analytic_planar_boolean_batch_result_a0_packet,
            encode_analytic_planar_boolean_batch_request_a0_packet,
        )

        packet = encode_analytic_planar_boolean_batch_request_a0_packet(request)
        projection = PackedAttachmentProjectionA0(
            schema=ANALYTIC_REQUEST_CONTRACT,
            packet=PackedAttachmentReferenceA0(
                attachment=ANALYTIC_REQUEST_ATTACHMENT,
                format=ANALYTIC_PACKET_FORMAT,
            ),
        )
        response = client.execute(
            ANALYTIC_OPERATION,
            projection,
            (
                Attachment(
                    name=ANALYTIC_REQUEST_ATTACHMENT,
                    media_type=ANALYTIC_REQUEST_MEDIA_TYPE,
                    data=packet,
                ),
            ),
            timeout=timeout,
        )
        if isinstance(response.outcome, OperationFailureA0):
            raise GeometerOperationError(response.outcome.operation, response.outcome.diagnostics)
        try:
            result_projection = response.outcome.result
            if not isinstance(result_projection, PackedAttachmentProjectionA0) or result_projection != (
                PackedAttachmentProjectionA0(
                    schema=ANALYTIC_RESULT_CONTRACT,
                    packet=PackedAttachmentReferenceA0(
                        attachment=ANALYTIC_RESULT_ATTACHMENT,
                        format=ANALYTIC_PACKET_FORMAT,
                    ),
                )
            ):
                raise GeometerIpcProtocolError("analytic response contains an incompatible packed projection")
            if len(response.attachments) != 1:
                raise GeometerIpcProtocolError("analytic response does not contain exactly one result attachment")
            return decode_analytic_planar_boolean_batch_result_a0_packet(response.attachments[0].data)
        except GeometerIpcProtocolError:
            client._terminate()
            raise
        except Exception as error:
            client._terminate()
            raise GeometerIpcProtocolError("analytic response contains an invalid packed result") from error

    def model_tessellation(
        self,
        model: bytes,
        options: ModelTessellationRequestA0 | None = None,
        *,
        timeout: float | None = None,
    ) -> ModelTessellation:
        """Tessellate STEP bytes into the shared colored millimeter mesh contract."""
        from ._tessellation import model_tessellation

        return model_tessellation(cast("GeometerIpcClient", self), model, options, timeout)

    def model_hlr_projection(
        self,
        model: bytes,
        options: HlrProjectionOptionsA0 | None = None,
        *,
        media_type: str = "application/step",
        timeout: float | None = None,
    ) -> HlrProjectionResultA0:
        """Project a STEP attachment through the governed HLR A0 operation."""

        return self._hlr_projection(
            MODEL_HLR_OPERATION,
            "model",
            media_type,
            model,
            options or HlrProjectionOptionsA0(),
            timeout,
        )

    def mesh_hlr_projection(
        self,
        mesh: bytes | IndexedTriangleMeshA0,
        options: HlrProjectionOptionsA0 | None = None,
        *,
        timeout: float | None = None,
    ) -> HlrProjectionResultA0:
        """Project an encoded or structured indexed mesh through Fast HLR A0."""

        from ._indexed_mesh_packet_a0 import IndexedTriangleMeshA0, encode_indexed_triangle_mesh_a0_packet

        packet = encode_indexed_triangle_mesh_a0_packet(mesh) if isinstance(mesh, IndexedTriangleMeshA0) else mesh
        return self._hlr_projection(
            MESH_HLR_OPERATION,
            "mesh",
            INDEXED_MESH_MEDIA_TYPE,
            packet,
            options or HlrProjectionOptionsA0(),
            timeout,
        )

    def _hlr_projection(
        self,
        operation: str,
        attachment_name: str,
        media_type: str,
        data: bytes,
        options: HlrProjectionOptionsA0,
        timeout: float | None,
    ) -> HlrProjectionResultA0:
        client = cast("GeometerIpcClient", self)
        # The IPC request union also contains presence-only model-bounds options.
        # Preserve the HLR default while making the logical variant unambiguous.
        if options.output_detail is None:
            options = replace(options, output_detail=True)
        response = client.execute(
            operation,
            options,
            (Attachment(name=attachment_name, media_type=media_type, data=data),),
            timeout=timeout,
        )
        if isinstance(response.outcome, OperationFailureA0):
            raise GeometerOperationError(response.outcome.operation, response.outcome.diagnostics)
        if response.attachments or not isinstance(response.outcome.result, HlrProjectionResultA0):
            client._terminate()
            raise GeometerIpcProtocolError("HLR response contains an incompatible result")
        return response.outcome.result


class _GeometerIpcResponse(_GeometerIpcExecution):
    def _take_request_id(self) -> int:
        request_id = self._next_request_id
        if request_id > (1 << 64) - 1:
            raise GeometerIpcProtocolError("IPC request id space is exhausted")
        self._next_request_id += 1
        return request_id

    def _write(self, frame: Frame) -> None:
        stream = self._process.stdin
        if stream is None:
            raise GeometerIpcProcessError("server stdin is unavailable")
        try:
            write_frame(cast(BinaryIO, stream), frame)
        except (BrokenPipeError, OSError, IpcFrameError) as error:
            raise GeometerIpcProcessError(f"could not write IPC frame: {error}") from error

    def _read_required(self, label: str, deadline: float | None) -> Frame:
        try:
            wait = None if deadline is None else max(0.0, deadline - time.monotonic())
            item = self._stdout_queue.get(timeout=wait)
        except Empty as error:
            raise _ReadDeadlineExpired from error
        if item is None:
            return_code = self._process.poll()
            raise GeometerIpcProcessError(
                f"server closed stdout before {label}"
                + ("" if return_code is None else f" (exit {return_code})")
                + ("" if not self.stderr_text else f": {self.stderr_text}")
            )
        if isinstance(item, _ReadFailure):
            if isinstance(item.error, IpcFrameError):
                raise GeometerIpcProtocolError(f"malformed IPC framing before {label}: {item.error}") from item.error
            raise GeometerIpcProcessError(f"could not read IPC {label}: {item.error}") from item.error
        return item

    def _read_operation_response(self, deadline: float | None) -> OperationResponse:
        client = cast("GeometerIpcClient", self)
        pending = self._pending
        if pending is None:
            raise GeometerIpcProtocolError("internal IPC request state is missing")
        while True:
            frame = self._read_required(f"response for request {pending.request_id}", deadline)
            if client._discard_late_cancel_rejection(frame):
                continue
            if frame.kind is FrameKind.PROTOCOL_ERROR:
                raise GeometerIpcProtocolError(_protocol_error_message(frame))
            if frame.kind is not FrameKind.RESPONSE or frame.request_id != pending.request_id:
                raise GeometerIpcProtocolError(
                    f"expected response for request {pending.request_id}, received {frame.kind.name.lower()} "
                    f"for request {frame.request_id}"
                )
            return self._decode_response(frame, pending)

    def _decode_response(self, frame: Frame, pending: _PendingRequest) -> OperationResponse:
        _validate_effective_frame(frame, self._welcome.limits)
        try:
            outcome = decode_operation_outcome_a0_json(frame.json)
        except Exception as error:
            raise GeometerIpcProtocolError("response contains invalid generated outcome JSON") from error
        if _outcome_operation(outcome) != pending.operation:
            raise GeometerIpcProtocolError("response operation does not match its request")
        declarations = pending.declaration.output_attachments if isinstance(outcome, OperationSuccessA0) else ()
        _validate_declared_attachments(frame.attachments, declarations, "response")
        return OperationResponse(outcome=outcome, attachments=frame.attachments)


class _GeometerIpcDrain(_GeometerIpcResponse):
    def _refresh_timed_out_request(self) -> None:
        client = cast("GeometerIpcClient", self)
        while self._pending is not None:
            try:
                frame = self._stdout_queue.get_nowait()
            except Empty:
                return
            client._consume_timed_out_frame(frame, "timed-out request drain")
        client._discard_available_late_rejections()

    def _consume_timed_out_frame(self, item: Frame | _ReadFailure | None, label: str) -> None:
        client = cast("GeometerIpcClient", self)
        if item is None:
            raise GeometerIpcProcessError(f"server closed stdout during {label}")
        if isinstance(item, _ReadFailure):
            if isinstance(item.error, IpcFrameError):
                raise GeometerIpcProtocolError(f"malformed IPC framing during {label}: {item.error}") from item.error
            raise GeometerIpcProcessError(f"could not read IPC during {label}: {item.error}") from item.error
        pending = self._pending
        if pending is None:
            if not client._discard_late_cancel_rejection(item):
                raise GeometerIpcProtocolError("unexpected frame after terminal timed-out response")
            return
        if item.kind is FrameKind.PROTOCOL_ERROR:
            raise GeometerIpcProtocolError(_protocol_error_message(item))
        if item.request_id != pending.request_id:
            raise GeometerIpcProtocolError("timed-out request drain received a mismatched request id")
        if item.kind is FrameKind.CANCEL_REJECTED:
            try:
                decode_ipc_cancel_rejected_a0_json(item.json)
            except Exception as error:
                raise GeometerIpcProtocolError("server returned invalid cancel_rejected JSON") from error
            pending.cancel_rejected = True
            return
        if item.kind is FrameKind.CANCELLED:
            try:
                decode_ipc_cancelled_a0_json(item.json)
            except Exception as error:
                raise GeometerIpcProtocolError("server returned invalid cancelled JSON") from error
            if item.attachments:
                raise GeometerIpcProtocolError("cancelled frame carries attachments")
            self._pending = None
            return
        if item.kind is FrameKind.RESPONSE:
            self._decode_response(item, pending)
            if not pending.cancel_rejected:
                self._late_cancel_ids.add(pending.request_id)
            self._pending = None
            return
        raise GeometerIpcProtocolError(f"unexpected {item.kind.name.lower()} during timed-out request drain")


class _GeometerIpcShutdown(_GeometerIpcDrain):
    def _discard_late_cancel_rejection(self, frame: Frame) -> bool:
        if frame.kind is not FrameKind.CANCEL_REJECTED or frame.request_id not in self._late_cancel_ids:
            return False
        try:
            decode_ipc_cancel_rejected_a0_json(frame.json)
        except Exception as error:
            raise GeometerIpcProtocolError("server returned invalid late cancel_rejected JSON") from error
        self._late_cancel_ids.remove(frame.request_id)
        return True

    def _discard_available_late_rejections(self) -> None:
        while self._late_cancel_ids:
            try:
                item = self._stdout_queue.get_nowait()
            except Empty:
                return
            if item is None or isinstance(item, _ReadFailure):
                self._consume_timed_out_frame(item, "late cancellation drain")
                continue
            if not self._discard_late_cancel_rejection(item):
                self._stdout_queue.put(item)
                return

    def _read_shutdown_ack(self, deadline: float) -> tuple[Frame, bool]:
        active_request_eligible = False
        while True:
            frame = self._read_required("shutdown terminal or acknowledgment", deadline)
            if self._pending is not None:
                pending = self._pending
                was_rejected_active = pending.cancel_rejected
                self._consume_timed_out_frame(frame, "shutdown request drain")
                if self._pending is None and frame.kind is FrameKind.RESPONSE and was_rejected_active:
                    active_request_eligible = True
                continue
            if self._discard_late_cancel_rejection(frame):
                continue
            if frame.kind is FrameKind.PROTOCOL_ERROR:
                raise GeometerIpcProtocolError(_protocol_error_message(frame))
            if frame.kind is FrameKind.SHUTDOWN_ACK and frame.request_id == 0 and not frame.attachments:
                return frame, active_request_eligible
            raise GeometerIpcProtocolError("server did not return a valid shutdown_ack frame")


class GeometerIpcClient(_GeometerIpcShutdown):
    """One-request-at-a-time client for a persistent Geometer executable."""

    def _read_stdout(self) -> None:
        stream = self._process.stdout
        if stream is None:
            self._stdout_queue.put(_ReadFailure(GeometerIpcProcessError("server stdout is unavailable")))
            return
        first = True
        while True:
            try:
                if first:
                    frame = read_frame(cast(BinaryIO, stream))
                else:
                    limits = self._welcome.limits
                    frame = read_frame(
                        cast(BinaryIO, stream),
                        max_json_bytes=limits.json_bytes,
                        max_attachment_count=limits.attachment_count,
                        max_attachment_name_bytes=limits.attachment_name_bytes,
                        max_attachment_media_type_bytes=limits.attachment_media_type_bytes,
                        max_attachment_bytes=limits.attachment_bytes,
                        max_frame_bytes=limits.frame_bytes,
                    )
            except (OSError, IpcFrameError) as error:
                self._stdout_queue.put(_ReadFailure(error))
                return
            self._stdout_queue.put(frame)
            if frame is None:
                return
            if first:
                self._welcome_ready.wait()
                if self._closed:
                    return
                first = False

    def _require_open(self) -> None:
        if self._closed or self._process.poll() is not None:
            raise GeometerIpcProcessError("Geometer IPC client is closed")

    def _capture_stderr(self) -> None:
        stream = self._process.stderr
        if stream is None:
            return
        try:
            while True:
                chunk = stream.read(4096)
                if not chunk:
                    return
                with self._stderr_lock:
                    remaining = _STDERR_CAPTURE_LIMIT - len(self._stderr)
                    if remaining > 0:
                        self._stderr.extend(chunk[:remaining])
        except OSError:
            return

    def _close_pipes(self) -> None:
        for stream in (self._process.stdout, self._process.stderr):
            if stream is not None:
                try:
                    stream.close()
                except OSError:
                    pass
        self._stderr_thread.join(timeout=1)
        self._stdout_thread.join(timeout=1)

    def _terminate(self) -> None:
        if self._closed:
            return
        self._closed = True
        if self._process.stdin is not None:
            try:
                self._process.stdin.close()
            except OSError:
                pass
        if self._process.poll() is None:
            try:
                self._process.terminate()
                self._process.wait(timeout=2)
            except (OSError, subprocess.TimeoutExpired):
                try:
                    self._process.kill()
                    self._process.wait(timeout=2)
                except (OSError, subprocess.TimeoutExpired):
                    pass
        self._close_pipes()


def _validate_timeout(value: float, label: str) -> None:
    if isinstance(value, bool) or not isinstance(value, (int, float)) or value <= 0 or not math.isfinite(value):
        raise ValueError(f"{label} must be a finite positive number")


def _validate_welcome(welcome: IpcWelcomeA0) -> None:
    if welcome.ipc != IPC_IDENTITY or welcome.catalog_sha256 != NORMALIZED_CATALOG_SHA256:
        raise GeometerIpcProtocolError("welcome selected an unsupported IPC or contract catalog")
    if not _valid_effective_limits(welcome.limits):
        raise GeometerIpcProtocolError("welcome advertises an invalid effective A0 limit")
    if not _REQUIRED_CAPABILITIES.issubset(welcome.capabilities):
        missing = sorted(_REQUIRED_CAPABILITIES.difference(welcome.capabilities))
        raise GeometerIpcProtocolError(f"welcome is missing required capabilities: {', '.join(missing)}")
    catalog = welcome.operation_catalog
    expected_catalog = expected_operation_catalog(welcome.release_version, welcome.c_abi_generation)
    if catalog != expected_catalog:
        raise GeometerIpcProtocolError("welcome operation catalog differs from generated authority")
    analytic = _operation(welcome, ANALYTIC_OPERATION)
    if (
        analytic.request_contract != ANALYTIC_REQUEST_CONTRACT
        or analytic.result_contract != ANALYTIC_RESULT_CONTRACT
        or analytic.runtime_dispatch is not IpcRuntimeDispatchA0.PACKED_ATTACHMENT
        or analytic.request_projection is None
        or analytic.result_projection is None
        or analytic.request_projection.attachment_name != ANALYTIC_REQUEST_ATTACHMENT
        or analytic.result_projection.attachment_name != ANALYTIC_RESULT_ATTACHMENT
        or analytic.request_projection.format != ANALYTIC_PACKET_FORMAT
        or analytic.result_projection.format != ANALYTIC_PACKET_FORMAT
    ):
        raise GeometerIpcProtocolError("welcome analytic operation declaration is incompatible")
    _require_exact_declaration(
        analytic.input_attachments,
        ANALYTIC_REQUEST_ATTACHMENT,
        ANALYTIC_REQUEST_MEDIA_TYPE,
        "analytic request",
    )
    _require_exact_declaration(
        analytic.output_attachments,
        ANALYTIC_RESULT_ATTACHMENT,
        ANALYTIC_RESULT_MEDIA_TYPE,
        "analytic result",
    )
    bounds = _operation(welcome, "geometry.model_bounds.a0")
    if (
        bounds.request_contract != "geometry.model_bounds.options.a0"
        or bounds.result_contract != "geometry.model_bounds.a0"
        or bounds.runtime_dispatch is not IpcRuntimeDispatchA0.LOGICAL_DTO
        or bounds.request_projection is not None
        or bounds.result_projection is not None
        or bounds.output_attachments
    ):
        raise GeometerIpcProtocolError("welcome model-bounds operation declaration is incompatible")
    if len(bounds.input_attachments) != 1:
        raise GeometerIpcProtocolError("welcome model-bounds attachment inventory is incompatible")
    model = bounds.input_attachments[0]
    if (
        model.name != "model"
        or not model.required
        or model.media_types != ("application/step", "model/step")
        or model.max_bytes != MAX_ATTACHMENT_BYTES
    ):
        raise GeometerIpcProtocolError("welcome model-bounds attachment declaration is incompatible")


def _operation(welcome: IpcWelcomeA0, identity: str) -> IpcOperationDeclarationA0:
    matches = [operation for operation in welcome.operation_catalog.operations if operation.identity == identity]
    if len(matches) != 1:
        raise GeometerIpcProtocolError(f"operation {identity} is absent or duplicated in negotiated catalog")
    return matches[0]


def _require_exact_declaration(
    declarations: tuple[IpcAttachmentDeclarationA0, ...],
    name: str,
    media_type: str,
    label: str,
) -> None:
    if len(declarations) != 1:
        raise GeometerIpcProtocolError(f"welcome {label} attachment inventory is incompatible")
    declaration = declarations[0]
    if (
        declaration.name != name
        or not declaration.required
        or declaration.media_types != (media_type,)
        or declaration.max_bytes != MAX_ATTACHMENT_BYTES
    ):
        raise GeometerIpcProtocolError(f"welcome {label} attachment declaration is incompatible")


def _valid_effective_limits(limits: IpcEffectiveLimitsA0) -> bool:
    bounded = (
        (limits.json_bytes, MAX_JSON_BYTES),
        (limits.attachment_count, MAX_ATTACHMENT_COUNT),
        (limits.attachment_name_bytes, MAX_ATTACHMENT_TEXT_BYTES),
        (limits.attachment_media_type_bytes, MAX_ATTACHMENT_TEXT_BYTES),
        (limits.attachment_bytes, MAX_ATTACHMENT_BYTES),
        (limits.frame_bytes, MAX_FRAME_BYTES),
        (limits.queued_requests, 8),
        (limits.queued_bytes, MAX_FRAME_BYTES),
        (limits.resident_request_bytes, MAX_FRAME_BYTES),
        (limits.pending_writer_bytes, MAX_FRAME_BYTES),
    )
    return all(0 < value <= maximum for value, maximum in bounded)


def _validate_request_value(declaration: IpcOperationDeclarationA0, request: IpcRequestValueA0) -> None:
    is_packed = isinstance(request, PackedAttachmentProjectionA0)
    if is_packed != (declaration.runtime_dispatch is IpcRuntimeDispatchA0.PACKED_ATTACHMENT):
        raise GeometerIpcProtocolError("request projection does not match negotiated runtime dispatch")
    if declaration.request_contract == "geometry.hlr_projection.options.a0" and not isinstance(
        request, HlrProjectionOptionsA0
    ):
        raise GeometerIpcProtocolError("request value does not match the negotiated HLR contract")


def _validate_declared_attachments(
    attachments: tuple[Attachment, ...],
    declarations: tuple[IpcAttachmentDeclarationA0, ...],
    lane: str,
) -> None:
    by_name = {declaration.name: declaration for declaration in declarations}
    names: set[str] = set()
    for attachment in attachments:
        if attachment.name in names:
            raise GeometerIpcProtocolError(f"{lane} contains duplicate attachment {attachment.name}")
        names.add(attachment.name)
        declaration = by_name.get(attachment.name)
        if declaration is None:
            raise GeometerIpcProtocolError(f"{lane} contains undeclared attachment {attachment.name}")
        if attachment.media_type not in declaration.media_types:
            raise GeometerIpcProtocolError(f"{lane} attachment {attachment.name} has incompatible media type")
        if len(attachment.data) > declaration.max_bytes:
            raise GeometerIpcProtocolError(f"{lane} attachment {attachment.name} exceeds its catalog limit")
    missing = sorted(
        declaration.name for declaration in declarations if declaration.required and declaration.name not in names
    )
    if missing:
        raise GeometerIpcProtocolError(f"{lane} is missing required attachments: {', '.join(missing)}")


def _validate_effective_frame(frame: Frame, limits: IpcEffectiveLimitsA0) -> None:
    if (
        len(frame.json) > limits.json_bytes
        or len(frame.attachments) > limits.attachment_count
        or encoded_size(frame) > limits.frame_bytes
        or any(
            len(attachment.name.encode()) > limits.attachment_name_bytes
            or len(attachment.media_type.encode()) > limits.attachment_media_type_bytes
            or len(attachment.data) > limits.attachment_bytes
            for attachment in frame.attachments
        )
    ):
        raise GeometerIpcProtocolError("request exceeds an effective limit advertised by welcome")


def _outcome_operation(outcome: OperationOutcomeA0) -> str:
    return outcome.operation


def _protocol_error_message(frame: Frame) -> str:
    try:
        control = decode_ipc_protocol_error_a0_json(frame.json)
    except Exception:
        return "server returned invalid protocol_error JSON"
    return f"server protocol error: {control.diagnostic.message}"

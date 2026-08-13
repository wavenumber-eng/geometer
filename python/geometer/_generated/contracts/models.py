# Generated from wn_geometer_contract_catalog.a0.json. Do not edit.

from dataclasses import dataclass
from enum import Enum
from typing import Literal, TypeAlias


# Stable diagnostic category with distinct transport, contract, and operation domains.
class DiagnosticCategory(str, Enum):
    TRANSPORT = "transport"
    CONTRACT = "contract"
    OPERATION = "operation"


# A strict cross-transport diagnostic; message text is informative rather than identity.
@dataclass(frozen=True, slots=True, kw_only=True)
class DiagnosticA0:
    # Stable namespaced diagnostic identity.
    code: str
    category: DiagnosticCategory
    # Human-readable non-normative detail.
    message: str
    retryable: bool
    # RFC 6901 JSON Pointer when a document location is meaningful.
    path: str | None = None
    # Stable operation identity when the diagnostic is operation-scoped.
    operation: str | None = None
    # Transport correlation identifier when available.
    request_id: str | None = None


# Named raw-attachment declaration in the negotiated operation catalog.
@dataclass(frozen=True, slots=True, kw_only=True)
class IpcAttachmentDeclarationA0:
    name: str
    required: bool
    media_types: tuple[str, ...]
    max_bytes: int


# wasm32 attachment-descriptor member offsets.
@dataclass(frozen=True, slots=True, kw_only=True)
class IpcAttachmentOffsetsWasm32A0:
    struct_size: int
    flags: int
    name: int
    name_size: int
    media_type: int
    media_type_size: int
    data: int
    data_size: int
    reserved0: int


@dataclass(frozen=True, slots=True, kw_only=True)
class IpcAttachmentLayoutWasm32A0:
    size: int
    offsets: IpcAttachmentOffsetsWasm32A0


# 64-bit attachment-descriptor member offsets.
@dataclass(frozen=True, slots=True, kw_only=True)
class IpcAttachmentOffsetsPointer64A0:
    struct_size: int
    flags: int
    name: int
    name_size: int
    media_type: int
    media_type_size: int
    data: int
    data_size: int
    reserved0: int


@dataclass(frozen=True, slots=True, kw_only=True)
class IpcAttachmentLayoutPointer64A0:
    size: int
    offsets: IpcAttachmentOffsetsPointer64A0


@dataclass(frozen=True, slots=True, kw_only=True)
class IpcAttachmentDescriptorA0:
    wasm32: IpcAttachmentLayoutWasm32A0
    pointer64: IpcAttachmentLayoutPointer64A0


# Successful queue-only cancellation outcome.
@dataclass(frozen=True, slots=True, kw_only=True)
class IpcCancelledA0:
    status: Literal["cancelled"]


# Nonterminal cancellation rejection for an active or unknown request.
@dataclass(frozen=True, slots=True, kw_only=True)
class IpcCancelRejectedA0:
    status: Literal["rejected"]
    diagnostic: DiagnosticA0


# Effective executable IPC A0 limits selected by the server.
@dataclass(frozen=True, slots=True, kw_only=True)
class IpcEffectiveLimitsA0:
    json_bytes: int
    attachment_count: int
    attachment_name_bytes: int
    attachment_media_type_bytes: int
    attachment_bytes: int
    frame_bytes: int
    queued_requests: int
    queued_bytes: int
    resident_request_bytes: int
    pending_writer_bytes: int


# Bounds shared by the generic C ABI operation catalog.
@dataclass(frozen=True, slots=True, kw_only=True)
class IpcGenericAbiLimitsA0:
    operation_id_bytes: int
    request_json_bytes: int
    response_json_bytes: int
    attachment_count: int
    attachment_name_bytes: int
    attachment_media_type_bytes: int
    attachment_bytes: int
    aggregate_attachment_bytes_native: int
    aggregate_attachment_bytes_wasm: int


# Client handshake for executable IPC A0.
@dataclass(frozen=True, slots=True, kw_only=True)
class IpcHelloA0:
    client_name: str
    client_version: str
    protocols: tuple[str, ...]
    capabilities: tuple[str, ...] | None = None


# One operation exposed by the negotiated generic transport.
@dataclass(frozen=True, slots=True, kw_only=True)
class IpcOperationDeclarationA0:
    identity: str
    request_contract: str
    result_contract: str
    input_attachments: tuple[IpcAttachmentDeclarationA0, ...]
    output_attachments: tuple[IpcAttachmentDeclarationA0, ...]


# Exact operation catalog embedded into the welcome frame.
@dataclass(frozen=True, slots=True, kw_only=True)
class IpcOperationCatalogA0:
    catalog: Literal["wn.geometer.operation_catalog.a0"]
    generic_abi: Literal["a0"]
    release_version: str
    c_abi_generation: int
    operations: tuple[IpcOperationDeclarationA0, ...]
    attachment_descriptor: IpcAttachmentDescriptorA0
    limits: IpcGenericAbiLimitsA0


# Fatal connection-level protocol diagnostic.
@dataclass(frozen=True, slots=True, kw_only=True)
class IpcProtocolErrorA0:
    status: Literal["protocol_error"]
    diagnostic: DiagnosticA0


# Optional human-readable reason used by cancel and shutdown controls.
@dataclass(frozen=True, slots=True, kw_only=True)
class IpcReasonA0:
    reason: str | None = None


# Canonical model source format. Compatibility readers may additionally accept STEP.
class ModelFormat(str, Enum):
    STEP = "step"


# Row-major affine 4x4 matrix. Semantic validation requires final row [0,0,0,1].
Matrix4x4: TypeAlias = tuple[
    float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float
]


# Presence-preserving patch applied over focused C++ defaults.
@dataclass(frozen=True, slots=True, kw_only=True)
class ModelBoundsOptionsA0:
    # Absent preserves the inherited value; canonical default intent is step.
    format: ModelFormat | None = None
    # Absent preserves the inherited transform; canonical default intent is identity.
    model_transform: Matrix4x4 | None = None


# Strict generic request envelope for executable IPC A0.
@dataclass(frozen=True, slots=True, kw_only=True)
class IpcRequestA0:
    operation: str
    request: ModelBoundsOptionsA0


# Final acknowledgment after every accepted request is terminal and flushed.
@dataclass(frozen=True, slots=True, kw_only=True)
class IpcShutdownAckA0:
    status: Literal["complete"]
    active_request_completed: bool
    rejected_queued_request_count: int


# Server handshake selecting executable IPC A0 and its effective limits.
@dataclass(frozen=True, slots=True, kw_only=True)
class IpcWelcomeA0:
    release_version: str
    c_abi_generation: int
    ipc: Literal["a0"]
    catalog_sha256: str
    operation_catalog: IpcOperationCatalogA0
    limits: IpcEffectiveLimitsA0
    capabilities: tuple[str, ...]


# Source identity included in a successful model-bounds result.
@dataclass(frozen=True, slots=True, kw_only=True)
class ModelBoundsSource:
    format: ModelFormat
    hash: str


# Exactly three finite millimeter coordinates.
Vector3: TypeAlias = tuple[float, float, float]


# Axis-aligned bounds in millimeters.
@dataclass(frozen=True, slots=True, kw_only=True)
class ModelBoundsValues:
    min: Vector3
    max: Vector3
    size: Vector3
    center: Vector3


# Nondeterministic timings excluded only by explicit conformance projection.
@dataclass(frozen=True, slots=True, kw_only=True)
class ModelBoundsTimings:
    model_read_ms: float
    bounds_ms: float


# Successful model-bounds result matching the compatible public JSON shape.
@dataclass(frozen=True, slots=True, kw_only=True)
class ModelBoundsResultA0:
    schema: Literal["geometry.model_bounds.a0"]
    units: Literal["mm"]
    source: ModelBoundsSource
    bounds: ModelBoundsValues
    timings: ModelBoundsTimings


# A rejected or failed operation with governed diagnostics.
@dataclass(frozen=True, slots=True, kw_only=True)
class OperationFailureA0:
    operation: str
    ok: Literal[False]
    diagnostics: tuple[DiagnosticA0, ...]


# Results currently available through the generic operation transport.
OperationResultValueA0: TypeAlias = ModelBoundsResultA0


# A completed operation with its operation-specific result.
@dataclass(frozen=True, slots=True, kw_only=True)
class OperationSuccessA0:
    operation: str
    ok: Literal[True]
    result: OperationResultValueA0


# Transport-neutral typed outcome shared by the generic C ABI and executable IPC.
OperationOutcomeA0: TypeAlias = OperationSuccessA0 | OperationFailureA0

MODEL_TYPES = {
    "Wavenumber.Geometer.Contracts.Common.DiagnosticA0": DiagnosticA0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentDeclarationA0": IpcAttachmentDeclarationA0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentDescriptorA0": IpcAttachmentDescriptorA0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentLayoutPointer64A0": IpcAttachmentLayoutPointer64A0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentLayoutWasm32A0": IpcAttachmentLayoutWasm32A0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentOffsetsPointer64A0": IpcAttachmentOffsetsPointer64A0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentOffsetsWasm32A0": IpcAttachmentOffsetsWasm32A0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcCancelledA0": IpcCancelledA0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcCancelRejectedA0": IpcCancelRejectedA0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcEffectiveLimitsA0": IpcEffectiveLimitsA0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcGenericAbiLimitsA0": IpcGenericAbiLimitsA0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcHelloA0": IpcHelloA0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcOperationCatalogA0": IpcOperationCatalogA0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcOperationDeclarationA0": IpcOperationDeclarationA0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcProtocolErrorA0": IpcProtocolErrorA0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcReasonA0": IpcReasonA0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcRequestA0": IpcRequestA0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcShutdownAckA0": IpcShutdownAckA0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcWelcomeA0": IpcWelcomeA0,
    "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsOptionsA0": ModelBoundsOptionsA0,
    "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsResultA0": ModelBoundsResultA0,
    "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsSource": ModelBoundsSource,
    "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsTimings": ModelBoundsTimings,
    "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsValues": ModelBoundsValues,
    "Wavenumber.Geometer.Contracts.OperationOutcomeA0.OperationFailureA0": OperationFailureA0,
    "Wavenumber.Geometer.Contracts.OperationOutcomeA0.OperationSuccessA0": OperationSuccessA0,
}

ENUM_TYPES = {
    "Wavenumber.Geometer.Contracts.Common.DiagnosticCategory": DiagnosticCategory,
    "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelFormat": ModelFormat,
}

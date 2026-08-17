import "@typespec/json-schema";
import "@wavenumber/wn-geometer-contract-emitter";
import "./common.tsp";
import "./operations/model-bounds-a0.tsp";

using TypeSpec.JsonSchema;
using Wavenumber.Geometer.ContractMetadata;

namespace Wavenumber.Geometer.Contracts.IpcA0;

/** Client handshake for executable IPC A0. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometer.ipc.hello:a0")
@contractIdentity("geometer.ipc.hello.a0")
model IpcHelloA0 {
  @minLength(1)
  @maxLength(128)
  client_name: string;

  @minLength(1)
  @maxLength(128)
  client_version: string;

  @minItems(1)
  @maxItems(16)
  protocols: string[];

  @maxItems(64)
  capabilities?: string[];
}

/** Optional human-readable reason used by cancel and shutdown controls. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometer.ipc.reason:a0")
@contractIdentity("geometer.ipc.reason.a0")
model IpcReasonA0 {
  @maxLength(1024)
  reason?: string;
}

/** Strict generic request envelope for executable IPC A0. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometer.ipc.request:a0")
@contractIdentity("geometer.ipc.request.a0")
model IpcRequestA0 {
  @minLength(1)
  @maxLength(128)
  operation: string;
  request: Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsOptionsA0;
}

/** Named raw-attachment declaration in the negotiated operation catalog. */
model IpcAttachmentDeclarationA0 {
  @minLength(1)
  @maxLength(128)
  name: string;
  required: boolean;

  @minItems(1)
  @maxItems(16)
  media_types: string[];

  @maxValue(268435456)
  max_bytes: uint32;
}

enum IpcRuntimeDispatchA0 {
  logical_dto: "logical_dto",
  packed_attachment: "packed_attachment",
}

model IpcPackedProjectionA0 {
  kind: "packed_attachment";
  @minLength(1) @maxLength(128) attachment_name: string;
  @minLength(1) @maxLength(128) format: string;
}

/** One operation exposed by the negotiated generic transport. */
model IpcOperationDeclarationA0 {
  @minLength(1)
  @maxLength(128)
  identity: string;

  @minLength(1)
  @maxLength(128)
  request_contract: string;

  @minLength(1)
  @maxLength(128)
  result_contract: string;

  runtime_dispatch: IpcRuntimeDispatchA0;

  @maxItems(16)
  input_attachments: IpcAttachmentDeclarationA0[];

  @maxItems(16)
  output_attachments: IpcAttachmentDeclarationA0[];

  request_projection?: IpcPackedProjectionA0;
  result_projection?: IpcPackedProjectionA0;
}

/** wasm32 attachment-descriptor member offsets. */
model IpcAttachmentOffsetsWasm32A0 {
  @maxValue(0) struct_size: uint32;
  @minValue(4) @maxValue(4) flags: uint32;
  @minValue(8) @maxValue(8) name: uint32;
  @minValue(12) @maxValue(12) name_size: uint32;
  @minValue(16) @maxValue(16) media_type: uint32;
  @minValue(20) @maxValue(20) media_type_size: uint32;
  @minValue(24) @maxValue(24) data: uint32;
  @minValue(28) @maxValue(28) data_size: uint32;
  @minValue(32) @maxValue(32) reserved0: uint32;
}

/** 64-bit attachment-descriptor member offsets. */
model IpcAttachmentOffsetsPointer64A0 {
  @maxValue(0) struct_size: uint32;
  @minValue(4) @maxValue(4) flags: uint32;
  @minValue(8) @maxValue(8) name: uint32;
  @minValue(16) @maxValue(16) name_size: uint32;
  @minValue(24) @maxValue(24) media_type: uint32;
  @minValue(32) @maxValue(32) media_type_size: uint32;
  @minValue(40) @maxValue(40) data: uint32;
  @minValue(48) @maxValue(48) data_size: uint32;
  @minValue(52) @maxValue(52) reserved0: uint32;
}

model IpcAttachmentLayoutWasm32A0 {
  @minValue(36) @maxValue(36) size: uint32;
  offsets: IpcAttachmentOffsetsWasm32A0;
}

model IpcAttachmentLayoutPointer64A0 {
  @minValue(56) @maxValue(56) size: uint32;
  offsets: IpcAttachmentOffsetsPointer64A0;
}

model IpcAttachmentDescriptorA0 {
  wasm32: IpcAttachmentLayoutWasm32A0;
  pointer64: IpcAttachmentLayoutPointer64A0;
}

/** Bounds shared by the generic C ABI operation catalog. */
model IpcGenericAbiLimitsA0 {
  @maxValue(128)
  operation_id_bytes: uint32;
  @maxValue(8388608)
  request_json_bytes: uint32;
  @maxValue(8388608)
  response_json_bytes: uint32;
  @maxValue(16)
  attachment_count: uint32;
  @maxValue(128)
  attachment_name_bytes: uint32;
  @maxValue(128)
  attachment_media_type_bytes: uint32;
  @maxValue(268435456)
  attachment_bytes: uint32;
  @maxValue(536870912)
  aggregate_attachment_bytes_native: uint32;
  @maxValue(268435456)
  aggregate_attachment_bytes_wasm: uint32;
}

/** Exact operation catalog embedded into the welcome frame. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometer.ipc.operation_catalog:a0")
@contractIdentity("geometer.ipc.operation_catalog.a0")
model IpcOperationCatalogA0 {
  catalog: "wn.geometer.operation_catalog.a0";
  generic_abi: "a0";

  @minLength(1)
  @maxLength(32)
  release_version: string;

  c_abi_generation: uint32;

  @minItems(1)
  operations: IpcOperationDeclarationA0[];
  attachment_descriptor: IpcAttachmentDescriptorA0;
  limits: IpcGenericAbiLimitsA0;
}

/** Effective executable IPC A0 limits selected by the server. */
model IpcEffectiveLimitsA0 {
  @maxValue(8388608)
  json_bytes: uint32;
  @maxValue(16)
  attachment_count: uint32;
  @maxValue(128)
  attachment_name_bytes: uint32;
  @maxValue(128)
  attachment_media_type_bytes: uint32;
  @maxValue(268435456)
  attachment_bytes: uint32;
  @maxValue(536870912)
  frame_bytes: uint32;
  @maxValue(8)
  queued_requests: uint32;
  @maxValue(536870912)
  queued_bytes: uint32;
  @maxValue(536870912)
  resident_request_bytes: uint32;
  @maxValue(536870912)
  pending_writer_bytes: uint32;
}

/** Server handshake selecting executable IPC A0 and its effective limits. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometer.ipc.welcome:a0")
@contractIdentity("geometer.ipc.welcome.a0")
model IpcWelcomeA0 {
  @minLength(1)
  @maxLength(32)
  release_version: string;
  c_abi_generation: uint32;
  ipc: "a0";

  @minLength(64)
  @maxLength(64)
  catalog_sha256: string;

  operation_catalog: IpcOperationCatalogA0;
  limits: IpcEffectiveLimitsA0;

  @maxItems(64)
  capabilities: string[];
}

/** Successful queue-only cancellation outcome. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometer.ipc.cancelled:a0")
@contractIdentity("geometer.ipc.cancelled.a0")
model IpcCancelledA0 {
  status: "cancelled";
}

/** Nonterminal cancellation rejection for an active or unknown request. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometer.ipc.cancel_rejected:a0")
@contractIdentity("geometer.ipc.cancel_rejected.a0")
model IpcCancelRejectedA0 {
  status: "rejected";
  diagnostic: Wavenumber.Geometer.Contracts.Common.DiagnosticA0;
}

/** Fatal connection-level protocol diagnostic. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometer.ipc.protocol_error:a0")
@contractIdentity("geometer.ipc.protocol_error.a0")
model IpcProtocolErrorA0 {
  status: "protocol_error";
  diagnostic: Wavenumber.Geometer.Contracts.Common.DiagnosticA0;
}

/** Final acknowledgment after every accepted request is terminal and flushed. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometer.ipc.shutdown_ack:a0")
@contractIdentity("geometer.ipc.shutdown_ack.a0")
model IpcShutdownAckA0 {
  status: "complete";
  activeRequestCompleted: boolean;
  rejectedQueuedRequestCount: uint32;
}

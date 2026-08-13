import "@typespec/json-schema";
import "@wavenumber/wn-geometer-contract-emitter";

using TypeSpec.JsonSchema;
using Wavenumber.Geometer.ContractMetadata;

namespace Wavenumber.Geometer.Contracts.Common;

/** Stable diagnostic category with distinct transport, contract, and operation domains. */
enum DiagnosticCategory {
  transport: "transport",
  contract: "contract",
  operation: "operation",
}

/** A strict cross-transport diagnostic; message text is informative rather than identity. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.common.diagnostic:a0")
@contractIdentity("geometry.common.diagnostic.a0")
model DiagnosticA0 {
  /** Stable namespaced diagnostic identity. */
  @minLength(1)
  code: string;

  category: DiagnosticCategory;

  /** Human-readable non-normative detail. */
  message: string;

  retryable: boolean;

  /** RFC 6901 JSON Pointer when a document location is meaningful. */
  path?: string;

  /** Stable operation identity when the diagnostic is operation-scoped. */
  operation?: string;

  /** Transport correlation identifier when available. */
  request_id?: string;
}

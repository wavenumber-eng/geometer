import "@typespec/json-schema";
import "@wavenumber/wn-geometer-contract-emitter";

using TypeSpec.JsonSchema;
using Wavenumber.Geometer.ContractMetadata;

namespace Wavenumber.Geometer.Tests;

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.test.fail-open.request:a0")
@contractIdentity("geometry.test.fail_open.request.a0")
model FailOpenRequest {
  value: string;
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.test.fail-open.result:a0")
@contractIdentity("geometry.test.fail_open.result.a0")
model FailOpenResult {
  value: string;
}

@nativeExperimentalOperation
@operationIdentity("geometry.test.fail_open.a0")
op failOpen(request: FailOpenRequest): FailOpenResult;

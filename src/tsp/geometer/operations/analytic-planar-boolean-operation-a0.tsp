import "@wavenumber/wn-geometer-contract-emitter";
import "./analytic-planar-boolean-a0.tsp";

using Wavenumber.Geometer.ContractMetadata;

namespace Wavenumber.Geometer.Contracts.Operations;

/** Execute independent ordered analytic planar Boolean jobs through packed attachments. */
@operationIdentity("geometry.analytic_planar_boolean_batch.a0")
@inputAttachment(
  "analytic_planar_boolean_request",
  true,
  "application/vnd.wavenumber.geometer.analytic-planar-boolean-request",
  268435456
)
@outputAttachment(
  "analytic_planar_boolean_result",
  true,
  "application/vnd.wavenumber.geometer.analytic-planar-boolean-result",
  268435456
)
@requestPackedProjection(
  "analytic_planar_boolean_request",
  "geometry.analytic_planar_boolean.packet.a0"
)
@resultPackedProjection(
  "analytic_planar_boolean_result",
  "geometry.analytic_planar_boolean.packet.a0"
)
op analyticPlanarBooleanBatch(
  request: Wavenumber.Geometer.Contracts.AnalyticPlanarBooleanA0.AnalyticPlanarBooleanBatchRequestA0
): Wavenumber.Geometer.Contracts.AnalyticPlanarBooleanA0.AnalyticPlanarBooleanBatchResultA0;

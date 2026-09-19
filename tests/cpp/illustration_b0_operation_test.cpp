#include "geometer/mesh_illustration.h"
#include "geometer/operation_registry.h"
#include "geometer/operation_transport.h"

#include <iostream>
#include <stdexcept>

namespace
{
using namespace geometer;
using namespace geometer::contracts;

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

MeshCollectionA0 collection()
{
    MeshIllustrationMaterial material;
    material.color = {0.2, 0.6, 0.8};
    material.opacity = 1;
    MeshIllustrationMesh mesh;
    mesh.id = "crossing";
    mesh.positions = {-1, 0, 0, 1, 0, 0, 1, 1, 0};
    mesh.normals = std::vector<double>{0, 0, 1, 0, 0, 1, 0, 0, 1};
    mesh.indices = std::vector<std::uint32_t>{0, 1, 2};
    mesh.materials = {material};
    MeshCollectionA0 result;
    result.meshes.push_back(std::move(mesh));
    return result;
}

IllustrationClipping clipping(double distance)
{
    HalfSpacePlane plane;
    plane.normal = {1, 0, 0};
    plane.distance_mm = distance;
    IllustrationClipping result;
    result.planes.push_back(std::move(plane));
    return result;
}

bool same_stats(const MeshIllustrationRenderStats& first, const MeshIllustrationRenderStats& second)
{
    return first.triangles == second.triangles && first.surface_draws == second.surface_draws &&
           first.layered_surfaces == second.layered_surfaces && first.outlines == second.outlines &&
           first.details == second.details && first.creases == second.creases &&
           first.commands == second.commands;
}

template <typename Request>
OperationExecution invoke(const std::string& operation, const Request& request,
                          const std::string& mesh_json, const std::string* linework_json = nullptr)
{
    ContractError error;
    std::string request_json;
    require(encode_json(request, &request_json, &error), "B0 request should encode");
    std::vector<OperationAttachmentView> attachments = {
        {"mesh_collection", "application/vnd.wavenumber.geometer.mesh-collection+json",
         reinterpret_cast<const unsigned char*>(mesh_json.data()), mesh_json.size()}};
    if (linework_json)
        attachments.push_back(
            {"hlr_projection", "application/vnd.wavenumber.geometer.hlr-projection+json",
             reinterpret_cast<const unsigned char*>(linework_json->data()), linework_json->size()});
    OperationExecution execution;
    execute_operation(operation, reinterpret_cast<const unsigned char*>(request_json.data()),
                      request_json.size(), attachments, &execution);
    return execution;
}

void b0_operations_share_one_fragment_identity()
{
    ContractError error;
    std::string mesh_json;
    require(encode_json(collection(), &mesh_json, &error), "mesh collection should encode");

    MeshHlrProjectionRequestB0 hlr_request;
    hlr_request.views = std::vector<HlrViewSpec>{{"front", {0, 0, 1}, {0, 1, 0}}};
    hlr_request.clipping = clipping(0);
    auto hlr_execution = invoke("geometry.mesh_hlr_projection.b0", hlr_request, mesh_json);
    const auto* hlr_success = std::get_if<OperationSuccessB0>(&hlr_execution.outcome);
    require(hlr_success != nullptr, "B0 mesh HLR should return a B0 success");
    const auto* hlr = std::get_if<HlrProjectionResultB0>(&hlr_success->result);
    require(hlr && !hlr->empty && hlr->fragment.output_triangles == 2,
            "B0 HLR should project the clipped fragment");
    std::string hlr_json;
    require(encode_json(*hlr, &hlr_json, &error), "B0 HLR result should encode");

    MeshIllustrationRequestB0 illustration_request;
    illustration_request.view.direction = {0, 0, 1};
    illustration_request.view.up = {0, 1, 0};
    illustration_request.clipping = clipping(0);
    auto illustration_execution =
        invoke("geometry.mesh_illustration.b0", illustration_request, mesh_json, &hlr_json);
    const auto* illustration_success =
        std::get_if<OperationSuccessB0>(&illustration_execution.outcome);
    require(illustration_success != nullptr, "B0 illustration should accept matching B0 linework");
    const auto* illustration = std::get_if<MeshIllustrationResultB0>(&illustration_success->result);
    require(illustration && !illustration->empty && !illustration->svg.empty() &&
                illustration->fragment.linework_geometry_sha256 ==
                    hlr->fragment.linework_geometry_sha256,
            "shading and linework should identify the same clipped fragment");

    std::string outcome_json;
    require(encode_operation_outcome(illustration_execution.outcome, &outcome_json, &error),
            "runtime B0 outcome should encode");
    std::string validation;
    require(validate_operation_response("geometry.mesh_illustration.b0", outcome_json, {},
                                        &validation) == OperationResponseValidationStatus::ok,
            "the generic transport should validate a B0 success");
}

void a0_adapts_to_the_unclipped_b0_renderer()
{
    auto reflected = collection();
    reflected.meshes.front().matrix =
        IllustrationMatrix4x4{-1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 3, 0, 0, 1};
    ContractError error;
    std::string mesh_json;
    require(encode_json(reflected, &mesh_json, &error), "reflected collection should encode");

    MeshIllustrationRequestA0 a0_request;
    a0_request.view.direction = {0.4, 0.7, 1};
    a0_request.view.up = {0, 1, 0};
    auto a0_execution = invoke("geometry.mesh_illustration.a0", a0_request, mesh_json);
    const auto* a0_success = std::get_if<OperationSuccessA0>(&a0_execution.outcome);
    require(a0_success, "A0 compatibility request should succeed");
    const auto* a0 = std::get_if<MeshIllustrationResultA0>(&a0_success->result);
    require(a0, "A0 compatibility request should return its unchanged result root");

    MeshIllustrationRequestB0 b0_request;
    b0_request.view = a0_request.view;
    auto b0_execution = invoke("geometry.mesh_illustration.b0", b0_request, mesh_json);
    const auto* b0_success = std::get_if<OperationSuccessB0>(&b0_execution.outcome);
    require(b0_success, "unclipped B0 request should succeed");
    const auto* b0 = std::get_if<MeshIllustrationResultB0>(&b0_success->result);
    require(b0 && !b0->empty && b0->fragment.input_triangles == 1 &&
                b0->fragment.output_triangles == 1,
            "unclipped B0 should add fragment metadata without changing the source fragment");

    std::string normalized_svg = b0->svg;
    const std::string b0_metadata = "<metadata>geometry.mesh_illustration.result.b0</metadata>";
    const auto metadata_offset = normalized_svg.find(b0_metadata);
    require(metadata_offset != std::string::npos, "B0 SVG should identify its result generation");
    normalized_svg.replace(metadata_offset, b0_metadata.size(),
                           "<metadata>geometry.mesh_illustration.result.a0</metadata>");
    require(normalized_svg == a0->svg && same_stats(b0->stats, a0->stats) &&
                b0->warnings == a0->warnings,
            "A0 and unclipped B0 must share renderer behavior apart from B0 metadata");
}

void empty_geometry_is_success_without_bounds()
{
    ContractError error;
    std::string mesh_json;
    require(encode_json(collection(), &mesh_json, &error), "mesh collection should encode");
    MeshIllustrationGeometryRequestB0 request;
    request.view.direction = {0, 0, 1};
    request.view.up = {0, 1, 0};
    request.clipping = clipping(10);
    auto execution = invoke("geometry.mesh_illustration_geometry.b0", request, mesh_json);
    const auto* success = std::get_if<OperationSuccessB0>(&execution.outcome);
    require(success && execution.attachments.size() == 1,
            "empty B0 geometry should succeed with its required attachment");
    const auto* metadata = std::get_if<MeshIllustrationGeometryResultB0>(&success->result);
    require(metadata && metadata->empty && metadata->fragment.output_triangles == 0,
            "empty B0 geometry metadata should explicitly identify the empty fragment");
    MeshIllustrationGeometryB0 geometry;
    require(decode_json(execution.attachments[0].data.data(), execution.attachments[0].data.size(),
                        &geometry, &error),
            "empty B0 geometry attachment should decode");
    require(geometry.empty && !geometry.bounds && geometry.surfaces.empty(),
            "empty B0 geometry must omit bounds and surfaces");
}

void b0_direct_value_apis_clip_and_report_fragment_identity()
{
    MeshIllustrationInputB0 input;
    input.meshes = collection().meshes;
    input.view.direction = {0, 0, 1};
    input.view.up = {0, 1, 0};
    input.clipping = clipping(0);
    MeshIllustrationResultB0 rendered;
    Status status;
    require(illustrate_mesh(input, &rendered, &status) == 0 && !rendered.empty &&
                rendered.fragment.input_triangles == 1 && rendered.fragment.output_triangles == 2,
            "B0 direct SVG API should apply clipping and return fragment identity");
    require(rendered.fragment.fragment_sha256 ==
                    "30739631202635b15ea2cf56c1acc5549a49465a9187c7dc0eccd69bb56b768e" &&
                rendered.fragment.linework_geometry_sha256 ==
                    "c784211b7217d3552b81d7cf9e105e24bfc3ddb02b8b2ed61ab1c0f6de654c38",
            "governed clipping digests must remain identical across native, macOS, and WASM");

    MeshIllustrationGeometryInputB0 geometry_input;
    geometry_input.meshes = collection().meshes;
    geometry_input.view = input.view;
    geometry_input.clipping = input.clipping;
    MeshIllustrationGeometryB0 geometry;
    require(illustrate_mesh_geometry(geometry_input, &geometry, &status) == 0 && !geometry.empty &&
                geometry.bounds &&
                geometry.fragment.fragment_sha256 == rendered.fragment.fragment_sha256,
            "B0 direct geometry API should consume the same governed fragment");

    input.clipping = clipping(10);
    require(illustrate_mesh(input, &rendered, &status) == 0 && rendered.empty,
            "B0 direct SVG API must represent a fully clipped fragment as success");
    geometry_input.clipping = input.clipping;
    require(illustrate_mesh_geometry(geometry_input, &geometry, &status) == 0 && geometry.empty &&
                !geometry.bounds && geometry.surfaces.empty(),
            "B0 direct geometry API must return unmistakable empty output");
}

void operation_generation_is_catalog_selected_and_mixed_outcomes_fail()
{
    require(operation_uses_b0("geometry.mesh_illustration.b0"),
            "catalog-declared B0 request root should select B0 codecs");
    require(!operation_uses_b0("geometry.unregistered_suffix.b0"),
            "an operation suffix alone must never select a codec generation");

    OperationSuccessA0 a0_success;
    a0_success.operation = "geometry.mesh_illustration.b0";
    MeshIllustrationResultA0 a0_result;
    a0_result.svg = "<svg/>";
    a0_success.result = std::move(a0_result);
    OperationExecution::Outcome a0_outcome = std::move(a0_success);
    ContractError error;
    std::string json;
    require(encode_operation_outcome(a0_outcome, &json, &error), "A0 mixed vector should encode");
    std::string validation;
    require(validate_operation_response("geometry.mesh_illustration.b0", json, {}, &validation) ==
                OperationResponseValidationStatus::invalid,
            "B0 catalog declaration must reject an A0 outcome root");

    OperationSuccessB0 b0_success;
    b0_success.operation = "geometry.mesh_illustration.a0";
    MeshIllustrationInputB0 input;
    input.meshes = collection().meshes;
    input.view.direction = {0, 0, 1};
    input.view.up = {0, 1, 0};
    MeshIllustrationResultB0 b0_result;
    Status status;
    require(illustrate_mesh(input, &b0_result, &status) == 0,
            "valid B0 mixed vector payload should render");
    b0_success.result = std::move(b0_result);
    OperationExecution::Outcome b0_outcome = std::move(b0_success);
    require(encode_operation_outcome(b0_outcome, &json, &error), "B0 mixed vector should encode");
    require(validate_operation_response("geometry.mesh_illustration.a0", json, {}, &validation) ==
                OperationResponseValidationStatus::invalid,
            "A0 catalog declaration must reject a B0 outcome root");
}
} // namespace

int main()
{
    try
    {
        b0_operations_share_one_fragment_identity();
        a0_adapts_to_the_unclipped_b0_renderer();
        empty_geometry_is_success_without_bounds();
        b0_direct_value_apis_clip_and_report_fragment_identity();
        operation_generation_is_catalog_selected_and_mixed_outcomes_fail();
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

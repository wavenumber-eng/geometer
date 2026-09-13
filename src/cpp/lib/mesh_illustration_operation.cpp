#include "mesh_illustration_operation.h"
#include "geometer/mesh_illustration.h"
#include "geometer/sha256.h"

namespace geometer
{
void execute_mesh_illustration(const unsigned char* request, std::size_t size,
                               const std::vector<OperationAttachmentView>& attachments,
                               OperationExecution* execution, bool geometry_only)
{
    const char* operation =
        geometry_only ? "geometry.mesh_illustration_geometry.a0" : "geometry.mesh_illustration.a0";
    const auto mesh_limit = operation_input_attachment_max_bytes(operation, "mesh_collection");
    const auto linework_limit = operation_input_attachment_max_bytes(operation, "hlr_projection");
    const auto fail = [&](const std::string& code, const std::string& message,
                          contracts::DiagnosticCategory category)
    {
        contracts::DiagnosticA0 diagnostic;
        diagnostic.code = code;
        diagnostic.message = message;
        diagnostic.operation = operation;
        diagnostic.category = category;
        diagnostic.retryable = false;
        contracts::OperationFailureA0 failure;
        failure.operation = operation;
        failure.diagnostics.push_back(std::move(diagnostic));
        execution->outcome = std::move(failure);
        execution->attachments.clear();
    };
    contracts::MeshIllustrationRequestA0 options;
    contracts::ContractError error;
    bool decoded = false;
    if (geometry_only)
    {
        contracts::MeshIllustrationGeometryRequestA0 geometry_options;
        decoded = contracts::decode_json(request, size, &geometry_options, &error);
        options.view = std::move(geometry_options.view);
        options.prepare = std::move(geometry_options.prepare);
        options.style = std::move(geometry_options.style);
    }
    else
        decoded = contracts::decode_json(request, size, &options, &error);
    if (!decoded)
    {
        fail(error.code, error.message, contracts::DiagnosticCategory::contract);
        return;
    }
    const OperationAttachmentView* meshes = nullptr;
    const OperationAttachmentView* linework = nullptr;
    for (const auto& attachment : attachments)
    {
        if (attachment.name == "mesh_collection" && !meshes &&
            attachment.media_type == "application/vnd.wavenumber.geometer.mesh-collection+json" &&
            attachment.size <= mesh_limit)
            meshes = &attachment;
        else if (attachment.name == "hlr_projection" && !linework &&
                 attachment.media_type ==
                     "application/vnd.wavenumber.geometer.hlr-projection+json" &&
                 attachment.size <= linework_limit)
            linework = &attachment;
        else
        {
            fail("geometer.contract.invalid_attachment",
                 "Unexpected, duplicate or invalid illustration attachment.",
                 contracts::DiagnosticCategory::contract);
            return;
        }
    }
    if (!meshes)
    {
        fail("geometer.contract.invalid_attachment",
             "Expected a bounded mesh_collection JSON attachment.",
             contracts::DiagnosticCategory::contract);
        return;
    }
    contracts::MeshCollectionA0 collection;
    if (!contracts::decode_json(meshes->data, meshes->size, &collection, &error, mesh_limit))
    {
        fail(error.code, error.message, contracts::DiagnosticCategory::contract);
        return;
    }
    contracts::MeshIllustrationInputA0 input;
    input.meshes = std::move(collection.meshes);
    input.view = std::move(options.view);
    input.prepare = std::move(options.prepare);
    input.style = std::move(options.style);
    input.svg = std::move(options.svg);
    contracts::MeshIllustrationResultA0 result;
    Status status;
    contracts::HlrProjectionResultA0 hlr;
    if (linework &&
        !contracts::decode_json(linework->data, linework->size, &hlr, &error, linework_limit))
    {
        fail(error.code, error.message, contracts::DiagnosticCategory::contract);
        return;
    }
    contracts::MeshIllustrationGeometryA0 geometry;
    int code;
    if (geometry_only)
    {
        contracts::MeshIllustrationGeometryInputA0 geometry_input;
        geometry_input.meshes = std::move(input.meshes);
        geometry_input.view = std::move(input.view);
        geometry_input.prepare = std::move(input.prepare);
        geometry_input.style = std::move(input.style);
        code = linework ? illustrate_mesh_geometry(geometry_input, hlr, &geometry, &status)
                        : illustrate_mesh_geometry(geometry_input, &geometry, &status);
    }
    else
        code = linework ? illustrate_mesh(input, hlr, &result, &status)
                        : illustrate_mesh(input, &result, &status);
    if (code != 0)
    {
        fail(code == 102 ? "geometer.operation.resource_limit_exceeded"
                         : "geometer.operation.illustration_failed",
             status.message, contracts::DiagnosticCategory::operation);
        return;
    }
    contracts::OperationSuccessA0 success;
    success.operation = operation;
    if (geometry_only)
    {
        std::string json;
        if (!contracts::encode_json(geometry, &json, &error))
        {
            fail(error.code, error.message, contracts::DiagnosticCategory::operation);
            return;
        }
        const auto geometry_limit =
            operation_output_attachment_max_bytes(operation, "illustration_geometry");
        if (json.size() > geometry_limit)
        {
            fail("geometer.operation.resource_limit_exceeded",
                 "Illustration geometry JSON exceeds 256 MiB.",
                 contracts::DiagnosticCategory::operation);
            return;
        }
        contracts::MeshIllustrationGeometryResultA0 metadata;
        metadata.stats = geometry.stats;
        metadata.warnings = std::move(geometry.warnings);
        metadata.geometry.byte_length = static_cast<std::uint32_t>(json.size());
        metadata.geometry.sha256 =
            sha256_hex(reinterpret_cast<const std::uint8_t*>(json.data()), json.size());
        success.result = std::move(metadata);
        execution->outcome = std::move(success);
        execution->attachments = {{"illustration_geometry",
                                   "application/vnd.wavenumber.geometer.illustration-geometry+json",
                                   std::vector<unsigned char>(json.begin(), json.end())}};
        return;
    }
    success.result = std::move(result);
    execution->outcome = std::move(success);
    execution->attachments.clear();
}
} // namespace geometer

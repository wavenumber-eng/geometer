#include "model_tessellation_operation.h"
#include "geometer/model_tessellation.h"
#include "geometer/sha256.h"

#include <cstdint>
#include <utility>

namespace geometer
{
void execute_model_tessellation(const unsigned char* request, std::size_t size,
                                const std::vector<OperationAttachmentView>& attachments,
                                OperationExecution* execution)
{
    constexpr const char* operation = "geometry.model_tessellation.a0";
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
    contracts::ModelTessellationRequestA0 options;
    contracts::ContractError error;
    if (!contracts::decode_json(request, size, &options, &error))
    {
        fail(error.code, error.message, contracts::DiagnosticCategory::contract);
        return;
    }
    if (attachments.size() != 1 || attachments[0].name != "model" ||
        (attachments[0].media_type != "application/step" &&
         attachments[0].media_type != "model/step") ||
        attachments[0].size > 268435456)
    {
        fail("geometer.contract.invalid_attachment",
             "Expected one bounded STEP attachment named model.",
             contracts::DiagnosticCategory::contract);
        return;
    }
    const auto& model = attachments[0];
    contracts::MeshCollectionA0 collection;
    Status status;
    const int code =
        model_tessellation_from_bytes(model.data, model.size, options, &collection, &status);
    if (code != 0)
    {
        fail(code == 102   ? "geometer.operation.resource_limit_exceeded"
             : code == 103 ? "geometer.contract.external_model_reference"
                           : "geometer.operation.tessellation_failed",
             status.message,
             code == 103 ? contracts::DiagnosticCategory::contract
                         : contracts::DiagnosticCategory::operation);
        return;
    }
    std::string json;
    if (!contracts::encode_json(collection, &json, &error))
    {
        fail(error.code, error.message, contracts::DiagnosticCategory::operation);
        return;
    }
    if (json.size() > 268435456)
    {
        fail("geometer.operation.resource_limit_exceeded", "Mesh collection JSON exceeds 256 MiB.",
             contracts::DiagnosticCategory::operation);
        return;
    }
    contracts::ModelTessellationResultA0 result;
    result.mesh_collection.byte_length = static_cast<std::uint32_t>(json.size());
    result.mesh_collection.sha256 =
        sha256_hex(reinterpret_cast<const std::uint8_t*>(json.data()), json.size());
    result.source_sha256 = sha256_hex(model.data, model.size);
    result.meshes = static_cast<std::uint32_t>(collection.meshes.size());
    for (const auto& mesh : collection.meshes)
        result.triangles += static_cast<std::uint32_t>(mesh.indices->size() / 3);
    contracts::OperationSuccessA0 success;
    success.operation = operation;
    success.result = std::move(result);
    execution->outcome = std::move(success);
    execution->attachments = {{"mesh_collection",
                               "application/vnd.wavenumber.geometer.mesh-collection+json",
                               std::vector<unsigned char>(json.begin(), json.end())}};
}
} // namespace geometer

#include "mesh_illustration_operation.h"
#include "geometer/mesh_illustration.h"

namespace geometer
{
void execute_mesh_illustration(const unsigned char* request, std::size_t size,
                               const std::vector<OperationAttachmentView>& attachments,
                               OperationExecution* execution)
{
    constexpr const char* operation = "geometry.mesh_illustration.a0";
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
    if (!contracts::decode_json(request, size, &options, &error))
    {
        fail(error.code, error.message, contracts::DiagnosticCategory::contract);
        return;
    }
    if (attachments.size() != 1 || attachments[0].name != "mesh_collection" ||
        attachments[0].media_type != "application/vnd.wavenumber.geometer.mesh-collection+json" ||
        attachments[0].size > 268435456)
    {
        fail("geometer.contract.invalid_attachment",
             "Expected one bounded mesh_collection JSON attachment.",
             contracts::DiagnosticCategory::contract);
        return;
    }
    contracts::MeshCollectionA0 collection;
    if (!contracts::decode_json(attachments[0].data, attachments[0].size, &collection, &error))
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
    const auto code = illustrate_mesh(input, &result, &status);
    if (code != 0)
    {
        fail(code == 102 ? "geometer.operation.resource_limit_exceeded"
                         : "geometer.operation.illustration_failed",
             status.message, contracts::DiagnosticCategory::operation);
        return;
    }
    contracts::OperationSuccessA0 success;
    success.operation = operation;
    success.result = std::move(result);
    execution->outcome = std::move(success);
    execution->attachments.clear();
}
} // namespace geometer

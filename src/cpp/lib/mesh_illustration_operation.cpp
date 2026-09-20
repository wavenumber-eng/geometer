#include "mesh_illustration_operation.h"
#include "geometer/mesh_illustration.h"
#include "geometer/sha256.h"
#include "illustration_clipping.h"
#include "mesh_illustration_internal.h"

namespace geometer
{
namespace
{
contracts::HlrProjectionResultA0 as_a0(const contracts::HlrProjectionResultB0& source)
{
    contracts::HlrProjectionResultA0 result;
    result.source.kind = contracts::HlrSourceKind::indexed_mesh;
    result.source.hash = source.source.hash;
    result.views = source.views;
    result.timings = source.timings;
    return result;
}

int set_direct_status(Status* status, int code, std::string message)
{
    if (status)
        *status = {code, std::move(message)};
    return code;
}

int render_mesh_illustration(bool geometry_only, bool empty,
                             contracts::MeshIllustrationInputA0 input,
                             const contracts::HlrProjectionResultA0* hlr,
                             const MeshIllustrationExecutionLimits& limits,
                             contracts::MeshIllustrationResultA0* rendered,
                             contracts::MeshIllustrationGeometryA0* geometry, Status* status)
{
    if (geometry_only)
    {
        contracts::MeshIllustrationGeometryInputA0 geometry_input;
        geometry_input.meshes = std::move(input.meshes);
        geometry_input.view = std::move(input.view);
        geometry_input.prepare = std::move(input.prepare);
        geometry_input.style = std::move(input.style);
        if (empty)
        {
            const illustration_detail::IllustrationInputView view{
                geometry_input.meshes, geometry_input.view, geometry_input.prepare,
                geometry_input.style};
            const auto prepared = illustration_detail::prepare_illustration(view, hlr);
            *geometry =
                illustration_detail::make_illustration_geometry(prepared, geometry_input.view);
            return 0;
        }
        return illustrate_mesh_geometry(geometry_input, hlr, limits, geometry, status);
    }
    if (empty)
    {
        const illustration_detail::IllustrationInputView view{input.meshes, input.view,
                                                              input.prepare, input.style};
        const auto prepared = illustration_detail::prepare_illustration(view, hlr);
        rendered->svg = illustration_detail::render_svg(
            prepared.scene, prepared.style, prepared.commands,
            input.svg.value_or(contracts::MeshIllustrationSvgOptions{}),
            "geometry.mesh_illustration.result.b0");
        rendered->stats = prepared.commands.stats;
        rendered->warnings = prepared.scene.warnings;
        return 0;
    }
    return illustrate_mesh(input, hlr, limits, rendered, status);
}

void execute_mesh_illustration_b0(const std::string& operation, const unsigned char* request,
                                  std::size_t size,
                                  const std::vector<OperationAttachmentView>& attachments,
                                  OperationExecution* execution)
{
    const bool geometry_only = operation == "geometry.mesh_illustration_geometry.b0";
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
        contracts::OperationFailureB0 failure;
        failure.operation = operation;
        failure.diagnostics.push_back(std::move(diagnostic));
        execution->outcome = std::move(failure);
        execution->attachments.clear();
    };

    contracts::MeshIllustrationRequestB0 options;
    contracts::ContractError error;
    bool decoded = false;
    if (geometry_only)
    {
        contracts::MeshIllustrationGeometryRequestB0 geometry_options;
        decoded = contracts::decode_json(request, size, &geometry_options, &error);
        options.view = std::move(geometry_options.view);
        options.prepare = std::move(geometry_options.prepare);
        options.style = std::move(geometry_options.style);
        options.clipping = std::move(geometry_options.clipping);
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
    std::vector<contracts::MeshIllustrationMesh> legacy_renderer_meshes;
    if (!options.clipping)
        legacy_renderer_meshes = collection.meshes;
    const std::string raw_sha = sha256_hex(meshes->data, meshes->size);
    PreparedIllustrationFragment fragment;
    Status status;
    const int clip_code = prepare_illustration_fragment(
        std::move(collection.meshes), options.clipping, {}, raw_sha, &fragment, &status);
    if (clip_code != 0)
    {
        fail(clip_code == 102 ? "geometer.operation.resource_limit_exceeded"
                              : "geometer.contract.invalid_clipping",
             status.message,
             clip_code == 102 ? contracts::DiagnosticCategory::operation
                              : contracts::DiagnosticCategory::contract);
        return;
    }

    std::optional<contracts::HlrProjectionResultA0> hlr;
    if (linework)
    {
        contracts::HlrProjectionResultB0 supplied;
        if (!contracts::decode_json(linework->data, linework->size, &supplied, &error,
                                    linework_limit))
        {
            fail(error.code, error.message, contracts::DiagnosticCategory::contract);
            return;
        }
        if (supplied.fragment.linework_geometry_sha256 !=
            fragment.metadata.linework_geometry_sha256)
        {
            fail("geometer.contract.linework_fragment_mismatch",
                 "Supplied linework was not produced from the same transformed and clipped "
                 "fragment.",
                 contracts::DiagnosticCategory::contract);
            return;
        }
        hlr = as_a0(supplied);
    }

    contracts::MeshIllustrationInputA0 input;
    input.meshes = options.clipping ? std::move(fragment.collection.meshes)
                                    : std::move(legacy_renderer_meshes);
    input.view = std::move(options.view);
    input.prepare = std::move(options.prepare);
    input.style = std::move(options.style);
    input.svg = std::move(options.svg);
    contracts::MeshIllustrationResultA0 rendered;
    contracts::MeshIllustrationGeometryA0 geometry;
    int code = 0;
    try
    {
        code = render_mesh_illustration(geometry_only, fragment.empty, std::move(input),
                                        hlr ? &*hlr : nullptr, {}, &rendered, &geometry, &status);
    }
    catch (const std::exception& exception)
    {
        fail("geometer.operation.illustration_failed", exception.what(),
             contracts::DiagnosticCategory::operation);
        return;
    }
    if (code != 0)
    {
        fail(code == 102 ? "geometer.operation.resource_limit_exceeded"
                         : "geometer.operation.illustration_failed",
             status.message, contracts::DiagnosticCategory::operation);
        return;
    }
    const std::string a0_metadata = "<metadata>geometry.mesh_illustration.result.a0</metadata>";
    const auto metadata_offset = rendered.svg.find(a0_metadata);
    if (metadata_offset != std::string::npos)
        rendered.svg.replace(metadata_offset, a0_metadata.size(),
                             "<metadata>geometry.mesh_illustration.result.b0</metadata>");

    contracts::OperationSuccessB0 success;
    success.operation = operation;
    if (!geometry_only)
    {
        contracts::MeshIllustrationResultB0 result;
        result.empty = fragment.empty;
        result.svg = std::move(rendered.svg);
        result.stats = rendered.stats;
        result.fragment = std::move(fragment.metadata);
        result.warnings = std::move(rendered.warnings);
        success.result = std::move(result);
        execution->outcome = std::move(success);
        execution->attachments.clear();
        return;
    }

    contracts::MeshIllustrationGeometryB0 wire_geometry;
    wire_geometry.empty = fragment.empty;
    wire_geometry.view = std::move(geometry.view);
    if (!fragment.empty)
        wire_geometry.bounds = std::move(geometry.bounds);
    wire_geometry.surfaces = std::move(geometry.surfaces);
    wire_geometry.lines = std::move(geometry.lines);
    wire_geometry.presentation = std::move(geometry.presentation);
    wire_geometry.stats = geometry.stats;
    wire_geometry.fragment = fragment.metadata;
    wire_geometry.warnings = std::move(geometry.warnings);
    std::string json;
    if (!contracts::encode_json(wire_geometry, &json, &error))
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
    contracts::MeshIllustrationGeometryResultB0 result;
    result.empty = fragment.empty;
    result.stats = wire_geometry.stats;
    result.fragment = std::move(fragment.metadata);
    result.warnings = wire_geometry.warnings;
    result.geometry.byte_length = static_cast<std::uint32_t>(json.size());
    result.geometry.sha256 =
        sha256_hex(reinterpret_cast<const std::uint8_t*>(json.data()), json.size());
    success.result = std::move(result);
    execution->outcome = std::move(success);
    execution->attachments = {{"illustration_geometry",
                               "application/vnd.wavenumber.geometer.illustration-geometry+json",
                               std::vector<unsigned char>(json.begin(), json.end())}};
}
} // namespace

int illustrate_mesh(const contracts::MeshIllustrationInputB0& input,
                    const contracts::HlrProjectionResultB0* supplied_hlr,
                    const MeshIllustrationExecutionLimits& limits,
                    contracts::MeshIllustrationResultB0* result, Status* status)
{
    if (result)
        *result = {};
    if (status)
        *status = {};
    if (!result)
        return set_direct_status(status, 1, "Mesh illustration B0 result pointer is null.");
    try
    {
        std::vector<contracts::MeshIllustrationMesh> renderer_meshes;
        if (!input.clipping)
            renderer_meshes = input.meshes;
        PreparedIllustrationFragment fragment;
        Status nested;
        int code =
            prepare_illustration_fragment(input.meshes, input.clipping, {}, {}, &fragment, &nested);
        if (code != 0)
            return set_direct_status(status, code, std::move(nested.message));
        std::optional<contracts::HlrProjectionResultA0> hlr;
        if (supplied_hlr)
        {
            if (supplied_hlr->fragment.linework_geometry_sha256 !=
                fragment.metadata.linework_geometry_sha256)
                return set_direct_status(
                    status, 1,
                    "Supplied B0 linework was not produced from the same transformed and clipped "
                    "fragment.");
            hlr = as_a0(*supplied_hlr);
        }
        contracts::MeshIllustrationInputA0 legacy;
        legacy.meshes =
            input.clipping ? std::move(fragment.collection.meshes) : std::move(renderer_meshes);
        legacy.view = input.view;
        legacy.prepare = input.prepare;
        legacy.style = input.style;
        legacy.svg = input.svg;
        contracts::MeshIllustrationResultA0 rendered;
        contracts::MeshIllustrationGeometryA0 unused_geometry;
        code = render_mesh_illustration(false, fragment.empty, std::move(legacy),
                                        hlr ? &*hlr : nullptr, limits, &rendered, &unused_geometry,
                                        &nested);
        if (code != 0)
            return set_direct_status(status, code, std::move(nested.message));
        const std::string a0_metadata = "<metadata>geometry.mesh_illustration.result.a0</metadata>";
        const auto metadata_offset = rendered.svg.find(a0_metadata);
        if (metadata_offset != std::string::npos)
            rendered.svg.replace(metadata_offset, a0_metadata.size(),
                                 "<metadata>geometry.mesh_illustration.result.b0</metadata>");
        result->empty = fragment.empty;
        result->svg = std::move(rendered.svg);
        result->stats = rendered.stats;
        result->fragment = std::move(fragment.metadata);
        result->warnings = std::move(rendered.warnings);
        return 0;
    }
    catch (const std::bad_alloc&)
    {
        *result = {};
        return set_direct_status(status, 102, "Mesh illustration B0 allocation failed.");
    }
    catch (const std::exception& error)
    {
        *result = {};
        return set_direct_status(status, 1, error.what());
    }
}

int illustrate_mesh(const contracts::MeshIllustrationInputB0& input,
                    contracts::MeshIllustrationResultB0* result, Status* status)
{
    return illustrate_mesh(input, nullptr, {}, result, status);
}

int illustrate_mesh(const contracts::MeshIllustrationInputB0& input,
                    const contracts::HlrProjectionResultB0& hlr,
                    contracts::MeshIllustrationResultB0* result, Status* status)
{
    return illustrate_mesh(input, &hlr, {}, result, status);
}

int illustrate_mesh_geometry(const contracts::MeshIllustrationGeometryInputB0& input,
                             const contracts::HlrProjectionResultB0* supplied_hlr,
                             const MeshIllustrationExecutionLimits& limits,
                             contracts::MeshIllustrationGeometryB0* result, Status* status)
{
    if (result)
        *result = {};
    if (status)
        *status = {};
    if (!result)
        return set_direct_status(status, 1,
                                 "Mesh illustration geometry B0 result pointer is null.");
    try
    {
        std::vector<contracts::MeshIllustrationMesh> renderer_meshes;
        if (!input.clipping)
            renderer_meshes = input.meshes;
        PreparedIllustrationFragment fragment;
        Status nested;
        int code =
            prepare_illustration_fragment(input.meshes, input.clipping, {}, {}, &fragment, &nested);
        if (code != 0)
            return set_direct_status(status, code, std::move(nested.message));
        std::optional<contracts::HlrProjectionResultA0> hlr;
        if (supplied_hlr)
        {
            if (supplied_hlr->fragment.linework_geometry_sha256 !=
                fragment.metadata.linework_geometry_sha256)
                return set_direct_status(
                    status, 1,
                    "Supplied B0 linework was not produced from the same transformed and clipped "
                    "fragment.");
            hlr = as_a0(*supplied_hlr);
        }
        contracts::MeshIllustrationInputA0 legacy;
        legacy.meshes =
            input.clipping ? std::move(fragment.collection.meshes) : std::move(renderer_meshes);
        legacy.view = input.view;
        legacy.prepare = input.prepare;
        legacy.style = input.style;
        contracts::MeshIllustrationGeometryA0 geometry;
        contracts::MeshIllustrationResultA0 unused_rendered;
        code =
            render_mesh_illustration(true, fragment.empty, std::move(legacy), hlr ? &*hlr : nullptr,
                                     limits, &unused_rendered, &geometry, &nested);
        if (code != 0)
            return set_direct_status(status, code, std::move(nested.message));
        result->empty = fragment.empty;
        result->view = std::move(geometry.view);
        if (!fragment.empty)
            result->bounds = std::move(geometry.bounds);
        result->surfaces = std::move(geometry.surfaces);
        result->lines = std::move(geometry.lines);
        result->presentation = std::move(geometry.presentation);
        result->stats = geometry.stats;
        result->fragment = std::move(fragment.metadata);
        result->warnings = std::move(geometry.warnings);
        return 0;
    }
    catch (const std::bad_alloc&)
    {
        *result = {};
        return set_direct_status(status, 102, "Mesh illustration geometry B0 allocation failed.");
    }
    catch (const std::exception& error)
    {
        *result = {};
        return set_direct_status(status, 1, error.what());
    }
}

int illustrate_mesh_geometry(const contracts::MeshIllustrationGeometryInputB0& input,
                             contracts::MeshIllustrationGeometryB0* result, Status* status)
{
    return illustrate_mesh_geometry(input, nullptr, {}, result, status);
}

int illustrate_mesh_geometry(const contracts::MeshIllustrationGeometryInputB0& input,
                             const contracts::HlrProjectionResultB0& hlr,
                             contracts::MeshIllustrationGeometryB0* result, Status* status)
{
    return illustrate_mesh_geometry(input, &hlr, {}, result, status);
}

void execute_mesh_illustration(const std::string& operation_id, const unsigned char* request,
                               std::size_t size,
                               const std::vector<OperationAttachmentView>& attachments,
                               OperationExecution* execution)
{
    if (operation_uses_b0(operation_id))
    {
        execute_mesh_illustration_b0(operation_id, request, size, attachments, execution);
        return;
    }
    const bool geometry_only = operation_id == "geometry.mesh_illustration_geometry.a0";
    const char* operation = operation_id.c_str();
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
    const int code =
        render_mesh_illustration(geometry_only, false, std::move(input), linework ? &hlr : nullptr,
                                 {}, &result, &geometry, &status);
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

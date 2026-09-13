#include "model_illustration_operation.h"
#include "analytic_illustration_lowering.h"
#include "mesh_illustration_internal.h"
#include "model_illustration_transform.h"

#include "geometer/fast_hlr.h"
#include "geometer/mesh_illustration.h"
#include "geometer/model_illustration.h"
#include "geometer/model_tessellation.h"
#include "geometer/projection.h"
#include "geometer/sha256.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>

namespace geometer
{
namespace
{
using Clock = std::chrono::steady_clock;

double milliseconds(Clock::time_point start)
{
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

void fail(OperationExecution* execution, const char* operation, std::string code,
          std::string message, contracts::DiagnosticCategory category)
{
    contracts::DiagnosticA0 diagnostic;
    diagnostic.code = std::move(code);
    diagnostic.message = std::move(message);
    diagnostic.operation = operation;
    diagnostic.category = category;
    diagnostic.retryable = false;
    contracts::OperationFailureA0 failure;
    failure.operation = operation;
    failure.diagnostics.push_back(std::move(diagnostic));
    execution->outcome = std::move(failure);
    execution->attachments.clear();
}

std::array<double, 16> matrix_or_identity(const std::optional<std::vector<double>>& value)
{
    std::array<double, 16> result{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    if (value)
        std::copy(value->begin(), value->end(), result.begin());
    return result;
}

std::array<double, 3> transform_point(const std::array<double, 16>& matrix, double x, double y,
                                      double z)
{
    return {matrix[0] * x + matrix[4] * y + matrix[8] * z + matrix[12],
            matrix[1] * x + matrix[5] * y + matrix[9] * z + matrix[13],
            matrix[2] * x + matrix[6] * y + matrix[10] * z + matrix[14]};
}

bool append_hlr_mesh(const contracts::MeshIllustrationMesh& source, std::uint32_t face,
                     FastHlrIndexedMesh* target)
{
    const auto matrix = matrix_or_identity(source.matrix);
    const std::size_t first_vertex = target->vertices.size();
    if (source.positions.size() % 3 != 0 ||
        first_vertex + source.positions.size() / 3 > std::numeric_limits<std::uint32_t>::max())
        return false;
    target->vertices.reserve(first_vertex + source.positions.size() / 3);
    for (std::size_t index = 0; index < source.positions.size(); index += 3)
    {
        const auto point =
            transform_point(matrix, source.positions[index], source.positions[index + 1],
                            source.positions[index + 2]);
        if (!std::all_of(point.begin(), point.end(),
                         [](double value) { return std::isfinite(value); }))
            return false;
        target->vertices.push_back({point[0], point[1], point[2]});
    }
    const std::size_t elements =
        source.indices ? source.indices->size() : source.positions.size() / 3;
    if (elements % 3 != 0)
        return false;
    target->triangles.reserve(target->triangles.size() + elements / 3);
    for (std::size_t element = 0; element < elements; element += 3)
    {
        FastHlrIndexedTriangle triangle;
        triangle.source_face = face;
        for (std::size_t corner = 0; corner < 3; ++corner)
        {
            const std::size_t local =
                source.indices ? (*source.indices)[element + corner] : element + corner;
            if (local >= source.positions.size() / 3)
                return false;
            triangle.vertices[corner] = static_cast<std::uint32_t>(first_vertex + local);
        }
        target->triangles.push_back(triangle);
    }
    return true;
}

bool reserve_hlr_mesh(const std::vector<contracts::MeshIllustrationMesh>& meshes,
                      FastHlrIndexedMesh* target)
{
    std::size_t vertices = 0;
    std::size_t triangles = 0;
    for (const auto& mesh : meshes)
    {
        if (mesh.positions.size() % 3 != 0)
            return false;
        const std::size_t mesh_vertices = mesh.positions.size() / 3;
        const std::size_t elements = mesh.indices ? mesh.indices->size() : mesh_vertices;
        if (elements % 3 != 0 || mesh_vertices > std::numeric_limits<std::uint32_t>::max() ||
            vertices > std::numeric_limits<std::uint32_t>::max() - mesh_vertices ||
            triangles > std::numeric_limits<std::size_t>::max() - elements / 3)
            return false;
        vertices += mesh_vertices;
        triangles += elements / 3;
    }
    target->vertices.reserve(vertices);
    target->triangles.reserve(triangles);
    return true;
}

FastHlrOptions
fast_options(const std::optional<contracts::ModelIllustrationLineworkOptionsA0>& patch)
{
    FastHlrOptions result;
    if (!patch || !patch->fast)
        return result;
    const auto& value = *patch->fast;
    result.include_boundaries = value.include_boundaries.value_or(result.include_boundaries);
    result.include_creases = value.include_creases.value_or(result.include_creases);
    result.include_silhouettes = value.include_silhouettes.value_or(result.include_silhouettes);
    result.include_hidden = false;
    result.suppress_coplanar_seams =
        value.suppress_coplanar_seams.value_or(result.suppress_coplanar_seams);
    result.crease_angle_rad = value.crease_angle_rad.value_or(result.crease_angle_rad);
    result.weld_tolerance = value.weld_tolerance.value_or(result.weld_tolerance);
    result.projected_tolerance = value.projected_tolerance.value_or(result.projected_tolerance);
    result.depth_tolerance = value.depth_tolerance.value_or(result.depth_tolerance);
    result.coplanar_seam_angle_rad =
        value.coplanar_seam_angle_rad.value_or(result.coplanar_seam_angle_rad);
    result.coplanar_seam_depth_tolerance =
        value.coplanar_seam_depth_tolerance.value_or(result.coplanar_seam_depth_tolerance);
    result.coplanar_seam_lateral_tolerance =
        value.coplanar_seam_lateral_tolerance.value_or(result.coplanar_seam_lateral_tolerance);
    if (value.limits)
    {
        const auto& limits = *value.limits;
        result.limits.max_vertices = limits.max_vertices.value_or(result.limits.max_vertices);
        result.limits.max_triangles = limits.max_triangles.value_or(result.limits.max_triangles);
        result.limits.max_edges = limits.max_edges.value_or(result.limits.max_edges);
        result.limits.max_grid_references =
            limits.max_grid_references.value_or(result.limits.max_grid_references);
        result.limits.max_candidate_pairs =
            limits.max_candidate_pairs.value_or(result.limits.max_candidate_pairs);
        result.limits.max_fragments = limits.max_fragments.value_or(result.limits.max_fragments);
        result.limits.max_output_segments =
            limits.max_output_segments.value_or(result.limits.max_output_segments);
    }
    return result;
}

contracts::ProjectedGeometry contract_geometry(const ProjectedModeGeometry& source)
{
    contracts::ProjectedGeometry result;
    result.segments.reserve(source.segments.size());
    for (const auto& segment : source.segments)
        result.segments.push_back({segment.x1, segment.y1, segment.x2, segment.y2});
    return result;
}

contracts::HlrProjectionResultA0 contract_hlr(const HlrProjectionResult& source,
                                              const std::string& hash)
{
    contracts::HlrProjectionResultA0 result;
    result.source.kind = contracts::HlrSourceKind::indexed_mesh;
    result.source.hash = hash;
    for (const auto& view : source.views)
    {
        contracts::HlrProjectedView converted;
        converted.id = view.view.id;
        converted.direction.assign(view.view.direction.begin(), view.view.direction.end());
        converted.up.assign(view.view.up.begin(), view.view.up.end());
        converted.modes.outline = contract_geometry(view.outline);
        converted.modes.detail = contract_geometry(view.detail);
        converted.modes.bbox = contract_geometry(view.bbox);
        result.views.push_back(std::move(converted));
    }
    result.timings.mesh_ms = source.timings.mesh_ms;
    result.timings.hlr_ms = source.timings.hlr_ms;
    result.timings.extract_ms = source.timings.extract_ms;
    return result;
}

void cap_warnings(std::vector<std::string>* warnings)
{
    if (warnings->size() <= 256)
        return;
    const auto omitted = warnings->size() - 255;
    warnings->resize(255);
    warnings->push_back(std::to_string(omitted) + " additional warnings omitted");
}

enum ModelIllustrationCode
{
    invalid_attachment = 10,
    unsupported_linework = 11,
    invalid_transform = 12,
    invalid_analytic_scene = 13,
    invalid_prepared_model = 14,
    linework_failed = 15,
    illustration_failed = 16,
    tessellation_failed = 17,
};

struct PreparedModelIllustration
{
    contracts::MeshCollectionA0 collection;
    contracts::ModelIllustrationSourceSummaryA0 source;
    contracts::ModelIllustrationTimingsA0 timings;
    std::vector<std::string> warnings;
    std::optional<contracts::HlrProjectionResultA0> hlr;
    MeshIllustrationExecutionLimits limits;
    contracts::ModelIllustrationBounds3MmA0 bounds;
    std::optional<contracts::MeshIllustrationStyleA0> style;
};

int set_status(Status* status, int code, std::string message)
{
    if (status)
        *status = {code, std::move(message)};
    return code;
}

template <typename Request>
int prepare_model_illustration(const Request& request, const ModelIllustrationAttachmentView* model,
                               PreparedModelIllustration* output, Status* status)
{
    if (output)
        *output = {};
    if (status)
        *status = {};
    if (!output)
        return set_status(status, 1, "Model illustration output pointer is null.");

    std::string validated;
    contracts::ContractError contract_error;
    if (!contracts::encode_json(request, &validated, &contract_error))
        return set_status(status, invalid_analytic_scene, contract_error.message);
    const auto& source = request.source;
    const bool model_input =
        std::holds_alternative<contracts::ModelAttachmentIllustrationSourceA0>(source);
    if ((model_input &&
         (!model || !model->data || model->size == 0 ||
          (model->media_type != "application/step" && model->media_type != "model/step"))) ||
        (!model_input && model))
        return set_status(status, invalid_attachment,
                          model_input ? "A model source requires one STEP attachment."
                                      : "An analytic source does not accept a model attachment.");
    if (request.linework && request.linework->fast &&
        request.linework->fast->include_hidden.value_or(false))
        return set_status(status, unsupported_linework,
                          "Model illustration does not support hidden linework.");
    if (request.work_limits)
    {
        output->limits.max_candidate_comparisons =
            request.work_limits->max_visibility_candidate_pairs.value_or(
                output->limits.max_candidate_comparisons);
        output->limits.max_drawing_commands =
            request.work_limits->max_drawing_commands.value_or(output->limits.max_drawing_commands);
    }

    std::string source_hash;
    const auto preparation_start = Clock::now();
    Status nested;
    int code = 0;
    if (model_input)
    {
        const auto& model_source = std::get<contracts::ModelAttachmentIllustrationSourceA0>(source);
        contracts::ModelTessellationRequestA0 tessellation;
        if (model_source.tessellation)
        {
            tessellation.linear_deflection_mm = model_source.tessellation->linear_deflection_mm;
            tessellation.angular_deflection_rad = model_source.tessellation->angular_deflection_rad;
            tessellation.root_placement = model_source.tessellation->root_placement;
            tessellation.max_triangles = model_source.tessellation->max_triangles;
            tessellation.allow_partial = model_source.tessellation->allow_partial;
        }
        code = model_tessellation_from_bytes(model->data, model->size, tessellation,
                                             &output->collection, &nested, &output->warnings);
        if (code != 0)
            return set_status(status, code == 102 || code == 103 ? code : tessellation_failed,
                              nested.message);
        if (model_source.transform)
        {
            model_illustration_detail::Matrix4 matrix;
            std::string transform_error;
            if (!model_illustration_detail::validate_affine_matrix(*model_source.transform, &matrix,
                                                                   &transform_error))
                return set_status(status, invalid_transform, std::move(transform_error));
            for (auto& mesh : output->collection.meshes)
                if (!model_illustration_detail::apply_affine_matrix(&mesh, matrix,
                                                                    &transform_error))
                    return set_status(status, invalid_transform, std::move(transform_error));
        }
        if (model_source.material_override)
            for (auto& mesh : output->collection.meshes)
                for (auto& material : mesh.materials)
                    material = *model_source.material_override;
        contracts::ModelAttachmentSourceSummaryA0 summary;
        summary.media_type = model->media_type == "model/step"
                                 ? contracts::ModelAttachmentMediaTypeA0::model_step
                                 : contracts::ModelAttachmentMediaTypeA0::application_step;
        source_hash = sha256_hex(model->data, model->size);
        summary.source_sha256 = source_hash;
        summary.meshes = static_cast<std::uint32_t>(output->collection.meshes.size());
        for (const auto& mesh : output->collection.meshes)
            summary.triangles += static_cast<std::uint32_t>(
                (mesh.indices ? mesh.indices->size() : mesh.positions.size() / 3) / 3);
        output->source = std::move(summary);
    }
    else
    {
        const auto& analytic = std::get<contracts::AnalyticIllustrationSourceA0>(source);
        model_illustration_detail::AnalyticLoweringStats lowered;
        code = model_illustration_detail::lower_analytic_scene(
            analytic, request.prepare, &output->collection, &lowered, &nested);
        if (code != 0)
            return set_status(status, code == 102 ? 102 : invalid_analytic_scene, nested.message);
        contracts::AnalyticSourceSummaryA0 summary;
        summary.definitions = lowered.definitions;
        summary.occurrences = lowered.occurrences;
        summary.primitives = lowered.primitives;
        summary.triangles = lowered.triangles;
        output->source = std::move(summary);
        source_hash =
            sha256_hex(reinterpret_cast<const std::uint8_t*>(validated.data()), validated.size());
    }
    std::array<double, 3> minimum = {std::numeric_limits<double>::infinity(),
                                     std::numeric_limits<double>::infinity(),
                                     std::numeric_limits<double>::infinity()};
    std::array<double, 3> maximum = {-std::numeric_limits<double>::infinity(),
                                     -std::numeric_limits<double>::infinity(),
                                     -std::numeric_limits<double>::infinity()};
    bool has_point = false;
    for (const auto& mesh : output->collection.meshes)
    {
        const auto matrix = matrix_or_identity(mesh.matrix);
        for (std::size_t index = 0; index + 2 < mesh.positions.size(); index += 3)
        {
            const auto point =
                transform_point(matrix, mesh.positions[index], mesh.positions[index + 1],
                                mesh.positions[index + 2]);
            for (std::size_t axis = 0; axis < 3; ++axis)
            {
                minimum[axis] = std::min(minimum[axis], point[axis]);
                maximum[axis] = std::max(maximum[axis], point[axis]);
            }
            has_point = true;
        }
    }
    if (!has_point)
        return set_status(status, invalid_prepared_model,
                          "Model illustration source contains no usable geometry.");
    output->bounds = {minimum[0], minimum[1], minimum[2], maximum[0], maximum[1], maximum[2]};
    output->style = request.style;
    if (request.linework &&
        (request.linework->outline_width_mm || request.linework->detail_width_mm))
    {
        if (!output->style)
            output->style.emplace();
        const double span = std::max({maximum[0] - minimum[0], maximum[1] - minimum[1], 1e-9});
        if (request.linework->outline_width_mm)
            output->style->outline_width = *request.linework->outline_width_mm / span;
        if (request.linework->detail_width_mm)
            output->style->crease_width = *request.linework->detail_width_mm / span;
    }
    output->timings.source_preparation_ms = milliseconds(preparation_start);

    const bool outline = !request.style || request.style->show_hlr_outline.value_or(true);
    const bool detail = request.style && request.style->show_hlr_detail.value_or(false);
    if (!outline && !detail)
        return 0;
    FastHlrIndexedMesh indexed;
    if (!reserve_hlr_mesh(output->collection.meshes, &indexed))
        return set_status(status, invalid_prepared_model,
                          "The prepared model cannot be converted to Fast linework geometry.");
    std::uint32_t face = 0;
    for (const auto& mesh : output->collection.meshes)
        if (!append_hlr_mesh(mesh, face++, &indexed))
            return set_status(status, invalid_prepared_model,
                              "The prepared model cannot be converted to Fast linework geometry.");
    HlrProjectionOptions options;
    options.projection_algorithm = ProjectionAlgorithm::Fast;
    options.outline_algorithm = ProjectionOutlineAlgorithm::FastMeshShadow;
    options.output_outline = outline;
    options.output_detail = detail;
    options.output_bbox = false;
    options.curve_mode = ProjectionCurveMode::Polyline;
    options.views = {
        {"illustration",
         {request.view.direction[0], request.view.direction[1], request.view.direction[2]},
         {request.view.up[0], request.view.up[1], request.view.up[2]}}};
    options.fast = fast_options(request.linework);
    options.fast.include_hidden = false;
    options.fast.limits.max_candidate_pairs =
        std::min(options.fast.limits.max_candidate_pairs, output->limits.max_candidate_comparisons);
    options.fast.limits.max_output_segments =
        std::min(options.fast.limits.max_output_segments, output->limits.max_drawing_commands);
    HlrProjectionResult projected;
    const auto line_start = Clock::now();
    code = mesh_hlr_projection(indexed, options, &projected, &nested);
    output->timings.linework_ms = milliseconds(line_start);
    if (code != 0)
    {
        const bool resource_limit = code == 3 || code == 6 || code == 7 || code == 102;
        return set_status(status, resource_limit ? 102 : linework_failed, nested.message);
    }
    output->hlr = contract_hlr(projected, source_hash);
    return 0;
}
} // namespace

int illustrate_model(const contracts::ModelIllustrationRequestA0& request,
                     const ModelIllustrationAttachmentView* model,
                     contracts::ModelIllustrationResultA0* result, Status* status)
{
    if (result)
        *result = {};
    if (!result)
        return set_status(status, 1, "Model illustration result pointer is null.");
    PreparedModelIllustration prepared;
    int code = prepare_model_illustration(request, model, &prepared, status);
    if (code != 0)
        return code;
    const auto illustration_start = Clock::now();
    try
    {
        const illustration_detail::IllustrationInputView input{
            prepared.collection.meshes, request.view, request.prepare, prepared.style};
        const auto rendered = illustration_detail::prepare_illustration(
            input, prepared.hlr ? &*prepared.hlr : nullptr,
            prepared.limits.max_candidate_comparisons, prepared.limits.max_drawing_commands);
        result->svg = illustration_detail::render_svg(
            rendered.scene, rendered.style, rendered.commands,
            request.svg.value_or(contracts::MeshIllustrationSvgOptions{}));
        result->stats = rendered.commands.stats;
        prepared.warnings.insert(prepared.warnings.end(), rendered.scene.warnings.begin(),
                                 rendered.scene.warnings.end());
    }
    catch (const illustration_detail::ResourceLimit& error)
    {
        return set_status(status, 102, error.what());
    }
    catch (const std::bad_alloc&)
    {
        return set_status(status, 102, "Model illustration allocation failed.");
    }
    catch (const std::exception& error)
    {
        return set_status(status, illustration_failed, error.what());
    }
    prepared.timings.illustration_ms = milliseconds(illustration_start);
    cap_warnings(&prepared.warnings);
    result->bounds_mm = prepared.bounds;
    result->source = std::move(prepared.source);
    result->timings = prepared.timings;
    result->warnings = std::move(prepared.warnings);
    return 0;
}

int illustrate_model_geometry(const contracts::ModelIllustrationGeometryRequestA0& request,
                              const ModelIllustrationAttachmentView* model,
                              ModelIllustrationGeometry* result, Status* status)
{
    if (result)
        *result = {};
    if (!result)
        return set_status(status, 1, "Model illustration geometry result pointer is null.");
    PreparedModelIllustration prepared;
    int code = prepare_model_illustration(request, model, &prepared, status);
    if (code != 0)
        return code;
    const auto illustration_start = Clock::now();
    try
    {
        const illustration_detail::IllustrationInputView input{
            prepared.collection.meshes, request.view, request.prepare, prepared.style};
        const auto rendered = illustration_detail::prepare_illustration(
            input, prepared.hlr ? &*prepared.hlr : nullptr,
            prepared.limits.max_candidate_comparisons, prepared.limits.max_drawing_commands);
        result->geometry = illustration_detail::make_illustration_geometry(rendered, request.view);
    }
    catch (const illustration_detail::ResourceLimit& error)
    {
        return set_status(status, 102, error.what());
    }
    catch (const std::bad_alloc&)
    {
        return set_status(status, 102, "Model illustration allocation failed.");
    }
    catch (const std::exception& error)
    {
        return set_status(status, illustration_failed, error.what());
    }
    prepared.timings.illustration_ms = milliseconds(illustration_start);
    prepared.warnings.insert(prepared.warnings.end(), result->geometry.warnings.begin(),
                             result->geometry.warnings.end());
    cap_warnings(&prepared.warnings);
    result->geometry.warnings = prepared.warnings;
    result->metadata.source = std::move(prepared.source);
    result->metadata.bounds_mm = prepared.bounds;
    result->metadata.stats = result->geometry.stats;
    result->metadata.timings = prepared.timings;
    result->metadata.warnings = prepared.warnings;

    return 0;
}

void execute_model_illustration(const unsigned char* request, std::size_t size,
                                const std::vector<OperationAttachmentView>& attachments,
                                OperationExecution* execution, bool geometry_only)
{
    const char* operation = geometry_only ? "geometry.model_illustration_geometry.a0"
                                          : "geometry.model_illustration.a0";
    contracts::ContractError error;
    contracts::ModelIllustrationGeometryRequestA0 geometry_request;
    contracts::ModelIllustrationRequestA0 svg_request;
    bool decoded = geometry_only ? contracts::decode_json(request, size, &geometry_request, &error)
                                 : contracts::decode_json(request, size, &svg_request, &error);
    if (!decoded)
    {
        fail(execution, operation, error.code, error.message,
             contracts::DiagnosticCategory::contract);
        return;
    }
    const auto& source = geometry_only ? geometry_request.source : svg_request.source;
    const bool model_input =
        std::holds_alternative<contracts::ModelAttachmentIllustrationSourceA0>(source);
    if ((model_input &&
         (attachments.size() != 1 || attachments[0].name != "model" ||
          (attachments[0].media_type != "application/step" &&
           attachments[0].media_type != "model/step") ||
          attachments[0].size > operation_input_attachment_max_bytes(operation, "model"))) ||
        (!model_input && !attachments.empty()))
    {
        fail(execution, operation, "geometer.contract.invalid_attachment",
             model_input ? "A model source requires one bounded STEP attachment named model."
                         : "An analytic source does not accept attachments.",
             contracts::DiagnosticCategory::contract);
        return;
    }
    const auto report_failure = [&](int code, const Status& status)
    {
        const char* diagnostic =
            code == invalid_attachment       ? "geometer.contract.invalid_attachment"
            : code == unsupported_linework   ? "geometer.contract.unsupported_linework_option"
            : code == invalid_transform      ? "geometer.contract.invalid_transform"
            : code == invalid_analytic_scene ? "geometer.contract.invalid_analytic_scene"
            : code == invalid_prepared_model ? "geometer.operation.invalid_prepared_model"
            : code == linework_failed        ? "geometer.operation.linework_failed"
            : code == illustration_failed    ? "geometer.operation.illustration_failed"
            : code == tessellation_failed    ? "geometer.operation.tessellation_failed"
            : code == 103                    ? "geometer.contract.external_model_reference"
            : code == 102                    ? "geometer.operation.resource_limit_exceeded"
                                             : "geometer.operation.model_illustration_failed";
        const bool contract = code == invalid_attachment || code == unsupported_linework ||
                              code == invalid_transform || code == invalid_analytic_scene ||
                              code == 103;
        fail(execution, operation, diagnostic, status.message,
             contract ? contracts::DiagnosticCategory::contract
                      : contracts::DiagnosticCategory::operation);
    };
    std::optional<ModelIllustrationAttachmentView> model;
    if (model_input)
        model = ModelIllustrationAttachmentView{attachments[0].data, attachments[0].size,
                                                attachments[0].media_type};
    Status status;
    contracts::OperationSuccessA0 success;
    success.operation = operation;
    if (!geometry_only)
    {
        contracts::ModelIllustrationResultA0 result;
        const int code = illustrate_model(svg_request, model ? &*model : nullptr, &result, &status);
        if (code != 0)
        {
            report_failure(code, status);
            return;
        }
        success.result = std::move(result);
        execution->outcome = std::move(success);
        execution->attachments.clear();
        return;
    }

    ModelIllustrationGeometry result;
    const int code =
        illustrate_model_geometry(geometry_request, model ? &*model : nullptr, &result, &status);
    if (code != 0)
    {
        report_failure(code, status);
        return;
    }
    const auto serialization_start = Clock::now();
    std::string json;
    if (!contracts::encode_json(result.geometry, &json, &error))
    {
        fail(execution, operation, error.code, error.message,
             contracts::DiagnosticCategory::operation);
        return;
    }
    result.metadata.timings.attachment_encoding_ms = milliseconds(serialization_start);
    if (json.size() > operation_output_attachment_max_bytes(operation, "illustration_geometry"))
    {
        fail(execution, operation, "geometer.operation.resource_limit_exceeded",
             "Illustration geometry JSON exceeds its operation attachment limit.",
             contracts::DiagnosticCategory::operation);
        return;
    }
    contracts::ModelIllustrationGeometryResultA0 wire;
    wire.bounds_mm = std::move(result.metadata.bounds_mm);
    wire.source = std::move(result.metadata.source);
    wire.stats = result.metadata.stats;
    wire.timings = result.metadata.timings;
    wire.warnings = std::move(result.metadata.warnings);
    wire.geometry.byte_length = static_cast<std::uint32_t>(json.size());
    wire.geometry.sha256 =
        sha256_hex(reinterpret_cast<const std::uint8_t*>(json.data()), json.size());
    success.result = std::move(wire);
    execution->outcome = std::move(success);
    execution->attachments = {{"illustration_geometry",
                               "application/vnd.wavenumber.geometer.illustration-geometry+json",
                               std::vector<unsigned char>(json.begin(), json.end())}};
}
} // namespace geometer

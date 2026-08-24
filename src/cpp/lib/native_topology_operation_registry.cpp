#include "geometer/operation_registry.h"

#include "geometer/operation_transport.h"
#include "geometer/sha256.h"
#include "geometer/step_topology_session.h"
#include "geometer/version.h"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

namespace geometer
{
namespace
{

constexpr const char* kOpenOperation = "geometry.step_topology.open.a0";
constexpr const char* kInspectOperation = "geometry.step_topology.inspect.a0";
constexpr const char* kCloseOperation = "geometry.step_topology.close.a0";
constexpr const char* kRenderOperation = "geometry.step_topology.render.a0";
constexpr const char* kResolveHitOperation = "geometry.step_topology.resolve_hit.a0";
constexpr const char* kApplyGroupsOperation = "geometry.step_topology.apply_logical_groups.a0";
constexpr const char* kApplyProbesOperation = "geometry.step_topology.apply_metadata_probes.a0";
constexpr const char* kCheckpointOperation = "geometry.step_topology.checkpoint_edit_journal.a0";
constexpr const char* kRestoreOperation = "geometry.step_topology.restore.a0";
constexpr int kResourceLimit = 102;
constexpr int kInternalFailure = 108;

StepTopologySessionStore& session_store()
{
    static StepTopologySessionStore store;
    return store;
}

contracts::DiagnosticA0 diagnostic(const std::string& operation, const std::string& code,
                                   const std::string& message, std::string path = {})
{
    contracts::DiagnosticA0 value;
    value.code = code;
    value.category = contracts::DiagnosticCategory::operation;
    value.message = message;
    value.retryable = false;
    value.operation = operation;
    if (!path.empty())
        value.path = std::move(path);
    return value;
}

void fail(OperationExecution* execution, const std::string& operation,
          contracts::DiagnosticA0 value)
{
    contracts::OperationFailureA0 failure;
    failure.operation = operation;
    failure.diagnostics.push_back(std::move(value));
    execution->outcome = std::move(failure);
    execution->attachments.clear();
}

template <typename Result>
void succeed(OperationExecution* execution, const std::string& operation, Result result)
{
    contracts::OperationSuccessA0 success;
    success.operation = operation;
    success.result = std::move(result);
    execution->outcome = std::move(success);
    execution->attachments.clear();
}

template <typename Request>
bool decode_request(const std::string& operation, const unsigned char* request_json,
                    std::size_t request_json_size, Request* request, OperationExecution* execution)
{
    contracts::ContractError error;
    if (contracts::decode_json(request_json, request_json_size, request, &error))
        return true;
    contracts::DiagnosticA0 value;
    value.code = error.code;
    value.category = contracts::DiagnosticCategory::contract;
    value.message = error.message;
    value.retryable = false;
    value.operation = operation;
    if (!error.path.empty())
        value.path = error.path;
    fail(execution, operation, std::move(value));
    return false;
}

const OperationAttachmentView*
step_attachment(const std::string& operation,
                const std::vector<OperationAttachmentView>& attachments,
                OperationExecution* execution)
{
    if (attachments.size() != 1U || attachments.front().name != "step")
    {
        fail(execution, operation,
             diagnostic(operation, "geometer.contract.missing_attachment",
                        "The native topology open operation requires exactly one step attachment.",
                        "/attachments/step"));
        return nullptr;
    }
    const auto& attachment = attachments.front();
    if (attachment.media_type != "application/step" && attachment.media_type != "model/step")
    {
        fail(execution, operation,
             diagnostic(operation, "geometer.contract.attachment_media_type_mismatch",
                        "The step attachment media type is not supported.",
                        "/attachments/step/media_type"));
        return nullptr;
    }
    return &attachment;
}

bool require_no_attachments(const std::string& operation,
                            const std::vector<OperationAttachmentView>& attachments,
                            OperationExecution* execution)
{
    if (attachments.empty())
        return true;
    fail(execution, operation,
         diagnostic(operation, "geometer.contract.undeclared_attachment",
                    "This topology operation does not accept attachments.", "/attachments"));
    return false;
}

const OperationAttachmentView*
named_attachment(const std::string& operation,
                 const std::vector<OperationAttachmentView>& attachments, const std::string& name,
                 OperationExecution* execution)
{
    const OperationAttachmentView* found = nullptr;
    for (const auto& attachment : attachments)
    {
        if (attachment.name != name)
            continue;
        if (found != nullptr)
        {
            fail(execution, operation,
                 diagnostic(operation, "geometer.contract.duplicate_attachment",
                            "The native topology operation received a duplicate attachment.",
                            "/attachments/" + name));
            return nullptr;
        }
        found = &attachment;
    }
    if (found == nullptr)
    {
        fail(execution, operation,
             diagnostic(operation, "geometer.contract.missing_attachment",
                        "The native topology operation is missing a required attachment.",
                        "/attachments/" + name));
    }
    return found;
}

bool require_generation(const std::string& operation, const contracts::SessionReference& reference,
                        StepTopologySessionInfo* info, OperationExecution* execution)
{
    Status status;
    if (session_store().info(reference.session_handle, info, &status) != 0)
    {
        fail(execution, operation,
             diagnostic(operation, "geometer.operation.step_topology.unknown_session",
                        status.message, "/session/session_handle"));
        return false;
    }
    if (info->generation != reference.generation)
    {
        fail(execution, operation,
             diagnostic(operation, "geometer.operation.step_topology.stale_generation",
                        "The session generation does not match the live native session.",
                        "/session/generation"));
        return false;
    }
    return true;
}

contracts::SessionReference session_reference(const StepTopologySessionInfo& info)
{
    return {info.session_handle, static_cast<std::uint32_t>(info.generation)};
}

contracts::MutationSessionState mutation_state(const StepTopologySessionInfo& info)
{
    contracts::MutationSessionState state;
    state.session = session_reference(info);
    state.edit_journal_revision = static_cast<std::uint32_t>(info.edit_journal_revision);
    state.accounted_string_bytes = static_cast<std::uint32_t>(info.accounted_string_bytes);
    state.estimated_resident_bytes = static_cast<std::uint32_t>(info.estimated_resident_bytes);
    return state;
}

contracts::LogicalGroup logical_group(const StepTopologyLogicalGroup& source)
{
    contracts::LogicalGroup group;
    group.authored_id = source.authored_id;
    group.revision = static_cast<std::uint32_t>(source.revision);
    group.name = source.name;
    for (const auto& source_member : source.members)
    {
        contracts::LogicalGroupMember member;
        member.kind = source_member.kind == StepTopologyTargetKind::body
                          ? contracts::LogicalGroupMemberKind::body
                          : contracts::LogicalGroupMemberKind::face;
        member.target_handle = source_member.target_handle;
        group.members.push_back(std::move(member));
    }
    return group;
}

StepTopologyProbeTarget probe_target(const contracts::MetadataProbeTarget& source)
{
    return std::visit(
        [](const auto& value)
        {
            using Value = std::decay_t<decltype(value)>;
            StepTopologyProbeTarget target;
            if constexpr (std::is_same_v<Value, contracts::DocumentProbeTarget>)
            {
                target.kind = StepTopologyProbeTargetKind::document;
            }
            else if constexpr (std::is_same_v<Value, contracts::DefinitionProbeTarget>)
            {
                target.kind = StepTopologyProbeTargetKind::definition;
                target.target_handle = value.target_handle;
            }
            else if constexpr (std::is_same_v<Value, contracts::RootOccurrenceProbeTarget>)
            {
                target.kind = StepTopologyProbeTargetKind::root_occurrence;
                target.target_handle = value.target_handle;
            }
            else if constexpr (std::is_same_v<Value, contracts::ComponentOccurrenceProbeTarget>)
            {
                target.kind = StepTopologyProbeTargetKind::occurrence;
                target.target_handle = value.target_handle;
            }
            else if constexpr (std::is_same_v<Value, contracts::BodyProbeTarget>)
            {
                target.kind = StepTopologyProbeTargetKind::body;
                target.target_handle = value.target_handle;
            }
            else if constexpr (std::is_same_v<Value, contracts::FaceProbeTarget>)
            {
                target.kind = StepTopologyProbeTargetKind::face;
                target.target_handle = value.target_handle;
            }
            else
            {
                target.kind = StepTopologyProbeTargetKind::logical_group;
                target.group_authored_id = value.group_authored_id;
            }
            return target;
        },
        source);
}

contracts::MetadataProbeTarget probe_target(const StepTopologyProbeTarget& source)
{
    switch (source.kind)
    {
    case StepTopologyProbeTargetKind::document:
        return contracts::DocumentProbeTarget{};
    case StepTopologyProbeTargetKind::definition:
        return contracts::DefinitionProbeTarget{"definition", source.target_handle};
    case StepTopologyProbeTargetKind::root_occurrence:
        return contracts::RootOccurrenceProbeTarget{"root_occurrence", source.target_handle};
    case StepTopologyProbeTargetKind::occurrence:
        return contracts::ComponentOccurrenceProbeTarget{"occurrence", source.target_handle};
    case StepTopologyProbeTargetKind::body:
        return contracts::BodyProbeTarget{"body", source.target_handle};
    case StepTopologyProbeTargetKind::face:
        return contracts::FaceProbeTarget{"face", source.target_handle};
    case StepTopologyProbeTargetKind::logical_group:
        return contracts::LogicalGroupProbeTarget{"logical_group", source.group_authored_id};
    }
    return contracts::DocumentProbeTarget{};
}

contracts::MetadataProbe metadata_probe(const StepTopologyMetadataProbe& source)
{
    contracts::MetadataProbe probe;
    probe.authored_id = source.authored_id;
    probe.revision = static_cast<std::uint32_t>(source.revision);
    probe.target = probe_target(source.target);
    probe.key = source.key;
    probe.value = source.value;
    return probe;
}

contracts::StepTopologyApplyLogicalGroupsResultA0
group_result(const StepTopologyGroupTransactionResult& applied)
{
    contracts::StepTopologyApplyLogicalGroupsResultA0 result;
    result.state = mutation_state(applied.session);
    for (const auto& group : applied.groups)
        result.groups.push_back(logical_group(group));
    return result;
}

contracts::StepTopologyApplyMetadataProbesResultA0
probe_result(const StepTopologyProbeTransactionResult& applied)
{
    contracts::StepTopologyApplyMetadataProbesResultA0 result;
    result.state = mutation_state(applied.session);
    for (const auto& group : applied.groups)
        result.groups.push_back(logical_group(group));
    for (const auto& probe : applied.probes)
        result.probes.push_back(metadata_probe(probe));
    return result;
}

template <typename Result>
int validate_mutation_publication(const std::string& operation, Result result, Status* status)
{
    OperationExecution preview;
    succeed(&preview, operation, std::move(result));
    std::string json;
    contracts::ContractError error;
    if (!contracts::encode_json(preview.outcome, &json, &error))
    {
        if (status != nullptr)
        {
            status->code = kInternalFailure;
            status->message =
                "Mutation result could not be encoded before publication: " + error.message;
        }
        return kInternalFailure;
    }
    std::string validation_message;
    const OperationResponseValidationStatus validation =
        validate_operation_response(operation, json, {}, &validation_message);
    if (validation == OperationResponseValidationStatus::ok)
        return 0;
    const int code = validation == OperationResponseValidationStatus::limit_exceeded
                         ? kResourceLimit
                         : kInternalFailure;
    if (status != nullptr)
    {
        status->code = code;
        status->message = "Mutation result rejected before publication: " + validation_message;
    }
    return code;
}

int validate_group_publication(const StepTopologyGroupTransactionResult& applied, void*,
                               Status* status)
{
    return validate_mutation_publication(kApplyGroupsOperation, group_result(applied), status);
}

int validate_probe_publication(const StepTopologyProbeTransactionResult& applied, void*,
                               Status* status)
{
    return validate_mutation_publication(kApplyProbesOperation, probe_result(applied), status);
}

std::optional<contracts::SourceEntityEvidence>
source_evidence(const StepSourceEntityEvidence& source, bool included)
{
    if (!included)
        return std::nullopt;
    contracts::SourceEntityEvidence evidence;
    evidence.mapped = source.mapped;
    evidence.shape_result_round_trip = source.shape_result_round_trip;
    if (source.model_number > 0 &&
        source.model_number <= static_cast<std::int64_t>(std::numeric_limits<std::uint32_t>::max()))
        evidence.model_number = static_cast<std::uint32_t>(source.model_number);
    if (!source.entity_type.empty())
        evidence.entity_type = source.entity_type;
    if (!source.mapping_method.empty())
        evidence.mapping_method = source.mapping_method;
    return evidence;
}

std::string cursor_for(const StepTopologySessionInfo& info,
                       const StepTopologyPagePosition& position)
{
    return "a0:" + info.session_handle + ":" + std::to_string(info.generation) + ":" +
           std::to_string(position.section) + ":" + std::to_string(position.record) + ":" +
           std::to_string(position.member);
}

bool cursor_position(const std::optional<std::string>& cursor, const StepTopologySessionInfo& info,
                     StepTopologyPagePosition* position)
{
    *position = {};
    if (!cursor.has_value())
        return true;
    const std::string prefix =
        "a0:" + info.session_handle + ":" + std::to_string(info.generation) + ":";
    if (cursor->compare(0U, prefix.size(), prefix) != 0)
        return false;
    const char* current = cursor->data() + prefix.size();
    const char* last = cursor->data() + cursor->size();
    std::size_t* fields[] = {&position->section, &position->record, &position->member};
    for (std::size_t index = 0; index < 3U; ++index)
    {
        const char* end = index == 2U ? last : std::find(current, last, ':');
        if (end == current)
            return false;
        const auto result = std::from_chars(current, end, *fields[index]);
        if (result.ec != std::errc{} || result.ptr != end)
            return false;
        current = end + (index == 2U ? 0U : 1U);
    }
    return current == last;
}

void execute_open(const unsigned char* request_json, std::size_t request_json_size,
                  const std::vector<OperationAttachmentView>& attachments,
                  OperationExecution* execution)
{
    contracts::StepTopologyOpenRequestA0 request;
    if (!decode_request(kOpenOperation, request_json, request_json_size, &request, execution))
        return;
    const auto* step = step_attachment(kOpenOperation, attachments, execution);
    if (step == nullptr)
        return;
    StepTopologyOpenResult opened;
    Status status;
    if (session_store().open_step(step->data, step->size, &opened, &status) != 0)
    {
        fail(execution, kOpenOperation,
             diagnostic(kOpenOperation, "geometer.operation.step_topology.open_failed",
                        status.message));
        return;
    }
    contracts::StepTopologyOpenResultA0 result;
    result.session = session_reference(opened.session);
    result.source.sha256 = opened.session.source_sha256;
    result.source.bytes = static_cast<std::uint32_t>(opened.session.source_bytes);
    result.tool.release_version = version_string();
    result.tool.occt_version = opened.session.occt_version;
    result.evicted_session_handles = std::move(opened.evicted_session_handles);
    succeed(execution, kOpenOperation, std::move(result));
}

void execute_inspect(const unsigned char* request_json, std::size_t request_json_size,
                     const std::vector<OperationAttachmentView>& attachments,
                     OperationExecution* execution)
{
    contracts::StepTopologyInspectRequestA0 request;
    if (!decode_request(kInspectOperation, request_json, request_json_size, &request, execution) ||
        !require_no_attachments(kInspectOperation, attachments, execution))
        return;
    StepTopologySessionInfo info;
    if (!require_generation(kInspectOperation, request.session, &info, execution))
        return;
    if (request.include_diagnostics)
    {
        fail(execution, kInspectOperation,
             diagnostic(kInspectOperation, "geometer.operation.step_topology.unsupported_option",
                        "include_diagnostics is deferred until diagnostic carriers have a bounded "
                        "wire contract.",
                        "/include_diagnostics"));
        return;
    }
    StepTopologyInspectionOptions options;
    options.include_source_entity_evidence = request.include_source_entity_evidence;
    StepTopologyPagePosition position;
    if (!cursor_position(request.page.cursor, info, &position))
    {
        fail(
            execution, kInspectOperation,
            diagnostic(kInspectOperation, "geometer.operation.step_topology.invalid_cursor",
                       "The topology page cursor is invalid, stale, or belongs to another session.",
                       "/page/cursor"));
        return;
    }
    StepTopologySnapshotPage page;
    Status status;
    if (session_store().inspect_page(request.session.session_handle, options, position,
                                     request.page.limit, &page, &status) != 0)
    {
        fail(execution, kInspectOperation,
             diagnostic(kInspectOperation, "geometer.operation.step_topology.inspect_failed",
                        status.message));
        return;
    }

    contracts::StepTopologyInspectResultA0 result;
    result.session = session_reference(page.session);
    result.counts.definitions = static_cast<std::uint32_t>(page.definition_count);
    result.counts.root_occurrences = static_cast<std::uint32_t>(page.root_occurrence_count);
    result.counts.component_occurrences =
        static_cast<std::uint32_t>(page.component_occurrence_count);
    result.counts.bodies = static_cast<std::uint32_t>(page.body_count);
    result.counts.shells = static_cast<std::uint32_t>(page.shell_count);
    result.counts.faces = static_cast<std::uint32_t>(page.face_count);
    result.counts.memberships = static_cast<std::uint32_t>(page.membership_count);

    for (const auto& item : page.definitions)
    {
        contracts::DefinitionSummary value;
        value.handle = item.handle;
        value.name = item.label.name;
        value.assembly = item.is_assembly;
        value.body_count = static_cast<std::uint32_t>(item.body_count);
        value.face_count = static_cast<std::uint32_t>(item.face_count);
        value.source_entity =
            source_evidence(item.source_entity, request.include_source_entity_evidence);
        result.page.definitions.push_back(std::move(value));
    }
    for (const auto& item : page.root_occurrences)
    {
        contracts::RootOccurrenceSummary value;
        value.handle = item.handle;
        value.definition_handle = item.definition_handle;
        value.name = item.label.name;
        value.transform.assign(item.transform.begin(), item.transform.end());
        result.page.occurrences.emplace_back(std::move(value));
    }
    for (const auto& item : page.occurrences)
    {
        contracts::ComponentOccurrenceSummary value;
        value.handle = item.handle;
        value.definition_handle = item.definition_handle;
        value.parent_occurrence_handle = item.parent_occurrence_handle;
        value.depth = static_cast<std::uint32_t>(item.depth);
        value.name = item.label.name;
        value.transform.assign(item.transform.begin(), item.transform.end());
        result.page.occurrences.emplace_back(std::move(value));
    }
    for (const auto& item : page.bodies)
    {
        contracts::BodySummary value;
        value.handle = item.handle;
        value.definition_handle = item.definition_handle;
        value.topology_kind = item.topology_kind;
        value.shell_count = static_cast<std::uint32_t>(item.shell_count);
        value.face_count = static_cast<std::uint32_t>(item.face_count);
        value.bounds_mm.assign(item.bounds.begin(), item.bounds.end());
        value.volume_mm3 = item.volume;
        value.source_entity =
            source_evidence(item.source_entity, request.include_source_entity_evidence);
        result.page.bodies.push_back(std::move(value));
    }
    for (const auto& item : page.shells)
    {
        contracts::ShellSummary value;
        value.handle = item.handle;
        value.definition_handle = item.definition_handle;
        value.body_count = static_cast<std::uint32_t>(item.body_count);
        value.face_count = static_cast<std::uint32_t>(item.face_count);
        value.source_entity =
            source_evidence(item.source_entity, request.include_source_entity_evidence);
        result.page.shells.push_back(std::move(value));
    }
    for (const auto& item : page.faces)
    {
        contracts::FaceSummary value;
        value.handle = item.handle;
        value.definition_handle = item.definition_handle;
        value.body_count = static_cast<std::uint32_t>(item.body_count);
        value.shell_count = static_cast<std::uint32_t>(item.shell_count);
        value.bounds_mm.assign(item.bounds.begin(), item.bounds.end());
        value.area_mm2 = item.area;
        value.centroid_mm.assign(item.centroid.begin(), item.centroid.end());
        value.source_entity =
            source_evidence(item.source_entity, request.include_source_entity_evidence);
        result.page.faces.push_back(std::move(value));
    }
    for (const auto& item : page.memberships)
    {
        contracts::TopologyMembership value;
        value.kind = item.kind == StepTopologyMembershipKind::body_shell
                         ? contracts::TopologyMembershipKind::body_shell
                     : item.kind == StepTopologyMembershipKind::body_face
                         ? contracts::TopologyMembershipKind::body_face
                         : contracts::TopologyMembershipKind::shell_face;
        value.owner_handle = item.owner_handle;
        value.member_handle = item.member_handle;
        result.page.memberships.push_back(std::move(value));
    }
    if (page.has_next)
        result.page.next_cursor = cursor_for(page.session, page.next);
    succeed(execution, kInspectOperation, std::move(result));
}

void execute_close(const unsigned char* request_json, std::size_t request_json_size,
                   const std::vector<OperationAttachmentView>& attachments,
                   OperationExecution* execution)
{
    contracts::StepTopologyCloseRequestA0 request;
    if (!decode_request(kCloseOperation, request_json, request_json_size, &request, execution) ||
        !require_no_attachments(kCloseOperation, attachments, execution))
        return;
    StepTopologySessionInfo info;
    if (!require_generation(kCloseOperation, request.session, &info, execution))
        return;
    Status status;
    if (session_store().close(request.session.session_handle, &status) != 0)
    {
        fail(execution, kCloseOperation,
             diagnostic(kCloseOperation, "geometer.operation.step_topology.close_failed",
                        status.message));
        return;
    }
    contracts::StepTopologyCloseResultA0 result;
    result.session_handle = request.session.session_handle;
    succeed(execution, kCloseOperation, std::move(result));
}

void execute_render(const unsigned char* request_json, std::size_t request_json_size,
                    const std::vector<OperationAttachmentView>& attachments,
                    OperationExecution* execution)
{
    contracts::StepTopologyRenderRequestA0 request;
    if (!decode_request(kRenderOperation, request_json, request_json_size, &request, execution) ||
        !require_no_attachments(kRenderOperation, attachments, execution))
        return;
    StepTopologySessionInfo info;
    if (!require_generation(kRenderOperation, request.session, &info, execution))
        return;
    StepTopologyGlbOptions options;
    options.tessellation.linear_deflection = request.tessellation.linear_deflection_mm;
    options.tessellation.angular_deflection = request.tessellation.angular_deflection_rad;
    options.tessellation.relative = request.tessellation.relative;
    options.tessellation.parallel = request.tessellation.parallel;
    std::copy(request.tessellation.source_to_render.begin(),
              request.tessellation.source_to_render.end(),
              options.tessellation.source_to_render.begin());
    StepTopologyGlbRenderOutput rendered;
    Status status;
    if (session_store().render_glb_work_packet(request.session.session_handle, options, &rendered,
                                               &status) != 0)
    {
        fail(execution, kRenderOperation,
             diagnostic(kRenderOperation, "geometer.operation.step_topology.render_failed",
                        status.message));
        return;
    }
    contracts::StepTopologyRenderResultA0 result;
    result.session = session_reference(rendered.session);
    result.artifact.artifact_handle = rendered.artifact_handle;
    result.artifact.content_sha256 = rendered.content_sha256;
    result.artifact.render_artifact_handle = rendered.render_artifact_handle;
    result.artifact.render_content_sha256 = rendered.render_content_sha256;
    result.artifact.counts.meshes = static_cast<std::uint32_t>(rendered.mesh_count);
    result.artifact.counts.instances = static_cast<std::uint32_t>(rendered.instance_count);
    result.artifact.counts.primitives = static_cast<std::uint32_t>(rendered.primitive_count);
    result.artifact.counts.geometry_triangles =
        static_cast<std::uint32_t>(rendered.geometry_triangle_count);
    result.artifact.counts.instanced_triangles =
        static_cast<std::uint32_t>(rendered.instanced_triangle_count);
    result.glb.bytes = static_cast<std::uint32_t>(rendered.glb.size());
    result.glb.sha256 = rendered.content_sha256;
    succeed(execution, kRenderOperation, std::move(result));
    OperationOutputAttachment attachment;
    attachment.name = "glb";
    attachment.media_type = "model/gltf-binary";
    attachment.data = std::move(rendered.glb);
    execution->attachments.push_back(std::move(attachment));
}

void execute_resolve_hit(const unsigned char* request_json, std::size_t request_json_size,
                         const std::vector<OperationAttachmentView>& attachments,
                         OperationExecution* execution)
{
    contracts::StepTopologyResolveHitRequestA0 request;
    if (!decode_request(kResolveHitOperation, request_json, request_json_size, &request,
                        execution) ||
        !require_no_attachments(kResolveHitOperation, attachments, execution))
        return;
    StepTopologySessionInfo info;
    if (!require_generation(kResolveHitOperation, request.session, &info, execution))
        return;
    StepTopologyGlbHitDescriptor descriptor;
    descriptor.artifact_handle = request.artifact_handle;
    descriptor.content_sha256 = request.content_sha256;
    descriptor.instance_index = request.instance_index;
    descriptor.primitive_index = request.primitive_index;
    descriptor.primitive_triangle_index = request.primitive_triangle_index;
    descriptor.occurrence_handle = request.occurrence_handle;
    descriptor.body_handle = request.body_handle;
    descriptor.face_handle = request.face_handle;
    StepTopologyRenderHit hit;
    Status status;
    if (session_store().resolve_glb_hit(request.session.session_handle, descriptor, &hit,
                                        &status) != 0)
    {
        fail(execution, kResolveHitOperation,
             diagnostic(kResolveHitOperation, "geometer.operation.step_topology.resolve_hit_failed",
                        status.message));
        return;
    }
    contracts::StepTopologyResolveHitResultA0 result;
    result.session = session_reference(info);
    result.instance_index = static_cast<std::uint32_t>(hit.instance_index);
    result.primitive_index = static_cast<std::uint32_t>(hit.primitive_index);
    result.triangle_index = static_cast<std::uint32_t>(hit.triangle_index);
    result.occurrence_handle = hit.occurrence_handle;
    result.body_handle = hit.body_handle;
    result.face_handle = hit.face_handle;
    succeed(execution, kResolveHitOperation, std::move(result));
}

void execute_apply_groups(const unsigned char* request_json, std::size_t request_json_size,
                          const std::vector<OperationAttachmentView>& attachments,
                          OperationExecution* execution)
{
    contracts::StepTopologyApplyLogicalGroupsRequestA0 request;
    if (!decode_request(kApplyGroupsOperation, request_json, request_json_size, &request,
                        execution) ||
        !require_no_attachments(kApplyGroupsOperation, attachments, execution))
        return;
    StepTopologySessionInfo info;
    if (!require_generation(kApplyGroupsOperation, request.session, &info, execution))
        return;

    StepTopologyGroupTransaction transaction;
    transaction.expected_generation = request.session.generation;
    for (const auto& source : request.commands)
    {
        transaction.commands.push_back(std::visit(
            [](const auto& value)
            {
                using Value = std::decay_t<decltype(value)>;
                StepTopologyGroupCommand command;
                command.authored_id = value.authored_id;
                if constexpr (std::is_same_v<Value, contracts::CreateLogicalGroupCommand>)
                {
                    command.kind = StepTopologyGroupCommandKind::create;
                    command.name = value.name;
                    command.member_handles = value.member_handles;
                }
                else if constexpr (std::is_same_v<Value, contracts::RenameLogicalGroupCommand>)
                {
                    command.kind = StepTopologyGroupCommandKind::rename;
                    command.expected_revision = value.expected_revision;
                    command.name = value.name;
                }
                else if constexpr (std::is_same_v<Value,
                                                  contracts::ReplaceLogicalGroupMembersCommand>)
                {
                    command.kind = StepTopologyGroupCommandKind::replace_members;
                    command.expected_revision = value.expected_revision;
                    command.member_handles = value.member_handles;
                }
                else
                {
                    command.kind = StepTopologyGroupCommandKind::erase;
                    command.expected_revision = value.expected_revision;
                }
                return command;
            },
            source));
    }

    StepTopologyGroupTransactionResult applied;
    Status status;
    if (session_store().apply_logical_groups(request.session.session_handle, transaction,
                                             validate_group_publication, nullptr, &applied,
                                             &status) != 0)
    {
        fail(execution, kApplyGroupsOperation,
             diagnostic(kApplyGroupsOperation,
                        "geometer.operation.step_topology.apply_logical_groups_failed",
                        status.message));
        return;
    }
    succeed(execution, kApplyGroupsOperation, group_result(applied));
}

void execute_apply_probes(const unsigned char* request_json, std::size_t request_json_size,
                          const std::vector<OperationAttachmentView>& attachments,
                          OperationExecution* execution)
{
    contracts::StepTopologyApplyMetadataProbesRequestA0 request;
    if (!decode_request(kApplyProbesOperation, request_json, request_json_size, &request,
                        execution) ||
        !require_no_attachments(kApplyProbesOperation, attachments, execution))
        return;
    StepTopologySessionInfo info;
    if (!require_generation(kApplyProbesOperation, request.session, &info, execution))
        return;

    StepTopologyProbeTransaction transaction;
    transaction.expected_generation = request.session.generation;
    for (const auto& source : request.commands)
    {
        transaction.commands.push_back(std::visit(
            [](const auto& value)
            {
                using Value = std::decay_t<decltype(value)>;
                StepTopologyProbeCommand command;
                command.authored_id = value.authored_id;
                if constexpr (std::is_same_v<Value, contracts::AttachMetadataProbeCommand>)
                {
                    command.kind = StepTopologyProbeCommandKind::attach;
                    command.target = probe_target(value.target);
                    command.key = value.key;
                    command.value = value.value;
                }
                else if constexpr (std::is_same_v<Value, contracts::ReplaceMetadataProbeCommand>)
                {
                    command.kind = StepTopologyProbeCommandKind::replace;
                    command.expected_revision = value.expected_revision;
                    command.target = probe_target(value.target);
                    command.key = value.key;
                    command.value = value.value;
                }
                else
                {
                    command.kind = StepTopologyProbeCommandKind::erase;
                    command.expected_revision = value.expected_revision;
                }
                return command;
            },
            source));
    }

    StepTopologyProbeTransactionResult applied;
    Status status;
    if (session_store().apply_metadata_probes(request.session.session_handle, transaction,
                                              validate_probe_publication, nullptr, &applied,
                                              &status) != 0)
    {
        fail(execution, kApplyProbesOperation,
             diagnostic(kApplyProbesOperation,
                        "geometer.operation.step_topology.apply_metadata_probes_failed",
                        status.message));
        return;
    }
    succeed(execution, kApplyProbesOperation, probe_result(applied));
}

void execute_checkpoint(const unsigned char* request_json, std::size_t request_json_size,
                        const std::vector<OperationAttachmentView>& attachments,
                        OperationExecution* execution)
{
    contracts::StepTopologyCheckpointEditJournalRequestA0 request;
    if (!decode_request(kCheckpointOperation, request_json, request_json_size, &request,
                        execution) ||
        !require_no_attachments(kCheckpointOperation, attachments, execution))
        return;
    StepTopologySessionInfo info;
    if (!require_generation(kCheckpointOperation, request.session, &info, execution))
        return;
    StepTopologyEditJournalCheckpoint checkpoint;
    Status status;
    if (session_store().checkpoint_edit_journal(request.session.session_handle, &checkpoint,
                                                &status) != 0)
    {
        fail(execution, kCheckpointOperation,
             diagnostic(kCheckpointOperation,
                        "geometer.operation.step_topology.checkpoint_edit_journal_failed",
                        status.message));
        return;
    }
    if (session_store().info(request.session.session_handle, &info, &status) != 0)
    {
        fail(execution, kCheckpointOperation,
             diagnostic(kCheckpointOperation,
                        "geometer.operation.step_topology.checkpoint_edit_journal_failed",
                        status.message));
        return;
    }
    contracts::StepTopologyCheckpointEditJournalResultA0 result;
    result.state = mutation_state(info);
    result.source_sha256 = checkpoint.source_sha256;
    result.source_brep_sha256 = checkpoint.source_brep_sha256;
    result.target_inventory_sha256 = checkpoint.target_inventory_sha256;
    result.occt_version = checkpoint.occt_version;
    result.transaction_count = static_cast<std::uint32_t>(checkpoint.transaction_count);
    result.journal.bytes = static_cast<std::uint32_t>(checkpoint.bytes.size());
    result.journal.sha256 = checkpoint.content_sha256;
    succeed(execution, kCheckpointOperation, std::move(result));
    OperationOutputAttachment attachment;
    attachment.name = "edit_journal";
    attachment.media_type = "application/vnd.wavenumber.geometer.step-topology-edit-journal";
    attachment.data = std::move(checkpoint.bytes);
    execution->attachments.push_back(std::move(attachment));
}

void execute_restore(const unsigned char* request_json, std::size_t request_json_size,
                     const std::vector<OperationAttachmentView>& attachments,
                     OperationExecution* execution)
{
    contracts::StepTopologyRestoreRequestA0 request;
    if (!decode_request(kRestoreOperation, request_json, request_json_size, &request, execution))
        return;
    if (request.include_diagnostics)
    {
        fail(execution, kRestoreOperation,
             diagnostic(kRestoreOperation, "geometer.operation.step_topology.unsupported_option",
                        "include_diagnostics is deferred until diagnostic carriers have a bounded "
                        "wire contract.",
                        "/include_diagnostics"));
        return;
    }
    if (attachments.size() != 2U)
    {
        fail(execution, kRestoreOperation,
             diagnostic(kRestoreOperation, "geometer.contract.attachment_count_mismatch",
                        "The native topology restore operation requires exactly source and "
                        "state_artifact attachments.",
                        "/attachments"));
        return;
    }
    const auto* source = named_attachment(kRestoreOperation, attachments, "source", execution);
    const auto* state_artifact =
        named_attachment(kRestoreOperation, attachments, "state_artifact", execution);
    if (source == nullptr || state_artifact == nullptr)
        return;
    const auto* journal_descriptor =
        std::get_if<contracts::EditJournalPersistenceArtifact>(&request.state_artifact);
    if (journal_descriptor == nullptr)
    {
        fail(execution, kRestoreOperation,
             diagnostic(kRestoreOperation, "geometer.operation.step_topology.unsupported_carrier",
                        "The native restore slice currently supports only edit_journal state "
                        "artifacts.",
                        "/state_artifact/carrier"));
        return;
    }
    if (!request.replay_preconditions.has_value())
    {
        fail(execution, kRestoreOperation,
             diagnostic(kRestoreOperation,
                        "geometer.operation.step_topology.missing_replay_preconditions",
                        "Edit-journal restore requires explicit replay preconditions.",
                        "/replay_preconditions"));
        return;
    }
    const bool source_media_valid =
        source->media_type == "application/step" || source->media_type == "model/step";
    const std::string source_sha256 = sha256_hex(source->data, source->size);
    const std::string journal_sha256 = sha256_hex(state_artifact->data, state_artifact->size);
    if (!source_media_valid || state_artifact->media_type != journal_descriptor->media_type ||
        request.source.bytes != source->size || request.source.sha256 != source_sha256 ||
        journal_descriptor->bytes != state_artifact->size ||
        journal_descriptor->sha256 != journal_sha256)
    {
        fail(execution, kRestoreOperation,
             diagnostic(kRestoreOperation, "geometer.contract.attachment_descriptor_mismatch",
                        "Restore attachment bytes, media types, or SHA-256 descriptors do not "
                        "match.",
                        "/attachments"));
        return;
    }

    const auto& replay = *request.replay_preconditions;
    StepTopologyEditJournalReplayPreconditions preconditions;
    preconditions.source_sha256 = replay.source_sha256;
    preconditions.source_brep_sha256 = replay.source_brep_sha256;
    preconditions.target_inventory_sha256 = replay.target_inventory_sha256;
    preconditions.occt_version = replay.occt_version;
    preconditions.transaction_count = replay.transaction_count;
    StepTopologyOpenResult opened;
    StepTopologyEditJournalRestoreResult restored;
    Status status;
    if (session_store().open_step_with_edit_journal(
            source->data, source->size, state_artifact->data, state_artifact->size, preconditions,
            &opened, &restored, &status) != 0)
    {
        fail(execution, kRestoreOperation,
             diagnostic(kRestoreOperation, "geometer.operation.step_topology.restore_failed",
                        status.message));
        return;
    }
    contracts::StepTopologyRestoreResultA0 result;
    result.session = session_reference(opened.session);
    result.source = request.source;
    result.tool.release_version = version_string();
    result.tool.occt_version = opened.session.occt_version;
    result.replayed_transaction_count = replay.transaction_count;
    result.evicted_session_handles = std::move(opened.evicted_session_handles);
    succeed(execution, kRestoreOperation, std::move(result));
}

} // namespace

void execute_native_operation(const std::string& operation_id, const unsigned char* request_json,
                              std::size_t request_json_size,
                              const std::vector<OperationAttachmentView>& attachments,
                              OperationExecution* execution)
{
    if (operation_id == kOpenOperation)
    {
        execute_open(request_json, request_json_size, attachments, execution);
        return;
    }
    if (operation_id == kInspectOperation)
    {
        execute_inspect(request_json, request_json_size, attachments, execution);
        return;
    }
    if (operation_id == kCloseOperation)
    {
        execute_close(request_json, request_json_size, attachments, execution);
        return;
    }
    if (operation_id == kRenderOperation)
    {
        execute_render(request_json, request_json_size, attachments, execution);
        return;
    }
    if (operation_id == kResolveHitOperation)
    {
        execute_resolve_hit(request_json, request_json_size, attachments, execution);
        return;
    }
    if (operation_id == kApplyGroupsOperation)
    {
        execute_apply_groups(request_json, request_json_size, attachments, execution);
        return;
    }
    if (operation_id == kApplyProbesOperation)
    {
        execute_apply_probes(request_json, request_json_size, attachments, execution);
        return;
    }
    if (operation_id == kCheckpointOperation)
    {
        execute_checkpoint(request_json, request_json_size, attachments, execution);
        return;
    }
    if (operation_id == kRestoreOperation)
    {
        execute_restore(request_json, request_json_size, attachments, execution);
        return;
    }
    execute_operation(operation_id, request_json, request_json_size, attachments, execution);
}

} // namespace geometer

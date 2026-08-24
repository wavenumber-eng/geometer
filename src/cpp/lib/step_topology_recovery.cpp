#include "geometer/step_topology_recovery.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <exception>
#include <limits>
#include <new>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace geometer
{
namespace
{

constexpr int kInvalidArgument = 101;
constexpr int kResourceLimit = 102;
constexpr int kInternalFailure = 108;

void set_status(Status* status, int code, const std::string& message)
{
    if (status != nullptr)
    {
        status->code = code;
        status->message = message;
    }
}

bool checked_add(std::size_t* total, std::size_t value)
{
    if (*total > std::numeric_limits<std::size_t>::max() - value)
        return false;
    *total += value;
    return true;
}

bool valid_sha256(const std::string& value)
{
    return value.size() == 64U && std::all_of(value.begin(), value.end(),
                                              [](unsigned char character)
                                              {
                                                  return (character >= '0' && character <= '9') ||
                                                         (character >= 'a' && character <= 'f');
                                              });
}

bool finite_fingerprint(const StepTopologyRecoveryFingerprint& fingerprint)
{
    if (!fingerprint.available)
        return true;
    if (fingerprint.normalized_length_unit != "millimeter" ||
        fingerprint.coordinate_frame.empty() || fingerprint.occurrence_context.empty() ||
        fingerprint.geometry_kind.empty() || !valid_sha256(fingerprint.adjacency_sha256) ||
        !std::isfinite(fingerprint.area_mm2) || !std::isfinite(fingerprint.volume_mm3) ||
        fingerprint.area_mm2 < 0.0 || fingerprint.volume_mm3 < 0.0)
        return false;
    for (double value : fingerprint.centroid_mm)
    {
        if (!std::isfinite(value))
            return false;
    }
    for (double value : fingerprint.bounds_mm)
    {
        if (!std::isfinite(value))
            return false;
    }
    for (std::size_t axis = 0; axis < 3U; ++axis)
    {
        if (fingerprint.bounds_mm[axis] > fingerprint.bounds_mm[axis + 3U])
            return false;
    }
    return true;
}

bool close(double left, double right, double tolerance)
{
    return std::abs(left - right) <= tolerance;
}

bool local_fingerprint_matches(const StepTopologyRecoveryFingerprint& source,
                               const StepTopologyRecoveryFingerprint& candidate,
                               StepTopologyTargetKind kind,
                               const StepTopologyRecoveryTolerances& tolerances)
{
    if (!source.available || !candidate.available ||
        source.normalized_length_unit != candidate.normalized_length_unit ||
        source.coordinate_frame != candidate.coordinate_frame ||
        source.geometry_kind != candidate.geometry_kind ||
        source.adjacency_sha256 != candidate.adjacency_sha256)
        return false;
    if (kind == StepTopologyTargetKind::face &&
        !close(source.area_mm2, candidate.area_mm2, tolerances.area_mm2))
        return false;
    if (kind == StepTopologyTargetKind::body &&
        !close(source.volume_mm3, candidate.volume_mm3, tolerances.volume_mm3))
        return false;
    for (std::size_t index = 0; index < source.centroid_mm.size(); ++index)
    {
        if (!close(source.centroid_mm[index], candidate.centroid_mm[index], tolerances.length_mm))
            return false;
    }
    for (std::size_t index = 0; index < source.bounds_mm.size(); ++index)
    {
        if (!close(source.bounds_mm[index], candidate.bounds_mm[index], tolerances.length_mm))
            return false;
    }
    return true;
}

bool complete_fingerprint_matches(const StepTopologyRecoveryFingerprint& source,
                                  const StepTopologyRecoveryFingerprint& candidate,
                                  StepTopologyTargetKind kind,
                                  const StepTopologyRecoveryTolerances& tolerances)
{
    return local_fingerprint_matches(source, candidate, kind, tolerances) &&
           source.occurrence_context == candidate.occurrence_context;
}

std::vector<std::string> fingerprint_fields(StepTopologyTargetKind kind)
{
    std::vector<std::string> fields = {
        "normalized_length_unit", "coordinate_frame", "occurrence_context",
        "geometry_kind",          "centroid_mm",      "bounds_mm",
        "adjacency_sha256"};
    fields.push_back(kind == StepTopologyTargetKind::body ? "volume_mm3" : "area_mm2");
    return fields;
}

StepTopologyRecoveryTopologyComparison
compare_topology(const StepTopologyRecoveryMemberRequest& member,
                 const StepTopologyRecoveryCandidate& candidate,
                 StepTopologyRecoveryResolutionMethod resolution_method,
                 const StepTopologyRecoveryTolerances& tolerances)
{
    if (resolution_method == StepTopologyRecoveryResolutionMethod::authored_id_topology_link ||
        resolution_method == StepTopologyRecoveryResolutionMethod::validated_carrier_locator)
    {
        if (candidate.lineage == StepTopologyRecoveryLineage::split_from_source)
            return StepTopologyRecoveryTopologyComparison::split;
        if (candidate.lineage == StepTopologyRecoveryLineage::merged_from_sources)
            return StepTopologyRecoveryTopologyComparison::merged;
    }
    if (!member.source_fingerprint.available || !candidate.fingerprint.available)
        return StepTopologyRecoveryTopologyComparison::unavailable;
    if (!local_fingerprint_matches(member.source_fingerprint, candidate.fingerprint, member.kind,
                                   tolerances))
        return StepTopologyRecoveryTopologyComparison::otherwise_changed;
    if (member.source_fingerprint.occurrence_context != candidate.fingerprint.occurrence_context)
        return StepTopologyRecoveryTopologyComparison::relocated;
    return StepTopologyRecoveryTopologyComparison::unchanged;
}

bool account_string(const std::string& value, const StepTopologyLimits& limits, std::size_t* total)
{
    return value.size() <= limits.max_string_bytes && checked_add(total, value.size()) &&
           *total <= limits.max_total_string_bytes;
}

int validate_request(const StepTopologyRecoveryGroupRequest& request,
                     const StepTopologyLimits& limits, Status* status)
{
    if (request.group_authored_id.empty() || request.members.empty() ||
        request.members.size() > limits.max_group_members ||
        !valid_sha256(request.provenance.source_artifact_sha256) ||
        !valid_sha256(request.provenance.candidate_artifact_sha256) ||
        request.provenance.source_occt_version.empty() ||
        request.provenance.candidate_occt_version.empty() ||
        request.provenance.source_driver.empty() || request.provenance.candidate_driver.empty() ||
        request.provenance.source_writer_settings.empty() ||
        request.provenance.candidate_writer_settings.empty() ||
        request.provenance.command_provenance.empty() ||
        !std::isfinite(request.provenance.measured_wall_time_milliseconds) ||
        request.provenance.measured_wall_time_milliseconds < 0.0 ||
        !std::isfinite(request.tolerances.length_mm) ||
        !std::isfinite(request.tolerances.area_mm2) ||
        !std::isfinite(request.tolerances.volume_mm3) || request.tolerances.length_mm <= 0.0 ||
        request.tolerances.area_mm2 <= 0.0 || request.tolerances.volume_mm3 <= 0.0)
    {
        set_status(status, kInvalidArgument, "Recovery request or tolerances are invalid.");
        return kInvalidArgument;
    }
    std::size_t total_string_bytes = 0;
    std::size_t total_candidates = 0;
    std::unordered_set<std::string> member_ids;
    if (!account_string(request.group_authored_id, limits, &total_string_bytes))
    {
        set_status(status, kResourceLimit, "Recovery request exceeds its string limits.");
        return kResourceLimit;
    }
    for (const std::string* value :
         {&request.provenance.source_artifact_sha256, &request.provenance.candidate_artifact_sha256,
          &request.provenance.source_occt_version, &request.provenance.candidate_occt_version,
          &request.provenance.source_driver, &request.provenance.candidate_driver,
          &request.provenance.source_writer_settings, &request.provenance.candidate_writer_settings,
          &request.provenance.command_provenance})
    {
        if (!account_string(*value, limits, &total_string_bytes))
        {
            set_status(status, kResourceLimit, "Recovery request exceeds its string limits.");
            return kResourceLimit;
        }
    }
    for (const StepTopologyRecoveryMemberRequest& member : request.members)
    {
        if (member.member_record_id.empty() || !member_ids.insert(member.member_record_id).second ||
            !finite_fingerprint(member.source_fingerprint))
        {
            set_status(status, kInvalidArgument,
                       "Recovery members require unique ids and finite fingerprints.");
            return kInvalidArgument;
        }
        if (!checked_add(&total_candidates, member.candidates.size()) ||
            total_candidates > limits.max_handles)
        {
            set_status(status, kResourceLimit, "Recovery candidate count exceeds its limit.");
            return kResourceLimit;
        }
        for (const std::string* value :
             {&member.member_record_id, &member.authored_target_id, &member.carrier_locator,
              &member.source_fingerprint.normalized_length_unit,
              &member.source_fingerprint.coordinate_frame,
              &member.source_fingerprint.occurrence_context,
              &member.source_fingerprint.geometry_kind,
              &member.source_fingerprint.adjacency_sha256})
        {
            if (!account_string(*value, limits, &total_string_bytes))
            {
                set_status(status, kResourceLimit, "Recovery request exceeds its string limits.");
                return kResourceLimit;
            }
        }
        std::unordered_set<std::string> candidate_handles;
        for (const StepTopologyRecoveryCandidate& candidate : member.candidates)
        {
            if (candidate.target_handle.empty() ||
                !candidate_handles.insert(candidate.target_handle).second ||
                !finite_fingerprint(candidate.fingerprint) ||
                (candidate.topology_link_verified && candidate.authored_target_id.empty()) ||
                (candidate.carrier_locator_validated && candidate.carrier_locator.empty()) ||
                ((candidate.topology_link_verified || candidate.carrier_locator_validated) &&
                 candidate.carrier_record.empty()) ||
                (candidate.lineage != StepTopologyRecoveryLineage::none &&
                 !candidate.topology_link_verified && !candidate.carrier_locator_validated))
            {
                set_status(status, kInvalidArgument,
                           "Recovery candidates require unique handles and finite fingerprints.");
                return kInvalidArgument;
            }
            for (const std::string* value :
                 {&candidate.target_handle, &candidate.authored_target_id,
                  &candidate.carrier_locator, &candidate.carrier_record,
                  &candidate.fingerprint.normalized_length_unit,
                  &candidate.fingerprint.coordinate_frame,
                  &candidate.fingerprint.occurrence_context, &candidate.fingerprint.geometry_kind,
                  &candidate.fingerprint.adjacency_sha256})
            {
                if (!account_string(*value, limits, &total_string_bytes))
                {
                    set_status(status, kResourceLimit,
                               "Recovery request exceeds its string limits.");
                    return kResourceLimit;
                }
            }
        }
    }
    return 0;
}

StepTopologyRecoveryMemberResult analyze_member(const StepTopologyRecoveryMemberRequest& member,
                                                const StepTopologyRecoveryTolerances& tolerances)
{
    StepTopologyRecoveryMemberResult result;
    result.member_record_id = member.member_record_id;
    result.kind = member.kind;
    result.authored_target_id = member.authored_target_id;
    result.evidence.candidate_count = member.candidates.size();
    result.evidence.tolerances = tolerances;
    for (const StepTopologyRecoveryCandidate& candidate : member.candidates)
    {
        if (!candidate.carrier_record.empty())
            result.evidence.carrier_records.push_back(candidate.carrier_record);
    }
    if (member.kind != StepTopologyTargetKind::body && member.kind != StepTopologyTargetKind::face)
    {
        result.resolution_state = StepTopologyRecoveryResolutionState::unsupported;
        result.topology_comparison = StepTopologyRecoveryTopologyComparison::unavailable;
        return result;
    }

    std::vector<std::size_t> authored_matches;
    std::vector<std::size_t> locator_matches;
    std::vector<std::size_t> geometry_matches;
    for (std::size_t index = 0; index < member.candidates.size(); ++index)
    {
        const StepTopologyRecoveryCandidate& candidate = member.candidates[index];
        if (candidate.kind != member.kind)
            continue;
        if (!member.authored_target_id.empty() &&
            candidate.authored_target_id == member.authored_target_id &&
            candidate.topology_link_verified)
            authored_matches.push_back(index);
        if (!member.carrier_locator.empty() &&
            candidate.carrier_locator == member.carrier_locator &&
            candidate.carrier_locator_validated)
            locator_matches.push_back(index);
        if (complete_fingerprint_matches(member.source_fingerprint, candidate.fingerprint,
                                         member.kind, tolerances))
            geometry_matches.push_back(index);
    }

    const std::vector<std::size_t>* matches = nullptr;
    if (!authored_matches.empty())
    {
        matches = &authored_matches;
        result.resolution_method = StepTopologyRecoveryResolutionMethod::authored_id_topology_link;
        result.evidence.compared_fields = {"authored_target_id", "topology_link"};
        result.confidence = StepTopologyRecoveryConfidence::high;
    }
    else if (!locator_matches.empty())
    {
        matches = &locator_matches;
        result.resolution_method = StepTopologyRecoveryResolutionMethod::validated_carrier_locator;
        result.evidence.compared_fields = {"carrier_locator", "carrier_locator_validation"};
        result.confidence = StepTopologyRecoveryConfidence::high;
    }
    else if (!geometry_matches.empty())
    {
        matches = &geometry_matches;
        result.resolution_method =
            StepTopologyRecoveryResolutionMethod::unique_geometry_adjacency_fingerprint;
        result.evidence.compared_fields = fingerprint_fields(member.kind);
        result.confidence = StepTopologyRecoveryConfidence::medium;
    }

    if (matches == nullptr)
    {
        result.resolution_state = StepTopologyRecoveryResolutionState::unresolved;
        if (!member.authored_target_id.empty())
        {
            result.evidence.compared_fields.push_back("authored_target_id");
            result.evidence.compared_fields.push_back("topology_link");
        }
        if (!member.carrier_locator.empty())
        {
            result.evidence.compared_fields.push_back("carrier_locator");
            result.evidence.compared_fields.push_back("carrier_locator_validation");
        }
        if (member.source_fingerprint.available)
        {
            std::vector<std::string> fields = fingerprint_fields(member.kind);
            result.evidence.compared_fields.insert(result.evidence.compared_fields.end(),
                                                   fields.begin(), fields.end());
        }
        for (const StepTopologyRecoveryCandidate& candidate : member.candidates)
            result.evidence.rejected_alternatives.push_back(
                {candidate.target_handle, "no_supported_evidence_match"});
        return result;
    }
    result.evidence.matching_candidate_count = matches->size();
    if (matches->size() != 1)
    {
        result.resolution_state = StepTopologyRecoveryResolutionState::ambiguous;
        result.confidence = StepTopologyRecoveryConfidence::none;
        std::size_t match_position = 0;
        for (std::size_t index = 0; index < member.candidates.size(); ++index)
        {
            if (match_position < matches->size() && (*matches)[match_position] == index)
            {
                ++match_position;
                continue;
            }
            result.evidence.rejected_alternatives.push_back(
                {member.candidates[index].target_handle, "lower_or_nonmatching_evidence"});
        }
        return result;
    }

    const std::size_t selected_index = matches->front();
    const StepTopologyRecoveryCandidate& selected = member.candidates[selected_index];
    result.resolution_state = StepTopologyRecoveryResolutionState::resolved;
    result.resolved_target_handle = selected.target_handle;
    result.topology_comparison =
        compare_topology(member, selected, result.resolution_method, tolerances);
    if (result.topology_comparison == StepTopologyRecoveryTopologyComparison::split ||
        result.topology_comparison == StepTopologyRecoveryTopologyComparison::merged)
    {
        result.evidence.compared_fields.push_back("lineage");
    }
    else if (result.topology_comparison == StepTopologyRecoveryTopologyComparison::unavailable)
    {
        result.evidence.compared_fields.push_back("fingerprint_availability");
    }
    else if (result.resolution_method !=
             StepTopologyRecoveryResolutionMethod::unique_geometry_adjacency_fingerprint)
    {
        std::vector<std::string> fields = fingerprint_fields(member.kind);
        result.evidence.compared_fields.insert(result.evidence.compared_fields.end(),
                                               fields.begin(), fields.end());
    }
    for (std::size_t index = 0; index < member.candidates.size(); ++index)
    {
        if (index != selected_index)
            result.evidence.rejected_alternatives.push_back(
                {member.candidates[index].target_handle, "lower_or_nonmatching_evidence"});
    }
    return result;
}

} // namespace

int analyze_step_topology_recovery(const StepTopologyRecoveryGroupRequest& request,
                                   const StepTopologyLimits& limits,
                                   StepTopologyRecoveryGroupResult* result, Status* status)
{
    if (result == nullptr)
    {
        set_status(status, kInvalidArgument, "Recovery result is null.");
        return kInvalidArgument;
    }
    *result = {};
    try
    {
        const int validation = validate_request(request, limits, status);
        if (validation != 0)
            return validation;

        result->group_authored_id = request.group_authored_id;
        result->provenance = request.provenance;
        result->members.reserve(request.members.size());
        for (const StepTopologyRecoveryMemberRequest& member : request.members)
        {
            StepTopologyRecoveryMemberResult member_result =
                analyze_member(member, request.tolerances);
            switch (member_result.resolution_state)
            {
            case StepTopologyRecoveryResolutionState::resolved:
                ++result->resolved_member_count;
                break;
            case StepTopologyRecoveryResolutionState::ambiguous:
                ++result->ambiguous_member_count;
                break;
            case StepTopologyRecoveryResolutionState::unresolved:
                ++result->unresolved_member_count;
                break;
            case StepTopologyRecoveryResolutionState::unsupported:
                ++result->unsupported_member_count;
                break;
            }
            result->members.push_back(std::move(member_result));
        }

        if (result->resolved_member_count == result->members.size())
        {
            result->resolution_state = StepTopologyRecoveryResolutionState::resolved;
            result->completeness = StepTopologyRecoveryGroupCompleteness::fully_recovered;
        }
        else if (result->resolved_member_count != 0)
        {
            result->resolution_state = result->ambiguous_member_count != 0
                                           ? StepTopologyRecoveryResolutionState::ambiguous
                                           : StepTopologyRecoveryResolutionState::unresolved;
            result->completeness = StepTopologyRecoveryGroupCompleteness::partially_recovered;
        }
        else if (result->unsupported_member_count == result->members.size())
        {
            result->resolution_state = StepTopologyRecoveryResolutionState::unsupported;
            result->completeness = StepTopologyRecoveryGroupCompleteness::unsupported;
        }
        else
        {
            result->resolution_state = result->ambiguous_member_count != 0
                                           ? StepTopologyRecoveryResolutionState::ambiguous
                                           : StepTopologyRecoveryResolutionState::unresolved;
            result->completeness = StepTopologyRecoveryGroupCompleteness::unrecovered;
        }
        set_status(status, 0, "");
        return 0;
    }
    catch (const std::bad_alloc&)
    {
        *result = {};
        set_status(status, kResourceLimit, "Recovery analysis exhausted memory.");
        return kResourceLimit;
    }
    catch (const std::exception& error)
    {
        *result = {};
        set_status(status, kInternalFailure, error.what());
        return kInternalFailure;
    }
}

} // namespace geometer

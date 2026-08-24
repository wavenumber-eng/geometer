#include "geometer/step_topology_recovery.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{

constexpr const char* kAdjacencyA =
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
constexpr const char* kAdjacencyB =
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

geometer::StepTopologyRecoveryFingerprint
face_fingerprint(const std::string& occurrence, double area = 100.0,
                 const std::string& adjacency = kAdjacencyA)
{
    geometer::StepTopologyRecoveryFingerprint fingerprint;
    fingerprint.available = true;
    fingerprint.coordinate_frame = "definition-local-right-handed";
    fingerprint.occurrence_context = occurrence;
    fingerprint.geometry_kind = "plane";
    fingerprint.area_mm2 = area;
    fingerprint.centroid_mm = {5.0, 5.0, 0.0};
    fingerprint.bounds_mm = {0.0, 0.0, 0.0, 10.0, 10.0, 0.0};
    fingerprint.adjacency_sha256 = adjacency;
    return fingerprint;
}

geometer::StepTopologyRecoveryFingerprint body_fingerprint(const std::string& occurrence,
                                                           double volume = 6000.0)
{
    geometer::StepTopologyRecoveryFingerprint fingerprint;
    fingerprint.available = true;
    fingerprint.coordinate_frame = "definition-local-right-handed";
    fingerprint.occurrence_context = occurrence;
    fingerprint.geometry_kind = "solid";
    fingerprint.area_mm2 = 2200.0;
    fingerprint.volume_mm3 = volume;
    fingerprint.centroid_mm = {5.0, 10.0, 15.0};
    fingerprint.bounds_mm = {0.0, 0.0, 0.0, 10.0, 20.0, 30.0};
    fingerprint.adjacency_sha256 = kAdjacencyA;
    return fingerprint;
}

geometer::StepTopologyRecoveryCandidate
candidate(const std::string& handle, const geometer::StepTopologyRecoveryFingerprint& fingerprint)
{
    geometer::StepTopologyRecoveryCandidate value;
    value.target_handle = handle;
    value.fingerprint = fingerprint;
    return value;
}

geometer::StepTopologyRecoveryMemberRequest member(const std::string& record_id)
{
    geometer::StepTopologyRecoveryMemberRequest value;
    value.member_record_id = record_id;
    value.source_fingerprint = face_fingerprint("occurrence:source");
    return value;
}

geometer::StepTopologyRecoveryGroupRequest group_request(const std::string& authored_id)
{
    geometer::StepTopologyRecoveryGroupRequest request;
    request.group_authored_id = authored_id;
    request.provenance.source_artifact_sha256 = kAdjacencyA;
    request.provenance.candidate_artifact_sha256 = kAdjacencyB;
    request.provenance.source_occt_version = "8.0.1";
    request.provenance.candidate_occt_version = "8.0.1";
    request.provenance.source_driver = "step-ap242";
    request.provenance.candidate_driver = "step-ap242";
    request.provenance.source_writer_settings = "external-input:unknown";
    request.provenance.candidate_writer_settings = "schema=AP242DIS;metadata=true";
    request.provenance.command_provenance = "geometer-recovery-test:a0";
    request.provenance.measured_wall_time_milliseconds = 12.5;
    return request;
}

geometer::StepTopologyRecoveryGroupResult
analyze(const geometer::StepTopologyRecoveryGroupRequest& request)
{
    geometer::StepTopologyRecoveryGroupResult result;
    geometer::Status status;
    require(geometer::analyze_step_topology_recovery(request, {}, &result, &status) == 0,
            "recovery analysis failed: " + status.message);
    return result;
}

void durable_identity_is_independent_from_topology_change()
{
    auto request = group_request("wn.geometer.research.group.durable-change");
    auto source = member("member:face:durable");
    source.authored_target_id = "wn.geometer.research.target.face.durable";
    auto changed =
        candidate("face:changed", face_fingerprint("occurrence:source", 125.0, kAdjacencyB));
    changed.authored_target_id = source.authored_target_id;
    changed.topology_link_verified = true;
    changed.carrier_record = "ap242:gisu:verified";
    source.candidates.push_back(changed);
    source.candidates.push_back(candidate("face:geometry-decoy", source.source_fingerprint));
    request.members.push_back(source);

    const auto result = analyze(request);
    const auto& recovered = result.members[0];
    require(
        result.completeness == geometer::StepTopologyRecoveryGroupCompleteness::fully_recovered &&
            recovered.resolution_state == geometer::StepTopologyRecoveryResolutionState::resolved &&
            recovered.resolution_method ==
                geometer::StepTopologyRecoveryResolutionMethod::authored_id_topology_link &&
            recovered.topology_comparison ==
                geometer::StepTopologyRecoveryTopologyComparison::otherwise_changed &&
            recovered.confidence == geometer::StepTopologyRecoveryConfidence::high &&
            recovered.resolved_target_handle == "face:changed" &&
            recovered.evidence.candidate_count == 2 &&
            recovered.evidence.matching_candidate_count == 1 &&
            recovered.evidence.carrier_records.size() == 1 &&
            recovered.evidence.rejected_alternatives.size() == 1 &&
            std::find(recovered.evidence.compared_fields.begin(),
                      recovered.evidence.compared_fields.end(),
                      "adjacency_sha256") != recovered.evidence.compared_fields.end() &&
            std::find(recovered.evidence.compared_fields.begin(),
                      recovered.evidence.compared_fields.end(),
                      "area_mm2") != recovered.evidence.compared_fields.end() &&
            result.provenance.candidate_artifact_sha256 == kAdjacencyB &&
            result.provenance.measured_wall_time_milliseconds == 12.5,
        "a proven authored topology link must survive a separately reported geometry change");
}

void body_geometry_recovery_uses_volume_policy()
{
    auto request = group_request("wn.geometer.research.group.body");
    auto source = member("member:body:geometry");
    source.kind = geometer::StepTopologyTargetKind::body;
    source.source_fingerprint = body_fingerprint("occurrence:source");
    auto exact = candidate("body:exact", source.source_fingerprint);
    exact.kind = geometer::StepTopologyTargetKind::body;
    exact.fingerprint.area_mm2 = 999999.0;
    auto wrong_volume =
        candidate("body:wrong-volume", body_fingerprint("occurrence:source", 6001.0));
    wrong_volume.kind = geometer::StepTopologyTargetKind::body;
    source.candidates = {exact, wrong_volume};
    request.members.push_back(source);

    const auto result = analyze(request);
    const auto& recovered = result.members[0];
    require(recovered.resolution_state == geometer::StepTopologyRecoveryResolutionState::resolved &&
                recovered.resolution_method == geometer::StepTopologyRecoveryResolutionMethod::
                                                   unique_geometry_adjacency_fingerprint &&
                recovered.topology_comparison ==
                    geometer::StepTopologyRecoveryTopologyComparison::unchanged &&
                recovered.resolved_target_handle == "body:exact" &&
                recovered.evidence.matching_candidate_count == 1 &&
                std::find(recovered.evidence.compared_fields.begin(),
                          recovered.evidence.compared_fields.end(),
                          "volume_mm3") != recovered.evidence.compared_fields.end() &&
                std::find(recovered.evidence.compared_fields.begin(),
                          recovered.evidence.compared_fields.end(),
                          "area_mm2") == recovered.evidence.compared_fields.end(),
            "body geometry recovery must compare volume rather than face area");
}

void validated_carrier_precedes_geometry_and_reports_relocation()
{
    auto request = group_request("wn.geometer.research.group.carrier");
    auto source = member("member:face:carrier");
    source.carrier_locator = "ap242:gisu:face:17";
    auto relocated = candidate("face:relocated", face_fingerprint("occurrence:moved"));
    relocated.carrier_locator = source.carrier_locator;
    relocated.carrier_locator_validated = true;
    relocated.carrier_record = "ap242:gisu:locator-validated";
    source.candidates.push_back(relocated);
    source.candidates.push_back(candidate("face:geometry-only", source.source_fingerprint));
    request.members.push_back(source);

    const auto result = analyze(request);
    const auto& recovered = result.members[0];
    require(recovered.resolution_method ==
                    geometer::StepTopologyRecoveryResolutionMethod::validated_carrier_locator &&
                recovered.resolved_target_handle == "face:relocated" &&
                recovered.topology_comparison ==
                    geometer::StepTopologyRecoveryTopologyComparison::relocated,
            "a validated carrier must outrank geometry and report relocation independently");
}

void symmetric_geometry_fails_closed()
{
    auto request = group_request("wn.geometer.research.group.symmetric");
    auto source = member("member:face:symmetric");
    source.candidates.push_back(candidate("face:symmetric:a", source.source_fingerprint));
    source.candidates.push_back(candidate("face:symmetric:b", source.source_fingerprint));
    request.members.push_back(source);

    const auto result = analyze(request);
    const auto& recovered = result.members[0];
    require(recovered.resolution_state ==
                    geometer::StepTopologyRecoveryResolutionState::ambiguous &&
                recovered.resolution_method == geometer::StepTopologyRecoveryResolutionMethod::
                                                   unique_geometry_adjacency_fingerprint &&
                recovered.resolved_target_handle.empty() &&
                recovered.evidence.matching_candidate_count == 2 &&
                recovered.confidence == geometer::StepTopologyRecoveryConfidence::none,
            "symmetric geometry candidates must remain ambiguous without order tie-breaking");
}

void group_completeness_cannot_hide_partial_loss()
{
    auto request = group_request("wn.geometer.research.group.partial");
    auto recovered = member("member:face:recovered");
    recovered.candidates.push_back(candidate("face:recovered", recovered.source_fingerprint));
    auto missing = member("member:face:missing");
    missing.candidates.push_back(
        candidate("face:not-a-match", face_fingerprint("occurrence:source", 300.0)));
    request.members = {recovered, missing};

    const auto result = analyze(request);
    require(result.members.size() == 2 && result.resolved_member_count == 1 &&
                result.unresolved_member_count == 1 && result.ambiguous_member_count == 0 &&
                result.completeness ==
                    geometer::StepTopologyRecoveryGroupCompleteness::partially_recovered &&
                result.resolution_state ==
                    geometer::StepTopologyRecoveryResolutionState::unresolved,
            "aggregate recovery must expose one recovered and one lost member");
}

void split_merge_and_unsupported_are_explicit()
{
    auto request = group_request("wn.geometer.research.group.lineage");
    auto split = member("member:face:split");
    split.authored_target_id = "wn.geometer.research.target.face.split";
    auto split_candidate = candidate("face:split:a", face_fingerprint("occurrence:source", 50.0));
    split_candidate.authored_target_id = split.authored_target_id;
    split_candidate.topology_link_verified = true;
    split_candidate.carrier_record = "ap242:gisu:split-lineage";
    split_candidate.lineage = geometer::StepTopologyRecoveryLineage::split_from_source;
    split.candidates.push_back(split_candidate);

    auto merged = member("member:face:merged");
    merged.authored_target_id = "wn.geometer.research.target.face.merged";
    auto merged_candidate = candidate("face:merged", face_fingerprint("occurrence:source", 200.0));
    merged_candidate.authored_target_id = merged.authored_target_id;
    merged_candidate.topology_link_verified = true;
    merged_candidate.carrier_record = "ap242:gisu:merged-lineage";
    merged_candidate.lineage = geometer::StepTopologyRecoveryLineage::merged_from_sources;
    merged.candidates.push_back(merged_candidate);

    auto geometry_only = member("member:face:geometry-only-lineage");
    auto foreign_lineage = candidate("face:foreign-lineage", geometry_only.source_fingerprint);
    foreign_lineage.authored_target_id = "wn.geometer.research.target.face.someone-else";
    foreign_lineage.topology_link_verified = true;
    foreign_lineage.carrier_record = "ap242:gisu:foreign-lineage";
    foreign_lineage.lineage = geometer::StepTopologyRecoveryLineage::split_from_source;
    geometry_only.candidates.push_back(foreign_lineage);

    auto unsupported = member("member:shell:unsupported");
    unsupported.kind = geometer::StepTopologyTargetKind::shell;
    request.members = {split, merged, geometry_only, unsupported};

    const auto result = analyze(request);
    require(result.members[0].topology_comparison ==
                    geometer::StepTopologyRecoveryTopologyComparison::split &&
                result.members[1].topology_comparison ==
                    geometer::StepTopologyRecoveryTopologyComparison::merged &&
                result.members[2].resolution_method ==
                    geometer::StepTopologyRecoveryResolutionMethod::
                        unique_geometry_adjacency_fingerprint &&
                result.members[2].topology_comparison ==
                    geometer::StepTopologyRecoveryTopologyComparison::unchanged &&
                result.members[3].resolution_state ==
                    geometer::StepTopologyRecoveryResolutionState::unsupported &&
                result.completeness ==
                    geometer::StepTopologyRecoveryGroupCompleteness::partially_recovered,
            "split, merged, and unsupported dimensions must remain explicit");
}

void malformed_and_over_limit_requests_fail_without_output()
{
    auto request = group_request("wn.geometer.research.group.invalid");
    request.members = {member("member:duplicate"), member("member:duplicate")};
    geometer::StepTopologyRecoveryGroupResult result;
    result.group_authored_id = "stale";
    geometer::Status status;
    require(geometer::analyze_step_topology_recovery(request, {}, &result, &status) != 0 &&
                result.group_authored_id.empty(),
            "duplicate member ids must fail without stale output");

    request.members = {member("member:bounded")};
    request.members[0].candidates.push_back(
        candidate("face:one", request.members[0].source_fingerprint));
    geometer::StepTopologyLimits limits;
    limits.max_handles = 0;
    require(geometer::analyze_step_topology_recovery(request, limits, &result, &status) != 0 &&
                result.members.empty(),
            "candidate limits must reject before publishing partial results");
}

void high_cardinality_ambiguity_is_linear_and_bounded()
{
    constexpr std::size_t candidate_count = 20000;
    auto request = group_request("wn.geometer.research.group.high-cardinality");
    auto source = member("member:face:high-cardinality");
    source.candidates.reserve(candidate_count);
    for (std::size_t index = 0; index < candidate_count; ++index)
    {
        source.candidates.push_back(
            candidate("face:symmetric:" + std::to_string(index), source.source_fingerprint));
    }
    request.members.push_back(std::move(source));
    geometer::StepTopologyLimits limits;
    limits.max_handles = candidate_count;
    geometer::StepTopologyRecoveryGroupResult result;
    geometer::Status status;
    require(geometer::analyze_step_topology_recovery(request, limits, &result, &status) == 0 &&
                result.members.size() == 1 &&
                result.members[0].resolution_state ==
                    geometer::StepTopologyRecoveryResolutionState::ambiguous &&
                result.members[0].evidence.candidate_count == candidate_count &&
                result.members[0].evidence.matching_candidate_count == candidate_count &&
                result.members[0].evidence.rejected_alternatives.empty(),
            "high-cardinality symmetric ambiguity must remain a bounded linear pass");
}

} // namespace

int main()
{
    try
    {
        durable_identity_is_independent_from_topology_change();
        body_geometry_recovery_uses_volume_policy();
        validated_carrier_precedes_geometry_and_reports_relocation();
        symmetric_geometry_fails_closed();
        group_completeness_cannot_hide_partial_loss();
        split_merge_and_unsupported_are_explicit();
        malformed_and_over_limit_requests_fail_without_output();
        high_cardinality_ambiguity_is_linear_and_bounded();
        std::cout << "STEP topology multidimensional recovery tests passed\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << '\n';
        return 1;
    }
}

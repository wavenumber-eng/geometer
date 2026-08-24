#pragma once

#include "step_topology_session.h"

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace geometer
{

enum class StepTopologyRecoveryResolutionState
{
    resolved,
    ambiguous,
    unresolved,
    unsupported,
};

enum class StepTopologyRecoveryResolutionMethod
{
    authored_id_topology_link,
    validated_carrier_locator,
    unique_geometry_adjacency_fingerprint,
    none,
};

enum class StepTopologyRecoveryTopologyComparison
{
    unchanged,
    relocated,
    split,
    merged,
    otherwise_changed,
    not_compared,
    unavailable,
};

enum class StepTopologyRecoveryConfidence
{
    high,
    medium,
    low,
    none,
};

enum class StepTopologyRecoveryLineage
{
    none,
    split_from_source,
    merged_from_sources,
};

enum class StepTopologyRecoveryGroupCompleteness
{
    fully_recovered,
    partially_recovered,
    unrecovered,
    unsupported,
};

struct StepTopologyRecoveryTolerances
{
    double length_mm = 1.0e-6;
    double area_mm2 = 1.0e-6;
    double volume_mm3 = 1.0e-6;
};

struct StepTopologyRecoveryFingerprint
{
    bool available = false;
    std::string normalized_length_unit = "millimeter";
    std::string coordinate_frame;
    std::string occurrence_context;
    std::string geometry_kind;
    double area_mm2 = 0.0;
    double volume_mm3 = 0.0;
    std::array<double, 3> centroid_mm{};
    std::array<double, 6> bounds_mm{};
    std::string adjacency_sha256;
};

struct StepTopologyRecoveryCandidate
{
    std::string target_handle;
    StepTopologyTargetKind kind = StepTopologyTargetKind::face;
    std::string authored_target_id;
    bool topology_link_verified = false;
    std::string carrier_locator;
    bool carrier_locator_validated = false;
    std::string carrier_record;
    StepTopologyRecoveryLineage lineage = StepTopologyRecoveryLineage::none;
    StepTopologyRecoveryFingerprint fingerprint;
};

struct StepTopologyRecoveryMemberRequest
{
    std::string member_record_id;
    StepTopologyTargetKind kind = StepTopologyTargetKind::face;
    std::string authored_target_id;
    std::string carrier_locator;
    StepTopologyRecoveryFingerprint source_fingerprint;
    std::vector<StepTopologyRecoveryCandidate> candidates;
};

struct StepTopologyRecoveryProvenance
{
    std::string source_artifact_sha256;
    std::string candidate_artifact_sha256;
    std::string source_occt_version;
    std::string candidate_occt_version;
    std::string source_driver;
    std::string candidate_driver;
    std::string source_writer_settings;
    std::string candidate_writer_settings;
    std::string command_provenance;
    double measured_wall_time_milliseconds = 0.0;
};

struct StepTopologyRecoveryGroupRequest
{
    std::string group_authored_id;
    StepTopologyRecoveryProvenance provenance;
    StepTopologyRecoveryTolerances tolerances;
    std::vector<StepTopologyRecoveryMemberRequest> members;
};

struct StepTopologyRecoveryRejectedAlternative
{
    std::string target_handle;
    std::string reason;
};

struct StepTopologyRecoveryEvidence
{
    std::size_t candidate_count = 0;
    std::size_t matching_candidate_count = 0;
    std::vector<std::string> compared_fields;
    StepTopologyRecoveryTolerances tolerances;
    std::vector<std::string> carrier_records;
    std::vector<StepTopologyRecoveryRejectedAlternative> rejected_alternatives;
};

struct StepTopologyRecoveryMemberResult
{
    std::string member_record_id;
    StepTopologyTargetKind kind = StepTopologyTargetKind::face;
    std::string authored_target_id;
    StepTopologyRecoveryResolutionState resolution_state =
        StepTopologyRecoveryResolutionState::unresolved;
    StepTopologyRecoveryResolutionMethod resolution_method =
        StepTopologyRecoveryResolutionMethod::none;
    StepTopologyRecoveryTopologyComparison topology_comparison =
        StepTopologyRecoveryTopologyComparison::not_compared;
    StepTopologyRecoveryConfidence confidence = StepTopologyRecoveryConfidence::none;
    std::string resolved_target_handle;
    StepTopologyRecoveryEvidence evidence;
};

struct StepTopologyRecoveryGroupResult
{
    std::string research_format = "geometer.step_topology_recovery.research";
    std::string group_authored_id;
    StepTopologyRecoveryProvenance provenance;
    StepTopologyRecoveryResolutionState resolution_state =
        StepTopologyRecoveryResolutionState::unresolved;
    StepTopologyRecoveryGroupCompleteness completeness =
        StepTopologyRecoveryGroupCompleteness::unrecovered;
    std::size_t resolved_member_count = 0;
    std::size_t ambiguous_member_count = 0;
    std::size_t unresolved_member_count = 0;
    std::size_t unsupported_member_count = 0;
    std::vector<StepTopologyRecoveryMemberResult> members;
};

int analyze_step_topology_recovery(const StepTopologyRecoveryGroupRequest& request,
                                   const StepTopologyLimits& limits,
                                   StepTopologyRecoveryGroupResult* result,
                                   Status* status = nullptr);

} // namespace geometer

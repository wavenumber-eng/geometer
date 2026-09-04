#include "geometer/analytic_curve_broad_phase.h"
#include "geometer/analytic_filtered_arrangement.h"
#include "geometer/analytic_filtered_batch.h"
#include "geometer/analytic_filtered_boolean_selection.h"
#include "geometer/analytic_filtered_lowering.h"
#include "geometer/analytic_filtered_normalization.h"
#include "geometer/analytic_filtered_outcomes.h"
#include "geometer/analytic_filtered_packet.h"
#include "geometer/analytic_filtered_regions.h"
#include "geometer/analytic_request_packet.h"
#include "geometer/analytic_solver_limits.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace
{

constexpr const char* kSchema = "wn.geometer.analytic_solver_telemetry.a0";
constexpr const char* kImplementation =
    "decode_analytic_request_packet+build_analytic_filtered_batch";

bool read_bytes(const char* path, std::vector<std::uint8_t>& bytes)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    bytes.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    return input.good() || input.eof();
}

bool write_bytes(const char* path, const std::vector<std::uint8_t>& bytes)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output)
        return false;
    output.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
    return output.good();
}

void identity()
{
    std::cout << "{\"implementation\":\"" << kImplementation << "\",\"request_magic\":"
              << "\"GMABRQ01\",\"schema\":\"" << kSchema << "\"}\n";
}

void error(const char* kind, std::uint32_t code)
{
    std::cout << "{\"error\":{\"code\":" << code << ",\"kind\":\"" << kind << "\"},\"schema\":\""
              << kSchema << "\",\"status\":\"error\"}\n";
}

bool consume_budget(std::uint64_t& remaining, std::uint64_t amount)
{
    if (amount > remaining)
        return false;
    remaining -= amount;
    return true;
}

void phase_diagnostics(const geometer::AnalyticRequestPacketRecords& request)
{
    if (request.jobs.size() != 1)
    {
        error("job-count", static_cast<std::uint32_t>(request.jobs.size()));
        return;
    }
    const auto lowered = geometer::lower_analytic_job_to_filtered_curves(request, 0);
    if (lowered.error != geometer::AnalyticFilteredLoweringError::none || !lowered.value)
    {
        error("lowering", static_cast<std::uint32_t>(lowered.error));
        return;
    }
    const auto broad = geometer::build_analytic_curve_candidates(lowered.value->bounds);
    if (broad.error != geometer::AnalyticBroadPhaseError::none)
    {
        error("broad-phase", static_cast<std::uint32_t>(broad.error));
        return;
    }
    geometer::AnalyticSolverLimits packet_limits;
    if (!consume_budget(packet_limits.predicate_calls, lowered.telemetry.work_units) ||
        !consume_budget(packet_limits.predicate_calls, broad.telemetry.work_units) ||
        !consume_budget(packet_limits.working_memory_bytes,
                        lowered.telemetry.retained_geometry_bytes) ||
        !consume_budget(packet_limits.working_memory_bytes, broad.telemetry.retained_pair_bytes))
    {
        error("phase-budget", 1);
        return;
    }
    const auto packet = geometer::build_analytic_filtered_job_records(request, 0, *lowered.value,
                                                                      broad.pairs, packet_limits);
    geometer::AnalyticSolverLimits normalization_limits = packet_limits;
    if (!consume_budget(normalization_limits.predicate_calls,
                        packet.telemetry.reserved_packet_work_units) ||
        !consume_budget(normalization_limits.predicate_calls, 2) ||
        !consume_budget(normalization_limits.working_memory_bytes,
                        packet.telemetry.reserved_packet_memory_bytes))
    {
        error("phase-budget", 2);
        return;
    }
    const auto normalization = geometer::build_analytic_filtered_normalization(
        request, 0, *lowered.value, broad.pairs, normalization_limits);
    geometer::AnalyticSolverLimits outcomes_limits = normalization_limits;
    if (!consume_budget(outcomes_limits.predicate_calls,
                        normalization.telemetry.reserved_normalization_work_units) ||
        !consume_budget(outcomes_limits.working_memory_bytes,
                        normalization.telemetry.reserved_normalization_memory_bytes))
    {
        error("phase-budget", 3);
        return;
    }
    const auto outcomes = geometer::build_analytic_filtered_outcomes(request, 0, *lowered.value,
                                                                     broad.pairs, outcomes_limits);
    const auto regions = geometer::build_analytic_filtered_regions(request, 0, *lowered.value,
                                                                   broad.pairs, outcomes_limits);
    const auto arrangement =
        geometer::build_analytic_filtered_arrangement(*lowered.value, broad.pairs, outcomes_limits);
    const auto selection = geometer::build_analytic_filtered_boolean_selection(
        request, 0, *lowered.value, broad.pairs, outcomes_limits);
    const auto& value = packet.telemetry;
    std::cout << "{\"broad_phase_work_units\":" << broad.telemetry.work_units
              << ",\"candidate_pairs\":" << broad.telemetry.candidate_pairs
              << ",\"origin_x_nm\":" << lowered.value->origin_x_nm
              << ",\"origin_y_nm\":" << lowered.value->origin_y_nm
              << ",\"error\":" << static_cast<std::uint32_t>(packet.error)
              << ",\"lowering_work_units\":" << lowered.telemetry.work_units
              << ",\"capsule_coalescences\":" << value.capsule_coalescences
              << ",\"maximum_capsule_adjustment_nm\":" << value.maximum_capsule_adjustment_nm
              << ",\"normalization_error\":"
              << static_cast<std::uint32_t>(packet.normalization_error)
              << ",\"normalization_peak_working_memory_bytes\":"
              << value.normalization_peak_working_memory_bytes
              << ",\"normalization_work_units\":" << value.normalization_work_units
              << ",\"outcomes_work_units\":" << normalization.telemetry.outcomes_work_units
              << ",\"outcomes_error\":" << static_cast<std::uint32_t>(outcomes.error)
              << ",\"lineage_work_units\":" << outcomes.telemetry.lineage_work_units
              << ",\"regions_work_units\":" << outcomes.lineage.telemetry.regions_work_units
              << ",\"regions_error\":" << static_cast<std::uint32_t>(regions.error)
              << ",\"selection_error\":" << static_cast<std::uint32_t>(regions.selection.error)
              << ",\"arrangement_error\":"
              << static_cast<std::uint32_t>(regions.selection.arrangement.error)
              << ",\"direct_arrangement_error\":" << static_cast<std::uint32_t>(arrangement.error)
              << ",\"direct_arrangement_work_units\":" << arrangement.telemetry.predicate_calls
              << ",\"direct_selection_error\":" << static_cast<std::uint32_t>(selection.error)
              << ",\"direct_selection_faces\":" << selection.faces.size()
              << ",\"selection_work_units\":" << regions.telemetry.selection_predicate_calls
              << ",\"selection_unresolved_predicates\":"
              << regions.selection.telemetry.unresolved_predicates
              << ",\"event_columns\":" << regions.selection.telemetry.event_columns
              << ",\"resolution_event_columns\":"
              << regions.selection.telemetry.resolution_event_columns
              << ",\"sweep_status_node_visits\":"
              << regions.selection.telemetry.sweep_status_node_visits
              << ",\"sweep_status_update_work_units\":"
              << regions.selection.telemetry.sweep_status_update_work_units
              << ",\"selection_unresolved_predicate_failure\":"
              << (regions.selection.telemetry.unresolved_predicate_failure ? "true" : "false")
              << ",\"selection_required_working_memory_bytes\":"
              << regions.selection.telemetry.required_working_memory_bytes
              << ",\"region_phase_work_units\":" << regions.telemetry.region_work_units
              << ",\"reserved_region_work_units\":" << regions.telemetry.reserved_region_work_units
              << ",\"boundary_half_edges\":" << regions.telemetry.boundary_half_edges
              << ",\"region_rotation_visits\":" << regions.telemetry.vertex_rotation_visits
              << ",\"regions_required_working_memory_bytes\":"
              << regions.telemetry.required_working_memory_bytes
              << ",\"lineage_phase_work_units\":" << outcomes.lineage.telemetry.lineage_work_units
              << ",\"arrangement_work_units\":" << outcomes.telemetry.arrangement_work_units
              << ",\"outcomes_phase_work_units\":" << outcomes.telemetry.outcome_work_units
              << ",\"lineage_required_working_memory_bytes\":"
              << outcomes.lineage.telemetry.required_working_memory_bytes
              << ",\"normalization_phase_work_units\":"
              << normalization.telemetry.normalization_work_units
              << ",\"reserved_normalization_work_units\":"
              << normalization.telemetry.reserved_normalization_work_units
              << ",\"reserved_normalization_memory_bytes\":"
              << normalization.telemetry.reserved_normalization_memory_bytes
              << ",\"arc_critical_candidates\":" << normalization.telemetry.arc_critical_candidates
              << ",\"strict_replay_candidate_pairs\":"
              << normalization.telemetry.strict_replay_candidate_pairs
              << ",\"normalization_required_working_memory_bytes\":"
              << normalization.telemetry.required_working_memory_bytes
              << ",\"packet_work_units\":" << value.packet_work_units
              << ",\"peak_working_memory_bytes\":" << value.peak_working_memory_bytes
              << ",\"required_working_memory_bytes\":" << value.required_working_memory_bytes
              << ",\"reserved_packet_memory_bytes\":" << value.reserved_packet_memory_bytes
              << ",\"reserved_packet_work_units\":" << value.reserved_packet_work_units
              << ",\"schema\":\"" << kSchema << "\",\"status\":\"phase-diagnostics\"}\n";
}

std::uint64_t job_work(const geometer::AnalyticFilteredBatchJobTelemetry& job)
{
    return job.lowering_work_units + job.broad_phase_work_units + job.packet_work_units;
}

void telemetry(const geometer::AnalyticFilteredBatchResult& result)
{
    const auto& value = result.telemetry;
    const std::uint64_t work_units = value.lowering_work_units + value.broad_phase_work_units +
                                     value.packet_work_units + value.merge_work_units;
    std::cout << "{\"batch\":{\"candidate_pairs\":" << value.candidate_pairs
              << ",\"emitted_bytes\":" << value.emitted_packet_bytes
              << ",\"failures\":" << value.jobs_failed
              << ",\"fallback_count\":" << value.algebraic_fallback_calls
              << ",\"capsule_coalescences\":" << value.capsule_coalescences
              << ",\"maximum_capsule_adjustment_nm\":" << value.maximum_capsule_adjustment_nm
              << ",\"peak_working_memory_bytes\":" << value.peak_working_memory_bytes
              << ",\"work_units\":" << work_units << "},\"jobs\":[";
    for (std::size_t index = 0; index < result.jobs.size(); ++index)
    {
        const auto& job = result.jobs[index];
        if (index != 0)
            std::cout << ',';
        const bool failed = index >= result.packet->records.job_results.size() ||
                            result.packet->records.job_results[index].status != 0;
        std::cout << "{\"candidate_pairs\":" << job.candidate_pairs
                  << ",\"emitted_bytes\":" << job.emitted_record_bytes
                  << ",\"failures\":" << (failed ? 1 : 0)
                  << ",\"fallback_count\":" << job.algebraic_fallback_calls
                  << ",\"capsule_coalescences\":" << job.capsule_coalescences
                  << ",\"maximum_capsule_adjustment_nm\":" << job.maximum_capsule_adjustment_nm
                  << ",\"job_id\":" << job.job_id
                  << ",\"peak_working_memory_bytes\":" << job.peak_working_memory_bytes
                  << ",\"work_units\":" << job_work(job) << '}';
    }
    std::cout << "],\"schema\":\"" << kSchema << "\",\"status\":\"ok\"}\n";
}

void request_inventory(const geometer::AnalyticRequestPacketRecords& records)
{
    std::cout << "{\"stages\":[";
    bool first_stage = true;
    for (const auto& job : records.jobs)
        for (std::uint32_t stage_offset = 0; stage_offset < job.stage_count; ++stage_offset)
        {
            const auto& stage = records.stages[job.stage_begin + stage_offset];
            if (!first_stage)
                std::cout << ',';
            first_stage = false;
            std::cout << "{\"job_id\":" << job.job_id
                      << ",\"operand_count\":" << stage.operand_count
                      << ",\"operation\":" << static_cast<unsigned int>(stage.operation)
                      << ",\"stage_id\":" << stage.stage_id << '}';
        }
    std::cout << "],\"capsules\":[";
    bool first = true;
    for (const auto& job : records.jobs)
        for (std::uint32_t stage_offset = 0; stage_offset < job.stage_count; ++stage_offset)
        {
            const auto& stage = records.stages[job.stage_begin + stage_offset];
            for (std::uint32_t operand_offset = 0; operand_offset < stage.operand_count;
                 ++operand_offset)
            {
                const auto& operand = records.operands[stage.operand_begin + operand_offset];
                if (operand.geometry_kind != 4)
                    continue;
                const auto& capsule = records.capsules[operand.geometry_index];
                if (!first)
                    std::cout << ',';
                first = false;
                std::cout << "{\"end_x_nm\":" << capsule.end_x_nm
                          << ",\"end_y_nm\":" << capsule.end_y_nm
                          << ",\"feature_id\":" << capsule.feature_id
                          << ",\"job_id\":" << job.job_id
                          << ",\"operand_id\":" << operand.operand_id
                          << ",\"operation\":" << static_cast<unsigned int>(stage.operation)
                          << ",\"stage_id\":" << stage.stage_id
                          << ",\"start_x_nm\":" << capsule.start_x_nm
                          << ",\"start_y_nm\":" << capsule.start_y_nm
                          << ",\"width_nm\":" << capsule.width_nm << '}';
            }
        }
    std::cout << "],\"schema\":\"wn.geometer.analytic_request_inventory.a0\"}\n";
}

} // namespace

int main(int argc, char** argv)
{
    if (argc == 2 && std::string(argv[1]) == "--identity")
    {
        identity();
        return 0;
    }
    if (argc == 3 && (std::string(argv[1]) == "--phase-diagnostics" ||
                      std::string(argv[1]) == "--request-inventory"))
    {
        std::vector<std::uint8_t> request;
        if (!read_bytes(argv[2], request))
        {
            error("io", 1);
            return 2;
        }
        const auto decoded =
            geometer::decode_analytic_request_packet(request.data(), request.size());
        if (decoded.error != geometer::AnalyticRequestPacketError::none || !decoded.value)
        {
            error("request", static_cast<std::uint32_t>(decoded.error));
            return 3;
        }
        if (std::string(argv[1]) == "--request-inventory")
            request_inventory(*decoded.value);
        else
            phase_diagnostics(*decoded.value);
        return 0;
    }
    if (argc != 3)
    {
        std::cerr << "usage: geometer_analytic_solver_telemetry_helper <request.bin> "
                     "<result.bin>\n"
                     "   or: geometer_analytic_solver_telemetry_helper --phase-diagnostics "
                     "<request.bin>\n"
                     "   or: geometer_analytic_solver_telemetry_helper --request-inventory "
                     "<request.bin>\n";
        return 64;
    }

    std::vector<std::uint8_t> request;
    if (!read_bytes(argv[1], request))
    {
        error("io", 1);
        return 2;
    }
    const auto decoded = geometer::decode_analytic_request_packet(request.data(), request.size());
    if (decoded.error != geometer::AnalyticRequestPacketError::none || !decoded.value)
    {
        error("request", static_cast<std::uint32_t>(decoded.error));
        return 3;
    }
    const auto result = geometer::build_analytic_filtered_batch(*decoded.value);
    if (result.error != geometer::AnalyticFilteredBatchError::none || !result.packet)
    {
        error("solver", static_cast<std::uint32_t>(result.error));
        return 4;
    }
    if (!write_bytes(argv[2], result.packet->bytes))
    {
        error("io", 2);
        return 5;
    }
    telemetry(result);
    return 0;
}

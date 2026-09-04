#pragma once

#include "geometer/analytic_filtered_packet.h"
#include "geometer/analytic_request_packet.h"
#include "geometer/analytic_result_packet_records.h"
#include "geometer/analytic_solver_limits.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace geometer
{

enum class AnalyticFilteredBatchError : std::uint8_t
{
    none = 0,
    invalid_argument = 1,
    relationships_not_implemented = 2,
    resource_limit_exceeded = 3,
    internal_error = 4,
    encoding_failed = 5,
    solver_failed = 6,
};

struct AnalyticFilteredBatchLimits
{
    AnalyticSolverLimits per_job{};
    std::uint64_t assembly_work_units = 100'000'000;
    std::uint64_t working_memory_bytes = 1'073'741'824;
};

struct AnalyticFilteredBatchJobTelemetry
{
    std::uint64_t job_id = 0;
    std::uint32_t diagnostic_code = 0;
    std::uint32_t algebraic_fallback_calls = 0;
    std::uint64_t lowering_work_units = 0;
    std::uint64_t broad_phase_work_units = 0;
    std::uint64_t packet_work_units = 0;
    std::uint64_t candidate_pairs = 0;
    std::uint64_t peak_working_memory_bytes = 0;
    // Logical encoded footprint of the isolated job records before the
    // canonical batch merge and shared-table deduplication.
    std::uint64_t emitted_record_bytes = 0;
    std::uint64_t capsule_coalescences = 0;
    std::uint64_t maximum_capsule_adjustment_nm = 0;
};

struct AnalyticFilteredBatchTelemetry
{
    std::uint64_t jobs_visited = 0;
    std::uint64_t jobs_succeeded = 0;
    std::uint64_t jobs_failed = 0;
    std::uint64_t lowering_work_units = 0;
    std::uint64_t broad_phase_work_units = 0;
    std::uint64_t packet_work_units = 0;
    std::uint64_t broad_examined_pairs = 0;
    std::uint64_t candidate_pairs = 0;
    std::uint64_t merge_work_units = 0;
    std::uint64_t source_memberships = 0;
    std::uint64_t sequence_table_probes = 0;
    std::uint64_t retained_job_records_bytes = 0;
    std::uint64_t emitted_packet_bytes = 0;
    std::uint64_t peak_working_memory_bytes = 0;
    std::uint64_t algebraic_fallback_calls = 0;
    std::uint64_t capsule_coalescences = 0;
    std::uint64_t maximum_capsule_adjustment_nm = 0;
};

struct AnalyticFilteredBatchPacket
{
    AnalyticResultPacketRecords records;
    std::vector<std::uint8_t> bytes;
};

struct AnalyticFilteredBatchResult
{
    AnalyticFilteredBatchError error = AnalyticFilteredBatchError::none;
    std::optional<AnalyticFilteredBatchPacket> packet;
    std::vector<AnalyticFilteredBatchJobTelemetry> jobs;
    AnalyticFilteredBatchTelemetry telemetry;
};

// Owned batch implementation used by the packed generic operation adapter.
[[nodiscard]] AnalyticFilteredBatchResult
build_analytic_filtered_batch(const AnalyticRequestPacketRecords& records,
                              const AnalyticFilteredBatchLimits& limits = {});

} // namespace geometer

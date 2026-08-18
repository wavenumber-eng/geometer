#pragma once

#include "geometer/analytic_request_packet.h"
#include "geometer/analytic_result_packet_records.h"
#include "geometer/analytic_solver_limits.h"

#include <cstdint>
#include <vector>

namespace geometer::analytic_relationship_detail
{

enum class EvaluationError : std::uint8_t
{
    none = 0,
    invalid_argument = 1,
    resource_limit_exceeded = 2,
    solver_failed = 3,
};

struct EvaluationTelemetry
{
    std::uint64_t work_units = 0;
    std::uint64_t candidate_pairs = 0;
    std::uint64_t peak_working_memory_bytes = 0;
    std::uint64_t required_working_memory_bytes = 0;
    std::uint64_t algebraic_fallback_calls = 0;
    std::uint64_t unresolved_predicate_failures = 0;
};

struct EvaluationResult
{
    EvaluationError error = EvaluationError::none;
    std::vector<AnalyticRelationshipResultRecord> results;
    std::vector<AnalyticRelationshipPairRecord> pairs;
    EvaluationTelemetry telemetry;
};

[[nodiscard]] EvaluationResult
evaluate(const AnalyticRequestPacketRecords& request, const AnalyticResultPacketRecords& published,
         const AnalyticSolverLimits& per_pair_limits, std::uint64_t work_limit,
         std::uint64_t working_memory_limit, std::uint64_t retained_memory_bytes,
         std::uint64_t maximum_packet_bytes);

} // namespace geometer::analytic_relationship_detail

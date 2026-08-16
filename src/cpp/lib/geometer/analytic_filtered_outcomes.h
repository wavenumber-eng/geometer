#pragma once

#include "geometer/analytic_filtered_lineage.h"
#include "geometer/analytic_operand_outcome.h"

#include <cstdint>
#include <vector>

namespace geometer
{

enum class AnalyticFilteredResultReferenceKind : std::uint8_t
{
    ring = 1,
    region = 2,
};

// A pre-normalization topology handle. One-time 1 nm publication remaps this
// local index through its canonical old->published table before encoding the
// governed packet reference. Current ring/region indices are deliberately not
// frozen as final result IDs.
struct AnalyticFilteredTaggedResultReference
{
    AnalyticFilteredResultReferenceKind kind = AnalyticFilteredResultReferenceKind::ring;
    std::uint32_t local_index = 0;
};

struct AnalyticFilteredOperandOutcomeEvent
{
    std::uint64_t operand_id = 0;
    AnalyticOperandOutcomeKind kind = AnalyticOperandOutcomeKind::no_effect;
    AnalyticFilteredSourceRange result_references;
    AnalyticFilteredSourceRange sources;
};

enum class AnalyticFilteredOutcomesError : std::uint8_t
{
    none = 0,
    invalid_argument = 1,
    resource_limit_exceeded = 2,
};

struct AnalyticFilteredOutcomesTelemetry
{
    std::uint64_t lineage_work_units = 0;
    std::uint64_t lineage_peak_working_memory_bytes = 0;
    std::uint64_t arrangement_work_units = 0;
    std::uint64_t operand_source_visits = 0;
    std::uint64_t lineage_source_visits = 0;
    std::uint64_t emitted_events = 0;
    std::uint64_t emitted_result_references = 0;
    std::uint64_t emitted_source_references = 0;
    std::uint64_t sort_work_units = 0;
    std::uint64_t reserved_outcomes_work_units = 0;
    std::uint64_t outcome_work_units = 0;
    std::uint64_t predicate_calls = 0;
    std::uint64_t peak_working_memory_bytes = 0;
    std::uint64_t algebraic_fallback_calls = 0;
};

struct AnalyticFilteredOutcomesResult
{
    AnalyticFilteredOutcomesError error = AnalyticFilteredOutcomesError::none;
    AnalyticFilteredLineageResult lineage;
    std::vector<AnalyticFilteredOperandOutcomeEvent> events;
    std::vector<AnalyticFilteredTaggedResultReference> result_references;
    // Complete sorted/unique original occurrence sources grouped once per
    // operand. Every event for that operand shares the same range.
    std::vector<AnalyticFilteredSourceReference> source_references;
    AnalyticFilteredOutcomesTelemetry telemetry;
};

// Owned production stage. It collects sparse history during face selection,
// owns regions and lineage, then projects governed outcomes without accepting
// caller-constructible topology or replaying every operand for every face.
[[nodiscard]] AnalyticFilteredOutcomesResult
build_analytic_filtered_outcomes(const AnalyticRequestPacketRecords& records,
                                 std::uint32_t job_index, const AnalyticFilteredGeometry& geometry,
                                 const std::vector<AnalyticCurvePair>& candidate_pairs,
                                 const AnalyticSolverLimits& limits = {});

} // namespace geometer

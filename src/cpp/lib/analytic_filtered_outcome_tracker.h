#pragma once

#include "analytic_filtered_boolean_selection_support.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace geometer::analytic_selection_detail
{

inline constexpr std::uint64_t kOutcomeEvidenceLogicalBytes = 16;

[[nodiscard]] std::uint64_t
outcome_tracker_logical_bytes(std::uint64_t operands, std::uint64_t stages, bool& valid) noexcept;
[[nodiscard]] std::uint64_t outcome_tracker_work_upper_bound(std::uint64_t transitions,
                                                             std::uint64_t faces,
                                                             std::uint64_t operands,
                                                             std::uint64_t stages,
                                                             bool& valid) noexcept;

// Tracks the six monotone operand-history facts while SelectionBuilder walks
// the face dual. Each reporter contains only operands that are both active and
// have not yet produced that fact. Consequently a range drain is proportional
// to newly emitted evidence, never to all operands in a face.
class OutcomeHistoryTracker
{
  public:
    OutcomeHistoryTracker(const std::vector<OperandMetadata>& operands,
                          const std::vector<std::uint8_t>& stage_operations,
                          std::vector<AnalyticFilteredOperandOutcomeEvidence>& evidence,
                          AnalyticFilteredBooleanSelectionTelemetry& telemetry,
                          std::uint64_t& total_work, std::uint64_t work_limit);
    ~OutcomeHistoryTracker();
    OutcomeHistoryTracker(const OutcomeHistoryTracker&) = delete;
    OutcomeHistoryTracker& operator=(const OutcomeHistoryTracker&) = delete;

    [[nodiscard]] bool initialize();
    void begin_batch();
    [[nodiscard]] bool record_toggle(std::uint32_t operand);
    [[nodiscard]] bool finish_batch();
    [[nodiscard]] bool evaluate(const AnalyticFilteredSelectedFace& face);
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] bool resource_exhausted() const noexcept;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace geometer::analytic_selection_detail

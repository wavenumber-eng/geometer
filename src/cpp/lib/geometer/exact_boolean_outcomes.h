#pragma once

#include "geometer/exact_boolean_provenance.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace geometer::exact
{

enum class ExactOperandOutcomeKind : std::uint16_t
{
    contributes_final_material = 1,
    redundant_or_absorbed_coverage = 2,
    partially_removed_later = 3,
    completely_removed_later = 4,
    subtraction_effect_survives = 5,
    subtraction_effect_overwritten_later = 6,
    no_effect = 7,
};

struct ExactOperandOutcomeEvent
{
    std::uint64_t operand_id = 0;
    ExactOperandOutcomeKind kind = ExactOperandOutcomeKind::no_effect;
    std::uint32_t ring_reference_begin = 0;
    std::uint32_t ring_reference_count = 0;
    std::uint32_t region_reference_begin = 0;
    std::uint32_t region_reference_count = 0;
    std::uint32_t source_begin = 0;
    std::uint32_t source_count = 0;
};

struct ExactBooleanOutcomesResult;

class ExactBooleanOutcomes
{
  public:
    ~ExactBooleanOutcomes();
    ExactBooleanOutcomes(ExactBooleanOutcomes&& other) noexcept;
    ExactBooleanOutcomes& operator=(ExactBooleanOutcomes&& other) noexcept;
    ExactBooleanOutcomes(const ExactBooleanOutcomes&) = delete;
    ExactBooleanOutcomes& operator=(const ExactBooleanOutcomes&) = delete;

    [[nodiscard]] const std::vector<ExactOperandOutcomeEvent>& events() const;
    [[nodiscard]] const std::vector<std::uint32_t>& ring_references() const;
    [[nodiscard]] const std::vector<std::uint32_t>& region_references() const;
    [[nodiscard]] const std::vector<ExactSourceReference>& source_references() const;

  private:
    friend struct ExactBooleanOutcomesResult;
    friend ExactBooleanOutcomesResult
    build_exact_boolean_outcomes(Budget&, const ExactArrangement&, const ExactBooleanSelection&,
                                 const ExactBooleanRegions&, const ExactBooleanProvenance&,
                                 const std::vector<ExactBooleanStage>&,
                                 const std::vector<ExactOccurrenceSource>&);
    ExactBooleanOutcomes(Budget& budget, std::uint64_t charged_bytes,
                         std::vector<ExactOperandOutcomeEvent> events,
                         std::vector<std::uint32_t> ring_references,
                         std::vector<std::uint32_t> region_references,
                         std::vector<ExactSourceReference> source_references) noexcept;
    void release();

    Budget* budget_ = nullptr;
    std::uint64_t charged_bytes_ = 0;
    std::vector<ExactOperandOutcomeEvent> events_;
    std::vector<std::uint32_t> ring_references_;
    std::vector<std::uint32_t> region_references_;
    std::vector<ExactSourceReference> source_references_;
};

struct ExactBooleanOutcomesResult
{
    Error error = Error::none;
    std::optional<ExactBooleanOutcomes> value;
};

[[nodiscard]] ExactBooleanOutcomesResult build_exact_boolean_outcomes(
    Budget& budget, const ExactArrangement& arrangement, const ExactBooleanSelection& selection,
    const ExactBooleanRegions& regions, const ExactBooleanProvenance& provenance,
    const std::vector<ExactBooleanStage>& stages,
    const std::vector<ExactOccurrenceSource>& occurrence_sources);

} // namespace geometer::exact

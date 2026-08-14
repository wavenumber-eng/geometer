#pragma once

#include "geometer/exact_arrangement.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace geometer::exact
{

enum class ExactBooleanStageOperation : std::uint8_t
{
    union_ = 1,
    difference = 2,
};

struct ExactBooleanOperand
{
    std::uint64_t coverage_id = 0;
    std::uint64_t source_id = 0;
};

struct ExactBooleanStage
{
    std::uint64_t stage_id = 0;
    ExactBooleanStageOperation operation = ExactBooleanStageOperation::union_;
    std::vector<ExactBooleanOperand> operands;
};

struct ExactSelectedFace
{
    bool material = false;
    std::uint32_t positive_source_begin = 0;
    std::uint32_t positive_source_count = 0;
    std::uint32_t subtraction_source_begin = 0;
    std::uint32_t subtraction_source_count = 0;
};

struct ExactBooleanSelectionResult;

class ExactBooleanSelection
{
  public:
    ~ExactBooleanSelection();
    ExactBooleanSelection(ExactBooleanSelection&& other) noexcept;
    ExactBooleanSelection& operator=(ExactBooleanSelection&& other) noexcept;
    ExactBooleanSelection(const ExactBooleanSelection&) = delete;
    ExactBooleanSelection& operator=(const ExactBooleanSelection&) = delete;

    [[nodiscard]] const std::vector<ExactSelectedFace>& faces() const;
    [[nodiscard]] const std::vector<std::uint64_t>& positive_sources() const;
    [[nodiscard]] const std::vector<std::uint64_t>& subtraction_sources() const;

  private:
    friend struct ExactBooleanSelectionResult;
    friend ExactBooleanSelectionResult
    evaluate_exact_boolean_stages(Budget&, const ExactArrangement&,
                                  const std::vector<ExactBooleanStage>&);
    ExactBooleanSelection(Budget& budget, std::uint64_t charged_bytes,
                          std::vector<ExactSelectedFace> faces,
                          std::vector<std::uint64_t> positive_sources,
                          std::vector<std::uint64_t> subtraction_sources) noexcept;
    void release();

    Budget* budget_ = nullptr;
    std::uint64_t charged_bytes_ = 0;
    std::vector<ExactSelectedFace> faces_;
    std::vector<std::uint64_t> positive_sources_;
    std::vector<std::uint64_t> subtraction_sources_;
};

struct ExactBooleanSelectionResult
{
    Error error = Error::none;
    std::optional<ExactBooleanSelection> value;
};

[[nodiscard]] ExactBooleanSelectionResult
evaluate_exact_boolean_stages(Budget& budget, const ExactArrangement& arrangement,
                              const std::vector<ExactBooleanStage>& stages);

} // namespace geometer::exact

#include "geometer/exact_boolean_stages.h"

#include <algorithm>
#include <exception>
#include <limits>
#include <stdexcept>
#include <utility>

namespace geometer::exact
{
namespace
{

constexpr std::uint64_t kMaximumStages = 131'072;
constexpr std::uint64_t kMaximumOperands = 8'388'608;
constexpr std::uint64_t kMaximumLineageMemberships = 8'388'608;

std::uint64_t checked_add(std::uint64_t left, std::uint64_t right)
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
        throw std::overflow_error("stage size addition overflow");
    return left + right;
}

std::uint64_t checked_multiply(std::uint64_t left, std::uint64_t right)
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left)
        throw std::overflow_error("stage size multiplication overflow");
    return left * right;
}

class StorageReservation
{
  public:
    StorageReservation(Budget& budget, std::uint64_t bytes) : budget_(&budget), bytes_(bytes)
    {
        acquired_ = budget.acquire_storage(bytes);
    }
    ~StorageReservation()
    {
        if (acquired_ && budget_ != nullptr)
            budget_->release_storage(bytes_);
    }
    [[nodiscard]] bool acquired() const
    {
        return acquired_;
    }
    [[nodiscard]] std::uint64_t transfer()
    {
        budget_ = nullptr;
        return bytes_;
    }

  private:
    Budget* budget_ = nullptr;
    std::uint64_t bytes_ = 0;
    bool acquired_ = false;
};

ExactBooleanSelectionResult failure(Error error)
{
    return {error, std::nullopt};
}

void insert_unique(std::vector<std::uint64_t>& values, std::uint64_t value)
{
    const auto position = std::lower_bound(values.begin(), values.end(), value);
    if (position == values.end() || *position != value)
        values.insert(position, value);
}

bool face_contains_coverage(const ExactArrangement& arrangement, std::uint32_t face,
                            std::uint64_t coverage_id)
{
    const ExactArrangementFace& value = arrangement.faces()[face];
    const auto begin = arrangement.face_coverages().begin() + value.coverage_begin;
    return std::binary_search(begin, begin + value.coverage_count, coverage_id);
}

Error normalize_stages(const std::vector<ExactBooleanStage>& input,
                       std::vector<ExactBooleanStage>& stages,
                       std::vector<std::uint64_t>& coverage_ids, std::uint64_t& operand_count)
{
    if (input.size() > kMaximumStages)
        return Error::resource_limit_exceeded;
    stages = input;
    std::vector<std::uint64_t> stage_ids;
    std::vector<std::uint64_t> source_ids;
    stage_ids.reserve(stages.size());
    for (ExactBooleanStage& stage : stages)
    {
        if (stage.stage_id == 0 || (stage.operation != ExactBooleanStageOperation::union_ &&
                                    stage.operation != ExactBooleanStageOperation::difference))
            return Error::invalid_argument;
        operand_count = checked_add(operand_count, stage.operands.size());
        if (operand_count > kMaximumOperands)
            return Error::resource_limit_exceeded;
        std::sort(stage.operands.begin(), stage.operands.end(),
                  [](const ExactBooleanOperand& left, const ExactBooleanOperand& right)
                  {
                      return left.coverage_id != right.coverage_id
                                 ? left.coverage_id < right.coverage_id
                                 : left.source_id < right.source_id;
                  });
        for (std::size_t index = 0; index < stage.operands.size(); ++index)
        {
            const ExactBooleanOperand& operand = stage.operands[index];
            if (operand.coverage_id == 0 || operand.source_id == 0 ||
                (index != 0 && stage.operands[index - 1].coverage_id == operand.coverage_id))
                return Error::invalid_argument;
            coverage_ids.push_back(operand.coverage_id);
            source_ids.push_back(operand.source_id);
        }
        stage_ids.push_back(stage.stage_id);
    }
    for (auto* values : {&stage_ids, &coverage_ids, &source_ids})
    {
        std::sort(values->begin(), values->end());
        if (std::adjacent_find(values->begin(), values->end()) != values->end())
            return Error::invalid_argument;
    }
    return Error::none;
}

} // namespace

ExactBooleanSelection::ExactBooleanSelection(
    Budget& budget, std::uint64_t charged_bytes, std::vector<ExactSelectedFace> faces,
    std::vector<std::uint64_t> positive_sources,
    std::vector<std::uint64_t> subtraction_sources) noexcept
    : budget_(&budget), charged_bytes_(charged_bytes), faces_(std::move(faces)),
      positive_sources_(std::move(positive_sources)),
      subtraction_sources_(std::move(subtraction_sources))
{
}

ExactBooleanSelection::~ExactBooleanSelection()
{
    release();
}

ExactBooleanSelection::ExactBooleanSelection(ExactBooleanSelection&& other) noexcept
    : budget_(std::exchange(other.budget_, nullptr)),
      charged_bytes_(std::exchange(other.charged_bytes_, 0)), faces_(std::move(other.faces_)),
      positive_sources_(std::move(other.positive_sources_)),
      subtraction_sources_(std::move(other.subtraction_sources_))
{
}

ExactBooleanSelection& ExactBooleanSelection::operator=(ExactBooleanSelection&& other) noexcept
{
    if (this != &other)
    {
        release();
        budget_ = std::exchange(other.budget_, nullptr);
        charged_bytes_ = std::exchange(other.charged_bytes_, 0);
        faces_ = std::move(other.faces_);
        positive_sources_ = std::move(other.positive_sources_);
        subtraction_sources_ = std::move(other.subtraction_sources_);
    }
    return *this;
}

void ExactBooleanSelection::release()
{
    if (budget_ != nullptr)
        budget_->release_storage(charged_bytes_);
    budget_ = nullptr;
    charged_bytes_ = 0;
}

const std::vector<ExactSelectedFace>& ExactBooleanSelection::faces() const
{
    return faces_;
}
const std::vector<std::uint64_t>& ExactBooleanSelection::positive_sources() const
{
    return positive_sources_;
}
const std::vector<std::uint64_t>& ExactBooleanSelection::subtraction_sources() const
{
    return subtraction_sources_;
}

ExactBooleanSelectionResult
evaluate_exact_boolean_stages(Budget& budget, const ExactArrangement& arrangement,
                              const std::vector<ExactBooleanStage>& input_stages)
{
    try
    {
        if (input_stages.size() > kMaximumStages)
            return failure(Error::resource_limit_exceeded);
        std::uint64_t operand_count = 0;
        for (const ExactBooleanStage& stage : input_stages)
            operand_count = checked_add(operand_count, stage.operands.size());
        if (operand_count > kMaximumOperands)
            return failure(Error::resource_limit_exceeded);

        const std::uint64_t face_count = arrangement.faces().size();
        const std::uint64_t maximum_lineage =
            std::min(kMaximumLineageMemberships,
                     checked_multiply(checked_multiply(face_count, operand_count), 2));
        const std::uint64_t charge =
            checked_add(4096, checked_add(checked_multiply(face_count, sizeof(ExactSelectedFace)),
                                          checked_add(checked_multiply(operand_count, 32),
                                                      checked_multiply(maximum_lineage, 8))));
        const std::uint64_t work = checked_add(
            256, checked_add(checked_multiply(input_stages.size(), 32),
                             checked_multiply(checked_multiply(face_count, operand_count), 4)));
        if (!budget.consume_work(work))
            return failure(Error::resource_limit_exceeded);
        StorageReservation reservation(budget, charge);
        if (!reservation.acquired())
            return failure(Error::resource_limit_exceeded);

        std::vector<ExactBooleanStage> stages;
        std::vector<std::uint64_t> coverage_ids;
        std::uint64_t normalized_operand_count = 0;
        if (const Error error =
                normalize_stages(input_stages, stages, coverage_ids, normalized_operand_count);
            error != Error::none)
            return failure(error);
        if (normalized_operand_count != operand_count)
            return failure(Error::invalid_argument);
        for (const std::uint64_t coverage_id : arrangement.face_coverages())
            if (!std::binary_search(coverage_ids.begin(), coverage_ids.end(), coverage_id))
                return failure(Error::invalid_argument);

        std::vector<ExactSelectedFace> selected_faces;
        std::vector<std::uint64_t> positive_sources;
        std::vector<std::uint64_t> subtraction_sources;
        selected_faces.reserve(static_cast<std::size_t>(face_count));
        for (std::uint32_t face = 0; face < face_count; ++face)
        {
            bool material = false;
            std::vector<std::uint64_t> positive;
            std::vector<std::uint64_t> subtraction;
            for (const ExactBooleanStage& stage : stages)
            {
                std::vector<std::uint64_t> covered_sources;
                for (const ExactBooleanOperand& operand : stage.operands)
                    if (face_contains_coverage(arrangement, face, operand.coverage_id))
                        covered_sources.push_back(operand.source_id);
                if (covered_sources.empty())
                    continue;
                if (stage.operation == ExactBooleanStageOperation::union_)
                {
                    material = true;
                    for (const std::uint64_t source : covered_sources)
                        insert_unique(positive, source);
                }
                else if (material)
                {
                    for (const std::uint64_t source : covered_sources)
                        insert_unique(subtraction, source);
                    material = false;
                    positive.clear();
                }
            }
            const std::uint64_t new_total =
                checked_add(checked_add(positive_sources.size(), subtraction_sources.size()),
                            checked_add(positive.size(), subtraction.size()));
            if (new_total > kMaximumLineageMemberships ||
                positive_sources.size() > std::numeric_limits<std::uint32_t>::max() ||
                subtraction_sources.size() > std::numeric_limits<std::uint32_t>::max())
                return failure(Error::resource_limit_exceeded);
            selected_faces.push_back({material, static_cast<std::uint32_t>(positive_sources.size()),
                                      static_cast<std::uint32_t>(positive.size()),
                                      static_cast<std::uint32_t>(subtraction_sources.size()),
                                      static_cast<std::uint32_t>(subtraction.size())});
            positive_sources.insert(positive_sources.end(), positive.begin(), positive.end());
            subtraction_sources.insert(subtraction_sources.end(), subtraction.begin(),
                                       subtraction.end());
        }
        const std::uint64_t transferred = reservation.transfer();
        return {Error::none,
                ExactBooleanSelection(budget, transferred, std::move(selected_faces),
                                      std::move(positive_sources), std::move(subtraction_sources))};
    }
    catch (const std::exception&)
    {
        return failure(Error::resource_limit_exceeded);
    }
}

} // namespace geometer::exact

#include "geometer/exact_source_sets.h"

#include <algorithm>
#include <exception>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace geometer::exact
{
namespace
{

constexpr std::uint64_t kMaximumSets = 8'388'608;
constexpr std::uint64_t kMaximumMemberships = 8'388'608;
constexpr std::uint64_t kSetStorageBytes = 128;
constexpr std::uint64_t kMembershipStorageBytes = 192;
constexpr std::uint64_t kSetWorkUnits = 128;
constexpr std::uint64_t kMembershipWorkUnits = 128;

std::uint64_t checked_add(std::uint64_t left, std::uint64_t right)
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
        throw std::overflow_error("source-set size addition overflow");
    return left + right;
}

std::uint64_t checked_multiply(std::uint64_t left, std::uint64_t right)
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left)
        throw std::overflow_error("source-set size multiplication overflow");
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

ExactSourceSetTableResult failure(Error error)
{
    return {error, std::nullopt};
}

auto source_key(const ExactSourceReference& source)
{
    return std::tuple{static_cast<std::uint16_t>(source.kind),
                      static_cast<std::uint16_t>(source.role), source.operand_id, source.primary_id,
                      source.secondary_id};
}

bool source_less(const ExactSourceReference& left, const ExactSourceReference& right)
{
    return source_key(left) < source_key(right);
}

bool source_equal(const ExactSourceReference& left, const ExactSourceReference& right)
{
    return source_key(left) == source_key(right);
}

bool set_less(const std::vector<ExactSourceReference>& left,
              const std::vector<ExactSourceReference>& right)
{
    return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end(),
                                        source_less);
}

bool set_equal(const std::vector<ExactSourceReference>& left,
               const std::vector<ExactSourceReference>& right)
{
    return left.size() == right.size() &&
           std::equal(left.begin(), left.end(), right.begin(), source_equal);
}

bool valid_source(const ExactSourceReference& source)
{
    if (source.operand_id == 0 || source.primary_id == 0)
        return false;
    switch (source.kind)
    {
    case ExactSourceKind::authored_segment_curve:
        return source.secondary_id != 0 && (source.role == ExactSourceRole::authored_line ||
                                            source.role == ExactSourceRole::authored_circular_arc);
    case ExactSourceKind::compact_feature_role:
    {
        const std::uint32_t high = static_cast<std::uint32_t>(source.secondary_id >> 32U);
        const std::uint32_t low = static_cast<std::uint32_t>(source.secondary_id);
        switch (source.role)
        {
        case ExactSourceRole::primitive_outer_circle:
        case ExactSourceRole::primitive_inner_circle:
        case ExactSourceRole::capsule_left_line:
        case ExactSourceRole::capsule_end_cap:
        case ExactSourceRole::capsule_right_line:
        case ExactSourceRole::capsule_start_cap:
            return source.secondary_id == 0;
        case ExactSourceRole::swept_left_offset_line:
        case ExactSourceRole::swept_left_offset_arc:
        case ExactSourceRole::swept_right_offset_line:
        case ExactSourceRole::swept_right_offset_arc:
        case ExactSourceRole::swept_end_cap:
            return high != 0 && low == 0;
        case ExactSourceRole::swept_round_join:
            return high != 0 && low != 0;
        case ExactSourceRole::swept_start_cap:
            return high == 1 && low == 0;
        default:
            return false;
        }
    }
    case ExactSourceKind::subtractive_operand_effect:
        return source.role == ExactSourceRole::none && source.secondary_id == 0;
    }
    return false;
}

} // namespace

ExactSourceSetTable::ExactSourceSetTable(Budget& budget, std::uint64_t charged_bytes,
                                         std::vector<ExactSourceReference> source_references,
                                         std::vector<ExactSourceSetRecord> source_sets,
                                         std::vector<std::uint32_t> source_reference_indices,
                                         std::vector<std::uint32_t> input_handles) noexcept
    : budget_(&budget), charged_bytes_(charged_bytes),
      source_references_(std::move(source_references)), source_sets_(std::move(source_sets)),
      source_reference_indices_(std::move(source_reference_indices)),
      input_handles_(std::move(input_handles))
{
}

ExactSourceSetTable::~ExactSourceSetTable()
{
    release();
}

ExactSourceSetTable::ExactSourceSetTable(ExactSourceSetTable&& other) noexcept
    : budget_(std::exchange(other.budget_, nullptr)),
      charged_bytes_(std::exchange(other.charged_bytes_, 0)),
      source_references_(std::move(other.source_references_)),
      source_sets_(std::move(other.source_sets_)),
      source_reference_indices_(std::move(other.source_reference_indices_)),
      input_handles_(std::move(other.input_handles_))
{
}

ExactSourceSetTable& ExactSourceSetTable::operator=(ExactSourceSetTable&& other) noexcept
{
    if (this != &other)
    {
        release();
        budget_ = std::exchange(other.budget_, nullptr);
        charged_bytes_ = std::exchange(other.charged_bytes_, 0);
        source_references_ = std::move(other.source_references_);
        source_sets_ = std::move(other.source_sets_);
        source_reference_indices_ = std::move(other.source_reference_indices_);
        input_handles_ = std::move(other.input_handles_);
    }
    return *this;
}

void ExactSourceSetTable::release()
{
    if (budget_ != nullptr)
        budget_->release_storage(charged_bytes_);
    budget_ = nullptr;
    charged_bytes_ = 0;
}

const std::vector<ExactSourceReference>& ExactSourceSetTable::source_references() const
{
    return source_references_;
}

const std::vector<ExactSourceSetRecord>& ExactSourceSetTable::source_sets() const
{
    return source_sets_;
}

const std::vector<std::uint32_t>& ExactSourceSetTable::source_reference_indices() const
{
    return source_reference_indices_;
}

const std::vector<std::uint32_t>& ExactSourceSetTable::input_handles() const
{
    return input_handles_;
}

ExactSourceSetTableResult
build_exact_source_sets(Budget& budget,
                        const std::vector<std::vector<ExactSourceReference>>& input_sets)
{
    try
    {
        if (input_sets.size() > kMaximumSets)
            return failure(Error::resource_limit_exceeded);
        std::uint64_t membership_count = 0;
        for (const auto& set : input_sets)
        {
            membership_count = checked_add(membership_count, set.size());
            if (membership_count > kMaximumMemberships)
                return failure(Error::resource_limit_exceeded);
            if (!std::all_of(set.begin(), set.end(), valid_source))
                return failure(Error::invalid_argument);
        }
        const std::uint64_t work = checked_add(
            4096, checked_add(checked_multiply(input_sets.size(), kSetWorkUnits),
                              checked_multiply(membership_count, kMembershipWorkUnits)));
        const std::uint64_t storage = checked_add(
            4096, checked_add(checked_multiply(input_sets.size(), kSetStorageBytes),
                              checked_multiply(membership_count, kMembershipStorageBytes)));
        if (!budget.consume_work(work))
            return failure(Error::resource_limit_exceeded);
        StorageReservation reservation(budget, storage);
        if (!reservation.acquired())
            return failure(Error::resource_limit_exceeded);

        std::vector<std::vector<ExactSourceReference>> normalized_sets = input_sets;
        std::vector<ExactSourceReference> source_references;
        source_references.reserve(static_cast<std::size_t>(membership_count));
        for (auto& set : normalized_sets)
        {
            std::sort(set.begin(), set.end(), source_less);
            set.erase(std::unique(set.begin(), set.end(), source_equal), set.end());
            source_references.insert(source_references.end(), set.begin(), set.end());
        }
        std::sort(source_references.begin(), source_references.end(), source_less);
        source_references.erase(
            std::unique(source_references.begin(), source_references.end(), source_equal),
            source_references.end());

        std::vector<std::vector<ExactSourceReference>> unique_sets;
        unique_sets.reserve(normalized_sets.size());
        for (const auto& set : normalized_sets)
            if (!set.empty())
                unique_sets.push_back(set);
        std::sort(unique_sets.begin(), unique_sets.end(), set_less);
        unique_sets.erase(std::unique(unique_sets.begin(), unique_sets.end(), set_equal),
                          unique_sets.end());

        std::vector<ExactSourceSetRecord> records;
        std::vector<std::uint32_t> indices;
        records.reserve(unique_sets.size());
        indices.reserve(static_cast<std::size_t>(membership_count));
        for (const auto& set : unique_sets)
        {
            const std::uint32_t begin = static_cast<std::uint32_t>(indices.size());
            for (const ExactSourceReference& source : set)
            {
                const auto found = std::lower_bound(source_references.begin(),
                                                    source_references.end(), source, source_less);
                if (found == source_references.end() || !source_equal(*found, source))
                    return failure(Error::invalid_argument);
                indices.push_back(static_cast<std::uint32_t>(found - source_references.begin()));
            }
            records.push_back({begin, static_cast<std::uint32_t>(set.size())});
        }

        std::vector<std::uint32_t> handles;
        handles.reserve(normalized_sets.size());
        for (const auto& set : normalized_sets)
        {
            if (set.empty())
            {
                handles.push_back(0);
                continue;
            }
            const auto found =
                std::lower_bound(unique_sets.begin(), unique_sets.end(), set, set_less);
            if (found == unique_sets.end() || !set_equal(*found, set))
                return failure(Error::invalid_argument);
            handles.push_back(static_cast<std::uint32_t>(found - unique_sets.begin()) + 1U);
        }
        return {Error::none,
                ExactSourceSetTable(budget, reservation.transfer(), std::move(source_references),
                                    std::move(records), std::move(indices), std::move(handles))};
    }
    catch (const std::exception&)
    {
        return failure(Error::resource_limit_exceeded);
    }
}

} // namespace geometer::exact

#include "analytic_filtered_outcome_tracker.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <tuple>
#include <utility>
#include <vector>

namespace geometer::analytic_selection_detail
{
namespace
{
constexpr std::uint32_t kNone = std::numeric_limits<std::uint32_t>::max();
constexpr std::uint64_t kTrackerObjectLogicalBytes = 4096;

std::uint64_t leaf_capacity(std::uint64_t count, bool& valid) noexcept
{
    std::uint64_t capacity = 1;
    while (capacity < std::max<std::uint64_t>(1, count))
    {
        if (capacity > std::numeric_limits<std::uint64_t>::max() / 2)
        {
            valid = false;
            return 0;
        }
        capacity *= 2;
    }
    return capacity;
}
} // namespace

std::uint64_t outcome_tracker_logical_bytes(std::uint64_t operands, std::uint64_t stages,
                                            bool& valid) noexcept
{
    const std::uint64_t leaves = leaf_capacity(stages, valid);
    std::uint64_t bytes = kTrackerObjectLogicalBytes;
    // Evidence is retained in the selection result. Active state plus three
    // fixed-capacity operand/stage scratch lists are owned by the tracker.
    bytes = checked_add(
        bytes, checked_multiply(operands, kOutcomeEvidenceLogicalBytes + 1 + 4, valid), valid);
    bytes = checked_add(bytes, checked_multiply(stages, 9 + 4 * 5, valid), valid);
    // Stage state: five target-independent int32 trees and one uint32 count
    // vector. Six reporters each own prev/next/linked operand slots, four
    // stage vectors, and one uint32 sum tree.
    bytes = checked_add(bytes, checked_multiply(leaves * 2, 20, valid), valid);
    const std::uint64_t reporter_operands = checked_multiply(operands, 9, valid);
    const std::uint64_t reporter_stages = checked_multiply(stages, 13, valid);
    const std::uint64_t reporter_tree = checked_multiply(leaves * 2, 4, valid);
    const std::uint64_t reporter =
        checked_add(checked_add(reporter_operands, reporter_stages, valid), reporter_tree, valid);
    bytes = checked_add(bytes, checked_multiply(reporter, 6, valid), valid);
    return bytes;
}

std::uint64_t outcome_tracker_work_upper_bound(std::uint64_t transitions, std::uint64_t faces,
                                               std::uint64_t operands, std::uint64_t stages,
                                               bool& valid) noexcept
{
    const std::uint64_t leaves = leaf_capacity(stages, valid);
    const std::uint64_t tree = tree_operation_units(std::max<std::uint64_t>(1, stages));
    // Every tree edge is applied and reverted. A changed operand can touch
    // three reporters, one stage leaf, old/new successor searches, and three
    // qualification leaves. This is deliberately conservative but remains
    // O(T log S), not an operand/face product.
    std::uint64_t work = checked_multiply(checked_multiply(transitions, 2, valid),
                                          checked_add(tree * 12, 24, valid), valid);
    work =
        checked_add(work, checked_multiply(faces, checked_add(tree * 8, 12, valid), valid), valid);
    work = checked_add(work, checked_multiply(operands, checked_add(tree * 6, 12, valid), valid),
                       valid);
    std::uint64_t initialization = checked_add(
        operands,
        checked_add(checked_multiply(stages, 8, valid), checked_multiply(leaves, 8, valid), valid),
        valid);
    initialization = checked_add(
        initialization, checked_multiply(checked_multiply(stages, 2, valid), tree, valid), valid);
    work = checked_add(work, initialization, valid);
    return work;
}

struct OutcomeHistoryTracker::Impl
{
    enum class Fact : std::uint8_t
    {
        covered,
        redundant,
        removed,
        attributed,
        unfilled,
        overwritten,
    };

    struct StageState
    {
        StageState(const std::vector<std::uint8_t>& operations, Impl& owner)
            : operations(operations), owner(owner), counts(operations.size())
        {
            capacity = 1;
            while (capacity < std::max<std::size_t>(1, operations.size()))
            {
                capacity <<= 1U;
                ++depth;
            }
            first_active.assign(capacity * 2, -1);
            last_active.assign(capacity * 2, -1);
            first_difference.assign(capacity * 2, -1);
            last_difference.assign(capacity * 2, -1);
            last_union.assign(capacity * 2, -1);
        }

        bool set_count(std::uint32_t stage, std::uint32_t count)
        {
            if (stage >= counts.size() || !owner.consume(depth + 1))
                return false;
            owner.telemetry.outcome_stage_state_update_work_units += depth + 1;
            counts[stage] = count;
            std::size_t node = capacity + stage;
            const bool active = count != 0;
            first_active[node] = active ? static_cast<std::int32_t>(stage) : -1;
            last_active[node] = first_active[node];
            first_difference[node] =
                active && operations[stage] == 2 ? static_cast<std::int32_t>(stage) : -1;
            last_difference[node] = first_difference[node];
            last_union[node] =
                active && operations[stage] == 1 ? static_cast<std::int32_t>(stage) : -1;
            while (node > 1)
            {
                node /= 2;
                first_active[node] = first_active[node * 2] >= 0 ? first_active[node * 2]
                                                                 : first_active[node * 2 + 1];
                last_active[node] = std::max(last_active[node * 2], last_active[node * 2 + 1]);
                first_difference[node] = first_difference[node * 2] >= 0
                                             ? first_difference[node * 2]
                                             : first_difference[node * 2 + 1];
                last_difference[node] =
                    std::max(last_difference[node * 2], last_difference[node * 2 + 1]);
                last_union[node] = std::max(last_union[node * 2], last_union[node * 2 + 1]);
            }
            return true;
        }

        std::int32_t predecessor(std::uint32_t stage)
        {
            return find_last(1, 0, static_cast<std::uint32_t>(capacity), 0, stage);
        }

        std::int32_t successor(std::uint32_t stage)
        {
            return find_first(1, 0, static_cast<std::uint32_t>(capacity), stage + 1,
                              static_cast<std::uint32_t>(operations.size()), first_active);
        }

        std::int32_t first_difference_after(std::uint32_t minimum)
        {
            return find_first(1, 0, static_cast<std::uint32_t>(capacity), minimum,
                              static_cast<std::uint32_t>(operations.size()), first_difference);
        }

        std::int32_t last_active_stage() const noexcept
        {
            return last_active[1];
        }

        std::int32_t last_union_stage() const noexcept
        {
            return last_union[1];
        }

        std::int32_t last_difference_stage() const noexcept
        {
            return last_difference[1];
        }

        bool material() const noexcept
        {
            const std::int32_t last = last_active_stage();
            return last >= 0 && operations[static_cast<std::size_t>(last)] == 1;
        }

        std::uint32_t positive_stage_begin() const noexcept
        {
            const std::int32_t difference = last_difference_stage();
            return difference < 0 ? 0 : static_cast<std::uint32_t>(difference) + 1;
        }

        std::uint32_t active_removal_stage()
        {
            if (material() || last_union_stage() < 0)
                return kNone;
            const std::int32_t value =
                first_difference_after(static_cast<std::uint32_t>(last_union_stage()) + 1);
            return value < 0 ? kNone : static_cast<std::uint32_t>(value);
        }

        std::int32_t find_last(std::size_t node, std::uint32_t begin, std::uint32_t width,
                               std::uint32_t query_begin, std::uint32_t query_end)
        {
            if (!owner.consume(1))
                return -2;
            ++owner.telemetry.outcome_reporter_node_visits;
            if (query_end <= begin || begin + width <= query_begin || last_active[node] < 0)
                return -1;
            if (width == 1)
                return static_cast<std::int32_t>(begin);
            const std::uint32_t half = width / 2;
            const std::int32_t right =
                find_last(node * 2 + 1, begin + half, half, query_begin, query_end);
            return right >= 0 || right == -2
                       ? right
                       : find_last(node * 2, begin, half, query_begin, query_end);
        }

        std::int32_t find_first(std::size_t node, std::uint32_t begin, std::uint32_t width,
                                std::uint32_t query_begin, std::uint32_t query_end,
                                const std::vector<std::int32_t>& values)
        {
            if (!owner.consume(1))
                return -2;
            ++owner.telemetry.outcome_reporter_node_visits;
            if (query_end <= begin || begin + width <= query_begin || values[node] < 0)
                return -1;
            if (width == 1)
                return static_cast<std::int32_t>(begin);
            const std::uint32_t half = width / 2;
            const std::int32_t left =
                find_first(node * 2, begin, half, query_begin, query_end, values);
            return left >= 0 || left == -2 ? left
                                           : find_first(node * 2 + 1, begin + half, half,
                                                        query_begin, query_end, values);
        }

        const std::vector<std::uint8_t>& operations;
        Impl& owner;
        std::vector<std::uint32_t> counts;
        std::vector<std::int32_t> first_active;
        std::vector<std::int32_t> last_active;
        std::vector<std::int32_t> first_difference;
        std::vector<std::int32_t> last_difference;
        std::vector<std::int32_t> last_union;
        std::size_t capacity = 1;
        std::uint32_t depth = 0;
    };

    struct Reporter
    {
        Reporter(Fact fact, std::size_t operand_count, std::size_t stage_count,
                 std::size_t stage_capacity, Impl& owner)
            : fact(fact), owner(owner), previous(operand_count, kNone), next(operand_count, kNone),
              linked(operand_count), head(stage_count, kNone), tail(stage_count, kNone),
              count(stage_count), eligible(stage_count), tree(stage_capacity * 2)
        {
            capacity = stage_capacity;
            depth = 0;
            for (std::size_t value = capacity; value > 1; value >>= 1U)
                ++depth;
        }

        bool set_eligible(std::uint32_t stage, bool value)
        {
            if (stage >= eligible.size())
                return false;
            if ((eligible[stage] != 0) == value)
                return true;
            eligible[stage] = value ? 1 : 0;
            return update_leaf(stage);
        }

        bool set_active(std::uint32_t operand, std::uint32_t stage, bool value)
        {
            if (operand >= linked.size() || stage >= head.size())
                return false;
            if (owner.fact_set(fact, operand))
                return linked[operand] == 0;
            if (value)
            {
                if (linked[operand])
                    return false;
                linked[operand] = 1;
                previous[operand] = tail[stage];
                next[operand] = kNone;
                if (tail[stage] != kNone)
                    next[tail[stage]] = operand;
                else
                    head[stage] = operand;
                tail[stage] = operand;
                ++count[stage];
            }
            else
            {
                if (!linked[operand])
                    return false;
                const std::uint32_t before = previous[operand];
                const std::uint32_t after = next[operand];
                if (before == kNone)
                    head[stage] = after;
                else
                    next[before] = after;
                if (after == kNone)
                    tail[stage] = before;
                else
                    previous[after] = before;
                previous[operand] = kNone;
                next[operand] = kNone;
                linked[operand] = 0;
                if (count[stage] == 0)
                    return false;
                --count[stage];
            }
            return !eligible[stage] || update_leaf(stage);
        }

        bool drain(std::uint32_t begin, std::uint32_t end)
        {
            end = std::min<std::uint32_t>(end, static_cast<std::uint32_t>(head.size()));
            while (begin < end)
            {
                const std::int32_t found =
                    find_first(1, 0, static_cast<std::uint32_t>(capacity), begin, end);
                if (found == -2)
                    return false;
                if (found < 0)
                    return true;
                const std::uint32_t stage = static_cast<std::uint32_t>(found);
                std::uint32_t operand = head[stage];
                std::uint32_t emitted = 0;
                while (operand != kNone)
                {
                    const std::uint32_t following = next[operand];
                    if (!owner.consume(1) || !owner.mark(fact, operand))
                        return false;
                    ++emitted;
                    linked[operand] = 0;
                    previous[operand] = kNone;
                    next[operand] = kNone;
                    operand = following;
                }
                if (emitted != count[stage])
                    return false;
                head[stage] = kNone;
                tail[stage] = kNone;
                count[stage] = 0;
                if (!update_leaf(stage))
                    return false;
            }
            return true;
        }

        bool empty() const noexcept
        {
            return tree[1] == 0;
        }

        bool update_leaf(std::uint32_t stage)
        {
            if (!owner.consume(depth + 1))
                return false;
            owner.telemetry.outcome_reporter_node_visits += depth + 1;
            std::size_t node = capacity + stage;
            tree[node] = eligible[stage] ? count[stage] : 0;
            while (node > 1)
            {
                node /= 2;
                tree[node] = tree[node * 2] + tree[node * 2 + 1];
            }
            return true;
        }

        std::int32_t find_first(std::size_t node, std::uint32_t node_begin, std::uint32_t width,
                                std::uint32_t query_begin, std::uint32_t query_end)
        {
            if (!owner.consume(1))
                return -2;
            ++owner.telemetry.outcome_reporter_node_visits;
            if (query_end <= node_begin || node_begin + width <= query_begin || tree[node] == 0)
                return -1;
            if (width == 1)
                return static_cast<std::int32_t>(node_begin);
            const std::uint32_t half = width / 2;
            const std::int32_t left =
                find_first(node * 2, node_begin, half, query_begin, query_end);
            return left >= 0 || left == -2
                       ? left
                       : find_first(node * 2 + 1, node_begin + half, half, query_begin, query_end);
        }

        Fact fact;
        Impl& owner;
        std::vector<std::uint32_t> previous;
        std::vector<std::uint32_t> next;
        std::vector<std::uint8_t> linked;
        std::vector<std::uint32_t> head;
        std::vector<std::uint32_t> tail;
        std::vector<std::uint32_t> count;
        std::vector<std::uint8_t> eligible;
        std::vector<std::uint32_t> tree;
        std::size_t capacity = 1;
        std::uint32_t depth = 0;
    };

    Impl(const std::vector<OperandMetadata>& operands, const std::vector<std::uint8_t>& operations,
         std::vector<AnalyticFilteredOperandOutcomeEvidence>& evidence,
         AnalyticFilteredBooleanSelectionTelemetry& telemetry, std::uint64_t& total_work,
         std::uint64_t work_limit)
        : operands(operands), operations(operations), evidence(evidence), telemetry(telemetry),
          total_work(total_work), work_limit(work_limit), stages(operations, *this),
          covered(Fact::covered, operands.size(), operations.size(), stages.capacity, *this),
          redundant(Fact::redundant, operands.size(), operations.size(), stages.capacity, *this),
          removed(Fact::removed, operands.size(), operations.size(), stages.capacity, *this),
          attributed(Fact::attributed, operands.size(), operations.size(), stages.capacity, *this),
          unfilled(Fact::unfilled, operands.size(), operations.size(), stages.capacity, *this),
          overwritten(Fact::overwritten, operands.size(), operations.size(), stages.capacity,
                      *this),
          active(operands.size()), stage_delta(operations.size()), stage_touched(operations.size())
    {
        batch.reserve(operands.size());
        changed_stages.reserve(operations.size());
        affected_stages.reserve(operations.size() * 3);
    }

    bool initialize()
    {
        if (!consume(operands.size() + operations.size() * 8 + stages.capacity * 8))
            return false;
        evidence.clear();
        evidence.reserve(operands.size());
        for (const OperandMetadata& operand : operands)
            evidence.push_back({operand.operand_id});
        for (std::uint32_t stage = 0; stage < operations.size(); ++stage)
        {
            if (operations[stage] == 1)
            {
                if (!covered.set_eligible(stage, true) || !removed.set_eligible(stage, true))
                    return false;
            }
            else if (operations[stage] == 2)
            {
                if (!unfilled.set_eligible(stage, true))
                    return false;
            }
            else
                return false;
        }
        return true;
    }

    bool consume(std::uint64_t units) noexcept
    {
        if (total_work > work_limit || units > work_limit - total_work)
        {
            resource_exhausted = true;
            return false;
        }
        total_work += units;
        return true;
    }

    bool fact_set(Fact fact, std::uint32_t operand) const noexcept
    {
        if (operand >= evidence.size())
            return false;
        const auto& value = evidence[operand];
        switch (fact)
        {
        case Fact::covered:
            return value.covered_positive_area;
        case Fact::redundant:
            return value.redundant_or_absorbed;
        case Fact::removed:
            return value.removed_later;
        case Fact::attributed:
            return value.attributed_removal;
        case Fact::unfilled:
            return value.unfilled_removal;
        case Fact::overwritten:
            return value.overwritten;
        }
        return false;
    }

    bool mark(Fact fact, std::uint32_t operand)
    {
        if (operand >= evidence.size() || fact_set(fact, operand))
            return false;
        auto& value = evidence[operand];
        switch (fact)
        {
        case Fact::covered:
            value.covered_positive_area = true;
            break;
        case Fact::redundant:
            value.redundant_or_absorbed = true;
            break;
        case Fact::removed:
            value.removed_later = true;
            break;
        case Fact::attributed:
            value.attributed_removal = true;
            break;
        case Fact::unfilled:
            value.unfilled_removal = true;
            break;
        case Fact::overwritten:
            value.overwritten = true;
            break;
        }
        ++telemetry.outcome_evidence_flags_set;
        return true;
    }

    bool refresh_stage(std::uint32_t stage)
    {
        if (stage >= operations.size())
            return false;
        const std::int32_t predecessor = stages.predecessor(stage);
        if (predecessor == -2)
            return false;
        const bool predecessor_union =
            predecessor >= 0 && operations[static_cast<std::size_t>(predecessor)] == 1;
        if (operations[stage] == 1)
            return redundant.set_eligible(stage, stages.counts[stage] > 1 || predecessor_union);
        const bool effective = stages.counts[stage] != 0 && predecessor_union;
        return attributed.set_eligible(stage, effective) &&
               overwritten.set_eligible(stage, effective);
    }

    void begin_batch()
    {
        batch.clear();
    }

    bool record_toggle(std::uint32_t operand)
    {
        if (operand >= operands.size())
            return false;
        batch.push_back(operand);
        return true;
    }

    bool finish_batch()
    {
        if (batch.empty())
            return true;
        const std::uint64_t sort_work = sort_units(batch.size());
        if (!consume(sort_work))
            return false;
        telemetry.sort_work_units += sort_work;
        std::sort(batch.begin(), batch.end());
        if (std::adjacent_find(batch.begin(), batch.end()) != batch.end())
            return false;
        changed_stages.clear();
        for (const std::uint32_t operand : batch)
        {
            const std::uint32_t stage = operands[operand].stage_ordinal;
            if (stage >= stage_touched.size())
                return false;
            if (stage_touched[stage] == 0)
            {
                stage_touched[stage] = 1;
                changed_stages.push_back(stage);
            }
        }
        const std::uint64_t stage_sort = sort_units(changed_stages.size());
        if (!consume(stage_sort))
            return false;
        telemetry.sort_work_units += stage_sort;
        std::sort(changed_stages.begin(), changed_stages.end());
        changed_stages.erase(std::unique(changed_stages.begin(), changed_stages.end()),
                             changed_stages.end());
        affected_stages = changed_stages;
        for (const std::uint32_t stage : changed_stages)
        {
            const std::int32_t successor = stages.successor(stage);
            if (successor == -2)
                return false;
            if (successor >= 0)
                affected_stages.push_back(static_cast<std::uint32_t>(successor));
        }
        for (const std::uint32_t operand : batch)
        {
            const auto& metadata = operands[operand];
            const bool activate = active[operand] == 0;
            active[operand] = activate ? 1 : 0;
            stage_delta[metadata.stage_ordinal] += activate ? 1 : -1;
            if (metadata.operation == 1)
            {
                if (!covered.set_active(operand, metadata.stage_ordinal, activate) ||
                    !removed.set_active(operand, metadata.stage_ordinal, activate) ||
                    !redundant.set_active(operand, metadata.stage_ordinal, activate))
                    return false;
            }
            else if (metadata.operation == 2)
            {
                if (!attributed.set_active(operand, metadata.stage_ordinal, activate) ||
                    !unfilled.set_active(operand, metadata.stage_ordinal, activate) ||
                    !overwritten.set_active(operand, metadata.stage_ordinal, activate))
                    return false;
            }
            else
                return false;
        }
        for (const std::uint32_t stage : changed_stages)
            stage_touched[stage] = 0;
        for (const std::uint32_t stage : changed_stages)
        {
            const std::int64_t updated =
                static_cast<std::int64_t>(stages.counts[stage]) + stage_delta[stage];
            stage_delta[stage] = 0;
            if (updated < 0 || updated > std::numeric_limits<std::uint32_t>::max() ||
                !stages.set_count(stage, static_cast<std::uint32_t>(updated)))
                return false;
        }
        for (const std::uint32_t stage : changed_stages)
        {
            const std::int32_t successor = stages.successor(stage);
            if (successor == -2)
                return false;
            if (successor >= 0)
                affected_stages.push_back(static_cast<std::uint32_t>(successor));
        }
        const std::uint64_t affected_sort = sort_units(affected_stages.size());
        if (!consume(affected_sort))
            return false;
        telemetry.sort_work_units += affected_sort;
        std::sort(affected_stages.begin(), affected_stages.end());
        affected_stages.erase(std::unique(affected_stages.begin(), affected_stages.end()),
                              affected_stages.end());
        for (const std::uint32_t stage : affected_stages)
            if (!refresh_stage(stage))
                return false;
        return true;
    }

    bool evaluate(const AnalyticFilteredSelectedFace& face)
    {
        const std::int32_t last_difference = stages.last_difference_stage();
        const std::int32_t last_union = stages.last_union_stage();
        const std::uint32_t active_removal = stages.active_removal_stage();
        if (face.material != stages.material() ||
            face.positive_stage_begin !=
                (last_difference < 0 ? 0 : static_cast<std::uint32_t>(last_difference) + 1) ||
            face.active_removal_stage != active_removal)
            return false;
        if (face.unbounded)
            return !face.material;
        const std::uint32_t stage_count = static_cast<std::uint32_t>(operations.size());
        if (!covered.drain(0, stage_count) || !redundant.drain(0, stage_count) ||
            (last_difference >= 0 &&
             !removed.drain(0, static_cast<std::uint32_t>(last_difference))) ||
            !attributed.drain(0, stage_count) ||
            (last_union >= 0 && !overwritten.drain(0, static_cast<std::uint32_t>(last_union))))
            return false;
        if (!face.material && active_removal != kNone &&
            !unfilled.drain(active_removal, active_removal + 1))
            return false;
        return true;
    }

    bool empty() const noexcept
    {
        return std::find(active.begin(), active.end(), 1) == active.end() && covered.empty() &&
               redundant.empty() && removed.empty() && attributed.empty() && unfilled.empty() &&
               overwritten.empty() && stages.last_active_stage() < 0;
    }

    const std::vector<OperandMetadata>& operands;
    const std::vector<std::uint8_t>& operations;
    std::vector<AnalyticFilteredOperandOutcomeEvidence>& evidence;
    AnalyticFilteredBooleanSelectionTelemetry& telemetry;
    std::uint64_t& total_work;
    std::uint64_t work_limit = 0;
    bool resource_exhausted = false;
    StageState stages;
    Reporter covered;
    Reporter redundant;
    Reporter removed;
    Reporter attributed;
    Reporter unfilled;
    Reporter overwritten;
    std::vector<std::uint8_t> active;
    std::vector<std::int64_t> stage_delta;
    std::vector<std::uint8_t> stage_touched;
    std::vector<std::uint32_t> batch;
    std::vector<std::uint32_t> changed_stages;
    std::vector<std::uint32_t> affected_stages;
};

OutcomeHistoryTracker::OutcomeHistoryTracker(
    const std::vector<OperandMetadata>& operands, const std::vector<std::uint8_t>& stage_operations,
    std::vector<AnalyticFilteredOperandOutcomeEvidence>& evidence,
    AnalyticFilteredBooleanSelectionTelemetry& telemetry, std::uint64_t& total_work,
    std::uint64_t work_limit)
    : impl_(std::make_unique<Impl>(operands, stage_operations, evidence, telemetry, total_work,
                                   work_limit))
{
}

OutcomeHistoryTracker::~OutcomeHistoryTracker()
{
    static_assert(sizeof(Impl) <= kTrackerObjectLogicalBytes);
}

bool OutcomeHistoryTracker::initialize()
{
    return impl_->initialize();
}

void OutcomeHistoryTracker::begin_batch()
{
    impl_->begin_batch();
}

bool OutcomeHistoryTracker::record_toggle(std::uint32_t operand)
{
    return impl_->record_toggle(operand);
}

bool OutcomeHistoryTracker::finish_batch()
{
    return impl_->finish_batch();
}

bool OutcomeHistoryTracker::evaluate(const AnalyticFilteredSelectedFace& face)
{
    return impl_->evaluate(face);
}

bool OutcomeHistoryTracker::empty() const noexcept
{
    return impl_->empty();
}

bool OutcomeHistoryTracker::resource_exhausted() const noexcept
{
    return impl_->resource_exhausted;
}

static_assert(sizeof(AnalyticFilteredOperandOutcomeEvidence) <= kOutcomeEvidenceLogicalBytes);
} // namespace geometer::analytic_selection_detail

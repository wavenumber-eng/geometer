#include "geometer/exact_boolean_outcomes.h"

#include <algorithm>
#include <array>
#include <exception>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace geometer::exact
{
namespace
{

constexpr std::uint64_t kMaximumOperands = 4'194'304;
constexpr std::uint64_t kMaximumEvents = 4'194'304;
constexpr std::uint64_t kMaximumReferences = 8'388'608;
constexpr std::uint64_t kMaximumStages = 131'072;
constexpr std::uint64_t kMaximumFaces = 4'194'304;
constexpr std::uint64_t kMaximumEdges = 4'194'304;
constexpr std::uint64_t kMaximumHalfEdges = 8'388'608;
constexpr std::uint64_t kMaximumMemberships = 8'388'608;

// Portable logical allocation bounds include vector growth (less than twice the final element
// count) and are deliberately based on the larger supported 64-bit vector/object layout.
constexpr std::uint64_t kStageStorageBytes = 16; // push-grown stage-id index
constexpr std::uint64_t kOperandStorageBytes =
    448; // two state indices, nested references, and three live face-id sets
constexpr std::uint64_t kOccurrenceStorageBytes =
    192; // sorted input copy, nested operand sources, and flattened output sources
constexpr std::uint64_t kMembershipStorageBytes = 16; // push-grown used-occurrence ids
constexpr std::uint64_t kFaceStorageBytes = 24;       // DSU, root map, and alignment
constexpr std::uint64_t kEdgeStorageBytes = 16;       // twin-pair map and alignment
constexpr std::uint64_t kHalfEdgeStorageBytes = 16;   // ring map and visited bits
constexpr std::uint64_t kRingStorageBytes = 16;       // ring-to-region map and alignment
constexpr std::uint64_t kRegionStorageBytes = 64;     // nested source-vector object and maps
constexpr std::uint64_t kRegionSourceStorageBytes =
    64; // push-grown association plus nested expected source
constexpr std::uint64_t kEventStorageBytes = 80; // push-grown 40-byte event
constexpr std::uint64_t kResultReferenceStorageBytes =
    16; // nested and flattened push-grown u32 references

std::uint64_t checked_add(std::uint64_t left, std::uint64_t right)
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
        throw std::overflow_error("outcome size addition overflow");
    return left + right;
}

std::uint64_t checked_multiply(std::uint64_t left, std::uint64_t right)
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left)
        throw std::overflow_error("outcome size multiplication overflow");
    return left * right;
}

std::uint64_t outcome_storage_charge(std::uint64_t stage_count, std::uint64_t operand_count,
                                     std::uint64_t occurrence_count, std::uint64_t membership_count,
                                     std::uint64_t face_count, std::uint64_t edge_count,
                                     std::uint64_t half_edge_count, std::uint64_t ring_count,
                                     std::uint64_t region_count, std::uint64_t region_source_count,
                                     std::uint64_t maximum_events,
                                     std::uint64_t maximum_result_references)
{
    // These portable logical sizes cover vector growth as well as elements. Operand state includes
    // both sorted indices, the three per-operand reference vectors, and the three live per-face id
    // sets. Occurrences cover the sorted metadata copy, per-operand sources, and output sources.
    std::uint64_t charge = 4096;
    const std::array<std::pair<std::uint64_t, std::uint64_t>, 12> terms{{
        {stage_count, kStageStorageBytes},
        {operand_count, kOperandStorageBytes},
        {occurrence_count, kOccurrenceStorageBytes},
        {membership_count, kMembershipStorageBytes},
        {face_count, kFaceStorageBytes},
        {edge_count, kEdgeStorageBytes},
        {half_edge_count, kHalfEdgeStorageBytes},
        {ring_count, kRingStorageBytes},
        {region_count, kRegionStorageBytes},
        {region_source_count, kRegionSourceStorageBytes},
        {maximum_events, kEventStorageBytes},
        {maximum_result_references, kResultReferenceStorageBytes},
    }};
    for (const auto& [count, logical_bytes] : terms)
        charge = checked_add(charge, checked_multiply(count, logical_bytes));
    return charge;
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

class DisjointSet
{
  public:
    explicit DisjointSet(std::size_t count) : parent_(count), rank_(count)
    {
        std::iota(parent_.begin(), parent_.end(), 0U);
    }
    std::uint32_t find(std::uint32_t value)
    {
        if (parent_[value] != value)
            parent_[value] = find(parent_[value]);
        return parent_[value];
    }
    void unite(std::uint32_t left, std::uint32_t right)
    {
        left = find(left);
        right = find(right);
        if (left == right)
            return;
        if (rank_[left] < rank_[right])
            std::swap(left, right);
        parent_[right] = left;
        if (rank_[left] == rank_[right])
            ++rank_[left];
    }

  private:
    std::vector<std::uint32_t> parent_;
    std::vector<std::uint8_t> rank_;
};

struct OperandState
{
    std::uint64_t coverage_id = 0;
    std::uint64_t operand_id = 0;
    std::uint64_t stage_id = 0;
    std::uint32_t stage_index = 0;
    ExactBooleanStageOperation operation = ExactBooleanStageOperation::union_;
    bool covered_area = false;
    bool redundant = false;
    bool removed_later = false;
    bool final_lineage = false;
    bool attributed_removal = false;
    bool unfilled_removal = false;
    bool overwritten = false;
    std::vector<std::uint32_t> rings;
    std::vector<std::uint32_t> regions;
    std::vector<ExactSourceReference> sources;
    std::uint32_t source_begin = 0;
    std::uint32_t source_count = 0;
};

ExactBooleanOutcomesResult failure(Error error)
{
    return {error, std::nullopt};
}

void insert_id(std::vector<std::uint64_t>& values, std::uint64_t value)
{
    const auto position = std::lower_bound(values.begin(), values.end(), value);
    if (position == values.end() || *position != value)
        values.insert(position, value);
}

void insert_index(std::vector<std::uint32_t>& values, std::uint32_t value)
{
    const auto position = std::lower_bound(values.begin(), values.end(), value);
    if (position == values.end() || *position != value)
        values.insert(position, value);
}

auto source_key(const ExactSourceReference& source)
{
    return std::tuple{static_cast<std::uint16_t>(source.kind),
                      static_cast<std::uint16_t>(source.role), source.operand_id, source.primary_id,
                      source.secondary_id};
}

void insert_source(std::vector<ExactSourceReference>& values, const ExactSourceReference& value)
{
    const auto position =
        std::lower_bound(values.begin(), values.end(), value,
                         [](const ExactSourceReference& left, const ExactSourceReference& right)
                         { return source_key(left) < source_key(right); });
    if (position == values.end() || source_key(*position) != source_key(value))
        values.insert(position, value);
}

OperandState* find_by_id(std::vector<OperandState>& states, std::uint64_t operand_id)
{
    const auto found = std::lower_bound(states.begin(), states.end(), operand_id,
                                        [](const OperandState& state, std::uint64_t value)
                                        { return state.operand_id < value; });
    return found != states.end() && found->operand_id == operand_id ? &*found : nullptr;
}

const OperandState* find_by_coverage(const std::vector<OperandState>& by_coverage,
                                     std::uint64_t coverage_id)
{
    const auto found = std::lower_bound(by_coverage.begin(), by_coverage.end(), coverage_id,
                                        [](const OperandState& state, std::uint64_t value)
                                        { return state.coverage_id < value; });
    return found != by_coverage.end() && found->coverage_id == coverage_id ? &*found : nullptr;
}

const ExactOccurrenceSource* find_occurrence(const std::vector<ExactOccurrenceSource>& occurrences,
                                             std::uint64_t occurrence_id)
{
    const auto found =
        std::lower_bound(occurrences.begin(), occurrences.end(), occurrence_id,
                         [](const ExactOccurrenceSource& occurrence, std::uint64_t value)
                         { return occurrence.occurrence_id < value; });
    return found != occurrences.end() && found->occurrence_id == occurrence_id ? &*found : nullptr;
}

bool role_matches_curve(ExactSourceRole role, ExactAtomicCurveKind kind)
{
    const bool line_role = role == ExactSourceRole::authored_line ||
                           role == ExactSourceRole::capsule_left_line ||
                           role == ExactSourceRole::capsule_right_line ||
                           role == ExactSourceRole::swept_left_offset_line ||
                           role == ExactSourceRole::swept_right_offset_line;
    const bool arc_role =
        role == ExactSourceRole::authored_circular_arc ||
        role == ExactSourceRole::primitive_outer_circle ||
        role == ExactSourceRole::primitive_inner_circle ||
        role == ExactSourceRole::capsule_end_cap || role == ExactSourceRole::capsule_start_cap ||
        role == ExactSourceRole::swept_left_offset_arc ||
        role == ExactSourceRole::swept_right_offset_arc ||
        role == ExactSourceRole::swept_round_join || role == ExactSourceRole::swept_start_cap ||
        role == ExactSourceRole::swept_end_cap;
    return kind == ExactAtomicCurveKind::line ? line_role : arc_role;
}

bool occurrence_source_valid(const ExactOccurrenceSource& occurrence)
{
    const ExactSourceReference& source = occurrence.source;
    if (occurrence.occurrence_id == 0 || occurrence.coverage_id == 0 || source.operand_id == 0 ||
        source.primary_id == 0)
        return false;
    if (source.kind == ExactSourceKind::authored_segment_curve)
        return (source.role == ExactSourceRole::authored_line ||
                source.role == ExactSourceRole::authored_circular_arc) &&
               source.secondary_id != 0;
    if (source.kind != ExactSourceKind::compact_feature_role)
        return false;
    const bool whole_feature = source.role == ExactSourceRole::primitive_outer_circle ||
                               source.role == ExactSourceRole::primitive_inner_circle ||
                               source.role == ExactSourceRole::capsule_left_line ||
                               source.role == ExactSourceRole::capsule_end_cap ||
                               source.role == ExactSourceRole::capsule_right_line ||
                               source.role == ExactSourceRole::capsule_start_cap;
    if (whole_feature)
        return source.secondary_id == 0;
    const std::uint64_t incoming = source.secondary_id >> 32U;
    const std::uint64_t outgoing = source.secondary_id & 0xFFFF'FFFFULL;
    if (source.role == ExactSourceRole::swept_round_join)
        return incoming != 0 && outgoing != 0;
    if (source.role == ExactSourceRole::swept_start_cap)
        return incoming == 1 && outgoing == 0;
    const bool offset_or_end = source.role == ExactSourceRole::swept_left_offset_line ||
                               source.role == ExactSourceRole::swept_left_offset_arc ||
                               source.role == ExactSourceRole::swept_right_offset_line ||
                               source.role == ExactSourceRole::swept_right_offset_arc ||
                               source.role == ExactSourceRole::swept_end_cap;
    return offset_or_end && incoming != 0 && outgoing == 0;
}

Error normalize_operands(const std::vector<ExactBooleanStage>& stages,
                         std::vector<OperandState>& states, std::vector<OperandState>& by_coverage)
{
    if (stages.size() > std::numeric_limits<std::uint32_t>::max())
        return Error::resource_limit_exceeded;
    std::vector<std::uint64_t> stage_ids;
    for (std::uint32_t stage_index = 0; stage_index < stages.size(); ++stage_index)
    {
        const ExactBooleanStage& stage = stages[stage_index];
        if (stage.stage_id == 0 || (stage.operation != ExactBooleanStageOperation::union_ &&
                                    stage.operation != ExactBooleanStageOperation::difference))
            return Error::invalid_argument;
        stage_ids.push_back(stage.stage_id);
        for (const ExactBooleanOperand& operand : stage.operands)
        {
            if (operand.coverage_id == 0 || operand.source_id == 0)
                return Error::invalid_argument;
            states.push_back({operand.coverage_id, operand.source_id, stage.stage_id, stage_index,
                              stage.operation});
        }
    }
    if (states.size() > kMaximumOperands)
        return Error::resource_limit_exceeded;
    std::sort(stage_ids.begin(), stage_ids.end());
    if (std::adjacent_find(stage_ids.begin(), stage_ids.end()) != stage_ids.end())
        return Error::invalid_argument;
    std::sort(states.begin(), states.end(), [](const OperandState& left, const OperandState& right)
              { return left.operand_id < right.operand_id; });
    if (std::adjacent_find(states.begin(), states.end(),
                           [](const OperandState& left, const OperandState& right)
                           { return left.operand_id == right.operand_id; }) != states.end())
        return Error::invalid_argument;
    by_coverage = states;
    std::sort(by_coverage.begin(), by_coverage.end(),
              [](const OperandState& left, const OperandState& right)
              { return left.coverage_id < right.coverage_id; });
    if (std::adjacent_find(by_coverage.begin(), by_coverage.end(),
                           [](const OperandState& left, const OperandState& right)
                           { return left.coverage_id == right.coverage_id; }) != by_coverage.end())
        return Error::invalid_argument;
    return Error::none;
}

bool face_contains_coverage(const ExactArrangement& arrangement, std::uint32_t face,
                            std::uint64_t coverage_id)
{
    const ExactArrangementFace& value = arrangement.faces()[face];
    const auto begin = arrangement.face_coverages().begin() + value.coverage_begin;
    return std::binary_search(begin, begin + value.coverage_count, coverage_id);
}

Error simulate_faces(const ExactArrangement& arrangement, const ExactBooleanSelection& selection,
                     const std::vector<ExactBooleanStage>& stages,
                     std::vector<OperandState>& states,
                     const std::vector<OperandState>& by_coverage)
{
    std::uint64_t positive_cursor = 0;
    std::uint64_t subtraction_cursor = 0;
    for (std::uint32_t face_id = 0; face_id < arrangement.faces().size(); ++face_id)
    {
        const ExactArrangementFace& face = arrangement.faces()[face_id];
        const std::uint64_t coverage_end =
            static_cast<std::uint64_t>(face.coverage_begin) + face.coverage_count;
        if (coverage_end > arrangement.face_coverages().size())
            return Error::invalid_argument;
        const auto coverage = arrangement.face_coverages().begin() + face.coverage_begin;
        if (!std::is_sorted(coverage, coverage + face.coverage_count) ||
            std::adjacent_find(coverage, coverage + face.coverage_count) !=
                coverage + face.coverage_count)
            return Error::invalid_argument;
        for (std::uint32_t index = 0; index < face.coverage_count; ++index)
            if (find_by_coverage(by_coverage, coverage[index]) == nullptr)
                return Error::invalid_argument;

        bool material = false;
        const bool positive_area = !face.unbounded;
        std::vector<std::uint64_t> positive;
        std::vector<std::uint64_t> removal_history;
        std::vector<std::uint64_t> active_removals;
        for (const ExactBooleanStage& stage : stages)
        {
            std::vector<std::uint64_t> covered;
            for (const ExactBooleanOperand& operand : stage.operands)
                if (face_contains_coverage(arrangement, face_id, operand.coverage_id))
                    insert_id(covered, operand.source_id);
            if (covered.empty())
                continue;
            if (stage.operation == ExactBooleanStageOperation::union_)
            {
                if (!material && positive_area)
                    for (const std::uint64_t source : active_removals)
                    {
                        OperandState* state = find_by_id(states, source);
                        if (state == nullptr)
                            return Error::invalid_argument;
                        state->overwritten = true;
                    }
                if (!material)
                    active_removals.clear();
                for (const std::uint64_t source : covered)
                {
                    OperandState* state = find_by_id(states, source);
                    if (state == nullptr)
                        return Error::invalid_argument;
                    if (positive_area)
                    {
                        state->covered_area = true;
                        state->redundant = state->redundant || material || covered.size() > 1;
                    }
                    insert_id(positive, source);
                }
                material = true;
            }
            else
            {
                if (material && positive_area)
                {
                    active_removals.clear();
                    for (const std::uint64_t source : covered)
                    {
                        OperandState* state = find_by_id(states, source);
                        if (state == nullptr)
                            return Error::invalid_argument;
                        state->attributed_removal = true;
                        insert_id(removal_history, source);
                        insert_id(active_removals, source);
                    }
                }
                if (material)
                {
                    if (positive_area)
                        for (const std::uint64_t source : positive)
                        {
                            OperandState* state = find_by_id(states, source);
                            if (state == nullptr)
                                return Error::invalid_argument;
                            state->removed_later = true;
                        }
                    material = false;
                    positive.clear();
                }
            }
        }

        const ExactSelectedFace& actual = selection.faces()[face_id];
        const std::uint64_t positive_end =
            static_cast<std::uint64_t>(actual.positive_source_begin) + actual.positive_source_count;
        const std::uint64_t subtraction_end =
            static_cast<std::uint64_t>(actual.subtraction_source_begin) +
            actual.subtraction_source_count;
        if (actual.positive_source_begin != positive_cursor ||
            actual.subtraction_source_begin != subtraction_cursor ||
            positive_end > selection.positive_sources().size() ||
            subtraction_end > selection.subtraction_sources().size() ||
            actual.material != material || actual.positive_source_count != positive.size() ||
            actual.subtraction_source_count != removal_history.size() ||
            !std::equal(positive.begin(), positive.end(),
                        selection.positive_sources().begin() + actual.positive_source_begin) ||
            !std::equal(removal_history.begin(), removal_history.end(),
                        selection.subtraction_sources().begin() + actual.subtraction_source_begin))
            return Error::invalid_argument;
        if (positive_area)
        {
            for (const std::uint64_t source : positive)
                find_by_id(states, source)->final_lineage = true;
            if (!material)
                for (const std::uint64_t source : active_removals)
                    find_by_id(states, source)->unfilled_removal = true;
        }
        positive_cursor = positive_end;
        subtraction_cursor = subtraction_end;
    }
    return positive_cursor == selection.positive_sources().size() &&
                   subtraction_cursor == selection.subtraction_sources().size()
               ? Error::none
               : Error::invalid_argument;
}

Error validate_sources(const ExactArrangement& arrangement,
                       const std::vector<ExactOccurrenceSource>& occurrences,
                       std::vector<OperandState>& states,
                       const std::vector<OperandState>& by_coverage)
{
    std::vector<ExactOccurrenceSource> sorted = occurrences;
    std::sort(sorted.begin(), sorted.end(), [](const auto& left, const auto& right)
              { return left.occurrence_id < right.occurrence_id; });
    for (std::size_t index = 0; index < sorted.size(); ++index)
    {
        const ExactOccurrenceSource& occurrence = sorted[index];
        const OperandState* coverage = find_by_coverage(by_coverage, occurrence.coverage_id);
        if (!occurrence_source_valid(occurrence) || coverage == nullptr ||
            occurrence.source.operand_id != coverage->operand_id ||
            (index != 0 && sorted[index - 1].occurrence_id == occurrence.occurrence_id))
            return Error::invalid_argument;
        OperandState* state = find_by_id(states, occurrence.source.operand_id);
        if (state == nullptr)
            return Error::invalid_argument;
        insert_source(state->sources, occurrence.source);
    }
    std::vector<std::uint64_t> used;
    std::uint64_t membership_cursor = 0;
    for (const ExactArrangementEdge& edge : arrangement.edges())
    {
        const std::uint64_t membership_end =
            static_cast<std::uint64_t>(edge.membership_begin) + edge.membership_count;
        if (edge.membership_begin != membership_cursor || edge.membership_count == 0 ||
            membership_end > arrangement.memberships().size())
            return Error::invalid_argument;
        for (std::uint32_t index = 0; index < edge.membership_count; ++index)
        {
            const ExactCurveMembership& membership =
                arrangement.memberships()[edge.membership_begin + index];
            const ExactOccurrenceSource* occurrence =
                find_occurrence(sorted, membership.occurrence_id);
            if (occurrence == nullptr || !role_matches_curve(occurrence->source.role, edge.kind))
                return Error::invalid_argument;
            used.push_back(membership.occurrence_id);
        }
        membership_cursor = membership_end;
    }
    if (membership_cursor != arrangement.memberships().size())
        return Error::invalid_argument;
    std::sort(used.begin(), used.end());
    used.erase(std::unique(used.begin(), used.end()), used.end());
    if (used.size() != sorted.size())
        return Error::invalid_argument;
    for (std::size_t index = 0; index < sorted.size(); ++index)
        if (used[index] != sorted[index].occurrence_id)
            return Error::invalid_argument;
    for (const OperandState& state : states)
        if (state.sources.empty())
            return Error::invalid_argument;
    return Error::none;
}

struct RegionMaps
{
    std::vector<std::uint32_t> half_edge_ring;
    std::vector<std::uint32_t> ring_region;
};

Error validate_regions(const ExactArrangement& arrangement, const ExactBooleanSelection& selection,
                       const ExactBooleanRegions& regions, std::vector<OperandState>& states,
                       RegionMaps& maps)
{
    const std::uint32_t none = std::numeric_limits<std::uint32_t>::max();
    const std::uint32_t face_count = static_cast<std::uint32_t>(arrangement.faces().size());
    const std::uint32_t edge_count = static_cast<std::uint32_t>(arrangement.edges().size());
    const std::uint32_t half_edge_count =
        static_cast<std::uint32_t>(arrangement.half_edges().size());
    if (selection.faces().size() != face_count ||
        half_edge_count != static_cast<std::uint64_t>(edge_count) * 2)
        return Error::invalid_argument;
    std::vector<std::array<std::uint32_t, 2>> edge_half_edges(edge_count, {none, none});
    for (std::uint32_t id = 0; id < half_edge_count; ++id)
    {
        const ExactArrangementHalfEdge& half_edge = arrangement.half_edges()[id];
        if (half_edge.edge >= edge_count || half_edge.face >= face_count ||
            half_edge.twin >= half_edge_count)
            return Error::invalid_argument;
        auto& pair = edge_half_edges[half_edge.edge];
        if (pair[0] == none)
            pair[0] = id;
        else if (pair[1] == none)
            pair[1] = id;
        else
            return Error::invalid_argument;
    }
    DisjointSet components(face_count);
    for (const auto& pair : edge_half_edges)
    {
        if (pair[0] == none || pair[1] == none)
            return Error::invalid_argument;
        const std::uint32_t left = arrangement.half_edges()[pair[0]].face;
        const std::uint32_t right = arrangement.half_edges()[pair[1]].face;
        if (left == right)
            return Error::invalid_argument;
        if (selection.faces()[left].material == selection.faces()[right].material)
            components.unite(left, right);
    }

    maps.half_edge_ring.assign(half_edge_count, none);
    maps.ring_region.assign(regions.rings().size(), none);
    std::uint64_t ring_cursor = 0;
    for (std::uint32_t ring_id = 0; ring_id < regions.rings().size(); ++ring_id)
    {
        const ExactResultRing& ring = regions.rings()[ring_id];
        const std::uint64_t end =
            static_cast<std::uint64_t>(ring.half_edge_begin) + ring.half_edge_count;
        if (ring.half_edge_begin != ring_cursor || ring.half_edge_count == 0 ||
            end > regions.ring_half_edges().size())
            return Error::invalid_argument;
        for (std::uint32_t index = 0; index < ring.half_edge_count; ++index)
        {
            const std::uint32_t half_edge = regions.ring_half_edges()[ring.half_edge_begin + index];
            if (half_edge >= half_edge_count || maps.half_edge_ring[half_edge] != none)
                return Error::invalid_argument;
            maps.half_edge_ring[half_edge] = ring_id;
        }
        ring_cursor = end;
    }
    if (ring_cursor != regions.ring_half_edges().size())
        return Error::invalid_argument;
    for (std::uint32_t id = 0; id < half_edge_count; ++id)
    {
        const ExactArrangementHalfEdge& half_edge = arrangement.half_edges()[id];
        const bool boundary =
            selection.faces()[half_edge.face].material &&
            !selection.faces()[arrangement.half_edges()[half_edge.twin].face].material;
        if ((maps.half_edge_ring[id] != none) != boundary)
            return Error::invalid_argument;
    }

    std::vector<std::uint32_t> root_region(face_count, none);
    std::uint64_t region_source_cursor = 0;
    std::vector<ExactResultAssociation> expected_associations;
    for (std::uint32_t region_id = 0; region_id < regions.regions().size(); ++region_id)
    {
        const ExactResultRegion& region = regions.regions()[region_id];
        const std::uint64_t source_end =
            static_cast<std::uint64_t>(region.positive_source_begin) + region.positive_source_count;
        if (region.outer_ring >= regions.rings().size() ||
            region.positive_source_begin != region_source_cursor ||
            region.positive_source_count == 0 || source_end > regions.positive_sources().size())
            return Error::invalid_argument;
        const ExactResultRing& outer = regions.rings()[region.outer_ring];
        const std::uint32_t half_edge = regions.ring_half_edges()[outer.half_edge_begin];
        const std::uint32_t root = components.find(arrangement.half_edges()[half_edge].face);
        if (root_region[root] != none)
            return Error::invalid_argument;
        root_region[root] = region_id;
        for (std::uint32_t index = 0; index < region.positive_source_count; ++index)
        {
            const std::uint64_t operand =
                regions.positive_sources()[region.positive_source_begin + index];
            if (index != 0 &&
                regions.positive_sources()[region.positive_source_begin + index - 1] >= operand)
                return Error::invalid_argument;
            expected_associations.push_back({operand, region_id});
        }
        region_source_cursor = source_end;
    }
    if (region_source_cursor != regions.positive_sources().size())
        return Error::invalid_argument;
    std::sort(expected_associations.begin(), expected_associations.end(),
              [](const ExactResultAssociation& left, const ExactResultAssociation& right)
              {
                  return std::tie(left.source_id, left.result_region) <
                         std::tie(right.source_id, right.result_region);
              });
    if (expected_associations.size() != regions.associations().size())
        return Error::invalid_argument;
    for (std::size_t index = 0; index < expected_associations.size(); ++index)
        if (expected_associations[index].source_id != regions.associations()[index].source_id ||
            expected_associations[index].result_region !=
                regions.associations()[index].result_region)
            return Error::invalid_argument;

    std::vector<std::vector<std::uint64_t>> expected_sources(regions.regions().size());
    for (std::uint32_t face = 0; face < face_count; ++face)
    {
        if (!selection.faces()[face].material)
            continue;
        const std::uint32_t region = root_region[components.find(face)];
        if (region == none)
            return Error::invalid_argument;
        const ExactSelectedFace& selected = selection.faces()[face];
        for (std::uint32_t index = 0; index < selected.positive_source_count; ++index)
            insert_id(expected_sources[region],
                      selection.positive_sources()[selected.positive_source_begin + index]);
    }
    for (std::uint32_t region = 0; region < regions.regions().size(); ++region)
    {
        const ExactResultRegion& actual = regions.regions()[region];
        if (actual.positive_source_count != expected_sources[region].size() ||
            !std::equal(expected_sources[region].begin(), expected_sources[region].end(),
                        regions.positive_sources().begin() + actual.positive_source_begin))
            return Error::invalid_argument;
        for (const std::uint64_t operand : expected_sources[region])
        {
            OperandState* state = find_by_id(states, operand);
            if (state == nullptr || state->operation != ExactBooleanStageOperation::union_)
                return Error::invalid_argument;
            insert_index(state->regions, region);
        }
    }
    for (std::uint32_t ring = 0; ring < regions.rings().size(); ++ring)
    {
        const ExactResultRing& value = regions.rings()[ring];
        const std::uint32_t half_edge = regions.ring_half_edges()[value.half_edge_begin];
        maps.ring_region[ring] =
            root_region[components.find(arrangement.half_edges()[half_edge].face)];
        if (maps.ring_region[ring] == none)
            return Error::invalid_argument;
    }
    return Error::none;
}

Error collect_surviving_subtractions(const ExactArrangement& arrangement,
                                     const ExactBooleanSelection& selection,
                                     const ExactBooleanRegions& regions,
                                     const ExactBooleanProvenance& provenance,
                                     const RegionMaps& maps, std::vector<OperandState>& states)
{
    if (provenance.fragments().size() != regions.ring_half_edges().size())
        return Error::invalid_argument;
    std::vector<bool> seen(arrangement.half_edges().size());
    for (const ExactBoundaryFragmentProvenance& fragment : provenance.fragments())
    {
        if (fragment.half_edge >= maps.half_edge_ring.size() || seen[fragment.half_edge] ||
            maps.half_edge_ring[fragment.half_edge] == std::numeric_limits<std::uint32_t>::max())
            return Error::invalid_argument;
        seen[fragment.half_edge] = true;
        const std::uint64_t subtraction_end =
            static_cast<std::uint64_t>(fragment.subtraction_source_begin) +
            fragment.subtraction_source_count;
        if (subtraction_end > provenance.source_references().size())
            return Error::invalid_argument;
        const ExactArrangementHalfEdge& half_edge = arrangement.half_edges()[fragment.half_edge];
        const ExactSelectedFace& empty =
            selection.faces()[arrangement.half_edges()[half_edge.twin].face];
        if (fragment.subtraction_source_count != empty.subtraction_source_count)
            return Error::invalid_argument;
        for (std::uint32_t index = 0; index < fragment.subtraction_source_count; ++index)
        {
            const ExactSourceReference& source =
                provenance.source_references()[fragment.subtraction_source_begin + index];
            const std::uint64_t expected_operand =
                selection.subtraction_sources()[empty.subtraction_source_begin + index];
            OperandState* state = find_by_id(states, expected_operand);
            if (state == nullptr || state->operation != ExactBooleanStageOperation::difference ||
                source.kind != ExactSourceKind::subtractive_operand_effect ||
                source.role != ExactSourceRole::none || source.operand_id != expected_operand ||
                source.primary_id != state->stage_id || source.secondary_id != 0)
                return Error::invalid_argument;
            const std::uint32_t ring = maps.half_edge_ring[fragment.half_edge];
            insert_index(state->rings, ring);
            insert_index(state->regions, maps.ring_region[ring]);
        }
    }
    return Error::none;
}

Error append_indices(std::vector<std::uint32_t>& output, const std::vector<std::uint32_t>& values,
                     std::uint32_t& begin, std::uint32_t& count)
{
    if (output.size() > kMaximumReferences || values.size() > kMaximumReferences - output.size())
        return Error::resource_limit_exceeded;
    begin = static_cast<std::uint32_t>(output.size());
    count = static_cast<std::uint32_t>(values.size());
    output.insert(output.end(), values.begin(), values.end());
    return Error::none;
}

void add_event(std::vector<ExactOperandOutcomeEvent>& events,
               std::vector<std::uint32_t>& ring_references,
               std::vector<std::uint32_t>& region_references, const OperandState& state,
               ExactOperandOutcomeKind kind, bool include_rings, bool include_regions)
{
    if (events.size() >= kMaximumEvents)
        throw std::length_error("operand outcome event limit exceeded");
    ExactOperandOutcomeEvent event;
    event.operand_id = state.operand_id;
    event.kind = kind;
    event.source_begin = state.source_begin;
    event.source_count = state.source_count;
    if (include_rings && append_indices(ring_references, state.rings, event.ring_reference_begin,
                                        event.ring_reference_count) != Error::none)
        throw std::length_error("operand outcome ring-reference limit exceeded");
    if (include_regions &&
        append_indices(region_references, state.regions, event.region_reference_begin,
                       event.region_reference_count) != Error::none)
        throw std::length_error("operand outcome region-reference limit exceeded");
    events.push_back(event);
}

} // namespace

ExactBooleanOutcomes::ExactBooleanOutcomes(
    Budget& budget, std::uint64_t charged_bytes, std::vector<ExactOperandOutcomeEvent> events,
    std::vector<std::uint32_t> ring_references, std::vector<std::uint32_t> region_references,
    std::vector<ExactSourceReference> source_references) noexcept
    : budget_(&budget), charged_bytes_(charged_bytes), events_(std::move(events)),
      ring_references_(std::move(ring_references)),
      region_references_(std::move(region_references)),
      source_references_(std::move(source_references))
{
}

ExactBooleanOutcomes::~ExactBooleanOutcomes()
{
    release();
}

ExactBooleanOutcomes::ExactBooleanOutcomes(ExactBooleanOutcomes&& other) noexcept
    : budget_(std::exchange(other.budget_, nullptr)),
      charged_bytes_(std::exchange(other.charged_bytes_, 0)), events_(std::move(other.events_)),
      ring_references_(std::move(other.ring_references_)),
      region_references_(std::move(other.region_references_)),
      source_references_(std::move(other.source_references_))
{
}

ExactBooleanOutcomes& ExactBooleanOutcomes::operator=(ExactBooleanOutcomes&& other) noexcept
{
    if (this != &other)
    {
        release();
        budget_ = std::exchange(other.budget_, nullptr);
        charged_bytes_ = std::exchange(other.charged_bytes_, 0);
        events_ = std::move(other.events_);
        ring_references_ = std::move(other.ring_references_);
        region_references_ = std::move(other.region_references_);
        source_references_ = std::move(other.source_references_);
    }
    return *this;
}

void ExactBooleanOutcomes::release()
{
    if (budget_ != nullptr)
        budget_->release_storage(charged_bytes_);
    budget_ = nullptr;
    charged_bytes_ = 0;
}

const std::vector<ExactOperandOutcomeEvent>& ExactBooleanOutcomes::events() const
{
    return events_;
}

const std::vector<std::uint32_t>& ExactBooleanOutcomes::ring_references() const
{
    return ring_references_;
}

const std::vector<std::uint32_t>& ExactBooleanOutcomes::region_references() const
{
    return region_references_;
}

const std::vector<ExactSourceReference>& ExactBooleanOutcomes::source_references() const
{
    return source_references_;
}

ExactBooleanOutcomesResult build_exact_boolean_outcomes(
    Budget& budget, const ExactArrangement& arrangement, const ExactBooleanSelection& selection,
    const ExactBooleanRegions& regions, const ExactBooleanProvenance& provenance,
    const std::vector<ExactBooleanStage>& stages,
    const std::vector<ExactOccurrenceSource>& occurrence_sources)
{
    try
    {
        const std::uint64_t face_count = arrangement.faces().size();
        const std::uint64_t edge_count = arrangement.edges().size();
        const std::uint64_t half_edge_count = arrangement.half_edges().size();
        const std::uint64_t membership_count = arrangement.memberships().size();
        const std::uint64_t ring_count = regions.rings().size();
        const std::uint64_t region_count = regions.regions().size();
        const std::uint64_t region_source_count = regions.positive_sources().size();
        const std::uint64_t occurrence_count = occurrence_sources.size();
        std::uint64_t operand_count = 0;
        for (const ExactBooleanStage& stage : stages)
            operand_count = checked_add(operand_count, stage.operands.size());
        if (face_count == 0 || face_count > kMaximumFaces || edge_count > kMaximumEdges ||
            half_edge_count > kMaximumHalfEdges || membership_count > kMaximumMemberships ||
            stages.size() > kMaximumStages ||
            face_count > std::numeric_limits<std::uint32_t>::max() ||
            half_edge_count > std::numeric_limits<std::uint32_t>::max() ||
            edge_count > std::numeric_limits<std::uint32_t>::max() ||
            ring_count > std::numeric_limits<std::uint32_t>::max() ||
            region_count > std::numeric_limits<std::uint32_t>::max() ||
            regions.ring_half_edges().size() > kMaximumReferences ||
            region_source_count > kMaximumReferences ||
            selection.positive_sources().size() > kMaximumReferences ||
            selection.subtraction_sources().size() > kMaximumReferences ||
            operand_count > kMaximumOperands || occurrence_count > kMaximumReferences)
            return failure(Error::resource_limit_exceeded);
        const std::uint64_t maximum_events =
            std::min(kMaximumEvents, checked_multiply(operand_count, 3));
        const std::uint64_t maximum_result_references =
            checked_multiply(operand_count, checked_add(ring_count, region_count));
        if (maximum_result_references > kMaximumReferences)
            return failure(Error::resource_limit_exceeded);
        const std::uint64_t charge = outcome_storage_charge(
            stages.size(), operand_count, occurrence_count, membership_count, face_count,
            edge_count, half_edge_count, ring_count, region_count, region_source_count,
            maximum_events, maximum_result_references);
        const std::uint64_t work = checked_add(
            256, checked_add(checked_multiply(face_count, checked_multiply(operand_count, 4)),
                             checked_add(checked_multiply(half_edge_count, 32),
                                         checked_multiply(occurrence_count, 32))));
        if (!budget.consume_work(work))
            return failure(Error::resource_limit_exceeded);
        StorageReservation reservation(budget, charge);
        if (!reservation.acquired())
            return failure(Error::resource_limit_exceeded);

        std::vector<OperandState> states;
        std::vector<OperandState> by_coverage;
        if (const Error error = normalize_operands(stages, states, by_coverage);
            error != Error::none)
            return failure(error);
        if (states.size() != operand_count || selection.faces().size() != face_count)
            return failure(Error::invalid_argument);
        if (const Error error = simulate_faces(arrangement, selection, stages, states, by_coverage);
            error != Error::none)
            return failure(error);
        if (const Error error =
                validate_sources(arrangement, occurrence_sources, states, by_coverage);
            error != Error::none)
            return failure(error);
        RegionMaps maps;
        if (const Error error = validate_regions(arrangement, selection, regions, states, maps);
            error != Error::none)
            return failure(error);
        if (const Error error = collect_surviving_subtractions(arrangement, selection, regions,
                                                               provenance, maps, states);
            error != Error::none)
            return failure(error);

        std::vector<ExactSourceReference> source_references;
        for (OperandState& state : states)
        {
            if (source_references.size() > kMaximumReferences ||
                state.sources.size() > kMaximumReferences - source_references.size())
                return failure(Error::resource_limit_exceeded);
            state.source_begin = static_cast<std::uint32_t>(source_references.size());
            state.source_count = static_cast<std::uint32_t>(state.sources.size());
            source_references.insert(source_references.end(), state.sources.begin(),
                                     state.sources.end());
        }

        std::vector<ExactOperandOutcomeEvent> events;
        std::vector<std::uint32_t> ring_references;
        std::vector<std::uint32_t> region_references;
        for (const OperandState& state : states)
        {
            if (state.operation == ExactBooleanStageOperation::union_)
            {
                if (state.final_lineage)
                    add_event(events, ring_references, region_references, state,
                              ExactOperandOutcomeKind::contributes_final_material, false, true);
                if (state.redundant)
                    add_event(events, ring_references, region_references, state,
                              ExactOperandOutcomeKind::redundant_or_absorbed_coverage, false,
                              false);
                if (state.removed_later && state.final_lineage)
                    add_event(events, ring_references, region_references, state,
                              ExactOperandOutcomeKind::partially_removed_later, false, false);
                if (state.covered_area && !state.final_lineage)
                    add_event(events, ring_references, region_references, state,
                              ExactOperandOutcomeKind::completely_removed_later, false, false);
                if (!state.covered_area)
                    add_event(events, ring_references, region_references, state,
                              ExactOperandOutcomeKind::no_effect, false, false);
            }
            else
            {
                if (state.unfilled_removal)
                    add_event(events, ring_references, region_references, state,
                              ExactOperandOutcomeKind::subtraction_effect_survives, true, true);
                if (state.overwritten)
                    add_event(events, ring_references, region_references, state,
                              ExactOperandOutcomeKind::subtraction_effect_overwritten_later, false,
                              false);
                if (!state.attributed_removal)
                    add_event(events, ring_references, region_references, state,
                              ExactOperandOutcomeKind::no_effect, false, false);
            }
        }

        const std::uint64_t transferred = reservation.transfer();
        return {Error::none, ExactBooleanOutcomes(
                                 budget, transferred, std::move(events), std::move(ring_references),
                                 std::move(region_references), std::move(source_references))};
    }
    catch (const std::length_error&)
    {
        return failure(Error::resource_limit_exceeded);
    }
    catch (const std::exception&)
    {
        return failure(Error::resource_limit_exceeded);
    }
}

} // namespace geometer::exact

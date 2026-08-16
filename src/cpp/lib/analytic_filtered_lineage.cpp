#include "geometer/analytic_filtered_lineage.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <tuple>
#include <unordered_map>
#include <utility>

namespace geometer
{
namespace
{
constexpr std::uint32_t kNoIndex = std::numeric_limits<std::uint32_t>::max();
constexpr std::uint64_t kSourceReferenceLogicalBytes = 32;
constexpr std::uint64_t kRawSourceLogicalBytes = 40;
constexpr std::uint64_t kBoundaryLogicalBytes = 24;
constexpr std::uint64_t kVertexLogicalBytes = 16;
constexpr std::uint64_t kRegionLogicalBytes = 16;
constexpr std::uint64_t kIndexLogicalBytes = 8;
constexpr std::uint64_t kSetNodeLogicalBytes = 8;

struct Operand
{
    std::uint64_t id = 0;
    std::uint64_t stage_id = 0;
    std::uint32_t stage = 0;
    std::uint8_t operation = 0;
};

struct Lookup
{
    std::uint64_t id = 0;
    std::uint32_t ordinal = 0;
};

enum class OwnerKind : std::uint8_t
{
    boundary_positive = 0,
    boundary_subtraction = 1,
    vertex = 2,
    region = 3,
};

struct RawSource
{
    OwnerKind kind = OwnerKind::boundary_positive;
    std::uint32_t owner = 0;
    AnalyticFilteredSourceReference source;
};

bool checked_add(std::uint64_t left, std::uint64_t right, std::uint64_t& output) noexcept
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
        return false;
    output = left + right;
    return true;
}

bool checked_multiply(std::uint64_t left, std::uint64_t right, std::uint64_t& output) noexcept
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left)
        return false;
    output = left * right;
    return true;
}

std::uint64_t sort_units(std::uint64_t count) noexcept
{
    if (count < 2)
        return 0;
    std::uint64_t levels = 0;
    for (std::uint64_t value = count - 1; value != 0; value >>= 1U)
        ++levels;
    return count * levels;
}

auto source_key(const AnalyticFilteredSourceReference& value) noexcept
{
    return std::tie(value.kind, value.role, value.operand_id, value.primary_id, value.secondary_id);
}

bool valid_source(const AnalyticFilteredSourceReference& source) noexcept
{
    const auto kind = static_cast<std::uint16_t>(source.kind);
    const auto role = static_cast<std::uint16_t>(source.role);
    if (kind < 1 || kind > 3 || source.operand_id == 0)
        return false;
    switch (role)
    {
    case 0:
    case 1:
    case 2:
    case 16:
    case 17:
    case 32:
    case 33:
    case 34:
    case 35:
    case 48:
    case 49:
    case 50:
    case 51:
    case 52:
    case 53:
    case 54:
        return true;
    default:
        return false;
    }
}

class CoverageDag
{
  public:
    CoverageDag(const std::vector<AnalyticFilteredCoverageStateNode>& input,
                std::uint32_t operand_count, std::uint64_t& visits, std::uint64_t& union_visits)
        : nodes_(input), operand_count_(operand_count), visits_(visits), union_visits_(union_visits)
    {
        leaf_capacity_ = 1;
        while (leaf_capacity_ < std::max<std::uint32_t>(1, operand_count_))
            leaf_capacity_ <<= 1U;
        for (std::uint32_t index = 2; index < nodes_.size(); ++index)
            intern_.emplace(key(nodes_[index].left, nodes_[index].right), index);
    }

    [[nodiscard]] bool valid() const noexcept
    {
        if (nodes_.size() < 2 || nodes_[0].left != 0 || nodes_[0].right != 0 ||
            nodes_[1].left != kNoIndex || nodes_[1].right != kNoIndex)
            return false;
        for (std::uint32_t index = 2; index < nodes_.size(); ++index)
            if (nodes_[index].left >= index || nodes_[index].right >= index ||
                (nodes_[index].left == 0 && nodes_[index].right == 0))
                return false;
        return true;
    }

    [[nodiscard]] bool contains(std::uint32_t root, std::uint32_t operand) noexcept
    {
        if (root >= nodes_.size() || operand >= operand_count_)
            return false;
        std::uint32_t begin = 0;
        std::uint32_t width = leaf_capacity_;
        while (width > 1)
        {
            ++visits_;
            if (root == 0)
                return false;
            if (root == 1 || root >= nodes_.size())
                return false;
            const std::uint32_t half = width / 2;
            if (operand < begin + half)
                root = nodes_[root].left;
            else
            {
                begin += half;
                root = nodes_[root].right;
            }
            width = half;
        }
        ++visits_;
        return root == 1;
    }

    std::uint32_t suffix(std::uint32_t root, std::uint32_t first)
    {
        return suffix_impl(root, 0, leaf_capacity_, first);
    }

    std::uint32_t unite(std::uint32_t left, std::uint32_t right)
    {
        if (left > right)
            std::swap(left, right);
        const std::uint64_t memo_key = key(left, right);
        if (const auto found = union_memo_.find(memo_key); found != union_memo_.end())
            return found->second;
        const std::uint32_t output = unite_impl(left, right, leaf_capacity_);
        union_memo_.emplace(memo_key, output);
        return output;
    }

    template <typename Emit> bool enumerate(std::uint32_t root, Emit&& emit)
    {
        return enumerate_impl(root, 0, leaf_capacity_, emit);
    }

    [[nodiscard]] std::uint64_t logical_bytes() const noexcept
    {
        return static_cast<std::uint64_t>(nodes_.size()) * kSetNodeLogicalBytes;
    }

  private:
    static std::uint64_t key(std::uint32_t left, std::uint32_t right) noexcept
    {
        return (static_cast<std::uint64_t>(left) << 32U) | right;
    }

    std::uint32_t intern(std::uint32_t left, std::uint32_t right)
    {
        if (left == 0 && right == 0)
            return 0;
        const std::uint64_t value = key(left, right);
        if (const auto found = intern_.find(value); found != intern_.end())
            return found->second;
        if (nodes_.size() >= std::numeric_limits<std::uint32_t>::max())
            return kNoIndex;
        const std::uint32_t node = static_cast<std::uint32_t>(nodes_.size());
        nodes_.push_back({left, right});
        intern_.emplace(value, node);
        return node;
    }

    std::uint32_t suffix_impl(std::uint32_t root, std::uint32_t begin, std::uint32_t width,
                              std::uint32_t first)
    {
        ++visits_;
        if (root == 0 || begin + width <= first)
            return 0;
        if (root >= nodes_.size() || (root == 1 && width != 1))
            return kNoIndex;
        if (begin >= first || width == 1)
            return root;
        const std::uint32_t half = width / 2;
        const std::uint32_t left = suffix_impl(nodes_[root].left, begin, half, first);
        const std::uint32_t right = suffix_impl(nodes_[root].right, begin + half, half, first);
        if (left == kNoIndex || right == kNoIndex)
            return kNoIndex;
        return intern(left, right);
    }

    std::uint32_t unite_impl(std::uint32_t left, std::uint32_t right, std::uint32_t width)
    {
        ++union_visits_;
        if (left == right || right == 0)
            return left;
        if (left == 0)
            return right;
        if (left >= nodes_.size() || right >= nodes_.size())
            return kNoIndex;
        if (width == 1)
            return left == 1 && right == 1 ? 1 : kNoIndex;
        if (left == 1 || right == 1)
            return kNoIndex;
        const std::uint32_t half = width / 2;
        const std::uint32_t merged_left = unite_impl(nodes_[left].left, nodes_[right].left, half);
        const std::uint32_t merged_right =
            unite_impl(nodes_[left].right, nodes_[right].right, half);
        if (merged_left == kNoIndex || merged_right == kNoIndex)
            return kNoIndex;
        return intern(merged_left, merged_right);
    }

    template <typename Emit>
    bool enumerate_impl(std::uint32_t root, std::uint32_t begin, std::uint32_t width, Emit& emit)
    {
        ++visits_;
        if (root == 0)
            return true;
        if (root >= nodes_.size())
            return false;
        if (width == 1)
            return root == 1 && begin < operand_count_ && emit(begin);
        if (root == 1)
            return false;
        const std::uint32_t half = width / 2;
        return enumerate_impl(nodes_[root].left, begin, half, emit) &&
               enumerate_impl(nodes_[root].right, begin + half, half, emit);
    }

    std::vector<AnalyticFilteredCoverageStateNode> nodes_;
    std::uint32_t operand_count_ = 0;
    std::uint32_t leaf_capacity_ = 1;
    std::uint64_t& visits_;
    std::uint64_t& union_visits_;
    std::unordered_map<std::uint64_t, std::uint32_t> intern_;
    std::unordered_map<std::uint64_t, std::uint32_t> union_memo_;
};

class Builder
{
  public:
    Builder(const AnalyticRequestPacketRecords& records, std::uint32_t job_index,
            const AnalyticFilteredGeometry& geometry,
            const std::vector<AnalyticCurvePair>& candidate_pairs,
            const AnalyticSolverLimits& limits)
        : records_(records), job_index_(job_index), geometry_(geometry),
          candidate_pairs_(candidate_pairs), limits_(limits)
    {
    }

    AnalyticFilteredLineageResult build()
    {
        result_.regions = build_analytic_filtered_regions(records_, job_index_, geometry_,
                                                          candidate_pairs_, limits_);
        result_.telemetry.regions_work_units = result_.regions.telemetry.predicate_calls;
        result_.telemetry.regions_peak_working_memory_bytes =
            result_.regions.telemetry.peak_working_memory_bytes;
        result_.telemetry.algebraic_fallback_calls =
            result_.regions.telemetry.algebraic_fallback_calls;
        if (result_.regions.error != AnalyticFilteredRegionsError::none)
        {
            result_.error = result_.regions.error == AnalyticFilteredRegionsError::invalid_argument
                                ? AnalyticFilteredLineageError::invalid_argument
                                : AnalyticFilteredLineageError::resource_limit_exceeded;
            return std::move(result_);
        }
        try
        {
            if (!prepare_operands() || !project())
                return failure();
            result_.telemetry.predicate_calls = result_.regions.telemetry.predicate_calls + work_;
            result_.telemetry.lineage_work_units = work_;
            result_.telemetry.emitted_boundary_records = result_.boundaries.size();
            result_.telemetry.emitted_vertex_records = result_.vertices.size();
            result_.telemetry.emitted_region_records = result_.region_lineage.size();
            result_.telemetry.emitted_source_references = result_.source_references.size();
            return std::move(result_);
        }
        catch (const std::bad_alloc&)
        {
            result_.error = AnalyticFilteredLineageError::resource_limit_exceeded;
            return failure();
        }
    }

  private:
    bool charge(std::uint64_t units)
    {
        if (units >
            limits_.predicate_calls - std::min(limits_.predicate_calls,
                                               result_.regions.telemetry.predicate_calls + work_))
        {
            result_.error = AnalyticFilteredLineageError::resource_limit_exceeded;
            return false;
        }
        work_ += units;
        return true;
    }

    AnalyticFilteredLineageResult failure()
    {
        if (result_.error == AnalyticFilteredLineageError::none)
            result_.error = AnalyticFilteredLineageError::invalid_argument;
        const auto regions_work = result_.regions.telemetry.predicate_calls;
        const auto regions_memory = result_.regions.telemetry.peak_working_memory_bytes;
        const auto fallback = result_.regions.telemetry.algebraic_fallback_calls;
        const auto origin_x = result_.regions.selection.origin_x_nm;
        const auto origin_y = result_.regions.selection.origin_y_nm;
        result_.regions = {};
        result_.regions.selection.origin_x_nm = origin_x;
        result_.regions.selection.origin_y_nm = origin_y;
        result_.boundaries.clear();
        result_.vertices.clear();
        result_.region_lineage.clear();
        result_.source_references.clear();
        result_.telemetry.regions_work_units = regions_work;
        result_.telemetry.regions_peak_working_memory_bytes = regions_memory;
        result_.telemetry.lineage_work_units = work_;
        result_.telemetry.predicate_calls = regions_work + work_;
        result_.telemetry.peak_working_memory_bytes = regions_memory;
        result_.telemetry.algebraic_fallback_calls = fallback;
        return std::move(result_);
    }

    bool prepare_operands()
    {
        if (job_index_ >= records_.jobs.size())
            return false;
        const AnalyticRequestJobRecord& job = records_.jobs[job_index_];
        if (job.stage_begin > records_.stages.size() ||
            job.stage_count > records_.stages.size() - job.stage_begin || !charge(job.stage_count))
            return false;
        stage_operand_begin_.reserve(job.stage_count + 1);
        for (std::uint32_t local = 0; local < job.stage_count; ++local)
        {
            const AnalyticRequestStageRecord& stage = records_.stages[job.stage_begin + local];
            if (stage.operand_begin > records_.operands.size() ||
                stage.operand_count > records_.operands.size() - stage.operand_begin ||
                (stage.operation != 1 && stage.operation != 2) || stage.stage_id == 0)
                return false;
            stage_operand_begin_.push_back(static_cast<std::uint32_t>(operands_.size()));
            for (std::uint32_t offset = 0; offset < stage.operand_count; ++offset)
            {
                const AnalyticRequestOperandRecord& operand =
                    records_.operands[stage.operand_begin + offset];
                if (operand.operand_id == 0 || operands_.size() >= limits_.boundary_occurrences)
                    return false;
                operands_.push_back({operand.operand_id, stage.stage_id, local, stage.operation});
            }
        }
        stage_operand_begin_.push_back(static_cast<std::uint32_t>(operands_.size()));
        if (!charge(operands_.size() + sort_units(operands_.size())))
            return false;
        lookup_.reserve(operands_.size());
        for (std::uint32_t index = 0; index < operands_.size(); ++index)
            lookup_.push_back({operands_[index].id, index});
        std::sort(lookup_.begin(), lookup_.end(),
                  [](const Lookup& left, const Lookup& right) { return left.id < right.id; });
        for (std::uint32_t index = 1; index < lookup_.size(); ++index)
            if (lookup_[index - 1].id == lookup_[index].id)
                return false;
        occurrence_operands_.reserve(geometry_.occurrences.size());
        if (!charge(geometry_.occurrences.size()))
            return false;
        for (std::uint32_t index = 0; index < geometry_.occurrences.size(); ++index)
        {
            const AnalyticFilteredOccurrence& occurrence = geometry_.occurrences[index];
            if (!valid_source(occurrence.source) ||
                occurrence.source.operand_id != occurrence.coverage_id ||
                occurrence.occurrence_id != static_cast<std::uint64_t>(index) + 1)
                return false;
            const auto found = std::lower_bound(
                lookup_.begin(), lookup_.end(), occurrence.coverage_id,
                [](const Lookup& value, std::uint64_t id) { return value.id < id; });
            if (found == lookup_.end() || found->id != occurrence.coverage_id)
                return false;
            occurrence_operands_.push_back(found->ordinal);
        }
        operand_occurrence_begin_.assign(operands_.size() + 1, 0);
        for (const std::uint32_t operand : occurrence_operands_)
            ++operand_occurrence_begin_[operand + 1];
        for (std::uint32_t operand = 1; operand < operand_occurrence_begin_.size(); ++operand)
            operand_occurrence_begin_[operand] += operand_occurrence_begin_[operand - 1];
        ordered_occurrences_.resize(geometry_.occurrences.size());
        std::vector<std::uint32_t> cursor = operand_occurrence_begin_;
        for (std::uint32_t occurrence = 0; occurrence < occurrence_operands_.size(); ++occurrence)
            ordered_occurrences_[cursor[occurrence_operands_[occurrence]]++] = occurrence;
        return true;
    }

    const Operand* operand_for_occurrence(std::uint32_t curve_index, std::uint32_t& ordinal) const
    {
        if (curve_index == 0 || curve_index > occurrence_operands_.size())
            return nullptr;
        ordinal = occurrence_operands_[curve_index - 1];
        return ordinal < operands_.size() ? &operands_[ordinal] : nullptr;
    }

    bool append_raw(OwnerKind kind, std::uint32_t owner,
                    const AnalyticFilteredSourceReference& source)
    {
        if (raw_.size() == limits_.source_reference_memberships)
        {
            result_.error = AnalyticFilteredLineageError::resource_limit_exceeded;
            return false;
        }
        raw_.push_back({kind, owner, source});
        return true;
    }

    bool project()
    {
        auto& regions = result_.regions;
        auto& selection = regions.selection;
        auto& arrangement = selection.arrangement;
        CoverageDag dag(selection.coverage_state_nodes,
                        static_cast<std::uint32_t>(operands_.size()),
                        result_.telemetry.coverage_node_visits, result_.telemetry.set_union_visits);
        if (!dag.valid())
            return false;
        std::vector<std::uint32_t> region_by_component(selection.faces.size(), kNoIndex);
        for (std::uint32_t region = 0; region < regions.regions.size(); ++region)
        {
            const std::uint32_t component = regions.regions[region].material_component;
            if (component >= region_by_component.size() ||
                region_by_component[component] != kNoIndex)
                return false;
            region_by_component[component] = region;
        }
        std::vector<std::uint32_t> region_roots(regions.regions.size());
        if (!charge(selection.faces.size()))
            return false;
        for (std::uint32_t face = 0; face < selection.faces.size(); ++face)
        {
            const AnalyticFilteredSelectedFace& selected = selection.faces[face];
            if (!selected.material)
                continue;
            if (face >= regions.face_components.size())
                return false;
            const std::uint32_t component = regions.face_components[face];
            if (component >= region_by_component.size() ||
                region_by_component[component] == kNoIndex ||
                selected.positive_stage_begin >= stage_operand_begin_.size())
                return false;
            const std::uint32_t root = dag.suffix(
                selected.coverage_state_root, stage_operand_begin_[selected.positive_stage_begin]);
            if (root == kNoIndex)
                return false;
            std::uint32_t& region_root = region_roots[region_by_component[component]];
            region_root = dag.unite(region_root, root);
            if (region_root == kNoIndex)
                return false;
        }

        if (!charge(regions.ring_half_edges.size()))
            return false;
        result_.boundaries.resize(regions.ring_half_edges.size());
        for (std::uint32_t owner = 0; owner < regions.ring_half_edges.size(); ++owner)
        {
            const std::uint32_t half_edge_index = regions.ring_half_edges[owner];
            if (half_edge_index >= arrangement.half_edges.size() ||
                half_edge_index >= selection.half_edge_faces.size())
                return false;
            const AnalyticArrangementHalfEdge& half_edge = arrangement.half_edges[half_edge_index];
            if (half_edge.twin >= arrangement.half_edges.size() ||
                half_edge.twin >= selection.half_edge_faces.size() ||
                half_edge.edge >= arrangement.edges.size())
                return false;
            const std::uint32_t material_face = selection.half_edge_faces[half_edge_index];
            const std::uint32_t empty_face = selection.half_edge_faces[half_edge.twin];
            if (material_face >= selection.faces.size() || empty_face >= selection.faces.size() ||
                !selection.faces[material_face].material || selection.faces[empty_face].material)
                return false;
            const AnalyticArrangementEdgeNm& edge = arrangement.edges[half_edge.edge];
            if (edge.membership_begin > arrangement.memberships.size() ||
                edge.membership_count > arrangement.memberships.size() - edge.membership_begin)
                return false;
            for (std::uint32_t offset = 0; offset < edge.membership_count; ++offset)
            {
                ++result_.telemetry.boundary_membership_visits;
                const AnalyticSpanMembership& membership =
                    arrangement.memberships[edge.membership_begin + offset];
                std::uint32_t ordinal = 0;
                const Operand* operand = operand_for_occurrence(membership.curve_index, ordinal);
                if (operand == nullptr)
                    return false;
                const bool covered_on_material_left =
                    membership.material_on_span_left == half_edge.forward;
                if (operand->operation == 1 && covered_on_material_left &&
                    operand->stage >= selection.faces[material_face].positive_stage_begin &&
                    dag.contains(selection.faces[material_face].coverage_state_root, ordinal))
                {
                    if (!append_raw(OwnerKind::boundary_positive, owner,
                                    geometry_.occurrences[membership.curve_index - 1].source))
                        return false;
                }
                if (operand->operation == 2 && !covered_on_material_left &&
                    operand->stage == selection.faces[empty_face].active_removal_stage &&
                    dag.contains(selection.faces[empty_face].coverage_state_root, ordinal))
                {
                    if (!append_raw(OwnerKind::boundary_subtraction, owner,
                                    {AnalyticFilteredSourceKind::subtractive_operand_effect,
                                     AnalyticFilteredSourceRole::none, operand->id,
                                     operand->stage_id, 0}))
                        return false;
                }
            }
        }

        std::vector<std::uint8_t> retained_vertices(arrangement.vertices.size());
        for (const std::uint32_t half_edge_index : regions.ring_half_edges)
        {
            if (half_edge_index >= arrangement.half_edges.size())
                return false;
            retained_vertices[arrangement.half_edges[half_edge_index].origin_vertex] = 1;
        }
        std::vector<std::uint32_t> vertex_owner(arrangement.vertices.size(), kNoIndex);
        for (std::uint32_t vertex = 0; vertex < retained_vertices.size(); ++vertex)
            if (retained_vertices[vertex])
            {
                vertex_owner[vertex] = static_cast<std::uint32_t>(result_.vertices.size());
                result_.vertices.push_back({vertex, {}});
            }
        if (!charge(arrangement.edges.size() + arrangement.collapsed_spans.size()))
            return false;
        for (const AnalyticArrangementEdgeNm& edge : arrangement.edges)
        {
            if (edge.start_vertex >= vertex_owner.size() ||
                edge.end_vertex >= vertex_owner.size() ||
                edge.membership_begin > arrangement.memberships.size() ||
                edge.membership_count > arrangement.memberships.size() - edge.membership_begin)
                return false;
            for (const std::uint32_t vertex : {edge.start_vertex, edge.end_vertex})
            {
                if (vertex_owner[vertex] == kNoIndex)
                    continue;
                for (std::uint32_t offset = 0; offset < edge.membership_count; ++offset)
                {
                    ++result_.telemetry.vertex_membership_visits;
                    const AnalyticSpanMembership& membership =
                        arrangement.memberships[edge.membership_begin + offset];
                    if (membership.curve_index == 0 ||
                        membership.curve_index > geometry_.occurrences.size() ||
                        !append_raw(OwnerKind::vertex, vertex_owner[vertex],
                                    geometry_.occurrences[membership.curve_index - 1].source))
                        return false;
                }
            }
        }
        for (const AnalyticArrangementCollapsedSpan& span : arrangement.collapsed_spans)
        {
            if (span.vertex >= vertex_owner.size() ||
                span.membership_begin > arrangement.memberships.size() ||
                span.membership_count > arrangement.memberships.size() - span.membership_begin)
                return false;
            if (vertex_owner[span.vertex] == kNoIndex)
                continue;
            for (std::uint32_t offset = 0; offset < span.membership_count; ++offset)
            {
                ++result_.telemetry.vertex_membership_visits;
                const AnalyticSpanMembership& membership =
                    arrangement.memberships[span.membership_begin + offset];
                if (membership.curve_index == 0 ||
                    membership.curve_index > geometry_.occurrences.size() ||
                    !append_raw(OwnerKind::vertex, vertex_owner[span.vertex],
                                geometry_.occurrences[membership.curve_index - 1].source))
                    return false;
            }
        }

        result_.region_lineage.resize(regions.regions.size());
        for (std::uint32_t region = 0; region < regions.regions.size(); ++region)
        {
            result_.region_lineage[region].region = region;
            if (!dag.enumerate(region_roots[region],
                               [&](std::uint32_t operand)
                               {
                                   if (operand + 1 >= operand_occurrence_begin_.size())
                                       return false;
                                   for (std::uint32_t cursor = operand_occurrence_begin_[operand];
                                        cursor < operand_occurrence_begin_[operand + 1]; ++cursor)
                                   {
                                       const std::uint32_t occurrence =
                                           ordered_occurrences_[cursor];
                                       if (!append_raw(OwnerKind::region, region,
                                                       geometry_.occurrences[occurrence].source))
                                           return false;
                                   }
                                   return true;
                               }))
                return false;
        }
        return publish(dag);
    }

    bool publish(const CoverageDag& dag)
    {
        std::uint64_t units = sort_units(raw_.size());
        if (!charge(units))
            return false;
        result_.telemetry.sort_work_units += units;
        std::sort(raw_.begin(), raw_.end(),
                  [](const RawSource& left, const RawSource& right)
                  {
                      if (left.kind != right.kind)
                          return left.kind < right.kind;
                      if (left.owner != right.owner)
                          return left.owner < right.owner;
                      return source_key(left.source) < source_key(right.source);
                  });
        raw_.erase(std::unique(raw_.begin(), raw_.end(),
                               [](const RawSource& left, const RawSource& right)
                               {
                                   return left.kind == right.kind && left.owner == right.owner &&
                                          source_key(left.source) == source_key(right.source);
                               }),
                   raw_.end());
        if (raw_.size() > limits_.source_reference_memberships ||
            raw_.size() > std::numeric_limits<std::uint32_t>::max())
        {
            result_.error = AnalyticFilteredLineageError::resource_limit_exceeded;
            return false;
        }
        result_.source_references.reserve(raw_.size());
        std::size_t cursor = 0;
        const auto fill = [&](OwnerKind kind, std::uint32_t owner,
                              AnalyticFilteredSourceRange& range, std::size_t& position,
                              std::vector<AnalyticFilteredSourceReference>& output)
        {
            range.begin = static_cast<std::uint32_t>(output.size());
            while (position < raw_.size() && raw_[position].kind == kind &&
                   raw_[position].owner == owner)
                output.push_back(raw_[position++].source);
            range.count = static_cast<std::uint32_t>(output.size()) - range.begin;
        };
        for (std::uint32_t owner = 0; owner < result_.boundaries.size(); ++owner)
        {
            result_.boundaries[owner].half_edge = result_.regions.ring_half_edges[owner];
            fill(OwnerKind::boundary_positive, owner, result_.boundaries[owner].positive, cursor,
                 result_.source_references);
        }
        for (std::uint32_t owner = 0; owner < result_.boundaries.size(); ++owner)
            fill(OwnerKind::boundary_subtraction, owner, result_.boundaries[owner].subtraction,
                 cursor, result_.source_references);
        for (std::uint32_t owner = 0; owner < result_.vertices.size(); ++owner)
            fill(OwnerKind::vertex, owner, result_.vertices[owner].intersection, cursor,
                 result_.source_references);
        for (std::uint32_t owner = 0; owner < result_.region_lineage.size(); ++owner)
            fill(OwnerKind::region, owner, result_.region_lineage[owner].positive_contributors,
                 cursor, result_.source_references);
        if (cursor != raw_.size())
            return false;

        std::uint64_t retained = result_.regions.telemetry.peak_working_memory_bytes;
        std::uint64_t added = 0;
        std::uint64_t term = 0;
        if (!checked_multiply(raw_.size(), kRawSourceLogicalBytes, added) ||
            !checked_multiply(result_.source_references.size(), kSourceReferenceLogicalBytes,
                              term) ||
            !checked_add(added, term, added) ||
            !checked_multiply(result_.boundaries.size(), kBoundaryLogicalBytes, term) ||
            !checked_add(added, term, added) ||
            !checked_multiply(result_.vertices.size(), kVertexLogicalBytes, term) ||
            !checked_add(added, term, added) ||
            !checked_multiply(result_.region_lineage.size(), kRegionLogicalBytes, term) ||
            !checked_add(added, term, added) || !checked_add(added, dag.logical_bytes(), added) ||
            !checked_add(retained, added, retained))
        {
            result_.error = AnalyticFilteredLineageError::resource_limit_exceeded;
            return false;
        }
        if (retained > limits_.working_memory_bytes)
        {
            result_.error = AnalyticFilteredLineageError::resource_limit_exceeded;
            return false;
        }
        result_.telemetry.peak_working_memory_bytes = retained;
        return charge(raw_.size() + result_.source_references.size());
    }

    const AnalyticRequestPacketRecords& records_;
    std::uint32_t job_index_ = 0;
    const AnalyticFilteredGeometry& geometry_;
    const std::vector<AnalyticCurvePair>& candidate_pairs_;
    const AnalyticSolverLimits& limits_;
    AnalyticFilteredLineageResult result_;
    std::vector<Operand> operands_;
    std::vector<Lookup> lookup_;
    std::vector<std::uint32_t> stage_operand_begin_;
    std::vector<std::uint32_t> occurrence_operands_;
    std::vector<std::uint32_t> operand_occurrence_begin_;
    std::vector<std::uint32_t> ordered_occurrences_;
    std::vector<RawSource> raw_;
    std::uint64_t work_ = 0;
};
} // namespace

AnalyticFilteredLineageResult
build_analytic_filtered_lineage(const AnalyticRequestPacketRecords& records,
                                std::uint32_t job_index, const AnalyticFilteredGeometry& geometry,
                                const std::vector<AnalyticCurvePair>& candidate_pairs,
                                const AnalyticSolverLimits& limits)
{
    return Builder(records, job_index, geometry, candidate_pairs, limits).build();
}

} // namespace geometer

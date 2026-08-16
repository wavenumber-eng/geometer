#include "geometer/analytic_filtered_lineage.h"

#include "analytic_filtered_boolean_selection_support.h"
#include "analytic_filtered_capacity.h"
#include "analytic_filtered_regions_internal.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <new>
#include <tuple>
#include <utility>
#include <vector>

namespace geometer
{
namespace
{
constexpr std::uint32_t kNoIndex = std::numeric_limits<std::uint32_t>::max();
constexpr std::uint64_t kRawLogicalBytes = 40;
constexpr std::uint64_t kSourceLogicalBytes = 32;
constexpr std::uint64_t kBoundaryLogicalBytes = 24;
constexpr std::uint64_t kVertexLogicalBytes = 16;
constexpr std::uint64_t kRegionLogicalBytes = 16;
constexpr std::uint64_t kOperandLogicalBytes = 32;
constexpr std::uint64_t kLookupLogicalBytes = 16;
constexpr std::uint64_t kAdjacencyLogicalBytes = 16;
constexpr std::uint64_t kIndexLogicalBytes = 8;
constexpr std::uint64_t kByteLogicalBytes = 1;

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
    boundary_positive,
    boundary_subtraction,
    vertex,
    region,
};
struct RawSource
{
    OwnerKind kind{};
    std::uint32_t owner = 0;
    AnalyticFilteredSourceReference source;
};
struct Adjacency
{
    std::uint32_t neighbor = 0;
    std::uint32_t edge = 0;
};

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

class Builder
{
  public:
    Builder(const AnalyticRequestPacketRecords& records, std::uint32_t job_index,
            const AnalyticFilteredGeometry& geometry,
            const std::vector<AnalyticCurvePair>& candidate_pairs,
            const AnalyticSolverLimits& limits)
        : records_(records), job_index_(job_index), geometry_(geometry), pairs_(candidate_pairs),
          limits_(limits)
    {
    }

    AnalyticFilteredLineageResult build()
    {
        analytic_regions_detail::LineageRegionsAdmission admission =
            analytic_regions_detail::build_regions_for_lineage(records_, job_index_, geometry_,
                                                               pairs_, limits_);
        reserved_work_ = admission.reserved_lineage_work;
        result_.telemetry.reserved_lineage_work_units = reserved_work_;
        result_.regions = std::move(admission.regions);
        const auto& region_telemetry = result_.regions.telemetry;
        result_.telemetry.regions_work_units = region_telemetry.predicate_calls;
        result_.telemetry.regions_peak_working_memory_bytes =
            region_telemetry.peak_working_memory_bytes;
        result_.telemetry.arrangement_work_units =
            result_.regions.selection.telemetry.arrangement_predicate_calls;
        result_.telemetry.algebraic_fallback_calls = region_telemetry.algebraic_fallback_calls;
        if (result_.regions.error != AnalyticFilteredRegionsError::none)
        {
            result_.error = result_.regions.error == AnalyticFilteredRegionsError::invalid_argument
                                ? AnalyticFilteredLineageError::invalid_argument
                                : AnalyticFilteredLineageError::resource_limit_exceeded;
            return std::move(result_);
        }
        try
        {
            if (!prepare() || !validate_coverage())
                return failure();
            counting_ = true;
            if (!build_region_contributors() || !build_boundary_and_vertex_rows() ||
                !preflight_publication())
                return failure();
            counting_ = false;
            if (!build_region_contributors() || !build_boundary_and_vertex_rows() || !publish())
                return failure();
        }
        catch (const std::bad_alloc&)
        {
            result_.error = AnalyticFilteredLineageError::resource_limit_exceeded;
            return failure();
        }
        result_.telemetry.lineage_work_units = work_;
        result_.telemetry.predicate_calls = result_.regions.telemetry.predicate_calls + work_;
        result_.telemetry.emitted_boundary_records = result_.boundaries.size();
        result_.telemetry.emitted_vertex_records = result_.vertices.size();
        result_.telemetry.emitted_region_records = result_.region_lineage.size();
        result_.telemetry.emitted_source_references = result_.source_references.size();
        return std::move(result_);
    }

  private:
    bool charge(std::uint64_t units)
    {
        const std::uint64_t used = result_.regions.telemetry.predicate_calls + work_;
        if (used > limits_.predicate_calls || units > limits_.predicate_calls - used)
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
        const auto region_work = result_.regions.telemetry.predicate_calls;
        const auto region_memory = result_.regions.telemetry.peak_working_memory_bytes;
        const auto fallback = result_.regions.telemetry.algebraic_fallback_calls;
        const auto arrangement_work =
            result_.regions.selection.telemetry.arrangement_predicate_calls;
        result_.regions = {};
        result_.boundaries.clear();
        result_.vertices.clear();
        result_.region_lineage.clear();
        result_.source_references.clear();
        result_.telemetry = {};
        result_.telemetry.regions_work_units = region_work;
        result_.telemetry.regions_peak_working_memory_bytes = region_memory;
        result_.telemetry.arrangement_work_units = arrangement_work;
        result_.telemetry.reserved_lineage_work_units = reserved_work_;
        result_.telemetry.lineage_work_units = work_;
        result_.telemetry.predicate_calls = region_work + work_;
        result_.telemetry.peak_working_memory_bytes = region_memory;
        result_.telemetry.algebraic_fallback_calls = fallback;
        return std::move(result_);
    }
    bool prepare()
    {
        if (job_index_ >= records_.jobs.size())
            return false;
        const auto& job = records_.jobs[job_index_];
        if (job.stage_begin > records_.stages.size() ||
            job.stage_count > records_.stages.size() - job.stage_begin || !charge(job.stage_count))
            return false;
        std::uint64_t operand_count = 0;
        for (std::uint32_t local = 0; local < job.stage_count; ++local)
        {
            const auto& stage = records_.stages[job.stage_begin + local];
            if (stage.stage_id == 0 || (stage.operation != 1 && stage.operation != 2) ||
                stage.operand_begin > records_.operands.size() ||
                stage.operand_count > records_.operands.size() - stage.operand_begin ||
                operand_count > limits_.boundary_occurrences ||
                stage.operand_count > limits_.boundary_occurrences - operand_count)
                return false;
            operand_count += stage.operand_count;
        }
        operands_.reserve(static_cast<std::size_t>(operand_count));
        lookup_.reserve(static_cast<std::size_t>(operand_count));
        stage_begin_.reserve(static_cast<std::size_t>(job.stage_count) + 1);
        if (!charge(job.stage_count + operand_count))
            return false;
        for (std::uint32_t local = 0; local < job.stage_count; ++local)
        {
            const auto& stage = records_.stages[job.stage_begin + local];
            stage_begin_.push_back(static_cast<std::uint32_t>(operands_.size()));
            for (std::uint32_t offset = 0; offset < stage.operand_count; ++offset)
            {
                const auto& operand = records_.operands[stage.operand_begin + offset];
                if (operand.operand_id == 0)
                    return false;
                operands_.push_back({operand.operand_id, stage.stage_id, local, stage.operation});
            }
        }
        stage_begin_.push_back(static_cast<std::uint32_t>(operands_.size()));
        if (!charge(operands_.size() * 2 + sort_units(operands_.size())))
            return false;
        for (std::uint32_t index = 0; index < operands_.size(); ++index)
            lookup_.push_back({operands_[index].id, index});
        std::sort(lookup_.begin(), lookup_.end(),
                  [](const Lookup& left, const Lookup& right) { return left.id < right.id; });
        for (std::uint32_t index = 1; index < lookup_.size(); ++index)
            if (lookup_[index - 1].id == lookup_[index].id)
                return false;
        occurrence_operands_.reserve(geometry_.occurrences.size());
        operand_occurrence_begin_.assign(operands_.size() + 1, 0);
        bool valid = true;
        const std::uint64_t occurrence_work = analytic_selection_detail::checked_multiply(
            geometry_.occurrences.size(),
            analytic_selection_detail::tree_operation_units(operands_.size()), valid);
        if (!valid || !charge(occurrence_work))
            return false;
        for (std::uint32_t index = 0; index < geometry_.occurrences.size(); ++index)
        {
            const auto& occurrence = geometry_.occurrences[index];
            if (index >= geometry_.curves.size() ||
                occurrence.occurrence_id != static_cast<std::uint64_t>(index) + 1 ||
                occurrence.source.operand_id != occurrence.coverage_id ||
                !analytic_selection_detail::valid_occurrence_source_for_curve(
                    occurrence.source, geometry_.curves[index].kind))
                return false;
            const auto found = std::lower_bound(
                lookup_.begin(), lookup_.end(), occurrence.coverage_id,
                [](const Lookup& value, std::uint64_t id) { return value.id < id; });
            if (found == lookup_.end() || found->id != occurrence.coverage_id)
                return false;
            occurrence_operands_.push_back(found->ordinal);
            ++operand_occurrence_begin_[found->ordinal + 1];
        }
        if (!charge(operands_.size() * 2 + geometry_.occurrences.size()))
            return false;
        for (std::uint32_t i = 1; i < operand_occurrence_begin_.size(); ++i)
            operand_occurrence_begin_[i] += operand_occurrence_begin_[i - 1];
        ordered_occurrences_.resize(geometry_.occurrences.size());
        auto cursor = operand_occurrence_begin_;
        for (std::uint32_t i = 0; i < occurrence_operands_.size(); ++i)
            ordered_occurrences_[cursor[occurrence_operands_[i]]++] = i;
        return true;
    }
    bool validate_coverage()
    {
        const auto& nodes = result_.regions.selection.coverage_state_nodes;
        if (!charge(nodes.size()) || nodes.size() < 2 || nodes[0].left != 0 ||
            nodes[0].right != 0 || nodes[1].left != kNoIndex || nodes[1].right != kNoIndex)
            return false;
        for (std::uint32_t i = 2; i < nodes.size(); ++i)
            if (nodes[i].left >= i || nodes[i].right >= i ||
                (nodes[i].left == 0 && nodes[i].right == 0))
                return false;
        leaf_capacity_ = 1;
        while (leaf_capacity_ < std::max<std::uint32_t>(1, operands_.size()))
            leaf_capacity_ <<= 1U;
        return true;
    }
    template <typename Emit>
    bool enumerate(std::uint32_t root, std::uint32_t begin, std::uint32_t width,
                   std::uint32_t first, std::uint32_t end, Emit&& emit)
    {
        if (!charge(1))
            return false;
        ++result_.telemetry.coverage_node_visits;
        const auto& nodes = result_.regions.selection.coverage_state_nodes;
        if (root == 0 || begin >= end || begin + width <= first)
            return true;
        if (root >= nodes.size() || (root == 1 && width != 1))
            return false;
        if (width == 1)
            return begin < operands_.size() && emit(begin);
        const std::uint32_t half = width / 2;
        return enumerate(nodes[root].left, begin, half, first, end, emit) &&
               enumerate(nodes[root].right, begin + half, half, first, end, emit);
    }
    bool emit_region_operand(std::uint32_t region, std::uint32_t operand)
    {
        if (seen_operand_[operand] == region + 1)
            return true;
        seen_operand_[operand] = region + 1;
        if (!charge(1))
            return false;
        for (std::uint32_t at = operand_occurrence_begin_[operand];
             at < operand_occurrence_begin_[operand + 1]; ++at)
            if (!append(OwnerKind::region, region,
                        geometry_.occurrences[ordered_occurrences_[at]].source))
                return false;
        return true;
    }
    bool build_region_contributors()
    {
        const auto& regions = result_.regions;
        const auto& selection = regions.selection;
        const auto& arrangement = selection.arrangement;
        const std::uint32_t face_count = static_cast<std::uint32_t>(selection.faces.size());
        std::vector<std::uint32_t> left(arrangement.edges.size(), kNoIndex),
            right(arrangement.edges.size(), kNoIndex);
        bool valid = true;
        std::uint64_t structural_work =
            analytic_selection_detail::checked_multiply(arrangement.edges.size(), 4, valid);
        structural_work = analytic_selection_detail::checked_add(
            structural_work, arrangement.half_edges.size(), valid);
        structural_work = analytic_selection_detail::checked_add(
            structural_work, analytic_selection_detail::checked_multiply(face_count, 3, valid),
            valid);
        structural_work =
            analytic_selection_detail::checked_add(structural_work, regions.regions.size(), valid);
        if (!valid || !charge(structural_work))
            return false;
        for (std::uint32_t h = 0; h < arrangement.half_edges.size(); ++h)
        {
            const auto& half = arrangement.half_edges[h];
            if (half.edge >= left.size() || h >= selection.half_edge_faces.size())
                return false;
            (half.forward ? left[half.edge] : right[half.edge]) = selection.half_edge_faces[h];
        }
        std::vector<std::uint32_t> counts(face_count + 1, 0);
        for (std::uint32_t edge = 0; edge < arrangement.edges.size(); ++edge)
            if (left[edge] < face_count && right[edge] < face_count &&
                selection.faces[left[edge]].material && selection.faces[right[edge]].material &&
                regions.face_components[left[edge]] == regions.face_components[right[edge]])
            {
                ++counts[left[edge] + 1];
                ++counts[right[edge] + 1];
            }
        for (std::uint32_t i = 1; i < counts.size(); ++i)
            counts[i] += counts[i - 1];
        std::vector<Adjacency> adjacency(counts.back());
        auto cursor = counts;
        for (std::uint32_t edge = 0; edge < arrangement.edges.size(); ++edge)
            if (left[edge] < face_count && right[edge] < face_count &&
                selection.faces[left[edge]].material && selection.faces[right[edge]].material &&
                regions.face_components[left[edge]] == regions.face_components[right[edge]])
            {
                adjacency[cursor[left[edge]]++] = {right[edge], edge};
                adjacency[cursor[right[edge]]++] = {left[edge], edge};
            }
        std::vector<std::uint32_t> seed(selection.faces.size(), kNoIndex);
        for (std::uint32_t face = 0; face < face_count; ++face)
            if (selection.faces[face].material)
                seed[regions.face_components[face]] = face;
        std::vector<std::uint8_t> visited(face_count, 0);
        std::vector<std::uint32_t> stack;
        stack.reserve(face_count);
        seen_operand_.assign(operands_.size(), 0);
        for (std::uint32_t region = 0; region < regions.regions.size(); ++region)
        {
            const std::uint32_t component = regions.regions[region].material_component;
            if (component >= seed.size() || seed[component] == kNoIndex)
                return false;
            const std::uint32_t first = seed[component];
            std::uint32_t minimum_stage = selection.faces[first].positive_stage_begin;
            if (minimum_stage >= stage_begin_.size() ||
                !enumerate(selection.faces[first].coverage_state_root, 0, leaf_capacity_,
                           stage_begin_[minimum_stage], operands_.size(), [&](std::uint32_t operand)
                           { return emit_region_operand(region, operand); }))
                return false;
            visited[first] = 1;
            stack.push_back(first);
            while (!stack.empty())
            {
                const std::uint32_t face = stack.back();
                stack.pop_back();
                for (std::uint32_t at = counts[face]; at < counts[face + 1]; ++at)
                {
                    ++result_.telemetry.component_transition_visits;
                    const Adjacency next = adjacency[at];
                    if (visited[next.neighbor])
                        continue;
                    visited[next.neighbor] = 1;
                    stack.push_back(next.neighbor);
                    const auto& next_face = selection.faces[next.neighbor];
                    if (next_face.positive_stage_begin < minimum_stage)
                    {
                        if (!enumerate(next_face.coverage_state_root, 0, leaf_capacity_,
                                       stage_begin_[next_face.positive_stage_begin],
                                       stage_begin_[minimum_stage], [&](std::uint32_t operand)
                                       { return emit_region_operand(region, operand); }))
                            return false;
                        minimum_stage = next_face.positive_stage_begin;
                    }
                    const auto& edge = arrangement.edges[next.edge];
                    for (std::uint32_t m = 0; m < edge.membership_count; ++m)
                    {
                        if (!charge(1))
                            return false;
                        const auto& membership = arrangement.memberships[edge.membership_begin + m];
                        if (membership.curve_index == 0 ||
                            membership.curve_index > occurrence_operands_.size())
                            return false;
                        const std::uint32_t operand =
                            occurrence_operands_[membership.curve_index - 1];
                        const std::uint32_t covered =
                            membership.material_on_span_left ? left[next.edge] : right[next.edge];
                        if (covered == next.neighbor &&
                            operands_[operand].stage >= next_face.positive_stage_begin &&
                            !emit_region_operand(region, operand))
                            return false;
                    }
                }
            }
        }
        return true;
    }
    bool append(OwnerKind kind, std::uint32_t owner, const AnalyticFilteredSourceReference& source)
    {
        if (!charge(1) || raw_count_ == limits_.source_reference_memberships)
        {
            result_.error = AnalyticFilteredLineageError::resource_limit_exceeded;
            return false;
        }
        ++raw_count_;
        if (!counting_)
            raw_.push_back({kind, owner, source});
        return true;
    }
    bool build_boundary_and_vertex_rows()
    {
        const auto& regions = result_.regions;
        const auto& selection = regions.selection;
        const auto& arrangement = selection.arrangement;
        bool valid = true;
        std::uint64_t structural_work =
            analytic_selection_detail::checked_multiply(regions.ring_half_edges.size(), 2, valid);
        structural_work = analytic_selection_detail::checked_add(
            structural_work,
            analytic_selection_detail::checked_multiply(arrangement.vertices.size(), 2, valid),
            valid);
        structural_work = analytic_selection_detail::checked_add(
            structural_work,
            analytic_selection_detail::checked_multiply(arrangement.edges.size(), 3, valid), valid);
        structural_work = analytic_selection_detail::checked_add(
            structural_work, arrangement.collapsed_spans.size(), valid);
        structural_work =
            analytic_selection_detail::checked_add(structural_work, regions.regions.size(), valid);
        if (!valid || !charge(structural_work))
            return false;
        if (!counting_)
            result_.boundaries.resize(regions.ring_half_edges.size());
        for (std::uint32_t owner = 0; owner < regions.ring_half_edges.size(); ++owner)
        {
            const std::uint32_t h = regions.ring_half_edges[owner];
            if (h >= arrangement.half_edges.size())
                return false;
            const auto& half = arrangement.half_edges[h];
            const std::uint32_t material_face = selection.half_edge_faces[h];
            const std::uint32_t empty_face = selection.half_edge_faces[half.twin];
            const auto& edge = arrangement.edges[half.edge];
            for (std::uint32_t m = 0; m < edge.membership_count; ++m)
            {
                if (!charge(1))
                    return false;
                ++result_.telemetry.boundary_membership_visits;
                const auto& membership = arrangement.memberships[edge.membership_begin + m];
                if (membership.curve_index == 0 ||
                    membership.curve_index > occurrence_operands_.size())
                    return false;
                const std::uint32_t operand = occurrence_operands_[membership.curve_index - 1];
                if (operands_[operand].operation == 1 &&
                    membership.material_on_span_left == half.forward &&
                    operands_[operand].stage >=
                        selection.faces[material_face].positive_stage_begin &&
                    !append(OwnerKind::boundary_positive, owner,
                            geometry_.occurrences[membership.curve_index - 1].source))
                    return false;
            }
            const std::uint32_t removal = selection.faces[empty_face].active_removal_stage;
            if (removal != kNoIndex)
            {
                if (removal + 1 >= stage_begin_.size() ||
                    !enumerate(selection.faces[empty_face].coverage_state_root, 0, leaf_capacity_,
                               stage_begin_[removal], stage_begin_[removal + 1],
                               [&](std::uint32_t operand)
                               {
                                   return append(
                                       OwnerKind::boundary_subtraction, owner,
                                       {AnalyticFilteredSourceKind::subtractive_operand_effect,
                                        AnalyticFilteredSourceRole::none, operands_[operand].id,
                                        operands_[operand].stage_id, 0});
                               }))
                    return false;
            }
        }
        std::vector<std::uint8_t> retained(arrangement.vertices.size(), 0);
        for (const std::uint32_t h : regions.ring_half_edges)
            retained[arrangement.half_edges[h].origin_vertex] = 1;
        std::vector<std::uint32_t> owner_by_vertex(arrangement.vertices.size(), kNoIndex);
        std::uint32_t vertex_count = 0;
        for (std::uint32_t v = 0; v < retained.size(); ++v)
            if (retained[v])
            {
                owner_by_vertex[v] = vertex_count++;
                if (!counting_)
                    result_.vertices.push_back({v, {}});
            }
        for (const auto& edge : arrangement.edges)
            for (const std::uint32_t vertex : {edge.start_vertex, edge.end_vertex})
                if (owner_by_vertex[vertex] != kNoIndex)
                    for (std::uint32_t m = 0; m < edge.membership_count; ++m)
                    {
                        ++result_.telemetry.vertex_membership_visits;
                        const auto& membership = arrangement.memberships[edge.membership_begin + m];
                        if (!append(OwnerKind::vertex, owner_by_vertex[vertex],
                                    geometry_.occurrences[membership.curve_index - 1].source))
                            return false;
                    }
        for (const auto& span : arrangement.collapsed_spans)
            if (owner_by_vertex[span.vertex] != kNoIndex)
                for (std::uint32_t m = 0; m < span.membership_count; ++m)
                {
                    ++result_.telemetry.vertex_membership_visits;
                    const auto& membership = arrangement.memberships[span.membership_begin + m];
                    if (!append(OwnerKind::vertex, owner_by_vertex[span.vertex],
                                geometry_.occurrences[membership.curve_index - 1].source))
                        return false;
                }
        if (!counting_)
            result_.region_lineage.resize(regions.regions.size());
        return true;
    }
    bool preflight_publication()
    {
        expected_raw_count_ = raw_count_;
        if (expected_raw_count_ > limits_.source_reference_memberships ||
            expected_raw_count_ > limits_.provenance_references)
        {
            result_.error = AnalyticFilteredLineageError::resource_limit_exceeded;
            return false;
        }
        const auto& regions = result_.regions;
        const auto& selection = regions.selection;
        const auto& arrangement = selection.arrangement;
        bool valid = true;
        using analytic_selection_detail::checked_add;
        using analytic_selection_detail::checked_multiply;
        std::uint64_t retained = checked_multiply(arrangement.vertices.size(),
                                                  kAnalyticArrangementVertexLogicalBytes, valid);
        retained = checked_add(
            retained,
            checked_multiply(arrangement.edges.size(), kAnalyticArrangementEdgeLogicalBytes, valid),
            valid);
        retained = checked_add(retained,
                               checked_multiply(arrangement.half_edges.size(),
                                                kAnalyticArrangementHalfEdgeLogicalBytes, valid),
                               valid);
        retained =
            checked_add(retained,
                        checked_multiply(arrangement.collapsed_spans.size(),
                                         kAnalyticArrangementCollapsedSpanLogicalBytes, valid),
                        valid);
        retained = checked_add(retained,
                               checked_multiply(arrangement.memberships.size(),
                                                kAnalyticOverlayMembershipLogicalBytes, valid),
                               valid);
        retained = checked_add(retained,
                               checked_multiply(arrangement.cycles.size(),
                                                kAnalyticArrangementCycleLogicalBytes, valid),
                               valid);
        retained = checked_add(
            retained,
            checked_multiply(arrangement.cycle_half_edges.size(), kIndexLogicalBytes, valid),
            valid);
        retained =
            checked_add(retained,
                        checked_multiply(selection.occurrences.size(),
                                         analytic_selection_detail::kOccurrenceLogicalBytes, valid),
                        valid);
        retained = checked_add(
            retained, checked_multiply(selection.half_edge_faces.size(), kIndexLogicalBytes, valid),
            valid);
        retained =
            checked_add(retained,
                        checked_multiply(selection.faces.size(),
                                         analytic_selection_detail::kFaceLogicalBytes, valid),
                        valid);
        retained = checked_add(
            retained,
            checked_multiply(selection.face_boundary_cycles.size(), kIndexLogicalBytes, valid),
            valid);
        retained = checked_add(
            retained,
            checked_multiply(selection.coverage_state_nodes.size(),
                             analytic_selection_detail::kCoverageNodeLogicalBytes, valid),
            valid);
        retained = checked_add(
            retained,
            checked_multiply(regions.rings.size(),
                             analytic_selection_detail::kMaterialRingLogicalBytes, valid),
            valid);
        retained = checked_add(
            retained, checked_multiply(regions.ring_half_edges.size(), kIndexLogicalBytes, valid),
            valid);
        retained = checked_add(
            retained,
            checked_multiply(regions.regions.size(),
                             analytic_selection_detail::kMaterialRegionLogicalBytes, valid),
            valid);
        retained = checked_add(
            retained, checked_multiply(regions.face_components.size(), kIndexLogicalBytes, valid),
            valid);

        std::uint64_t persistent = checked_multiply(operands_.size(), kOperandLogicalBytes, valid);
        persistent = checked_add(
            persistent, checked_multiply(lookup_.size(), kLookupLogicalBytes, valid), valid);
        persistent = checked_add(
            persistent, checked_multiply(stage_begin_.size(), kIndexLogicalBytes, valid), valid);
        persistent = checked_add(
            persistent, checked_multiply(occurrence_operands_.size(), kIndexLogicalBytes, valid),
            valid);
        persistent = checked_add(
            persistent,
            checked_multiply(operand_occurrence_begin_.size(), kIndexLogicalBytes, valid), valid);
        persistent = checked_add(
            persistent, checked_multiply(ordered_occurrences_.size(), kIndexLogicalBytes, valid),
            valid);
        persistent = checked_add(
            persistent, checked_multiply(seen_operand_.size(), kIndexLogicalBytes, valid), valid);

        std::uint64_t contributor_scratch = checked_multiply(
            arrangement.edges.size(), kIndexLogicalBytes * 2 + kAdjacencyLogicalBytes * 2, valid);
        contributor_scratch =
            checked_add(contributor_scratch,
                        checked_multiply(selection.faces.size(),
                                         kIndexLogicalBytes * 3 + kByteLogicalBytes, valid),
                        valid);
        std::uint64_t boundary_scratch = checked_multiply(
            arrangement.vertices.size(), kIndexLogicalBytes + kByteLogicalBytes, valid);
        const std::uint64_t traversal_scratch = std::max(contributor_scratch, boundary_scratch);

        std::uint64_t publication = checked_multiply(expected_raw_count_, kRawLogicalBytes, valid);
        publication = checked_add(
            publication, checked_multiply(expected_raw_count_, kSourceLogicalBytes, valid), valid);
        publication = checked_add(
            publication,
            checked_multiply(regions.ring_half_edges.size(), kBoundaryLogicalBytes, valid), valid);
        publication = checked_add(
            publication, checked_multiply(arrangement.vertices.size(), kVertexLogicalBytes, valid),
            valid);
        publication = checked_add(
            publication, checked_multiply(regions.regions.size(), kRegionLogicalBytes, valid),
            valid);
        const std::uint64_t preparation_scratch =
            checked_multiply(operand_occurrence_begin_.size(), kIndexLogicalBytes, valid);
        const std::uint64_t phase = checked_add(
            checked_add(retained, persistent, valid),
            std::max(preparation_scratch, checked_add(traversal_scratch, publication, valid)),
            valid);
        if (!valid || phase > limits_.working_memory_bytes)
        {
            result_.error = AnalyticFilteredLineageError::resource_limit_exceeded;
            return false;
        }
        result_.telemetry.peak_working_memory_bytes =
            std::max(result_.regions.telemetry.peak_working_memory_bytes, phase);
        raw_.reserve(static_cast<std::size_t>(expected_raw_count_));
        result_.source_references.reserve(static_cast<std::size_t>(expected_raw_count_));
        result_.boundaries.reserve(regions.ring_half_edges.size());
        result_.vertices.reserve(arrangement.vertices.size());
        result_.region_lineage.reserve(regions.regions.size());
        raw_count_ = 0;
        return true;
    }
    bool publish()
    {
        if (raw_count_ != expected_raw_count_ || raw_.size() != expected_raw_count_)
            return false;
        bool valid = true;
        std::uint64_t traversal_work =
            analytic_selection_detail::checked_multiply(raw_.size(), 2, valid);
        traversal_work = analytic_selection_detail::checked_add(
            traversal_work,
            analytic_selection_detail::checked_multiply(result_.boundaries.size(), 2, valid),
            valid);
        traversal_work = analytic_selection_detail::checked_add(
            traversal_work, result_.vertices.size() + result_.region_lineage.size(), valid);
        if (!valid || !charge(traversal_work))
            return false;
        const std::uint64_t units = sort_units(raw_.size());
        if (!charge(units))
            return false;
        result_.telemetry.sort_work_units = units;
        std::sort(
            raw_.begin(), raw_.end(),
            [](const RawSource& a, const RawSource& b)
            {
                return std::tie(a.kind, a.owner, a.source.kind, a.source.role, a.source.operand_id,
                                a.source.primary_id, a.source.secondary_id) <
                       std::tie(b.kind, b.owner, b.source.kind, b.source.role, b.source.operand_id,
                                b.source.primary_id, b.source.secondary_id);
            });
        raw_.erase(std::unique(raw_.begin(), raw_.end(),
                               [](const RawSource& a, const RawSource& b)
                               {
                                   return a.kind == b.kind && a.owner == b.owner &&
                                          source_key(a.source) == source_key(b.source);
                               }),
                   raw_.end());
        if (raw_.size() > limits_.source_reference_memberships)
            return false;
        std::size_t cursor = 0;
        const auto fill =
            [&](OwnerKind kind, std::uint32_t owner, AnalyticFilteredSourceRange& range)
        {
            range.begin = static_cast<std::uint32_t>(result_.source_references.size());
            while (cursor < raw_.size() && raw_[cursor].kind == kind && raw_[cursor].owner == owner)
                result_.source_references.push_back(raw_[cursor++].source);
            range.count =
                static_cast<std::uint32_t>(result_.source_references.size()) - range.begin;
        };
        for (std::uint32_t i = 0; i < result_.boundaries.size(); ++i)
        {
            result_.boundaries[i].half_edge = result_.regions.ring_half_edges[i];
            fill(OwnerKind::boundary_positive, i, result_.boundaries[i].positive);
        }
        for (std::uint32_t i = 0; i < result_.boundaries.size(); ++i)
            fill(OwnerKind::boundary_subtraction, i, result_.boundaries[i].subtraction);
        for (std::uint32_t i = 0; i < result_.vertices.size(); ++i)
            fill(OwnerKind::vertex, i, result_.vertices[i].intersection);
        for (std::uint32_t i = 0; i < result_.region_lineage.size(); ++i)
        {
            result_.region_lineage[i].region = i;
            fill(OwnerKind::region, i, result_.region_lineage[i].positive_contributors);
        }
        if (cursor != raw_.size() ||
            result_.source_references.size() > limits_.provenance_references)
            return false;
        return true;
    }

    const AnalyticRequestPacketRecords& records_;
    std::uint32_t job_index_;
    const AnalyticFilteredGeometry& geometry_;
    const std::vector<AnalyticCurvePair>& pairs_;
    const AnalyticSolverLimits& limits_;
    AnalyticFilteredLineageResult result_;
    std::vector<Operand> operands_;
    std::vector<Lookup> lookup_;
    std::vector<std::uint32_t> stage_begin_;
    std::vector<std::uint32_t> occurrence_operands_;
    std::vector<std::uint32_t> operand_occurrence_begin_;
    std::vector<std::uint32_t> ordered_occurrences_;
    std::vector<std::uint32_t> seen_operand_;
    std::vector<RawSource> raw_;
    std::uint32_t leaf_capacity_ = 1;
    std::uint64_t work_ = 0;
    std::uint64_t reserved_work_ = 0;
    std::uint64_t raw_count_ = 0;
    std::uint64_t expected_raw_count_ = 0;
    bool counting_ = false;
};

static_assert(sizeof(Operand) <= kOperandLogicalBytes);
static_assert(sizeof(Lookup) <= kLookupLogicalBytes);
static_assert(sizeof(Adjacency) <= kAdjacencyLogicalBytes);
static_assert(sizeof(RawSource) <= kRawLogicalBytes);
static_assert(sizeof(AnalyticFilteredSourceReference) <= kSourceLogicalBytes);
static_assert(sizeof(AnalyticFilteredBoundaryLineage) <= kBoundaryLogicalBytes);
static_assert(sizeof(AnalyticFilteredVertexLineage) <= kVertexLogicalBytes);
static_assert(sizeof(AnalyticFilteredRegionLineage) <= kRegionLogicalBytes);
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

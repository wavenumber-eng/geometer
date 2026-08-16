#include "geometer/analytic_filtered_outcomes.h"

#include "analytic_filtered_boolean_selection_support.h"
#include "analytic_filtered_capacity.h"
#include "analytic_filtered_lineage_internal.h"
#include "analytic_filtered_outcome_tracker.h"

#include <algorithm>
#include <cstddef>
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
using analytic_selection_detail::checked_add;
using analytic_selection_detail::checked_multiply;
using analytic_selection_detail::kByteLogicalBytes;
using analytic_selection_detail::kCoverageNodeLogicalBytes;
using analytic_selection_detail::kFaceLogicalBytes;
using analytic_selection_detail::kIndexLogicalBytes;
using analytic_selection_detail::kMaterialRegionLogicalBytes;
using analytic_selection_detail::kMaterialRingLogicalBytes;
using analytic_selection_detail::kOccurrenceLogicalBytes;
using analytic_selection_detail::sort_units;
using analytic_selection_detail::tree_operation_units;

constexpr std::uint32_t kNone = std::numeric_limits<std::uint32_t>::max();
constexpr std::uint64_t kStateLogicalBytes = 48;
constexpr std::uint64_t kLookupLogicalBytes = 16;
constexpr std::uint64_t kRawSourceLogicalBytes = 40;
constexpr std::uint64_t kRawReferenceLogicalBytes = 16;
constexpr std::uint64_t kEventLogicalBytes = 32;
constexpr std::uint64_t kTaggedReferenceLogicalBytes = 8;
constexpr std::uint64_t kSourceLogicalBytes = 32;
constexpr std::uint64_t kBoundaryLogicalBytes = 24;
constexpr std::uint64_t kVertexLogicalBytes = 16;
constexpr std::uint64_t kRegionLineageLogicalBytes = 16;

struct State
{
    std::uint64_t operand_id = 0;
    std::uint32_t stage = 0;
    std::uint8_t operation = 0;
    bool final_lineage = false;
    AnalyticFilteredSourceRange sources;
    AnalyticFilteredSourceRange references;
};

struct Lookup
{
    std::uint64_t operand_id = 0;
    std::uint32_t ordinal = 0;
};

struct RawSource
{
    std::uint32_t operand = 0;
    AnalyticFilteredSourceReference source;
};

struct RawReference
{
    std::uint32_t operand = 0;
    AnalyticFilteredTaggedResultReference reference;
};

auto source_key(const AnalyticFilteredSourceReference& value) noexcept
{
    return std::tie(value.kind, value.role, value.operand_id, value.primary_id, value.secondary_id);
}

std::uint64_t retained_lineage_bytes(const AnalyticFilteredLineageResult& lineage,
                                     bool& valid) noexcept
{
    const auto& regions = lineage.regions;
    const auto& selection = regions.selection;
    const auto& arrangement = selection.arrangement;
    std::uint64_t bytes = checked_multiply(arrangement.vertices.size(),
                                           kAnalyticArrangementVertexLogicalBytes, valid);
    bytes = checked_add(
        bytes,
        checked_multiply(arrangement.edges.size(), kAnalyticArrangementEdgeLogicalBytes, valid),
        valid);
    bytes = checked_add(bytes,
                        checked_multiply(arrangement.half_edges.size(),
                                         kAnalyticArrangementHalfEdgeLogicalBytes, valid),
                        valid);
    bytes = checked_add(
        bytes, checked_multiply(arrangement.outgoing_half_edges.size(), kIndexLogicalBytes, valid),
        valid);
    bytes = checked_add(bytes,
                        checked_multiply(arrangement.collapsed_spans.size(),
                                         kAnalyticArrangementCollapsedSpanLogicalBytes, valid),
                        valid);
    bytes = checked_add(bytes,
                        checked_multiply(arrangement.memberships.size(),
                                         kAnalyticOverlayMembershipLogicalBytes, valid),
                        valid);
    bytes = checked_add(
        bytes,
        checked_multiply(arrangement.cycles.size(), kAnalyticArrangementCycleLogicalBytes, valid),
        valid);
    bytes = checked_add(
        bytes, checked_multiply(arrangement.cycle_half_edges.size(), kIndexLogicalBytes, valid),
        valid);
    bytes = checked_add(
        bytes, checked_multiply(selection.occurrences.size(), kOccurrenceLogicalBytes, valid),
        valid);
    bytes = checked_add(
        bytes, checked_multiply(selection.half_edge_faces.size(), kIndexLogicalBytes, valid),
        valid);
    bytes = checked_add(bytes, checked_multiply(selection.faces.size(), kFaceLogicalBytes, valid),
                        valid);
    bytes = checked_add(
        bytes, checked_multiply(selection.face_boundary_cycles.size(), kIndexLogicalBytes, valid),
        valid);
    bytes = checked_add(
        bytes,
        checked_multiply(selection.coverage_state_nodes.size(), kCoverageNodeLogicalBytes, valid),
        valid);
    bytes = checked_add(bytes,
                        checked_multiply(selection.outcome_evidence.size(),
                                         analytic_selection_detail::kOutcomeEvidenceLogicalBytes,
                                         valid),
                        valid);
    bytes = checked_add(
        bytes, checked_multiply(regions.rings.size(), kMaterialRingLogicalBytes, valid), valid);
    bytes = checked_add(
        bytes, checked_multiply(regions.ring_half_edges.size(), kIndexLogicalBytes, valid), valid);
    bytes = checked_add(
        bytes, checked_multiply(regions.regions.size(), kMaterialRegionLogicalBytes, valid), valid);
    bytes = checked_add(
        bytes, checked_multiply(regions.face_components.size(), kIndexLogicalBytes, valid), valid);
    bytes = checked_add(
        bytes, checked_multiply(lineage.boundaries.size(), kBoundaryLogicalBytes, valid), valid);
    bytes = checked_add(
        bytes, checked_multiply(lineage.vertices.size(), kVertexLogicalBytes, valid), valid);
    bytes = checked_add(
        bytes, checked_multiply(lineage.region_lineage.size(), kRegionLineageLogicalBytes, valid),
        valid);
    bytes = checked_add(
        bytes, checked_multiply(lineage.source_references.size(), kSourceLogicalBytes, valid),
        valid);
    return bytes;
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

    AnalyticFilteredOutcomesResult build()
    {
        result_.lineage = analytic_lineage_detail::build_lineage_for_outcomes(
            records_, job_index_, geometry_, pairs_, limits_);
        const auto& upstream = result_.lineage.telemetry;
        result_.telemetry.lineage_work_units = upstream.predicate_calls;
        result_.telemetry.lineage_peak_working_memory_bytes = upstream.peak_working_memory_bytes;
        result_.telemetry.arrangement_work_units = upstream.arrangement_work_units;
        result_.telemetry.reserved_outcomes_work_units = upstream.reserved_outcomes_work_units;
        result_.telemetry.predicate_calls = upstream.predicate_calls;
        result_.telemetry.peak_working_memory_bytes = upstream.peak_working_memory_bytes;
        result_.telemetry.algebraic_fallback_calls = upstream.algebraic_fallback_calls;
        if (result_.lineage.error != AnalyticFilteredLineageError::none)
        {
            result_.error = result_.lineage.error == AnalyticFilteredLineageError::invalid_argument
                                ? AnalyticFilteredOutcomesError::invalid_argument
                                : AnalyticFilteredOutcomesError::resource_limit_exceeded;
            return std::move(result_);
        }
        try
        {
            if (!prepare_states() || !collect_counts() || !preflight_publication() ||
                !fill_sources() || !fill_references() || !publish_events())
                return failure();
        }
        catch (const std::bad_alloc&)
        {
            result_.error = AnalyticFilteredOutcomesError::resource_limit_exceeded;
            return failure();
        }
        result_.telemetry.outcome_work_units = work_;
        result_.telemetry.predicate_calls = result_.lineage.telemetry.predicate_calls + work_;
        result_.telemetry.emitted_events = result_.events.size();
        result_.telemetry.emitted_result_references = result_.result_references.size();
        result_.telemetry.emitted_source_references = result_.source_references.size();
        return std::move(result_);
    }

  private:
    bool charge(std::uint64_t units)
    {
        const std::uint64_t upstream = result_.lineage.telemetry.predicate_calls;
        if (upstream > limits_.predicate_calls || work_ > limits_.predicate_calls - upstream ||
            units > limits_.predicate_calls - upstream - work_)
        {
            result_.error = AnalyticFilteredOutcomesError::resource_limit_exceeded;
            return false;
        }
        work_ += units;
        return true;
    }

    AnalyticFilteredOutcomesResult failure()
    {
        const std::uint64_t upstream_work = result_.lineage.telemetry.predicate_calls;
        result_.events.clear();
        result_.result_references.clear();
        result_.source_references.clear();
        result_.lineage = {};
        result_.telemetry.outcome_work_units = work_;
        result_.telemetry.predicate_calls = upstream_work + work_;
        return std::move(result_);
    }

    std::uint32_t find_operand(std::uint64_t id)
    {
        const std::uint64_t units = tree_operation_units(lookup_.size());
        if (!charge(units))
            return kNone;
        const auto found = std::lower_bound(lookup_.begin(), lookup_.end(), id,
                                            [](const Lookup& value, std::uint64_t key)
                                            { return value.operand_id < key; });
        return found != lookup_.end() && found->operand_id == id ? found->ordinal : kNone;
    }

    bool prepare_states()
    {
        if (job_index_ >= records_.jobs.size())
            return invalid();
        const AnalyticRequestJobRecord& job = records_.jobs[job_index_];
        if (job.stage_begin > records_.stages.size() ||
            job.stage_count > records_.stages.size() - job.stage_begin)
            return invalid();
        const auto& evidence = result_.lineage.regions.selection.outcome_evidence;
        bool valid = true;
        std::uint64_t operand_count = 0;
        for (std::uint32_t local = 0; local < job.stage_count; ++local)
            operand_count = checked_add(
                operand_count, records_.stages[job.stage_begin + local].operand_count, valid);
        const std::uint64_t retained = retained_lineage_bytes(result_.lineage, valid);
        std::uint64_t ring_indices = result_.lineage.regions.ring_half_edges.size();
        ring_indices = checked_add(ring_indices, result_.lineage.regions.rings.size(), valid);
        ring_indices =
            checked_add(ring_indices,
                        checked_multiply(result_.lineage.regions.regions.size(), 2, valid), valid);
        const std::uint64_t structural = checked_add(
            checked_multiply(operand_count, kStateLogicalBytes + kLookupLogicalBytes, valid),
            checked_multiply(ring_indices, kIndexLogicalBytes, valid), valid);
        if (!valid || operand_count != evidence.size() ||
            checked_add(retained, structural, valid) > limits_.working_memory_bytes)
            return resource();
        if (!charge(job.stage_count + operand_count))
            return false;
        states_.reserve(static_cast<std::size_t>(operand_count));
        lookup_.reserve(static_cast<std::size_t>(operand_count));
        std::uint32_t ordinal = 0;
        for (std::uint32_t local = 0; local < job.stage_count; ++local)
        {
            const AnalyticRequestStageRecord& stage = records_.stages[job.stage_begin + local];
            if (stage.operation != 1 && stage.operation != 2)
                return invalid();
            for (std::uint32_t offset = 0; offset < stage.operand_count; ++offset)
            {
                const AnalyticRequestOperandRecord& operand =
                    records_.operands[stage.operand_begin + offset];
                if (ordinal >= evidence.size() ||
                    operand.operand_id != evidence[ordinal].operand_id)
                    return invalid();
                states_.push_back({operand.operand_id, local, stage.operation});
                lookup_.push_back({operand.operand_id, ordinal++});
            }
        }
        if (!charge(sort_units(lookup_.size())))
            return false;
        result_.telemetry.sort_work_units += sort_units(lookup_.size());
        std::sort(lookup_.begin(), lookup_.end(),
                  [](const Lookup& a, const Lookup& b) { return a.operand_id < b.operand_id; });
        if (std::adjacent_find(lookup_.begin(), lookup_.end(), [](const Lookup& a, const Lookup& b)
                               { return a.operand_id == b.operand_id; }) != lookup_.end())
            return invalid();
        return build_ring_maps();
    }

    bool build_ring_maps()
    {
        const auto& regions = result_.lineage.regions;
        const auto& selection = regions.selection;
        ring_by_boundary_.assign(regions.ring_half_edges.size(), kNone);
        ring_region_.assign(regions.rings.size(), kNone);
        bool valid = true;
        std::uint64_t map_work = checked_add(regions.rings.size(), regions.regions.size(), valid);
        map_work = checked_add(map_work, regions.ring_half_edges.size(), valid);
        map_work =
            checked_add(map_work,
                        checked_multiply(regions.rings.size(),
                                         tree_operation_units(regions.regions.size()), valid),
                        valid);
        if (!valid || !charge(map_work))
            return false;
        std::uint32_t cursor = 0;
        for (std::uint32_t ring = 0; ring < regions.rings.size(); ++ring)
        {
            const auto& value = regions.rings[ring];
            const std::uint64_t end =
                static_cast<std::uint64_t>(value.half_edge_begin) + value.half_edge_count;
            if (value.half_edge_begin != cursor || end > regions.ring_half_edges.size())
                return invalid();
            while (cursor < end)
                ring_by_boundary_[cursor++] = ring;
        }
        if (cursor != regions.ring_half_edges.size() ||
            result_.lineage.boundaries.size() != regions.ring_half_edges.size())
            return invalid();
        std::vector<std::pair<std::uint32_t, std::uint32_t>> by_component;
        by_component.reserve(regions.regions.size());
        for (std::uint32_t region = 0; region < regions.regions.size(); ++region)
            by_component.push_back({regions.regions[region].material_component, region});
        if (!charge(sort_units(by_component.size())))
            return false;
        result_.telemetry.sort_work_units += sort_units(by_component.size());
        std::sort(by_component.begin(), by_component.end());
        if (std::adjacent_find(by_component.begin(), by_component.end(),
                               [](const auto& a, const auto& b)
                               { return a.first == b.first; }) != by_component.end())
            return invalid();
        for (std::uint32_t ring = 0; ring < regions.rings.size(); ++ring)
        {
            const auto& value = regions.rings[ring];
            if (value.half_edge_count == 0 ||
                value.half_edge_begin >= regions.ring_half_edges.size())
                return invalid();
            const std::uint32_t half_edge = regions.ring_half_edges[value.half_edge_begin];
            if (half_edge >= selection.half_edge_faces.size())
                return invalid();
            const std::uint32_t face = selection.half_edge_faces[half_edge];
            if (face >= regions.face_components.size())
                return invalid();
            const std::uint32_t component = regions.face_components[face];
            const auto found = std::lower_bound(by_component.begin(), by_component.end(), component,
                                                [](const auto& value, std::uint32_t key)
                                                { return value.first < key; });
            if (found == by_component.end() || found->first != component)
                return invalid();
            ring_region_[ring] = found->second;
        }
        return true;
    }

    bool collect_counts()
    {
        raw_source_count_ = geometry_.occurrences.size();
        raw_reference_count_ = 0;
        const auto& lineage = result_.lineage;
        for (const auto& region : lineage.region_lineage)
        {
            if (region.positive_contributors.begin > lineage.source_references.size() ||
                region.positive_contributors.count >
                    lineage.source_references.size() - region.positive_contributors.begin)
                return invalid();
            raw_reference_count_ =
                checked_count_add(raw_reference_count_, region.positive_contributors.count);
        }
        for (const auto& boundary : lineage.boundaries)
        {
            if (boundary.subtraction.begin > lineage.source_references.size() ||
                boundary.subtraction.count >
                    lineage.source_references.size() - boundary.subtraction.begin)
                return invalid();
            bool valid = true;
            const std::uint64_t additions = checked_multiply(boundary.subtraction.count, 2, valid);
            if (!valid)
                return resource();
            raw_reference_count_ = checked_count_add(raw_reference_count_, additions);
        }
        if (result_.error != AnalyticFilteredOutcomesError::none)
            return false;
        if (!charge(lineage.region_lineage.size() + lineage.boundaries.size()))
            return false;
        // Mark final contributors without allocating a face/operand matrix.
        for (const auto& region : lineage.region_lineage)
            for (std::uint32_t offset = 0; offset < region.positive_contributors.count; ++offset)
            {
                ++result_.telemetry.lineage_source_visits;
                const auto& source =
                    lineage.source_references[region.positive_contributors.begin + offset];
                const std::uint32_t operand = find_operand(source.operand_id);
                if (operand == kNone)
                    return result_.error == AnalyticFilteredOutcomesError::none ? invalid() : false;
                states_[operand].final_lineage = true;
            }
        const auto& evidence = lineage.regions.selection.outcome_evidence;
        event_count_ = 0;
        if (!charge(states_.size()))
            return false;
        for (std::uint32_t ordinal = 0; ordinal < states_.size(); ++ordinal)
        {
            const State& state = states_[ordinal];
            const auto& facts = evidence[ordinal];
            if (state.operation == 1)
            {
                event_count_ += state.final_lineage ? 1 : 0;
                event_count_ += facts.redundant_or_absorbed ? 1 : 0;
                event_count_ += facts.removed_later && state.final_lineage ? 1 : 0;
                event_count_ += facts.covered_positive_area && !state.final_lineage ? 1 : 0;
                event_count_ += !facts.covered_positive_area ? 1 : 0;
                if (facts.attributed_removal || facts.unfilled_removal || facts.overwritten)
                    return invalid();
            }
            else
            {
                event_count_ += facts.unfilled_removal ? 1 : 0;
                event_count_ += facts.overwritten ? 1 : 0;
                event_count_ += !facts.attributed_removal ? 1 : 0;
                if (facts.covered_positive_area || facts.redundant_or_absorbed ||
                    facts.removed_later || state.final_lineage)
                    return invalid();
            }
        }
        if (event_count_ > states_.size() * 3ULL)
            return invalid();
        return true;
    }

    std::uint64_t checked_count_add(std::uint64_t left, std::uint64_t right)
    {
        bool valid = true;
        const std::uint64_t value = checked_add(left, right, valid);
        if (!valid)
            result_.error = AnalyticFilteredOutcomesError::resource_limit_exceeded;
        return value;
    }

    bool preflight_publication()
    {
        if (event_count_ > limits_.provenance_references ||
            raw_reference_count_ > limits_.provenance_references ||
            raw_source_count_ > limits_.source_reference_memberships)
            return resource();
        bool valid = true;
        const std::uint64_t retained = retained_lineage_bytes(result_.lineage, valid);
        std::uint64_t persistent =
            checked_multiply(states_.size(), kStateLogicalBytes + kLookupLogicalBytes, valid);
        persistent = checked_add(persistent,
                                 checked_multiply(ring_by_boundary_.size() + ring_region_.size(),
                                                  kIndexLogicalBytes, valid),
                                 valid);
        std::uint64_t publication = checked_multiply(
            raw_source_count_, kRawSourceLogicalBytes + kSourceLogicalBytes, valid);
        publication = checked_add(
            publication,
            checked_multiply(raw_reference_count_,
                             kRawReferenceLogicalBytes + kTaggedReferenceLogicalBytes, valid),
            valid);
        publication = checked_add(publication,
                                  checked_multiply(event_count_, kEventLogicalBytes, valid), valid);
        const std::uint64_t phase =
            checked_add(checked_add(retained, persistent, valid), publication, valid);
        std::uint64_t remaining_work = raw_source_count_ + raw_reference_count_ * 2 + event_count_;
        remaining_work = checked_add(remaining_work, states_.size(), valid);
        remaining_work = checked_add(remaining_work, result_.lineage.region_lineage.size(), valid);
        remaining_work = checked_add(remaining_work, result_.lineage.boundaries.size(), valid);
        remaining_work = checked_add(remaining_work, sort_units(raw_source_count_), valid);
        remaining_work = checked_add(remaining_work, sort_units(raw_reference_count_), valid);
        remaining_work = checked_add(remaining_work,
                                     checked_multiply(raw_source_count_ + raw_reference_count_,
                                                      tree_operation_units(states_.size()), valid),
                                     valid);
        const std::uint64_t upstream = result_.lineage.telemetry.predicate_calls;
        if (!valid || phase > limits_.working_memory_bytes || upstream > limits_.predicate_calls ||
            work_ > limits_.predicate_calls - upstream ||
            remaining_work > limits_.predicate_calls - upstream - work_)
            return resource();
        result_.telemetry.peak_working_memory_bytes =
            std::max(result_.lineage.telemetry.peak_working_memory_bytes, phase);
        raw_sources_.reserve(static_cast<std::size_t>(raw_source_count_));
        raw_references_.reserve(static_cast<std::size_t>(raw_reference_count_));
        result_.source_references.reserve(static_cast<std::size_t>(raw_source_count_));
        result_.result_references.reserve(static_cast<std::size_t>(raw_reference_count_));
        result_.events.reserve(static_cast<std::size_t>(event_count_));
        return true;
    }

    bool fill_sources()
    {
        if (!charge(geometry_.occurrences.size()))
            return false;
        for (const auto& occurrence : geometry_.occurrences)
        {
            ++result_.telemetry.operand_source_visits;
            const std::uint32_t operand = find_operand(occurrence.source.operand_id);
            if (operand == kNone)
                return result_.error == AnalyticFilteredOutcomesError::none ? invalid() : false;
            raw_sources_.push_back({operand, occurrence.source});
        }
        const std::uint64_t units = sort_units(raw_sources_.size());
        if (!charge(units))
            return false;
        result_.telemetry.sort_work_units += units;
        std::sort(raw_sources_.begin(), raw_sources_.end(),
                  [](const RawSource& a, const RawSource& b)
                  {
                      return std::tuple_cat(std::tie(a.operand), source_key(a.source)) <
                             std::tuple_cat(std::tie(b.operand), source_key(b.source));
                  });
        raw_sources_.erase(
            std::unique(
                raw_sources_.begin(), raw_sources_.end(), [](const RawSource& a, const RawSource& b)
                { return a.operand == b.operand && source_key(a.source) == source_key(b.source); }),
            raw_sources_.end());
        std::size_t cursor = 0;
        for (std::uint32_t operand = 0; operand < states_.size(); ++operand)
        {
            states_[operand].sources.begin =
                static_cast<std::uint32_t>(result_.source_references.size());
            while (cursor < raw_sources_.size() && raw_sources_[cursor].operand == operand)
                result_.source_references.push_back(raw_sources_[cursor++].source);
            states_[operand].sources.count =
                static_cast<std::uint32_t>(result_.source_references.size()) -
                states_[operand].sources.begin;
            if (states_[operand].sources.count == 0)
                return invalid();
        }
        return cursor == raw_sources_.size();
    }

    bool append_reference(std::uint64_t operand_id, AnalyticFilteredResultReferenceKind kind,
                          std::uint32_t local_index)
    {
        const std::uint32_t operand = find_operand(operand_id);
        if (operand == kNone)
            return result_.error == AnalyticFilteredOutcomesError::none ? invalid() : false;
        raw_references_.push_back({operand, {kind, local_index}});
        return true;
    }

    bool fill_references()
    {
        const auto& lineage = result_.lineage;
        if (!charge(lineage.region_lineage.size() + lineage.boundaries.size()))
            return false;
        for (const auto& region : lineage.region_lineage)
        {
            if (region.region >= lineage.regions.regions.size())
                return invalid();
            for (std::uint32_t offset = 0; offset < region.positive_contributors.count; ++offset)
            {
                ++result_.telemetry.lineage_source_visits;
                const auto& source =
                    lineage.source_references[region.positive_contributors.begin + offset];
                if (!append_reference(source.operand_id,
                                      AnalyticFilteredResultReferenceKind::region, region.region))
                    return false;
            }
        }
        for (std::uint32_t boundary = 0; boundary < lineage.boundaries.size(); ++boundary)
        {
            const std::uint32_t ring = ring_by_boundary_[boundary];
            if (ring == kNone || ring >= ring_region_.size() || ring_region_[ring] == kNone)
                return invalid();
            const auto& range = lineage.boundaries[boundary].subtraction;
            for (std::uint32_t offset = 0; offset < range.count; ++offset)
            {
                ++result_.telemetry.lineage_source_visits;
                const auto& source = lineage.source_references[range.begin + offset];
                if (!append_reference(source.operand_id, AnalyticFilteredResultReferenceKind::ring,
                                      ring) ||
                    !append_reference(source.operand_id,
                                      AnalyticFilteredResultReferenceKind::region,
                                      ring_region_[ring]))
                    return false;
            }
        }
        if (raw_references_.size() != raw_reference_count_)
            return invalid();
        const std::uint64_t units = sort_units(raw_references_.size());
        if (!charge(units))
            return false;
        result_.telemetry.sort_work_units += units;
        std::sort(raw_references_.begin(), raw_references_.end(),
                  [](const RawReference& a, const RawReference& b)
                  {
                      return std::tie(a.operand, a.reference.kind, a.reference.local_index) <
                             std::tie(b.operand, b.reference.kind, b.reference.local_index);
                  });
        raw_references_.erase(std::unique(raw_references_.begin(), raw_references_.end(),
                                          [](const RawReference& a, const RawReference& b)
                                          {
                                              return a.operand == b.operand &&
                                                     a.reference.kind == b.reference.kind &&
                                                     a.reference.local_index ==
                                                         b.reference.local_index;
                                          }),
                              raw_references_.end());
        std::size_t cursor = 0;
        for (std::uint32_t operand = 0; operand < states_.size(); ++operand)
        {
            states_[operand].references.begin =
                static_cast<std::uint32_t>(result_.result_references.size());
            while (cursor < raw_references_.size() && raw_references_[cursor].operand == operand)
                result_.result_references.push_back(raw_references_[cursor++].reference);
            states_[operand].references.count =
                static_cast<std::uint32_t>(result_.result_references.size()) -
                states_[operand].references.begin;
        }
        if (cursor != raw_references_.size())
            return invalid();
        const auto& evidence = result_.lineage.regions.selection.outcome_evidence;
        for (std::uint32_t operand = 0; operand < states_.size(); ++operand)
        {
            if (states_[operand].operation == 1 &&
                (states_[operand].references.count != 0) != states_[operand].final_lineage)
                return invalid();
            if (states_[operand].operation == 2 && states_[operand].references.count != 0 &&
                !evidence[operand].unfilled_removal)
                return invalid();
        }
        return true;
    }

    void add_event(const State& state, AnalyticOperandOutcomeKind kind, bool references)
    {
        result_.events.push_back({state.operand_id, kind,
                                  references ? state.references : AnalyticFilteredSourceRange{},
                                  state.sources});
    }

    bool publish_events()
    {
        const auto& evidence = result_.lineage.regions.selection.outcome_evidence;
        if (!charge(states_.size() + event_count_))
            return false;
        for (const Lookup& entry : lookup_)
        {
            const State& state = states_[entry.ordinal];
            const auto& facts = evidence[entry.ordinal];
            if (state.operation == 1)
            {
                if (state.final_lineage)
                    add_event(state, AnalyticOperandOutcomeKind::contributes_final_material, true);
                if (facts.redundant_or_absorbed)
                    add_event(state, AnalyticOperandOutcomeKind::redundant_or_absorbed_coverage,
                              false);
                if (facts.removed_later && state.final_lineage)
                    add_event(state, AnalyticOperandOutcomeKind::partially_removed_later, false);
                if (facts.covered_positive_area && !state.final_lineage)
                    add_event(state, AnalyticOperandOutcomeKind::completely_removed_later, false);
                if (!facts.covered_positive_area)
                    add_event(state, AnalyticOperandOutcomeKind::no_effect, false);
            }
            else
            {
                if (facts.unfilled_removal)
                    add_event(state, AnalyticOperandOutcomeKind::subtraction_effect_survives, true);
                if (facts.overwritten)
                    add_event(state,
                              AnalyticOperandOutcomeKind::subtraction_effect_overwritten_later,
                              false);
                if (!facts.attributed_removal)
                    add_event(state, AnalyticOperandOutcomeKind::no_effect, false);
            }
        }
        if (result_.events.size() != event_count_)
            return invalid();
        return std::is_sorted(
            result_.events.begin(), result_.events.end(), [](const auto& a, const auto& b)
            { return std::tie(a.operand_id, a.kind) < std::tie(b.operand_id, b.kind); });
    }

    bool invalid()
    {
        result_.error = AnalyticFilteredOutcomesError::invalid_argument;
        return false;
    }

    bool resource()
    {
        result_.error = AnalyticFilteredOutcomesError::resource_limit_exceeded;
        return false;
    }

    const AnalyticRequestPacketRecords& records_;
    std::uint32_t job_index_ = 0;
    const AnalyticFilteredGeometry& geometry_;
    const std::vector<AnalyticCurvePair>& pairs_;
    const AnalyticSolverLimits& limits_;
    AnalyticFilteredOutcomesResult result_;
    std::vector<State> states_;
    std::vector<Lookup> lookup_;
    std::vector<std::uint32_t> ring_by_boundary_;
    std::vector<std::uint32_t> ring_region_;
    std::vector<RawSource> raw_sources_;
    std::vector<RawReference> raw_references_;
    std::uint64_t raw_source_count_ = 0;
    std::uint64_t raw_reference_count_ = 0;
    std::uint64_t event_count_ = 0;
    std::uint64_t work_ = 0;
};

static_assert(sizeof(State) <= kStateLogicalBytes);
static_assert(sizeof(Lookup) <= kLookupLogicalBytes);
static_assert(sizeof(RawSource) <= kRawSourceLogicalBytes);
static_assert(sizeof(RawReference) <= kRawReferenceLogicalBytes);
static_assert(sizeof(AnalyticFilteredOperandOutcomeEvent) <= kEventLogicalBytes);
static_assert(sizeof(AnalyticFilteredTaggedResultReference) <= kTaggedReferenceLogicalBytes);
} // namespace

AnalyticFilteredOutcomesResult
build_analytic_filtered_outcomes(const AnalyticRequestPacketRecords& records,
                                 std::uint32_t job_index, const AnalyticFilteredGeometry& geometry,
                                 const std::vector<AnalyticCurvePair>& candidate_pairs,
                                 const AnalyticSolverLimits& limits)
{
    return Builder(records, job_index, geometry, candidate_pairs, limits).build();
}

} // namespace geometer

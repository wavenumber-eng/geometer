#include "geometer/analytic_result_packet_topology.h"

#include "geometer/exact_arrangement.h"
#include "geometer/exact_boolean_regions.h"
#include "geometer/exact_boolean_stages.h"
#include "geometer/exact_construction.h"

#include <algorithm>
#include <exception>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <vector>

namespace geometer
{
namespace
{

using LayoutError = AnalyticResultPacketLayoutError;
using namespace exact;
constexpr std::uint32_t kNone = std::numeric_limits<std::uint32_t>::max();

LayoutError map_error(Error error)
{
    return error == Error::resource_limit_exceeded ? LayoutError::limit_exceeded
                                                   : LayoutError::invalid_packet;
}

struct Ownership
{
    std::vector<std::uint32_t> region_owner;
    std::vector<std::uint32_t> outer_region;
    std::vector<std::uint32_t> ring_region;
    std::vector<std::uint32_t> ring_owner;
    std::vector<std::vector<std::uint32_t>> rings_by_job;
    std::vector<std::vector<std::uint32_t>> regions_by_job;
};

std::optional<Ownership> derive_ownership(const AnalyticResultPacketRecords& records)
{
    Ownership output;
    output.region_owner.resize(records.regions.size(), kNone);
    output.outer_region.resize(records.rings.size(), kNone);
    output.ring_region.resize(records.rings.size(), kNone);
    output.ring_owner.resize(records.rings.size(), kNone);
    output.rings_by_job.resize(records.job_results.size());
    output.regions_by_job.resize(records.job_results.size());
    for (std::uint32_t job = 0; job < records.job_results.size(); ++job)
    {
        const auto& value = records.job_results[job];
        for (std::uint32_t offset = 0; offset < value.result_region_count; ++offset)
        {
            const std::uint32_t region = value.result_region_begin + offset;
            if (region >= records.regions.size())
                return std::nullopt;
            output.region_owner[region] = job;
            output.regions_by_job[job].push_back(region);
        }
    }
    for (std::uint32_t region = 0; region < records.regions.size(); ++region)
    {
        const std::uint32_t ring = records.regions[region].outer_ring;
        if (ring >= records.rings.size() || output.outer_region[ring] != kNone)
            return std::nullopt;
        output.outer_region[ring] = region;
    }
    for (std::uint32_t ring = 0; ring < records.rings.size(); ++ring)
    {
        const auto& value = records.rings[ring];
        const std::uint32_t component_ring = value.depth % 2 == 0 ? ring : value.parent_ring;
        if (component_ring >= records.rings.size() || output.outer_region[component_ring] == kNone)
            return std::nullopt;
        output.ring_region[ring] = output.outer_region[component_ring];
        output.ring_owner[ring] = output.region_owner[output.ring_region[ring]];
        if (output.ring_owner[ring] == kNone)
            return std::nullopt;
        output.rings_by_job[output.ring_owner[ring]].push_back(ring);
    }
    return output;
}

ExactPoint make_point(ConstructionBuilder& builder, const AnalyticResultVertexRecord& vertex)
{
    return {builder.rational(vertex.x_nm), builder.rational(vertex.y_nm)};
}

bool append_curve(ConstructionBuilder& builder, const AnalyticResultPacketRecords& records,
                  std::uint32_t fragment_index, std::uint64_t coverage_id,
                  std::vector<ExactAtomicCurve>& curves,
                  std::vector<ExactCoverageOccurrence>& coverages)
{
    const auto& fragment = records.fragments[fragment_index];
    const auto& start_value = records.vertices[fragment.start_vertex];
    const auto& end_value = records.vertices[fragment.end_vertex];
    ExactAtomicCurve curve;
    curve.kind =
        fragment.kind == 1 ? ExactAtomicCurveKind::line : ExactAtomicCurveKind::circular_arc;
    curve.start = make_point(builder, start_value);
    curve.end = make_point(builder, end_value);
    const std::uint64_t occurrence = static_cast<std::uint64_t>(fragment_index) + 1;
    if (curve.kind == ExactAtomicCurveKind::line)
    {
        const bool agrees =
            start_value.x_nm < end_value.x_nm ||
            (start_value.x_nm == end_value.x_nm && start_value.y_nm < end_value.y_nm);
        curve.memberships.push_back({occurrence, agrees});
    }
    else
    {
        const bool counterclockwise = fragment.direction == 1;
        const ConstructionNodeId two = builder.rational(2);
        const ConstructionNodeId four = builder.rational(4);
        const ConstructionNodeId radius = builder.rational(fragment.radius_nm);
        const ConstructionNodeId dx = builder.subtract(curve.end.x, curve.start.x);
        const ConstructionNodeId dy = builder.subtract(curve.end.y, curve.start.y);
        const ConstructionNodeId chord_squared =
            builder.sum(builder.square(dx), builder.square(dy));
        const ConstructionNodeId gap =
            builder.subtract(builder.product(four, builder.square(radius)), chord_squared);
        const ConstructionNodeId scale =
            builder.square_root(builder.divide(gap, builder.product(four, chord_squared)));
        const ConstructionNodeId midpoint_x =
            builder.divide(builder.sum(curve.start.x, curve.end.x), two);
        const ConstructionNodeId midpoint_y =
            builder.divide(builder.sum(curve.start.y, curve.end.y), two);
        const ConstructionNodeId signed_scale =
            counterclockwise != fragment.major_arc ? scale : builder.negate(scale);
        curve.circle = {{builder.subtract(midpoint_x, builder.product(dy, signed_scale)),
                         builder.sum(midpoint_y, builder.product(dx, signed_scale))},
                        radius};
        curve.counterclockwise = counterclockwise;
        curve.major_arc = fragment.major_arc;
        curve.memberships.push_back({occurrence, counterclockwise});
    }
    if (!builder.good())
        return false;
    curves.push_back(std::move(curve));
    coverages.push_back({occurrence, coverage_id, true});
    return true;
}

std::vector<std::uint64_t> expected_ring_key(const AnalyticResultPacketRecords& records,
                                             std::uint32_t ring)
{
    const auto& value = records.rings[ring];
    std::vector<std::uint64_t> key;
    key.reserve(value.fragment_reference_count);
    for (std::uint32_t offset = 0; offset < value.fragment_reference_count; ++offset)
        key.push_back(static_cast<std::uint64_t>(
                          records.fragment_references[value.fragment_reference_begin + offset]) +
                      1);
    std::sort(key.begin(), key.end());
    return key;
}

std::optional<std::vector<std::uint64_t>> replayed_ring_key(const ExactArrangement& arrangement,
                                                            const ExactBooleanRegions& regions,
                                                            std::uint32_t ring)
{
    const auto& value = regions.rings()[ring];
    std::vector<std::uint64_t> key;
    for (std::uint32_t offset = 0; offset < value.half_edge_count; ++offset)
    {
        const std::uint32_t half_edge = regions.ring_half_edges()[value.half_edge_begin + offset];
        const auto& edge = arrangement.edges()[arrangement.half_edges()[half_edge].edge];
        if (edge.membership_count == 0)
            return std::nullopt;
        for (std::uint32_t membership = 0; membership < edge.membership_count; ++membership)
            key.push_back(
                arrangement.memberships()[edge.membership_begin + membership].occurrence_id);
    }
    std::sort(key.begin(), key.end());
    key.erase(std::unique(key.begin(), key.end()), key.end());
    return key;
}

LayoutError compare_replay(const AnalyticResultPacketRecords& records, const Ownership& ownership,
                           std::uint32_t job_index, const ExactArrangement& arrangement,
                           const ExactBooleanRegions& replay)
{
    std::map<std::vector<std::uint64_t>, std::uint32_t> expected;
    for (const std::uint32_t ring : ownership.rings_by_job[job_index])
        if (!expected.emplace(expected_ring_key(records, ring), ring).second)
            return LayoutError::invalid_packet;
    const auto& job = records.job_results[job_index];
    if (replay.rings().size() != ownership.rings_by_job[job_index].size() ||
        replay.regions().size() != job.result_region_count)
        return LayoutError::invalid_packet;
    std::vector<std::uint32_t> replay_to_expected(replay.rings().size(), kNone);
    std::set<std::uint32_t> matched_rings;
    for (std::uint32_t ring = 0; ring < replay.rings().size(); ++ring)
    {
        const auto key = replayed_ring_key(arrangement, replay, ring);
        if (!key)
            return LayoutError::invalid_packet;
        const auto found = expected.find(*key);
        if (found == expected.end() || !matched_rings.insert(found->second).second)
            return LayoutError::invalid_packet;
        replay_to_expected[ring] = found->second;
        const auto& actual = replay.rings()[ring];
        const auto& claimed = records.rings[found->second];
        if (actual.depth != claimed.depth || actual.counterclockwise != (claimed.depth % 2 == 0))
            return LayoutError::invalid_packet;
    }
    for (std::uint32_t ring = 0; ring < replay.rings().size(); ++ring)
    {
        const auto& actual = replay.rings()[ring];
        const std::uint32_t expected_parent =
            actual.parent_ring == kNone ? kNone : replay_to_expected[actual.parent_ring];
        if (records.rings[replay_to_expected[ring]].parent_ring != expected_parent)
            return LayoutError::invalid_packet;
    }
    std::set<std::uint32_t> matched_regions;
    for (const auto& region : replay.regions())
    {
        if (region.positive_source_count != 1)
            return LayoutError::invalid_packet;
        const std::uint64_t source = replay.positive_sources()[region.positive_source_begin];
        if (source == 0 || source > records.regions.size())
            return LayoutError::invalid_packet;
        const std::uint32_t expected_region = static_cast<std::uint32_t>(source - 1);
        if (ownership.region_owner[expected_region] != job_index ||
            !matched_regions.insert(expected_region).second ||
            records.regions[expected_region].outer_ring != replay_to_expected[region.outer_ring])
            return LayoutError::invalid_packet;
    }
    return LayoutError::none;
}

LayoutError validate_job(const AnalyticResultPacketRecords& records, const Ownership& ownership,
                         std::uint32_t job_index)
{
    const auto& job = records.job_results[job_index];
    if (job.result_region_count == 0)
        return LayoutError::none;
    Budget budget({1'000'000'000, 268'435'456, 100'000'000, 100'000'000});
    ConstructionArena arena(budget);
    ConstructionBuilder builder(arena);
    std::vector<ExactAtomicCurve> curves;
    std::vector<ExactCoverageOccurrence> coverages;
    for (const std::uint32_t ring : ownership.rings_by_job[job_index])
    {
        const std::uint64_t coverage_id =
            static_cast<std::uint64_t>(ownership.ring_region[ring]) + 1;
        const auto& value = records.rings[ring];
        for (std::uint32_t offset = 0; offset < value.fragment_reference_count; ++offset)
            if (!append_curve(builder, records,
                              records.fragment_references[value.fragment_reference_begin + offset],
                              coverage_id, curves, coverages))
                return map_error(builder.error());
    }
    ExactArrangementResult arrangement = build_exact_arrangement(arena, curves, coverages);
    if (arrangement.error != Error::none || !arrangement.value)
        return map_error(arrangement.error);
    std::vector<ExactBooleanOperand> operands;
    for (const std::uint32_t region_index : ownership.regions_by_job[job_index])
    {
        const std::uint64_t region = static_cast<std::uint64_t>(region_index) + 1;
        operands.push_back({region, region});
    }
    ExactBooleanSelectionResult selection = evaluate_exact_boolean_stages(
        budget, *arrangement.value, {{1, ExactBooleanStageOperation::union_, std::move(operands)}});
    if (selection.error != Error::none || !selection.value)
        return map_error(selection.error);
    ExactBooleanRegionsResult regions =
        build_exact_boolean_regions(budget, *arrangement.value, *selection.value);
    if (regions.error != Error::none || !regions.value)
        return map_error(regions.error);
    return compare_replay(records, ownership, job_index, *arrangement.value, *regions.value);
}

} // namespace

AnalyticResultPacketLayoutError
validate_analytic_result_packet_topology(const AnalyticResultPacketRecords& records)
{
    try
    {
        if (const LayoutError structural = validate_analytic_result_packet_records(records);
            structural != LayoutError::none)
            return structural;
        const auto ownership = derive_ownership(records);
        if (!ownership)
            return LayoutError::invalid_packet;
        for (std::uint32_t job = 0; job < records.job_results.size(); ++job)
            if (const LayoutError error = validate_job(records, *ownership, job);
                error != LayoutError::none)
                return error;
        return LayoutError::none;
    }
    catch (const std::exception&)
    {
        return LayoutError::limit_exceeded;
    }
}

} // namespace geometer

#include "geometer/analytic_result_packet_standalone.h"

#include "geometer/analytic_result_packet_canonical.h"
#include "geometer/sha256.h"

#include <algorithm>
#include <exception>
#include <limits>
#include <utility>

namespace geometer
{
namespace
{

using LayoutError = AnalyticResultPacketLayoutError;
constexpr std::uint32_t kNone = std::numeric_limits<std::uint32_t>::max();

std::vector<std::uint32_t> dense_map(const std::vector<bool>& selected)
{
    std::vector<std::uint32_t> mapping(selected.size(), kNone);
    std::uint32_t next = 0;
    for (std::uint32_t index = 0; index < selected.size(); ++index)
        if (selected[index])
            mapping[index] = next++;
    return mapping;
}

void mark_handle(std::vector<bool>& selected, std::uint32_t handle)
{
    if (handle != 0)
        selected[handle - 1] = true;
}

std::uint64_t remap_reference(std::uint64_t reference, const std::vector<std::uint32_t>& ring_map,
                              const std::vector<std::uint32_t>& region_map)
{
    const std::uint32_t kind = static_cast<std::uint32_t>(reference >> 32U);
    const std::uint32_t index = static_cast<std::uint32_t>(reference);
    return (static_cast<std::uint64_t>(kind) << 32U) |
           (kind == 1 ? ring_map[index] : region_map[index]);
}

} // namespace

AnalyticStandaloneJobResult
build_analytic_standalone_job(const AnalyticResultPacketRecords& records, std::uint64_t job_id)
{
    try
    {
        AnalyticResultPacketRecordsResult projected =
            canonicalize_analytic_result_packet_records(records);
        if (projected.error != LayoutError::none || !projected.value)
            return {projected.error, std::nullopt};
        const AnalyticResultPacketRecords& input = *projected.value;
        const auto found =
            std::lower_bound(input.job_results.begin(), input.job_results.end(), job_id,
                             [](const AnalyticJobResultRecord& value, std::uint64_t id)
                             { return value.job_id < id; });
        if (found == input.job_results.end() || found->job_id != job_id)
            return {LayoutError::invalid_packet, std::nullopt};
        const AnalyticJobResultRecord& job = *found;

        std::vector<bool> selected_regions(input.regions.size());
        std::vector<bool> selected_events(input.operand_events.size());
        std::vector<bool> selected_rings(input.rings.size());
        for (std::uint32_t offset = 0; offset < job.result_region_count; ++offset)
        {
            const std::uint32_t region = job.result_region_begin + offset;
            selected_regions[region] = true;
            selected_rings[input.regions[region].outer_ring] = true;
        }
        for (std::uint32_t offset = 0; offset < job.operand_event_count; ++offset)
        {
            const std::uint32_t event = job.operand_event_begin + offset;
            selected_events[event] = true;
            const auto& value = input.operand_events[event];
            for (std::uint32_t at = 0; at < value.result_reference_count; ++at)
            {
                const std::uint64_t reference =
                    input.ring_region_references[value.result_reference_begin + at];
                const std::uint32_t kind = static_cast<std::uint32_t>(reference >> 32U);
                const std::uint32_t index = static_cast<std::uint32_t>(reference);
                if (kind == 1)
                    selected_rings[index] = true;
                else
                    selected_regions[index] = true;
            }
        }
        for (std::uint32_t ring = 0; ring < input.rings.size(); ++ring)
            if (input.rings[ring].parent_ring != kNone &&
                selected_rings[input.rings[ring].parent_ring])
                selected_rings[ring] = true;

        std::vector<bool> selected_fragments(input.fragments.size());
        for (std::uint32_t ring = 0; ring < input.rings.size(); ++ring)
            if (selected_rings[ring])
            {
                const auto& value = input.rings[ring];
                for (std::uint32_t offset = 0; offset < value.fragment_reference_count; ++offset)
                    selected_fragments[input.fragment_references[value.fragment_reference_begin +
                                                                 offset]] = true;
            }
        std::vector<bool> selected_vertices(input.vertices.size());
        for (std::uint32_t fragment = 0; fragment < input.fragments.size(); ++fragment)
            if (selected_fragments[fragment])
            {
                selected_vertices[input.fragments[fragment].start_vertex] = true;
                selected_vertices[input.fragments[fragment].end_vertex] = true;
            }
        std::vector<bool> selected_sets(input.source_sets.size());
        for (std::uint32_t vertex = 0; vertex < input.vertices.size(); ++vertex)
            if (selected_vertices[vertex])
                mark_handle(selected_sets, input.vertices[vertex].intersection_source_set);
        for (std::uint32_t fragment = 0; fragment < input.fragments.size(); ++fragment)
            if (selected_fragments[fragment])
            {
                mark_handle(selected_sets, input.fragments[fragment].positive_source_set);
                mark_handle(selected_sets, input.fragments[fragment].subtraction_source_set);
            }
        for (std::uint32_t region = 0; region < input.regions.size(); ++region)
            if (selected_regions[region])
                mark_handle(selected_sets, input.regions[region].positive_source_set);
        for (std::uint32_t event = 0; event < input.operand_events.size(); ++event)
            if (selected_events[event])
                mark_handle(selected_sets, input.operand_events[event].source_set);
        std::vector<bool> selected_sources(input.source_references.size());
        for (std::uint32_t set = 0; set < input.source_sets.size(); ++set)
            if (selected_sets[set])
            {
                const auto& value = input.source_sets[set];
                for (std::uint32_t offset = 0; offset < value.source_reference_index_count;
                     ++offset)
                    selected_sources[input.source_reference_indices
                                         [value.source_reference_index_begin + offset]] = true;
            }

        const auto vertex_map = dense_map(selected_vertices);
        const auto fragment_map = dense_map(selected_fragments);
        const auto ring_map = dense_map(selected_rings);
        const auto region_map = dense_map(selected_regions);
        const auto set_map = dense_map(selected_sets);
        const auto source_map = dense_map(selected_sources);
        AnalyticResultPacketRecords output;
        output.diagnostics.insert(
            output.diagnostics.end(), input.diagnostics.begin() + job.diagnostic_begin,
            input.diagnostics.begin() + job.diagnostic_begin + job.diagnostic_count);
        for (std::uint32_t vertex = 0; vertex < input.vertices.size(); ++vertex)
            if (selected_vertices[vertex])
            {
                auto value = input.vertices[vertex];
                value.id = static_cast<std::uint64_t>(output.vertices.size()) + 1;
                value.intersection_source_set =
                    value.intersection_source_set == 0
                        ? 0
                        : set_map[value.intersection_source_set - 1] + 1;
                output.vertices.push_back(value);
            }
        for (std::uint32_t fragment = 0; fragment < input.fragments.size(); ++fragment)
            if (selected_fragments[fragment])
            {
                auto value = input.fragments[fragment];
                value.id = static_cast<std::uint64_t>(output.fragments.size()) + 1;
                value.start_vertex = vertex_map[value.start_vertex];
                value.end_vertex = vertex_map[value.end_vertex];
                value.positive_source_set =
                    value.positive_source_set == 0 ? 0 : set_map[value.positive_source_set - 1] + 1;
                value.subtraction_source_set = value.subtraction_source_set == 0
                                                   ? 0
                                                   : set_map[value.subtraction_source_set - 1] + 1;
                output.fragments.push_back(value);
            }
        for (std::uint32_t ring = 0; ring < input.rings.size(); ++ring)
            if (selected_rings[ring])
            {
                auto value = input.rings[ring];
                value.id = static_cast<std::uint64_t>(output.rings.size()) + 1;
                value.fragment_reference_begin =
                    static_cast<std::uint32_t>(output.fragment_references.size());
                value.parent_ring =
                    value.parent_ring == kNone ? kNone : ring_map[value.parent_ring];
                for (std::uint32_t offset = 0; offset < value.fragment_reference_count; ++offset)
                    output.fragment_references.push_back(
                        fragment_map[input.fragment_references
                                         [input.rings[ring].fragment_reference_begin + offset]]);
                output.rings.push_back(value);
            }
        for (std::uint32_t region = 0; region < input.regions.size(); ++region)
            if (selected_regions[region])
            {
                auto value = input.regions[region];
                value.id = static_cast<std::uint64_t>(output.regions.size()) + 1;
                value.outer_ring = ring_map[value.outer_ring];
                value.positive_source_set = set_map[value.positive_source_set - 1] + 1;
                output.regions.push_back(value);
            }
        for (std::uint32_t source = 0; source < input.source_references.size(); ++source)
            if (selected_sources[source])
                output.source_references.push_back(input.source_references[source]);
        for (std::uint32_t set = 0; set < input.source_sets.size(); ++set)
            if (selected_sets[set])
            {
                const auto& value = input.source_sets[set];
                const std::uint32_t begin =
                    static_cast<std::uint32_t>(output.source_reference_indices.size());
                for (std::uint32_t offset = 0; offset < value.source_reference_index_count;
                     ++offset)
                    output.source_reference_indices.push_back(
                        source_map[input.source_reference_indices
                                       [value.source_reference_index_begin + offset]]);
                output.source_sets.push_back({begin, value.source_reference_index_count});
            }
        for (std::uint32_t event = 0; event < input.operand_events.size(); ++event)
            if (selected_events[event])
            {
                auto value = input.operand_events[event];
                value.result_reference_begin =
                    value.result_reference_count == 0
                        ? 0
                        : static_cast<std::uint32_t>(output.ring_region_references.size());
                value.source_set = value.source_set == 0 ? 0 : set_map[value.source_set - 1] + 1;
                for (std::uint32_t offset = 0; offset < value.result_reference_count; ++offset)
                    output.ring_region_references.push_back(remap_reference(
                        input.ring_region_references
                            [input.operand_events[event].result_reference_begin + offset],
                        ring_map, region_map));
                output.operand_events.push_back(value);
            }
        auto standalone_job = job;
        standalone_job.diagnostic_begin = 0;
        standalone_job.result_region_begin = 0;
        standalone_job.operand_event_begin = 0;
        output.job_results.push_back(standalone_job);
        AnalyticResultPacketRecordsResult canonical =
            canonicalize_analytic_result_packet_records(output);
        if (canonical.error != LayoutError::none || !canonical.value)
            return {canonical.error, std::nullopt};
        AnalyticResultPacketEncodeResult encoded =
            encode_analytic_result_packet_records(*canonical.value);
        if (encoded.error != LayoutError::none || !encoded.value)
            return {encoded.error, std::nullopt};
        AnalyticStandaloneJob standalone{
            std::move(*canonical.value), std::move(*encoded.value), {}};
        standalone.digest_sha256 = sha256_hex(standalone.bytes.data(), standalone.bytes.size());
        return {LayoutError::none, std::move(standalone)};
    }
    catch (const std::exception&)
    {
        return {LayoutError::limit_exceeded, std::nullopt};
    }
}

} // namespace geometer

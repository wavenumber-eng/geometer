#include "geometer/analytic_result_packet_canonical.h"

#include <algorithm>
#include <exception>
#include <limits>
#include <numeric>
#include <tuple>
#include <utility>

namespace geometer
{
namespace
{

using LayoutError = AnalyticResultPacketLayoutError;
constexpr std::uint32_t kNone = std::numeric_limits<std::uint32_t>::max();

struct Owners
{
    std::vector<std::uint32_t> vertices;
    std::vector<std::uint32_t> fragments;
    std::vector<std::uint32_t> rings;
    std::vector<std::uint32_t> regions;
    std::vector<std::uint32_t> events;
};

struct IncidentKey
{
    std::uint8_t side = 0;
    std::int64_t other_x = 0;
    std::int64_t other_y = 0;
    std::uint8_t kind = 0;
    std::uint8_t direction = 0;
    bool major_arc = false;
    std::uint64_t radius_nm = 0;
};

auto incident_key(const IncidentKey& value)
{
    return std::tuple{value.side,      value.other_x,   value.other_y,  value.kind,
                      value.direction, value.major_arc, value.radius_nm};
}

bool operator<(const IncidentKey& left, const IncidentKey& right)
{
    return incident_key(left) < incident_key(right);
}

bool operator==(const IncidentKey& left, const IncidentKey& right)
{
    return incident_key(left) == incident_key(right);
}

struct VertexKey
{
    std::uint64_t owner_job = 0;
    std::int64_t x = 0;
    std::int64_t y = 0;
    std::vector<IncidentKey> incidents;
    std::uint32_t intersection_source_set = 0;
};

bool vertex_less(const VertexKey& left, const VertexKey& right)
{
    return std::tie(left.owner_job, left.x, left.y, left.incidents, left.intersection_source_set) <
           std::tie(right.owner_job, right.x, right.y, right.incidents,
                    right.intersection_source_set);
}

template <typename Less> bool equivalent(std::uint32_t left, std::uint32_t right, Less less)
{
    return !less(left, right) && !less(right, left);
}

std::size_t least_rotation(const std::vector<std::uint32_t>& values)
{
    const std::size_t count = values.size();
    std::size_t left = 0;
    std::size_t right = 1;
    std::size_t offset = 0;
    while (left < count && right < count && offset < count)
    {
        const std::uint32_t a = values[(left + offset) % count];
        const std::uint32_t b = values[(right + offset) % count];
        if (a == b)
        {
            ++offset;
            continue;
        }
        if (a > b)
        {
            left += offset + 1;
            if (left == right)
                ++left;
        }
        else
        {
            right += offset + 1;
            if (left == right)
                ++right;
        }
        offset = 0;
    }
    return std::min(left, right);
}

Owners build_owners(const AnalyticResultPacketRecords& records)
{
    Owners owners;
    owners.regions.resize(records.regions.size());
    owners.events.resize(records.operand_events.size());
    for (std::uint32_t job = 0; job < records.job_results.size(); ++job)
    {
        const auto& value = records.job_results[job];
        for (std::uint32_t offset = 0; offset < value.result_region_count; ++offset)
            owners.regions[value.result_region_begin + offset] = job;
        for (std::uint32_t offset = 0; offset < value.operand_event_count; ++offset)
            owners.events[value.operand_event_begin + offset] = job;
    }
    std::vector<std::uint32_t> outer_region(records.rings.size(), kNone);
    for (std::uint32_t region = 0; region < records.regions.size(); ++region)
        outer_region[records.regions[region].outer_ring] = region;
    std::vector<std::uint32_t> roots(records.rings.size(), kNone);
    std::vector<std::uint32_t> path;
    owners.rings.resize(records.rings.size());
    for (std::uint32_t ring = 0; ring < records.rings.size(); ++ring)
    {
        std::uint32_t current = ring;
        path.clear();
        while (roots[current] == kNone)
        {
            path.push_back(current);
            if (records.rings[current].parent_ring == kNone)
                break;
            current = records.rings[current].parent_ring;
        }
        const std::uint32_t root = roots[current] == kNone ? current : roots[current];
        for (auto entry = path.rbegin(); entry != path.rend(); ++entry)
            roots[*entry] = root;
        owners.rings[ring] = owners.regions[outer_region[root]];
    }
    owners.fragments.resize(records.fragments.size());
    for (std::uint32_t ring = 0; ring < records.rings.size(); ++ring)
    {
        const auto& value = records.rings[ring];
        for (std::uint32_t offset = 0; offset < value.fragment_reference_count; ++offset)
            owners.fragments[records.fragment_references[value.fragment_reference_begin + offset]] =
                owners.rings[ring];
    }
    owners.vertices.assign(records.vertices.size(), kNone);
    for (std::uint32_t fragment = 0; fragment < records.fragments.size(); ++fragment)
    {
        const auto& value = records.fragments[fragment];
        owners.vertices[value.start_vertex] = owners.fragments[fragment];
        owners.vertices[value.end_vertex] = owners.fragments[fragment];
    }
    return owners;
}

std::uint64_t owner_job_id(const AnalyticResultPacketRecords& records, std::uint32_t owner)
{
    return records.job_results[owner].job_id;
}

LayoutError canonicalize_vertices(AnalyticResultPacketRecords& output, const Owners& owners,
                                  std::vector<std::uint32_t>& old_to_new)
{
    std::vector<VertexKey> keys(output.vertices.size());
    for (std::uint32_t vertex = 0; vertex < output.vertices.size(); ++vertex)
    {
        keys[vertex].owner_job = owner_job_id(output, owners.vertices[vertex]);
        keys[vertex].x = output.vertices[vertex].x_nm;
        keys[vertex].y = output.vertices[vertex].y_nm;
        keys[vertex].intersection_source_set = output.vertices[vertex].intersection_source_set;
    }
    for (const auto& fragment : output.fragments)
    {
        const auto& start = output.vertices[fragment.start_vertex];
        const auto& end = output.vertices[fragment.end_vertex];
        keys[fragment.start_vertex].incidents.push_back({0, end.x_nm, end.y_nm, fragment.kind,
                                                         fragment.direction, fragment.major_arc,
                                                         fragment.radius_nm});
        keys[fragment.end_vertex].incidents.push_back({1, start.x_nm, start.y_nm, fragment.kind,
                                                       fragment.direction, fragment.major_arc,
                                                       fragment.radius_nm});
    }
    for (auto& key : keys)
        std::sort(key.incidents.begin(), key.incidents.end());
    std::vector<std::uint32_t> order(output.vertices.size());
    std::iota(order.begin(), order.end(), 0);
    const auto less = [&keys](std::uint32_t left, std::uint32_t right)
    { return vertex_less(keys[left], keys[right]); };
    std::sort(order.begin(), order.end(), less);
    for (std::size_t index = 1; index < order.size(); ++index)
        if (equivalent(order[index - 1], order[index], less))
            return LayoutError::invalid_packet;
    old_to_new.resize(order.size());
    std::vector<AnalyticResultVertexRecord> sorted;
    sorted.reserve(order.size());
    for (std::uint32_t index = 0; index < order.size(); ++index)
    {
        old_to_new[order[index]] = index;
        auto value = output.vertices[order[index]];
        value.id = static_cast<std::uint64_t>(index) + 1;
        sorted.push_back(value);
    }
    output.vertices = std::move(sorted);
    for (auto& fragment : output.fragments)
    {
        fragment.start_vertex = old_to_new[fragment.start_vertex];
        fragment.end_vertex = old_to_new[fragment.end_vertex];
    }
    return LayoutError::none;
}

LayoutError canonicalize_fragments(AnalyticResultPacketRecords& output, const Owners& owners,
                                   std::vector<std::uint32_t>& old_to_new)
{
    std::vector<std::uint32_t> order(output.fragments.size());
    std::iota(order.begin(), order.end(), 0);
    const auto less = [&output, &owners](std::uint32_t left, std::uint32_t right)
    {
        const auto& a = output.fragments[left];
        const auto& b = output.fragments[right];
        return std::tuple{owner_job_id(output, owners.fragments[left]),
                          a.start_vertex,
                          a.end_vertex,
                          a.kind,
                          a.direction,
                          a.major_arc,
                          a.radius_nm,
                          a.positive_source_set,
                          a.subtraction_source_set} <
               std::tuple{owner_job_id(output, owners.fragments[right]),
                          b.start_vertex,
                          b.end_vertex,
                          b.kind,
                          b.direction,
                          b.major_arc,
                          b.radius_nm,
                          b.positive_source_set,
                          b.subtraction_source_set};
    };
    std::sort(order.begin(), order.end(), less);
    for (std::size_t index = 1; index < order.size(); ++index)
        if (equivalent(order[index - 1], order[index], less))
            return LayoutError::invalid_packet;
    old_to_new.resize(order.size());
    std::vector<AnalyticDirectedFragmentRecord> sorted;
    sorted.reserve(order.size());
    for (std::uint32_t index = 0; index < order.size(); ++index)
    {
        old_to_new[order[index]] = index;
        auto value = output.fragments[order[index]];
        value.id = static_cast<std::uint64_t>(index) + 1;
        sorted.push_back(value);
    }
    output.fragments = std::move(sorted);
    for (auto& reference : output.fragment_references)
        reference = old_to_new[reference];
    return LayoutError::none;
}

struct RingKey
{
    std::uint64_t owner_job = 0;
    std::uint32_t depth = 0;
    std::vector<std::uint32_t> fragments;
    std::uint32_t parent_rank = kNone;
};

bool ring_less(const RingKey& left, const RingKey& right)
{
    return std::tie(left.owner_job, left.depth, left.fragments, left.parent_rank) <
           std::tie(right.owner_job, right.depth, right.fragments, right.parent_rank);
}

LayoutError canonicalize_rings(AnalyticResultPacketRecords& output, const Owners& owners,
                               std::vector<std::uint32_t>& old_to_new)
{
    std::vector<RingKey> keys(output.rings.size());
    std::vector<std::uint32_t> by_depth(output.rings.size());
    std::iota(by_depth.begin(), by_depth.end(), 0);
    std::sort(by_depth.begin(), by_depth.end(), [&output](std::uint32_t left, std::uint32_t right)
              { return output.rings[left].depth < output.rings[right].depth; });
    std::vector<std::uint32_t> ranks(output.rings.size(), kNone);
    std::size_t begin = 0;
    std::uint32_t next_rank = 0;
    while (begin < by_depth.size())
    {
        std::size_t end = begin + 1;
        const std::uint32_t depth = output.rings[by_depth[begin]].depth;
        while (end < by_depth.size() && output.rings[by_depth[end]].depth == depth)
            ++end;
        for (std::size_t at = begin; at < end; ++at)
        {
            const std::uint32_t ring = by_depth[at];
            const auto& value = output.rings[ring];
            auto& key = keys[ring];
            key.owner_job = owner_job_id(output, owners.rings[ring]);
            key.depth = depth;
            key.parent_rank = value.parent_ring == kNone ? kNone : ranks[value.parent_ring];
            key.fragments.assign(
                output.fragment_references.begin() + value.fragment_reference_begin,
                output.fragment_references.begin() + value.fragment_reference_begin +
                    value.fragment_reference_count);
            const std::size_t rotation = least_rotation(key.fragments);
            std::rotate(key.fragments.begin(), key.fragments.begin() + rotation,
                        key.fragments.end());
        }
        std::sort(by_depth.begin() + begin, by_depth.begin() + end,
                  [&keys](std::uint32_t left, std::uint32_t right)
                  { return ring_less(keys[left], keys[right]); });
        for (std::size_t at = begin; at < end; ++at)
        {
            if (at != begin && !ring_less(keys[by_depth[at - 1]], keys[by_depth[at]]) &&
                !ring_less(keys[by_depth[at]], keys[by_depth[at - 1]]))
                return LayoutError::invalid_packet;
            ranks[by_depth[at]] = next_rank++;
        }
        begin = end;
    }
    std::vector<std::uint32_t> order(output.rings.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&keys](std::uint32_t left, std::uint32_t right)
              { return ring_less(keys[left], keys[right]); });
    old_to_new.resize(order.size());
    for (std::uint32_t index = 0; index < order.size(); ++index)
        old_to_new[order[index]] = index;
    std::vector<AnalyticResultRingRecord> sorted;
    std::vector<std::uint32_t> references;
    sorted.reserve(order.size());
    references.reserve(output.fragment_references.size());
    for (std::uint32_t index = 0; index < order.size(); ++index)
    {
        const std::uint32_t old = order[index];
        auto value = output.rings[old];
        value.id = static_cast<std::uint64_t>(index) + 1;
        value.fragment_reference_begin = static_cast<std::uint32_t>(references.size());
        value.fragment_reference_count = static_cast<std::uint32_t>(keys[old].fragments.size());
        value.parent_ring = value.parent_ring == kNone ? kNone : old_to_new[value.parent_ring];
        references.insert(references.end(), keys[old].fragments.begin(), keys[old].fragments.end());
        sorted.push_back(value);
    }
    output.rings = std::move(sorted);
    output.fragment_references = std::move(references);
    return LayoutError::none;
}

LayoutError canonicalize_regions(AnalyticResultPacketRecords& output, const Owners& owners,
                                 const std::vector<std::uint32_t>& ring_map,
                                 std::vector<std::uint32_t>& old_to_new)
{
    for (auto& region : output.regions)
        region.outer_ring = ring_map[region.outer_ring];
    std::vector<std::uint32_t> order(output.regions.size());
    std::iota(order.begin(), order.end(), 0);
    const auto less = [&output, &owners](std::uint32_t left, std::uint32_t right)
    {
        const auto& a = output.regions[left];
        const auto& b = output.regions[right];
        return std::tuple{owner_job_id(output, owners.regions[left]), a.outer_ring,
                          a.positive_source_set} <
               std::tuple{owner_job_id(output, owners.regions[right]), b.outer_ring,
                          b.positive_source_set};
    };
    std::sort(order.begin(), order.end(), less);
    for (std::size_t index = 1; index < order.size(); ++index)
        if (equivalent(order[index - 1], order[index], less))
            return LayoutError::invalid_packet;
    old_to_new.resize(order.size());
    std::vector<AnalyticResultRegionRecord> sorted;
    sorted.reserve(order.size());
    for (std::uint32_t index = 0; index < order.size(); ++index)
    {
        old_to_new[order[index]] = index;
        auto value = output.regions[order[index]];
        value.id = static_cast<std::uint64_t>(index) + 1;
        sorted.push_back(value);
    }
    output.regions = std::move(sorted);
    std::uint32_t cursor = 0;
    for (std::uint32_t job = 0; job < output.job_results.size(); ++job)
    {
        auto& result = output.job_results[job];
        result.result_region_begin = result.result_region_count == 0 ? 0 : cursor;
        cursor += result.result_region_count;
    }
    return LayoutError::none;
}

std::uint64_t remap_reference(std::uint64_t reference, const std::vector<std::uint32_t>& ring_map,
                              const std::vector<std::uint32_t>& region_map)
{
    const std::uint32_t kind = static_cast<std::uint32_t>(reference >> 32U);
    const std::uint32_t index = static_cast<std::uint32_t>(reference);
    return (static_cast<std::uint64_t>(kind) << 32U) |
           (kind == 1 ? ring_map[index] : region_map[index]);
}

LayoutError canonicalize_events(AnalyticResultPacketRecords& output, const Owners& owners,
                                const std::vector<std::uint32_t>& ring_map,
                                const std::vector<std::uint32_t>& region_map)
{
    std::vector<std::vector<std::uint64_t>> references(output.operand_events.size());
    for (std::uint32_t event = 0; event < output.operand_events.size(); ++event)
    {
        const auto& value = output.operand_events[event];
        auto& current = references[event];
        current.reserve(value.result_reference_count);
        for (std::uint32_t offset = 0; offset < value.result_reference_count; ++offset)
            current.push_back(remap_reference(
                output.ring_region_references[value.result_reference_begin + offset], ring_map,
                region_map));
        std::sort(current.begin(), current.end());
    }
    std::vector<std::uint32_t> order(output.operand_events.size());
    std::iota(order.begin(), order.end(), 0);
    const auto less = [&output, &owners, &references](std::uint32_t left, std::uint32_t right)
    {
        const auto& a = output.operand_events[left];
        const auto& b = output.operand_events[right];
        return std::tuple{owner_job_id(output, owners.events[left]), a.operand_id,
                          static_cast<std::uint16_t>(a.kind), references[left], a.source_set} <
               std::tuple{owner_job_id(output, owners.events[right]), b.operand_id,
                          static_cast<std::uint16_t>(b.kind), references[right], b.source_set};
    };
    std::sort(order.begin(), order.end(), less);
    for (std::size_t index = 1; index < order.size(); ++index)
        if (equivalent(order[index - 1], order[index], less))
            return LayoutError::invalid_packet;
    std::vector<AnalyticOperandEventRecord> events;
    std::vector<std::uint64_t> packed_references;
    events.reserve(order.size());
    packed_references.reserve(output.ring_region_references.size());
    for (std::uint32_t old : order)
    {
        auto value = output.operand_events[old];
        value.result_reference_begin =
            references[old].empty() ? 0 : static_cast<std::uint32_t>(packed_references.size());
        value.result_reference_count = static_cast<std::uint32_t>(references[old].size());
        packed_references.insert(packed_references.end(), references[old].begin(),
                                 references[old].end());
        events.push_back(value);
    }
    output.operand_events = std::move(events);
    output.ring_region_references = std::move(packed_references);
    std::uint32_t cursor = 0;
    for (auto& job : output.job_results)
    {
        job.operand_event_begin = job.operand_event_count == 0 ? 0 : cursor;
        cursor += job.operand_event_count;
    }
    return LayoutError::none;
}

void canonicalize_relationships(AnalyticResultPacketRecords& output,
                                const std::vector<std::uint32_t>& region_map)
{
    std::vector<AnalyticRelationshipPairRecord> pairs;
    pairs.reserve(output.relationship_pairs.size());
    for (auto& result : output.relationship_results)
    {
        if (result.pair_count == 0)
        {
            result.pair_begin = 0;
            continue;
        }
        std::vector<AnalyticRelationshipPairRecord> current;
        current.reserve(result.pair_count);
        for (std::uint32_t offset = 0; offset < result.pair_count; ++offset)
        {
            auto value = output.relationship_pairs[result.pair_begin + offset];
            value.left_result_region_id = region_map[value.left_result_region_id - 1] + 1;
            value.right_result_region_id = region_map[value.right_result_region_id - 1] + 1;
            current.push_back(value);
        }
        std::sort(current.begin(), current.end(),
                  [](const auto& left, const auto& right)
                  {
                      return std::tie(left.left_result_region_id, left.right_result_region_id,
                                      left.dimension, left.equality, left.left_contains_right,
                                      left.right_contains_left) <
                             std::tie(right.left_result_region_id, right.right_result_region_id,
                                      right.dimension, right.equality, right.left_contains_right,
                                      right.right_contains_left);
                  });
        result.pair_begin = static_cast<std::uint32_t>(pairs.size());
        pairs.insert(pairs.end(), current.begin(), current.end());
    }
    output.relationship_pairs = std::move(pairs);
}

} // namespace

AnalyticResultPacketRecordsResult
canonicalize_analytic_result_packet_records(const AnalyticResultPacketRecords& records)
{
    if (const LayoutError error = validate_analytic_result_packet_records(records);
        error != LayoutError::none)
        return {error, std::nullopt};
    try
    {
        AnalyticResultPacketRecords output = records;
        const Owners owners = build_owners(records);
        std::vector<std::uint32_t> vertex_map;
        std::vector<std::uint32_t> fragment_map;
        std::vector<std::uint32_t> ring_map;
        std::vector<std::uint32_t> region_map;
        if (const LayoutError error = canonicalize_vertices(output, owners, vertex_map);
            error != LayoutError::none)
            return {error, std::nullopt};
        if (const LayoutError error = canonicalize_fragments(output, owners, fragment_map);
            error != LayoutError::none)
            return {error, std::nullopt};
        if (const LayoutError error = canonicalize_rings(output, owners, ring_map);
            error != LayoutError::none)
            return {error, std::nullopt};
        if (const LayoutError error = canonicalize_regions(output, owners, ring_map, region_map);
            error != LayoutError::none)
            return {error, std::nullopt};
        if (const LayoutError error = canonicalize_events(output, owners, ring_map, region_map);
            error != LayoutError::none)
            return {error, std::nullopt};
        canonicalize_relationships(output, region_map);
        if (const LayoutError error = validate_analytic_result_packet_records(output);
            error != LayoutError::none)
            return {error, std::nullopt};
        return {LayoutError::none, std::move(output)};
    }
    catch (const std::exception&)
    {
        return {LayoutError::limit_exceeded, std::nullopt};
    }
}

} // namespace geometer

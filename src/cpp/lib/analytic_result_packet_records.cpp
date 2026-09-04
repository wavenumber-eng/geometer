#include "geometer/analytic_result_packet_records.h"

#include "geometer/analytic_result_packet_canonical.h"

#include "analytic_result_packet_records_internal.h"
#include "analytic_wide_integer.h"

#include <algorithm>
#include <exception>
#include <limits>
#include <tuple>

namespace geometer
{
namespace
{

using LayoutError = AnalyticResultPacketLayoutError;

struct Reader
{
    const std::uint8_t* data;
    std::uint64_t offset;
    std::uint8_t u8(std::uint64_t at) const
    {
        return data[offset + at];
    }
    std::uint16_t u16(std::uint64_t at) const
    {
        return static_cast<std::uint16_t>(u8(at)) | (static_cast<std::uint16_t>(u8(at + 1)) << 8U);
    }
    std::uint32_t u32(std::uint64_t at) const
    {
        std::uint32_t value = 0;
        for (std::uint32_t index = 0; index < 4; ++index)
            value |= static_cast<std::uint32_t>(u8(at + index)) << (index * 8U);
        return value;
    }
    std::uint64_t u64(std::uint64_t at) const
    {
        std::uint64_t value = 0;
        for (std::uint32_t index = 0; index < 8; ++index)
            value |= static_cast<std::uint64_t>(u8(at + index)) << (index * 8U);
        return value;
    }
    std::int64_t i64(std::uint64_t at) const
    {
        const std::uint64_t value = u64(at);
        if (value <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
            return static_cast<std::int64_t>(value);
        return -1 - static_cast<std::int64_t>(std::numeric_limits<std::uint64_t>::max() - value);
    }
    bool zero(std::uint64_t at, std::uint64_t count) const
    {
        for (std::uint64_t index = 0; index < count; ++index)
            if (u8(at + index) != 0)
                return false;
        return true;
    }
};

void append_u8(std::vector<std::uint8_t>& bytes, std::uint8_t value)
{
    bytes.push_back(value);
}

void append_u16(std::vector<std::uint8_t>& bytes, std::uint16_t value)
{
    for (std::uint32_t index = 0; index < 2; ++index)
        bytes.push_back(static_cast<std::uint8_t>(value >> (index * 8U)));
}

void append_u32(std::vector<std::uint8_t>& bytes, std::uint32_t value)
{
    for (std::uint32_t index = 0; index < 4; ++index)
        bytes.push_back(static_cast<std::uint8_t>(value >> (index * 8U)));
}

void append_u64(std::vector<std::uint8_t>& bytes, std::uint64_t value)
{
    for (std::uint32_t index = 0; index < 8; ++index)
        bytes.push_back(static_cast<std::uint8_t>(value >> (index * 8U)));
}

void append_i64(std::vector<std::uint8_t>& bytes, std::int64_t value)
{
    const std::uint64_t encoded = value >= 0 ? static_cast<std::uint64_t>(value)
                                             : std::numeric_limits<std::uint64_t>::max() -
                                                   static_cast<std::uint64_t>(-(value + 1));
    append_u64(bytes, encoded);
}

void append_zero(std::vector<std::uint8_t>& bytes, std::size_t count)
{
    bytes.insert(bytes.end(), count, 0);
}

bool valid_range(std::uint32_t begin, std::uint32_t count, std::size_t size)
{
    return count == 0 ? begin == 0
                      : begin <= size && count <= size - static_cast<std::size_t>(begin);
}

bool valid_source(const AnalyticSourceReference& source)
{
    if (source.operand_id == 0 || source.primary_id == 0)
        return false;
    using Kind = AnalyticSourceKind;
    using Role = AnalyticSourceRole;
    if (source.kind == Kind::authored_segment_curve)
        return source.secondary_id != 0 &&
               (source.role == Role::authored_line || source.role == Role::authored_circular_arc);
    if (source.kind == Kind::subtractive_operand_effect)
        return source.role == Role::none && source.secondary_id == 0;
    if (source.kind != Kind::compact_feature_role)
        return false;
    const std::uint32_t high = static_cast<std::uint32_t>(source.secondary_id >> 32U);
    const std::uint32_t low = static_cast<std::uint32_t>(source.secondary_id);
    switch (source.role)
    {
    case Role::primitive_outer_circle:
    case Role::primitive_inner_circle:
    case Role::capsule_left_line:
    case Role::capsule_end_cap:
    case Role::capsule_right_line:
    case Role::capsule_start_cap:
        return source.secondary_id == 0;
    case Role::swept_left_offset_line:
    case Role::swept_left_offset_arc:
    case Role::swept_right_offset_line:
    case Role::swept_right_offset_arc:
    case Role::swept_end_cap:
        return high != 0 && low == 0;
    case Role::swept_round_join:
        return high != 0 && low == high + 1U;
    case Role::swept_start_cap:
        return high == 1 && low == 0;
    default:
        return false;
    }
}

auto source_key(const AnalyticSourceReference& source)
{
    return std::tuple{static_cast<std::uint16_t>(source.kind),
                      static_cast<std::uint16_t>(source.role), source.operand_id, source.primary_id,
                      source.secondary_id};
}

std::uint64_t absolute_difference(std::int64_t left, std::int64_t right) noexcept
{
    const auto ordered = [](std::int64_t value)
    { return static_cast<std::uint64_t>(value) ^ (std::uint64_t{1} << 63U); };
    const std::uint64_t a = ordered(left);
    const std::uint64_t b = ordered(right);
    return a >= b ? a - b : b - a;
}

bool valid_diagnostic_code(std::uint32_t code)
{
    return code == 65'539 || code == 65'540 || code == 65'541 || code == 65'543 || code == 65'544 ||
           code == 65'545 || code == 65'546 || code == 65'547 || code == 65'548;
}

template <typename Records, typename Begin, typename Count>
bool gapless_partition(const Records& records, std::size_t total, Begin begin, Count count)
{
    std::size_t cursor = 0;
    for (const auto& record : records)
    {
        const std::uint32_t first = begin(record);
        const std::uint32_t length = count(record);
        if (length == 0)
        {
            if (first != 0)
                return false;
            continue;
        }
        if (first != cursor || !valid_range(first, length, total))
            return false;
        cursor += length;
    }
    return cursor == total;
}

LayoutError validate_sources(const AnalyticResultPacketRecords& records)
{
    for (std::size_t index = 0; index < records.source_references.size(); ++index)
    {
        if (!valid_source(records.source_references[index]) ||
            (index != 0 && !(source_key(records.source_references[index - 1]) <
                             source_key(records.source_references[index]))))
            return LayoutError::invalid_packet;
    }
    if (!gapless_partition(
            records.source_sets, records.source_reference_indices.size(),
            [](const auto& set) { return set.source_reference_index_begin; },
            [](const auto& set) { return set.source_reference_index_count; }))
        return LayoutError::invalid_packet;
    std::vector<bool> used(records.source_references.size());
    std::vector<std::uint32_t> previous;
    for (const auto& set : records.source_sets)
    {
        if (set.source_reference_index_count == 0)
            return LayoutError::invalid_packet;
        std::vector<std::uint32_t> current;
        for (std::uint32_t offset = 0; offset < set.source_reference_index_count; ++offset)
        {
            const std::uint32_t source =
                records.source_reference_indices[set.source_reference_index_begin + offset];
            if (source >= records.source_references.size() ||
                (!current.empty() && current.back() >= source))
                return LayoutError::invalid_packet;
            current.push_back(source);
            used[source] = true;
        }
        if (!previous.empty() && !std::lexicographical_compare(previous.begin(), previous.end(),
                                                               current.begin(), current.end()))
            return LayoutError::invalid_packet;
        previous = std::move(current);
    }
    return std::all_of(used.begin(), used.end(), [](bool value) { return value; })
               ? LayoutError::none
               : LayoutError::invalid_packet;
}

LayoutError validate_geometry(const AnalyticResultPacketRecords& records)
{
    const std::uint32_t set_count = static_cast<std::uint32_t>(records.source_sets.size());
    std::vector<bool> used_vertices(records.vertices.size());
    for (std::size_t index = 0; index < records.vertices.size(); ++index)
        if (records.vertices[index].id != index + 1 || records.vertices[index].flags > 1 ||
            records.vertices[index].flags !=
                (records.vertices[index].intersection_source_set == 0 ? 0U : 1U) ||
            records.vertices[index].intersection_source_set > set_count)
            return LayoutError::invalid_packet;
    for (std::size_t index = 0; index < records.fragments.size(); ++index)
    {
        const auto& fragment = records.fragments[index];
        if (fragment.id != index + 1 || fragment.start_vertex >= records.vertices.size() ||
            fragment.end_vertex >= records.vertices.size() ||
            fragment.start_vertex == fragment.end_vertex ||
            fragment.positive_source_set > set_count || fragment.subtraction_source_set > set_count)
            return LayoutError::invalid_packet;
        const bool line = fragment.kind == 1 && fragment.direction == 0 && !fragment.major_arc &&
                          fragment.radius_nm == 0;
        const bool arc = fragment.kind == 2 &&
                         (fragment.direction == 1 || fragment.direction == 2) &&
                         fragment.radius_nm != 0 && fragment.radius_nm <= 1'000'000'000'000ULL;
        if (!line && !arc)
            return LayoutError::invalid_packet;
        if (arc)
        {
            const auto& start = records.vertices[fragment.start_vertex];
            const auto& end = records.vertices[fragment.end_vertex];
            const std::uint64_t dx = absolute_difference(start.x_nm, end.x_nm);
            const std::uint64_t dy = absolute_difference(start.y_nm, end.y_nm);
            const std::uint64_t diameter = fragment.radius_nm * 2;
            if (dx > diameter || dy > diameter)
                return LayoutError::invalid_packet;
            const auto chord_squared = analytic_detail::wide_add(
                analytic_detail::wide_multiply(static_cast<std::int64_t>(dx),
                                               static_cast<std::int64_t>(dx)),
                analytic_detail::wide_multiply(static_cast<std::int64_t>(dy),
                                               static_cast<std::int64_t>(dy)));
            const auto diameter_squared = analytic_detail::wide_multiply(
                static_cast<std::int64_t>(diameter), static_cast<std::int64_t>(diameter));
            const int comparison = analytic_detail::wide_compare(chord_squared, diameter_squared);
            if (comparison > 0 || (fragment.major_arc && comparison == 0))
                return LayoutError::invalid_packet;
        }
        used_vertices[fragment.start_vertex] = true;
        used_vertices[fragment.end_vertex] = true;
    }
    if (!std::all_of(used_vertices.begin(), used_vertices.end(), [](bool value) { return value; }))
        return LayoutError::invalid_packet;
    if (!gapless_partition(
            records.rings, records.fragment_references.size(),
            [](const auto& ring) { return ring.fragment_reference_begin; },
            [](const auto& ring) { return ring.fragment_reference_count; }))
        return LayoutError::invalid_packet;
    std::vector<bool> used_fragments(records.fragments.size());
    for (std::size_t index = 0; index < records.rings.size(); ++index)
    {
        const auto& ring = records.rings[index];
        if (ring.id != index + 1 || ring.fragment_reference_count == 0 || ring.flags > 1 ||
            (ring.flags & 1U) != (ring.depth & 1U))
            return LayoutError::invalid_packet;
        if (ring.depth == 0)
        {
            if (ring.parent_ring != std::numeric_limits<std::uint32_t>::max())
                return LayoutError::invalid_packet;
        }
        else if (ring.parent_ring >= records.rings.size() ||
                 records.rings[ring.parent_ring].depth + 1 != ring.depth)
            return LayoutError::invalid_packet;
        for (std::uint32_t offset = 0; offset < ring.fragment_reference_count; ++offset)
        {
            const std::uint32_t fragment =
                records.fragment_references[ring.fragment_reference_begin + offset];
            if (fragment >= records.fragments.size() || used_fragments[fragment])
                return LayoutError::invalid_packet;
            const std::uint32_t next_fragment =
                records.fragment_references[ring.fragment_reference_begin +
                                            (offset + 1) % ring.fragment_reference_count];
            if (next_fragment >= records.fragments.size() ||
                records.fragments[fragment].end_vertex !=
                    records.fragments[next_fragment].start_vertex)
                return LayoutError::invalid_packet;
            used_fragments[fragment] = true;
        }
    }
    if (!std::all_of(used_fragments.begin(), used_fragments.end(),
                     [](bool value) { return value; }))
        return LayoutError::invalid_packet;
    std::vector<bool> outer(records.rings.size());
    for (std::size_t index = 0; index < records.regions.size(); ++index)
    {
        const auto& region = records.regions[index];
        if (region.id != index + 1 || region.outer_ring >= records.rings.size() ||
            records.rings[region.outer_ring].depth % 2 != 0 || outer[region.outer_ring] ||
            region.positive_source_set == 0 || region.positive_source_set > set_count)
            return LayoutError::invalid_packet;
        outer[region.outer_ring] = true;
    }
    for (std::size_t index = 0; index < records.rings.size(); ++index)
        if ((records.rings[index].depth % 2 == 0) != outer[index])
            return LayoutError::invalid_packet;
    return LayoutError::none;
}

LayoutError validate_owners(const AnalyticResultPacketRecords& records)
{
    if (!gapless_partition(
            records.job_results, records.diagnostics.size(),
            [](const auto& job) { return job.diagnostic_begin; },
            [](const auto& job) { return job.diagnostic_count; }) ||
        !gapless_partition(
            records.job_results, records.regions.size(),
            [](const auto& job) { return job.result_region_begin; },
            [](const auto& job) { return job.result_region_count; }) ||
        !gapless_partition(
            records.job_results, records.operand_events.size(),
            [](const auto& job) { return job.operand_event_begin; },
            [](const auto& job) { return job.operand_event_count; }))
        return LayoutError::invalid_packet;
    std::uint64_t previous_job = 0;
    std::vector<std::uint32_t> region_owner(records.regions.size());
    std::vector<std::uint32_t> event_owner(records.operand_events.size());
    for (std::uint32_t job_index = 0; job_index < records.job_results.size(); ++job_index)
    {
        const auto& job = records.job_results[job_index];
        if (job.job_id == 0 || job.job_id <= previous_job || job.status > 1 ||
            (job.status == 1 && job.result_region_count != 0))
            return LayoutError::invalid_packet;
        previous_job = job.job_id;
        bool has_error = false;
        for (std::uint32_t offset = 0; offset < job.diagnostic_count; ++offset)
        {
            const auto& diagnostic = records.diagnostics[job.diagnostic_begin + offset];
            if (diagnostic.job_id != job.job_id)
                return LayoutError::invalid_packet;
            has_error = has_error || diagnostic.severity == 1;
        }
        if ((job.status == 1) != has_error)
            return LayoutError::invalid_packet;
        for (std::uint32_t offset = 0; offset < job.result_region_count; ++offset)
            region_owner[job.result_region_begin + offset] = job_index;
        for (std::uint32_t offset = 0; offset < job.operand_event_count; ++offset)
            event_owner[job.operand_event_begin + offset] = job_index;
    }
    std::tuple<std::uint64_t, std::uint8_t, std::uint32_t, std::uint16_t, std::uint64_t,
               std::uint64_t, std::uint64_t, std::uint32_t>
        previous_diagnostic{};
    bool have_previous_diagnostic = false;
    for (const auto& diagnostic : records.diagnostics)
    {
        const bool ids_match =
            ((diagnostic.presence_flags & 1U) != 0) == (diagnostic.job_id != 0) &&
            ((diagnostic.presence_flags & 2U) != 0) == (diagnostic.stage_id != 0) &&
            ((diagnostic.presence_flags & 4U) != 0) == (diagnostic.operand_id != 0) &&
            ((diagnostic.presence_flags & 8U) != 0) == (diagnostic.geometry_source_id != 0);
        if (!valid_diagnostic_code(diagnostic.code) || diagnostic.severity < 1 ||
            diagnostic.severity > 2 || diagnostic.presence_flags > 15 ||
            (diagnostic.presence_flags & 1U) == 0 || !ids_match || diagnostic.path_token > 26)
            return LayoutError::invalid_packet;
        const auto key = std::tuple{diagnostic.job_id,
                                    diagnostic.severity,
                                    diagnostic.code,
                                    diagnostic.presence_flags,
                                    diagnostic.stage_id,
                                    diagnostic.operand_id,
                                    diagnostic.geometry_source_id,
                                    diagnostic.path_token};
        if (have_previous_diagnostic && !(previous_diagnostic < key))
            return LayoutError::invalid_packet;
        previous_diagnostic = key;
        have_previous_diagnostic = true;
    }
    const std::uint32_t set_count = static_cast<std::uint32_t>(records.source_sets.size());
    if (!gapless_partition(
            records.operand_events, records.ring_region_references.size(),
            [](const auto& event) { return event.result_reference_begin; },
            [](const auto& event) { return event.result_reference_count; }))
        return LayoutError::invalid_packet;
    for (const auto& event : records.operand_events)
    {
        const auto kind = static_cast<std::uint16_t>(event.kind);
        if (event.operand_id == 0 || kind < 1 || kind > 7 || event.source_set > set_count ||
            !valid_range(event.result_reference_begin, event.result_reference_count,
                         records.ring_region_references.size()))
            return LayoutError::invalid_packet;
        std::uint64_t previous_reference = 0;
        bool have_reference = false;
        for (std::uint32_t offset = 0; offset < event.result_reference_count; ++offset)
        {
            const std::uint64_t reference =
                records.ring_region_references[event.result_reference_begin + offset];
            if (have_reference && reference <= previous_reference)
                return LayoutError::invalid_packet;
            previous_reference = reference;
            have_reference = true;
        }
    }
    for (std::uint64_t reference : records.ring_region_references)
    {
        const std::uint32_t kind = static_cast<std::uint32_t>(reference >> 32U);
        const std::uint32_t index = static_cast<std::uint32_t>(reference);
        if ((kind == 1 && index >= records.rings.size()) ||
            (kind == 2 && index >= records.regions.size()) || (kind != 1 && kind != 2))
            return LayoutError::invalid_packet;
    }
    std::vector<std::uint32_t> outer_region(records.rings.size(),
                                            std::numeric_limits<std::uint32_t>::max());
    for (std::uint32_t region = 0; region < records.regions.size(); ++region)
        outer_region[records.regions[region].outer_ring] = region;
    const std::uint32_t no_ring = std::numeric_limits<std::uint32_t>::max();
    std::vector<std::uint32_t> ring_root(records.rings.size(), no_ring);
    std::vector<std::uint32_t> path;
    for (std::uint32_t ring = 0; ring < records.rings.size(); ++ring)
    {
        std::uint32_t current = ring;
        path.clear();
        while (ring_root[current] == no_ring)
        {
            path.push_back(current);
            const std::uint32_t parent = records.rings[current].parent_ring;
            if (parent == no_ring)
                break;
            current = parent;
        }
        const std::uint32_t root = ring_root[current] == no_ring ? current : ring_root[current];
        for (auto entry = path.rbegin(); entry != path.rend(); ++entry)
            ring_root[*entry] = root;
    }
    std::vector<std::uint32_t> ring_owner(records.rings.size());
    for (std::uint32_t ring = 0; ring < records.rings.size(); ++ring)
    {
        const std::uint32_t root = ring_root[ring];
        if (outer_region[root] == std::numeric_limits<std::uint32_t>::max())
            return LayoutError::invalid_packet;
        ring_owner[ring] = region_owner[outer_region[root]];
    }
    for (std::uint32_t region = 0; region < records.regions.size(); ++region)
        if (ring_owner[records.regions[region].outer_ring] != region_owner[region])
            return LayoutError::invalid_packet;
    const std::uint32_t no_owner = std::numeric_limits<std::uint32_t>::max();
    std::vector<std::uint32_t> vertex_owner(records.vertices.size(), no_owner);
    for (std::uint32_t ring = 0; ring < records.rings.size(); ++ring)
    {
        const auto& value = records.rings[ring];
        for (std::uint32_t offset = 0; offset < value.fragment_reference_count; ++offset)
        {
            const auto& fragment =
                records.fragments[records.fragment_references[value.fragment_reference_begin +
                                                              offset]];
            for (std::uint32_t vertex : {fragment.start_vertex, fragment.end_vertex})
            {
                if (vertex_owner[vertex] != no_owner && vertex_owner[vertex] != ring_owner[ring])
                    return LayoutError::invalid_packet;
                vertex_owner[vertex] = ring_owner[ring];
            }
        }
    }
    std::vector<bool> job_has_vertex(records.job_results.size());
    std::vector<std::int64_t> minimum_x(records.job_results.size());
    std::vector<std::int64_t> maximum_x(records.job_results.size());
    std::vector<std::int64_t> minimum_y(records.job_results.size());
    std::vector<std::int64_t> maximum_y(records.job_results.size());
    for (std::uint32_t vertex = 0; vertex < records.vertices.size(); ++vertex)
    {
        const std::uint32_t owner = vertex_owner[vertex];
        if (owner == no_owner)
            return LayoutError::invalid_packet;
        const auto& value = records.vertices[vertex];
        if (!job_has_vertex[owner])
        {
            minimum_x[owner] = maximum_x[owner] = value.x_nm;
            minimum_y[owner] = maximum_y[owner] = value.y_nm;
            job_has_vertex[owner] = true;
        }
        else
        {
            minimum_x[owner] = std::min(minimum_x[owner], value.x_nm);
            maximum_x[owner] = std::max(maximum_x[owner], value.x_nm);
            minimum_y[owner] = std::min(minimum_y[owner], value.y_nm);
            maximum_y[owner] = std::max(maximum_y[owner], value.y_nm);
        }
    }
    for (std::uint32_t job = 0; job < records.job_results.size(); ++job)
        if (job_has_vertex[job] &&
            (absolute_difference(maximum_x[job], minimum_x[job]) > 1'000'000'000'000ULL ||
             absolute_difference(maximum_y[job], minimum_y[job]) > 1'000'000'000'000ULL))
            return LayoutError::limit_exceeded;
    for (std::uint32_t event_index = 0; event_index < records.operand_events.size(); ++event_index)
    {
        const auto& event = records.operand_events[event_index];
        for (std::uint32_t offset = 0; offset < event.result_reference_count; ++offset)
        {
            const std::uint64_t reference =
                records.ring_region_references[event.result_reference_begin + offset];
            const std::uint32_t kind = static_cast<std::uint32_t>(reference >> 32U);
            const std::uint32_t index = static_cast<std::uint32_t>(reference);
            const std::uint32_t owner = kind == 1 ? ring_owner[index] : region_owner[index];
            if (owner != event_owner[event_index])
                return LayoutError::invalid_packet;
        }
    }
    for (const auto& job : records.job_results)
    {
        std::tuple<std::uint64_t, std::uint16_t, std::vector<std::uint64_t>, std::uint32_t>
            previous_event{};
        bool have_previous_event = false;
        for (std::uint32_t offset = 0; offset < job.operand_event_count; ++offset)
        {
            const auto& event = records.operand_events[job.operand_event_begin + offset];
            std::vector<std::uint64_t> references;
            references.reserve(event.result_reference_count);
            for (std::uint32_t reference = 0; reference < event.result_reference_count; ++reference)
                references.push_back(
                    records.ring_region_references[event.result_reference_begin + reference]);
            const auto key = std::tuple{event.operand_id, static_cast<std::uint16_t>(event.kind),
                                        std::move(references), event.source_set};
            if (have_previous_event && !(previous_event < key))
                return LayoutError::invalid_packet;
            previous_event = key;
            have_previous_event = true;
        }
    }
    return LayoutError::none;
}

LayoutError validate_source_uses(const AnalyticResultPacketRecords& records)
{
    std::vector<bool> used(records.source_sets.size());
    const auto mark = [&used](std::uint32_t handle)
    {
        if (handle > used.size())
            return false;
        if (handle != 0)
            used[handle - 1] = true;
        return true;
    };
    for (const auto& vertex : records.vertices)
        if (!mark(vertex.intersection_source_set))
            return LayoutError::invalid_packet;
    for (const auto& fragment : records.fragments)
        if (!mark(fragment.positive_source_set) || !mark(fragment.subtraction_source_set))
            return LayoutError::invalid_packet;
    for (const auto& region : records.regions)
        if (!mark(region.positive_source_set))
            return LayoutError::invalid_packet;
    for (const auto& event : records.operand_events)
        if (!mark(event.source_set))
            return LayoutError::invalid_packet;
    return std::all_of(used.begin(), used.end(), [](bool value) { return value; })
               ? LayoutError::none
               : LayoutError::invalid_packet;
}

LayoutError validate_relationships(const AnalyticResultPacketRecords& records)
{
    if (!gapless_partition(
            records.relationship_results, records.relationship_pairs.size(), [](const auto& result)
            { return result.pair_begin; }, [](const auto& result) { return result.pair_count; }))
        return LayoutError::invalid_packet;
    std::uint64_t previous_query = 0;
    for (const auto& result : records.relationship_results)
    {
        if (result.query_id == 0 || result.query_id <= previous_query || result.status > 1 ||
            result.aggregate_dimension > 3 ||
            (result.status == 1 &&
             (result.aggregate_dimension != 0 || result.pair_begin != 0 || result.pair_count != 0)))
            return LayoutError::invalid_packet;
        std::uint8_t aggregate = 0;
        std::tuple<std::uint64_t, std::uint64_t, std::uint8_t, bool, bool, bool> previous{};
        bool have_previous = false;
        for (std::uint32_t offset = 0; offset < result.pair_count; ++offset)
        {
            const auto& pair = records.relationship_pairs[result.pair_begin + offset];
            const auto key = std::tuple{pair.left_result_region_id,
                                        pair.right_result_region_id,
                                        pair.dimension,
                                        pair.equality,
                                        pair.left_contains_right,
                                        pair.right_contains_left};
            if (have_previous && !(previous < key))
                return LayoutError::invalid_packet;
            previous = key;
            have_previous = true;
            aggregate = std::max(aggregate, pair.dimension);
        }
        if (result.status == 0 && result.aggregate_dimension != aggregate)
            return LayoutError::invalid_packet;
        previous_query = result.query_id;
    }
    for (const auto& pair : records.relationship_pairs)
        if (pair.left_result_region_id == 0 || pair.right_result_region_id == 0 ||
            pair.left_result_region_id > records.regions.size() ||
            pair.right_result_region_id > records.regions.size() || pair.dimension > 3 ||
            ((pair.equality || pair.left_contains_right || pair.right_contains_left) &&
             pair.dimension != 3) ||
            (pair.equality && (!pair.left_contains_right || !pair.right_contains_left)))
            return LayoutError::invalid_packet;
    return LayoutError::none;
}

LayoutError preflight_logical_source_reference_expansions(
    const std::uint8_t* data,
    const std::array<AnalyticResultTableView, kAnalyticResultTableCount>& views)
{
    std::uint64_t total = 0;
    const auto& source_sets = views[8];
    const auto charge = [&](std::uint32_t handle)
    {
        if (handle == 0)
            return LayoutError::none;
        if (handle > source_sets.record_count)
            return LayoutError::invalid_packet;
        const Reader source_set{data, source_sets.offset + (handle - 1) * 8};
        const std::uint64_t count = source_set.u32(4);
        if (total > kAnalyticMaximumLogicalSourceReferenceExpansions ||
            count > kAnalyticMaximumLogicalSourceReferenceExpansions - total)
            return LayoutError::limit_exceeded;
        total += count;
        return LayoutError::none;
    };
    const auto charge_table = [&](std::size_t table_index, std::uint64_t field_offset)
    {
        const auto& view = views[table_index];
        for (std::uint64_t index = 0; index < view.record_count; ++index)
            if (const LayoutError error =
                    charge(Reader{data, view.offset + index * view.record_bytes}.u32(field_offset));
                error != LayoutError::none)
                return error;
        return LayoutError::none;
    };
    if (const LayoutError error = charge_table(2, 24); error != LayoutError::none)
        return error;
    const auto& fragments = views[3];
    for (std::uint64_t index = 0; index < fragments.record_count; ++index)
    {
        const Reader fragment{data, fragments.offset + index * fragments.record_bytes};
        if (const LayoutError error = charge(fragment.u32(32)); error != LayoutError::none)
            return error;
        if (const LayoutError error = charge(fragment.u32(36)); error != LayoutError::none)
            return error;
    }
    if (const LayoutError error = charge_table(6, 12); error != LayoutError::none)
        return error;
    return charge_table(10, 20);
}

} // namespace

namespace analytic_result_detail
{

AnalyticResultPacketLayoutError
charge_logical_source_reference_expansions(const AnalyticResultPacketRecords& records,
                                           std::uint64_t& total) noexcept
{
    const auto charge = [&](std::uint32_t handle)
    {
        if (handle == 0)
            return LayoutError::none;
        if (handle > records.source_sets.size())
            return LayoutError::invalid_packet;
        const std::uint64_t count = records.source_sets[handle - 1].source_reference_index_count;
        if (total > kAnalyticMaximumLogicalSourceReferenceExpansions ||
            count > kAnalyticMaximumLogicalSourceReferenceExpansions - total)
            return LayoutError::limit_exceeded;
        total += count;
        return LayoutError::none;
    };
    for (const auto& vertex : records.vertices)
        if (const LayoutError error = charge(vertex.intersection_source_set);
            error != LayoutError::none)
            return error;
    for (const auto& fragment : records.fragments)
    {
        if (const LayoutError error = charge(fragment.positive_source_set);
            error != LayoutError::none)
            return error;
        if (const LayoutError error = charge(fragment.subtraction_source_set);
            error != LayoutError::none)
            return error;
    }
    for (const auto& region : records.regions)
        if (const LayoutError error = charge(region.positive_source_set);
            error != LayoutError::none)
            return error;
    for (const auto& event : records.operand_events)
        if (const LayoutError error = charge(event.source_set); error != LayoutError::none)
            return error;
    return LayoutError::none;
}

} // namespace analytic_result_detail

AnalyticResultPacketLayoutError
validate_analytic_result_packet_records(const AnalyticResultPacketRecords& records)
{
    try
    {
        if (records.job_results.size() > 65'535 ||
            records.relationship_results.size() > 1'048'576 ||
            records.source_sets.size() > std::numeric_limits<std::uint32_t>::max())
            return LayoutError::limit_exceeded;
        std::uint64_t logical_source_expansions = 0;
        if (const LayoutError error =
                analytic_result_detail::charge_logical_source_reference_expansions(
                    records, logical_source_expansions);
            error != LayoutError::none)
            return error;
        const auto validators = {validate_sources, validate_geometry, validate_owners,
                                 validate_source_uses, validate_relationships};
        for (const auto validator : validators)
        {
            const LayoutError error = validator(records);
            if (error != LayoutError::none)
                return error;
        }
        return LayoutError::none;
    }
    catch (const std::exception&)
    {
        return LayoutError::limit_exceeded;
    }
}

AnalyticResultPacketEncodeResult
serialize_records_unchecked(const AnalyticResultPacketRecords& records)
{
    try
    {
        AnalyticResultTableBytes tables;
        const std::size_t counts[]{
            records.job_results.size(),
            records.diagnostics.size(),
            records.vertices.size(),
            records.fragments.size(),
            records.rings.size(),
            records.fragment_references.size(),
            records.regions.size(),
            records.ring_region_references.size(),
            records.source_sets.size(),
            records.source_references.size(),
            records.operand_events.size(),
            records.relationship_results.size(),
            records.relationship_pairs.size(),
            records.source_reference_indices.size(),
        };
        constexpr std::uint32_t sizes[]{48, 56, 32, 48, 32, 4, 24, 8, 8, 32, 48, 32, 32, 4};
        std::uint64_t cursor = 512;
        for (std::size_t index = 0; index < kAnalyticResultTableCount; ++index)
        {
            cursor = (cursor + 7U) & ~std::uint64_t{7};
            if (cursor > 268'435'456U || counts[index] > (268'435'456U - cursor) / sizes[index])
                return {LayoutError::limit_exceeded, std::nullopt};
            const std::size_t bytes = counts[index] * sizes[index];
            cursor += bytes;
            tables[index].reserve(bytes);
        }
        for (const auto& value : records.job_results)
        {
            append_u64(tables[0], value.job_id);
            append_u8(tables[0], value.status);
            append_zero(tables[0], 7);
            append_u32(tables[0], value.diagnostic_begin);
            append_u32(tables[0], value.diagnostic_count);
            append_u32(tables[0], value.result_region_begin);
            append_u32(tables[0], value.result_region_count);
            append_u32(tables[0], value.operand_event_begin);
            append_u32(tables[0], value.operand_event_count);
            append_zero(tables[0], 8);
        }
        for (const auto& value : records.diagnostics)
        {
            append_u32(tables[1], value.code);
            append_u8(tables[1], value.severity);
            append_u8(tables[1], 1);
            append_u16(tables[1], value.presence_flags);
            append_u64(tables[1], value.job_id);
            append_u64(tables[1], value.stage_id);
            append_u64(tables[1], value.operand_id);
            append_u64(tables[1], value.geometry_source_id);
            append_u32(tables[1], value.path_token);
            append_u32(tables[1], 0);
            append_u64(tables[1], 0);
        }
        for (const auto& value : records.vertices)
        {
            append_u64(tables[2], value.id);
            append_i64(tables[2], value.x_nm);
            append_i64(tables[2], value.y_nm);
            append_u32(tables[2], value.intersection_source_set);
            append_u32(tables[2], value.flags);
        }
        for (const auto& value : records.fragments)
        {
            append_u64(tables[3], value.id);
            append_u32(tables[3], value.start_vertex);
            append_u32(tables[3], value.end_vertex);
            append_u8(tables[3], value.kind);
            append_u8(tables[3], value.direction);
            append_u8(tables[3], value.major_arc ? 1 : 0);
            append_zero(tables[3], 5);
            append_u64(tables[3], value.radius_nm);
            append_u32(tables[3], value.positive_source_set);
            append_u32(tables[3], value.subtraction_source_set);
            append_u64(tables[3], 0);
        }
        for (const auto& value : records.rings)
        {
            append_u64(tables[4], value.id);
            append_u32(tables[4], value.fragment_reference_begin);
            append_u32(tables[4], value.fragment_reference_count);
            append_u32(tables[4], value.parent_ring);
            append_u32(tables[4], value.depth);
            append_u32(tables[4], value.flags);
            append_u32(tables[4], 0);
        }
        for (std::uint32_t value : records.fragment_references)
            append_u32(tables[5], value);
        for (const auto& value : records.regions)
        {
            append_u64(tables[6], value.id);
            append_u32(tables[6], value.outer_ring);
            append_u32(tables[6], value.positive_source_set);
            append_u32(tables[6], 0);
            append_u32(tables[6], 0);
        }
        for (std::uint64_t value : records.ring_region_references)
            append_u64(tables[7], value);
        for (const auto& value : records.source_sets)
        {
            append_u32(tables[8], value.source_reference_index_begin);
            append_u32(tables[8], value.source_reference_index_count);
        }
        for (const auto& value : records.source_references)
        {
            append_u16(tables[9], static_cast<std::uint16_t>(value.kind));
            append_u16(tables[9], static_cast<std::uint16_t>(value.role));
            append_u32(tables[9], 0);
            append_u64(tables[9], value.operand_id);
            append_u64(tables[9], value.primary_id);
            append_u64(tables[9], value.secondary_id);
        }
        for (const auto& value : records.operand_events)
        {
            append_u64(tables[10], value.operand_id);
            append_u16(tables[10], static_cast<std::uint16_t>(value.kind));
            append_u16(tables[10], 0);
            append_u32(tables[10], value.result_reference_begin);
            append_u32(tables[10], value.result_reference_count);
            append_u32(tables[10], value.source_set);
            append_u32(tables[10], 0);
            append_u32(tables[10], 0);
            append_u64(tables[10], 0);
            append_u64(tables[10], 0);
        }
        for (const auto& value : records.relationship_results)
        {
            append_u64(tables[11], value.query_id);
            append_u8(tables[11], value.status);
            append_u8(tables[11], value.aggregate_dimension);
            append_u16(tables[11], 0);
            append_u32(tables[11], value.pair_begin);
            append_u32(tables[11], value.pair_count);
            append_u32(tables[11], 0);
            append_u64(tables[11], 0);
        }
        for (const auto& value : records.relationship_pairs)
        {
            append_u64(tables[12], value.left_result_region_id);
            append_u64(tables[12], value.right_result_region_id);
            append_u8(tables[12], value.dimension);
            append_u8(tables[12], value.equality ? 1 : 0);
            append_u8(tables[12], value.left_contains_right ? 1 : 0);
            append_u8(tables[12], value.right_contains_left ? 1 : 0);
            append_zero(tables[12], 4);
            append_u64(tables[12], 0);
        }
        for (std::uint32_t value : records.source_reference_indices)
            append_u32(tables[13], value);
        return encode_analytic_result_packet_layout(tables);
    }
    catch (const std::exception&)
    {
        return {LayoutError::limit_exceeded, std::nullopt};
    }
}

AnalyticResultPacketEncodeResult encode_records_checked(const AnalyticResultPacketRecords& records)
{
    if (const LayoutError error = validate_analytic_result_packet_records(records);
        error != LayoutError::none)
        return {error, std::nullopt};
    return serialize_records_unchecked(records);
}

AnalyticResultPacketEncodeResult
encode_analytic_result_packet_records(const AnalyticResultPacketRecords& records)
{
    AnalyticResultPacketRecordsResult canonical =
        canonicalize_analytic_result_packet_records(records);
    if (canonical.error != LayoutError::none || !canonical.value)
        return {canonical.error, std::nullopt};
    return encode_records_checked(*canonical.value);
}

AnalyticResultPacketEncodeResult analytic_result_detail::encode_canonical_records_unchecked(
    const AnalyticResultPacketRecords& records)
{
    return serialize_records_unchecked(records);
}

AnalyticResultPacketRecordsResult decode_analytic_result_packet_records(const std::uint8_t* data,
                                                                        std::size_t size)
{
    AnalyticResultPacketLayoutResult decoded_layout =
        decode_analytic_result_packet_layout(data, size);
    if (decoded_layout.error != LayoutError::none || !decoded_layout.value)
        return {decoded_layout.error, std::nullopt};
    if (const LayoutError error =
            preflight_logical_source_reference_expansions(data, decoded_layout.value->tables);
        error != LayoutError::none)
        return {error, std::nullopt};
    try
    {
        const auto& views = decoded_layout.value->tables;
        AnalyticResultPacketRecords output;
        output.job_results.reserve(static_cast<std::size_t>(views[0].record_count));
        output.diagnostics.reserve(static_cast<std::size_t>(views[1].record_count));
        output.vertices.reserve(static_cast<std::size_t>(views[2].record_count));
        output.fragments.reserve(static_cast<std::size_t>(views[3].record_count));
        output.rings.reserve(static_cast<std::size_t>(views[4].record_count));
        output.fragment_references.reserve(static_cast<std::size_t>(views[5].record_count));
        output.regions.reserve(static_cast<std::size_t>(views[6].record_count));
        output.ring_region_references.reserve(static_cast<std::size_t>(views[7].record_count));
        output.source_sets.reserve(static_cast<std::size_t>(views[8].record_count));
        output.source_references.reserve(static_cast<std::size_t>(views[9].record_count));
        output.operand_events.reserve(static_cast<std::size_t>(views[10].record_count));
        output.relationship_results.reserve(static_cast<std::size_t>(views[11].record_count));
        output.relationship_pairs.reserve(static_cast<std::size_t>(views[12].record_count));
        output.source_reference_indices.reserve(static_cast<std::size_t>(views[13].record_count));
        for (std::uint64_t index = 0; index < views[0].record_count; ++index)
        {
            Reader r{data, views[0].offset + index * 48};
            if (!r.zero(9, 7) || !r.zero(40, 8))
                return {LayoutError::invalid_packet, std::nullopt};
            output.job_results.push_back({r.u64(0), r.u8(8), r.u32(16), r.u32(20), r.u32(24),
                                          r.u32(28), r.u32(32), r.u32(36)});
        }
        for (std::uint64_t index = 0; index < views[1].record_count; ++index)
        {
            Reader r{data, views[1].offset + index * 56};
            if (r.u8(5) != 1 || r.u32(44) != 0 || r.u64(48) != 0)
                return {LayoutError::invalid_packet, std::nullopt};
            output.diagnostics.push_back({r.u32(0), r.u8(4), r.u16(6), r.u64(8), r.u64(16),
                                          r.u64(24), r.u64(32), r.u32(40)});
        }
        for (std::uint64_t index = 0; index < views[2].record_count; ++index)
        {
            Reader r{data, views[2].offset + index * 32};
            output.vertices.push_back({r.u64(0), r.i64(8), r.i64(16), r.u32(24), r.u32(28)});
        }
        for (std::uint64_t index = 0; index < views[3].record_count; ++index)
        {
            Reader r{data, views[3].offset + index * 48};
            if (r.u8(18) > 1 || !r.zero(19, 5) || r.u64(40) != 0)
                return {LayoutError::invalid_packet, std::nullopt};
            output.fragments.push_back({r.u64(0), r.u32(8), r.u32(12), r.u8(16), r.u8(17),
                                        r.u8(18) != 0, r.u64(24), r.u32(32), r.u32(36)});
        }
        for (std::uint64_t index = 0; index < views[4].record_count; ++index)
        {
            Reader r{data, views[4].offset + index * 32};
            if (r.u32(28) != 0)
                return {LayoutError::invalid_packet, std::nullopt};
            output.rings.push_back(
                {r.u64(0), r.u32(8), r.u32(12), r.u32(16), r.u32(20), r.u32(24)});
        }
        for (std::uint64_t index = 0; index < views[5].record_count; ++index)
            output.fragment_references.push_back(Reader{data, views[5].offset + index * 4}.u32(0));
        for (std::uint64_t index = 0; index < views[6].record_count; ++index)
        {
            Reader r{data, views[6].offset + index * 24};
            if (r.u32(16) != 0 || r.u32(20) != 0)
                return {LayoutError::invalid_packet, std::nullopt};
            output.regions.push_back({r.u64(0), r.u32(8), r.u32(12)});
        }
        for (std::uint64_t index = 0; index < views[7].record_count; ++index)
            output.ring_region_references.push_back(
                Reader{data, views[7].offset + index * 8}.u64(0));
        for (std::uint64_t index = 0; index < views[8].record_count; ++index)
        {
            Reader r{data, views[8].offset + index * 8};
            output.source_sets.push_back({r.u32(0), r.u32(4)});
        }
        for (std::uint64_t index = 0; index < views[9].record_count; ++index)
        {
            Reader r{data, views[9].offset + index * 32};
            if (r.u32(4) != 0)
                return {LayoutError::invalid_packet, std::nullopt};
            output.source_references.push_back({static_cast<AnalyticSourceKind>(r.u16(0)),
                                                static_cast<AnalyticSourceRole>(r.u16(2)), r.u64(8),
                                                r.u64(16), r.u64(24)});
        }
        for (std::uint64_t index = 0; index < views[10].record_count; ++index)
        {
            Reader r{data, views[10].offset + index * 48};
            if (r.u16(10) != 0 || r.u32(24) != 0 || r.u32(28) != 0 || r.u64(32) != 0 ||
                r.u64(40) != 0)
                return {LayoutError::invalid_packet, std::nullopt};
            output.operand_events.push_back({r.u64(0),
                                             static_cast<AnalyticOperandOutcomeKind>(r.u16(8)),
                                             r.u32(12), r.u32(16), r.u32(20)});
        }
        for (std::uint64_t index = 0; index < views[11].record_count; ++index)
        {
            Reader r{data, views[11].offset + index * 32};
            if (r.u16(10) != 0 || r.u32(20) != 0 || r.u64(24) != 0)
                return {LayoutError::invalid_packet, std::nullopt};
            output.relationship_results.push_back(
                {r.u64(0), r.u8(8), r.u8(9), r.u32(12), r.u32(16)});
        }
        for (std::uint64_t index = 0; index < views[12].record_count; ++index)
        {
            Reader r{data, views[12].offset + index * 32};
            if (r.u8(17) > 1 || r.u8(18) > 1 || r.u8(19) > 1 || !r.zero(20, 4) || r.u64(24) != 0)
                return {LayoutError::invalid_packet, std::nullopt};
            output.relationship_pairs.push_back(
                {r.u64(0), r.u64(8), r.u8(16), r.u8(17) != 0, r.u8(18) != 0, r.u8(19) != 0});
        }
        for (std::uint64_t index = 0; index < views[13].record_count; ++index)
            output.source_reference_indices.push_back(
                Reader{data, views[13].offset + index * 4}.u32(0));
        if (const LayoutError error = validate_analytic_result_packet_records(output);
            error != LayoutError::none)
            return {error, std::nullopt};
        AnalyticResultPacketRecordsResult canonical =
            canonicalize_analytic_result_packet_records(output);
        if (canonical.error != LayoutError::none || !canonical.value)
            return {canonical.error, std::nullopt};
        AnalyticResultPacketEncodeResult encoded = serialize_records_unchecked(*canonical.value);
        if (encoded.error != LayoutError::none || !encoded.value || encoded.value->size() != size ||
            !std::equal(encoded.value->begin(), encoded.value->end(), data))
            return {LayoutError::invalid_packet, std::nullopt};
        return {LayoutError::none, std::move(*canonical.value)};
    }
    catch (const std::exception&)
    {
        return {LayoutError::limit_exceeded, std::nullopt};
    }
}

} // namespace geometer

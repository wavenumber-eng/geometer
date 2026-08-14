#include "geometer/analytic_result_packet_topology.h"
#include "geometer/sha256.h"

#include <boost/multiprecision/cpp_int.hpp>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{

using namespace geometer;
using namespace geometer::exact;
using BigInt = boost::multiprecision::cpp_int;
constexpr std::uint32_t kNone = std::numeric_limits<std::uint32_t>::max();

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct SymbolicMeasure
{
    // area = (area_four_rational + area_four_pi * pi) / 4
    BigInt area_four_rational = 0;
    BigInt area_four_pi = 0;
    // perimeter = (perimeter_two_rational + perimeter_two_pi * pi) / 2
    BigInt perimeter_two_rational = 0;
    BigInt perimeter_two_pi = 0;
    std::uint32_t components = 0;
    std::uint32_t holes = 0;

    bool operator==(const SymbolicMeasure& other) const
    {
        return area_four_rational == other.area_four_rational &&
               area_four_pi == other.area_four_pi &&
               perimeter_two_rational == other.perimeter_two_rational &&
               perimeter_two_pi == other.perimeter_two_pi && components == other.components &&
               holes == other.holes;
    }
};

void add_source(AnalyticResultPacketRecords& records, ExactSourceRole role)
{
    records.source_references = {{ExactSourceKind::authored_segment_curve, role, 1, 1, 1}};
    records.source_reference_indices = {0};
    records.source_sets = {{0, 1}};
}

std::uint32_t add_vertex(AnalyticResultPacketRecords& records, std::int64_t x, std::int64_t y)
{
    const std::uint32_t index = static_cast<std::uint32_t>(records.vertices.size());
    records.vertices.push_back({static_cast<std::uint64_t>(index) + 1, x, y, 0, 0});
    return index;
}

void add_line(AnalyticResultPacketRecords& records, std::uint32_t start, std::uint32_t end)
{
    const std::uint32_t index = static_cast<std::uint32_t>(records.fragments.size());
    records.fragments.push_back(
        {static_cast<std::uint64_t>(index) + 1, start, end, 1, 0, false, 0, 1, 0});
    records.fragment_references.push_back(index);
}

void add_half_arc(AnalyticResultPacketRecords& records, std::uint32_t start, std::uint32_t end,
                  std::uint8_t direction, std::uint64_t radius)
{
    const std::uint32_t index = static_cast<std::uint32_t>(records.fragments.size());
    records.fragments.push_back(
        {static_cast<std::uint64_t>(index) + 1, start, end, 2, direction, false, radius, 1, 0});
    records.fragment_references.push_back(index);
}

void add_ring(AnalyticResultPacketRecords& records, std::uint32_t fragment_begin,
              std::uint32_t fragment_count, std::uint32_t parent, std::uint32_t depth)
{
    records.rings.push_back({static_cast<std::uint64_t>(records.rings.size()) + 1, fragment_begin,
                             fragment_count, parent, depth, depth & 1U});
}

void finish(AnalyticResultPacketRecords& records, const std::vector<std::uint32_t>& outer_rings)
{
    for (const std::uint32_t ring : outer_rings)
        records.regions.push_back(
            {static_cast<std::uint64_t>(records.regions.size()) + 1, ring, 1});
    records.job_results = {
        {1, 0, 0, 0, 0, static_cast<std::uint32_t>(records.regions.size()), 0, 0}};
}

void add_box_ring(AnalyticResultPacketRecords& records, std::int64_t low, std::int64_t high,
                  bool clockwise, std::uint32_t parent, std::uint32_t depth)
{
    const std::uint32_t begin = static_cast<std::uint32_t>(records.fragments.size());
    std::vector<std::uint32_t> vertices;
    const std::pair<std::int64_t, std::int64_t> ccw[] = {
        {low, low}, {high, low}, {high, high}, {low, high}};
    const std::pair<std::int64_t, std::int64_t> cw[] = {
        {low, low}, {low, high}, {high, high}, {high, low}};
    for (std::uint32_t index = 0; index < 4; ++index)
    {
        const auto [x, y] = clockwise ? cw[index] : ccw[index];
        vertices.push_back(add_vertex(records, x, y));
    }
    for (std::uint32_t index = 0; index < vertices.size(); ++index)
        add_line(records, vertices[index], vertices[(index + 1) % vertices.size()]);
    add_ring(records, begin, 4, parent, depth);
}

AnalyticResultPacketRecords rectangle()
{
    AnalyticResultPacketRecords records;
    add_source(records, ExactSourceRole::authored_line);
    add_box_ring(records, 0, 10, false, kNone, 0);
    finish(records, {0});
    return records;
}

AnalyticResultPacketRecords circle()
{
    AnalyticResultPacketRecords records;
    add_source(records, ExactSourceRole::authored_circular_arc);
    const auto left = add_vertex(records, -10, 0);
    const auto right = add_vertex(records, 10, 0);
    add_half_arc(records, left, right, 1, 10);
    add_half_arc(records, right, left, 1, 10);
    add_ring(records, 0, 2, kNone, 0);
    finish(records, {0});
    return records;
}

AnalyticResultPacketRecords annulus()
{
    AnalyticResultPacketRecords records;
    add_source(records, ExactSourceRole::authored_circular_arc);
    const auto outer_left = add_vertex(records, -10, 0);
    const auto outer_right = add_vertex(records, 10, 0);
    add_half_arc(records, outer_left, outer_right, 1, 10);
    add_half_arc(records, outer_right, outer_left, 1, 10);
    add_ring(records, 0, 2, kNone, 0);
    const auto inner_left = add_vertex(records, -4, 0);
    const auto inner_right = add_vertex(records, 4, 0);
    add_half_arc(records, inner_left, inner_right, 2, 4);
    add_half_arc(records, inner_right, inner_left, 2, 4);
    add_ring(records, 2, 2, 0, 1);
    finish(records, {0});
    return records;
}

AnalyticResultPacketRecords capsule()
{
    AnalyticResultPacketRecords records;
    add_source(records, ExactSourceRole::authored_circular_arc);
    const auto right_top = add_vertex(records, 20, 5);
    const auto left_top = add_vertex(records, 0, 5);
    const auto left_bottom = add_vertex(records, 0, -5);
    const auto right_bottom = add_vertex(records, 20, -5);
    add_line(records, right_top, left_top);
    add_half_arc(records, left_top, left_bottom, 1, 5);
    add_line(records, left_bottom, right_bottom);
    add_half_arc(records, right_bottom, right_top, 1, 5);
    add_ring(records, 0, 4, kNone, 0);
    finish(records, {0});
    return records;
}

AnalyticResultPacketRecords nested_island()
{
    AnalyticResultPacketRecords records;
    add_source(records, ExactSourceRole::authored_line);
    add_box_ring(records, 0, 20, false, kNone, 0);
    add_box_ring(records, 4, 16, true, 0, 1);
    add_box_ring(records, 8, 12, false, 1, 2);
    finish(records, {0, 2});
    return records;
}

BigInt absolute(BigInt value)
{
    return value < 0 ? -value : value;
}

SymbolicMeasure measure(const AnalyticResultPacketRecords& records)
{
    SymbolicMeasure output;
    output.components = static_cast<std::uint32_t>(records.regions.size());
    for (const auto& ring : records.rings)
    {
        output.holes += ring.depth & 1U;
        for (std::uint32_t offset = 0; offset < ring.fragment_reference_count; ++offset)
        {
            const auto& fragment =
                records
                    .fragments[records.fragment_references[ring.fragment_reference_begin + offset]];
            const auto& start = records.vertices[fragment.start_vertex];
            const auto& end = records.vertices[fragment.end_vertex];
            const BigInt x1 = start.x_nm;
            const BigInt y1 = start.y_nm;
            const BigInt x2 = end.x_nm;
            const BigInt y2 = end.y_nm;
            if (fragment.kind == 1)
            {
                require(start.x_nm == end.x_nm || start.y_nm == end.y_nm,
                        "closed-form line oracle requires axis-aligned fixtures");
                output.area_four_rational += 2 * (x1 * y2 - y1 * x2);
                output.perimeter_two_rational += 2 * (absolute(x2 - x1) + absolute(y2 - y1));
                continue;
            }
            const BigInt dx = x2 - x1;
            const BigInt dy = y2 - y1;
            const BigInt radius = fragment.radius_nm;
            require(fragment.kind == 2 && !fragment.major_arc &&
                        dx * dx + dy * dy == 4 * radius * radius,
                    "closed-form arc oracle requires exact semicircle fixtures");
            const BigInt direction = fragment.direction == 1 ? 1 : -1;
            output.area_four_rational += (x1 + x2) * dy - (y1 + y2) * dx;
            output.area_four_pi += direction * 2 * radius * radius;
            output.perimeter_two_pi += 2 * radius;
        }
    }
    return output;
}

AnalyticResultPacketRecords translated(AnalyticResultPacketRecords records, std::int64_t dx,
                                       std::int64_t dy)
{
    for (auto& vertex : records.vertices)
    {
        vertex.x_nm += dx;
        vertex.y_nm += dy;
    }
    return records;
}

AnalyticResultPacketRecords rotated_90(AnalyticResultPacketRecords records)
{
    for (auto& vertex : records.vertices)
    {
        const std::int64_t x = vertex.x_nm;
        vertex.x_nm = -vertex.y_nm;
        vertex.y_nm = x;
    }
    return records;
}

AnalyticResultPacketRecords reflected_x(AnalyticResultPacketRecords records)
{
    for (auto& vertex : records.vertices)
        vertex.x_nm = -vertex.x_nm;
    for (const auto& ring : records.rings)
    {
        auto begin = records.fragment_references.begin() + ring.fragment_reference_begin;
        auto end = begin + ring.fragment_reference_count;
        std::reverse(begin, end);
        for (auto reference = begin; reference != end; ++reference)
        {
            auto& fragment = records.fragments[*reference];
            std::swap(fragment.start_vertex, fragment.end_vertex);
        }
    }
    return records;
}

AnalyticResultPacketRecords scaled(AnalyticResultPacketRecords records, std::int64_t factor)
{
    for (auto& vertex : records.vertices)
    {
        vertex.x_nm *= factor;
        vertex.y_nm *= factor;
    }
    for (auto& fragment : records.fragments)
        fragment.radius_nm *= static_cast<std::uint64_t>(factor);
    return records;
}

AnalyticResultPacketRecords renamed_sources(AnalyticResultPacketRecords records)
{
    for (auto& source : records.source_references)
    {
        source.operand_id += 100;
        source.primary_id += 1'000;
        source.secondary_id += 10'000;
    }
    return records;
}

SymbolicMeasure scaled_measure(SymbolicMeasure value, std::int64_t factor)
{
    const BigInt area_factor = BigInt(factor) * factor;
    value.area_four_rational *= area_factor;
    value.area_four_pi *= area_factor;
    value.perimeter_two_rational *= factor;
    value.perimeter_two_pi *= factor;
    return value;
}

std::string packet_digest(const AnalyticResultPacketRecords& records)
{
    const auto encoded = encode_analytic_result_packet_records(records);
    require(encoded.error == AnalyticResultPacketLayoutError::none && encoded.value,
            "metamorphic result packet failed canonical encoding");
    return sha256_hex(encoded.value->data(), encoded.value->size());
}

void require_common_record_fields(const AnalyticResultPacketRecords& base,
                                  const AnalyticResultPacketRecords& transformed,
                                  const std::string& name)
{
    require(base.job_results.size() == transformed.job_results.size() &&
                base.vertices.size() == transformed.vertices.size() &&
                base.fragments.size() == transformed.fragments.size() &&
                base.rings.size() == transformed.rings.size() &&
                base.regions.size() == transformed.regions.size() &&
                base.source_sets.size() == transformed.source_sets.size() &&
                base.source_references.size() == transformed.source_references.size(),
            name + " changed record counts");
    require(base.diagnostics.empty() && transformed.diagnostics.empty() &&
                base.operand_events.empty() && transformed.operand_events.empty() &&
                base.relationship_results.empty() && transformed.relationship_results.empty() &&
                base.relationship_pairs.empty() && transformed.relationship_pairs.empty() &&
                base.ring_region_references == transformed.ring_region_references &&
                base.source_reference_indices == transformed.source_reference_indices,
            name + " changed non-geometric tables");
    for (std::size_t index = 0; index < base.job_results.size(); ++index)
    {
        const auto& left = base.job_results[index];
        const auto& right = transformed.job_results[index];
        require(left.job_id == right.job_id && left.status == right.status &&
                    left.diagnostic_begin == right.diagnostic_begin &&
                    left.diagnostic_count == right.diagnostic_count &&
                    left.result_region_begin == right.result_region_begin &&
                    left.result_region_count == right.result_region_count &&
                    left.operand_event_begin == right.operand_event_begin &&
                    left.operand_event_count == right.operand_event_count,
                name + " changed job projection");
    }
    for (std::size_t index = 0; index < base.vertices.size(); ++index)
    {
        const auto& left = base.vertices[index];
        const auto& right = transformed.vertices[index];
        require(left.id == right.id &&
                    left.intersection_source_set == right.intersection_source_set &&
                    left.flags == right.flags,
                name + " changed vertex identity/provenance");
    }
    for (std::size_t index = 0; index < base.fragments.size(); ++index)
    {
        const auto& left = base.fragments[index];
        const auto& right = transformed.fragments[index];
        require(left.id == right.id && left.kind == right.kind &&
                    left.major_arc == right.major_arc &&
                    left.positive_source_set == right.positive_source_set &&
                    left.subtraction_source_set == right.subtraction_source_set,
                name + " changed fragment identity/provenance");
    }
    for (std::size_t index = 0; index < base.rings.size(); ++index)
    {
        const auto& left = base.rings[index];
        const auto& right = transformed.rings[index];
        require(left.id == right.id &&
                    left.fragment_reference_begin == right.fragment_reference_begin &&
                    left.fragment_reference_count == right.fragment_reference_count &&
                    left.parent_ring == right.parent_ring && left.depth == right.depth &&
                    left.flags == right.flags,
                name + " changed ring hierarchy");
    }
    for (std::size_t index = 0; index < base.regions.size(); ++index)
    {
        const auto& left = base.regions[index];
        const auto& right = transformed.regions[index];
        require(left.id == right.id && left.outer_ring == right.outer_ring &&
                    left.positive_source_set == right.positive_source_set,
                name + " changed result regions");
    }
    for (std::size_t index = 0; index < base.source_sets.size(); ++index)
    {
        const auto& left = base.source_sets[index];
        const auto& right = transformed.source_sets[index];
        require(left.source_reference_index_begin == right.source_reference_index_begin &&
                    left.source_reference_index_count == right.source_reference_index_count,
                name + " changed source-set projection");
    }
    for (std::size_t index = 0; index < base.source_references.size(); ++index)
        require(base.source_references[index].kind == transformed.source_references[index].kind &&
                    base.source_references[index].role == transformed.source_references[index].role,
                name + " changed source kind/role");
}

void require_unchanged_directed_fragments(const AnalyticResultPacketRecords& base,
                                          const AnalyticResultPacketRecords& transformed,
                                          std::int64_t radius_factor, const std::string& name)
{
    require(base.fragment_references == transformed.fragment_references,
            name + " changed ring traversal");
    for (std::size_t index = 0; index < base.fragments.size(); ++index)
    {
        const auto& left = base.fragments[index];
        const auto& right = transformed.fragments[index];
        require(left.start_vertex == right.start_vertex && left.end_vertex == right.end_vertex &&
                    left.direction == right.direction &&
                    right.radius_nm == left.radius_nm * radius_factor,
                name + " changed directed curve data");
    }
}

void require_unchanged_source_identities(const AnalyticResultPacketRecords& base,
                                         const AnalyticResultPacketRecords& transformed,
                                         const std::string& name)
{
    for (std::size_t index = 0; index < base.source_references.size(); ++index)
    {
        const auto& left = base.source_references[index];
        const auto& right = transformed.source_references[index];
        require(left.operand_id == right.operand_id && left.primary_id == right.primary_id &&
                    left.secondary_id == right.secondary_id,
                name + " changed source identity");
    }
}

void require_translation_relation(const AnalyticResultPacketRecords& base,
                                  const AnalyticResultPacketRecords& transformed, std::int64_t dx,
                                  std::int64_t dy)
{
    require_common_record_fields(base, transformed, "translation");
    require_unchanged_directed_fragments(base, transformed, 1, "translation");
    require_unchanged_source_identities(base, transformed, "translation");
    for (std::size_t index = 0; index < base.vertices.size(); ++index)
        require(transformed.vertices[index].x_nm == base.vertices[index].x_nm + dx &&
                    transformed.vertices[index].y_nm == base.vertices[index].y_nm + dy,
                "translation did not apply its exact coordinate map");
}

void require_rotation_relation(const AnalyticResultPacketRecords& base,
                               const AnalyticResultPacketRecords& transformed)
{
    require_common_record_fields(base, transformed, "rotation_90");
    require_unchanged_directed_fragments(base, transformed, 1, "rotation_90");
    require_unchanged_source_identities(base, transformed, "rotation_90");
    for (std::size_t index = 0; index < base.vertices.size(); ++index)
        require(transformed.vertices[index].x_nm == -base.vertices[index].y_nm &&
                    transformed.vertices[index].y_nm == base.vertices[index].x_nm,
                "rotation_90 did not apply (-y,x)");
}

void require_reflection_relation(const AnalyticResultPacketRecords& base,
                                 const AnalyticResultPacketRecords& transformed)
{
    require_common_record_fields(base, transformed, "reflection");
    require_unchanged_source_identities(base, transformed, "reflection");
    for (std::size_t index = 0; index < base.vertices.size(); ++index)
        require(transformed.vertices[index].x_nm == -base.vertices[index].x_nm &&
                    transformed.vertices[index].y_nm == base.vertices[index].y_nm,
                "reflection did not apply (-x,y)");
    for (std::size_t index = 0; index < base.fragments.size(); ++index)
    {
        const auto& left = base.fragments[index];
        const auto& right = transformed.fragments[index];
        require(right.start_vertex == left.end_vertex && right.end_vertex == left.start_vertex &&
                    right.direction == left.direction && right.radius_nm == left.radius_nm,
                "reflection did not reverse endpoints with net-preserved arc direction");
    }
    for (const auto& ring : base.rings)
        for (std::uint32_t offset = 0; offset < ring.fragment_reference_count; ++offset)
            require(transformed.fragment_references[ring.fragment_reference_begin + offset] ==
                        base.fragment_references[ring.fragment_reference_begin +
                                                 ring.fragment_reference_count - 1 - offset],
                    "reflection did not reverse ring traversal");
}

void require_scaling_relation(const AnalyticResultPacketRecords& base,
                              const AnalyticResultPacketRecords& transformed, std::int64_t factor)
{
    require_common_record_fields(base, transformed, "integer_scaling");
    require_unchanged_directed_fragments(base, transformed, factor, "integer_scaling");
    require_unchanged_source_identities(base, transformed, "integer_scaling");
    for (std::size_t index = 0; index < base.vertices.size(); ++index)
        require(transformed.vertices[index].x_nm == base.vertices[index].x_nm * factor &&
                    transformed.vertices[index].y_nm == base.vertices[index].y_nm * factor,
                "integer scaling did not apply its exact coordinate map");
}

void require_source_renaming_relation(const AnalyticResultPacketRecords& base,
                                      const AnalyticResultPacketRecords& transformed)
{
    require_common_record_fields(base, transformed, "source_id_renaming");
    require_unchanged_directed_fragments(base, transformed, 1, "source_id_renaming");
    for (std::size_t index = 0; index < base.vertices.size(); ++index)
        require(transformed.vertices[index].x_nm == base.vertices[index].x_nm &&
                    transformed.vertices[index].y_nm == base.vertices[index].y_nm,
                "source renaming changed the exact geometry projection");
    for (std::size_t index = 0; index < base.source_references.size(); ++index)
    {
        const auto& left = base.source_references[index];
        const auto& right = transformed.source_references[index];
        require(right.operand_id == left.operand_id + 100 &&
                    right.primary_id == left.primary_id + 1'000 &&
                    right.secondary_id == left.secondary_id + 10'000,
                "source renaming did not apply its explicit ID map");
    }
}

void check_metamorphic(const std::string& name, const AnalyticResultPacketRecords& base,
                       const AnalyticResultPacketRecords& transformed,
                       const SymbolicMeasure& expected, std::vector<std::string>& signatures)
{
    require(validate_analytic_result_packet_topology(transformed) ==
                AnalyticResultPacketLayoutError::none,
            name + " failed exact topology replay");
    require(measure(transformed) == expected, name + " changed its governed analytic measure");
    const std::string digest = packet_digest(transformed);
    require(digest != packet_digest(base), name + " did not alter canonical packet identity");
    signatures.push_back(name + '=' + digest);
}

void check_metamorphic_corpus(std::vector<std::string>& signatures)
{
    const auto capsule_value = capsule();
    const auto island_value = nested_island();
    const SymbolicMeasure capsule_measure{800, 100, 80, 20, 1, 0};
    const SymbolicMeasure island_measure{1088, 0, 288, 0, 2, 1};
    const auto translated_capsule = translated(capsule_value, 13, -17);
    const auto translated_island = translated(island_value, 13, -17);
    require_translation_relation(capsule_value, translated_capsule, 13, -17);
    require_translation_relation(island_value, translated_island, 13, -17);
    check_metamorphic("translation_capsule", capsule_value, translated_capsule, capsule_measure,
                      signatures);
    check_metamorphic("translation_nested_island", island_value, translated_island, island_measure,
                      signatures);

    const auto rotated_capsule = rotated_90(capsule_value);
    const auto rotated_island = rotated_90(island_value);
    require_rotation_relation(capsule_value, rotated_capsule);
    require_rotation_relation(island_value, rotated_island);
    check_metamorphic("rotation_90_capsule", capsule_value, rotated_capsule, capsule_measure,
                      signatures);
    check_metamorphic("rotation_90_nested_island", island_value, rotated_island, island_measure,
                      signatures);

    const auto reflected_capsule = reflected_x(capsule_value);
    const auto reflected_island = reflected_x(island_value);
    require_reflection_relation(capsule_value, reflected_capsule);
    require_reflection_relation(island_value, reflected_island);
    check_metamorphic("reflection_capsule", capsule_value, reflected_capsule, capsule_measure,
                      signatures);
    check_metamorphic("reflection_nested_island", island_value, reflected_island, island_measure,
                      signatures);

    const auto scaled_capsule = scaled(capsule_value, 3);
    const auto scaled_island = scaled(island_value, 3);
    require_scaling_relation(capsule_value, scaled_capsule, 3);
    require_scaling_relation(island_value, scaled_island, 3);
    check_metamorphic("integer_scaling_capsule", capsule_value, scaled_capsule,
                      scaled_measure(capsule_measure, 3), signatures);
    check_metamorphic("integer_scaling_nested_island", island_value, scaled_island,
                      scaled_measure(island_measure, 3), signatures);

    const auto renamed_capsule = renamed_sources(capsule_value);
    const auto renamed_island = renamed_sources(island_value);
    require_source_renaming_relation(capsule_value, renamed_capsule);
    require_source_renaming_relation(island_value, renamed_island);
    check_metamorphic("source_id_renaming_capsule", capsule_value, renamed_capsule, capsule_measure,
                      signatures);
    check_metamorphic("source_id_renaming_nested_island", island_value, renamed_island,
                      island_measure, signatures);
}

std::string signature(const SymbolicMeasure& value)
{
    std::ostringstream out;
    out << value.area_four_rational << '+' << value.area_four_pi << "p/4;"
        << value.perimeter_two_rational << '+' << value.perimeter_two_pi << "p/2;C"
        << value.components << "H" << value.holes << "E"
        << static_cast<std::int64_t>(value.components) - value.holes;
    return out.str();
}

void check(const std::string& name, const AnalyticResultPacketRecords& records,
           const SymbolicMeasure& expected, std::vector<std::string>& signatures)
{
    const auto topology = validate_analytic_result_packet_topology(records);
    require(topology == AnalyticResultPacketLayoutError::none,
            name + " failed exact topology replay: " + std::to_string(static_cast<int>(topology)));
    const SymbolicMeasure actual = measure(records);
    require(actual == expected, name + " closed-form Green measure changed: " + signature(actual));
    signatures.push_back(name + '=' + signature(actual));
}

} // namespace

int main()
{
    std::vector<std::string> signatures;
    check("rectangle", rectangle(), {400, 0, 80, 0, 1, 0}, signatures);
    check("circle", circle(), {0, 400, 0, 40, 1, 0}, signatures);
    check("annulus", annulus(), {0, 336, 0, 56, 1, 1}, signatures);
    check("capsule", capsule(), {800, 100, 80, 20, 1, 0}, signatures);
    check("nested_island", nested_island(), {1088, 0, 288, 0, 2, 1}, signatures);
    std::cout << "ANALYTIC_CLOSED_FORM_INVARIANTS=";
    for (std::size_t index = 0; index < signatures.size(); ++index)
        std::cout << (index == 0 ? "" : "|") << signatures[index];
    std::cout << '\n';
    signatures.clear();
    check_metamorphic_corpus(signatures);
    std::cout << "ANALYTIC_METAMORPHIC_INVARIANTS=";
    for (std::size_t index = 0; index < signatures.size(); ++index)
        std::cout << (index == 0 ? "" : "|") << signatures[index];
    std::cout << '\n';
    return 0;
}

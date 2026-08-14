#include "geometer/analytic_result_packet_topology.h"

#include <boost/multiprecision/cpp_int.hpp>

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
    return 0;
}

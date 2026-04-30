#include "geometer/planar_contours.h"

#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

geometer::PlanarContourSegment segment(double x1, double y1, double x2, double y2)
{
    return {{x1, y1}, {x2, y2}};
}

void append_rect(std::vector<geometer::PlanarContourSegment>* segments, double min_x, double min_y,
                 double max_x, double max_y)
{
    segments->push_back(segment(min_x, min_y, max_x, min_y));
    segments->push_back(segment(max_x, min_y, max_x, max_y));
    segments->push_back(segment(max_x, max_y, min_x, max_y));
    segments->push_back(segment(min_x, max_y, min_x, min_y));
}

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

bool near(double actual, double expected)
{
    return std::fabs(actual - expected) < 1.0e-9;
}

geometer::PlanarContourResult build(const std::vector<geometer::PlanarContourSegment>& segments)
{
    geometer::PlanarContourOptions options;
    options.round_digits = 3;
    options.union_polygons = true;

    geometer::PlanarContourResult result;
    geometer::Status status;
    const int code = geometer::build_planar_contours(segments, options, &result, &status);
    require(code == 0, "build_planar_contours failed: " + status.message);
    return result;
}

std::size_t hole_count(const geometer::PlanarContourResult& result)
{
    std::size_t count = 0;
    for (const geometer::PlanarContourRing& ring : result.rings)
    {
        if (ring.hole)
        {
            ++count;
        }
    }
    return count;
}

double total_abs_area(const geometer::PlanarContourResult& result)
{
    double area = 0.0;
    for (const geometer::PlanarContourRing& ring : result.rings)
    {
        area += std::fabs(ring.signed_area);
    }
    return area;
}

void closed_square()
{
    std::vector<geometer::PlanarContourSegment> segments;
    append_rect(&segments, 0.0, 0.0, 10.0, 10.0);

    const geometer::PlanarContourResult result = build(segments);
    require(result.rings.size() == 1, "closed square should produce one ring");
    require(!result.rings[0].hole, "closed square ring should be exterior");
    require(result.segments.size() == 4, "closed square should produce four contour segments");
    require(near(result.rings[0].signed_area, 100.0), "closed square area should be 100");
}

void duplicate_reversed_segments()
{
    std::vector<geometer::PlanarContourSegment> segments;
    append_rect(&segments, 0.0, 0.0, 10.0, 10.0);
    segments.push_back(segment(10.0, 0.0, 0.0, 0.0));
    segments.push_back(segment(0.0, 10.0, 10.0, 10.0));

    const geometer::PlanarContourResult result = build(segments);
    require(result.rings.size() == 1, "duplicate reversed square should produce one ring");
    require(result.segments.size() == 4, "duplicate reversed edges should be deduped");
    require(near(result.rings[0].signed_area, 100.0), "duplicate reversed area should be 100");
}

void hole_ring()
{
    std::vector<geometer::PlanarContourSegment> segments;
    append_rect(&segments, 0.0, 0.0, 10.0, 10.0);
    append_rect(&segments, 3.0, 3.0, 7.0, 7.0);

    const geometer::PlanarContourResult result = build(segments);
    require(result.rings.size() == 2, "nested square should produce exterior plus hole");
    require(hole_count(result) == 1, "nested square should classify one ring as a hole");
    require(result.segments.size() == 8, "nested square should emit both ring boundaries");
    require(result.rings[0].nesting_depth == 0, "outer ring should have depth zero");
    require(result.rings[1].nesting_depth == 1, "inner ring should have depth one");
    require(near(std::fabs(result.rings[1].signed_area), 16.0), "hole area should be 16");
}

void overlapping_collinear_segments()
{
    std::vector<geometer::PlanarContourSegment> segments = {
        segment(0.0, 0.0, 10.0, 0.0),   segment(2.0, 0.0, 8.0, 0.0),
        segment(10.0, 0.0, 10.0, 10.0), segment(10.0, 10.0, 0.0, 10.0),
        segment(0.0, 10.0, 0.0, 0.0),
    };

    const geometer::PlanarContourResult result = build(segments);
    require(result.rings.size() == 1, "overlapping collinear square should produce one ring");
    require(result.segments.size() == 6, "overlap endpoints should node the bottom edge");
    require(near(result.rings[0].signed_area, 100.0),
            "overlapping collinear square area should be 100");
}

void crossed_noded_linework()
{
    std::vector<geometer::PlanarContourSegment> segments;
    append_rect(&segments, 0.0, 0.0, 10.0, 10.0);
    segments.push_back(segment(0.0, 0.0, 10.0, 10.0));
    segments.push_back(segment(0.0, 10.0, 10.0, 0.0));

    const geometer::PlanarContourResult result = build(segments);
    require(result.rings.size() == 1, "crossed square should union interior faces");
    require(result.segments.size() == 4, "crossed square should remove internal diagonals");
    require(near(result.rings[0].signed_area, 100.0), "crossed square area should be 100");
}

void overlapping_rectangles_union()
{
    std::vector<geometer::PlanarContourSegment> segments;
    append_rect(&segments, 0.0, 0.0, 10.0, 10.0);
    append_rect(&segments, 5.0, 0.0, 15.0, 10.0);

    const geometer::PlanarContourResult result = build(segments);
    require(result.rings.size() == 1, "overlapping rectangles should union to one ring");
    require(result.segments.size() == 8,
            "overlapping rectangle boundary should keep noded vertices");
    require(near(result.rings[0].signed_area, 150.0),
            "overlapping rectangle union area should be 150");
}

void nested_faces_deterministic_order()
{
    std::vector<geometer::PlanarContourSegment> segments;
    append_rect(&segments, 0.0, 0.0, 20.0, 20.0);
    append_rect(&segments, 5.0, 5.0, 15.0, 15.0);
    append_rect(&segments, 8.0, 8.0, 12.0, 12.0);

    const geometer::PlanarContourResult result = build(segments);
    require(result.rings.size() == 3, "three nested squares should produce three rings");
    require(result.rings[0].nesting_depth == 0, "outer ring should be first");
    require(result.rings[1].nesting_depth == 1, "middle ring should be second");
    require(result.rings[2].nesting_depth == 2, "inner island should be third");
    require(!result.rings[0].hole, "outer ring should be exterior");
    require(result.rings[1].hole, "middle ring should be hole");
    require(!result.rings[2].hole, "inner ring should be exterior island");
    require(result.segments.size() == 12, "nested squares should emit all ring boundaries");
    require(near(total_abs_area(result), 516.0), "nested square absolute area should be stable");
}

} // namespace

int main()
{
    try
    {
        closed_square();
        duplicate_reversed_segments();
        hole_ring();
        overlapping_collinear_segments();
        crossed_noded_linework();
        overlapping_rectangles_union();
        nested_faces_deterministic_order();
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }

    return 0;
}

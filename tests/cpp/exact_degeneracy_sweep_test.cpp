#include "geometer/exact_curve_domain.h"
#include "geometer/exact_normalization.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace
{

using geometer::exact::BigInt;
using geometer::exact::ConstructionArena;
using geometer::exact::ConstructionNodeId;
using geometer::exact::Error;
using geometer::exact::ExactCircle;
using geometer::exact::ExactCircularArc;
using geometer::exact::ExactLine;
using geometer::exact::ExactPoint;
using geometer::exact::IntersectionRelation;

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

ConstructionNodeId rational(ConstructionArena& arena, const BigInt& numerator,
                            const BigInt& denominator = 1)
{
    auto result = arena.make_rational(numerator, denominator);
    require(result.error == Error::none && result.node.has_value(), "rational fixture failed");
    return *result.node;
}

ExactPoint point(ConstructionArena& arena, const BigInt& x_numerator, const BigInt& y_numerator,
                 const BigInt& denominator = 1)
{
    return {rational(arena, x_numerator, denominator), rational(arena, y_numerator, denominator)};
}

std::string relation_name(const geometer::exact::ExactIntersectionResult& result)
{
    require(result.error == Error::none, "intersection sweep unexpectedly failed");
    switch (result.relation)
    {
    case IntersectionRelation::disjoint:
        require(result.point_count == 0, "disjoint relation exposed points");
        return "disjoint";
    case IntersectionRelation::point:
        require(result.point_count == 1, "point relation exposed wrong point count");
        return "point";
    case IntersectionRelation::two_points:
        require(result.point_count == 2, "two-point relation exposed wrong point count");
        return "two_points";
    case IntersectionRelation::coincident:
        require(result.point_count == 0, "coincident relation exposed finite points");
        return "coincident";
    }
    require(false, "unknown intersection relation");
    return {};
}

std::string join(const std::vector<std::string>& values)
{
    std::string result;
    for (const auto& value : values)
    {
        if (!result.empty())
            result += ',';
        result += value;
    }
    return result;
}

void append_circle_sweeps(ConstructionArena& arena, std::string& signature)
{
    const ExactCircle unit{point(arena, 0, 0), rational(arena, 1)};
    std::vector<std::string> external;
    for (const BigInt& distance_hundredths : {BigInt(199), BigInt(200), BigInt(201)})
    {
        const ExactCircle other{point(arena, distance_hundredths, 0, 100), rational(arena, 1)};
        external.push_back(
            relation_name(geometer::exact::intersect_exact_circles(arena, unit, other)));
    }
    require(external == std::vector<std::string>({"two_points", "point", "disjoint"}),
            "external tangency boundary changed");
    signature += "external_tangency:" + join(external);

    const ExactCircle outer{point(arena, 0, 0), rational(arena, 2)};
    std::vector<std::string> internal;
    for (const BigInt& distance_hundredths : {BigInt(99), BigInt(100), BigInt(101)})
    {
        const ExactCircle inner{point(arena, distance_hundredths, 0, 100), rational(arena, 1)};
        internal.push_back(
            relation_name(geometer::exact::intersect_exact_circles(arena, outer, inner)));
    }
    require(internal == std::vector<std::string>({"disjoint", "point", "two_points"}),
            "internal tangency boundary changed");
    signature += "|internal_tangency:" + join(internal);

    std::vector<std::string> concentric;
    for (const BigInt& radius_hundredths : {BigInt(99), BigInt(100), BigInt(101)})
    {
        const ExactCircle other{unit.center, rational(arena, radius_hundredths, 100)};
        concentric.push_back(
            relation_name(geometer::exact::intersect_exact_circles(arena, unit, other)));
    }
    require(concentric == std::vector<std::string>({"disjoint", "coincident", "disjoint"}),
            "concentric coincidence boundary changed");
    signature += "|concentric:" + join(concentric);
}

void append_near_collinear_sweep(ConstructionArena& arena, std::string& signature)
{
    const ExactLine base{point(arena, 0, 0), point(arena, 10, 0)};
    std::vector<std::string> relations;
    for (const BigInt& end_y_thousandths : {BigInt(-1), BigInt(0), BigInt(1)})
    {
        const ExactLine candidate{point(arena, 0, 0),
                                  point(arena, 10'000, end_y_thousandths, 1000)};
        relations.push_back(
            relation_name(geometer::exact::intersect_exact_lines(arena, base, candidate)));
    }
    require(relations == std::vector<std::string>({"point", "coincident", "point"}),
            "near-collinear line classification changed");
    signature += "|near_collinear:" + join(relations);
}

void append_half_grid_sweep(ConstructionArena& arena, std::string& signature)
{
    const std::vector<std::pair<BigInt, BigInt>> inputs = {
        {-501, 1000}, {-1, 2},      {-499, 1000}, {499, 1000},  {1, 2},
        {501, 1000},  {1499, 1000}, {3, 2},       {1501, 1000},
    };
    const std::vector<std::int64_t> expected = {-1, -1, 0, 0, 1, 1, 1, 2, 2};
    std::vector<std::string> normalized;
    for (std::size_t index = 0; index < inputs.size(); ++index)
    {
        const ConstructionNodeId node = rational(arena, inputs[index].first, inputs[index].second);
        auto result =
            geometer::exact::normalize_exact_to_integer_nm(arena.budget(), arena.at(node).value());
        require(result.error == Error::none && result.value == expected[index],
                "half-grid ties-away normalization changed");
        normalized.push_back(std::to_string(*result.value));
    }
    signature += "|half_grid:" + join(normalized);
}

void require_arc_endpoint(ConstructionArena& arena, const ExactCircularArc& arc,
                          const std::string& message)
{
    auto result = geometer::exact::point_on_closed_exact_arc(arena, arc, arc.end);
    require(result.error == Error::none && result.value == true, message);
}

void append_arc_boundary_sweep(ConstructionArena& arena, std::string& signature)
{
    const ExactCircle circle{point(arena, 0, 0), rational(arena, 101)};
    const ExactPoint start = point(arena, 101, 0);
    const ExactCircularArc below{circle, start, point(arena, -99, 20), true, false};
    const ExactCircularArc exact{circle, start, point(arena, -101, 0), true, false};
    const ExactCircularArc above{circle, start, point(arena, -99, -20), true, true};
    require_arc_endpoint(arena, below, "sub-180-degree arc rejected");
    require_arc_endpoint(arena, exact, "exact 180-degree arc rejected");
    require_arc_endpoint(arena, above, "super-180-degree arc rejected");

    const ExactCircularArc wrong_below{below.circle, below.start, below.end, true, true};
    const ExactCircularArc wrong_above{above.circle, above.start, above.end, true, false};
    auto invalid_below =
        geometer::exact::point_on_closed_exact_arc(arena, wrong_below, wrong_below.end);
    auto invalid_above =
        geometer::exact::point_on_closed_exact_arc(arena, wrong_above, wrong_above.end);
    require(invalid_below.error == Error::invalid_argument && !invalid_below.value &&
                invalid_above.error == Error::invalid_argument && !invalid_above.value,
            "incoherent 180-degree boundary flag must reject");

    const ExactCircularArc closed{circle, start, start, true, false};
    auto invalid_closed = geometer::exact::point_on_closed_exact_arc(arena, closed, start);
    require(invalid_closed.error == Error::invalid_argument && !invalid_closed.value,
            "single-arc zero/full-circle encoding must reject");
    signature += "|arc_180:minor,semicircle,major|arc_closed:invalid";
}

} // namespace

int main()
{
    geometer::exact::Budget budget({4'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    std::string signature;
    append_circle_sweeps(arena, signature);
    append_near_collinear_sweep(arena, signature);
    append_half_grid_sweep(arena, signature);
    append_arc_boundary_sweep(arena, signature);
    std::cout << "EXACT_DEGENERACY_SWEEP=" << signature << '\n';
    return 0;
}

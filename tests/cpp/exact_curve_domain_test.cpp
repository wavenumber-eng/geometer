#include "geometer/exact_curve_domain.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace
{

using geometer::exact::BigInt;
using geometer::exact::ConstructionArena;
using geometer::exact::ConstructionNodeId;
using geometer::exact::Error;
using geometer::exact::ExactCircle;
using geometer::exact::ExactCircularArc;
using geometer::exact::ExactLine;
using geometer::exact::ExactOrderingResult;
using geometer::exact::ExactPoint;
using geometer::exact::ExactPredicateResult;
using geometer::exact::IntersectionRelation;

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

ConstructionNodeId rational(ConstructionArena& arena, const BigInt& value)
{
    auto result = arena.make_rational(value);
    require(result.error == Error::none && result.node, "curve-domain rational fixture failed");
    return *result.node;
}

ExactPoint point(ConstructionArena& arena, const BigInt& x, const BigInt& y)
{
    return {rational(arena, x), rational(arena, y)};
}

bool require_predicate(const ExactPredicateResult& result, const std::string& message)
{
    require(result.error == Error::none && result.value.has_value(), message);
    return *result.value;
}

std::int8_t require_order(const ExactOrderingResult& result, const std::string& message)
{
    require(result.error == Error::none && result.ordering.has_value(), message);
    return *result.ordering;
}

void append_bool(std::string& signature, bool value)
{
    signature.push_back(value ? '1' : '0');
}

void append_order(std::string& signature, std::int8_t value)
{
    signature.push_back(value < 0 ? '<' : value > 0 ? '>' : '=');
}

void test_carrier_identity_and_segment_domains(std::string& signature)
{
    geometer::exact::Budget budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    const ExactLine horizontal{point(arena, 0, 0), point(arena, 4, 0)};
    const ExactLine reversed_same{point(arena, 10, 0), point(arena, -2, 0)};
    const ExactLine parallel{point(arena, 0, 1), point(arena, 4, 1)};
    append_bool(signature, require_predicate(geometer::exact::same_exact_line_carrier(
                                                 arena, horizontal, reversed_same),
                                             "same line carrier predicate failed"));
    append_bool(signature, require_predicate(geometer::exact::same_exact_line_carrier(
                                                 arena, horizontal, parallel),
                                             "parallel carrier predicate failed"));

    const ExactCircle circle{point(arena, 0, 0), rational(arena, 5)};
    const ExactCircle same_circle{point(arena, 0, 0), rational(arena, 5)};
    const ExactCircle other_radius{point(arena, 0, 0), rational(arena, 4)};
    append_bool(signature, require_predicate(geometer::exact::same_exact_circle_carrier(
                                                 arena, circle, same_circle),
                                             "same circle carrier predicate failed"));
    append_bool(signature, require_predicate(geometer::exact::same_exact_circle_carrier(
                                                 arena, circle, other_radius),
                                             "different circle carrier predicate failed"));

    const ExactPoint middle = point(arena, 2, 0);
    const ExactPoint outside = point(arena, 5, 0);
    const ExactPoint off_line = point(arena, 2, 1);
    append_bool(signature, require_predicate(geometer::exact::point_on_closed_exact_segment(
                                                 arena, horizontal, horizontal.first),
                                             "segment start membership failed"));
    append_bool(signature, require_predicate(geometer::exact::point_on_closed_exact_segment(
                                                 arena, horizontal, middle),
                                             "segment middle membership failed"));
    append_bool(signature, require_predicate(geometer::exact::point_on_closed_exact_segment(
                                                 arena, horizontal, outside),
                                             "segment outside membership failed"));
    append_bool(signature, require_predicate(geometer::exact::point_on_closed_exact_segment(
                                                 arena, horizontal, off_line),
                                             "segment off-carrier membership failed"));

    append_order(signature, require_order(geometer::exact::compare_points_on_exact_line(
                                              arena, reversed_same, point(arena, -1, 0), middle),
                                          "canonical horizontal line ordering failed"));
    const ExactLine vertical{point(arena, 3, 9), point(arena, 3, -2)};
    append_order(signature,
                 require_order(geometer::exact::compare_points_on_exact_line(
                                   arena, vertical, point(arena, 3, -1), point(arena, 3, 7)),
                               "canonical vertical line ordering failed"));
    auto invalid_order =
        geometer::exact::compare_points_on_exact_line(arena, horizontal, middle, off_line);
    require(invalid_order.error == Error::invalid_argument && !invalid_order.ordering,
            "off-carrier line ordering must reject");
}

void test_circle_order_and_arc_domains(std::string& signature)
{
    geometer::exact::Budget budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    const ExactCircle circle{point(arena, 0, 0), rational(arena, 5)};
    const ExactPoint left = point(arena, -5, 0);
    const ExactPoint bottom = point(arena, 0, -5);
    const ExactPoint right = point(arena, 5, 0);
    const ExactPoint top = point(arena, 0, 5);
    const ExactPoint quarter = point(arena, 3, 4);
    const ExactPoint off_circle = point(arena, 1, 1);

    append_order(signature, require_order(geometer::exact::compare_points_on_exact_circle(
                                              arena, circle, left, bottom),
                                          "left-to-bottom circle order failed"));
    append_order(signature, require_order(geometer::exact::compare_points_on_exact_circle(
                                              arena, circle, bottom, right),
                                          "bottom-to-right circle order failed"));
    append_order(signature, require_order(geometer::exact::compare_points_on_exact_circle(
                                              arena, circle, right, top),
                                          "right-to-top circle order failed"));
    append_order(signature, require_order(geometer::exact::compare_points_on_exact_circle(
                                              arena, circle, top, left),
                                          "circle seam wrap order failed"));
    append_order(signature, require_order(geometer::exact::compare_points_on_exact_circle(
                                              arena, circle, left, left),
                                          "equal circle point order failed"));

    const ExactCircularArc lower{circle, left, right, true, false};
    append_bool(signature,
                require_predicate(geometer::exact::point_on_closed_exact_arc(arena, lower, bottom),
                                  "lower half-arc membership failed"));
    append_bool(signature,
                require_predicate(geometer::exact::point_on_closed_exact_arc(arena, lower, top),
                                  "upper point exclusion from lower arc failed"));
    append_bool(signature,
                require_predicate(geometer::exact::point_on_closed_exact_arc(arena, lower, right),
                                  "arc endpoint membership failed"));
    append_bool(signature, require_predicate(
                               geometer::exact::point_on_closed_exact_arc(arena, lower, off_circle),
                               "off-circle arc membership failed"));

    const ExactCircularArc upper_clockwise{circle, left, right, false, false};
    append_bool(signature, require_predicate(geometer::exact::point_on_closed_exact_arc(
                                                 arena, upper_clockwise, top),
                                             "clockwise upper half-arc membership failed"));
    append_bool(signature, require_predicate(geometer::exact::point_on_closed_exact_arc(
                                                 arena, upper_clockwise, bottom),
                                             "clockwise upper half exclusion failed"));

    const ExactCircularArc quarter_arc{circle, right, top, true, false};
    append_bool(signature, require_predicate(geometer::exact::point_on_closed_exact_arc(
                                                 arena, quarter_arc, quarter),
                                             "quarter-arc interior membership failed"));
    append_bool(signature, require_predicate(geometer::exact::point_on_closed_exact_arc(
                                                 arena, quarter_arc, bottom),
                                             "quarter-arc exterior exclusion failed"));

    const ExactCircularArc major{circle, top, right, true, true};
    append_bool(signature,
                require_predicate(geometer::exact::point_on_closed_exact_arc(arena, major, left),
                                  "major-arc left membership failed"));
    append_bool(signature,
                require_predicate(geometer::exact::point_on_closed_exact_arc(arena, major, bottom),
                                  "major-arc bottom membership failed"));
    append_bool(signature,
                require_predicate(geometer::exact::point_on_closed_exact_arc(arena, major, quarter),
                                  "major-arc excluded quadrant changed"));

    const ExactCircularArc incoherent_major{circle, left, right, true, true};
    auto invalid_major =
        geometer::exact::point_on_closed_exact_arc(arena, incoherent_major, bottom);
    require(invalid_major.error == Error::invalid_argument && !invalid_major.value,
            "incoherent half-turn major flag must reject");
    const ExactCircularArc closed_full{circle, left, left, true, false};
    auto invalid_full = geometer::exact::point_on_closed_exact_arc(arena, closed_full, left);
    require(invalid_full.error == Error::invalid_argument && !invalid_full.value,
            "topology-indexed full-circle arc must reject");
}

void test_irrational_intersection_domains(std::string& signature)
{
    geometer::exact::Budget budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    const ExactCircle circle{point(arena, 0, 0), rational(arena, 5)};
    const ExactLine secant{point(arena, -10, 1), point(arena, 10, 1)};
    const auto intersections = geometer::exact::intersect_exact_line_circle(arena, secant, circle);
    require(intersections.error == Error::none &&
                intersections.relation == IntersectionRelation::two_points &&
                intersections.point_count == 2,
            "irrational secant fixture failed");
    const ExactPoint& left = intersections.points[0];
    const ExactPoint& right = intersections.points[1];

    append_bool(signature, require_predicate(geometer::exact::same_exact_line_carrier(
                                                 arena, secant, ExactLine{right, left}),
                                             "irrational split carrier grouping failed"));
    append_order(signature, require_order(geometer::exact::compare_points_on_exact_line(
                                              arena, secant, left, right),
                                          "irrational line split ordering failed"));
    append_order(signature, require_order(geometer::exact::compare_points_on_exact_circle(
                                              arena, circle, left, right),
                                          "irrational circle split ordering failed"));
    append_bool(signature, require_predicate(geometer::exact::exact_points_equal(arena, left, left),
                                             "irrational point equality failed"));
    append_bool(signature,
                require_predicate(geometer::exact::exact_points_equal(arena, left, right),
                                  "distinct irrational point deduplication failed"));
    append_bool(signature, require_predicate(geometer::exact::point_on_closed_exact_segment(
                                                 arena, ExactLine{left, right}, left),
                                             "irrational segment endpoint membership failed"));

    const ExactCircularArc upper_clockwise{circle, point(arena, -5, 0), point(arena, 5, 0), false,
                                           false};
    append_bool(signature, require_predicate(geometer::exact::point_on_closed_exact_arc(
                                                 arena, upper_clockwise, left),
                                             "irrational arc membership failed"));
}

std::uint64_t test_arc_resource_boundary()
{
    geometer::exact::Budget measured({2'000'000'000, 268'435'456});
    {
        ConstructionArena arena(measured);
        const ExactCircle circle{point(arena, 0, 0), rational(arena, 5)};
        const ExactCircularArc arc{circle, point(arena, -5, 0), point(arena, 5, 0), true, false};
        const ExactPoint bottom = point(arena, 0, -5);
        require(require_predicate(geometer::exact::point_on_closed_exact_arc(arena, arc, bottom),
                                  "arc work measurement failed"),
                "arc work fixture must be contained");
    }
    require(measured.usage().owned_bytes == 0,
            "completed arc predicate must release all arena storage");
    const std::uint64_t required_work = measured.usage().work_units;

    geometer::exact::Budget short_budget({required_work - 1, 268'435'456});
    {
        ConstructionArena arena(short_budget);
        const ExactCircle circle{point(arena, 0, 0), rational(arena, 5)};
        const ExactCircularArc arc{circle, point(arena, -5, 0), point(arena, 5, 0), true, false};
        const ExactPoint bottom = point(arena, 0, -5);
        const std::size_t fixture_size = arena.size();
        auto failed = geometer::exact::point_on_closed_exact_arc(arena, arc, bottom);
        require(failed.error == Error::resource_limit_exceeded && !failed.value &&
                    arena.size() == fixture_size,
                "one-unit-short arc predicate must fail with semantic rollback");
    }
    require(short_budget.usage().owned_bytes == 0,
            "failed arc predicate must release all arena storage");
    return required_work;
}

} // namespace

int main()
{
    std::string signature = "ECD1:";
    test_carrier_identity_and_segment_domains(signature);
    signature += ':';
    test_circle_order_and_arc_domains(signature);
    signature += ':';
    test_irrational_intersection_domains(signature);
    const std::uint64_t work = test_arc_resource_boundary();
    std::cout << "EXACT_CURVE_DOMAIN_VECTOR=" << signature << '\n';
    std::cout << "EXACT_CURVE_DOMAIN_WORK=" << work << '\n';
    return 0;
}

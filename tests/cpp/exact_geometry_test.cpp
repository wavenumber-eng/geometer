#include "geometer/exact_artifact.h"
#include "geometer/exact_geometry.h"
#include "geometer/exact_normalization.h"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace
{

using geometer::exact::BigInt;
using geometer::exact::CanonicalRealKind;
using geometer::exact::ConstructionArena;
using geometer::exact::ConstructionNodeId;
using geometer::exact::Error;
using geometer::exact::ExactCircle;
using geometer::exact::ExactIntersectionResult;
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

ConstructionNodeId require_node(const geometer::exact::ConstructionResult& result,
                                const std::string& message)
{
    require(result.error == Error::none && result.node.has_value(), message);
    return *result.node;
}

ConstructionNodeId rational(ConstructionArena& arena, const BigInt& numerator,
                            const BigInt& denominator = 1)
{
    return require_node(arena.make_rational(numerator, denominator), "rational fixture failed");
}

ExactPoint point(ConstructionArena& arena, const BigInt& x, const BigInt& y)
{
    return {rational(arena, x), rational(arena, y)};
}

void require_rational(const ConstructionArena& arena, ConstructionNodeId node,
                      const BigInt& numerator, const BigInt& denominator,
                      const std::string& message)
{
    const auto& value = arena.at(node).value();
    require(value.kind() == CanonicalRealKind::rational && value.numerator() == numerator &&
                value.denominator() == denominator,
            message);
}

void require_sqrt(const ConstructionArena& arena, ConstructionNodeId node, const BigInt& radicand,
                  std::uint32_t ordinal, const std::string& message)
{
    const auto& value = arena.at(node).value();
    require(value.kind() == CanonicalRealKind::irrational && value.polynomial() != nullptr &&
                value.polynomial()->coefficients() == std::vector<BigInt>({-radicand, 0, 1}) &&
                value.root()->ordinal == ordinal,
            message);
}

void require_relation(const ExactIntersectionResult& result, IntersectionRelation relation,
                      std::uint32_t point_count, const std::string& message)
{
    require(result.error == Error::none && result.relation == relation &&
                result.point_count == point_count,
            message);
}

std::string hex_bytes(const std::vector<std::uint8_t>& bytes)
{
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (const std::uint8_t byte : bytes)
        output << std::setw(2) << static_cast<unsigned int>(byte);
    return output.str();
}

ExactIntersectionResult build_line_circle_vector(ConstructionArena& arena)
{
    const ExactLine line{point(arena, 1, -3), point(arena, 1, 3)};
    const ExactCircle circle{point(arena, 0, 0), rational(arena, 2)};
    return geometer::exact::intersect_exact_line_circle(arena, line, circle);
}

void test_line_line_classification_and_atomicity()
{
    geometer::exact::Budget budget({1'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    const ExactLine first{point(arena, 0, 0), point(arena, 2, 2)};
    const ExactLine second{point(arena, 0, 1), point(arena, 2, 0)};
    auto crossing = geometer::exact::intersect_exact_lines(arena, first, second);
    require_relation(crossing, IntersectionRelation::point, 1,
                     "rational line intersection classification changed");
    require_rational(arena, crossing.points[0].x, 2, 3, "line intersection x changed");
    require_rational(arena, crossing.points[0].y, 2, 3, "line intersection y changed");

    const ExactLine horizontal{point(arena, 0, 4), point(arena, 2, 4)};
    const ExactLine parallel{point(arena, 0, 5), point(arena, 2, 5)};
    const std::size_t before_parallel = arena.size();
    auto disjoint = geometer::exact::intersect_exact_lines(arena, horizontal, parallel);
    require_relation(disjoint, IntersectionRelation::disjoint, 0,
                     "parallel lines must be disjoint");
    require(arena.size() == before_parallel,
            "non-point line classification must roll construction temporaries back");

    const ExactLine same_domain{point(arena, -2, 4), point(arena, -1, 4)};
    auto coincident = geometer::exact::intersect_exact_lines(arena, horizontal, same_domain);
    require_relation(coincident, IntersectionRelation::coincident, 0,
                     "same-domain lines must classify as coincident");

    const ExactLine degenerate{horizontal.first, horizontal.first};
    const std::size_t before_invalid = arena.size();
    auto invalid = geometer::exact::intersect_exact_lines(arena, degenerate, parallel);
    require(invalid.error == Error::invalid_argument && arena.size() == before_invalid,
            "zero-length line must reject atomically");
}

void test_line_circle_vectors_and_normalization(std::string& artifact_hex)
{
    geometer::exact::Budget budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    auto secant = build_line_circle_vector(arena);
    require_relation(secant, IntersectionRelation::two_points, 2,
                     "line-circle secant classification changed");
    require_rational(arena, secant.points[0].x, 1, 1, "lower secant x changed");
    require_rational(arena, secant.points[1].x, 1, 1, "upper secant x changed");
    require_sqrt(arena, secant.points[0].y, 3, 0, "lower secant y must be -sqrt(3)");
    require_sqrt(arena, secant.points[1].y, 3, 1, "upper secant y must be sqrt(3)");

    auto lower_nm = geometer::exact::normalize_exact_to_integer_nm(
        budget, arena.at(secant.points[0].y).value());
    auto upper_nm = geometer::exact::normalize_exact_to_integer_nm(
        budget, arena.at(secant.points[1].y).value());
    require(lower_nm.error == Error::none && lower_nm.value == -2 &&
                upper_nm.error == Error::none && upper_nm.value == 2,
            "irrational certified nm normalization changed");

    const ExactCircle circle{point(arena, 0, 0), rational(arena, 2)};
    const ExactLine tangent{point(arena, -3, 2), point(arena, 3, 2)};
    auto tangent_result = geometer::exact::intersect_exact_line_circle(arena, tangent, circle);
    require_relation(tangent_result, IntersectionRelation::point, 1,
                     "line-circle tangency classification changed");
    require_rational(arena, tangent_result.points[0].x, 0, 1, "tangent x changed");
    require_rational(arena, tangent_result.points[0].y, 2, 1, "tangent y changed");

    const ExactLine outside{point(arena, -3, 3), point(arena, 3, 3)};
    const std::size_t before_disjoint = arena.size();
    auto disjoint = geometer::exact::intersect_exact_line_circle(arena, outside, circle);
    require_relation(disjoint, IntersectionRelation::disjoint, 0,
                     "outside line-circle pair must be disjoint");
    require(arena.size() == before_disjoint,
            "disjoint line-circle classification must roll temporaries back");

    const ExactCircle zero_radius{circle.center, rational(arena, 0)};
    const std::size_t before_invalid = arena.size();
    auto invalid = geometer::exact::intersect_exact_line_circle(arena, tangent, zero_radius);
    require(invalid.error == Error::invalid_argument && arena.size() == before_invalid,
            "zero-radius circle must reject atomically");

    geometer::exact::Budget encode_budget({1'000'000'000, 268'435'456});
    auto artifact = geometer::exact::encode_construction_artifact(
        encode_budget, arena,
        {secant.points[0].x, secant.points[0].y, secant.points[1].x, secant.points[1].y});
    require(artifact.error == Error::none && artifact.value.has_value(),
            "intersection artifact encoding failed");
    require(artifact.value->bytes().size() == 1'248,
            "governed intersection artifact byte length changed");
    artifact_hex = hex_bytes(artifact.value->bytes());
    require(!artifact_hex.empty(), "intersection artifact unexpectedly empty");
}

void test_circle_circle_vectors()
{
    geometer::exact::Budget budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    const ExactCircle left{point(arena, 0, 0), rational(arena, 2)};
    const ExactCircle secant_right{point(arena, 2, 0), rational(arena, 2)};
    auto secant = geometer::exact::intersect_exact_circles(arena, left, secant_right);
    require_relation(secant, IntersectionRelation::two_points, 2,
                     "circle-circle secant classification changed");
    require_rational(arena, secant.points[0].x, 1, 1, "circle secant lower x changed");
    require_rational(arena, secant.points[1].x, 1, 1, "circle secant upper x changed");
    require_sqrt(arena, secant.points[0].y, 3, 0, "circle secant lower y must be -sqrt(3)");
    require_sqrt(arena, secant.points[1].y, 3, 1, "circle secant upper y must be sqrt(3)");

    const ExactCircle tangent_right{point(arena, 4, 0), rational(arena, 2)};
    auto tangent = geometer::exact::intersect_exact_circles(arena, left, tangent_right);
    require_relation(tangent, IntersectionRelation::point, 1,
                     "external circle tangency classification changed");
    require_rational(arena, tangent.points[0].x, 2, 1, "circle tangent x changed");
    require_rational(arena, tangent.points[0].y, 0, 1, "circle tangent y changed");

    const ExactCircle outside{point(arena, 5, 0), rational(arena, 2)};
    auto disjoint = geometer::exact::intersect_exact_circles(arena, left, outside);
    require_relation(disjoint, IntersectionRelation::disjoint, 0,
                     "separated circles must be disjoint");
    const ExactCircle coincident{left.center, left.radius};
    auto same = geometer::exact::intersect_exact_circles(arena, left, coincident);
    require_relation(same, IntersectionRelation::coincident, 0, "equal circles must be coincident");
    const ExactCircle concentric{left.center, rational(arena, 1)};
    auto contained = geometer::exact::intersect_exact_circles(arena, left, concentric);
    require_relation(contained, IntersectionRelation::disjoint, 0,
                     "unequal concentric circles must have no finite intersection");

    const ExactCircle large{point(arena, 0, 0), rational(arena, 3)};
    const ExactCircle internally_tangent{point(arena, 2, 0), rational(arena, 1)};
    auto inner_tangent = geometer::exact::intersect_exact_circles(arena, large, internally_tangent);
    require_relation(inner_tangent, IntersectionRelation::point, 1,
                     "internal circle tangency classification changed");
    require_rational(arena, inner_tangent.points[0].x, 3, 1, "internal circle tangent x changed");
    require_rational(arena, inner_tangent.points[0].y, 0, 1, "internal circle tangent y changed");
    const ExactCircle strictly_inside{point(arena, 1, 0), rational(arena, 1)};
    auto no_crossing = geometer::exact::intersect_exact_circles(arena, large, strictly_inside);
    require_relation(no_crossing, IntersectionRelation::disjoint, 0,
                     "strict circle containment must have no finite intersection");
    const ExactCircle negative_radius{left.center, rational(arena, -1)};
    const std::size_t before_invalid = arena.size();
    auto invalid = geometer::exact::intersect_exact_circles(arena, left, negative_radius);
    require(invalid.error == Error::invalid_argument && arena.size() == before_invalid,
            "negative-radius circle must reject atomically");
}

void test_ties_away_and_range_rejection()
{
    geometer::exact::Budget budget({1'000'000'000, 268'435'456});
    auto check = [&](const BigInt& numerator, const BigInt& denominator, std::int64_t expected)
    {
        auto value = geometer::exact::make_canonical_rational(budget, numerator, denominator);
        require(value.error == Error::none && value.value, "normalization fixture failed");
        auto normalized = geometer::exact::normalize_exact_to_integer_nm(budget, *value.value);
        require(normalized.error == Error::none && normalized.value == expected,
                "ties-away rational normalization changed");
    };
    check(499, 1000, 0);
    check(1, 2, 1);
    check(501, 1000, 1);
    check(-499, 1000, 0);
    check(-1, 2, -1);
    check(-501, 1000, -1);

    auto huge = geometer::exact::make_canonical_rational(
        budget, BigInt(std::numeric_limits<std::int64_t>::max()) + 1, 1);
    require(huge.error == Error::none && huge.value, "range fixture failed");
    auto rejected = geometer::exact::normalize_exact_to_integer_nm(budget, *huge.value);
    require(rejected.error == Error::invalid_argument && !rejected.value,
            "out-of-int64 normalized coordinate must reject");

    geometer::exact::Budget boundary_fixture({1'000'000'000, 268'435'456});
    auto half = geometer::exact::make_canonical_rational(boundary_fixture, 1, 2);
    require(half.error == Error::none && half.value, "normalization boundary fixture failed");
    geometer::exact::Budget exact_budget({1'018'432, 1'000'000});
    auto exact = geometer::exact::normalize_exact_to_integer_nm(exact_budget, *half.value);
    require(exact.error == Error::none && exact.value == 1 &&
                exact_budget.usage().work_units == 1'018'432 &&
                exact_budget.usage().owned_bytes == 0,
            "normalization exact success boundary changed");
    geometer::exact::Budget short_budget({1'018'431, 1'000'000});
    auto short_result = geometer::exact::normalize_exact_to_integer_nm(short_budget, *half.value);
    require(short_result.error == Error::resource_limit_exceeded && !short_result.value &&
                short_budget.usage().work_units == 0 && short_budget.usage().owned_bytes == 0,
            "one-unit-short normalization must fail before arithmetic and release storage");
}

std::uint64_t test_exact_resource_boundary_and_rollback()
{
    geometer::exact::Budget measured({2'000'000'000, 268'435'456});
    {
        ConstructionArena arena(measured);
        auto result = build_line_circle_vector(arena);
        require_relation(result, IntersectionRelation::two_points, 2,
                         "resource-boundary measurement failed");
    }
    require(measured.usage().owned_bytes == 0,
            "completed geometric construction must release all arena storage");
    const std::uint64_t required_work = measured.usage().work_units;
    require(required_work == 41'482'728, "geometric construction success work boundary changed: " +
                                             std::to_string(required_work));

    geometer::exact::Budget short_budget({required_work - 1, 268'435'456});
    {
        ConstructionArena arena(short_budget);
        const ExactLine line{point(arena, 1, -3), point(arena, 1, 3)};
        const ExactCircle circle{point(arena, 0, 0), rational(arena, 2)};
        const std::size_t fixture_size = arena.size();
        auto result = geometer::exact::intersect_exact_line_circle(arena, line, circle);
        require(result.error == Error::resource_limit_exceeded && result.point_count == 0 &&
                    arena.size() == fixture_size,
                "one-unit-short geometric construction must fail with semantic rollback");
    }
    require(short_budget.usage().owned_bytes == 0,
            "failed geometric construction must release all owned storage");
    return required_work;
}

} // namespace

int main()
{
    std::string artifact_hex;
    test_line_line_classification_and_atomicity();
    test_line_circle_vectors_and_normalization(artifact_hex);
    test_circle_circle_vectors();
    test_ties_away_and_range_rejection();
    const std::uint64_t required_work = test_exact_resource_boundary_and_rollback();
    std::cout << "GEXPA001_GEOMETRY_HEX=" << artifact_hex << '\n';
    std::cout << "EXACT_GEOMETRY_WORK=" << required_work << '\n';
    return 0;
}

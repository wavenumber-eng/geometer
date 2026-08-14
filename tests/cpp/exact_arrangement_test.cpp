#include "geometer/exact_arrangement.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{

using geometer::exact::BigInt;
using geometer::exact::ConstructionArena;
using geometer::exact::ConstructionNodeId;
using geometer::exact::Error;
using geometer::exact::ExactArrangement;
using geometer::exact::ExactArrangementResult;
using geometer::exact::ExactAtomicCurve;
using geometer::exact::ExactAtomicCurveKind;
using geometer::exact::ExactCircle;
using geometer::exact::ExactCurveMembership;
using geometer::exact::ExactPoint;

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
    require(result.error == Error::none && result.node, "arrangement rational fixture failed");
    return *result.node;
}

ExactPoint point(ConstructionArena& arena, const BigInt& x, const BigInt& y)
{
    return {rational(arena, x), rational(arena, y)};
}

ExactAtomicCurve line(ExactPoint start, ExactPoint end, std::uint64_t occurrence,
                      bool agrees_with_carrier = true)
{
    return {ExactAtomicCurveKind::line,         start, end, {}, true, false,
            {{occurrence, agrees_with_carrier}}};
}

ExactAtomicCurve arc(const ExactCircle& circle, ExactPoint start, ExactPoint end,
                     bool counterclockwise, std::uint64_t occurrence)
{
    return {ExactAtomicCurveKind::circular_arc,
            start,
            end,
            circle,
            counterclockwise,
            false,
            {{occurrence, true}}};
}

struct SquareFixture
{
    ExactPoint a;
    ExactPoint b;
    ExactPoint c;
    ExactPoint d;
};

SquareFixture square_points(ConstructionArena& arena)
{
    return {point(arena, 0, 0), point(arena, 4, 0), point(arena, 4, 4), point(arena, 0, 4)};
}

std::vector<ExactAtomicCurve> square_curves(const SquareFixture& square, bool alternate)
{
    if (!alternate)
        return {
            line(square.b, square.a, 10, true),
            line(square.c, square.b, 11, true),
            line(square.d, square.c, 12, true),
            line(square.a, square.d, 13, true),
        };
    return {
        line(square.c, square.d, 12, true),
        line(square.a, square.b, 10, true),
        line(square.d, square.a, 13, true),
        line(square.b, square.c, 11, true),
    };
}

std::size_t cycle_count(const ExactArrangement& arrangement)
{
    std::vector<bool> visited(arrangement.half_edges().size());
    std::size_t cycles = 0;
    for (std::size_t start = 0; start < visited.size(); ++start)
    {
        if (visited[start])
            continue;
        ++cycles;
        std::uint32_t current = static_cast<std::uint32_t>(start);
        for (std::size_t steps = 0; steps <= visited.size(); ++steps)
        {
            if (visited[current])
            {
                require(current == start, "half-edge cycles must close at their start");
                break;
            }
            visited[current] = true;
            current = arrangement.half_edges()[current].next;
            require(current < visited.size(), "half-edge next index escaped the topology");
        }
    }
    return cycles;
}

std::string signature(const ExactArrangement& arrangement)
{
    std::ostringstream out;
    out << arrangement.vertices().size() << ',' << arrangement.edges().size() << ','
        << arrangement.half_edges().size() << ',' << cycle_count(arrangement) << '|';
    for (const auto& vertex : arrangement.vertices())
    {
        out << '[';
        for (std::uint32_t index = 0; index < vertex.outgoing_count; ++index)
            out << arrangement.outgoing_half_edges()[vertex.outgoing_begin + index] << ',';
        out << ']';
    }
    out << '|';
    for (const auto& edge : arrangement.edges())
    {
        out << edge.start_vertex << '-' << edge.end_vertex << '-'
            << static_cast<unsigned>(edge.kind) << (edge.counterclockwise ? 'c' : 'w')
            << (edge.major_arc ? 'm' : 's') << ':';
        for (std::uint32_t index = 0; index < edge.membership_count; ++index)
        {
            const auto& membership = arrangement.memberships()[edge.membership_begin + index];
            out << membership.occurrence_id << (membership.agrees_with_carrier ? '+' : '-') << ',';
        }
        out << ';';
    }
    out << '|';
    for (const auto& half_edge : arrangement.half_edges())
        out << half_edge.twin << '>' << half_edge.next << '>' << half_edge.previous << ',';
    return out.str();
}

std::string test_square_permutation_and_coincident_membership()
{
    geometer::exact::Budget first_budget({2'000'000'000, 268'435'456});
    ConstructionArena first_arena(first_budget);
    const SquareFixture first_square = square_points(first_arena);
    auto first_curves = square_curves(first_square, false);
    first_curves.push_back(line(first_square.a, first_square.b, 20, true));
    ExactArrangementResult first =
        geometer::exact::build_exact_arrangement(first_arena, first_curves);
    require(first.error == Error::none && first.value, "first square arrangement failed");
    require(first.value->vertices().size() == 4 && first.value->edges().size() == 4 &&
                first.value->half_edges().size() == 8 && first.value->memberships().size() == 5,
            "square topology or coincident membership count changed");
    require(cycle_count(*first.value) == 2, "square must have interior and exterior cycles");
    const std::string first_signature = signature(*first.value);

    geometer::exact::Budget second_budget({2'000'000'000, 268'435'456});
    ConstructionArena second_arena(second_budget);
    const SquareFixture second_square = square_points(second_arena);
    auto second_curves = square_curves(second_square, true);
    second_curves.push_back(line(second_square.b, second_square.a, 20, true));
    ExactArrangementResult second =
        geometer::exact::build_exact_arrangement(second_arena, second_curves);
    require(second.error == Error::none && second.value, "permuted square arrangement failed");
    const std::string second_signature = signature(*second.value);
    require(second_signature == first_signature,
            "curve permutation or reversal changed canonical arrangement topology: " +
                first_signature + " != " + second_signature);
    return first_signature;
}

void test_tangent_circle_outgoing_order()
{
    geometer::exact::Budget budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    const ConstructionNodeId radius = rational(arena, 1);
    const ExactPoint left = point(arena, -2, 0);
    const ExactPoint tangent = point(arena, 0, 0);
    const ExactPoint right = point(arena, 2, 0);
    const ExactCircle left_circle{point(arena, -1, 0), radius};
    const ExactCircle right_circle{point(arena, 1, 0), radius};
    const std::vector<ExactAtomicCurve> curves{
        arc(left_circle, left, tangent, true, 30),
        arc(left_circle, tangent, left, true, 31),
        arc(right_circle, tangent, right, true, 32),
        arc(right_circle, right, tangent, true, 33),
    };
    ExactArrangementResult result = geometer::exact::build_exact_arrangement(arena, curves);
    require(result.error == Error::none && result.value, "tangent-circle arrangement failed");
    require(result.value->vertices().size() == 3 && result.value->edges().size() == 4,
            "tangent-circle topology count changed");
    require(result.value->vertices()[1].outgoing_count == 4,
            "tangent vertex must retain all four analytic germs");
    require(cycle_count(*result.value) == 3,
            "two point-tangent disks must retain two interiors and one exterior cycle");
}

void test_non_atomic_overlap_rejects()
{
    geometer::exact::Budget budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    const SquareFixture square = square_points(arena);
    auto curves = square_curves(square, false);
    curves.push_back(line(square.a, point(arena, 2, 0), 99));
    ExactArrangementResult result = geometer::exact::build_exact_arrangement(arena, curves);
    require(result.error == Error::invalid_argument && !result.value,
            "non-atomic partial overlap must reject");
}

void test_unsplit_crossing_rejects()
{
    geometer::exact::Budget budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    const ExactPoint a = point(arena, 0, 0);
    const ExactPoint b = point(arena, 4, 4);
    const ExactPoint c = point(arena, 0, 4);
    const ExactPoint d = point(arena, 4, 0);
    const std::vector<ExactAtomicCurve> bow_tie{
        line(a, b, 100),
        line(b, c, 101),
        line(c, d, 102),
        line(d, a, 103),
    };
    ExactArrangementResult result = geometer::exact::build_exact_arrangement(arena, bow_tie);
    require(result.error == Error::invalid_argument && !result.value,
            "interior line crossing must be split before arrangement construction");
}

void test_invalid_node_reference_rejects()
{
    geometer::exact::Budget budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    const ExactPoint valid = point(arena, 0, 0);
    const ExactAtomicCurve invalid = line(valid, {999'999, valid.y}, 200);
    ExactArrangementResult result = geometer::exact::build_exact_arrangement(arena, {invalid});
    require(result.error == Error::invalid_argument && !result.value,
            "invalid construction-node reference must be a contract error");
}

void test_contradictory_membership_rejects()
{
    geometer::exact::Budget budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    const ExactPoint start = point(arena, 0, 0);
    const ExactPoint end = point(arena, 4, 0);
    const std::vector<ExactAtomicCurve> contradictory{
        line(start, end, 300, true),
        line(end, start, 300, false),
    };
    ExactArrangementResult result = geometer::exact::build_exact_arrangement(arena, contradictory);
    require(result.error == Error::invalid_argument && !result.value,
            "one occurrence cannot both agree with and oppose its canonical carrier");
}

std::pair<std::uint64_t, std::uint64_t> test_resource_boundaries()
{
    std::uint64_t work = 0;
    std::uint64_t storage = 0;
    std::uint64_t fixture_storage = 0;
    {
        geometer::exact::Budget budget({2'000'000'000, 268'435'456});
        ConstructionArena arena(budget);
        const SquareFixture square = square_points(arena);
        const auto curves = square_curves(square, false);
        const auto baseline = budget.usage();
        fixture_storage = baseline.owned_bytes;
        ExactArrangementResult result = geometer::exact::build_exact_arrangement(arena, curves);
        require(result.error == Error::none && result.value,
                "arrangement resource measurement failed");
        work = budget.usage().work_units - baseline.work_units;
        storage = budget.usage().owned_bytes - baseline.owned_bytes;
    }

    geometer::exact::Budget short_budget(
        {2'000'000'000, fixture_storage + storage - 1, 100'000'000, 100'000'000});
    {
        ConstructionArena arena(short_budget);
        const SquareFixture square = square_points(arena);
        const auto curves = square_curves(square, false);
        const std::size_t fixture_size = arena.size();
        const std::uint64_t baseline_storage = short_budget.usage().owned_bytes;
        require(baseline_storage == fixture_storage, "square fixture storage boundary changed");
        ExactArrangementResult result = geometer::exact::build_exact_arrangement(arena, curves);
        require(result.error == Error::resource_limit_exceeded && !result.value &&
                    arena.size() == fixture_size &&
                    short_budget.usage().owned_bytes == baseline_storage,
                "one-byte-short arrangement allocation must fail before publication");
    }
    require(short_budget.usage().owned_bytes == 0,
            "failed arrangement allocation leaked logical storage");
    return {work, storage};
}

} // namespace

int main()
{
    const std::string square_signature = test_square_permutation_and_coincident_membership();
    test_tangent_circle_outgoing_order();
    test_non_atomic_overlap_rejects();
    test_unsplit_crossing_rejects();
    test_invalid_node_reference_rejects();
    test_contradictory_membership_rejects();
    const auto [work, storage] = test_resource_boundaries();
    std::cout << "EXACT_ARRANGEMENT_VECTOR=EAR1:" << square_signature << '\n';
    std::cout << "EXACT_ARRANGEMENT_WORK=" << work << '\n';
    std::cout << "EXACT_ARRANGEMENT_STORAGE=" << storage << '\n';
    return 0;
}

#include "geometer/exact_construction.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace
{

using geometer::exact::CanonicalRealKind;
using geometer::exact::ConstructionArena;
using geometer::exact::ConstructionKind;
using geometer::exact::ConstructionNodeId;
using geometer::exact::Error;

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

ConstructionNodeId make_square_root(ConstructionArena& arena, int radicand)
{
    const ConstructionNodeId rational =
        require_node(arena.make_rational(radicand), "radicand construction failed");
    return require_node(arena.make_nonnegative_square_root(rational),
                        "square-root construction failed");
}

void test_rational_interning_identities_and_domains()
{
    geometer::exact::Budget budget({1'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    const ConstructionNodeId half =
        require_node(arena.make_rational(1, 2), "half construction failed");
    const ConstructionNodeId equivalent =
        require_node(arena.make_rational(2, 4), "equivalent half construction failed");
    require(half == equivalent && arena.size() == 1,
            "equivalent rationals must intern to one construction node");

    const ConstructionNodeId empty_sum =
        require_node(arena.make_sum({}), "empty sum construction failed");
    const ConstructionNodeId zero = require_node(arena.make_rational(0), "zero failed");
    require(empty_sum == zero, "empty sum must normalize to rational zero");
    const ConstructionNodeId empty_product =
        require_node(arena.make_product({}), "empty product construction failed");
    const ConstructionNodeId one = require_node(arena.make_rational(1), "one failed");
    require(empty_product == one, "empty product must normalize to rational one");

    const ConstructionNodeId four_ninths =
        require_node(arena.make_rational(4, 9), "four ninths failed");
    const ConstructionNodeId two_thirds = require_node(
        arena.make_nonnegative_square_root(four_ninths), "perfect rational root failed");
    require(arena.at(two_thirds).kind() == ConstructionKind::rational &&
                arena.at(two_thirds).value().numerator() == 2 &&
                arena.at(two_thirds).value().denominator() == 3,
            "perfect rational square root must collapse to rational node");

    const std::size_t before_invalid = arena.size();
    auto reciprocal_zero = arena.make_reciprocal(zero);
    require(reciprocal_zero.error == Error::invalid_argument && !reciprocal_zero.node &&
                arena.size() == before_invalid,
            "reciprocal zero must reject without semantic mutation");
    const ConstructionNodeId negative = require_node(arena.make_rational(-1), "negative failed");
    auto negative_root = arena.make_nonnegative_square_root(negative);
    require(negative_root.error == Error::invalid_argument && !negative_root.node &&
                arena.size() == before_invalid + 1,
            "negative square root must reject without semantic mutation");
}

void test_associative_flattening_sorting_and_rational_folding()
{
    geometer::exact::Budget nested_budget({1'000'000'000, 268'435'456});
    ConstructionArena nested_arena(nested_budget);
    const ConstructionNodeId sqrt_two = make_square_root(nested_arena, 2);
    const ConstructionNodeId sqrt_three = make_square_root(nested_arena, 3);
    const ConstructionNodeId pair =
        require_node(nested_arena.make_sum({sqrt_two, sqrt_three}), "pair sum failed");
    const ConstructionNodeId commuted =
        require_node(nested_arena.make_sum({sqrt_three, sqrt_two}), "commuted sum failed");
    require(pair == commuted && nested_arena.at(pair).kind() == ConstructionKind::sum,
            "commuted sums must normalize and intern identically");

    const ConstructionNodeId half = require_node(nested_arena.make_rational(1, 2), "half failed");
    const ConstructionNodeId nested =
        require_node(nested_arena.make_sum({pair, half}), "nested sum failed");

    geometer::exact::Budget flat_budget({1'000'000'000, 268'435'456});
    ConstructionArena flat_arena(flat_budget);
    const ConstructionNodeId flat_two = make_square_root(flat_arena, 2);
    const ConstructionNodeId flat_three = make_square_root(flat_arena, 3);
    const ConstructionNodeId flat_half =
        require_node(flat_arena.make_rational(1, 2), "flat half failed");
    const ConstructionNodeId flat =
        require_node(flat_arena.make_sum({flat_half, flat_three, flat_two}), "flat sum failed");
    require(nested_arena.at(nested).construction_key() == flat_arena.at(flat).construction_key() &&
                nested_arena.at(nested).children().size() == 3 &&
                flat_arena.at(flat).children().size() == 3,
            "associative sums must flatten before sorting and interning");

    geometer::exact::Budget fold_budget({1'000'000'000, 268'435'456});
    ConstructionArena fold_arena(fold_budget);
    const ConstructionNodeId fold_two = make_square_root(fold_arena, 2);
    const ConstructionNodeId fold_half =
        require_node(fold_arena.make_rational(1, 2), "fold half failed");
    const ConstructionNodeId third = require_node(fold_arena.make_rational(1, 3), "third failed");
    const ConstructionNodeId folded = require_node(
        fold_arena.make_sum({fold_two, fold_half, third}), "rational-folded sum failed");
    const ConstructionNodeId five_sixths =
        require_node(fold_arena.make_rational(5, 6), "five sixths failed");
    const ConstructionNodeId explicit_fold = require_node(
        fold_arena.make_sum({five_sixths, fold_two}), "explicit rational-fold sum failed");
    require(folded == explicit_fold && fold_arena.at(folded).children().size() == 2,
            "rational sum terms must fold to one sorted child");

    geometer::exact::Budget identity_budget({1'000'000'000, 268'435'456});
    ConstructionArena identity_arena(identity_budget);
    const ConstructionNodeId identity_two = make_square_root(identity_arena, 2);
    const ConstructionNodeId zero = require_node(identity_arena.make_rational(0), "zero failed");
    const ConstructionNodeId product_zero =
        require_node(identity_arena.make_product({identity_two, zero}), "zero product failed");
    require(product_zero == zero &&
                identity_arena.at(product_zero).kind() == ConstructionKind::rational,
            "zero factor must short-circuit to the rational-zero node");
    const ConstructionNodeId one = require_node(identity_arena.make_rational(1), "one failed");
    const ConstructionNodeId identity =
        require_node(identity_arena.make_product({identity_two, one}), "identity product failed");
    require(identity == identity_two, "multiplicative identity must be removed");
}

void test_rational_collapse_and_distinct_construction_value_identity()
{
    geometer::exact::Budget budget({1'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    const ConstructionNodeId sqrt_two = make_square_root(arena, 2);
    const ConstructionNodeId square =
        require_node(arena.make_product({sqrt_two, sqrt_two}), "sqrt-two square failed");
    require(arena.at(square).kind() == ConstructionKind::rational &&
                arena.at(square).value().numerator() == 2,
            "irrational product yielding two must collapse to rational node");

    const ConstructionNodeId negative_one =
        require_node(arena.make_rational(-1), "negative one failed");
    const ConstructionNodeId negative_sqrt_two =
        require_node(arena.make_product({negative_one, sqrt_two}), "negated sqrt two failed");
    const ConstructionNodeId cancellation = require_node(
        arena.make_sum({sqrt_two, negative_sqrt_two}), "irrational cancellation failed");
    require(arena.at(cancellation).kind() == ConstructionKind::rational &&
                arena.at(cancellation).value().numerator() == 0,
            "irrational cancellation must collapse to rational zero");

    const ConstructionNodeId eight = require_node(arena.make_rational(8), "eight failed");
    const ConstructionNodeId sqrt_eight =
        require_node(arena.make_nonnegative_square_root(eight), "sqrt eight construction failed");
    const ConstructionNodeId half = require_node(arena.make_rational(1, 2), "half failed");
    const ConstructionNodeId alternate =
        require_node(arena.make_product({sqrt_eight, half}), "sqrt-eight-half construction failed");
    require(arena.at(alternate).value_key() == arena.at(sqrt_two).value_key() &&
                arena.at(alternate).construction_key() != arena.at(sqrt_two).construction_key(),
            "equal values from distinct expressions need equal value keys and distinct "
            "construction keys");
}

void test_allocation_order_independent_keys()
{
    geometer::exact::Budget left_budget({1'000'000'000, 268'435'456});
    ConstructionArena left(left_budget);
    const ConstructionNodeId left_two = make_square_root(left, 2);
    const ConstructionNodeId left_three = make_square_root(left, 3);
    const ConstructionNodeId left_sum =
        require_node(left.make_sum({left_two, left_three}), "left sum failed");

    geometer::exact::Budget right_budget({1'000'000'000, 268'435'456});
    ConstructionArena right(right_budget);
    const ConstructionNodeId right_three = make_square_root(right, 3);
    const ConstructionNodeId right_two = make_square_root(right, 2);
    const ConstructionNodeId right_sum =
        require_node(right.make_sum({right_three, right_two}), "right sum failed");
    require(left.at(left_sum).construction_key() == right.at(right_sum).construction_key() &&
                left.at(left_sum).value_key() == right.at(right_sum).value_key(),
            "allocation and traversal order must not alter complete keys");
}

void test_transaction_work_boundary_and_rollback()
{
    geometer::exact::Budget exact_budget({4'893, 1'000'000});
    {
        ConstructionArena arena(exact_budget);
        const ConstructionNodeId half =
            require_node(arena.make_rational(1, 2), "boundary half failed");
        const ConstructionNodeId third =
            require_node(arena.make_rational(1, 3), "boundary third failed");
        const ConstructionNodeId sum =
            require_node(arena.make_sum({half, third}), "boundary sum failed");
        require(arena.at(sum).value().numerator() == 5 &&
                    arena.at(sum).value().denominator() == 6 &&
                    exact_budget.usage().work_units == 4'893,
                "construction success work boundary must be platform independent");
    }
    require(exact_budget.usage().owned_bytes == 0,
            "destroyed construction arena must release all retained storage");

    geometer::exact::Budget short_budget({4'892, 1'000'000});
    {
        ConstructionArena arena(short_budget);
        const ConstructionNodeId half =
            require_node(arena.make_rational(1, 2), "short boundary half failed");
        const ConstructionNodeId third =
            require_node(arena.make_rational(1, 3), "short boundary third failed");
        const std::uint64_t retained_storage = short_budget.usage().owned_bytes;
        auto failed = arena.make_sum({half, third});
        require(failed.error == Error::resource_limit_exceeded && !failed.node &&
                    arena.size() == 2 && short_budget.usage().work_units == 3'881 &&
                    short_budget.usage().owned_bytes == retained_storage,
                "one-unit-short construction must preserve prior nodes and roll new state back");
    }
    require(short_budget.usage().owned_bytes == 0,
            "failed construction arena must release prior retained storage at destruction");
}

void test_binary_child_allocation_preflight()
{
    geometer::exact::Budget measured({1'000'000'000, 268'435'456});
    std::uint64_t fixture_work = 0;
    {
        ConstructionArena arena(measured);
        require_node(arena.make_rational(1), "binary fixture one failed");
        require_node(arena.make_rational(2), "binary fixture two failed");
        fixture_work = measured.usage().work_units;
    }
    geometer::exact::Budget short_budget({fixture_work + 31, 268'435'456});
    {
        ConstructionArena arena(short_budget);
        const ConstructionNodeId one =
            require_node(arena.make_rational(1), "short binary fixture one failed");
        const ConstructionNodeId two =
            require_node(arena.make_rational(2), "short binary fixture two failed");
        const std::uint64_t retained_storage = short_budget.usage().owned_bytes;
        auto failed = arena.make_sum(one, two);
        require(failed.error == Error::resource_limit_exceeded && !failed.node &&
                    arena.size() == 2 && short_budget.usage().work_units == fixture_work &&
                    short_budget.usage().owned_bytes == retained_storage,
                "binary child allocation must be preflighted before vector materialization");
    }
    require(short_budget.usage().owned_bytes == 0,
            "binary preflight failure must release all retained storage at destruction");
}

} // namespace

int main()
{
    test_rational_interning_identities_and_domains();
    test_associative_flattening_sorting_and_rational_folding();
    test_rational_collapse_and_distinct_construction_value_identity();
    test_allocation_order_independent_keys();
    test_transaction_work_boundary_and_rollback();
    test_binary_child_allocation_preflight();
    return 0;
}

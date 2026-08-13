#include "geometer/exact_factorization.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace
{
using geometer::exact::BigInt;
void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

void test_reducible_and_irreducible_factorization()
{
    geometer::exact::Budget budget({1'000'000'000, 268'435'456});
    auto candidate = geometer::exact::make_primitive_polynomial(budget, {0, -8, 0, 1});
    require(candidate.value.has_value(), "candidate setup failed");
    auto factored = geometer::exact::factor_primitive_polynomial(budget, *candidate.value);
    require(factored.error == geometer::exact::Error::none && factored.value.has_value(),
            "reducible candidate factorization failed");
    const auto& factors = factored.value->factors();
    require(factors.size() == 2 && factors[0].coefficients() == std::vector<BigInt>({-8, 0, 1}) &&
                factors[1].coefficients() == std::vector<BigInt>({0, 1}),
            "deterministic candidate factors changed");
    auto positive_sqrt_eight =
        geometer::exact::select_unique_factor_root(budget, *factored.value, 2, 4, 0);
    require(positive_sqrt_eight.error == geometer::exact::Error::none &&
                positive_sqrt_eight.factor_index == 0 && positive_sqrt_eight.root_ordinal == 1,
            "exact interval selection must identify the positive sqrt(8) factor and root");
    auto rational_zero =
        geometer::exact::select_unique_factor_root(budget, *factored.value, -1, 1, 0);
    require(rational_zero.error == geometer::exact::Error::none &&
                rational_zero.factor_index == 1 && rational_zero.root_ordinal == 0,
            "exact interval selection must identify the rational zero factor");
    auto ambiguous = geometer::exact::select_unique_factor_root(budget, *factored.value, -4, 4, 0);
    require(ambiguous.error == geometer::exact::Error::resource_limit_exceeded,
            "an interval containing several candidate roots must fail closed for refinement");

    auto irreducible = geometer::exact::make_primitive_polynomial(budget, {1, 0, -10, 0, 1});
    require(irreducible.value.has_value(), "irreducible setup failed");
    auto single = geometer::exact::factor_primitive_polynomial(budget, *irreducible.value);
    require(single.error == geometer::exact::Error::none && single.value.has_value() &&
                single.value->factors().size() == 1 &&
                single.value->factors()[0].coefficients() == irreducible.value->coefficients(),
            "irreducible polynomial classification changed");
}

void test_factorization_resource_limit()
{
    geometer::exact::Budget budget({1'000'000, 1'000'000});
    auto candidate = geometer::exact::make_primitive_polynomial(budget, {0, -8, 0, 1});
    require(candidate.value.has_value(), "limited candidate setup failed");
    const auto storage = budget.usage().owned_bytes;
    auto failed = geometer::exact::factor_primitive_polynomial(budget, *candidate.value);
    require(failed.error == geometer::exact::Error::resource_limit_exceeded &&
                budget.usage().owned_bytes == storage,
            "factorization storage preflight must fail closed");
}
} // namespace

int main()
{
    test_reducible_and_irreducible_factorization();
    test_factorization_resource_limit();
    return 0;
}

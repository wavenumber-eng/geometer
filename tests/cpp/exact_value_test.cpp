#include "geometer/exact_value.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

geometer::exact::PolynomialFactorSet factor(geometer::exact::Budget& budget,
                                            std::initializer_list<int> coefficients)
{
    auto polynomial = geometer::exact::make_primitive_polynomial(
        budget, std::vector<geometer::exact::BigInt>(coefficients.begin(), coefficients.end()));
    require(polynomial.value.has_value(), "value polynomial setup failed");
    auto factors = geometer::exact::factor_primitive_polynomial(budget, *polynomial.value);
    require(factors.value.has_value(), "value factorization setup failed");
    return std::move(*factors.value);
}

void test_canonical_rational_and_irrational_values()
{
    geometer::exact::Budget budget({1'000'000'000, 268'435'456});
    auto mixed = factor(budget, {0, -8, 0, 1});
    auto zero = geometer::exact::make_canonical_real(budget, mixed, -1, 1, 0);
    require(zero.error == geometer::exact::Error::none && zero.value.has_value() &&
                zero.value->kind() == geometer::exact::CanonicalRealKind::rational &&
                zero.value->numerator() == 0 && zero.value->denominator() == 1,
            "degree-one factor must collapse to canonical rational zero");
    auto sqrt_eight = geometer::exact::make_canonical_real(budget, mixed, 2, 4, 0);
    require(sqrt_eight.error == geometer::exact::Error::none && sqrt_eight.value.has_value() &&
                sqrt_eight.value->kind() == geometer::exact::CanonicalRealKind::irrational &&
                sqrt_eight.value->polynomial()->coefficients() ==
                    std::vector<geometer::exact::BigInt>({-8, 0, 1}) &&
                sqrt_eight.value->root()->ordinal == 1 &&
                sqrt_eight.value->root()->precision == 0 &&
                sqrt_eight.value->root()->interval_k == 2 &&
                sqrt_eight.value->root()->thom_signs == std::vector<std::int8_t>({1, 1}),
            "selected factor/root must become canonical irrational identity");
    auto sign = geometer::exact::sign_of_canonical_real(budget, *sqrt_eight.value);
    require(sign.error == geometer::exact::Error::none && sign.ordering == 1,
            "positive irrational sign changed");
    auto order = geometer::exact::compare_canonical_reals(budget, *zero.value, *sqrt_eight.value);
    require(order.error == geometer::exact::Error::none && order.ordering == -1,
            "rational/irrational exact ordering changed");
    auto zero_sign = geometer::exact::sign_of_canonical_real(budget, *zero.value);
    require(zero_sign.error == geometer::exact::Error::none && zero_sign.ordering == 0,
            "canonical rational zero sign changed");
}

void test_irrational_equality_and_cross_polynomial_order()
{
    geometer::exact::Budget budget({1'000'000'000, 268'435'456});
    auto sqrt_two_factors = factor(budget, {-2, 0, 1});
    auto left = geometer::exact::make_canonical_real(budget, sqrt_two_factors, 1, 2, 0);
    auto same = geometer::exact::make_canonical_real(budget, sqrt_two_factors, 1, 2, 0);
    auto sqrt_three_factors = factor(budget, {-3, 0, 1});
    auto right = geometer::exact::make_canonical_real(budget, sqrt_three_factors, 1, 2, 0);
    auto negative = geometer::exact::make_canonical_real(budget, sqrt_two_factors, -2, -1, 0);
    require(left.value && same.value && right.value && negative.value,
            "irrational comparison setup failed");
    auto equal = geometer::exact::compare_canonical_reals(budget, *left.value, *same.value);
    require(equal.error == geometer::exact::Error::none && equal.ordering == 0,
            "canonical minimal-polynomial/root identity must compare equal");
    auto ordered = geometer::exact::compare_canonical_reals(budget, *left.value, *right.value);
    require(ordered.error == geometer::exact::Error::none && ordered.ordering == -1,
            "cross-polynomial interval refinement must order sqrt(2) below sqrt(3)");
    auto same_polynomial =
        geometer::exact::compare_canonical_reals(budget, *negative.value, *left.value);
    require(same_polynomial.error == geometer::exact::Error::none && same_polynomial.ordering == -1,
            "same-polynomial root ordinals must determine exact order");
    auto negative_sign = geometer::exact::sign_of_canonical_real(budget, *negative.value);
    require(negative_sign.error == geometer::exact::Error::none && negative_sign.ordering == -1,
            "negative irrational sign changed");
}

void test_value_resource_and_refinement_outcomes()
{
    geometer::exact::Budget budget({1'000'000'000, 268'435'456});
    auto mixed = factor(budget, {0, -8, 0, 1});
    auto ambiguous = geometer::exact::make_canonical_real(budget, mixed, -4, 4, 0);
    require(ambiguous.selection_status ==
                    geometer::exact::FactorRootSelectionStatus::needs_refinement &&
                ambiguous.error == geometer::exact::Error::none && !ambiguous.value,
            "canonical construction must preserve the non-error refinement outcome");

    geometer::exact::Budget limited({1'000'000'000, 1'000});
    auto exhausted = geometer::exact::make_canonical_real(limited, mixed, -1, 1, 0);
    require(exhausted.error == geometer::exact::Error::resource_limit_exceeded &&
                limited.usage().owned_bytes == 0,
            "canonical rational construction must fail closed on storage exhaustion");
}

void test_platform_independent_32_bit_limb_budget()
{
    geometer::exact::Budget fixture_budget({1'000'000'000, 268'435'456});
    const geometer::exact::BigInt numerator = geometer::exact::BigInt(1) << 40;
    auto polynomial = geometer::exact::make_primitive_polynomial(fixture_budget, {-numerator, 1});
    require(polynomial.value.has_value(), "40-bit rational polynomial setup failed");
    auto factors = geometer::exact::factor_primitive_polynomial(fixture_budget, *polynomial.value);
    require(factors.value.has_value(), "40-bit rational factor setup failed");

    geometer::exact::Budget exact_budget({25'627, 1'000'000});
    auto exact = geometer::exact::make_canonical_real(exact_budget, *factors.value, numerator - 1,
                                                      numerator + 1, 0);
    require(exact.error == geometer::exact::Error::none && exact.value.has_value() &&
                exact.value->numerator() == numerator && exact_budget.usage().work_units == 25'627,
            "40-bit rational construction must use deterministic 32-bit limb work units");

    geometer::exact::Budget short_budget({25'626, 1'000'000});
    auto short_result = geometer::exact::make_canonical_real(short_budget, *factors.value,
                                                             numerator - 1, numerator + 1, 0);
    require(short_result.error == geometer::exact::Error::resource_limit_exceeded &&
                short_budget.usage().work_units == 25'600 && short_budget.usage().owned_bytes == 0,
            "one-unit-short 40-bit construction must fail at the same phase on every target");
}
} // namespace

int main()
{
    test_canonical_rational_and_irrational_values();
    test_irrational_equality_and_cross_polynomial_order();
    test_value_resource_and_refinement_outcomes();
    test_platform_independent_32_bit_limb_budget();
    return 0;
}

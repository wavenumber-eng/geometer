#include "geometer/exact_expression.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace
{

using geometer::exact::BigInt;
using geometer::exact::CanonicalReal;
using geometer::exact::CanonicalRealKind;
using geometer::exact::CanonicalRealResult;
using geometer::exact::Error;

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

CanonicalReal make_rational(geometer::exact::Budget& budget, const BigInt& numerator,
                            const BigInt& denominator = 1)
{
    auto result = geometer::exact::make_canonical_rational(budget, numerator, denominator);
    require(result.error == Error::none && result.value.has_value(),
            "rational fixture construction failed");
    return std::move(*result.value);
}

CanonicalReal make_irrational(geometer::exact::Budget& budget,
                              const std::vector<BigInt>& coefficients, std::uint32_t root_ordinal)
{
    auto result = geometer::exact::make_canonical_irrational(budget, coefficients, root_ordinal);
    require(result.error == Error::none && result.value.has_value(),
            "irrational fixture construction failed");
    return std::move(*result.value);
}

void require_rational(const CanonicalRealResult& result, const BigInt& numerator,
                      const BigInt& denominator, const std::string& message)
{
    require(result.error == Error::none && result.value.has_value() &&
                result.value->kind() == CanonicalRealKind::rational &&
                result.value->numerator() == numerator &&
                result.value->denominator() == denominator,
            message);
}

void require_irrational(const CanonicalRealResult& result, const std::vector<BigInt>& coefficients,
                        std::uint32_t ordinal, const std::string& message)
{
    if (!(result.error == Error::none && result.value.has_value() &&
          result.value->kind() == CanonicalRealKind::irrational &&
          result.value->polynomial()->coefficients() == coefficients &&
          result.value->root()->ordinal == ordinal))
    {
        std::cerr << message << ": error=" << static_cast<int>(result.error);
        if (result.value)
        {
            std::cerr << " kind=" << static_cast<int>(result.value->kind());
            if (result.value->polynomial())
            {
                std::cerr << " polynomial=";
                for (const BigInt& coefficient : result.value->polynomial()->coefficients())
                    std::cerr << coefficient << ',';
                std::cerr << " ordinal=" << result.value->root()->ordinal;
            }
        }
        std::cerr << '\n';
        std::exit(1);
    }
}

void test_rational_arithmetic_and_domain_errors()
{
    geometer::exact::Budget budget({1'000'000'000, 268'435'456});
    auto half = make_rational(budget, 1, 2);
    auto third = make_rational(budget, 1, 3);
    auto sum = geometer::exact::add_canonical_reals(budget, half, third);
    require_rational(sum, 5, 6, "rational addition must reduce exactly");
    auto product = geometer::exact::multiply_canonical_reals(budget, half, third);
    require_rational(product, 1, 6, "rational multiplication must reduce exactly");
    auto reciprocal = geometer::exact::reciprocal_canonical_real(budget, half);
    require_rational(reciprocal, 2, 1, "rational reciprocal changed");

    auto four_ninths = make_rational(budget, 4, 9);
    auto square_root = geometer::exact::nonnegative_square_root_canonical_real(budget, four_ninths);
    require_rational(square_root, 2, 3, "perfect-square rational must collapse to rational");

    auto zero = make_rational(budget, 0);
    auto reciprocal_zero = geometer::exact::reciprocal_canonical_real(budget, zero);
    require(reciprocal_zero.error == Error::invalid_argument && !reciprocal_zero.value,
            "reciprocal zero must be a deterministic domain error");
    auto negative = make_rational(budget, -1);
    auto negative_root = geometer::exact::nonnegative_square_root_canonical_real(budget, negative);
    require(negative_root.error == Error::invalid_argument && !negative_root.value,
            "negative square root must be a deterministic domain error");
}

void test_mixed_rational_and_irrational_arithmetic()
{
    geometer::exact::Budget budget({1'000'000'000, 268'435'456});
    auto sqrt_two = make_irrational(budget, {-2, 0, 1}, 1);
    auto half = make_rational(budget, 1, 2);
    auto translated = geometer::exact::add_canonical_reals(budget, sqrt_two, half);
    require_irrational(translated, {-7, -4, 4}, 1, "sqrt(2)+1/2 canonical value changed");

    auto negative_three_halves = make_rational(budget, -3, 2);
    auto scaled =
        geometer::exact::multiply_canonical_reals(budget, sqrt_two, negative_three_halves);
    require_irrational(scaled, {-9, 0, 2}, 0, "-3/2*sqrt(2) canonical value changed");

    auto inverse = geometer::exact::reciprocal_canonical_real(budget, sqrt_two);
    require_irrational(inverse, {-1, 0, 2}, 1, "reciprocal sqrt(2) canonical value changed");
    geometer::exact::Budget nested_budget({1'000'000'000, 268'435'456});
    auto nested_root =
        geometer::exact::nonnegative_square_root_canonical_real(nested_budget, sqrt_two);
    require_irrational(nested_root, {-2, 0, 0, 0, 1}, 1,
                       "nonnegative sqrt(sqrt(2)) canonical value changed");
}

void test_irrational_binary_arithmetic_and_rational_collapse()
{
    geometer::exact::Budget budget({1'000'000'000, 268'435'456});
    auto sqrt_two = make_irrational(budget, {-2, 0, 1}, 1);
    auto sqrt_three = make_irrational(budget, {-3, 0, 1}, 1);
    geometer::exact::Budget sum_budget({1'000'000'000, 268'435'456});
    auto sum = geometer::exact::add_canonical_reals(sum_budget, sqrt_two, sqrt_three);
    require_irrational(sum, {1, 0, -10, 0, 1}, 3, "sqrt(2)+sqrt(3) canonical value changed");
    geometer::exact::Budget product_budget({1'000'000'000, 268'435'456});
    auto product = geometer::exact::multiply_canonical_reals(product_budget, sqrt_two, sqrt_three);
    require_irrational(product, {-6, 0, 1}, 1, "sqrt(2)*sqrt(3) canonical value changed");

    auto second_sqrt_two = make_irrational(budget, {-2, 0, 1}, 1);
    geometer::exact::Budget square_budget({1'000'000'000, 268'435'456});
    auto square =
        geometer::exact::multiply_canonical_reals(square_budget, sqrt_two, second_sqrt_two);
    require_rational(square, 2, 1, "irrational product must collapse to rational two");

    auto negative_sqrt_two = make_irrational(budget, {-2, 0, 1}, 0);
    geometer::exact::Budget cancellation_budget({1'000'000'000, 268'435'456});
    auto cancellation =
        geometer::exact::add_canonical_reals(cancellation_budget, sqrt_two, negative_sqrt_two);
    require_rational(cancellation, 0, 1, "irrational cancellation must collapse to zero");

    auto sqrt_eight = make_irrational(budget, {-8, 0, 1}, 1);
    auto half = make_rational(budget, 1, 2);
    geometer::exact::Budget alternate_path_budget({1'000'000'000, 268'435'456});
    auto alternate_path =
        geometer::exact::multiply_canonical_reals(alternate_path_budget, sqrt_eight, half);
    require_irrational(alternate_path, {-2, 0, 1}, 1,
                       "sqrt(8)/2 must share the canonical sqrt(2) value");
    geometer::exact::Budget equality_budget({1'000'000'000, 268'435'456});
    auto equal =
        geometer::exact::compare_canonical_reals(equality_budget, sqrt_two, *alternate_path.value);
    require(equal.error == Error::none && equal.ordering == 0,
            "distinct expression paths must compare as the same exact value");

    auto negative_one = make_rational(budget, -1);
    geometer::exact::Budget negate_budget({1'000'000'000, 268'435'456});
    auto negated = geometer::exact::multiply_canonical_reals(negate_budget, sqrt_two, negative_one);
    require_irrational(negated, {-2, 0, 1}, 0,
                       "multiplication by negative one must select negative sqrt(2)");
}

void test_resource_boundary_and_rollback()
{
    geometer::exact::Budget fixtures({1'000'000'000, 268'435'456});
    auto sqrt_two = make_irrational(fixtures, {-2, 0, 1}, 1);
    auto half = make_rational(fixtures, 1, 2);

    geometer::exact::Budget measured({1'000'000'000, 268'435'456});
    auto complete = geometer::exact::add_canonical_reals(measured, sqrt_two, half);
    require(complete.error == Error::none && complete.value.has_value(),
            "expression boundary measurement failed");
    const std::uint64_t required_work = measured.usage().work_units;
    require(required_work == 3'879'048,
            "expression success work boundary must be platform independent");
    complete.value.reset();
    require(measured.usage().owned_bytes == 0,
            "completed expression temporaries must release with the result");

    geometer::exact::Budget short_budget({3'879'047, 268'435'456});
    auto short_result = geometer::exact::add_canonical_reals(short_budget, sqrt_two, half);
    require(short_result.error == Error::resource_limit_exceeded && !short_result.value &&
                short_budget.usage().work_units == 3'490'248 &&
                short_budget.usage().owned_bytes == 0,
            "one-unit-short expression must fail after work with semantic/storage rollback");
}

} // namespace

int main()
{
    test_rational_arithmetic_and_domain_errors();
    test_mixed_rational_and_irrational_arithmetic();
    test_irrational_binary_arithmetic_and_rational_collapse();
    test_resource_boundary_and_rollback();
    return 0;
}

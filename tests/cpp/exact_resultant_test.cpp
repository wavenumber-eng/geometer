#include "geometer/exact_resultant.h"

#include <cstdint>
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

geometer::exact::Polynomial make_polynomial(geometer::exact::Budget& budget,
                                            const std::vector<BigInt>& coefficients)
{
    auto result = geometer::exact::make_primitive_polynomial(budget, coefficients);
    require(result.error == geometer::exact::Error::none && result.value.has_value(),
            "polynomial fixture construction failed");
    return std::move(*result.value);
}

void test_sum_and_product_resultants()
{
    geometer::exact::Budget budget({1'000'000'000, 200'000'000});
    auto sqrt_two = make_polynomial(budget, {-2, 0, 1});
    auto sqrt_three = make_polynomial(budget, {-3, 0, 1});
    auto sum = geometer::exact::make_square_free_resultant(
        budget, sqrt_two, sqrt_three, geometer::exact::ResultantOperation::sum);
    require(sum.error == geometer::exact::Error::none && sum.value.has_value(),
            "sqrt(2)+sqrt(3) resultant failed");
    require(sum.value->coefficients() == std::vector<BigInt>({1, 0, -10, 0, 1}),
            "sqrt(2)+sqrt(3) resultant polynomial changed");
    auto sum_roots = geometer::exact::isolate_real_roots(budget, *sum.value, 16);
    require(sum_roots.error == geometer::exact::Error::none && sum_roots.value.has_value() &&
                sum_roots.value->roots().size() == 4,
            "sum resultant must compose with exact real-root isolation");

    auto product = geometer::exact::make_square_free_resultant(
        budget, sqrt_two, sqrt_three, geometer::exact::ResultantOperation::product);
    require(product.error == geometer::exact::Error::none && product.value.has_value(),
            "sqrt(2)*sqrt(3) resultant failed");
    require(product.value->coefficients() == std::vector<BigInt>({-6, 0, 1}),
            "sqrt(2)*sqrt(3) square-free resultant changed");
    auto product_roots = geometer::exact::isolate_real_roots(budget, *product.value, 16);
    require(product_roots.error == geometer::exact::Error::none &&
                product_roots.value.has_value() && product_roots.value->roots().size() == 2,
            "product resultant must compose with exact real-root isolation");
}

void test_square_free_resultant_retains_multiple_factors_for_selection()
{
    geometer::exact::Budget budget({1'000'000'000, 200'000'000});
    auto sqrt_two = make_polynomial(budget, {-2, 0, 1});
    auto sum = geometer::exact::make_square_free_resultant(
        budget, sqrt_two, sqrt_two, geometer::exact::ResultantOperation::sum);
    require(sum.error == geometer::exact::Error::none && sum.value.has_value(),
            "same-polynomial sum resultant failed");
    require(sum.value->coefficients() == std::vector<BigInt>({0, -8, 0, 1}),
            "square-free sum resultant must retain all candidate irreducible factors");
}

void test_nonmonic_resultants()
{
    geometer::exact::Budget budget({1'000'000'000, 200'000'000});
    auto inverse_sqrt_two = make_polynomial(budget, {-1, 0, 2});
    auto sqrt_three = make_polynomial(budget, {-3, 0, 1});
    auto sum = geometer::exact::make_square_free_resultant(
        budget, inverse_sqrt_two, sqrt_three, geometer::exact::ResultantOperation::sum);
    require(sum.error == geometer::exact::Error::none && sum.value.has_value() &&
                sum.value->coefficients() == std::vector<BigInt>({25, 0, -28, 0, 4}),
            "nonmonic sum resultant changed");
    auto product = geometer::exact::make_square_free_resultant(
        budget, inverse_sqrt_two, sqrt_three, geometer::exact::ResultantOperation::product);
    require(product.error == geometer::exact::Error::none && product.value.has_value() &&
                product.value->coefficients() == std::vector<BigInt>({-3, 0, 2}),
            "nonmonic product resultant changed");
}

void test_unary_polynomial_transforms()
{
    geometer::exact::Budget budget({1'000'000'000, 200'000'000});
    auto sqrt_two = make_polynomial(budget, {-2, 0, 1});
    auto reciprocal = geometer::exact::make_reciprocal_polynomial(budget, sqrt_two);
    require(reciprocal.error == geometer::exact::Error::none && reciprocal.value.has_value() &&
                reciprocal.value->coefficients() == std::vector<BigInt>({-1, 0, 2}),
            "reciprocal polynomial transform changed");
    auto square_root = geometer::exact::make_square_root_polynomial(budget, sqrt_two);
    require(square_root.error == geometer::exact::Error::none && square_root.value.has_value() &&
                square_root.value->coefficients() == std::vector<BigInt>({-2, 0, 0, 0, 1}),
            "square-root polynomial transform changed");
}

void test_rational_translation_and_scaling()
{
    geometer::exact::Budget budget({1'000'000'000, 200'000'000});
    auto sqrt_two = make_polynomial(budget, {-2, 0, 1});
    auto translated = geometer::exact::make_translated_polynomial(budget, sqrt_two, 1, 2);
    require(translated.error == geometer::exact::Error::none && translated.value &&
                translated.value->coefficients() == std::vector<BigInt>({-7, -4, 4}),
            "translation by one half polynomial changed");
    auto scaled = geometer::exact::make_scaled_polynomial(budget, sqrt_two, -3, 2);
    require(scaled.error == geometer::exact::Error::none && scaled.value &&
                scaled.value->coefficients() == std::vector<BigInt>({-9, 0, 2}),
            "scaling by negative three halves polynomial changed");
}

void test_resultant_limits_fail_closed()
{
    geometer::exact::Budget budget({1'000'000'000, 200'000'000});
    auto degree_nine = make_polynomial(budget, {1, 1, 1, 1, 1, 1, 1, 1, 1, 1});
    const std::uint64_t before_work = budget.usage().work_units;
    const std::uint64_t before_storage = budget.usage().owned_bytes;
    auto invalid = geometer::exact::make_square_free_resultant(
        budget, degree_nine, degree_nine, static_cast<geometer::exact::ResultantOperation>(255));
    require(invalid.error == geometer::exact::Error::invalid_argument &&
                budget.usage().work_units == before_work &&
                budget.usage().owned_bytes == before_storage,
            "unknown resultant operations must be structurally rejected without work");
    auto excessive = geometer::exact::make_square_free_resultant(
        budget, degree_nine, degree_nine, geometer::exact::ResultantOperation::sum);
    require(excessive.error == geometer::exact::Error::resource_limit_exceeded &&
                budget.usage().work_units == before_work &&
                budget.usage().owned_bytes == before_storage,
            "degree-overflowing resultant must fail before phase work or allocation");

    geometer::exact::Budget limited({1'000, 100'000});
    auto left = make_polynomial(limited, {-2, 0, 1});
    auto right = make_polynomial(limited, {-3, 0, 1});
    const std::uint64_t retained_storage = limited.usage().owned_bytes;
    auto exhausted = geometer::exact::make_square_free_resultant(
        limited, left, right, geometer::exact::ResultantOperation::sum);
    require(exhausted.error == geometer::exact::Error::resource_limit_exceeded &&
                limited.usage().owned_bytes == retained_storage,
            "resultant phase preflight must fail without temporary storage leakage");

    geometer::exact::Budget transform_budget({1'000'000'000, 200'000'000});
    auto sqrt_two = make_polynomial(transform_budget, {-2, 0, 1});
    const std::uint64_t transform_work = transform_budget.usage().work_units;
    const std::uint64_t transform_storage = transform_budget.usage().owned_bytes;
    const BigInt oversized = BigInt(1) << 16'384;
    auto oversized_translation =
        geometer::exact::make_translated_polynomial(transform_budget, sqrt_two, oversized, 1);
    require(oversized_translation.error == geometer::exact::Error::resource_limit_exceeded &&
                transform_budget.usage().work_units == transform_work &&
                transform_budget.usage().owned_bytes == transform_storage,
            "oversized rational translation must fail before phase work or allocation");
    auto invalid_scale = geometer::exact::make_scaled_polynomial(transform_budget, sqrt_two, 0, 1);
    require(invalid_scale.error == geometer::exact::Error::invalid_argument &&
                transform_budget.usage().work_units == transform_work &&
                transform_budget.usage().owned_bytes == transform_storage,
            "zero rational scale must reject structurally without phase work");
}

} // namespace

int main()
{
    test_sum_and_product_resultants();
    test_square_free_resultant_retains_multiple_factors_for_selection();
    test_nonmonic_resultants();
    test_unary_polynomial_transforms();
    test_rational_translation_and_scaling();
    test_resultant_limits_fail_closed();
    return 0;
}

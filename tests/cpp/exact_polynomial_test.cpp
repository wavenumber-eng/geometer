#include "geometer/exact_polynomial.h"

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

void test_primitive_normalization()
{
    geometer::exact::Budget budget({1'000'000, 1'000'000});
    {
        const std::vector<BigInt> coefficients = {4, 0, -2, 0};
        auto result = geometer::exact::make_primitive_polynomial(budget, coefficients);
        require(result.error == geometer::exact::Error::none && result.value.has_value(),
                "primitive polynomial construction failed");
        require(result.value->coefficients() == std::vector<BigInt>({-2, 0, 1}),
                "content, sign, or trailing-zero normalization changed");
        require(result.value->degree() == 2, "normalized degree changed");
        require(budget.usage().owned_bytes > 0, "live polynomial must own charged storage");
    }
    require(budget.usage().owned_bytes == 0, "destroyed polynomial must release storage");
}

void test_sqrt_two_roots_and_thom_signs()
{
    geometer::exact::Budget budget({10'000'000, 10'000'000});
    auto polynomial =
        geometer::exact::make_primitive_polynomial(budget, std::vector<BigInt>({-2, 0, 1}));
    require(polynomial.error == geometer::exact::Error::none && polynomial.value.has_value(),
            "sqrt(2) polynomial setup failed");
    auto isolated = geometer::exact::isolate_real_roots(budget, *polynomial.value, 16);
    require(isolated.error == geometer::exact::Error::none && isolated.value.has_value(),
            "sqrt(2) root isolation failed");
    const auto& roots = isolated.value->roots();
    require(roots.size() == 2, "sqrt(2) polynomial must have two real roots");
    require(roots[0].ordinal == 0 && roots[0].precision == 0 && roots[0].interval_k == -2,
            "negative sqrt(2) canonical interval changed");
    require(roots[1].ordinal == 1 && roots[1].precision == 0 && roots[1].interval_k == 1,
            "positive sqrt(2) canonical interval changed");
    require(roots[0].thom_signs == std::vector<std::int8_t>({-1, 1}),
            "negative sqrt(2) Thom signs changed");
    require(roots[1].thom_signs == std::vector<std::int8_t>({1, 1}),
            "positive sqrt(2) Thom signs changed");
}

void test_canonical_precision_transition()
{
    geometer::exact::Budget budget({10'000'000, 10'000'000});
    auto polynomial =
        geometer::exact::make_primitive_polynomial(budget, std::vector<BigInt>({1, -16, 32}));
    require(polynomial.error == geometer::exact::Error::none && polynomial.value.has_value(),
            "precision-transition polynomial setup failed");
    auto isolated = geometer::exact::isolate_real_roots(budget, *polynomial.value, 16);
    require(isolated.error == geometer::exact::Error::none && isolated.value.has_value(),
            "precision-transition isolation failed");
    const auto& roots = isolated.value->roots();
    require(roots.size() == 2 && roots[0].precision == 2 && roots[0].interval_k == 0 &&
                roots[1].precision == 2 && roots[1].interval_k == 1,
            "smallest adjacent dyadic cells must be selected deterministically");
}

void test_reducible_square_free_thom_zero_signs()
{
    geometer::exact::Budget budget({1'000'000'000, 100'000'000});
    auto polynomial =
        geometer::exact::make_primitive_polynomial(budget, std::vector<BigInt>({20, 0, -12, 0, 1}));
    require(polynomial.error == geometer::exact::Error::none && polynomial.value.has_value(),
            "reducible square-free polynomial setup failed");
    auto isolated = geometer::exact::isolate_real_roots(budget, *polynomial.value, 16);
    require(isolated.error == geometer::exact::Error::none && isolated.value.has_value(),
            "reducible square-free polynomial isolation failed: " +
                std::to_string(static_cast<int>(isolated.error)) +
                " work=" + std::to_string(budget.usage().work_units) +
                " storage=" + std::to_string(budget.usage().owned_bytes) +
                " predicates=" + std::to_string(budget.usage().exact_predicate_calls) +
                " refinements=" + std::to_string(budget.usage().interval_refinement_steps));
    const auto& roots = isolated.value->roots();
    require(roots.size() == 4, "reducible square-free polynomial must have four real roots");
    require(roots[1].thom_signs == std::vector<std::int8_t>({1, 0, -1, 1}),
            "negative sqrt(2) must carry an exact zero second-derivative Thom sign");
    require(roots[2].thom_signs == std::vector<std::int8_t>({-1, 0, 1, 1}),
            "positive sqrt(2) must carry an exact zero second-derivative Thom sign");
    require(budget.usage().exact_predicate_calls > 0 &&
                budget.usage().interval_refinement_steps > 0,
            "successful root isolation must consume governed predicate and refinement budgets");
}

void test_empty_real_root_set_is_successful()
{
    geometer::exact::Budget budget({10'000'000, 10'000'000});
    auto polynomial =
        geometer::exact::make_primitive_polynomial(budget, std::vector<BigInt>({1, 0, 1}));
    require(polynomial.error == geometer::exact::Error::none && polynomial.value.has_value(),
            "empty-root polynomial setup failed");
    auto isolated = geometer::exact::isolate_real_roots(budget, *polynomial.value, 16);
    require(isolated.error == geometer::exact::Error::none && isolated.value.has_value() &&
                isolated.value->roots().empty(),
            "a polynomial without real roots must return successful empty isolation");
}

void test_square_free_normalization()
{
    geometer::exact::Budget budget({10'000'000, 10'000'000});
    auto repeated =
        geometer::exact::make_primitive_polynomial(budget, std::vector<BigInt>({0, 0, 1}));
    require(repeated.error == geometer::exact::Error::none && repeated.value.has_value(),
            "square-free normalization setup failed");
    auto normalized = geometer::exact::make_square_free_polynomial(budget, *repeated.value);
    require(normalized.error == geometer::exact::Error::none && normalized.value.has_value() &&
                normalized.value->coefficients() == std::vector<BigInt>({0, 1}),
            "square-free normalization must remove repeated polynomial factors");
}

void test_fail_closed_polynomial_and_root_limits()
{
    geometer::exact::Budget invalid_budget({1000, 1000});
    auto invalid =
        geometer::exact::make_primitive_polynomial(invalid_budget, std::vector<BigInt>({0, 0}));
    require(invalid.error == geometer::exact::Error::invalid_argument &&
                invalid_budget.usage().work_units == 0,
            "zero polynomial must be structurally rejected without work");
    std::vector<BigInt> excessive_degree(66, 1);
    auto degree_failure =
        geometer::exact::make_primitive_polynomial(invalid_budget, excessive_degree);
    require(degree_failure.error == geometer::exact::Error::invalid_argument &&
                invalid_budget.usage().work_units == 0,
            "degree above 64 must be structurally rejected without work");
    auto coefficient_failure = geometer::exact::make_primitive_polynomial(
        invalid_budget, std::vector<BigInt>({1, BigInt(1) << 16384}));
    require(coefficient_failure.error == geometer::exact::Error::resource_limit_exceeded &&
                invalid_budget.usage().work_units == 0,
            "coefficient above 16384 bits must fail before polynomial allocation");

    geometer::exact::Budget root_budget({10'000'000, 10'000'000});
    auto repeated =
        geometer::exact::make_primitive_polynomial(root_budget, std::vector<BigInt>({0, 0, 1}));
    require(repeated.error == geometer::exact::Error::none && repeated.value.has_value(),
            "repeated-root setup failed");
    const std::uint64_t before_work = root_budget.usage().work_units;
    const std::uint64_t before_storage = root_budget.usage().owned_bytes;
    auto rejected = geometer::exact::isolate_real_roots(root_budget, *repeated.value, 16);
    require(rejected.error == geometer::exact::Error::invalid_argument &&
                root_budget.usage().work_units > before_work &&
                root_budget.usage().owned_bytes == before_storage,
            "square-free rejection must retain work and roll back temporary storage");

    geometer::exact::Budget limited_budget({1000, 1000});
    auto limited =
        geometer::exact::make_primitive_polynomial(limited_budget, std::vector<BigInt>({-2, 0, 1}));
    require(limited.error == geometer::exact::Error::none && limited.value.has_value(),
            "limited root setup failed");
    const std::uint64_t limited_storage = limited_budget.usage().owned_bytes;
    auto exhausted = geometer::exact::isolate_real_roots(limited_budget, *limited.value, 16);
    require(exhausted.error == geometer::exact::Error::resource_limit_exceeded &&
                limited_budget.usage().owned_bytes == limited_storage,
            "root preflight failure must not leak temporary storage");
}

void test_predicate_and_refinement_limits_are_monotonic()
{
    geometer::exact::Budget predicate_budget({10'000'000, 10'000'000, 10, 100});
    auto predicate_polynomial = geometer::exact::make_primitive_polynomial(
        predicate_budget, std::vector<BigInt>({-2, 0, 1}));
    require(predicate_polynomial.error == geometer::exact::Error::none &&
                predicate_polynomial.value.has_value(),
            "predicate-limit polynomial setup failed");
    const std::uint64_t predicate_storage = predicate_budget.usage().owned_bytes;
    auto predicate_failure =
        geometer::exact::isolate_real_roots(predicate_budget, *predicate_polynomial.value, 16);
    require(predicate_failure.error == geometer::exact::Error::resource_limit_exceeded &&
                predicate_budget.usage().exact_predicate_calls == 10 &&
                predicate_budget.usage().owned_bytes == predicate_storage,
            "predicate exhaustion must be typed, bounded, and release temporary storage");
    const std::uint64_t predicate_work = predicate_budget.usage().work_units;
    const std::uint64_t predicate_refinements = predicate_budget.usage().interval_refinement_steps;
    auto predicate_retry =
        geometer::exact::isolate_real_roots(predicate_budget, *predicate_polynomial.value, 16);
    require(predicate_retry.error == geometer::exact::Error::resource_limit_exceeded &&
                predicate_budget.usage().work_units >= predicate_work &&
                predicate_budget.usage().exact_predicate_calls == 10 &&
                predicate_budget.usage().interval_refinement_steps == predicate_refinements &&
                predicate_budget.usage().owned_bytes == predicate_storage,
            "predicate exhaustion retry must not reset counters or leak temporary storage");

    geometer::exact::Budget refinement_budget({10'000'000, 10'000'000, 100'000, 1});
    auto refinement_polynomial = geometer::exact::make_primitive_polynomial(
        refinement_budget, std::vector<BigInt>({-2, 0, 1}));
    require(refinement_polynomial.error == geometer::exact::Error::none &&
                refinement_polynomial.value.has_value(),
            "refinement-limit polynomial setup failed");
    const std::uint64_t refinement_storage = refinement_budget.usage().owned_bytes;
    auto refinement_failure =
        geometer::exact::isolate_real_roots(refinement_budget, *refinement_polynomial.value, 16);
    require(refinement_failure.error == geometer::exact::Error::resource_limit_exceeded &&
                refinement_budget.usage().interval_refinement_steps == 1 &&
                refinement_budget.usage().exact_predicate_calls > 0 &&
                refinement_budget.usage().owned_bytes == refinement_storage,
            "refinement exhaustion must be typed, bounded, and release temporary storage");
    const std::uint64_t refinement_predicates = refinement_budget.usage().exact_predicate_calls;
    auto refinement_retry =
        geometer::exact::isolate_real_roots(refinement_budget, *refinement_polynomial.value, 16);
    require(refinement_retry.error == geometer::exact::Error::resource_limit_exceeded &&
                refinement_budget.usage().interval_refinement_steps == 1 &&
                refinement_budget.usage().exact_predicate_calls >= refinement_predicates &&
                refinement_budget.usage().owned_bytes == refinement_storage,
            "refinement exhaustion retry must preserve monotonic counters and storage ownership");
}

} // namespace

int main()
{
    test_primitive_normalization();
    test_sqrt_two_roots_and_thom_signs();
    test_canonical_precision_transition();
    test_reducible_square_free_thom_zero_signs();
    test_empty_real_root_set_is_successful();
    test_square_free_normalization();
    test_fail_closed_polynomial_and_root_limits();
    test_predicate_and_refinement_limits_are_monotonic();
    return 0;
}

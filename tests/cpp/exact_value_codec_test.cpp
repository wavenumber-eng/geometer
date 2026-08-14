#include "geometer/exact_value_codec.h"

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

geometer::exact::CanonicalReal make_value(geometer::exact::Budget& budget,
                                          const std::vector<BigInt>& coefficients,
                                          const BigInt& lower, const BigInt& upper)
{
    auto polynomial = geometer::exact::make_primitive_polynomial(budget, coefficients);
    require(polynomial.value.has_value(), "codec polynomial setup failed");
    auto factors = geometer::exact::factor_primitive_polynomial(budget, *polynomial.value);
    require(factors.value.has_value(), "codec factor setup failed");
    auto value = geometer::exact::make_canonical_real(budget, *factors.value, lower, upper, 0);
    require(value.value.has_value(), "codec canonical value setup failed");
    return std::move(*value.value);
}

void test_rational_zero_scalar_golden()
{
    geometer::exact::Budget budget({1'000'000'000, 268'435'456});
    auto zero = make_value(budget, {0, 1}, -1, 1);
    auto encoded = geometer::exact::encode_canonical_real(budget, zero);
    const std::vector<std::uint8_t> expected = {
        1, 0, 0, 0, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        1, 0, 0, 0, 1,  0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
    };
    require(encoded.error == geometer::exact::Error::none && encoded.value.has_value() &&
                encoded.value->bytes() == expected,
            "canonical rational-zero scalar bytes changed");
}

void test_irrational_scalar_golden()
{
    geometer::exact::Budget budget({1'000'000'000, 268'435'456});
    auto sqrt_eight = make_value(budget, {-8, 0, 1}, 2, 4);
    auto encoded = geometer::exact::encode_canonical_real(budget, sqrt_eight);
    const std::vector<std::uint8_t> expected = {
        2, 0, 0, 0, 72, 0, 0, 0, 3, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 8, 0, 0, 0,
        0, 0, 0, 0, 0,  0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
        0, 0, 0, 0, 1,  0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 1, 1, 0, 0,
    };
    require(encoded.error == geometer::exact::Error::none && encoded.value.has_value() &&
                encoded.value->bytes() == expected,
            "canonical sqrt(8) scalar bytes changed");
}

void test_encoding_budget_failure()
{
    geometer::exact::Budget fixture_budget({1'000'000'000, 268'435'456});
    auto zero = make_value(fixture_budget, {0, 1}, -1, 1);
    geometer::exact::Budget limited({255, 1'000'000});
    auto failed = geometer::exact::encode_canonical_real(limited, zero);
    require(failed.error == geometer::exact::Error::resource_limit_exceeded &&
                limited.usage().work_units == 0 && limited.usage().owned_bytes == 0,
            "scalar encoder must fail before work and release reserved storage");
}

void test_large_negative_rational_golden_and_budget_boundary()
{
    geometer::exact::Budget fixture_budget({1'000'000'000, 268'435'456});
    const BigInt magnitude = (BigInt(1) << 136) + 0x0102030405;
    auto negative = make_value(fixture_budget, {magnitude, 1}, -magnitude - 1, -magnitude + 1);
    const std::vector<std::uint8_t> expected = {
        1, 0, 0, 0, 48, 0, 0, 0, 2, 0, 0, 0, 18, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0,  1, 2, 3, 4, 5, 0, 0, 1,  0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
    };
    geometer::exact::Budget exact_budget({384, 1'000'000});
    auto encoded = geometer::exact::encode_canonical_real(exact_budget, negative);
    require(encoded.error == geometer::exact::Error::none && encoded.value.has_value() &&
                encoded.value->bytes() == expected && exact_budget.usage().work_units == 384,
            "large signed scalar bytes and linear work boundary changed");

    geometer::exact::Budget short_budget({383, 1'000'000});
    auto short_result = geometer::exact::encode_canonical_real(short_budget, negative);
    require(short_result.error == geometer::exact::Error::resource_limit_exceeded &&
                short_budget.usage().work_units == 0 && short_budget.usage().owned_bytes == 0,
            "one-unit-short large scalar encoding must fail identically before emission");
}
} // namespace

int main()
{
    test_rational_zero_scalar_golden();
    test_irrational_scalar_golden();
    test_encoding_budget_failure();
    test_large_negative_rational_golden_and_budget_boundary();
    return 0;
}

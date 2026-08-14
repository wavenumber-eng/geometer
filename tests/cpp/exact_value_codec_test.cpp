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
} // namespace

int main()
{
    test_rational_zero_scalar_golden();
    test_irrational_scalar_golden();
    test_encoding_budget_failure();
    return 0;
}

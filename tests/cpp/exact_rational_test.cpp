#include "geometer/exact_rational.h"

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

std::string hexadecimal(const std::vector<std::uint8_t>& bytes)
{
    static constexpr char digits[] = "0123456789abcdef";
    std::string result;
    result.reserve(bytes.size() * 2);
    for (const std::uint8_t byte : bytes)
    {
        result.push_back(digits[byte >> 4]);
        result.push_back(digits[byte & 0x0f]);
    }
    return result;
}

void test_integer_encoding()
{
    require(hexadecimal(geometer::exact::encode_canonical_integer(0)) == "0000000000000000",
            "zero integer encoding changed");
    require(hexadecimal(geometer::exact::encode_canonical_integer(1)) == "010000000100000001000000",
            "positive integer encoding changed");
    require(hexadecimal(geometer::exact::encode_canonical_integer(-256)) ==
                "020000000200000001000000",
            "negative integer encoding changed");
}

void test_rational_normalization_and_interning()
{
    geometer::exact::Budget budget({1'000'000, 1'000'000});
    {
        geometer::exact::RationalArena arena(budget);
        const BigInt numerator = (BigInt(1) << 160) * 6;
        const auto first = arena.intern(numerator, -9);
        require(first.error == geometer::exact::Error::none && first.id.has_value(),
                "large rational should normalize");
        const geometer::exact::Rational& value = arena.at(*first.id);
        require(value.numerator() == -(BigInt(1) << 161),
                "large rational numerator did not reduce exactly");
        require(value.denominator() == 3, "large rational denominator did not reduce exactly");

        const auto duplicate = arena.intern(-(BigInt(1) << 162), 6);
        require(duplicate.error == geometer::exact::Error::none && duplicate.id == first.id,
                "equivalent rational did not intern to one identity");

        const auto zero = arena.intern(0, -77);
        require(zero.error == geometer::exact::Error::none && zero.id.has_value(),
                "zero rational should normalize");
        require(arena.at(*zero.id).numerator() == 0 && arena.at(*zero.id).denominator() == 1,
                "zero rational must be encoded only as 0/1");
        require(budget.usage().owned_bytes > 0, "live arena values must own charged storage");
    }
    require(budget.usage().owned_bytes == 0, "destroyed arena must release live storage");
    require(budget.usage().work_units > 0, "executed work must remain monotonically consumed");
}

void test_invalid_and_late_budget_failure()
{
    geometer::exact::Budget invalid_budget({100, 100});
    geometer::exact::RationalArena invalid_arena(invalid_budget);
    const auto invalid = invalid_arena.intern(1, 0);
    require(invalid.error == geometer::exact::Error::invalid_argument,
            "zero denominator must be rejected");
    require(invalid_budget.usage().work_units == 0 && invalid_budget.usage().owned_bytes == 0,
            "structural rejection must not consume arithmetic budget");

    geometer::exact::Budget budget({10, 100});
    geometer::exact::RationalArena arena(budget);
    const auto failed = arena.intern(1, 2);
    require(failed.error == geometer::exact::Error::resource_limit_exceeded,
            "second-phase reservation should fail");
    require(arena.size() == 0 && budget.usage().owned_bytes == 0,
            "late failure must roll back semantic state and live storage");
    require(budget.usage().work_units == 6,
            "completed first-phase work must remain monotonically consumed");

    const auto retry = arena.intern(1, 2);
    require(retry.error == geometer::exact::Error::resource_limit_exceeded,
            "retry must observe the reduced remaining work budget");
    require(budget.usage().work_units == 6, "an unstarted retry phase must not consume work");
}

void test_rational_encoding()
{
    geometer::exact::Budget budget({1000, 1000});
    geometer::exact::RationalArena arena(budget);
    const auto result = arena.intern(-2, 3);
    require(result.error == geometer::exact::Error::none && result.id.has_value(),
            "-2/3 setup failed");
    require(hexadecimal(geometer::exact::encode_canonical_rational(arena.at(*result.id))) ==
                "0100000020000000020000000100000002000000010000000100000003000000",
            "canonical rational scalar encoding changed");
}

} // namespace

int main()
{
    test_integer_encoding();
    test_rational_normalization_and_interning();
    test_invalid_and_late_budget_failure();
    test_rational_encoding();
    return 0;
}

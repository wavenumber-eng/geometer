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
    geometer::exact::Budget budget({10'000, 10'000});
    auto zero = geometer::exact::encode_canonical_integer(budget, 0);
    auto positive = geometer::exact::encode_canonical_integer(budget, 1);
    auto negative = geometer::exact::encode_canonical_integer(budget, -256);
    require(zero.error == geometer::exact::Error::none && zero.value.has_value(),
            "zero integer encoding failed");
    require(positive.error == geometer::exact::Error::none && positive.value.has_value(),
            "positive integer encoding failed");
    require(negative.error == geometer::exact::Error::none && negative.value.has_value(),
            "negative integer encoding failed");
    require(hexadecimal(zero.value->bytes()) == "0000000000000000",
            "zero integer encoding changed");
    require(hexadecimal(positive.value->bytes()) == "010000000100000001000000",
            "positive integer encoding changed");
    require(hexadecimal(negative.value->bytes()) == "020000000200000001000000",
            "negative integer encoding changed");
    require(budget.usage().owned_bytes > 0, "live encodings must own charged storage");
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

    geometer::exact::Budget budget({10, 1000});
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
    auto encoded = geometer::exact::encode_canonical_rational(budget, arena.at(*result.id));
    require(encoded.error == geometer::exact::Error::none && encoded.value.has_value(),
            "canonical rational encoding failed");
    require(hexadecimal(encoded.value->bytes()) ==
                "0100000020000000020000000100000002000000010000000100000003000000",
            "canonical rational scalar encoding changed");
}

void test_encoding_budget_failures_are_typed_and_rollback_storage()
{
    const BigInt large_value = (BigInt(1) << 4096) - 1;

    geometer::exact::Budget storage_budget({100'000, 32});
    const auto storage_failure =
        geometer::exact::encode_canonical_integer(storage_budget, large_value);
    require(storage_failure.error == geometer::exact::Error::resource_limit_exceeded &&
                !storage_failure.value.has_value(),
            "encoding storage exhaustion must return a typed failure");
    require(storage_budget.usage().owned_bytes == 0 && storage_budget.usage().work_units == 0,
            "encoding storage preflight failure must not consume or retain resources");

    geometer::exact::Budget work_budget({1, 10'000});
    const auto work_failure = geometer::exact::encode_canonical_integer(work_budget, large_value);
    require(work_failure.error == geometer::exact::Error::resource_limit_exceeded &&
                !work_failure.value.has_value(),
            "encoding work exhaustion must return a typed failure");
    require(work_budget.usage().owned_bytes == 0 && work_budget.usage().work_units == 0,
            "unstarted encoding work must release its storage reservation");
}

} // namespace

int main()
{
    test_integer_encoding();
    test_rational_normalization_and_interning();
    test_invalid_and_late_budget_failure();
    test_rational_encoding();
    test_encoding_budget_failures_are_typed_and_rollback_storage();
    return 0;
}

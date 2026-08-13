#pragma once

#include <boost/multiprecision/cpp_int.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace geometer::exact
{

using BigInt = boost::multiprecision::cpp_int;

enum class Error
{
    none,
    invalid_argument,
    resource_limit_exceeded,
};

struct BudgetLimits
{
    std::uint64_t work_units = 1'000'000'000;
    std::uint64_t owned_bytes = 268'435'456;
};

struct BudgetUsage
{
    std::uint64_t work_units = 0;
    std::uint64_t owned_bytes = 0;
};

class Budget
{
  public:
    explicit Budget(BudgetLimits limits);

    [[nodiscard]] bool consume_work(std::uint64_t units);
    [[nodiscard]] bool acquire_storage(std::uint64_t bytes);
    void release_storage(std::uint64_t bytes);

    [[nodiscard]] const BudgetLimits& limits() const;
    [[nodiscard]] const BudgetUsage& usage() const;

  private:
    BudgetLimits limits_;
    BudgetUsage usage_;
};

class Rational
{
  public:
    [[nodiscard]] const BigInt& numerator() const;
    [[nodiscard]] const BigInt& denominator() const;

  private:
    friend class RationalArena;
    Rational(BigInt numerator, BigInt denominator);

    BigInt numerator_;
    BigInt denominator_;
};

struct InternResult
{
    Error error = Error::none;
    std::optional<std::size_t> id;
};

class EncodedBytes
{
  public:
    EncodedBytes(Budget& budget, std::uint64_t charged_bytes, std::vector<std::uint8_t> bytes);
    ~EncodedBytes();
    EncodedBytes(EncodedBytes&& other) noexcept;
    EncodedBytes& operator=(EncodedBytes&& other) noexcept;
    EncodedBytes(const EncodedBytes&) = delete;
    EncodedBytes& operator=(const EncodedBytes&) = delete;

    [[nodiscard]] const std::vector<std::uint8_t>& bytes() const;

  private:
    Budget* budget_ = nullptr;
    std::uint64_t charged_bytes_ = 0;
    std::vector<std::uint8_t> bytes_;
};

struct EncodeResult
{
    Error error = Error::none;
    std::optional<EncodedBytes> value;
};

class RationalArena
{
  public:
    explicit RationalArena(Budget& budget);
    ~RationalArena();

    RationalArena(const RationalArena&) = delete;
    RationalArena& operator=(const RationalArena&) = delete;

    [[nodiscard]] InternResult intern(const BigInt& numerator, const BigInt& denominator);
    [[nodiscard]] const Rational& at(std::size_t id) const;
    [[nodiscard]] std::size_t size() const;

  private:
    Budget& budget_;
    std::vector<Rational> values_;
    std::uint64_t owned_bytes_ = 0;
};

[[nodiscard]] EncodeResult encode_canonical_integer(Budget& budget, const BigInt& value);
[[nodiscard]] EncodeResult encode_canonical_rational(Budget& budget, const Rational& value);

} // namespace geometer::exact

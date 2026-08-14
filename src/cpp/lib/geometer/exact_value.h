#pragma once

#include "geometer/exact_factorization.h"

#include <cstdint>
#include <optional>

namespace geometer::exact
{

enum class CanonicalRealKind
{
    rational,
    irrational,
};

struct CanonicalRealResult;

class CanonicalReal
{
  public:
    ~CanonicalReal();
    CanonicalReal(CanonicalReal&& other) noexcept;
    CanonicalReal& operator=(CanonicalReal&& other) noexcept;
    CanonicalReal(const CanonicalReal&) = delete;
    CanonicalReal& operator=(const CanonicalReal&) = delete;

    [[nodiscard]] CanonicalRealKind kind() const;
    [[nodiscard]] const BigInt& numerator() const;
    [[nodiscard]] const BigInt& denominator() const;
    [[nodiscard]] const Polynomial* polynomial() const;
    [[nodiscard]] const IsolatedRoot* root() const;

  private:
    friend struct CanonicalRealResult;
    friend CanonicalRealResult make_canonical_real(Budget&, const PolynomialFactorSet&,
                                                   const BigInt&, const BigInt&, std::uint32_t);
    CanonicalReal(Budget& budget, std::uint64_t charged_bytes, BigInt numerator,
                  BigInt denominator);
    CanonicalReal(Polynomial polynomial, IsolatedRootSet roots, std::uint32_t ordinal);

    CanonicalRealKind kind_ = CanonicalRealKind::rational;
    Budget* budget_ = nullptr;
    std::uint64_t charged_bytes_ = 0;
    BigInt numerator_ = 0;
    BigInt denominator_ = 1;
    std::optional<Polynomial> polynomial_;
    std::optional<IsolatedRootSet> roots_;
    std::uint32_t ordinal_ = 0;
};

struct CanonicalRealResult
{
    Error error = Error::none;
    FactorRootSelectionStatus selection_status = FactorRootSelectionStatus::error;
    std::optional<CanonicalReal> value;
};

struct ComparisonResult
{
    Error error = Error::none;
    std::optional<std::int8_t> ordering;
};

[[nodiscard]] CanonicalRealResult make_canonical_real(Budget& budget,
                                                      const PolynomialFactorSet& factors,
                                                      const BigInt& lower_k, const BigInt& upper_k,
                                                      std::uint32_t precision);

[[nodiscard]] ComparisonResult compare_canonical_reals(Budget& budget, const CanonicalReal& left,
                                                       const CanonicalReal& right);

[[nodiscard]] ComparisonResult sign_of_canonical_real(Budget& budget, const CanonicalReal& value);

} // namespace geometer::exact

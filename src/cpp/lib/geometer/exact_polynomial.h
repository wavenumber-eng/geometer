#pragma once

#include "geometer/exact_rational.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace geometer::exact
{

class Polynomial
{
  public:
    Polynomial(Budget& budget, std::uint64_t charged_bytes, std::vector<BigInt> coefficients);
    ~Polynomial();
    Polynomial(Polynomial&& other) noexcept;
    Polynomial& operator=(Polynomial&& other) noexcept;
    Polynomial(const Polynomial&) = delete;
    Polynomial& operator=(const Polynomial&) = delete;

    [[nodiscard]] const std::vector<BigInt>& coefficients() const;
    [[nodiscard]] std::size_t degree() const;

  private:
    Budget* budget_ = nullptr;
    std::uint64_t charged_bytes_ = 0;
    std::vector<BigInt> coefficients_;
};

struct PolynomialResult
{
    Error error = Error::none;
    std::optional<Polynomial> value;
};

struct IsolatedRoot
{
    std::uint32_t ordinal = 0;
    std::uint32_t precision = 0;
    BigInt interval_k;
    std::vector<std::int8_t> thom_signs;
};

class IsolatedRootSet
{
  public:
    IsolatedRootSet(Budget& budget, std::uint64_t charged_bytes, std::vector<IsolatedRoot> roots);
    ~IsolatedRootSet();
    IsolatedRootSet(IsolatedRootSet&& other) noexcept;
    IsolatedRootSet& operator=(IsolatedRootSet&& other) noexcept;
    IsolatedRootSet(const IsolatedRootSet&) = delete;
    IsolatedRootSet& operator=(const IsolatedRootSet&) = delete;

    [[nodiscard]] const std::vector<IsolatedRoot>& roots() const;

  private:
    Budget* budget_ = nullptr;
    std::uint64_t charged_bytes_ = 0;
    std::vector<IsolatedRoot> roots_;
};

struct RootIsolationResult
{
    Error error = Error::none;
    std::optional<IsolatedRootSet> value;
};

struct RootIntervalCountResult
{
    Error error = Error::none;
    std::uint32_t count = 0;
    std::uint32_t first_ordinal = 0;
};

struct DyadicRootInterval
{
    std::uint32_t precision = 0;
    BigInt interval_k;
};

struct RootRefinementResult
{
    Error error = Error::none;
    std::optional<DyadicRootInterval> value;
};

[[nodiscard]] PolynomialResult make_primitive_polynomial(Budget& budget,
                                                         const std::vector<BigInt>& coefficients);

[[nodiscard]] PolynomialResult make_square_free_polynomial(Budget& budget,
                                                           const Polynomial& polynomial);

[[nodiscard]] RootIsolationResult isolate_real_roots(Budget& budget, const Polynomial& polynomial,
                                                     std::uint32_t maximum_precision = 4096);

[[nodiscard]] RootIntervalCountResult
count_real_roots_in_dyadic_interval(Budget& budget, const Polynomial& polynomial,
                                    const BigInt& lower_k, const BigInt& upper_k,
                                    std::uint32_t precision);

[[nodiscard]] RootRefinementResult refine_real_root(Budget& budget, const Polynomial& polynomial,
                                                    std::uint32_t ordinal, std::uint32_t precision);

[[nodiscard]] RootIntervalCountResult count_real_roots_below_rational(Budget& budget,
                                                                      const Polynomial& polynomial,
                                                                      const BigInt& numerator,
                                                                      const BigInt& denominator);

} // namespace geometer::exact

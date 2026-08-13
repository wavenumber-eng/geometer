#pragma once

#include "geometer/exact_polynomial.h"

#include <optional>
#include <vector>

namespace geometer::exact
{

class PolynomialFactorSet
{
  public:
    explicit PolynomialFactorSet(std::vector<Polynomial> factors);

    [[nodiscard]] const std::vector<Polynomial>& factors() const;

  private:
    std::vector<Polynomial> factors_;
};

struct FactorizationResult
{
    Error error = Error::none;
    std::optional<PolynomialFactorSet> value;
};

enum class FactorRootSelectionStatus
{
    selected,
    needs_refinement,
    error,
};

struct FactorRootSelectionResult
{
    FactorRootSelectionStatus status = FactorRootSelectionStatus::error;
    Error error = Error::none;
    std::optional<std::size_t> factor_index;
    std::uint32_t root_ordinal = 0;
};

[[nodiscard]] FactorizationResult factor_primitive_polynomial(Budget& budget,
                                                              const Polynomial& polynomial);

[[nodiscard]] FactorRootSelectionResult
select_unique_factor_root(Budget& budget, const PolynomialFactorSet& factors, const BigInt& lower_k,
                          const BigInt& upper_k, std::uint32_t precision);

} // namespace geometer::exact

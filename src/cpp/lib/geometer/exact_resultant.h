#pragma once

#include "geometer/exact_polynomial.h"

namespace geometer::exact
{

enum class ResultantOperation
{
    sum,
    product,
};

[[nodiscard]] PolynomialResult make_square_free_resultant(Budget& budget, const Polynomial& left,
                                                          const Polynomial& right,
                                                          ResultantOperation operation);

[[nodiscard]] PolynomialResult make_reciprocal_polynomial(Budget& budget,
                                                          const Polynomial& polynomial);

[[nodiscard]] PolynomialResult make_square_root_polynomial(Budget& budget,
                                                           const Polynomial& polynomial);

[[nodiscard]] PolynomialResult make_translated_polynomial(Budget& budget,
                                                          const Polynomial& polynomial,
                                                          const BigInt& numerator,
                                                          const BigInt& denominator);

[[nodiscard]] PolynomialResult make_scaled_polynomial(Budget& budget, const Polynomial& polynomial,
                                                      const BigInt& numerator,
                                                      const BigInt& denominator);

} // namespace geometer::exact

#pragma once

#include "geometer/exact_resultant.h"
#include "geometer/exact_value.h"

namespace geometer::exact
{

[[nodiscard]] CanonicalRealResult add_canonical_reals(Budget& budget, const CanonicalReal& left,
                                                      const CanonicalReal& right);
[[nodiscard]] CanonicalRealResult
multiply_canonical_reals(Budget& budget, const CanonicalReal& left, const CanonicalReal& right);
[[nodiscard]] CanonicalRealResult reciprocal_canonical_real(Budget& budget,
                                                            const CanonicalReal& value);
[[nodiscard]] CanonicalRealResult
nonnegative_square_root_canonical_real(Budget& budget, const CanonicalReal& value);

} // namespace geometer::exact

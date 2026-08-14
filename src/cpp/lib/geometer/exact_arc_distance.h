#pragma once

#include "geometer/exact_curve_domain.h"

namespace geometer::exact
{

[[nodiscard]] ExactPredicateResult exact_arc_hausdorff_within(ConstructionArena& arena,
                                                              const ExactCircularArc& left,
                                                              const ExactCircularArc& right,
                                                              const BigInt& threshold_numerator,
                                                              const BigInt& threshold_denominator);

} // namespace geometer::exact

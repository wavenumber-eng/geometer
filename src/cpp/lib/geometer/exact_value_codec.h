#pragma once

#include "geometer/exact_value.h"

namespace geometer::exact
{

[[nodiscard]] EncodeResult encode_canonical_real(Budget& budget, const CanonicalReal& value);

} // namespace geometer::exact

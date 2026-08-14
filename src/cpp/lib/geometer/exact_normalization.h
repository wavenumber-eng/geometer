#pragma once

#include "geometer/exact_value.h"

#include <cstdint>
#include <optional>

namespace geometer::exact
{

struct IntegerNormalizationResult
{
    Error error = Error::none;
    std::optional<std::int64_t> value;
};

[[nodiscard]] IntegerNormalizationResult normalize_exact_to_integer_nm(Budget& budget,
                                                                       const CanonicalReal& value);

} // namespace geometer::exact

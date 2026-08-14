#pragma once

#include "geometer/exact_value.h"

namespace geometer::exact
{

[[nodiscard]] EncodeResult encode_canonical_real(Budget& budget, const CanonicalReal& value);

struct DecodeCanonicalRealResult
{
    Error error = Error::none;
    std::optional<CanonicalReal> value;
};

[[nodiscard]] DecodeCanonicalRealResult
decode_canonical_real(Budget& budget, const std::vector<std::uint8_t>& bytes);

} // namespace geometer::exact

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace geometer
{

[[nodiscard]] std::array<std::uint8_t, 32> sha256(const std::uint8_t* data, std::size_t size);
[[nodiscard]] std::string sha256_hex(const std::uint8_t* data, std::size_t size);

} // namespace geometer

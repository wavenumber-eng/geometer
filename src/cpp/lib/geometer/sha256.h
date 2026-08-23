#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace geometer
{

class Sha256Builder
{
  public:
    Sha256Builder();
    void update(const std::uint8_t* data, std::size_t size);
    [[nodiscard]] std::array<std::uint8_t, 32> digest() const;
    [[nodiscard]] std::string hex_digest() const;

  private:
    std::array<std::uint32_t, 8> state_;
    std::array<std::uint8_t, 64> buffer_{};
    std::size_t buffer_size_ = 0;
    std::uint64_t total_size_ = 0;
};

[[nodiscard]] std::array<std::uint8_t, 32> sha256(const std::uint8_t* data, std::size_t size);
[[nodiscard]] std::string sha256_hex(const std::uint8_t* data, std::size_t size);

} // namespace geometer

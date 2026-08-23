#include "geometer/sha256.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace geometer
{
namespace
{

constexpr std::array<std::uint32_t, 64> kRoundConstants{
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U, 0x923f82a4U,
    0xab1c5ed5U, 0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU,
    0x9bdc06a7U, 0xc19bf174U, 0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU,
    0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU, 0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
    0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U, 0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU,
    0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U, 0xa2bfe8a1U, 0xa81a664bU,
    0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U, 0x19a4c116U,
    0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U, 0x90befffaU, 0xa4506cebU, 0xbef9a3f7U,
    0xc67178f2U,
};

std::uint32_t rotate_right(std::uint32_t value, std::uint32_t count)
{
    return (value >> count) | (value << (32U - count));
}

void compress(std::array<std::uint32_t, 8>& state, const std::uint8_t* block)
{
    std::array<std::uint32_t, 64> words{};
    for (std::uint32_t index = 0; index < 16; ++index)
    {
        const std::size_t offset = static_cast<std::size_t>(index) * 4;
        words[index] = (static_cast<std::uint32_t>(block[offset]) << 24U) |
                       (static_cast<std::uint32_t>(block[offset + 1]) << 16U) |
                       (static_cast<std::uint32_t>(block[offset + 2]) << 8U) |
                       static_cast<std::uint32_t>(block[offset + 3]);
    }
    for (std::uint32_t index = 16; index < 64; ++index)
    {
        const std::uint32_t s0 = rotate_right(words[index - 15], 7) ^
                                 rotate_right(words[index - 15], 18) ^ (words[index - 15] >> 3U);
        const std::uint32_t s1 = rotate_right(words[index - 2], 17) ^
                                 rotate_right(words[index - 2], 19) ^ (words[index - 2] >> 10U);
        words[index] = words[index - 16] + s0 + words[index - 7] + s1;
    }
    std::uint32_t a = state[0];
    std::uint32_t b = state[1];
    std::uint32_t c = state[2];
    std::uint32_t d = state[3];
    std::uint32_t e = state[4];
    std::uint32_t f = state[5];
    std::uint32_t g = state[6];
    std::uint32_t h = state[7];
    for (std::uint32_t index = 0; index < 64; ++index)
    {
        const std::uint32_t sum1 = rotate_right(e, 6) ^ rotate_right(e, 11) ^ rotate_right(e, 25);
        const std::uint32_t choice = (e & f) ^ (~e & g);
        const std::uint32_t temporary1 = h + sum1 + choice + kRoundConstants[index] + words[index];
        const std::uint32_t sum0 = rotate_right(a, 2) ^ rotate_right(a, 13) ^ rotate_right(a, 22);
        const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
        const std::uint32_t temporary2 = sum0 + majority;
        h = g;
        g = f;
        f = e;
        e = d + temporary1;
        d = c;
        c = b;
        b = a;
        a = temporary1 + temporary2;
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

} // namespace

Sha256Builder::Sha256Builder()
    : state_{0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
             0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U}
{
}

void Sha256Builder::update(const std::uint8_t* data, std::size_t size)
{
    if (data == nullptr && size != 0)
    {
        throw std::invalid_argument("SHA-256 input pointer is null");
    }
    if (size > std::numeric_limits<std::uint64_t>::max() / 8U - total_size_)
    {
        throw std::length_error("SHA-256 input is too large");
    }
    total_size_ += static_cast<std::uint64_t>(size);
    std::size_t offset = 0;
    if (buffer_size_ != 0)
    {
        const std::size_t copied = std::min(size, buffer_.size() - buffer_size_);
        if (copied != 0)
        {
            std::memcpy(buffer_.data() + buffer_size_, data, copied);
        }
        buffer_size_ += copied;
        offset += copied;
        if (buffer_size_ == buffer_.size())
        {
            compress(state_, buffer_.data());
            buffer_size_ = 0;
        }
    }
    while (size - offset >= buffer_.size())
    {
        compress(state_, data + offset);
        offset += buffer_.size();
    }
    if (offset < size)
    {
        buffer_size_ = size - offset;
        std::memcpy(buffer_.data(), data + offset, buffer_size_);
    }
}

std::array<std::uint8_t, 32> Sha256Builder::digest() const
{
    Sha256Builder copy = *this;
    std::array<std::uint8_t, 128> final{};
    std::memcpy(final.data(), copy.buffer_.data(), copy.buffer_size_);
    final[copy.buffer_size_] = 0x80;
    const std::size_t final_size = copy.buffer_size_ < 56 ? 64 : 128;
    const std::uint64_t bit_size = copy.total_size_ * 8U;
    for (std::uint32_t index = 0; index < 8; ++index)
    {
        final[final_size - 1U - index] = static_cast<std::uint8_t>(bit_size >> (index * 8U));
    }
    compress(copy.state_, final.data());
    if (final_size == 128)
    {
        compress(copy.state_, final.data() + 64);
    }
    std::array<std::uint8_t, 32> result{};
    for (std::uint32_t word = 0; word < copy.state_.size(); ++word)
    {
        for (std::uint32_t byte = 0; byte < 4; ++byte)
        {
            result[word * 4U + byte] =
                static_cast<std::uint8_t>(copy.state_[word] >> ((3U - byte) * 8U));
        }
    }
    return result;
}

std::string Sha256Builder::hex_digest() const
{
    constexpr char digits[] = "0123456789abcdef";
    const auto bytes = digest();
    std::string output;
    output.reserve(64);
    for (const std::uint8_t byte : bytes)
    {
        output.push_back(digits[byte >> 4U]);
        output.push_back(digits[byte & 15U]);
    }
    return output;
}

std::array<std::uint8_t, 32> sha256(const std::uint8_t* data, std::size_t size)
{
    Sha256Builder builder;
    builder.update(data, size);
    return builder.digest();
}

std::string sha256_hex(const std::uint8_t* data, std::size_t size)
{
    Sha256Builder builder;
    builder.update(data, size);
    return builder.hex_digest();
}

} // namespace geometer

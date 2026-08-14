#include "geometer/exact_value_codec.h"

#include <boost/multiprecision/cpp_int/import_export.hpp>

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <vector>

namespace geometer::exact
{
namespace
{

class StorageReservation
{
  public:
    StorageReservation(Budget& budget, std::uint64_t bytes)
        : budget_(budget), bytes_(bytes), acquired_(budget.acquire_storage(bytes))
    {
    }
    ~StorageReservation()
    {
        if (acquired_ && !committed_)
            budget_.release_storage(bytes_);
    }
    [[nodiscard]] bool acquired() const
    {
        return acquired_;
    }
    void commit()
    {
        committed_ = true;
    }

  private:
    Budget& budget_;
    std::uint64_t bytes_;
    bool acquired_;
    bool committed_ = false;
};

std::uint64_t checked_add(std::uint64_t left, std::uint64_t right)
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
        throw std::overflow_error("canonical scalar encoding size overflow");
    return left + right;
}

std::uint64_t checked_multiply(std::uint64_t left, std::uint64_t right)
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left)
        throw std::overflow_error("canonical scalar encoding work overflow");
    return left * right;
}

std::uint64_t magnitude_bytes(const BigInt& value)
{
    if (value == 0)
        return 0;
    const auto& backend = value.backend();
    auto high_limb = backend.limbs()[backend.size() - 1];
    std::uint64_t high_bits = 0;
    while (high_limb != 0)
    {
        ++high_bits;
        high_limb >>= 1;
    }
    return checked_add(checked_multiply(static_cast<std::uint64_t>(backend.size() - 1),
                                        sizeof(boost::multiprecision::limb_type)),
                       checked_add(high_bits, 7) / 8);
}

std::uint64_t align(std::uint64_t value, std::uint64_t boundary)
{
    return checked_multiply(checked_add(value, boundary - 1) / boundary, boundary);
}

std::uint64_t integer_bytes(const BigInt& value)
{
    return align(checked_add(8, magnitude_bytes(value)), 4);
}

void append_u32(std::vector<std::uint8_t>& output, std::uint32_t value)
{
    for (std::uint32_t shift = 0; shift < 32; shift += 8)
        output.push_back(static_cast<std::uint8_t>((value >> shift) & 0xff));
}

void patch_u32(std::vector<std::uint8_t>& output, std::size_t offset, std::uint32_t value)
{
    for (std::uint32_t shift = 0; shift < 32; shift += 8)
        output[offset++] = static_cast<std::uint8_t>((value >> shift) & 0xff);
}

void append_integer(std::vector<std::uint8_t>& output, const BigInt& value)
{
    const BigInt magnitude = value < 0 ? -value : value;
    const std::uint64_t bytes = magnitude_bytes(magnitude);
    output.push_back(value == 0 ? 0 : (value > 0 ? 1 : 2));
    output.insert(output.end(), 3, 0);
    append_u32(output, static_cast<std::uint32_t>(bytes));
    if (bytes != 0)
        boost::multiprecision::export_bits(magnitude, std::back_inserter(output), 8, true);
    while (output.size() % 4 != 0)
        output.push_back(0);
}

std::uint64_t encoded_size(const CanonicalReal& value)
{
    std::uint64_t size = 8;
    if (value.kind() == CanonicalRealKind::rational)
        size = checked_add(size, checked_add(integer_bytes(value.numerator()),
                                             integer_bytes(value.denominator())));
    else
    {
        size = checked_add(size, 4);
        for (const BigInt& coefficient : value.polynomial()->coefficients())
            size = checked_add(size, integer_bytes(coefficient));
        size = checked_add(size, 8);
        size = checked_add(size, integer_bytes(value.root()->interval_k));
        size = checked_add(size, 4);
        size = checked_add(size, static_cast<std::uint64_t>(value.root()->thom_signs.size()));
    }
    return align(size, 8);
}

} // namespace

EncodeResult encode_canonical_real(Budget& budget, const CanonicalReal& value)
{
    try
    {
        const std::uint64_t size = encoded_size(value);
        if (size > std::numeric_limits<std::uint32_t>::max())
            return {Error::resource_limit_exceeded, std::nullopt};
        const std::uint64_t storage = checked_add(256, checked_multiply(size, 2));
        const std::uint64_t work = checked_multiply(size, 8);
        StorageReservation reservation(budget, storage);
        if (!reservation.acquired() || !budget.consume_work(work))
            return {Error::resource_limit_exceeded, std::nullopt};
        std::vector<std::uint8_t> output;
        output.reserve(static_cast<std::size_t>(size));
        output.push_back(value.kind() == CanonicalRealKind::rational ? 1 : 2);
        output.insert(output.end(), 3, 0);
        append_u32(output, 0);
        if (value.kind() == CanonicalRealKind::rational)
        {
            append_integer(output, value.numerator());
            append_integer(output, value.denominator());
        }
        else
        {
            append_u32(output,
                       static_cast<std::uint32_t>(value.polynomial()->coefficients().size()));
            for (const BigInt& coefficient : value.polynomial()->coefficients())
                append_integer(output, coefficient);
            append_u32(output, value.root()->ordinal);
            append_u32(output, value.root()->precision);
            append_integer(output, value.root()->interval_k);
            append_u32(output, static_cast<std::uint32_t>(value.root()->thom_signs.size()));
            for (const std::int8_t sign : value.root()->thom_signs)
                output.push_back(static_cast<std::uint8_t>(sign));
        }
        output.resize(static_cast<std::size_t>(size), 0);
        patch_u32(output, 4, static_cast<std::uint32_t>(size));
        reservation.commit();
        return {Error::none, EncodedBytes(budget, storage, std::move(output))};
    }
    catch (const std::exception&)
    {
        return {Error::resource_limit_exceeded, std::nullopt};
    }
}

} // namespace geometer::exact

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

bool read_u32(const std::vector<std::uint8_t>& input, std::size_t& offset, std::uint32_t& value)
{
    if (offset > input.size() || input.size() - offset < 4)
        return false;
    value = 0;
    for (std::uint32_t shift = 0; shift < 32; shift += 8)
        value |= static_cast<std::uint32_t>(input[offset++]) << shift;
    return true;
}

bool read_integer(const std::vector<std::uint8_t>& input, std::size_t& offset, BigInt& value)
{
    const std::size_t start = offset;
    if (offset > input.size() || input.size() - offset < 8)
        return false;
    const std::uint8_t sign = input[offset++];
    if (input[offset++] != 0 || input[offset++] != 0 || input[offset++] != 0)
        return false;
    std::uint32_t magnitude_size = 0;
    if (!read_u32(input, offset, magnitude_size) || magnitude_size > 2048 ||
        magnitude_size > input.size() - offset)
        return false;
    if ((sign == 0) != (magnitude_size == 0) || sign > 2 ||
        (magnitude_size != 0 && input[offset] == 0))
        return false;
    value = 0;
    if (magnitude_size != 0)
        boost::multiprecision::import_bits(
            value, input.begin() + static_cast<std::ptrdiff_t>(offset),
            input.begin() + static_cast<std::ptrdiff_t>(offset + magnitude_size), 8, true);
    offset += magnitude_size;
    const std::uint64_t aligned = align(checked_add(start, checked_add(8, magnitude_size)), 4);
    if (aligned > input.size())
        return false;
    while (offset < aligned)
        if (input[offset++] != 0)
            return false;
    if (sign == 2)
        value = -value;
    return true;
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

DecodeCanonicalRealResult decode_canonical_real(Budget& budget,
                                                const std::vector<std::uint8_t>& input)
{
    if (input.size() < 8 || input.size() > 1'048'576 || input.size() % 8 != 0)
        return {Error::invalid_argument, std::nullopt};
    try
    {
        const std::uint64_t storage = checked_add(1024, checked_multiply(input.size(), 4));
        const std::uint64_t work = checked_multiply(input.size(), 16);
        StorageReservation reservation(budget, storage);
        if (!reservation.acquired() || !budget.consume_work(work))
            return {Error::resource_limit_exceeded, std::nullopt};
        std::size_t offset = 0;
        const std::uint8_t kind = input[offset++];
        if ((kind != 1 && kind != 2) || input[offset++] != 0 || input[offset++] != 0 ||
            input[offset++] != 0)
            return {Error::invalid_argument, std::nullopt};
        std::uint32_t record_bytes = 0;
        if (!read_u32(input, offset, record_bytes) || record_bytes != input.size())
            return {Error::invalid_argument, std::nullopt};
        if (kind == 1)
        {
            BigInt numerator;
            BigInt denominator;
            if (!read_integer(input, offset, numerator) ||
                !read_integer(input, offset, denominator) || denominator <= 0 ||
                align(offset, 8) != input.size())
                return {Error::invalid_argument, std::nullopt};
            while (offset < input.size())
                if (input[offset++] != 0)
                    return {Error::invalid_argument, std::nullopt};
            CanonicalRealResult value = make_canonical_rational(budget, numerator, denominator);
            if (value.error != Error::none || !value.value)
                return {value.error, std::nullopt};
            if (value.value->numerator() != numerator || value.value->denominator() != denominator)
                return {Error::invalid_argument, std::nullopt};
            return {value.error, std::move(value.value)};
        }
        std::uint32_t coefficient_count = 0;
        if (!read_u32(input, offset, coefficient_count) || coefficient_count < 3 ||
            coefficient_count > 65 ||
            checked_multiply(coefficient_count, 8) > input.size() - offset)
            return {Error::invalid_argument, std::nullopt};
        std::vector<BigInt> coefficients;
        coefficients.reserve(coefficient_count);
        for (std::uint32_t index = 0; index < coefficient_count; ++index)
        {
            BigInt coefficient;
            if (!read_integer(input, offset, coefficient))
                return {Error::invalid_argument, std::nullopt};
            coefficients.push_back(std::move(coefficient));
        }
        std::uint32_t ordinal = 0;
        std::uint32_t precision = 0;
        BigInt interval_k;
        std::uint32_t thom_count = 0;
        if (!read_u32(input, offset, ordinal) || !read_u32(input, offset, precision) ||
            precision > 4096 || !read_integer(input, offset, interval_k) ||
            !read_u32(input, offset, thom_count) || thom_count != coefficient_count - 1 ||
            thom_count > input.size() - offset)
            return {Error::invalid_argument, std::nullopt};
        std::vector<std::int8_t> thom_signs;
        thom_signs.reserve(thom_count);
        for (std::uint32_t index = 0; index < thom_count; ++index)
        {
            const std::uint8_t encoded = input[offset++];
            if (encoded != 0 && encoded != 1 && encoded != 255)
                return {Error::invalid_argument, std::nullopt};
            thom_signs.push_back(encoded == 255 ? -1 : static_cast<std::int8_t>(encoded));
        }
        if (align(offset, 8) != input.size())
            return {Error::invalid_argument, std::nullopt};
        while (offset < input.size())
            if (input[offset++] != 0)
                return {Error::invalid_argument, std::nullopt};
        CanonicalRealResult value = make_canonical_irrational(budget, coefficients, ordinal);
        if (value.error != Error::none || !value.value)
            return {value.error, std::nullopt};
        if (value.value->polynomial()->coefficients() != coefficients)
            return {Error::invalid_argument, std::nullopt};
        const IsolatedRoot& root = *value.value->root();
        if (root.precision != precision || root.interval_k != interval_k ||
            root.thom_signs != thom_signs)
            return {Error::invalid_argument, std::nullopt};
        return {Error::none, std::move(value.value)};
    }
    catch (const std::exception&)
    {
        return {Error::resource_limit_exceeded, std::nullopt};
    }
}

} // namespace geometer::exact

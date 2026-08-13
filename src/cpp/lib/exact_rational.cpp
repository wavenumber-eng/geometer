#include "geometer/exact_rational.h"

#include <boost/multiprecision/cpp_int/import_export.hpp>

#include <algorithm>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <utility>

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
        {
            budget_.release_storage(bytes_);
        }
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
    {
        throw std::overflow_error("exact arithmetic budget estimate overflow");
    }
    return left + right;
}

std::uint64_t checked_multiply(std::uint64_t left, std::uint64_t right)
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left)
    {
        throw std::overflow_error("exact arithmetic budget estimate overflow");
    }
    return left * right;
}

BigInt absolute(BigInt value)
{
    return value < 0 ? -value : value;
}

std::uint64_t magnitude_byte_count(const BigInt& value)
{
    if (value == 0)
    {
        return 0;
    }
    const auto& backend = value.backend();
    auto high_limb = backend.limbs()[backend.size() - 1];
    std::uint64_t high_bits = 0;
    while (high_limb != 0)
    {
        ++high_bits;
        high_limb >>= 1;
    }
    const std::uint64_t preceding_bytes = checked_multiply(
        static_cast<std::uint64_t>(backend.size() - 1), sizeof(boost::multiprecision::limb_type));
    return checked_add(preceding_bytes, checked_add(high_bits, 7) / 8);
}

std::uint64_t limb_count(const BigInt& value)
{
    return std::max<std::uint64_t>(1, checked_add(magnitude_byte_count(value), 3) / 4);
}

BigInt gcd(BigInt left, BigInt right)
{
    left = absolute(std::move(left));
    right = absolute(std::move(right));
    while (right != 0)
    {
        BigInt remainder = left % right;
        left = std::move(right);
        right = std::move(remainder);
    }
    return left;
}

std::uint64_t copy_work(const BigInt& numerator, const BigInt& denominator)
{
    return checked_add(checked_add(limb_count(numerator), limb_count(denominator)), 4);
}

std::uint64_t gcd_work(const BigInt& numerator, const BigInt& denominator)
{
    const std::uint64_t width =
        checked_add(checked_add(limb_count(numerator), limb_count(denominator)), 1);
    return checked_multiply(checked_multiply(width, width), width);
}

std::uint64_t interning_work(std::uint64_t value_count, std::uint64_t retained_limb_count,
                             const BigInt& numerator, const BigInt& denominator)
{
    const std::uint64_t candidate_limbs =
        checked_add(limb_count(numerator), limb_count(denominator));
    const std::uint64_t equality_work =
        checked_multiply(value_count, checked_add(candidate_limbs, 2));
    const std::uint64_t container_work = checked_add(value_count, 1);
    return checked_add(checked_add(retained_limb_count, equality_work), container_work);
}

std::uint64_t storage_bound(const BigInt& numerator, const BigInt& denominator)
{
    const std::uint64_t limbs =
        checked_add(checked_add(limb_count(numerator), limb_count(denominator)), 4);
    const std::uint64_t bigint_storage = checked_multiply(checked_multiply(limbs, 4), 12);
    return checked_add(bigint_storage, checked_add(sizeof(Rational), 64));
}

std::uint64_t align_size(std::uint64_t size, std::uint64_t alignment)
{
    return checked_multiply(checked_add(size, alignment - 1) / alignment, alignment);
}

std::uint64_t encoded_integer_size(const BigInt& value)
{
    return align_size(checked_add(8, magnitude_byte_count(value)), 4);
}

std::uint64_t encoded_storage_bound(std::uint64_t size)
{
    return checked_add(size, checked_add(sizeof(EncodedBytes), 64));
}

void append_u32(std::vector<std::uint8_t>& output, std::uint32_t value)
{
    for (unsigned shift = 0; shift < 32; shift += 8)
    {
        output.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffU));
    }
}

void align_to(std::vector<std::uint8_t>& output, std::size_t alignment)
{
    while (output.size() % alignment != 0)
    {
        output.push_back(0);
    }
}

bool same_value(const Rational& value, const BigInt& numerator, const BigInt& denominator)
{
    return value.numerator() == numerator && value.denominator() == denominator;
}

void append_integer_unchecked(std::vector<std::uint8_t>& output, const BigInt& value)
{
    output.push_back(value == 0 ? 0 : (value > 0 ? 1 : 2));
    output.insert(output.end(), 3, 0);
    const std::uint64_t magnitude_size = magnitude_byte_count(value);
    append_u32(output, static_cast<std::uint32_t>(magnitude_size));
    if (value != 0)
    {
        boost::multiprecision::export_bits(value, std::back_inserter(output), 8, true);
    }
    align_to(output, 4);
}

std::vector<std::uint8_t> encode_integer_unchecked(const BigInt& value, std::uint64_t encoded_size)
{
    std::vector<std::uint8_t> output;
    output.reserve(static_cast<std::size_t>(encoded_size));
    append_integer_unchecked(output, value);
    return output;
}

} // namespace

Budget::Budget(BudgetLimits limits) : limits_(limits) {}

bool Budget::consume_work(std::uint64_t units)
{
    if (units > limits_.work_units - usage_.work_units)
    {
        return false;
    }
    usage_.work_units += units;
    return true;
}

bool Budget::acquire_storage(std::uint64_t bytes)
{
    if (bytes > limits_.owned_bytes - usage_.owned_bytes)
    {
        return false;
    }
    usage_.owned_bytes += bytes;
    return true;
}

void Budget::release_storage(std::uint64_t bytes)
{
    if (bytes > usage_.owned_bytes)
    {
        throw std::logic_error("exact arithmetic storage release underflow");
    }
    usage_.owned_bytes -= bytes;
}

const BudgetLimits& Budget::limits() const
{
    return limits_;
}

const BudgetUsage& Budget::usage() const
{
    return usage_;
}

EncodedBytes::EncodedBytes(Budget& budget, std::uint64_t charged_bytes,
                           std::vector<std::uint8_t> bytes)
    : budget_(&budget), charged_bytes_(charged_bytes), bytes_(std::move(bytes))
{
}

EncodedBytes::~EncodedBytes()
{
    if (budget_ != nullptr)
    {
        budget_->release_storage(charged_bytes_);
    }
}

EncodedBytes::EncodedBytes(EncodedBytes&& other) noexcept
    : budget_(std::exchange(other.budget_, nullptr)),
      charged_bytes_(std::exchange(other.charged_bytes_, 0)), bytes_(std::move(other.bytes_))
{
}

EncodedBytes& EncodedBytes::operator=(EncodedBytes&& other) noexcept
{
    if (this != &other)
    {
        if (budget_ != nullptr)
        {
            budget_->release_storage(charged_bytes_);
        }
        budget_ = std::exchange(other.budget_, nullptr);
        charged_bytes_ = std::exchange(other.charged_bytes_, 0);
        bytes_ = std::move(other.bytes_);
    }
    return *this;
}

const std::vector<std::uint8_t>& EncodedBytes::bytes() const
{
    return bytes_;
}

Rational::Rational(BigInt numerator, BigInt denominator)
    : numerator_(std::move(numerator)), denominator_(std::move(denominator))
{
}

const BigInt& Rational::numerator() const
{
    return numerator_;
}

const BigInt& Rational::denominator() const
{
    return denominator_;
}

RationalArena::RationalArena(Budget& budget) : budget_(budget) {}

RationalArena::~RationalArena()
{
    budget_.release_storage(owned_bytes_);
}

InternResult RationalArena::intern(const BigInt& numerator, const BigInt& denominator)
{
    if (denominator == 0)
    {
        return {Error::invalid_argument, std::nullopt};
    }

    std::uint64_t storage = 0;
    std::uint64_t first_phase_work = 0;
    std::uint64_t second_phase_work = 0;
    std::uint64_t third_phase_work = 0;
    try
    {
        storage = storage_bound(numerator, denominator);
        first_phase_work = copy_work(numerator, denominator);
        second_phase_work = gcd_work(numerator, denominator);
        third_phase_work = interning_work(static_cast<std::uint64_t>(values_.size()),
                                          retained_limb_count_, numerator, denominator);
    }
    catch (const std::overflow_error&)
    {
        return {Error::resource_limit_exceeded, std::nullopt};
    }

    StorageReservation storage_reservation(budget_, storage);
    if (!storage_reservation.acquired())
    {
        return {Error::resource_limit_exceeded, std::nullopt};
    }

    if (!budget_.consume_work(first_phase_work))
    {
        return {Error::resource_limit_exceeded, std::nullopt};
    }
    try
    {
        BigInt normalized_numerator = numerator;
        BigInt normalized_denominator = denominator;
        if (normalized_denominator < 0)
        {
            normalized_numerator = -normalized_numerator;
            normalized_denominator = -normalized_denominator;
        }

        if (!budget_.consume_work(second_phase_work))
        {
            return {Error::resource_limit_exceeded, std::nullopt};
        }
        const BigInt divisor = gcd(normalized_numerator, normalized_denominator);
        normalized_numerator /= divisor;
        normalized_denominator /= divisor;
        if (normalized_numerator == 0)
        {
            normalized_denominator = 1;
        }

        const std::uint64_t normalized_limbs =
            checked_add(limb_count(normalized_numerator), limb_count(normalized_denominator));
        const std::uint64_t new_retained_limb_count =
            checked_add(retained_limb_count_, normalized_limbs);
        const std::uint64_t new_owned_bytes = checked_add(owned_bytes_, storage);
        if (!budget_.consume_work(third_phase_work))
        {
            return {Error::resource_limit_exceeded, std::nullopt};
        }

        const auto existing = std::find_if(
            values_.begin(), values_.end(), [&](const Rational& value)
            { return same_value(value, normalized_numerator, normalized_denominator); });
        if (existing != values_.end())
        {
            return {Error::none, static_cast<std::size_t>(existing - values_.begin())};
        }

        values_.push_back(
            Rational(std::move(normalized_numerator), std::move(normalized_denominator)));
        owned_bytes_ = new_owned_bytes;
        retained_limb_count_ = new_retained_limb_count;
        storage_reservation.commit();
        return {Error::none, values_.size() - 1};
    }
    catch (const std::exception&)
    {
        return {Error::resource_limit_exceeded, std::nullopt};
    }
}

const Rational& RationalArena::at(std::size_t id) const
{
    return values_.at(id);
}

std::size_t RationalArena::size() const
{
    return values_.size();
}

EncodeResult encode_canonical_integer(Budget& budget, const BigInt& value)
{
    try
    {
        const std::uint64_t size = encoded_integer_size(value);
        if (size > std::numeric_limits<std::uint32_t>::max() ||
            size > std::numeric_limits<std::size_t>::max())
        {
            return {Error::resource_limit_exceeded, std::nullopt};
        }
        const std::uint64_t storage = encoded_storage_bound(size);
        StorageReservation reservation(budget, storage);
        if (!reservation.acquired() || !budget.consume_work(checked_add(limb_count(value), size)))
        {
            return {Error::resource_limit_exceeded, std::nullopt};
        }
        std::vector<std::uint8_t> output = encode_integer_unchecked(value, size);
        reservation.commit();
        return {Error::none, EncodedBytes(budget, storage, std::move(output))};
    }
    catch (const std::exception&)
    {
        return {Error::resource_limit_exceeded, std::nullopt};
    }
}

EncodeResult encode_canonical_rational(Budget& budget, const Rational& value)
{
    try
    {
        const std::uint64_t numerator_size = encoded_integer_size(value.numerator());
        const std::uint64_t denominator_size = encoded_integer_size(value.denominator());
        const std::uint64_t size =
            align_size(checked_add(8, checked_add(numerator_size, denominator_size)), 8);
        if (size > std::numeric_limits<std::uint32_t>::max() ||
            size > std::numeric_limits<std::size_t>::max())
        {
            return {Error::resource_limit_exceeded, std::nullopt};
        }
        const std::uint64_t storage = encoded_storage_bound(size);
        const std::uint64_t work = checked_add(
            checked_add(limb_count(value.numerator()), limb_count(value.denominator())), size);
        StorageReservation reservation(budget, storage);
        if (!reservation.acquired() || !budget.consume_work(work))
        {
            return {Error::resource_limit_exceeded, std::nullopt};
        }

        std::vector<std::uint8_t> output = {1, 0, 0, 0, 0, 0, 0, 0};
        output.reserve(static_cast<std::size_t>(size));
        append_integer_unchecked(output, value.numerator());
        append_integer_unchecked(output, value.denominator());
        align_to(output, 8);
        const std::uint32_t encoded_size = static_cast<std::uint32_t>(output.size());
        for (unsigned byte = 0; byte < 4; ++byte)
        {
            output[4 + byte] = static_cast<std::uint8_t>((encoded_size >> (byte * 8)) & 0xffU);
        }
        reservation.commit();
        return {Error::none, EncodedBytes(budget, storage, std::move(output))};
    }
    catch (const std::exception&)
    {
        return {Error::resource_limit_exceeded, std::nullopt};
    }
}

} // namespace geometer::exact

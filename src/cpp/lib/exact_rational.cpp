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

std::uint64_t limb_count(const BigInt& value)
{
    if (value == 0)
    {
        return 1;
    }
    const BigInt magnitude = absolute(value);
    return static_cast<std::uint64_t>(boost::multiprecision::msb(magnitude) / 32 + 1);
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

std::uint64_t storage_bound(const BigInt& numerator, const BigInt& denominator)
{
    const std::uint64_t limbs =
        checked_add(checked_add(limb_count(numerator), limb_count(denominator)), 4);
    return checked_multiply(limbs, 4);
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
    try
    {
        storage = storage_bound(numerator, denominator);
        first_phase_work = copy_work(numerator, denominator);
        second_phase_work = gcd_work(numerator, denominator);
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

    const auto existing =
        std::find_if(values_.begin(), values_.end(), [&](const Rational& value)
                     { return same_value(value, normalized_numerator, normalized_denominator); });
    if (existing != values_.end())
    {
        return {Error::none, static_cast<std::size_t>(existing - values_.begin())};
    }

    const std::uint64_t new_owned_bytes = checked_add(owned_bytes_, storage);
    values_.push_back(Rational(std::move(normalized_numerator), std::move(normalized_denominator)));
    owned_bytes_ = new_owned_bytes;
    storage_reservation.commit();
    return {Error::none, values_.size() - 1};
}

const Rational& RationalArena::at(std::size_t id) const
{
    return values_.at(id);
}

std::size_t RationalArena::size() const
{
    return values_.size();
}

std::vector<std::uint8_t> encode_canonical_integer(const BigInt& value)
{
    std::vector<std::uint8_t> magnitude;
    if (value != 0)
    {
        const BigInt absolute_value = absolute(value);
        boost::multiprecision::export_bits(absolute_value, std::back_inserter(magnitude), 8, true);
    }

    std::vector<std::uint8_t> output;
    output.reserve(8 + magnitude.size() + 3);
    output.push_back(value == 0 ? 0 : (value > 0 ? 1 : 2));
    output.insert(output.end(), 3, 0);
    if (magnitude.size() > std::numeric_limits<std::uint32_t>::max())
    {
        throw std::length_error("canonical integer magnitude exceeds u32");
    }
    append_u32(output, static_cast<std::uint32_t>(magnitude.size()));
    output.insert(output.end(), magnitude.begin(), magnitude.end());
    align_to(output, 4);
    return output;
}

std::vector<std::uint8_t> encode_canonical_rational(const Rational& value)
{
    const std::vector<std::uint8_t> numerator = encode_canonical_integer(value.numerator());
    const std::vector<std::uint8_t> denominator = encode_canonical_integer(value.denominator());
    std::vector<std::uint8_t> output = {1, 0, 0, 0, 0, 0, 0, 0};
    output.insert(output.end(), numerator.begin(), numerator.end());
    output.insert(output.end(), denominator.begin(), denominator.end());
    align_to(output, 8);
    if (output.size() > std::numeric_limits<std::uint32_t>::max())
    {
        throw std::length_error("canonical rational exceeds u32");
    }
    const std::uint32_t size = static_cast<std::uint32_t>(output.size());
    for (unsigned byte = 0; byte < 4; ++byte)
    {
        output[4 + byte] = static_cast<std::uint8_t>((size >> (byte * 8)) & 0xffU);
    }
    return output;
}

} // namespace geometer::exact

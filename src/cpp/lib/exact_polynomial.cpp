#include "geometer/exact_polynomial.h"

#include <boost/rational.hpp>

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace geometer::exact
{
namespace
{

using Fraction = boost::rational<BigInt>;
using FractionPolynomial = std::vector<Fraction>;

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
        throw std::overflow_error("exact polynomial budget estimate overflow");
    }
    return left + right;
}

std::uint64_t checked_multiply(std::uint64_t left, std::uint64_t right)
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left)
    {
        throw std::overflow_error("exact polynomial budget estimate overflow");
    }
    return left * right;
}

std::uint64_t magnitude_bytes(const BigInt& value)
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
    return checked_add(checked_multiply(static_cast<std::uint64_t>(backend.size() - 1),
                                        sizeof(boost::multiprecision::limb_type)),
                       checked_add(high_bits, 7) / 8);
}

std::uint64_t limbs(const BigInt& value)
{
    return std::max<std::uint64_t>(1, checked_add(magnitude_bytes(value), 3) / 4);
}

BigInt absolute(BigInt value)
{
    return value < 0 ? -value : value;
}

BigInt integer_gcd(BigInt left, BigInt right)
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

std::size_t effective_size(const std::vector<BigInt>& coefficients)
{
    std::size_t size = coefficients.size();
    while (size > 0 && coefficients[size - 1] == 0)
    {
        --size;
    }
    return size;
}

std::uint64_t coefficient_limbs(const std::vector<BigInt>& coefficients, std::size_t size)
{
    std::uint64_t total = 0;
    for (std::size_t index = 0; index < size; ++index)
    {
        if (magnitude_bytes(coefficients[index]) > 2048)
        {
            throw std::overflow_error("polynomial coefficient exceeds 16384 bits");
        }
        total = checked_add(total, limbs(coefficients[index]));
    }
    return total;
}

void trim(FractionPolynomial& polynomial)
{
    while (!polynomial.empty() && polynomial.back() == 0)
    {
        polynomial.pop_back();
    }
}

FractionPolynomial derivative(const FractionPolynomial& polynomial)
{
    FractionPolynomial result;
    if (polynomial.size() < 2)
    {
        return result;
    }
    result.reserve(polynomial.size() - 1);
    for (std::size_t index = 1; index < polynomial.size(); ++index)
    {
        result.push_back(polynomial[index] * BigInt(index));
    }
    trim(result);
    return result;
}

FractionPolynomial remainder(FractionPolynomial dividend, const FractionPolynomial& divisor)
{
    while (!dividend.empty() && dividend.size() >= divisor.size())
    {
        const std::size_t shift = dividend.size() - divisor.size();
        const Fraction factor = dividend.back() / divisor.back();
        for (std::size_t index = 0; index < divisor.size(); ++index)
        {
            dividend[index + shift] -= factor * divisor[index];
        }
        trim(dividend);
    }
    return dividend;
}

FractionPolynomial exact_quotient(FractionPolynomial dividend, const FractionPolynomial& divisor)
{
    FractionPolynomial quotient(dividend.size() - divisor.size() + 1, Fraction(0));
    while (!dividend.empty() && dividend.size() >= divisor.size())
    {
        const std::size_t shift = dividend.size() - divisor.size();
        const Fraction factor = dividend.back() / divisor.back();
        quotient[shift] = factor;
        for (std::size_t index = 0; index < divisor.size(); ++index)
        {
            dividend[index + shift] -= factor * divisor[index];
        }
        trim(dividend);
    }
    if (!dividend.empty())
    {
        throw std::logic_error("non-exact polynomial quotient");
    }
    trim(quotient);
    return quotient;
}

void append_sturm_tail(FractionPolynomial first, FractionPolynomial second,
                       std::vector<FractionPolynomial>& sequence)
{
    sequence.push_back(std::move(first));
    while (!second.empty())
    {
        sequence.push_back(second);
        FractionPolynomial next = remainder(sequence[sequence.size() - 2], second);
        for (Fraction& coefficient : next)
        {
            coefficient = -coefficient;
        }
        second = std::move(next);
    }
}

bool make_sturm_sequence(const Polynomial& polynomial, std::vector<FractionPolynomial>& sequence)
{
    FractionPolynomial first;
    first.reserve(polynomial.coefficients().size());
    for (const BigInt& coefficient : polynomial.coefficients())
    {
        first.emplace_back(coefficient);
    }
    FractionPolynomial second = derivative(first);
    sequence.push_back(std::move(first));
    while (!second.empty())
    {
        sequence.push_back(second);
        FractionPolynomial next = remainder(sequence[sequence.size() - 2], second);
        if (next.empty())
        {
            return second.size() == 1;
        }
        for (Fraction& coefficient : next)
        {
            coefficient = -coefficient;
        }
        second = std::move(next);
    }
    return false;
}

FractionPolynomial polynomial_gcd(FractionPolynomial left, FractionPolynomial right)
{
    while (!right.empty())
    {
        FractionPolynomial next = remainder(left, right);
        left = std::move(right);
        right = std::move(next);
    }
    if (!left.empty())
    {
        const Fraction leading = left.back();
        for (Fraction& coefficient : left)
        {
            coefficient /= leading;
        }
    }
    return left;
}

void make_distinct_root_sequence(FractionPolynomial polynomial,
                                 std::vector<FractionPolynomial>& sequence)
{
    const FractionPolynomial common = polynomial_gcd(polynomial, derivative(polynomial));
    if (common.size() > 1)
    {
        polynomial = exact_quotient(std::move(polynomial), common);
    }
    append_sturm_tail(polynomial, derivative(polynomial), sequence);
}

int sign(const Fraction& value)
{
    return value == 0 ? 0 : (value > 0 ? 1 : -1);
}

struct EvaluationContext
{
    Budget& budget;
};

bool sign_at(EvaluationContext& context, const FractionPolynomial& polynomial,
             const Fraction& point, int& result)
{
    if (!context.budget.consume_exact_predicate())
    {
        return false;
    }
    Fraction value = 0;
    for (auto coefficient = polynomial.rbegin(); coefficient != polynomial.rend(); ++coefficient)
    {
        value = value * point + *coefficient;
    }
    result = sign(value);
    return true;
}

std::uint32_t variations(const std::vector<int>& signs)
{
    std::uint32_t count = 0;
    int prior = 0;
    for (const int current : signs)
    {
        if (current != 0)
        {
            if (prior != 0 && current != prior)
            {
                ++count;
            }
            prior = current;
        }
    }
    return count;
}

bool variations_at(EvaluationContext& context, const std::vector<FractionPolynomial>& sequence,
                   const Fraction& point, std::uint32_t& result)
{
    std::vector<int> signs;
    signs.reserve(sequence.size());
    for (const FractionPolynomial& polynomial : sequence)
    {
        int value = 0;
        if (!sign_at(context, polynomial, point, value))
        {
            return false;
        }
        signs.push_back(value);
    }
    result = variations(signs);
    return true;
}

bool variations_at_infinity(EvaluationContext& context,
                            const std::vector<FractionPolynomial>& sequence, bool positive,
                            std::uint32_t& result)
{
    std::vector<int> signs;
    signs.reserve(sequence.size());
    for (const FractionPolynomial& polynomial : sequence)
    {
        if (!context.budget.consume_exact_predicate())
        {
            return false;
        }
        int value = sign(polynomial.back());
        if (!positive && (polynomial.size() - 1) % 2 != 0)
        {
            value = -value;
        }
        signs.push_back(value);
    }
    result = variations(signs);
    return true;
}

enum class EvaluationStatus
{
    ok,
    endpoint_root,
    resource_limit,
};

EvaluationStatus roots_less_than(EvaluationContext& context,
                                 const std::vector<FractionPolynomial>& sequence,
                                 const Fraction& point, std::uint32_t& count)
{
    int polynomial_sign = 0;
    if (!sign_at(context, sequence.front(), point, polynomial_sign))
    {
        return EvaluationStatus::resource_limit;
    }
    if (polynomial_sign == 0)
    {
        return EvaluationStatus::endpoint_root;
    }
    std::uint32_t negative_infinity = 0;
    std::uint32_t point_variations = 0;
    if (!variations_at_infinity(context, sequence, false, negative_infinity) ||
        !variations_at(context, sequence, point, point_variations))
    {
        return EvaluationStatus::resource_limit;
    }
    count = negative_infinity - point_variations;
    return EvaluationStatus::ok;
}

BigInt root_bound(const Polynomial& polynomial)
{
    const BigInt leading = polynomial.coefficients().back();
    BigInt largest = 0;
    for (std::size_t index = 0; index + 1 < polynomial.coefficients().size(); ++index)
    {
        largest = std::max(largest, absolute(polynomial.coefficients()[index]));
    }
    BigInt quotient = largest / leading;
    if (largest % leading != 0)
    {
        ++quotient;
    }
    return quotient + 1;
}

EvaluationStatus cell_for_ordinal(EvaluationContext& context,
                                  const std::vector<FractionPolynomial>& sequence,
                                  const BigInt& bound, std::uint32_t precision,
                                  std::uint32_t ordinal, BigInt& cell, std::uint32_t& cell_roots)
{
    const BigInt denominator = BigInt(1) << precision;
    BigInt low = -bound * denominator;
    BigInt high = bound * denominator;
    while (high - low > 1)
    {
        const BigInt middle = (low + high) / 2;
        std::uint32_t count = 0;
        const EvaluationStatus status =
            roots_less_than(context, sequence, Fraction(middle, denominator), count);
        if (status != EvaluationStatus::ok)
        {
            return status;
        }
        (count <= ordinal ? low : high) = middle;
    }
    std::uint32_t below = 0;
    std::uint32_t above = 0;
    const EvaluationStatus below_status =
        roots_less_than(context, sequence, Fraction(low, denominator), below);
    if (below_status != EvaluationStatus::ok)
    {
        return below_status;
    }
    const EvaluationStatus above_status =
        roots_less_than(context, sequence, Fraction(high, denominator), above);
    if (above_status != EvaluationStatus::ok)
    {
        return above_status;
    }
    cell = low;
    cell_roots = above - below;
    return EvaluationStatus::ok;
}

struct ThomResult
{
    EvaluationStatus status = EvaluationStatus::ok;
    std::optional<std::vector<std::int8_t>> signs;
};

ThomResult thom_signs(EvaluationContext& context, const std::vector<FractionPolynomial>& sequence,
                      const BigInt& bound, std::uint32_t ordinal, std::uint32_t initial_precision,
                      std::uint32_t maximum_precision)
{
    for (std::uint32_t precision = initial_precision; precision <= maximum_precision; ++precision)
    {
        if (!context.budget.consume_interval_refinement())
        {
            return {EvaluationStatus::resource_limit, std::nullopt};
        }
        BigInt cell;
        std::uint32_t cell_roots = 0;
        const EvaluationStatus cell_status =
            cell_for_ordinal(context, sequence, bound, precision, ordinal, cell, cell_roots);
        if (cell_status != EvaluationStatus::ok)
        {
            return {cell_status, std::nullopt};
        }
        const BigInt denominator = BigInt(1) << precision;
        const Fraction lower(cell, denominator);
        const Fraction upper(cell + 1, denominator);
        std::vector<std::int8_t> result;
        FractionPolynomial current = sequence.front();
        bool resolved = true;
        for (std::size_t order = 1; order < sequence.front().size(); ++order)
        {
            current = derivative(current);
            const FractionPolynomial common = polynomial_gcd(sequence.front(), current);
            if (common.size() > 1)
            {
                std::vector<FractionPolynomial> common_sturm;
                append_sturm_tail(common, derivative(common), common_sturm);
                std::uint32_t lower_variations = 0;
                std::uint32_t upper_variations = 0;
                if (!variations_at(context, common_sturm, lower, lower_variations) ||
                    !variations_at(context, common_sturm, upper, upper_variations))
                {
                    return {EvaluationStatus::resource_limit, std::nullopt};
                }
                if (lower_variations - upper_variations == 1)
                {
                    result.push_back(0);
                    continue;
                }
            }
            int lower_sign = 0;
            int upper_sign = 0;
            if (!sign_at(context, current, lower, lower_sign) ||
                !sign_at(context, current, upper, upper_sign))
            {
                return {EvaluationStatus::resource_limit, std::nullopt};
            }
            std::vector<FractionPolynomial> derivative_sturm;
            make_distinct_root_sequence(current, derivative_sturm);
            std::uint32_t lower_variations = 0;
            std::uint32_t upper_variations = 0;
            if (!variations_at(context, derivative_sturm, lower, lower_variations) ||
                !variations_at(context, derivative_sturm, upper, upper_variations))
            {
                return {EvaluationStatus::resource_limit, std::nullopt};
            }
            const std::uint32_t derivative_roots = lower_variations - upper_variations;
            if (lower_sign == 0 || upper_sign == 0 || lower_sign != upper_sign ||
                derivative_roots != 0)
            {
                resolved = false;
                break;
            }
            result.push_back(static_cast<std::int8_t>(lower_sign));
        }
        if (resolved)
        {
            return {EvaluationStatus::ok, std::move(result)};
        }
    }
    return {EvaluationStatus::resource_limit, std::nullopt};
}

std::uint64_t root_work_bound(const Polynomial& polynomial, std::uint32_t precision)
{
    std::uint64_t width = checked_add(16, checked_add(precision / 32, 1));
    for (const BigInt& coefficient : polynomial.coefficients())
    {
        width = checked_add(width, limbs(coefficient));
    }
    const std::uint64_t order = static_cast<std::uint64_t>(polynomial.degree() + 1);
    const std::uint64_t order_squared = checked_multiply(order, order);
    const std::uint64_t order_to_fifth =
        checked_multiply(checked_multiply(order_squared, order_squared), order);
    std::uint64_t coefficient_growth = 1;
    for (std::size_t degree = 0; degree < polynomial.degree(); ++degree)
    {
        coefficient_growth = checked_multiply(coefficient_growth, 2);
    }
    return checked_multiply(coefficient_growth,
                            checked_multiply(order_to_fifth, checked_multiply(width, width)));
}

std::uint64_t root_storage_bound(const Polynomial& polynomial, std::uint32_t precision)
{
    const std::uint64_t order = static_cast<std::uint64_t>(polynomial.degree() + 1);
    const std::uint64_t width = checked_add(
        checked_add(precision / 8, 32),
        checked_multiply(static_cast<std::uint64_t>(polynomial.coefficients().size()), 2048));
    std::uint64_t coefficient_growth = 1;
    for (std::size_t degree = 0; degree < polynomial.degree(); ++degree)
    {
        coefficient_growth = checked_multiply(coefficient_growth, 2);
    }
    return checked_add(256,
                       checked_multiply(checked_multiply(coefficient_growth, 8),
                                        checked_multiply(checked_multiply(order, order), width)));
}

} // namespace

Polynomial::Polynomial(Budget& budget, std::uint64_t charged_bytes,
                       std::vector<BigInt> coefficients)
    : budget_(&budget), charged_bytes_(charged_bytes), coefficients_(std::move(coefficients))
{
}

Polynomial::~Polynomial()
{
    if (budget_ != nullptr)
    {
        budget_->release_storage(charged_bytes_);
    }
}

Polynomial::Polynomial(Polynomial&& other) noexcept
    : budget_(std::exchange(other.budget_, nullptr)),
      charged_bytes_(std::exchange(other.charged_bytes_, 0)),
      coefficients_(std::move(other.coefficients_))
{
}

Polynomial& Polynomial::operator=(Polynomial&& other) noexcept
{
    if (this != &other)
    {
        if (budget_ != nullptr)
        {
            budget_->release_storage(charged_bytes_);
        }
        budget_ = std::exchange(other.budget_, nullptr);
        charged_bytes_ = std::exchange(other.charged_bytes_, 0);
        coefficients_ = std::move(other.coefficients_);
    }
    return *this;
}

const std::vector<BigInt>& Polynomial::coefficients() const
{
    return coefficients_;
}

std::size_t Polynomial::degree() const
{
    return coefficients_.size() - 1;
}

IsolatedRootSet::IsolatedRootSet(Budget& budget, std::uint64_t charged_bytes,
                                 std::vector<IsolatedRoot> roots)
    : budget_(&budget), charged_bytes_(charged_bytes), roots_(std::move(roots))
{
}

IsolatedRootSet::~IsolatedRootSet()
{
    if (budget_ != nullptr)
    {
        budget_->release_storage(charged_bytes_);
    }
}

IsolatedRootSet::IsolatedRootSet(IsolatedRootSet&& other) noexcept
    : budget_(std::exchange(other.budget_, nullptr)),
      charged_bytes_(std::exchange(other.charged_bytes_, 0)), roots_(std::move(other.roots_))
{
}

IsolatedRootSet& IsolatedRootSet::operator=(IsolatedRootSet&& other) noexcept
{
    if (this != &other)
    {
        if (budget_ != nullptr)
        {
            budget_->release_storage(charged_bytes_);
        }
        budget_ = std::exchange(other.budget_, nullptr);
        charged_bytes_ = std::exchange(other.charged_bytes_, 0);
        roots_ = std::move(other.roots_);
    }
    return *this;
}

const std::vector<IsolatedRoot>& IsolatedRootSet::roots() const
{
    return roots_;
}

PolynomialResult make_primitive_polynomial(Budget& budget, const std::vector<BigInt>& coefficients)
{
    try
    {
        const std::size_t size = effective_size(coefficients);
        if (size == 0 || size > 65)
        {
            return {Error::invalid_argument, std::nullopt};
        }
        const std::uint64_t total_limbs = coefficient_limbs(coefficients, size);
        const std::uint64_t storage = checked_add(
            128, checked_add(checked_multiply(total_limbs, 64),
                             checked_multiply(static_cast<std::uint64_t>(size), sizeof(BigInt))));
        const std::uint64_t normalization_width =
            checked_add(total_limbs, static_cast<std::uint64_t>(size));
        const std::uint64_t work = checked_multiply(
            checked_multiply(normalization_width, normalization_width), normalization_width);
        StorageReservation reservation(budget, storage);
        if (!reservation.acquired() || !budget.consume_work(work))
        {
            return {Error::resource_limit_exceeded, std::nullopt};
        }
        std::vector<BigInt> normalized(coefficients.begin(), coefficients.begin() + size);
        BigInt content = 0;
        for (const BigInt& coefficient : normalized)
        {
            content = integer_gcd(std::move(content), coefficient);
        }
        for (BigInt& coefficient : normalized)
        {
            coefficient /= content;
        }
        if (normalized.back() < 0)
        {
            for (BigInt& coefficient : normalized)
            {
                coefficient = -coefficient;
            }
        }
        reservation.commit();
        return {Error::none, Polynomial(budget, storage, std::move(normalized))};
    }
    catch (const std::exception&)
    {
        return {Error::resource_limit_exceeded, std::nullopt};
    }
}

PolynomialResult make_square_free_polynomial(Budget& budget, const Polynomial& polynomial)
{
    try
    {
        const std::uint64_t work = root_work_bound(polynomial, 0);
        const std::uint64_t storage = root_storage_bound(polynomial, 0);
        StorageReservation reservation(budget, storage);
        if (!reservation.acquired() || !budget.consume_work(work))
        {
            return {Error::resource_limit_exceeded, std::nullopt};
        }
        FractionPolynomial source;
        source.reserve(polynomial.coefficients().size());
        for (const BigInt& coefficient : polynomial.coefficients())
        {
            source.emplace_back(coefficient);
        }
        const FractionPolynomial common = polynomial_gcd(source, derivative(source));
        const FractionPolynomial square_free =
            common.size() > 1 ? exact_quotient(std::move(source), common) : std::move(source);
        BigInt common_denominator = 1;
        for (const Fraction& coefficient : square_free)
        {
            const BigInt divisor = integer_gcd(common_denominator, coefficient.denominator());
            common_denominator *= coefficient.denominator() / divisor;
        }
        std::vector<BigInt> coefficients;
        coefficients.reserve(square_free.size());
        for (const Fraction& coefficient : square_free)
        {
            coefficients.push_back(coefficient.numerator() *
                                   (common_denominator / coefficient.denominator()));
        }
        return make_primitive_polynomial(budget, coefficients);
    }
    catch (const std::exception&)
    {
        return {Error::resource_limit_exceeded, std::nullopt};
    }
}

RootIsolationResult isolate_real_roots(Budget& budget, const Polynomial& polynomial,
                                       std::uint32_t maximum_precision)
{
    if (polynomial.degree() < 2 || maximum_precision > 4096)
    {
        return {Error::invalid_argument, std::nullopt};
    }
    try
    {
        const std::uint64_t work = root_work_bound(polynomial, maximum_precision);
        const std::uint64_t storage = root_storage_bound(polynomial, maximum_precision);
        StorageReservation reservation(budget, storage);
        if (!reservation.acquired() || !budget.consume_work(work))
        {
            return {Error::resource_limit_exceeded, std::nullopt};
        }
        std::vector<FractionPolynomial> sturm;
        if (!make_sturm_sequence(polynomial, sturm))
        {
            return {Error::invalid_argument, std::nullopt};
        }
        EvaluationContext context{budget};
        std::uint32_t negative_infinity = 0;
        std::uint32_t positive_infinity = 0;
        if (!variations_at_infinity(context, sturm, false, negative_infinity) ||
            !variations_at_infinity(context, sturm, true, positive_infinity))
        {
            return {Error::resource_limit_exceeded, std::nullopt};
        }
        const std::uint32_t root_count = negative_infinity - positive_infinity;
        const BigInt bound = root_bound(polynomial);
        std::vector<IsolatedRoot> roots(root_count);
        std::vector<bool> isolated(root_count, false);
        std::uint32_t remaining = root_count;
        for (std::uint32_t precision = 0; precision <= maximum_precision && remaining > 0;
             ++precision)
        {
            for (std::uint32_t ordinal = 0; ordinal < root_count; ++ordinal)
            {
                if (isolated[ordinal])
                {
                    continue;
                }
                if (!budget.consume_interval_refinement())
                {
                    return {Error::resource_limit_exceeded, std::nullopt};
                }
                BigInt cell;
                std::uint32_t cell_roots = 0;
                const EvaluationStatus cell_status =
                    cell_for_ordinal(context, sturm, bound, precision, ordinal, cell, cell_roots);
                if (cell_status == EvaluationStatus::resource_limit)
                {
                    return {Error::resource_limit_exceeded, std::nullopt};
                }
                if (cell_status != EvaluationStatus::ok)
                {
                    return {Error::invalid_argument, std::nullopt};
                }
                if (cell_roots == 1)
                {
                    ThomResult signs =
                        thom_signs(context, sturm, bound, ordinal, precision, maximum_precision);
                    if (signs.status != EvaluationStatus::ok || !signs.signs.has_value())
                    {
                        return {signs.status == EvaluationStatus::endpoint_root
                                    ? Error::invalid_argument
                                    : Error::resource_limit_exceeded,
                                std::nullopt};
                    }
                    roots[ordinal] = {ordinal, precision, std::move(cell), std::move(*signs.signs)};
                    isolated[ordinal] = true;
                    --remaining;
                }
            }
        }
        if (remaining != 0)
        {
            return {Error::resource_limit_exceeded, std::nullopt};
        }
        reservation.commit();
        return {Error::none, IsolatedRootSet(budget, storage, std::move(roots))};
    }
    catch (const std::exception&)
    {
        return {Error::resource_limit_exceeded, std::nullopt};
    }
}

RootIntervalCountResult count_real_roots_in_dyadic_interval(Budget& budget,
                                                            const Polynomial& polynomial,
                                                            const BigInt& lower_k,
                                                            const BigInt& upper_k,
                                                            std::uint32_t precision)
{
    if (polynomial.degree() == 0 || precision > 4096 || lower_k >= upper_k)
        return {Error::invalid_argument, 0, 0};
    try
    {
        const std::uint64_t work = root_work_bound(polynomial, precision);
        const std::uint64_t storage = root_storage_bound(polynomial, precision);
        StorageReservation reservation(budget, storage);
        if (!reservation.acquired() || !budget.consume_work(work) ||
            !budget.consume_interval_refinement())
            return {Error::resource_limit_exceeded, 0, 0};
        std::vector<FractionPolynomial> sturm;
        if (!make_sturm_sequence(polynomial, sturm))
            return {Error::invalid_argument, 0, 0};
        const BigInt denominator = BigInt(1) << precision;
        EvaluationContext context{budget};
        std::uint32_t below = 0;
        std::uint32_t above = 0;
        const EvaluationStatus lower_status =
            roots_less_than(context, sturm, Fraction(lower_k, denominator), below);
        if (lower_status == EvaluationStatus::resource_limit)
            return {Error::resource_limit_exceeded, 0, 0};
        if (lower_status != EvaluationStatus::ok)
            return {Error::invalid_argument, 0, 0};
        const EvaluationStatus upper_status =
            roots_less_than(context, sturm, Fraction(upper_k, denominator), above);
        if (upper_status == EvaluationStatus::resource_limit)
            return {Error::resource_limit_exceeded, 0, 0};
        if (upper_status != EvaluationStatus::ok)
            return {Error::invalid_argument, 0, 0};
        return {Error::none, above - below, below};
    }
    catch (const std::exception&)
    {
        return {Error::resource_limit_exceeded, 0, 0};
    }
}

RootRefinementResult refine_real_root(Budget& budget, const Polynomial& polynomial,
                                      std::uint32_t ordinal, std::uint32_t precision)
{
    if (polynomial.degree() < 2 || ordinal >= polynomial.degree() || precision > 4096)
        return {Error::invalid_argument, std::nullopt};
    try
    {
        const std::uint64_t work = root_work_bound(polynomial, precision);
        const std::uint64_t storage = root_storage_bound(polynomial, precision);
        StorageReservation reservation(budget, storage);
        if (!reservation.acquired() || !budget.consume_work(work) ||
            !budget.consume_interval_refinement())
            return {Error::resource_limit_exceeded, std::nullopt};
        std::vector<FractionPolynomial> sturm;
        if (!make_sturm_sequence(polynomial, sturm))
            return {Error::invalid_argument, std::nullopt};
        EvaluationContext context{budget};
        BigInt cell;
        std::uint32_t cell_roots = 0;
        const EvaluationStatus status = cell_for_ordinal(context, sturm, root_bound(polynomial),
                                                         precision, ordinal, cell, cell_roots);
        if (status == EvaluationStatus::resource_limit)
            return {Error::resource_limit_exceeded, std::nullopt};
        if (status != EvaluationStatus::ok || cell_roots != 1)
            return {Error::invalid_argument, std::nullopt};
        return {Error::none, DyadicRootInterval{precision, std::move(cell)}};
    }
    catch (const std::exception&)
    {
        return {Error::resource_limit_exceeded, std::nullopt};
    }
}

RootIntervalCountResult count_real_roots_below_rational(Budget& budget,
                                                        const Polynomial& polynomial,
                                                        const BigInt& numerator,
                                                        const BigInt& denominator)
{
    if (polynomial.degree() < 2 || denominator <= 0)
        return {Error::invalid_argument, 0, 0};
    try
    {
        const std::uint64_t work = root_work_bound(polynomial, 0);
        const std::uint64_t storage = root_storage_bound(polynomial, 0);
        StorageReservation reservation(budget, storage);
        if (!reservation.acquired() || !budget.consume_work(work))
            return {Error::resource_limit_exceeded, 0, 0};
        std::vector<FractionPolynomial> sturm;
        if (!make_sturm_sequence(polynomial, sturm))
            return {Error::invalid_argument, 0, 0};
        EvaluationContext context{budget};
        std::uint32_t count = 0;
        const EvaluationStatus status =
            roots_less_than(context, sturm, Fraction(numerator, denominator), count);
        if (status == EvaluationStatus::resource_limit)
            return {Error::resource_limit_exceeded, 0, 0};
        if (status != EvaluationStatus::ok)
            return {Error::invalid_argument, 0, 0};
        return {Error::none, count, count};
    }
    catch (const std::exception&)
    {
        return {Error::resource_limit_exceeded, 0, 0};
    }
}

} // namespace geometer::exact

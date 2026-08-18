#include "geometer/exact_factorization.h"

#include <boost/rational.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>

namespace geometer::exact
{
namespace
{

using Fraction = boost::rational<BigInt>;
using IntegerPolynomial = std::vector<BigInt>;
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
        if (acquired_)
        {
            budget_.release_storage(bytes_);
        }
    }
    [[nodiscard]] bool acquired() const
    {
        return acquired_;
    }

  private:
    Budget& budget_;
    std::uint64_t bytes_;
    bool acquired_;
};

std::uint64_t checked_add(std::uint64_t left, std::uint64_t right)
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
        throw std::overflow_error("exact factorization budget estimate overflow");
    return left + right;
}

std::uint64_t checked_multiply(std::uint64_t left, std::uint64_t right)
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left)
        throw std::overflow_error("exact factorization budget estimate overflow");
    return left * right;
}

BigInt absolute(BigInt value)
{
    return value < 0 ? -value : value;
}

BigInt gcd(BigInt left, BigInt right)
{
    left = absolute(std::move(left));
    right = absolute(std::move(right));
    while (right != 0)
    {
        BigInt next = left % right;
        left = std::move(right);
        right = std::move(next);
    }
    return left;
}

void trim(IntegerPolynomial& polynomial)
{
    while (!polynomial.empty() && polynomial.back() == 0)
        polynomial.pop_back();
}

std::uint64_t bit_length(const BigInt& value)
{
    if (value == 0)
        return 1;
    const auto& backend = value.backend();
    auto limb = backend.limbs()[backend.size() - 1];
    std::uint64_t high = 0;
    while (limb != 0)
    {
        ++high;
        limb >>= 1;
    }
    return checked_add(checked_multiply(static_cast<std::uint64_t>(backend.size() - 1),
                                        sizeof(boost::multiprecision::limb_type) * 8),
                       high);
}

BigInt evaluate(const IntegerPolynomial& polynomial, std::int64_t point)
{
    BigInt value = 0;
    for (auto coefficient = polynomial.rbegin(); coefficient != polynomial.rend(); ++coefficient)
        value = value * point + *coefficient;
    return value;
}

bool exact_quotient(IntegerPolynomial dividend, const IntegerPolynomial& divisor,
                    IntegerPolynomial& quotient)
{
    quotient.assign(dividend.size() - divisor.size() + 1, 0);
    while (dividend.size() >= divisor.size())
    {
        if (dividend.back() % divisor.back() != 0)
            return false;
        const std::size_t shift = dividend.size() - divisor.size();
        const BigInt factor = dividend.back() / divisor.back();
        quotient[shift] = factor;
        for (std::size_t index = 0; index < divisor.size(); ++index)
            dividend[index + shift] -= factor * divisor[index];
        trim(dividend);
        if (dividend.empty())
            break;
    }
    trim(quotient);
    return dividend.empty();
}

FractionPolynomial multiply_linear(const FractionPolynomial& polynomial, std::int64_t root)
{
    FractionPolynomial result(polynomial.size() + 1, 0);
    for (std::size_t index = 0; index < polynomial.size(); ++index)
    {
        result[index] -= polynomial[index] * root;
        result[index + 1] += polynomial[index];
    }
    return result;
}

bool interpolate(const std::vector<std::int64_t>& points, const std::vector<BigInt>& values,
                 IntegerPolynomial& result)
{
    FractionPolynomial sum(points.size(), 0);
    for (std::size_t index = 0; index < points.size(); ++index)
    {
        FractionPolynomial basis = {1};
        BigInt denominator = 1;
        for (std::size_t other = 0; other < points.size(); ++other)
        {
            if (other == index)
                continue;
            basis = multiply_linear(basis, points[other]);
            denominator *= points[index] - points[other];
        }
        const Fraction scale(values[index], denominator);
        for (std::size_t coefficient = 0; coefficient < basis.size(); ++coefficient)
            sum[coefficient] += basis[coefficient] * scale;
    }
    result.clear();
    result.reserve(sum.size());
    for (const Fraction& coefficient : sum)
    {
        if (coefficient.denominator() != 1)
            return false;
        result.push_back(coefficient.numerator());
    }
    trim(result);
    if (result.size() < 2)
        return false;
    BigInt content = 0;
    for (const BigInt& coefficient : result)
        content = gcd(std::move(content), coefficient);
    for (BigInt& coefficient : result)
        coefficient /= content;
    if (result.back() < 0)
        for (BigInt& coefficient : result)
            coefficient = -coefficient;
    return true;
}

enum class SearchStatus
{
    found,
    irreducible,
    resource_limit
};

BigInt integer_sqrt(const BigInt& value)
{
    if (value <= 0)
        return 0;
    BigInt estimate = BigInt(1) << static_cast<unsigned>((bit_length(value) + 2) / 2);
    while (true)
    {
        BigInt next = (estimate + value / estimate) / 2;
        if (next >= estimate)
            return estimate;
        estimate = std::move(next);
    }
}

// A primitive quadratic has a linear integer factor exactly when its
// discriminant is a perfect square (rational root theorem plus Gauss's
// lemma), so quadratics never need the divisor-enumeration search. This
// keeps square roots of large rationals (for example governed-span capsule
// lengths near the coordinate limit) inside the work budget.
SearchStatus find_quadratic_factor(Budget& budget, const IntegerPolynomial& polynomial,
                                   IntegerPolynomial& factor)
{
    try
    {
        const std::uint64_t bits = std::max(
            {bit_length(polynomial[0]), bit_length(polynomial[1]), bit_length(polynomial[2])});
        const std::uint64_t limbs = checked_add(bits, 31) / 32;
        if (!budget.consume_work(checked_multiply(
                16, checked_multiply(checked_add(bits, 1), checked_multiply(limbs, limbs)))))
            return SearchStatus::resource_limit;
        const BigInt discriminant =
            polynomial[1] * polynomial[1] - 4 * polynomial[2] * polynomial[0];
        if (discriminant < 0)
            return SearchStatus::irreducible;
        const BigInt root = integer_sqrt(discriminant);
        if (root * root != discriminant)
            return SearchStatus::irreducible;
        BigInt numerator = root - polynomial[1];
        BigInt denominator = 2 * polynomial[2];
        const BigInt divisor = gcd(numerator, denominator);
        numerator /= divisor;
        denominator /= divisor;
        if (denominator < 0)
        {
            numerator = -numerator;
            denominator = -denominator;
        }
        factor = {-numerator, std::move(denominator)};
        return SearchStatus::found;
    }
    catch (const std::exception&)
    {
        return SearchStatus::resource_limit;
    }
}

bool signed_divisors(Budget& budget, const BigInt& value, std::vector<BigInt>& divisors)
{
    const BigInt magnitude = absolute(value);
    for (BigInt candidate = 1; candidate <= magnitude / candidate; ++candidate)
    {
        if (!budget.consume_work(8))
            return false;
        if (magnitude % candidate != 0)
            continue;
        const BigInt pair = magnitude / candidate;
        divisors.push_back(candidate);
        divisors.push_back(-candidate);
        if (pair != candidate)
        {
            divisors.push_back(pair);
            divisors.push_back(-pair);
        }
        if (divisors.size() > 2000)
            return false;
    }
    std::sort(divisors.begin(), divisors.end());
    return true;
}

struct Enumeration
{
    Budget& budget;
    const IntegerPolynomial& source;
    const std::vector<std::int64_t>& points;
    const std::vector<std::vector<BigInt>>& choices;
    std::vector<BigInt> selected;
    IntegerPolynomial factor;
    bool exhausted = false;

    bool visit(std::size_t index)
    {
        if (index != choices.size())
        {
            for (const BigInt& value : choices[index])
            {
                selected[index] = value;
                if (visit(index + 1))
                    return true;
                if (exhausted)
                    return false;
            }
            return false;
        }
        const std::uint64_t order = static_cast<std::uint64_t>(points.size());
        std::uint64_t value_bits = 1;
        std::uint64_t point_bits = 1;
        for (const BigInt& value : selected)
            value_bits = std::max(value_bits, bit_length(value));
        for (const std::int64_t point : points)
        {
            std::uint64_t magnitude = static_cast<std::uint64_t>(point < 0 ? -point : point);
            std::uint64_t bits = 1;
            while (magnitude > 1)
            {
                ++bits;
                magnitude >>= 1;
            }
            point_bits = std::max(point_bits, bits);
        }
        const std::uint64_t bits =
            checked_add(checked_add(value_bits, checked_multiply(order, point_bits)), 31);
        const std::uint64_t limbs = bits / 32;
        const std::uint64_t order_cubed = checked_multiply(checked_multiply(order, order), order);
        const std::uint64_t work =
            checked_multiply(16, checked_multiply(order_cubed, checked_multiply(limbs, limbs)));
        if (!budget.consume_work(work))
        {
            exhausted = true;
            return false;
        }
        IntegerPolynomial candidate;
        IntegerPolynomial quotient;
        if (interpolate(points, selected, candidate) && candidate.size() < source.size() &&
            exact_quotient(source, candidate, quotient))
        {
            factor = std::move(candidate);
            return true;
        }
        return false;
    }
};

SearchStatus find_factor(Budget& budget, const IntegerPolynomial& polynomial,
                         IntegerPolynomial& factor)
{
    const std::size_t degree = polynomial.size() - 1;
    if (degree == 2)
        return find_quadratic_factor(budget, polynomial, factor);
    for (std::size_t candidate_degree = 1; candidate_degree <= degree / 2; ++candidate_degree)
    {
        std::vector<std::int64_t> points;
        std::vector<std::vector<BigInt>> choices;
        for (std::uint64_t cursor = 0; points.size() <= candidate_degree; ++cursor)
        {
            const std::int64_t point =
                cursor == 0 ? 0
                            : (cursor % 2 ? 1 : -1) * static_cast<std::int64_t>((cursor + 1) / 2);
            std::uint64_t coefficient_bits = 1;
            for (const BigInt& coefficient : polynomial)
                coefficient_bits = std::max(coefficient_bits, bit_length(coefficient));
            const std::uint64_t limbs = checked_add(checked_add(coefficient_bits, cursor), 31) / 32;
            const std::uint64_t work =
                checked_multiply(16, checked_multiply(static_cast<std::uint64_t>(degree + 1),
                                                      checked_multiply(limbs, limbs)));
            if (!budget.consume_work(work))
                return SearchStatus::resource_limit;
            const BigInt value = evaluate(polynomial, point);
            if (value == 0)
                continue;
            std::vector<BigInt> divisors;
            if (!signed_divisors(budget, value, divisors))
                return SearchStatus::resource_limit;
            points.push_back(point);
            choices.push_back(std::move(divisors));
        }
        Enumeration enumeration{budget, polynomial, points, choices,
                                std::vector<BigInt>(points.size())};
        if (enumeration.visit(0))
        {
            factor = std::move(enumeration.factor);
            return SearchStatus::found;
        }
        if (enumeration.exhausted)
            return SearchStatus::resource_limit;
    }
    return SearchStatus::irreducible;
}

} // namespace

PolynomialFactorSet::PolynomialFactorSet(std::vector<Polynomial> factors)
    : factors_(std::move(factors))
{
}

const std::vector<Polynomial>& PolynomialFactorSet::factors() const
{
    return factors_;
}

FactorizationResult factor_primitive_polynomial(Budget& budget, const Polynomial& polynomial)
{
    if (polynomial.degree() == 0)
        return {Error::invalid_argument, std::nullopt};
    try
    {
        StorageReservation reservation(budget, checked_multiply(192, checked_multiply(1024, 1024)));
        if (!reservation.acquired())
            return {Error::resource_limit_exceeded, std::nullopt};
        std::vector<IntegerPolynomial> pending = {polynomial.coefficients()};
        std::vector<IntegerPolynomial> factors;
        while (!pending.empty())
        {
            IntegerPolynomial current = std::move(pending.back());
            pending.pop_back();
            IntegerPolynomial factor;
            const SearchStatus status = find_factor(budget, current, factor);
            if (status == SearchStatus::resource_limit)
                return {Error::resource_limit_exceeded, std::nullopt};
            if (status == SearchStatus::irreducible)
            {
                factors.push_back(std::move(current));
                continue;
            }
            IntegerPolynomial quotient;
            if (!exact_quotient(current, factor, quotient))
                return {Error::invalid_argument, std::nullopt};
            pending.push_back(std::move(quotient));
            pending.push_back(std::move(factor));
        }
        std::sort(factors.begin(), factors.end());
        std::vector<Polynomial> output;
        output.reserve(factors.size());
        for (const IntegerPolynomial& factor : factors)
        {
            auto normalized = make_primitive_polynomial(budget, factor);
            if (normalized.error != Error::none || !normalized.value.has_value())
                return {normalized.error, std::nullopt};
            output.push_back(std::move(*normalized.value));
        }
        return {Error::none, PolynomialFactorSet(std::move(output))};
    }
    catch (const std::exception&)
    {
        return {Error::resource_limit_exceeded, std::nullopt};
    }
}

FactorRootSelectionResult select_unique_factor_root(Budget& budget,
                                                    const PolynomialFactorSet& factors,
                                                    const BigInt& lower_k, const BigInt& upper_k,
                                                    std::uint32_t precision)
{
    std::optional<std::size_t> selected_factor;
    std::uint32_t selected_ordinal = 0;
    for (std::size_t index = 0; index < factors.factors().size(); ++index)
    {
        const RootIntervalCountResult count = count_real_roots_in_dyadic_interval(
            budget, factors.factors()[index], lower_k, upper_k, precision);
        if (count.error != Error::none)
            return {FactorRootSelectionStatus::error, count.error, std::nullopt, 0};
        if (count.count == 0)
            continue;
        if (count.count != 1 || selected_factor.has_value())
            return {FactorRootSelectionStatus::needs_refinement, Error::none, std::nullopt, 0};
        selected_factor = index;
        selected_ordinal = count.first_ordinal;
    }
    if (!selected_factor.has_value())
        return {FactorRootSelectionStatus::error, Error::invalid_argument, std::nullopt, 0};
    return {FactorRootSelectionStatus::selected, Error::none, selected_factor, selected_ordinal};
}

} // namespace geometer::exact

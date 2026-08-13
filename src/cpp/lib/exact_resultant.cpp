#include "geometer/exact_resultant.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace geometer::exact
{
namespace
{

using IntegerPolynomial = std::vector<BigInt>;
using PolynomialMatrix = std::vector<std::vector<IntegerPolynomial>>;

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
    {
        throw std::overflow_error("exact resultant budget estimate overflow");
    }
    return left + right;
}

std::uint64_t checked_multiply(std::uint64_t left, std::uint64_t right)
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left)
    {
        throw std::overflow_error("exact resultant budget estimate overflow");
    }
    return left * right;
}

std::uint64_t bit_length(const BigInt& value)
{
    if (value == 0)
    {
        return 1;
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
                                        sizeof(boost::multiprecision::limb_type) * 8),
                       high_bits);
}

void trim(IntegerPolynomial& polynomial)
{
    while (!polynomial.empty() && polynomial.back() == 0)
    {
        polynomial.pop_back();
    }
}

IntegerPolynomial subtract(IntegerPolynomial left, const IntegerPolynomial& right)
{
    left.resize(std::max(left.size(), right.size()), 0);
    for (std::size_t index = 0; index < right.size(); ++index)
    {
        left[index] -= right[index];
    }
    trim(left);
    return left;
}

IntegerPolynomial multiply(const IntegerPolynomial& left, const IntegerPolynomial& right)
{
    if (left.empty() || right.empty())
    {
        return {};
    }
    IntegerPolynomial result(left.size() + right.size() - 1, 0);
    for (std::size_t left_index = 0; left_index < left.size(); ++left_index)
    {
        for (std::size_t right_index = 0; right_index < right.size(); ++right_index)
        {
            result[left_index + right_index] += left[left_index] * right[right_index];
        }
    }
    trim(result);
    return result;
}

IntegerPolynomial exact_quotient(IntegerPolynomial dividend, const IntegerPolynomial& divisor)
{
    if (divisor.empty())
    {
        throw std::logic_error("zero polynomial divisor in exact resultant");
    }
    IntegerPolynomial quotient(
        dividend.size() < divisor.size() ? 0 : dividend.size() - divisor.size() + 1, 0);
    while (!dividend.empty() && dividend.size() >= divisor.size())
    {
        const std::size_t shift = dividend.size() - divisor.size();
        if (dividend.back() % divisor.back() != 0)
        {
            throw std::logic_error("non-exact Bareiss polynomial quotient");
        }
        const BigInt factor = dividend.back() / divisor.back();
        quotient[shift] = factor;
        for (std::size_t index = 0; index < divisor.size(); ++index)
        {
            dividend[index + shift] -= factor * divisor[index];
        }
        trim(dividend);
    }
    if (!dividend.empty())
    {
        throw std::logic_error("non-exact Bareiss polynomial remainder");
    }
    trim(quotient);
    return quotient;
}

IntegerPolynomial determinant_bareiss(PolynomialMatrix matrix)
{
    if (matrix.empty())
    {
        return {1};
    }
    IntegerPolynomial prior_pivot = {1};
    bool negate = false;
    for (std::size_t pivot_index = 0; pivot_index + 1 < matrix.size(); ++pivot_index)
    {
        std::size_t pivot_row = pivot_index;
        while (pivot_row < matrix.size() && matrix[pivot_row][pivot_index].empty())
        {
            ++pivot_row;
        }
        if (pivot_row == matrix.size())
        {
            return {};
        }
        if (pivot_row != pivot_index)
        {
            std::swap(matrix[pivot_row], matrix[pivot_index]);
            negate = !negate;
        }
        const IntegerPolynomial pivot = matrix[pivot_index][pivot_index];
        for (std::size_t row = pivot_index + 1; row < matrix.size(); ++row)
        {
            for (std::size_t column = pivot_index + 1; column < matrix.size(); ++column)
            {
                IntegerPolynomial numerator =
                    subtract(multiply(matrix[row][column], pivot),
                             multiply(matrix[row][pivot_index], matrix[pivot_index][column]));
                if (pivot_index != 0)
                {
                    numerator = exact_quotient(std::move(numerator), prior_pivot);
                }
                matrix[row][column] = std::move(numerator);
            }
            matrix[row][pivot_index].clear();
        }
        prior_pivot = pivot;
    }
    IntegerPolynomial result = matrix.back().back();
    if (negate)
    {
        for (BigInt& coefficient : result)
        {
            coefficient = -coefficient;
        }
    }
    trim(result);
    return result;
}

BigInt binomial(std::size_t order, std::size_t selected)
{
    selected = std::min(selected, order - selected);
    BigInt result = 1;
    for (std::size_t index = 1; index <= selected; ++index)
    {
        result *= order - selected + index;
        result /= index;
    }
    return result;
}

void add_coefficient(IntegerPolynomial& polynomial, std::size_t degree, const BigInt& value)
{
    if (polynomial.size() <= degree)
    {
        polynomial.resize(degree + 1, 0);
    }
    polynomial[degree] += value;
    trim(polynomial);
}

std::vector<IntegerPolynomial> translated_coefficients(const Polynomial& polynomial)
{
    std::vector<IntegerPolynomial> result(polynomial.degree() + 1);
    for (std::size_t source_degree = 0; source_degree <= polynomial.degree(); ++source_degree)
    {
        for (std::size_t x_degree = 0; x_degree <= source_degree; ++x_degree)
        {
            const std::size_t y_degree = source_degree - x_degree;
            BigInt coefficient =
                polynomial.coefficients()[source_degree] * binomial(source_degree, x_degree);
            if (y_degree % 2 != 0)
            {
                coefficient = -coefficient;
            }
            add_coefficient(result[y_degree], x_degree, coefficient);
        }
    }
    return result;
}

std::vector<IntegerPolynomial> product_coefficients(const Polynomial& polynomial)
{
    std::vector<IntegerPolynomial> result(polynomial.degree() + 1);
    for (std::size_t source_degree = 0; source_degree <= polynomial.degree(); ++source_degree)
    {
        add_coefficient(result[polynomial.degree() - source_degree], source_degree,
                        polynomial.coefficients()[source_degree]);
    }
    return result;
}

PolynomialMatrix sylvester_matrix(const Polynomial& left,
                                  const std::vector<IntegerPolynomial>& right)
{
    const std::size_t left_degree = left.degree();
    const std::size_t right_degree = right.size() - 1;
    const std::size_t order = left_degree + right_degree;
    PolynomialMatrix matrix(order, std::vector<IntegerPolynomial>(order));
    for (std::size_t row = 0; row < right_degree; ++row)
    {
        for (std::size_t degree = 0; degree <= left_degree; ++degree)
        {
            matrix[row][row + degree] = {left.coefficients()[left_degree - degree]};
        }
    }
    for (std::size_t row = 0; row < left_degree; ++row)
    {
        for (std::size_t degree = 0; degree <= right_degree; ++degree)
        {
            matrix[right_degree + row][row + degree] = right[right_degree - degree];
        }
    }
    return matrix;
}

struct ResultantBudget
{
    std::uint64_t work = 0;
    std::uint64_t storage = 0;
};

ResultantBudget resultant_budget(const Polynomial& left, const Polynomial& right)
{
    const std::uint64_t left_degree = static_cast<std::uint64_t>(left.degree());
    const std::uint64_t right_degree = static_cast<std::uint64_t>(right.degree());
    const std::uint64_t output_degree = checked_multiply(left_degree, right_degree);
    if (output_degree > 64)
    {
        throw std::overflow_error("resultant degree exceeds exact backend limit");
    }
    std::uint64_t input_bits = 1;
    for (const BigInt& coefficient : left.coefficients())
    {
        input_bits = std::max(input_bits, bit_length(coefficient));
    }
    for (const BigInt& coefficient : right.coefficients())
    {
        input_bits = std::max(input_bits, bit_length(coefficient));
    }
    const std::uint64_t order = checked_add(left_degree, right_degree);
    const std::uint64_t coefficient_bits = checked_multiply(
        order, checked_add(input_bits, checked_add(std::max(left_degree, right_degree), order)));
    if (coefficient_bits > 16'384)
    {
        throw std::overflow_error("resultant coefficient estimate exceeds exact backend limit");
    }
    const std::uint64_t limbs = checked_add(coefficient_bits, 31) / 32;
    const std::uint64_t matrix_slots = checked_multiply(order, order);
    const std::uint64_t polynomial_slots = checked_add(output_degree, 1);
    const std::uint64_t polynomial_work = checked_multiply(polynomial_slots, polynomial_slots);
    const std::uint64_t work = checked_multiply(
        16,
        checked_multiply(checked_multiply(checked_multiply(matrix_slots, order), polynomial_work),
                         checked_multiply(checked_add(limbs, 1), checked_add(limbs, 1))));
    const std::uint64_t storage = checked_add(
        4096, checked_multiply(8, checked_multiply(checked_multiply(matrix_slots, polynomial_slots),
                                                   checked_add(32, checked_multiply(limbs, 8)))));
    return {work, storage};
}

ResultantBudget transform_budget(const Polynomial& polynomial, std::uint64_t output_degree)
{
    std::uint64_t coefficient_bits = 1;
    for (const BigInt& coefficient : polynomial.coefficients())
    {
        coefficient_bits = std::max(coefficient_bits, bit_length(coefficient));
    }
    const std::uint64_t limbs = checked_add(coefficient_bits, 31) / 32;
    const std::uint64_t slots = checked_add(output_degree, 1);
    return {
        checked_multiply(slots, checked_multiply(checked_add(limbs, 1), checked_add(limbs, 1))),
        checked_add(1024, checked_multiply(slots, checked_add(32, checked_multiply(limbs, 8))))};
}

PolynomialResult normalize_candidate(Budget& budget, const IntegerPolynomial& coefficients)
{
    auto primitive = make_primitive_polynomial(budget, coefficients);
    if (primitive.error != Error::none || !primitive.value.has_value())
    {
        return primitive;
    }
    return make_square_free_polynomial(budget, *primitive.value);
}

} // namespace

PolynomialResult make_square_free_resultant(Budget& budget, const Polynomial& left,
                                            const Polynomial& right, ResultantOperation operation)
{
    if (left.degree() < 2 || right.degree() < 2 ||
        (operation != ResultantOperation::sum && operation != ResultantOperation::product))
    {
        return {Error::invalid_argument, std::nullopt};
    }
    try
    {
        const ResultantBudget phase = resultant_budget(left, right);
        StorageReservation reservation(budget, phase.storage);
        if (!reservation.acquired() || !budget.consume_work(phase.work))
        {
            return {Error::resource_limit_exceeded, std::nullopt};
        }
        const std::vector<IntegerPolynomial> transformed = operation == ResultantOperation::sum
                                                               ? translated_coefficients(right)
                                                               : product_coefficients(right);
        const IntegerPolynomial resultant =
            determinant_bareiss(sylvester_matrix(left, transformed));
        if (resultant.empty())
        {
            return {Error::invalid_argument, std::nullopt};
        }
        return normalize_candidate(budget, resultant);
    }
    catch (const std::logic_error&)
    {
        return {Error::invalid_argument, std::nullopt};
    }
    catch (const std::exception&)
    {
        return {Error::resource_limit_exceeded, std::nullopt};
    }
}

PolynomialResult make_reciprocal_polynomial(Budget& budget, const Polynomial& polynomial)
{
    if (polynomial.coefficients().front() == 0)
    {
        return {Error::invalid_argument, std::nullopt};
    }
    try
    {
        const ResultantBudget phase = transform_budget(polynomial, polynomial.degree());
        StorageReservation reservation(budget, phase.storage);
        if (!reservation.acquired() || !budget.consume_work(phase.work))
        {
            return {Error::resource_limit_exceeded, std::nullopt};
        }
        std::vector<BigInt> coefficients(polynomial.coefficients().rbegin(),
                                         polynomial.coefficients().rend());
        return normalize_candidate(budget, coefficients);
    }
    catch (const std::exception&)
    {
        return {Error::resource_limit_exceeded, std::nullopt};
    }
}

PolynomialResult make_square_root_polynomial(Budget& budget, const Polynomial& polynomial)
{
    if (polynomial.degree() > 32)
    {
        return {Error::resource_limit_exceeded, std::nullopt};
    }
    try
    {
        const std::uint64_t output_degree = checked_multiply(polynomial.degree(), 2);
        const ResultantBudget phase = transform_budget(polynomial, output_degree);
        StorageReservation reservation(budget, phase.storage);
        if (!reservation.acquired() || !budget.consume_work(phase.work))
        {
            return {Error::resource_limit_exceeded, std::nullopt};
        }
        std::vector<BigInt> coefficients(static_cast<std::size_t>(output_degree) + 1, 0);
        for (std::size_t index = 0; index < polynomial.coefficients().size(); ++index)
        {
            coefficients[index * 2] = polynomial.coefficients()[index];
        }
        return normalize_candidate(budget, coefficients);
    }
    catch (const std::exception&)
    {
        return {Error::resource_limit_exceeded, std::nullopt};
    }
}

} // namespace geometer::exact

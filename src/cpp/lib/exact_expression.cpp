#include "geometer/exact_expression.h"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <utility>

namespace geometer::exact
{
namespace
{

struct FractionBound
{
    BigInt numerator;
    BigInt denominator = 1;
};

struct Bounds
{
    FractionBound lower;
    FractionBound upper;
};

struct BoundsResult
{
    Error error = Error::none;
    bool needs_refinement = false;
    std::optional<Bounds> value;
};

struct PhaseBudget
{
    std::uint64_t work = 0;
    std::uint64_t storage = 0;
};

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
            budget_.release_storage(bytes_);
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
        throw std::overflow_error("exact expression budget estimate overflow");
    return left + right;
}

std::uint64_t checked_multiply(std::uint64_t left, std::uint64_t right)
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left)
        throw std::overflow_error("exact expression budget estimate overflow");
    return left * right;
}

std::uint64_t bit_length(const BigInt& value)
{
    if (value == 0)
        return 1;
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

std::uint64_t limbs(const BigInt& value)
{
    return checked_add(bit_length(value), 31) / 32;
}

std::uint64_t polynomial_bit_length(const Polynomial& polynomial)
{
    std::uint64_t bits = 1;
    for (const BigInt& coefficient : polynomial.coefficients())
        bits = std::max(bits, bit_length(coefficient));
    return bits;
}

std::uint64_t value_bound_limbs(const CanonicalReal& value, std::uint32_t precision)
{
    if (value.kind() == CanonicalRealKind::rational)
        return checked_add(limbs(value.numerator()), limbs(value.denominator()));
    // Cauchy's bound is at most one plus the largest coefficient ratio. The
    // extra two limbs cover the ratio, sign, and outward dyadic endpoint.
    const std::uint64_t numerator_bits =
        checked_add(checked_add(polynomial_bit_length(*value.polynomial()), precision), 64);
    const std::uint64_t denominator_bits = checked_add(precision, 1);
    return checked_add(checked_add(numerator_bits, 31) / 32,
                       checked_add(denominator_bits, 31) / 32);
}

PhaseBudget bounds_budget(const CanonicalReal& left, const CanonicalReal* right,
                          std::uint32_t precision, std::uint64_t operation_factor)
{
    std::uint64_t width = value_bound_limbs(left, precision);
    if (right != nullptr)
        width = checked_add(width, value_bound_limbs(*right, precision));
    // Bounds hold two endpoints per value. The factor covers every live
    // product/quotient temporary used by interval arithmetic and conversion
    // to a common dyadic cell.
    width = checked_multiply(2, checked_add(width, 1));
    return {checked_multiply(operation_factor,
                             checked_multiply(checked_add(width, 1), checked_add(width, 1))),
            checked_add(4096, checked_multiply(operation_factor, checked_multiply(width, 8)))};
}

bool less(const FractionBound& left, const FractionBound& right)
{
    return left.numerator * right.denominator < right.numerator * left.denominator;
}

FractionBound normalized(BigInt numerator, BigInt denominator)
{
    if (denominator < 0)
    {
        numerator = -numerator;
        denominator = -denominator;
    }
    return {std::move(numerator), std::move(denominator)};
}

BigInt floor_divide(const BigInt& numerator, const BigInt& denominator)
{
    BigInt quotient = numerator / denominator;
    if (numerator < 0 && numerator % denominator != 0)
        --quotient;
    return quotient;
}

BigInt ceil_divide(const BigInt& numerator, const BigInt& denominator)
{
    return -floor_divide(-numerator, denominator);
}

BoundsResult value_bounds(Budget& budget, const CanonicalReal& value, std::uint32_t precision)
{
    if (value.kind() == CanonicalRealKind::rational)
    {
        FractionBound exact{value.numerator(), value.denominator()};
        return {Error::none, false, Bounds{exact, exact}};
    }
    RootRefinementResult interval =
        refine_real_root(budget, *value.polynomial(), value.root()->ordinal, precision);
    if (interval.error != Error::none || !interval.value)
        return {interval.error == Error::none ? Error::invalid_argument : interval.error, false,
                std::nullopt};
    const BigInt denominator = BigInt(1) << precision;
    return {Error::none, false,
            Bounds{{interval.value->interval_k, denominator},
                   {interval.value->interval_k + 1, denominator}}};
}

Bounds sum_bounds(const Bounds& left, const Bounds& right)
{
    return {{left.lower.numerator * right.lower.denominator +
                 right.lower.numerator * left.lower.denominator,
             left.lower.denominator * right.lower.denominator},
            {left.upper.numerator * right.upper.denominator +
                 right.upper.numerator * left.upper.denominator,
             left.upper.denominator * right.upper.denominator}};
}

Bounds product_bounds(const Bounds& left, const Bounds& right)
{
    std::array<FractionBound, 4> products = {
        normalized(left.lower.numerator * right.lower.numerator,
                   left.lower.denominator * right.lower.denominator),
        normalized(left.lower.numerator * right.upper.numerator,
                   left.lower.denominator * right.upper.denominator),
        normalized(left.upper.numerator * right.lower.numerator,
                   left.upper.denominator * right.lower.denominator),
        normalized(left.upper.numerator * right.upper.numerator,
                   left.upper.denominator * right.upper.denominator)};
    const auto minimum = std::min_element(products.begin(), products.end(), less);
    const auto maximum = std::max_element(products.begin(), products.end(), less);
    return {*minimum, *maximum};
}

std::pair<BigInt, BigInt> outward_dyadic(const Bounds& bounds, std::uint32_t precision)
{
    const BigInt scale = BigInt(1) << precision;
    return {floor_divide(bounds.lower.numerator * scale, bounds.lower.denominator),
            ceil_divide(bounds.upper.numerator * scale, bounds.upper.denominator)};
}

CanonicalRealResult rational_binary(Budget& budget, const CanonicalReal& left,
                                    const CanonicalReal& right, bool product)
{
    try
    {
        const std::uint64_t width =
            checked_add(checked_add(limbs(left.numerator()), limbs(left.denominator())),
                        checked_add(limbs(right.numerator()), limbs(right.denominator())));
        const PhaseBudget phase{
            checked_multiply(16, checked_multiply(checked_add(width, 1), checked_add(width, 1))),
            checked_add(1024, checked_multiply(width, 128))};
        StorageReservation reservation(budget, phase.storage);
        if (!reservation.acquired() || !budget.consume_work(phase.work))
            return {Error::resource_limit_exceeded, FactorRootSelectionStatus::error, std::nullopt};
        BigInt numerator;
        BigInt denominator;
        if (product)
        {
            numerator = left.numerator() * right.numerator();
            denominator = left.denominator() * right.denominator();
        }
        else
        {
            numerator =
                left.numerator() * right.denominator() + right.numerator() * left.denominator();
            denominator = left.denominator() * right.denominator();
        }
        return make_canonical_rational(budget, numerator, denominator);
    }
    catch (const std::exception&)
    {
        return {Error::resource_limit_exceeded, FactorRootSelectionStatus::error, std::nullopt};
    }
}

PolynomialResult rational_square_root_candidate(Budget& budget, const CanonicalReal& value)
{
    try
    {
        const std::uint64_t width =
            checked_add(limbs(value.numerator()), limbs(value.denominator()));
        const PhaseBudget phase{
            checked_multiply(8, checked_multiply(checked_add(width, 1), checked_add(width, 1))),
            checked_add(1024, checked_multiply(width, 128))};
        StorageReservation reservation(budget, phase.storage);
        if (!reservation.acquired() || !budget.consume_work(phase.work))
            return {Error::resource_limit_exceeded, std::nullopt};
        std::vector<BigInt> coefficients;
        coefficients.reserve(3);
        coefficients.push_back(-value.numerator());
        coefficients.emplace_back(0);
        coefficients.push_back(value.denominator());
        return make_primitive_polynomial(budget, coefficients);
    }
    catch (const std::exception&)
    {
        return {Error::resource_limit_exceeded, std::nullopt};
    }
}

template <typename PhaseFunction, typename BoundFunction>
CanonicalRealResult select_expression_value(Budget& budget, Polynomial candidate,
                                            const PhaseFunction& phase_for_precision,
                                            const BoundFunction& bounds_for_precision)
{
    auto factors = factor_primitive_polynomial(budget, candidate);
    if (factors.error != Error::none || !factors.value)
        return {factors.error, FactorRootSelectionStatus::error, std::nullopt};
    constexpr std::array<std::uint32_t, 5> precisions = {256, 512, 1024, 2048, 4096};
    for (const std::uint32_t precision : precisions)
    {
        try
        {
            const PhaseBudget phase = phase_for_precision(precision);
            StorageReservation reservation(budget, phase.storage);
            if (!reservation.acquired() || !budget.consume_work(phase.work))
                return {Error::resource_limit_exceeded, FactorRootSelectionStatus::error,
                        std::nullopt};
            BoundsResult bounds = bounds_for_precision(precision);
            if (bounds.error != Error::none)
                return {bounds.error, FactorRootSelectionStatus::error, std::nullopt};
            if (bounds.needs_refinement || !bounds.value)
                continue;
            auto [lower, upper] = outward_dyadic(*bounds.value, precision);
            // Candidate roots exactly on an outward endpoint still represent
            // the unique continuous expression value. Widen by one dyadic ulp
            // so the existing open-interval root counter handles this case.
            --lower;
            ++upper;
            FactorRootSelectionResult selected =
                select_unique_factor_root(budget, *factors.value, lower, upper, precision);
            if (selected.status == FactorRootSelectionStatus::selected)
                return make_canonical_real(budget, *factors.value, lower, upper, precision);
            if (selected.status == FactorRootSelectionStatus::error)
            {
                if (selected.error == Error::resource_limit_exceeded)
                    return {selected.error, selected.status, std::nullopt};
                // A broad interval can contain no candidate after outward
                // conversion only if a later precision is needed.
            }
        }
        catch (const std::exception&)
        {
            return {Error::resource_limit_exceeded, FactorRootSelectionStatus::error, std::nullopt};
        }
    }
    return {Error::resource_limit_exceeded, FactorRootSelectionStatus::error, std::nullopt};
}

BigInt integer_sqrt(const BigInt& value)
{
    if (value <= 0)
        return 0;
    BigInt estimate = BigInt(1) << ((boost::multiprecision::msb(value) + 2) / 2);
    while (true)
    {
        BigInt next = (estimate + value / estimate) / 2;
        if (next >= estimate)
            return estimate;
        estimate = std::move(next);
    }
}

BoundsResult binary_bounds(Budget& budget, const CanonicalReal& left, const CanonicalReal& right,
                           std::uint32_t precision, bool product)
{
    BoundsResult left_bounds = value_bounds(budget, left, precision);
    if (left_bounds.error != Error::none || !left_bounds.value)
        return left_bounds;
    BoundsResult right_bounds = value_bounds(budget, right, precision);
    if (right_bounds.error != Error::none || !right_bounds.value)
        return right_bounds;
    return {Error::none, false,
            product ? product_bounds(*left_bounds.value, *right_bounds.value)
                    : sum_bounds(*left_bounds.value, *right_bounds.value)};
}

} // namespace

CanonicalRealResult add_canonical_reals(Budget& budget, const CanonicalReal& left,
                                        const CanonicalReal& right)
{
    if (left.kind() == CanonicalRealKind::rational && right.kind() == CanonicalRealKind::rational)
        return rational_binary(budget, left, right, false);
    PolynomialResult candidate;
    if (left.kind() == CanonicalRealKind::irrational &&
        right.kind() == CanonicalRealKind::irrational)
        candidate = make_square_free_resultant(budget, *left.polynomial(), *right.polynomial(),
                                               ResultantOperation::sum);
    else
    {
        const CanonicalReal& irrational =
            left.kind() == CanonicalRealKind::irrational ? left : right;
        const CanonicalReal& rational = &irrational == &left ? right : left;
        candidate = make_translated_polynomial(budget, *irrational.polynomial(),
                                               rational.numerator(), rational.denominator());
    }
    if (candidate.error != Error::none || !candidate.value)
        return {candidate.error, FactorRootSelectionStatus::error, std::nullopt};
    return select_expression_value(
        budget, std::move(*candidate.value), [&](std::uint32_t precision)
        { return bounds_budget(left, &right, precision, 16); }, [&](std::uint32_t precision)
        { return binary_bounds(budget, left, right, precision, false); });
}

CanonicalRealResult multiply_canonical_reals(Budget& budget, const CanonicalReal& left,
                                             const CanonicalReal& right)
{
    if (left.kind() == CanonicalRealKind::rational && right.kind() == CanonicalRealKind::rational)
        return rational_binary(budget, left, right, true);
    const CanonicalReal* rational =
        left.kind() == CanonicalRealKind::rational
            ? &left
            : (right.kind() == CanonicalRealKind::rational ? &right : nullptr);
    if (rational != nullptr && rational->numerator() == 0)
        return make_canonical_rational(budget, 0, 1);
    PolynomialResult candidate;
    if (rational == nullptr)
        candidate = make_square_free_resultant(budget, *left.polynomial(), *right.polynomial(),
                                               ResultantOperation::product);
    else
    {
        const CanonicalReal& irrational = rational == &left ? right : left;
        candidate = make_scaled_polynomial(budget, *irrational.polynomial(), rational->numerator(),
                                           rational->denominator());
    }
    if (candidate.error != Error::none || !candidate.value)
        return {candidate.error, FactorRootSelectionStatus::error, std::nullopt};
    return select_expression_value(
        budget, std::move(*candidate.value), [&](std::uint32_t precision)
        { return bounds_budget(left, &right, precision, 24); }, [&](std::uint32_t precision)
        { return binary_bounds(budget, left, right, precision, true); });
}

CanonicalRealResult reciprocal_canonical_real(Budget& budget, const CanonicalReal& value)
{
    if (value.kind() == CanonicalRealKind::rational)
    {
        if (value.numerator() == 0)
            return {Error::invalid_argument, FactorRootSelectionStatus::error, std::nullopt};
        return make_canonical_rational(budget, value.denominator(), value.numerator());
    }
    PolynomialResult candidate = make_reciprocal_polynomial(budget, *value.polynomial());
    if (candidate.error != Error::none || !candidate.value)
        return {candidate.error, FactorRootSelectionStatus::error, std::nullopt};
    return select_expression_value(
        budget, std::move(*candidate.value),
        [&](std::uint32_t precision) { return bounds_budget(value, nullptr, precision, 16); },
        [&](std::uint32_t precision)
        {
            BoundsResult original = value_bounds(budget, value, precision);
            if (original.error != Error::none || !original.value)
                return original;
            if (!(original.value->upper.numerator < 0 || original.value->lower.numerator > 0))
                return BoundsResult{Error::none, true, std::nullopt};
            return BoundsResult{Error::none, false,
                                Bounds{normalized(original.value->upper.denominator,
                                                  original.value->upper.numerator),
                                       normalized(original.value->lower.denominator,
                                                  original.value->lower.numerator)}};
        });
}

CanonicalRealResult nonnegative_square_root_canonical_real(Budget& budget,
                                                           const CanonicalReal& value)
{
    ComparisonResult value_sign = sign_of_canonical_real(budget, value);
    if (value_sign.error != Error::none || !value_sign.ordering)
        return {value_sign.error == Error::none ? Error::invalid_argument : value_sign.error,
                FactorRootSelectionStatus::error, std::nullopt};
    if (*value_sign.ordering < 0)
        return {Error::invalid_argument, FactorRootSelectionStatus::error, std::nullopt};
    if (*value_sign.ordering == 0)
        return make_canonical_rational(budget, 0, 1);
    PolynomialResult candidate = value.kind() == CanonicalRealKind::irrational
                                     ? make_square_root_polynomial(budget, *value.polynomial())
                                     : rational_square_root_candidate(budget, value);
    if (candidate.error != Error::none || !candidate.value)
        return {candidate.error, FactorRootSelectionStatus::error, std::nullopt};
    return select_expression_value(
        budget, std::move(*candidate.value),
        [&](std::uint32_t precision) { return bounds_budget(value, nullptr, precision, 32); },
        [&](std::uint32_t precision)
        {
            BoundsResult original = value_bounds(budget, value, precision);
            if (original.error != Error::none || !original.value)
                return original;
            if (original.value->upper.numerator < 0)
                return BoundsResult{Error::invalid_argument, false, std::nullopt};
            FractionBound lower_bound = original.value->lower;
            if (lower_bound.numerator < 0)
                lower_bound = {0, 1};
            const BigInt scale = BigInt(1) << (2 * precision);
            const BigInt lower_scaled =
                floor_divide(lower_bound.numerator * scale, lower_bound.denominator);
            const BigInt upper_scaled = ceil_divide(original.value->upper.numerator * scale,
                                                    original.value->upper.denominator);
            BigInt lower = integer_sqrt(lower_scaled);
            BigInt upper = integer_sqrt(upper_scaled);
            if (upper * upper < upper_scaled)
                ++upper;
            const BigInt denominator = BigInt(1) << precision;
            return BoundsResult{
                Error::none, false,
                Bounds{{std::move(lower), denominator}, {std::move(upper), denominator}}};
        });
}

} // namespace geometer::exact

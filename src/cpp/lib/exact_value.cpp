#include "geometer/exact_value.h"

#include <algorithm>
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
        throw std::overflow_error("exact value budget estimate overflow");
    return left + right;
}

std::uint64_t checked_multiply(std::uint64_t left, std::uint64_t right)
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left)
        throw std::overflow_error("exact value budget estimate overflow");
    return left * right;
}

std::uint64_t limbs(const BigInt& value)
{
    return std::max<std::uint64_t>(1, static_cast<std::uint64_t>(value.backend().size()));
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

std::int8_t sign(const BigInt& value)
{
    return value == 0 ? 0 : (value > 0 ? 1 : -1);
}

ComparisonResult compare_irrational_to_rational(Budget& budget, const CanonicalReal& irrational,
                                                const CanonicalReal& rational)
{
    const RootIntervalCountResult count = count_real_roots_below_rational(
        budget, *irrational.polynomial(), rational.numerator(), rational.denominator());
    if (count.error != Error::none)
        return {count.error, std::nullopt};
    return {Error::none,
            static_cast<std::int8_t>(irrational.root()->ordinal < count.count ? -1 : 1)};
}

} // namespace

CanonicalReal::CanonicalReal(Budget& budget, std::uint64_t charged_bytes, BigInt numerator,
                             BigInt denominator)
    : budget_(&budget), charged_bytes_(charged_bytes), numerator_(std::move(numerator)),
      denominator_(std::move(denominator))
{
}

CanonicalReal::CanonicalReal(Polynomial polynomial, IsolatedRootSet roots, std::uint32_t ordinal)
    : kind_(CanonicalRealKind::irrational), polynomial_(std::move(polynomial)),
      roots_(std::move(roots)), ordinal_(ordinal)
{
}

CanonicalReal::~CanonicalReal()
{
    if (budget_ != nullptr)
        budget_->release_storage(charged_bytes_);
}

CanonicalReal::CanonicalReal(CanonicalReal&& other) noexcept
    : kind_(other.kind_), budget_(std::exchange(other.budget_, nullptr)),
      charged_bytes_(std::exchange(other.charged_bytes_, 0)),
      numerator_(std::move(other.numerator_)), denominator_(std::move(other.denominator_)),
      polynomial_(std::move(other.polynomial_)), roots_(std::move(other.roots_)),
      ordinal_(other.ordinal_)
{
}

CanonicalReal& CanonicalReal::operator=(CanonicalReal&& other) noexcept
{
    if (this != &other)
    {
        if (budget_ != nullptr)
            budget_->release_storage(charged_bytes_);
        kind_ = other.kind_;
        budget_ = std::exchange(other.budget_, nullptr);
        charged_bytes_ = std::exchange(other.charged_bytes_, 0);
        numerator_ = std::move(other.numerator_);
        denominator_ = std::move(other.denominator_);
        polynomial_ = std::move(other.polynomial_);
        roots_ = std::move(other.roots_);
        ordinal_ = other.ordinal_;
    }
    return *this;
}

CanonicalRealKind CanonicalReal::kind() const
{
    return kind_;
}
const BigInt& CanonicalReal::numerator() const
{
    return numerator_;
}
const BigInt& CanonicalReal::denominator() const
{
    return denominator_;
}
const Polynomial* CanonicalReal::polynomial() const
{
    return polynomial_ ? &*polynomial_ : nullptr;
}
const IsolatedRoot* CanonicalReal::root() const
{
    return roots_ ? &roots_->roots()[ordinal_] : nullptr;
}

CanonicalRealResult make_canonical_real(Budget& budget, const PolynomialFactorSet& factors,
                                        const BigInt& lower_k, const BigInt& upper_k,
                                        std::uint32_t precision)
{
    try
    {
        const FactorRootSelectionResult selected =
            select_unique_factor_root(budget, factors, lower_k, upper_k, precision);
        if (selected.status != FactorRootSelectionStatus::selected || !selected.factor_index)
            return {selected.error, selected.status, std::nullopt};
        const Polynomial& factor = factors.factors()[*selected.factor_index];
        if (factor.degree() == 1)
        {
            const std::uint64_t width =
                checked_add(limbs(factor.coefficients()[0]), limbs(factor.coefficients()[1]));
            const std::uint64_t storage = checked_add(512, checked_multiply(width, 64));
            const std::uint64_t work = checked_multiply(checked_multiply(width, width), width);
            StorageReservation reservation(budget, storage);
            if (!reservation.acquired() || !budget.consume_work(work))
                return {Error::resource_limit_exceeded, FactorRootSelectionStatus::error,
                        std::nullopt};
            BigInt numerator = -factor.coefficients()[0];
            BigInt denominator = factor.coefficients()[1];
            const BigInt divisor = gcd(numerator, denominator);
            numerator /= divisor;
            denominator /= divisor;
            if (denominator < 0)
            {
                numerator = -numerator;
                denominator = -denominator;
            }
            reservation.commit();
            return {Error::none, FactorRootSelectionStatus::selected,
                    CanonicalReal(budget, storage, std::move(numerator), std::move(denominator))};
        }
        auto polynomial = make_primitive_polynomial(budget, factor.coefficients());
        if (polynomial.error != Error::none || !polynomial.value)
            return {polynomial.error, FactorRootSelectionStatus::error, std::nullopt};
        auto roots = isolate_real_roots(budget, *polynomial.value);
        if (roots.error != Error::none || !roots.value ||
            selected.root_ordinal >= roots.value->roots().size())
            return {roots.error == Error::none ? Error::invalid_argument : roots.error,
                    FactorRootSelectionStatus::error, std::nullopt};
        return {Error::none, FactorRootSelectionStatus::selected,
                CanonicalReal(std::move(*polynomial.value), std::move(*roots.value),
                              selected.root_ordinal)};
    }
    catch (const std::exception&)
    {
        return {Error::resource_limit_exceeded, FactorRootSelectionStatus::error, std::nullopt};
    }
}

ComparisonResult compare_canonical_reals(Budget& budget, const CanonicalReal& left,
                                         const CanonicalReal& right)
{
    try
    {
        if (!budget.consume_exact_predicate())
            return {Error::resource_limit_exceeded, std::nullopt};
        if (left.kind() == CanonicalRealKind::rational &&
            right.kind() == CanonicalRealKind::rational)
        {
            const std::uint64_t width =
                checked_add(checked_add(limbs(left.numerator()), limbs(left.denominator())),
                            checked_add(limbs(right.numerator()), limbs(right.denominator())));
            const std::uint64_t storage = checked_add(256, checked_multiply(width, 64));
            const std::uint64_t work = checked_multiply(4, checked_multiply(width, width));
            StorageReservation reservation(budget, storage);
            if (!reservation.acquired() || !budget.consume_work(work))
                return {Error::resource_limit_exceeded, std::nullopt};
            return {Error::none, sign(left.numerator() * right.denominator() -
                                      right.numerator() * left.denominator())};
        }
        if (left.kind() == CanonicalRealKind::irrational &&
            right.kind() == CanonicalRealKind::rational)
            return compare_irrational_to_rational(budget, left, right);
        if (left.kind() == CanonicalRealKind::rational &&
            right.kind() == CanonicalRealKind::irrational)
        {
            ComparisonResult result = compare_irrational_to_rational(budget, right, left);
            if (result.ordering)
                *result.ordering = static_cast<std::int8_t>(-*result.ordering);
            return result;
        }
        if (left.polynomial()->coefficients() == right.polynomial()->coefficients())
            return {Error::none,
                    static_cast<std::int8_t>(left.root()->ordinal < right.root()->ordinal   ? -1
                                             : left.root()->ordinal > right.root()->ordinal ? 1
                                                                                            : 0)};
        const std::uint32_t initial = std::max(left.root()->precision, right.root()->precision);
        for (std::uint32_t precision = initial; precision <= 4096; ++precision)
        {
            auto left_interval =
                refine_real_root(budget, *left.polynomial(), left.root()->ordinal, precision);
            auto right_interval =
                refine_real_root(budget, *right.polynomial(), right.root()->ordinal, precision);
            if (left_interval.error != Error::none || right_interval.error != Error::none)
                return {left_interval.error != Error::none ? left_interval.error
                                                           : right_interval.error,
                        std::nullopt};
            if (left_interval.value->interval_k + 1 <= right_interval.value->interval_k)
                return {Error::none, -1};
            if (right_interval.value->interval_k + 1 <= left_interval.value->interval_k)
                return {Error::none, 1};
        }
        return {Error::resource_limit_exceeded, std::nullopt};
    }
    catch (const std::exception&)
    {
        return {Error::resource_limit_exceeded, std::nullopt};
    }
}

ComparisonResult sign_of_canonical_real(Budget& budget, const CanonicalReal& value)
{
    if (value.kind() == CanonicalRealKind::rational)
    {
        if (!budget.consume_exact_predicate())
            return {Error::resource_limit_exceeded, std::nullopt};
        return {Error::none, sign(value.numerator())};
    }
    const RootIntervalCountResult count =
        count_real_roots_below_rational(budget, *value.polynomial(), 0, 1);
    if (count.error != Error::none)
        return {count.error, std::nullopt};
    return {Error::none, static_cast<std::int8_t>(value.root()->ordinal < count.count ? -1 : 1)};
}

} // namespace geometer::exact

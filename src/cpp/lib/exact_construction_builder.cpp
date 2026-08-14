#include "geometer/exact_construction.h"

namespace geometer::exact
{

ConstructionArenaTransaction::ConstructionArenaTransaction(ConstructionArena& arena)
    : arena_(arena), checkpoint_(arena.nodes_.size())
{
}

ConstructionArenaTransaction::~ConstructionArenaTransaction()
{
    if (!committed_)
        arena_.rollback(checkpoint_);
}

void ConstructionArenaTransaction::commit()
{
    committed_ = true;
}

ConstructionBuilder::ConstructionBuilder(ConstructionArena& arena) : arena_(arena) {}

ConstructionNodeId ConstructionBuilder::rational(const BigInt& numerator, const BigInt& denominator)
{
    if (!good())
        return 0;
    return take(arena_.make_rational(numerator, denominator));
}

ConstructionNodeId ConstructionBuilder::sum(ConstructionNodeId left, ConstructionNodeId right)
{
    if (!good())
        return 0;
    return take(arena_.make_sum(left, right));
}

ConstructionNodeId ConstructionBuilder::product(ConstructionNodeId left, ConstructionNodeId right)
{
    if (!good())
        return 0;
    return take(arena_.make_product(left, right));
}

ConstructionNodeId ConstructionBuilder::negate(ConstructionNodeId value)
{
    const ConstructionNodeId negative_one = rational(-1);
    return product(negative_one, value);
}

ConstructionNodeId ConstructionBuilder::subtract(ConstructionNodeId left, ConstructionNodeId right)
{
    const ConstructionNodeId negative_right = negate(right);
    return sum(left, negative_right);
}

ConstructionNodeId ConstructionBuilder::square(ConstructionNodeId value)
{
    return product(value, value);
}

ConstructionNodeId ConstructionBuilder::divide(ConstructionNodeId numerator,
                                               ConstructionNodeId denominator)
{
    if (!good())
        return 0;
    const ConstructionNodeId reciprocal = take(arena_.make_reciprocal(denominator));
    return product(numerator, reciprocal);
}

ConstructionNodeId ConstructionBuilder::square_root(ConstructionNodeId value)
{
    if (!good())
        return 0;
    return take(arena_.make_nonnegative_square_root(value));
}

std::int8_t ConstructionBuilder::sign(ConstructionNodeId value)
{
    if (!good())
        return 0;
    ComparisonResult result = sign_of_canonical_real(arena_.budget(), arena_.at(value).value());
    if (result.error != Error::none || !result.ordering)
    {
        error_ = result.error == Error::none ? Error::invalid_argument : result.error;
        return 0;
    }
    return *result.ordering;
}

std::int8_t ConstructionBuilder::compare(ConstructionNodeId left, ConstructionNodeId right)
{
    if (!good())
        return 0;
    ComparisonResult result =
        compare_canonical_reals(arena_.budget(), arena_.at(left).value(), arena_.at(right).value());
    if (result.error != Error::none || !result.ordering)
    {
        error_ = result.error == Error::none ? Error::invalid_argument : result.error;
        return 0;
    }
    return *result.ordering;
}

bool ConstructionBuilder::good() const
{
    return error_ == Error::none;
}

Error ConstructionBuilder::error() const
{
    return error_;
}

ConstructionNodeId ConstructionBuilder::take(ConstructionResult result)
{
    if (!good())
        return 0;
    if (result.error != Error::none || !result.node)
    {
        error_ = result.error == Error::none ? Error::invalid_argument : result.error;
        return 0;
    }
    return *result.node;
}

} // namespace geometer::exact

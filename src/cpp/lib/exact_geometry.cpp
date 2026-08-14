#include "geometer/exact_geometry.h"

#include <algorithm>
#include <exception>
#include <utility>
#include <vector>

namespace geometer::exact
{

class ConstructionTransaction
{
  public:
    explicit ConstructionTransaction(ConstructionArena& arena)
        : arena_(arena), checkpoint_(arena.nodes_.size())
    {
    }

    ~ConstructionTransaction()
    {
        if (!committed_)
            arena_.rollback(checkpoint_);
    }

    void commit()
    {
        committed_ = true;
    }

  private:
    ConstructionArena& arena_;
    std::size_t checkpoint_ = 0;
    bool committed_ = false;
};

namespace
{

class Builder
{
  public:
    explicit Builder(ConstructionArena& arena) : arena_(arena) {}

    [[nodiscard]] ConstructionNodeId rational(const BigInt& numerator,
                                              const BigInt& denominator = 1)
    {
        if (!good())
            return 0;
        return take(arena_.make_rational(numerator, denominator));
    }

    [[nodiscard]] ConstructionNodeId sum(ConstructionNodeId left, ConstructionNodeId right)
    {
        if (!good())
            return 0;
        return take(arena_.make_sum(left, right));
    }

    [[nodiscard]] ConstructionNodeId product(ConstructionNodeId left, ConstructionNodeId right)
    {
        if (!good())
            return 0;
        return take(arena_.make_product(left, right));
    }

    [[nodiscard]] ConstructionNodeId negate(ConstructionNodeId value)
    {
        const ConstructionNodeId negative_one = rational(-1);
        return product(negative_one, value);
    }

    [[nodiscard]] ConstructionNodeId subtract(ConstructionNodeId left, ConstructionNodeId right)
    {
        const ConstructionNodeId negative_right = negate(right);
        return sum(left, negative_right);
    }

    [[nodiscard]] ConstructionNodeId square(ConstructionNodeId value)
    {
        return product(value, value);
    }

    [[nodiscard]] ConstructionNodeId divide(ConstructionNodeId numerator,
                                            ConstructionNodeId denominator)
    {
        if (!good())
            return 0;
        const ConstructionNodeId reciprocal = take(arena_.make_reciprocal(denominator));
        return product(numerator, reciprocal);
    }

    [[nodiscard]] ConstructionNodeId square_root(ConstructionNodeId value)
    {
        if (!good())
            return 0;
        return take(arena_.make_nonnegative_square_root(value));
    }

    [[nodiscard]] std::int8_t sign(ConstructionNodeId value)
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

    [[nodiscard]] std::int8_t compare(ConstructionNodeId left, ConstructionNodeId right)
    {
        if (!good())
            return 0;
        ComparisonResult result = compare_canonical_reals(arena_.budget(), arena_.at(left).value(),
                                                          arena_.at(right).value());
        if (result.error != Error::none || !result.ordering)
        {
            error_ = result.error == Error::none ? Error::invalid_argument : result.error;
            return 0;
        }
        return *result.ordering;
    }

    [[nodiscard]] bool good() const
    {
        return error_ == Error::none;
    }

    [[nodiscard]] Error error() const
    {
        return error_;
    }

  private:
    [[nodiscard]] ConstructionNodeId take(ConstructionResult result)
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

    ConstructionArena& arena_;
    Error error_ = Error::none;
};

bool valid_point(const ConstructionArena& arena, const ExactPoint& point)
{
    return static_cast<std::size_t>(point.x) < arena.size() &&
           static_cast<std::size_t>(point.y) < arena.size();
}

ExactIntersectionResult failure(Error error)
{
    return {error, IntersectionRelation::disjoint, 0, {}};
}

ExactIntersectionResult relation(IntersectionRelation value)
{
    return {Error::none, value, 0, {}};
}

bool order_points(Builder& builder, ExactPoint& first, ExactPoint& second)
{
    const std::int8_t x_order = builder.compare(first.x, second.x);
    if (!builder.good())
        return false;
    if (x_order > 0)
        std::swap(first, second);
    else if (x_order == 0)
    {
        const std::int8_t y_order = builder.compare(first.y, second.y);
        if (!builder.good())
            return false;
        if (y_order > 0)
            std::swap(first, second);
        else if (y_order == 0)
            return false;
    }
    return true;
}

ConstructionNodeId dot(Builder& builder, ConstructionNodeId ax, ConstructionNodeId ay,
                       ConstructionNodeId bx, ConstructionNodeId by)
{
    const ConstructionNodeId x_product = builder.product(ax, bx);
    const ConstructionNodeId y_product = builder.product(ay, by);
    return builder.sum(x_product, y_product);
}

ConstructionNodeId cross(Builder& builder, ConstructionNodeId ax, ConstructionNodeId ay,
                         ConstructionNodeId bx, ConstructionNodeId by)
{
    const ConstructionNodeId first = builder.product(ax, by);
    const ConstructionNodeId second = builder.product(ay, bx);
    return builder.subtract(first, second);
}

} // namespace

namespace
{

ExactIntersectionResult intersect_lines_impl(ConstructionArena& arena, const ExactLine& left,
                                             const ExactLine& right)
{
    if (!valid_point(arena, left.first) || !valid_point(arena, left.second) ||
        !valid_point(arena, right.first) || !valid_point(arena, right.second))
        return failure(Error::invalid_argument);
    ConstructionTransaction transaction(arena);
    Builder builder(arena);
    const ConstructionNodeId left_dx = builder.subtract(left.second.x, left.first.x);
    const ConstructionNodeId left_dy = builder.subtract(left.second.y, left.first.y);
    const ConstructionNodeId right_dx = builder.subtract(right.second.x, right.first.x);
    const ConstructionNodeId right_dy = builder.subtract(right.second.y, right.first.y);
    const ConstructionNodeId left_length = dot(builder, left_dx, left_dy, left_dx, left_dy);
    const ConstructionNodeId right_length = dot(builder, right_dx, right_dy, right_dx, right_dy);
    if (!builder.good())
        return failure(builder.error());
    if (builder.sign(left_length) == 0 || builder.sign(right_length) == 0)
        return builder.good() ? failure(Error::invalid_argument) : failure(builder.error());

    const ConstructionNodeId denominator = cross(builder, left_dx, left_dy, right_dx, right_dy);
    const ConstructionNodeId offset_x = builder.subtract(right.first.x, left.first.x);
    const ConstructionNodeId offset_y = builder.subtract(right.first.y, left.first.y);
    const std::int8_t denominator_sign = builder.sign(denominator);
    if (!builder.good())
        return failure(builder.error());
    if (denominator_sign == 0)
    {
        const ConstructionNodeId same_domain = cross(builder, offset_x, offset_y, left_dx, left_dy);
        const std::int8_t same_domain_sign = builder.sign(same_domain);
        if (!builder.good())
            return failure(builder.error());
        return relation(same_domain_sign == 0 ? IntersectionRelation::coincident
                                              : IntersectionRelation::disjoint);
    }

    const ConstructionNodeId parameter_numerator =
        cross(builder, offset_x, offset_y, right_dx, right_dy);
    const ConstructionNodeId parameter = builder.divide(parameter_numerator, denominator);
    const ConstructionNodeId scaled_x = builder.product(parameter, left_dx);
    const ConstructionNodeId scaled_y = builder.product(parameter, left_dy);
    const ConstructionNodeId point_x = builder.sum(left.first.x, scaled_x);
    const ConstructionNodeId point_y = builder.sum(left.first.y, scaled_y);
    ExactPoint point{point_x, point_y};
    if (!builder.good())
        return failure(builder.error());
    transaction.commit();
    return {Error::none, IntersectionRelation::point, 1, {point, {}}};
}

ExactIntersectionResult intersect_line_circle_impl(ConstructionArena& arena, const ExactLine& line,
                                                   const ExactCircle& circle)
{
    if (!valid_point(arena, line.first) || !valid_point(arena, line.second) ||
        !valid_point(arena, circle.center) ||
        static_cast<std::size_t>(circle.radius) >= arena.size())
        return failure(Error::invalid_argument);
    ConstructionTransaction transaction(arena);
    Builder builder(arena);
    const ConstructionNodeId dx = builder.subtract(line.second.x, line.first.x);
    const ConstructionNodeId dy = builder.subtract(line.second.y, line.first.y);
    const ConstructionNodeId fx = builder.subtract(line.first.x, circle.center.x);
    const ConstructionNodeId fy = builder.subtract(line.first.y, circle.center.y);
    const ConstructionNodeId a = dot(builder, dx, dy, dx, dy);
    if (!builder.good())
        return failure(builder.error());
    if (builder.sign(a) == 0)
        return builder.good() ? failure(Error::invalid_argument) : failure(builder.error());
    if (builder.sign(circle.radius) <= 0)
        return builder.good() ? failure(Error::invalid_argument) : failure(builder.error());

    const ConstructionNodeId two = builder.rational(2);
    const ConstructionNodeId four = builder.rational(4);
    const ConstructionNodeId direction_projection = dot(builder, fx, fy, dx, dy);
    const ConstructionNodeId b = builder.product(two, direction_projection);
    const ConstructionNodeId offset_length = dot(builder, fx, fy, fx, fy);
    const ConstructionNodeId radius_squared = builder.square(circle.radius);
    const ConstructionNodeId c = builder.subtract(offset_length, radius_squared);
    const ConstructionNodeId b_squared = builder.square(b);
    const ConstructionNodeId ac = builder.product(a, c);
    const ConstructionNodeId four_ac = builder.product(four, ac);
    const ConstructionNodeId discriminant = builder.subtract(b_squared, four_ac);
    const std::int8_t discriminant_sign = builder.sign(discriminant);
    if (!builder.good())
        return failure(builder.error());
    if (discriminant_sign < 0)
        return relation(IntersectionRelation::disjoint);

    const ConstructionNodeId root = builder.square_root(discriminant);
    const ConstructionNodeId denominator = builder.product(two, a);
    const ConstructionNodeId negative_b = builder.negate(b);
    const ConstructionNodeId first_numerator = builder.subtract(negative_b, root);
    const ConstructionNodeId first_parameter = builder.divide(first_numerator, denominator);
    const ConstructionNodeId first_scaled_x = builder.product(first_parameter, dx);
    const ConstructionNodeId first_scaled_y = builder.product(first_parameter, dy);
    const ConstructionNodeId first_x = builder.sum(line.first.x, first_scaled_x);
    const ConstructionNodeId first_y = builder.sum(line.first.y, first_scaled_y);
    ExactPoint first{first_x, first_y};
    if (!builder.good())
        return failure(builder.error());
    if (discriminant_sign == 0)
    {
        transaction.commit();
        return {Error::none, IntersectionRelation::point, 1, {first, {}}};
    }
    const ConstructionNodeId second_numerator = builder.sum(negative_b, root);
    const ConstructionNodeId second_parameter = builder.divide(second_numerator, denominator);
    const ConstructionNodeId second_scaled_x = builder.product(second_parameter, dx);
    const ConstructionNodeId second_scaled_y = builder.product(second_parameter, dy);
    const ConstructionNodeId second_x = builder.sum(line.first.x, second_scaled_x);
    const ConstructionNodeId second_y = builder.sum(line.first.y, second_scaled_y);
    ExactPoint second{second_x, second_y};
    if (!builder.good() || !order_points(builder, first, second))
        return failure(builder.good() ? Error::invalid_argument : builder.error());
    transaction.commit();
    return {Error::none, IntersectionRelation::two_points, 2, {first, second}};
}

ExactIntersectionResult intersect_circles_impl(ConstructionArena& arena, const ExactCircle& left,
                                               const ExactCircle& right)
{
    if (!valid_point(arena, left.center) || !valid_point(arena, right.center) ||
        static_cast<std::size_t>(left.radius) >= arena.size() ||
        static_cast<std::size_t>(right.radius) >= arena.size())
        return failure(Error::invalid_argument);
    ConstructionTransaction transaction(arena);
    Builder builder(arena);
    if (builder.sign(left.radius) <= 0 || builder.sign(right.radius) <= 0)
        return builder.good() ? failure(Error::invalid_argument) : failure(builder.error());
    const ConstructionNodeId dx = builder.subtract(right.center.x, left.center.x);
    const ConstructionNodeId dy = builder.subtract(right.center.y, left.center.y);
    const ConstructionNodeId distance_squared = dot(builder, dx, dy, dx, dy);
    const std::int8_t distance_sign = builder.sign(distance_squared);
    if (!builder.good())
        return failure(builder.error());
    if (distance_sign == 0)
    {
        const std::int8_t radius_order = builder.compare(left.radius, right.radius);
        if (!builder.good())
            return failure(builder.error());
        return relation(radius_order == 0 ? IntersectionRelation::coincident
                                          : IntersectionRelation::disjoint);
    }

    const ConstructionNodeId two = builder.rational(2);
    const ConstructionNodeId four = builder.rational(4);
    const ConstructionNodeId left_radius_squared = builder.square(left.radius);
    const ConstructionNodeId right_radius_squared = builder.square(right.radius);
    const ConstructionNodeId radius_difference =
        builder.subtract(left_radius_squared, right_radius_squared);
    const ConstructionNodeId numerator = builder.sum(radius_difference, distance_squared);
    const ConstructionNodeId radius_distance =
        builder.product(left_radius_squared, distance_squared);
    const ConstructionNodeId four_radius_distance = builder.product(four, radius_distance);
    const ConstructionNodeId numerator_squared = builder.square(numerator);
    const ConstructionNodeId discriminant =
        builder.subtract(four_radius_distance, numerator_squared);
    const std::int8_t discriminant_sign = builder.sign(discriminant);
    if (!builder.good())
        return failure(builder.error());
    if (discriminant_sign < 0)
        return relation(IntersectionRelation::disjoint);

    const ConstructionNodeId denominator = builder.product(two, distance_squared);
    const ConstructionNodeId along = builder.divide(numerator, denominator);
    const ConstructionNodeId discriminant_root = builder.square_root(discriminant);
    const ConstructionNodeId perpendicular = builder.divide(discriminant_root, denominator);
    const ConstructionNodeId along_x = builder.product(along, dx);
    const ConstructionNodeId along_y = builder.product(along, dy);
    const ConstructionNodeId base_x = builder.sum(left.center.x, along_x);
    const ConstructionNodeId base_y = builder.sum(left.center.y, along_y);
    const ConstructionNodeId perpendicular_y = builder.product(perpendicular, dy);
    const ConstructionNodeId offset_x = builder.negate(perpendicular_y);
    const ConstructionNodeId offset_y = builder.product(perpendicular, dx);
    const ConstructionNodeId first_x = builder.sum(base_x, offset_x);
    const ConstructionNodeId first_y = builder.sum(base_y, offset_y);
    ExactPoint first{first_x, first_y};
    if (!builder.good())
        return failure(builder.error());
    if (discriminant_sign == 0)
    {
        transaction.commit();
        return {Error::none, IntersectionRelation::point, 1, {first, {}}};
    }
    const ConstructionNodeId second_x = builder.subtract(base_x, offset_x);
    const ConstructionNodeId second_y = builder.subtract(base_y, offset_y);
    ExactPoint second{second_x, second_y};
    if (!builder.good() || !order_points(builder, first, second))
        return failure(builder.good() ? Error::invalid_argument : builder.error());
    transaction.commit();
    return {Error::none, IntersectionRelation::two_points, 2, {first, second}};
}

} // namespace

ExactIntersectionResult intersect_exact_lines(ConstructionArena& arena, const ExactLine& left,
                                              const ExactLine& right)
{
    try
    {
        return intersect_lines_impl(arena, left, right);
    }
    catch (const std::exception&)
    {
        return failure(Error::resource_limit_exceeded);
    }
}

ExactIntersectionResult intersect_exact_line_circle(ConstructionArena& arena, const ExactLine& line,
                                                    const ExactCircle& circle)
{
    try
    {
        return intersect_line_circle_impl(arena, line, circle);
    }
    catch (const std::exception&)
    {
        return failure(Error::resource_limit_exceeded);
    }
}

ExactIntersectionResult intersect_exact_circles(ConstructionArena& arena, const ExactCircle& left,
                                                const ExactCircle& right)
{
    try
    {
        return intersect_circles_impl(arena, left, right);
    }
    catch (const std::exception&)
    {
        return failure(Error::resource_limit_exceeded);
    }
}

} // namespace geometer::exact

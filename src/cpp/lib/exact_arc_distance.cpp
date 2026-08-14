#include "geometer/exact_arc_distance.h"

#include <exception>
#include <optional>
#include <vector>

namespace geometer::exact
{
namespace
{

class StorageReservation
{
  public:
    StorageReservation(Budget& budget, std::uint64_t bytes) : budget_(budget), bytes_(bytes)
    {
        acquired_ = budget_.acquire_storage(bytes_);
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
    std::uint64_t bytes_ = 0;
    bool acquired_ = false;
};

ExactPredicateResult failure(Error error)
{
    return {error, std::nullopt};
}

bool points_equal(ConstructionBuilder& builder, const ExactPoint& left, const ExactPoint& right)
{
    const std::int8_t x = builder.compare(left.x, right.x);
    const std::int8_t y = builder.compare(left.y, right.y);
    return builder.good() && x == 0 && y == 0;
}

bool arcs_share_nodes(const ExactCircularArc& left, const ExactCircularArc& right)
{
    return left.circle.center.x == right.circle.center.x &&
           left.circle.center.y == right.circle.center.y &&
           left.circle.radius == right.circle.radius && left.start.x == right.start.x &&
           left.start.y == right.start.y && left.end.x == right.end.x &&
           left.end.y == right.end.y && left.counterclockwise == right.counterclockwise &&
           left.major_arc == right.major_arc;
}

std::optional<ExactPredicateResult> translated_arcs_within(ConstructionArena& arena,
                                                           const ExactCircularArc& left,
                                                           const ExactCircularArc& right,
                                                           ConstructionNodeId threshold_squared)
{
    if (left.counterclockwise != right.counterclockwise || left.major_arc != right.major_arc)
        return std::nullopt;
    ConstructionArenaTransaction transaction(arena);
    ConstructionBuilder builder(arena);
    if (builder.compare(left.circle.radius, right.circle.radius) != 0)
        return builder.good() ? std::nullopt
                              : std::optional<ExactPredicateResult>{failure(builder.error())};
    const ConstructionNodeId dx = builder.subtract(left.circle.center.x, right.circle.center.x);
    const ConstructionNodeId dy = builder.subtract(left.circle.center.y, right.circle.center.y);
    const ConstructionNodeId start_dx = builder.subtract(left.start.x, right.start.x);
    const ConstructionNodeId start_dy = builder.subtract(left.start.y, right.start.y);
    const ConstructionNodeId end_dx = builder.subtract(left.end.x, right.end.x);
    const ConstructionNodeId end_dy = builder.subtract(left.end.y, right.end.y);
    const bool same_translation =
        builder.compare(dx, start_dx) == 0 && builder.compare(dy, start_dy) == 0 &&
        builder.compare(dx, end_dx) == 0 && builder.compare(dy, end_dy) == 0;
    if (!builder.good())
        return failure(builder.error());
    if (!same_translation)
        return std::nullopt;
    // Translation supplies a pointwise upper bound. A support extremum in the translation
    // direction supplies the matching lower bound for these nonempty compact arcs.
    const ConstructionNodeId dx_squared = builder.square(dx);
    const ConstructionNodeId dy_squared = builder.square(dy);
    const ConstructionNodeId translation_squared = builder.sum(dx_squared, dy_squared);
    const std::int8_t order = builder.compare(translation_squared, threshold_squared);
    return builder.good() ? ExactPredicateResult{Error::none, order <= 0}
                          : failure(builder.error());
}

ConstructionNodeId squared_distance(ConstructionBuilder& builder, const ExactPoint& left,
                                    const ExactPoint& right)
{
    const ConstructionNodeId dx = builder.subtract(left.x, right.x);
    const ConstructionNodeId dy = builder.subtract(left.y, right.y);
    const ConstructionNodeId dx_squared = builder.square(dx);
    const ConstructionNodeId dy_squared = builder.square(dy);
    return builder.sum(dx_squared, dy_squared);
}

Error append_line_candidates(ConstructionArena& arena, const ExactCircularArc& source,
                             const ExactPoint& first, const ExactPoint& second,
                             std::vector<ExactPoint>& candidates)
{
    ConstructionBuilder equality(arena);
    if (points_equal(equality, first, second))
        return equality.good() ? Error::none : equality.error();
    ExactIntersectionResult intersections =
        intersect_exact_line_circle(arena, {first, second}, source.circle);
    if (intersections.error != Error::none)
        return intersections.error;
    for (std::uint32_t index = 0; index < intersections.point_count; ++index)
    {
        ExactPredicateResult contained =
            point_on_closed_exact_arc(arena, source, intersections.points[index]);
        if (contained.error != Error::none || !contained.value)
            return contained.error != Error::none ? contained.error : Error::invalid_argument;
        if (*contained.value)
            candidates.push_back(intersections.points[index]);
    }
    return Error::none;
}

Error append_bisector_candidates(ConstructionArena& arena, const ExactCircularArc& source,
                                 const ExactCircularArc& target,
                                 std::vector<ExactPoint>& candidates)
{
    ConstructionBuilder builder(arena);
    const ConstructionNodeId two = builder.rational(2);
    const ConstructionNodeId x_sum = builder.sum(target.start.x, target.end.x);
    const ConstructionNodeId y_sum = builder.sum(target.start.y, target.end.y);
    const ConstructionNodeId midpoint_x = builder.divide(x_sum, two);
    const ConstructionNodeId midpoint_y = builder.divide(y_sum, two);
    const ExactPoint midpoint{midpoint_x, midpoint_y};
    const ConstructionNodeId dx = builder.subtract(target.end.x, target.start.x);
    const ConstructionNodeId dy = builder.subtract(target.end.y, target.start.y);
    const ConstructionNodeId negative_dy = builder.negate(dy);
    const ConstructionNodeId second_x = builder.sum(midpoint.x, negative_dy);
    const ConstructionNodeId second_y = builder.sum(midpoint.y, dx);
    const ExactPoint second{second_x, second_y};
    if (!builder.good())
        return builder.error();
    return append_line_candidates(arena, source, midpoint, second, candidates);
}

struct DistanceResult
{
    Error error = Error::none;
    ConstructionNodeId squared = 0;
};

DistanceResult squared_distance_to_arc(ConstructionArena& arena, const ExactPoint& point,
                                       const ExactCircularArc& target)
{
    ConstructionBuilder builder(arena);
    const ConstructionNodeId center_distance_squared =
        squared_distance(builder, point, target.circle.center);
    const std::int8_t center_sign = builder.sign(center_distance_squared);
    if (!builder.good())
        return {builder.error(), 0};
    if (center_sign == 0)
    {
        const ConstructionNodeId radius_squared = builder.square(target.circle.radius);
        return builder.good() ? DistanceResult{Error::none, radius_squared}
                              : DistanceResult{builder.error(), 0};
    }

    const ConstructionNodeId center_distance = builder.square_root(center_distance_squared);
    const ConstructionNodeId scale = builder.divide(target.circle.radius, center_distance);
    const ConstructionNodeId dx = builder.subtract(point.x, target.circle.center.x);
    const ConstructionNodeId dy = builder.subtract(point.y, target.circle.center.y);
    const ConstructionNodeId scaled_dx = builder.product(dx, scale);
    const ConstructionNodeId scaled_dy = builder.product(dy, scale);
    const ConstructionNodeId radial_x = builder.sum(target.circle.center.x, scaled_dx);
    const ConstructionNodeId radial_y = builder.sum(target.circle.center.y, scaled_dy);
    const ExactPoint radial{radial_x, radial_y};
    if (!builder.good())
        return {builder.error(), 0};
    ExactPredicateResult radial_inside = point_on_closed_exact_arc(arena, target, radial);
    if (radial_inside.error != Error::none || !radial_inside.value)
        return {radial_inside.error != Error::none ? radial_inside.error : Error::invalid_argument,
                0};
    if (*radial_inside.value)
    {
        const ConstructionNodeId radial_delta =
            builder.subtract(center_distance, target.circle.radius);
        const ConstructionNodeId radial_distance_squared = builder.square(radial_delta);
        return builder.good() ? DistanceResult{Error::none, radial_distance_squared}
                              : DistanceResult{builder.error(), 0};
    }
    const ConstructionNodeId start_distance = squared_distance(builder, point, target.start);
    const ConstructionNodeId end_distance = squared_distance(builder, point, target.end);
    const std::int8_t order = builder.compare(start_distance, end_distance);
    return builder.good() ? DistanceResult{Error::none, order <= 0 ? start_distance : end_distance}
                          : DistanceResult{builder.error(), 0};
}

Error append_directed_candidates(ConstructionArena& arena, const ExactCircularArc& source,
                                 const ExactCircularArc& target,
                                 std::vector<ExactPoint>& candidates)
{
    // On each closest-feature interval, distance is either radial circle distance or distance to
    // one endpoint. Its extrema occur on the center/center or center/endpoint lines. Feature
    // transitions occur on target endpoint rays and the endpoint perpendicular bisector.
    candidates.push_back(source.start);
    candidates.push_back(source.end);
    const std::pair<ExactPoint, ExactPoint> lines[]{
        {source.circle.center, target.circle.center}, {source.circle.center, target.start},
        {source.circle.center, target.end},           {target.circle.center, target.start},
        {target.circle.center, target.end},
    };
    for (const auto& [first, second] : lines)
        if (const Error error = append_line_candidates(arena, source, first, second, candidates);
            error != Error::none)
            return error;
    return append_bisector_candidates(arena, source, target, candidates);
}

ExactPredicateResult directed_within(ConstructionArena& arena, const ExactCircularArc& source,
                                     const ExactCircularArc& target,
                                     ConstructionNodeId threshold_squared)
{
    std::vector<ExactPoint> candidates;
    candidates.reserve(16);
    if (const Error error = append_directed_candidates(arena, source, target, candidates);
        error != Error::none)
        return failure(error);
    ConstructionBuilder comparison(arena);
    for (const ExactPoint& candidate : candidates)
    {
        DistanceResult distance = squared_distance_to_arc(arena, candidate, target);
        if (distance.error != Error::none)
            return failure(distance.error);
        if (comparison.compare(distance.squared, threshold_squared) > 0)
            return comparison.good() ? ExactPredicateResult{Error::none, false}
                                     : failure(comparison.error());
    }
    return comparison.good() ? ExactPredicateResult{Error::none, true}
                             : failure(comparison.error());
}

} // namespace

ExactPredicateResult exact_arc_hausdorff_within(ConstructionArena& arena,
                                                const ExactCircularArc& left,
                                                const ExactCircularArc& right,
                                                const BigInt& threshold_numerator,
                                                const BigInt& threshold_denominator)
{
    try
    {
        if (threshold_numerator < 0 || threshold_denominator <= 0)
            return failure(Error::invalid_argument);
        if (!arena.budget().consume_work(16'384))
            return failure(Error::resource_limit_exceeded);
        StorageReservation reservation(arena.budget(), 16'384);
        if (!reservation.acquired())
            return failure(Error::resource_limit_exceeded);
        ExactPredicateResult left_valid = point_on_closed_exact_arc(arena, left, left.start);
        ExactPredicateResult right_valid = point_on_closed_exact_arc(arena, right, right.start);
        if (left_valid.error != Error::none || right_valid.error != Error::none ||
            !left_valid.value || !right_valid.value || !*left_valid.value || !*right_valid.value)
            return failure(left_valid.error != Error::none
                               ? left_valid.error
                               : (right_valid.error != Error::none ? right_valid.error
                                                                   : Error::invalid_argument));
        if (arcs_share_nodes(left, right))
            return {Error::none, true};
        ConstructionArenaTransaction transaction(arena);
        ConstructionBuilder builder(arena);
        const ConstructionNodeId threshold =
            builder.rational(threshold_numerator, threshold_denominator);
        const ConstructionNodeId threshold_squared = builder.square(threshold);
        if (!builder.good())
            return failure(builder.error());
        if (auto translated = translated_arcs_within(arena, left, right, threshold_squared))
            return *translated;
        ExactPredicateResult forward = directed_within(arena, left, right, threshold_squared);
        if (forward.error != Error::none || !forward.value || !*forward.value)
            return forward;
        return directed_within(arena, right, left, threshold_squared);
    }
    catch (const std::exception&)
    {
        return failure(Error::resource_limit_exceeded);
    }
}

} // namespace geometer::exact

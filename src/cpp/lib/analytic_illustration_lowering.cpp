#include "analytic_illustration_lowering.h"

#include "geometer/planar_triangulate.h"
#include "model_illustration_transform.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace geometer::model_illustration_detail
{
namespace
{
constexpr double pi = 3.141592653589793238462643383279502884;

struct Limits
{
    std::size_t definitions = 4096;
    std::size_t occurrences = 65536;
    std::size_t primitives = 65536;
    std::size_t rings = 262144;
    std::size_t points = 2000000;
    std::size_t samples = 2000000;
    std::size_t definition_triangles = 750000;
    std::size_t expanded_triangles = 750000;
    std::size_t topology_candidate_pairs = 10000000;
};

struct Sampling
{
    double linear = 0.1;
    double angular = 0.5;
    Limits limits;
    std::size_t generated_samples = 0;
    std::size_t definition_triangles = 0;
    std::size_t topology_remaining = 0;
};

struct Point
{
    double x = 0;
    double y = 0;
};

using Ring = std::vector<Point>;

double coordinate_scale(const Ring& ring)
{
    double scale = 1;
    for (const auto& point : ring)
        scale = std::max({scale, std::abs(point.x), std::abs(point.y)});
    return scale;
}

double cross(Point a, Point b, Point c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

bool point_on_segment(Point point, Point a, Point b, double cross_epsilon,
                      double coordinate_epsilon)
{
    return std::abs(cross(a, b, point)) <= cross_epsilon &&
           point.x >= std::min(a.x, b.x) - coordinate_epsilon &&
           point.x <= std::max(a.x, b.x) + coordinate_epsilon &&
           point.y >= std::min(a.y, b.y) - coordinate_epsilon &&
           point.y <= std::max(a.y, b.y) + coordinate_epsilon;
}

bool segments_intersect(Point a, Point b, Point c, Point d, double cross_epsilon,
                        double coordinate_epsilon)
{
    const double ab_c = cross(a, b, c), ab_d = cross(a, b, d);
    const double cd_a = cross(c, d, a), cd_b = cross(c, d, b);
    if (((ab_c > cross_epsilon && ab_d < -cross_epsilon) ||
         (ab_c < -cross_epsilon && ab_d > cross_epsilon)) &&
        ((cd_a > cross_epsilon && cd_b < -cross_epsilon) ||
         (cd_a < -cross_epsilon && cd_b > cross_epsilon)))
        return true;
    return point_on_segment(c, a, b, cross_epsilon, coordinate_epsilon) ||
           point_on_segment(d, a, b, cross_epsilon, coordinate_epsilon) ||
           point_on_segment(a, c, d, cross_epsilon, coordinate_epsilon) ||
           point_on_segment(b, c, d, cross_epsilon, coordinate_epsilon);
}

void consume_topology_pair(Sampling* sampling)
{
    if (sampling->topology_remaining == 0)
        throw std::length_error("Analytic illustration exceeds the topology candidate-pair limit.");
    --sampling->topology_remaining;
}

bool boundaries_intersect(const Ring& a, const Ring& b, Sampling* sampling)
{
    const double scale = std::max(coordinate_scale(a), coordinate_scale(b));
    const double coordinate_epsilon = 64 * std::numeric_limits<double>::epsilon() * scale;
    const double cross_epsilon = coordinate_epsilon * scale;
    for (std::size_t i = 0; i < a.size(); ++i)
        for (std::size_t j = 0; j < b.size(); ++j)
        {
            consume_topology_pair(sampling);
            if (segments_intersect(a[i], a[(i + 1) % a.size()], b[j], b[(j + 1) % b.size()],
                                   cross_epsilon, coordinate_epsilon))
                return true;
        }
    return false;
}

bool self_intersects(const Ring& ring, Sampling* sampling)
{
    const double scale = coordinate_scale(ring);
    const double coordinate_epsilon = 64 * std::numeric_limits<double>::epsilon() * scale;
    const double cross_epsilon = coordinate_epsilon * scale;
    for (std::size_t i = 0; i < ring.size(); ++i)
        for (std::size_t j = i + 1; j < ring.size(); ++j)
        {
            if (j == i + 1 || (i == 0 && j + 1 == ring.size()))
                continue;
            consume_topology_pair(sampling);
            if (segments_intersect(ring[i], ring[(i + 1) % ring.size()], ring[j],
                                   ring[(j + 1) % ring.size()], cross_epsilon, coordinate_epsilon))
                return true;
        }
    return false;
}

enum class PointLocation
{
    outside,
    inside,
    boundary,
};

PointLocation locate(Point point, const Ring& ring)
{
    const double scale =
        std::max(coordinate_scale(ring), std::max({1.0, std::abs(point.x), std::abs(point.y)}));
    const double coordinate_epsilon = 64 * std::numeric_limits<double>::epsilon() * scale;
    const double cross_epsilon = coordinate_epsilon * scale;
    bool inside = false;
    for (std::size_t i = 0, j = ring.size() - 1; i < ring.size(); j = i++)
    {
        const auto& a = ring[j];
        const auto& b = ring[i];
        if (point_on_segment(point, a, b, cross_epsilon, coordinate_epsilon))
            return PointLocation::boundary;
        if ((a.y > point.y) != (b.y > point.y) &&
            point.x < (b.x - a.x) * (point.y - a.y) / (b.y - a.y) + a.x)
            inside = !inside;
    }
    return inside ? PointLocation::inside : PointLocation::outside;
}

bool in_filled_region(Point point, const std::vector<Ring>& rings)
{
    if (locate(point, rings.front()) != PointLocation::inside)
        return false;
    for (std::size_t index = 1; index < rings.size(); ++index)
        if (locate(point, rings[index]) != PointLocation::outside)
            return false;
    return true;
}

void validate_region(const std::vector<Ring>& rings, Sampling* sampling)
{
    for (const auto& ring : rings)
        if (self_intersects(ring, sampling))
            throw std::runtime_error("Analytic profile rings must not self-intersect or touch.");
    for (std::size_t hole = 1; hole < rings.size(); ++hole)
    {
        if (boundaries_intersect(rings.front(), rings[hole], sampling) ||
            locate(rings[hole].front(), rings.front()) != PointLocation::inside)
            throw std::runtime_error("Analytic profile holes must be strictly contained without "
                                     "touching the outer ring.");
        for (std::size_t other = 1; other < hole; ++other)
            if (boundaries_intersect(rings[hole], rings[other], sampling) ||
                locate(rings[hole].front(), rings[other]) == PointLocation::inside ||
                locate(rings[other].front(), rings[hole]) == PointLocation::inside)
                throw std::runtime_error(
                    "Analytic profile holes must have disjoint, non-nested interiors.");
    }
}

void validate_disjoint_regions(const std::vector<std::vector<Ring>>& regions, Sampling* sampling)
{
    for (std::size_t a = 0; a < regions.size(); ++a)
        for (std::size_t b = a + 1; b < regions.size(); ++b)
        {
            for (const auto& ring_a : regions[a])
                for (const auto& ring_b : regions[b])
                    if (boundaries_intersect(ring_a, ring_b, sampling))
                        throw std::runtime_error("Analytic extrusion regions must have disjoint "
                                                 "interiors and boundaries.");
            if (in_filled_region(regions[a].front().front(), regions[b]) ||
                in_filled_region(regions[b].front().front(), regions[a]))
                throw std::runtime_error(
                    "Analytic extrusion regions must have disjoint interiors and boundaries.");
        }
}

void check_add(std::size_t amount, std::size_t limit, std::size_t* value, const char* label)
{
    if (amount > limit - *value)
        throw std::length_error(std::string("Analytic illustration exceeds the ") + label +
                                " limit.");
    *value += amount;
}

double signed_area(const Ring& ring)
{
    double area = 0;
    for (std::size_t index = 0; index < ring.size(); ++index)
    {
        const auto& a = ring[index];
        const auto& b = ring[(index + 1) % ring.size()];
        area += a.x * b.y - b.x * a.y;
    }
    return area * 0.5;
}

unsigned circle_segments(double radius, const Sampling& sampling)
{
    const double ratio = std::clamp(1 - sampling.linear / radius, -1.0, 1.0);
    const double chord = 2 * std::acos(ratio);
    const double theta = chord > 0 ? std::min(sampling.angular, chord) : 0;
    if (!(theta > 0) || !std::isfinite(theta))
        return 4096;
    return static_cast<unsigned>(std::clamp(std::ceil(2 * pi / theta), 12.0, 4096.0));
}

Ring expand_ring(const contracts::IllustrationProfileRingA0& source, Sampling* sampling)
{
    if (source.points_mm.size() != source.segments.size())
        throw std::runtime_error("Analytic ring point and segment counts must match.");
    Ring result;
    for (std::size_t index = 0; index < source.points_mm.size(); ++index)
    {
        const auto& authored = source.points_mm[index];
        const auto& next = source.points_mm[(index + 1) % source.points_mm.size()];
        const Point start{authored[0], authored[1]};
        const Point end{next[0], next[1]};
        if (start.x == end.x && start.y == end.y)
            throw std::runtime_error("Analytic rings cannot contain zero-length segments.");
        result.push_back(start);
        const auto* arc =
            std::get_if<contracts::IllustrationProfileCircularArcA0>(&source.segments[index]);
        if (!arc)
            continue;
        const Point center{arc->center_mm[0], arc->center_mm[1]};
        const double radius = std::hypot(start.x - center.x, start.y - center.y);
        const double end_radius = std::hypot(end.x - center.x, end.y - center.y);
        const double tolerance = std::max(1e-9, radius * 1e-9);
        if (!(radius > 0) || !std::isfinite(radius) || std::abs(radius - end_radius) > tolerance)
            throw std::runtime_error(
                "Analytic circular arc endpoints must share one finite radius.");
        const double start_angle = std::atan2(start.y - center.y, start.x - center.x);
        const double end_angle = std::atan2(end.y - center.y, end.x - center.x);
        double sweep = end_angle - start_angle;
        if (arc->sweep == contracts::PlanarArcSweep::ccw)
        {
            while (sweep <= 0)
                sweep += 2 * pi;
        }
        else
        {
            while (sweep >= 0)
                sweep -= 2 * pi;
        }
        const unsigned full = circle_segments(radius, *sampling);
        const unsigned segments =
            std::max(1U, static_cast<unsigned>(std::ceil(std::abs(sweep) / (2 * pi) * full)));
        check_add(segments - 1, sampling->limits.samples, &sampling->generated_samples,
                  "generated curve sample");
        for (unsigned step = 1; step < segments; ++step)
        {
            const double angle = start_angle + sweep * static_cast<double>(step) / segments;
            result.push_back(
                {center.x + radius * std::cos(angle), center.y + radius * std::sin(angle)});
        }
    }
    if (result.size() < 3 || !std::isfinite(signed_area(result)) || signed_area(result) == 0)
        throw std::runtime_error("Analytic ring must have finite nonzero area.");
    return result;
}

contracts::MeshIllustrationMesh mesh(std::string id,
                                     const contracts::MeshIllustrationMaterial& material)
{
    contracts::MeshIllustrationMesh result;
    result.id = std::move(id);
    result.materials.push_back(material);
    result.indices.emplace();
    return result;
}

void position(contracts::MeshIllustrationMesh* target, Point point, double z)
{
    target->positions.insert(target->positions.end(), {point.x, point.y, z});
}

void triangle(contracts::MeshIllustrationMesh* target, std::uint32_t a, std::uint32_t b,
              std::uint32_t c, Sampling*)
{
    target->indices->insert(target->indices->end(), {a, b, c});
}

void append_extrusion(const std::string& id, const std::vector<Ring>& rings, double z_min,
                      double z_max, const contracts::MeshIllustrationMaterial& material,
                      std::vector<contracts::MeshIllustrationMesh>* output, Sampling* sampling,
                      bool triangles_admitted = false)
{
    std::size_t vertices = 0;
    for (const auto& ring : rings)
        vertices += ring.size();
    const std::size_t holes = rings.size() - 1;
    const std::size_t cap_triangles = vertices + 2 * holes - 2;
    const std::size_t total_triangles = 2 * cap_triangles + 2 * vertices;
    if (!triangles_admitted)
        check_add(total_triangles, sampling->limits.definition_triangles,
                  &sampling->definition_triangles, "definition-local triangle");

    PlanarTriangulateInput triangulation;
    PlanarTriangulateRegion region;
    for (const auto& point : rings.front())
        region.outline.push_back({point.x, point.y});
    for (std::size_t ring = 1; ring < rings.size(); ++ring)
    {
        region.holes.emplace_back();
        for (const auto& point : rings[ring])
            region.holes.back().push_back({point.x, point.y});
    }
    triangulation.regions.push_back(std::move(region));
    PlanarTriangulateResult triangles;
    Status status;
    if (triangulate_planar(triangulation, &triangles, &status) != 0 ||
        triangles.regions.size() != 1 || triangles.regions[0].status != PlanarTriangulateStatus::Ok)
        throw std::runtime_error("Analytic extrusion profile triangulation failed: " +
                                 status.message);

    Ring merged;
    for (const auto& ring : rings)
        merged.insert(merged.end(), ring.begin(), ring.end());
    auto top = mesh(id + "/top", material);
    auto bottom = mesh(id + "/bottom", material);
    for (const auto& point : merged)
    {
        position(&top, point, z_max);
        position(&bottom, point, z_min);
    }
    const auto& indices = triangles.regions[0].indices;
    for (std::size_t index = 0; index < indices.size(); index += 3)
    {
        const auto a = indices[index], b = indices[index + 1], c = indices[index + 2];
        const auto& pa = merged[a];
        const auto& pb = merged[b];
        const auto& pc = merged[c];
        const bool ccw = (pb.x - pa.x) * (pc.y - pa.y) - (pb.y - pa.y) * (pc.x - pa.x) > 0;
        triangle(&top, a, ccw ? b : c, ccw ? c : b, sampling);
        triangle(&bottom, a, ccw ? c : b, ccw ? b : c, sampling);
    }
    output->push_back(std::move(top));
    output->push_back(std::move(bottom));
    for (std::size_t ring_index = 0; ring_index < rings.size(); ++ring_index)
    {
        const auto& ring = rings[ring_index];
        auto wall = mesh(id + "/wall-" + std::to_string(ring_index), material);
        for (const auto& point : ring)
        {
            position(&wall, point, z_min);
            position(&wall, point, z_max);
        }
        for (std::size_t index = 0; index < ring.size(); ++index)
        {
            const auto next = (index + 1) % ring.size();
            const auto bottom_a = static_cast<std::uint32_t>(index * 2);
            const auto top_a = bottom_a + 1;
            const auto bottom_b = static_cast<std::uint32_t>(next * 2);
            const auto top_b = bottom_b + 1;
            triangle(&wall, bottom_a, bottom_b, top_b, sampling);
            triangle(&wall, bottom_a, top_b, top_a, sampling);
        }
        output->push_back(std::move(wall));
    }
}

void lower_extrusion(const contracts::AnalyticExtrusionA0& source,
                     std::vector<contracts::MeshIllustrationMesh>* output, Sampling* sampling)
{
    if (!(source.z_max_mm > source.z_min_mm))
        throw std::runtime_error("Analytic extrusion z_max_mm must exceed z_min_mm.");
    std::vector<std::vector<Ring>> expanded;
    expanded.reserve(source.regions.size());
    for (const auto& region : source.regions)
    {
        std::vector<Ring> rings;
        rings.push_back(expand_ring(region.outer, sampling));
        if (signed_area(rings.front()) < 0)
            std::reverse(rings.front().begin(), rings.front().end());
        if (region.holes)
            for (const auto& hole : *region.holes)
            {
                rings.push_back(expand_ring(hole, sampling));
                if (signed_area(rings.back()) > 0)
                    std::reverse(rings.back().begin(), rings.back().end());
            }
        validate_region(rings, sampling);
        expanded.push_back(std::move(rings));
    }
    validate_disjoint_regions(expanded, sampling);
    for (std::size_t region_index = 0; region_index < expanded.size(); ++region_index)
        append_extrusion(source.id + "/region-" + std::to_string(region_index),
                         expanded[region_index], source.z_min_mm, source.z_max_mm, source.material,
                         output, sampling);
}

void lower_cylinder(const contracts::AnalyticCylinderA0& source,
                    std::vector<contracts::MeshIllustrationMesh>* output, Sampling* sampling)
{
    if (!(source.z_max_mm > source.z_min_mm))
        throw std::runtime_error("Analytic cylinder z_max_mm must exceed z_min_mm.");
    if (!std::isfinite(source.center_mm[0] - source.radius_mm) ||
        !std::isfinite(source.center_mm[0] + source.radius_mm) ||
        !std::isfinite(source.center_mm[1] - source.radius_mm) ||
        !std::isfinite(source.center_mm[1] + source.radius_mm))
        throw std::runtime_error("Analytic cylinder has non-finite derived extents.");
    const unsigned count = circle_segments(source.radius_mm, *sampling);
    check_add(4ULL * count - 4, sampling->limits.definition_triangles,
              &sampling->definition_triangles, "definition-local triangle");
    check_add(count, sampling->limits.samples, &sampling->generated_samples,
              "generated curve sample");
    Ring ring;
    ring.reserve(count);
    for (unsigned index = 0; index < count; ++index)
    {
        const double angle = 2 * pi * index / count;
        ring.push_back({source.center_mm[0] + source.radius_mm * std::cos(angle),
                        source.center_mm[1] + source.radius_mm * std::sin(angle)});
    }
    append_extrusion(source.id, {std::move(ring)}, source.z_min_mm, source.z_max_mm,
                     source.material, output, sampling, true);
}

void lower_sphere(const contracts::AnalyticSphereA0& source,
                  std::vector<contracts::MeshIllustrationMesh>* output, Sampling* sampling)
{
    const unsigned longitude = circle_segments(source.radius_mm, *sampling);
    const unsigned latitude = std::clamp((longitude + 1) / 2, 6U, 2048U);
    for (const double center : source.center_mm)
        if (!std::isfinite(center - source.radius_mm) || !std::isfinite(center + source.radius_mm))
            throw std::runtime_error("Analytic sphere has non-finite derived extents.");
    const std::size_t vertex_count = static_cast<std::size_t>(latitude - 1) * longitude + 2;
    const std::size_t triangle_count = 2ULL * longitude * (latitude - 1);
    check_add(triangle_count, sampling->limits.definition_triangles,
              &sampling->definition_triangles, "definition-local triangle");
    check_add(vertex_count, sampling->limits.samples, &sampling->generated_samples,
              "generated curve sample");
    auto result = mesh(source.id, source.material);
    result.normals.emplace();
    const auto add = [&](double nx, double ny, double nz)
    {
        result.normals->insert(result.normals->end(), {nx, ny, nz});
        result.positions.insert(result.positions.end(),
                                {source.center_mm[0] + source.radius_mm * nx,
                                 source.center_mm[1] + source.radius_mm * ny,
                                 source.center_mm[2] + source.radius_mm * nz});
    };
    add(0, 0, 1);
    for (unsigned row = 1; row < latitude; ++row)
    {
        const double polar = pi * row / latitude;
        for (unsigned column = 0; column < longitude; ++column)
        {
            const double azimuth = 2 * pi * column / longitude;
            add(std::sin(polar) * std::cos(azimuth), std::sin(polar) * std::sin(azimuth),
                std::cos(polar));
        }
    }
    const std::uint32_t south = static_cast<std::uint32_t>(result.positions.size() / 3);
    add(0, 0, -1);
    for (unsigned column = 0; column < longitude; ++column)
        triangle(&result, 0, 1 + column, 1 + (column + 1) % longitude, sampling);
    for (unsigned row = 0; row + 2 < latitude; ++row)
        for (unsigned column = 0; column < longitude; ++column)
        {
            const auto next = (column + 1) % longitude;
            const std::uint32_t a = 1 + row * longitude + column;
            const std::uint32_t b = 1 + row * longitude + next;
            const std::uint32_t c = 1 + (row + 1) * longitude + column;
            const std::uint32_t d = 1 + (row + 1) * longitude + next;
            triangle(&result, a, c, d, sampling);
            triangle(&result, a, d, b, sampling);
        }
    const std::uint32_t last = 1 + (latitude - 2) * longitude;
    for (unsigned column = 0; column < longitude; ++column)
        triangle(&result, last + column, south, last + (column + 1) % longitude, sampling);
    output->push_back(std::move(result));
}

Limits effective_limits(const contracts::AnalyticIllustrationSourceA0& source,
                        const std::optional<contracts::MeshIllustrationPrepareOptions>& prepare)
{
    Limits result;
    result.expanded_triangles =
        prepare && prepare->max_triangles ? *prepare->max_triangles : 750000;
    if (!source.lowering || !source.lowering->limits)
        return result;
    const auto& limits = *source.lowering->limits;
    result.definitions = limits.max_reached_definitions.value_or(result.definitions);
    result.occurrences = limits.max_reached_occurrences.value_or(result.occurrences);
    result.primitives = limits.max_reached_primitives.value_or(result.primitives);
    result.rings = limits.max_reached_rings.value_or(result.rings);
    result.points = limits.max_reached_points.value_or(result.points);
    result.samples = limits.max_generated_curve_samples.value_or(result.samples);
    result.definition_triangles =
        limits.max_definition_triangles.value_or(result.definition_triangles);
    result.topology_candidate_pairs =
        limits.max_topology_candidate_pairs.value_or(result.topology_candidate_pairs);
    return result;
}
} // namespace

int lower_analytic_scene(const contracts::AnalyticIllustrationSourceA0& source,
                         const std::optional<contracts::MeshIllustrationPrepareOptions>& prepare,
                         contracts::MeshCollectionA0* collection, AnalyticLoweringStats* statistics,
                         Status* status)
{
    if (collection)
        *collection = {};
    if (statistics)
        *statistics = {};
    const auto fail = [&](int code, const std::string& message)
    {
        if (status)
            *status = {code, message};
        return code;
    };
    if (!collection || !statistics)
        return fail(1, "Analytic illustration lowering requires output values.");
    try
    {
        Sampling sampling;
        sampling.linear =
            source.lowering ? source.lowering->linear_deflection_mm.value_or(0.1) : 0.1;
        sampling.angular =
            source.lowering ? source.lowering->angular_deflection_rad.value_or(0.5) : 0.5;
        sampling.limits = effective_limits(source, prepare);
        sampling.topology_remaining = sampling.limits.topology_candidate_pairs;
        const auto& scene = source.scene;
        if (scene.definitions.size() > sampling.limits.definitions ||
            scene.occurrences.size() > sampling.limits.occurrences)
            throw std::length_error(
                "Analytic illustration exceeds its definition or occurrence limit.");

        std::unordered_map<std::string, std::size_t> definition_index;
        std::vector<std::vector<contracts::MeshIllustrationMesh>> definitions(
            scene.definitions.size());
        std::vector<std::size_t> occurrence_counts(scene.definitions.size());
        std::size_t primitive_count = 0, ring_count = 0, point_count = 0;
        for (std::size_t index = 0; index < scene.definitions.size(); ++index)
        {
            const auto& definition = scene.definitions[index];
            sampling.definition_triangles = 0;
            if (!definition_index.emplace(definition.id, index).second)
                throw std::runtime_error("Analytic definition IDs must be unique.");
            std::unordered_set<std::string> primitive_ids;
            check_add(definition.primitives.size(), sampling.limits.primitives, &primitive_count,
                      "reached primitive");
            for (const auto& primitive : definition.primitives)
            {
                std::visit(
                    [&](const auto& value)
                    {
                        if (!primitive_ids.insert(value.id).second)
                            throw std::runtime_error(
                                "Analytic primitive IDs must be unique within a definition.");
                        using Value = std::decay_t<decltype(value)>;
                        if constexpr (std::is_same_v<Value, contracts::AnalyticExtrusionA0>)
                        {
                            for (const auto& region : value.regions)
                            {
                                check_add(1 + (region.holes ? region.holes->size() : 0),
                                          sampling.limits.rings, &ring_count, "reached ring");
                                check_add(region.outer.points_mm.size(), sampling.limits.points,
                                          &point_count, "reached point");
                                if (region.holes)
                                    for (const auto& hole : *region.holes)
                                        check_add(hole.points_mm.size(), sampling.limits.points,
                                                  &point_count, "reached point");
                            }
                            lower_extrusion(value, &definitions[index], &sampling);
                        }
                        else if constexpr (std::is_same_v<Value, contracts::AnalyticCylinderA0>)
                            lower_cylinder(value, &definitions[index], &sampling);
                        else
                            lower_sphere(value, &definitions[index], &sampling);
                    },
                    primitive);
            }
        }

        std::unordered_set<std::string> occurrence_ids;
        std::size_t expanded_triangles = 0;
        for (const auto& occurrence : scene.occurrences)
        {
            if (!occurrence_ids.insert(occurrence.id).second)
                throw std::runtime_error("Analytic occurrence IDs must be unique.");
            const auto found = definition_index.find(occurrence.definition_id);
            if (found == definition_index.end())
                throw std::runtime_error("Analytic occurrence references an unknown definition.");
            ++occurrence_counts[found->second];
            Matrix4 matrix = identity_matrix();
            std::string transform_error;
            if (occurrence.transform &&
                !validate_affine_matrix(*occurrence.transform, &matrix, &transform_error))
                throw std::runtime_error(transform_error);
            for (const auto& local : definitions[found->second])
            {
                const std::size_t triangles = local.indices->size() / 3;
                check_add(triangles, sampling.limits.expanded_triangles, &expanded_triangles,
                          "occurrence-expanded triangle");
                if (collection->meshes.size() >= 65536)
                    throw std::length_error("Analytic illustration exceeds the 65536 mesh limit.");
                auto placed = local;
                placed.id = occurrence.id + "/" + local.id;
                if (!apply_affine_matrix(&placed, matrix, &transform_error))
                    throw std::runtime_error(transform_error);
                collection->meshes.push_back(std::move(placed));
            }
        }
        if (std::any_of(occurrence_counts.begin(), occurrence_counts.end(),
                        [](std::size_t count) { return count == 0; }))
            throw std::runtime_error("Every analytic definition must have an occurrence.");
        statistics->definitions = static_cast<std::uint32_t>(scene.definitions.size());
        statistics->occurrences = static_cast<std::uint32_t>(scene.occurrences.size());
        statistics->primitives = static_cast<std::uint32_t>(primitive_count);
        statistics->triangles = static_cast<std::uint32_t>(expanded_triangles);
        if (status)
            *status = {};
        return 0;
    }
    catch (const std::length_error& error)
    {
        return fail(102, error.what());
    }
    catch (const std::bad_alloc&)
    {
        return fail(102, "Analytic illustration allocation failed.");
    }
    catch (const std::exception& error)
    {
        return fail(1, error.what());
    }
}
} // namespace geometer::model_illustration_detail

#include "geometer/exact_result_normalization.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{

using namespace geometer::exact;

constexpr std::int64_t kGridSize = 4;

struct Rectangle
{
    std::int64_t x0 = 0;
    std::int64_t y0 = 0;
    std::int64_t x1 = 0;
    std::int64_t y1 = 0;
};

struct PropertyStage
{
    ExactBooleanStageOperation operation = ExactBooleanStageOperation::union_;
    std::vector<std::uint32_t> operands;
};

struct PropertyCase
{
    std::vector<Rectangle> rectangles;
    std::vector<PropertyStage> stages;
};

using IntegerPoint = std::pair<std::int64_t, std::int64_t>;
using IntegerEdge = std::pair<IntegerPoint, IntegerPoint>;

class SplitMix64
{
  public:
    explicit SplitMix64(std::uint64_t seed) : state_(seed) {}

    std::uint64_t next()
    {
        std::uint64_t value = (state_ += 0x9e3779b97f4a7c15ULL);
        value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
        value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
        return value ^ (value >> 31U);
    }

    std::uint32_t bounded(std::uint32_t limit)
    {
        return static_cast<std::uint32_t>(next() % limit);
    }

  private:
    std::uint64_t state_ = 0;
};

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

ConstructionNodeId rational(ConstructionArena& arena, std::int64_t value)
{
    auto result = arena.make_rational(value);
    require(result.error == Error::none && result.node, "seeded-property rational failed");
    return *result.node;
}

ExactPoint point(ConstructionArena& arena, std::int64_t x, std::int64_t y)
{
    return {rational(arena, x), rational(arena, y)};
}

void append_rectangle(ConstructionArena& arena, const Rectangle& rectangle,
                      std::uint64_t occurrence_base, std::uint64_t coverage_id,
                      std::vector<ExactAtomicCurve>& curves,
                      std::vector<ExactCoverageOccurrence>& coverages)
{
    auto append = [&](const ExactPoint& start, const ExactPoint& end, bool material_on_left)
    {
        curves.push_back(
            {ExactAtomicCurveKind::line, start, end, {}, true, false, {{occurrence_base, true}}});
        coverages.push_back({occurrence_base++, coverage_id, material_on_left});
    };
    for (std::int64_t x = rectangle.x0; x < rectangle.x1; ++x)
        append(point(arena, x, rectangle.y0), point(arena, x + 1, rectangle.y0), true);
    for (std::int64_t y = rectangle.y0; y < rectangle.y1; ++y)
        append(point(arena, rectangle.x1, y), point(arena, rectangle.x1, y + 1), true);
    for (std::int64_t x = rectangle.x0; x < rectangle.x1; ++x)
        append(point(arena, x, rectangle.y1), point(arena, x + 1, rectangle.y1), false);
    for (std::int64_t y = rectangle.y0; y < rectangle.y1; ++y)
        append(point(arena, rectangle.x0, y), point(arena, rectangle.x0, y + 1), false);
}

std::uint16_t rectangle_mask(const Rectangle& rectangle)
{
    std::uint16_t mask = 0;
    for (std::int64_t y = 0; y < kGridSize; ++y)
        for (std::int64_t x = 0; x < kGridSize; ++x)
            if (rectangle.x0 <= x && x < rectangle.x1 && rectangle.y0 <= y && y < rectangle.y1)
                mask |= static_cast<std::uint16_t>(1U << static_cast<unsigned>(y * kGridSize + x));
    return mask;
}

std::uint16_t expected_mask(const PropertyCase& property)
{
    std::uint16_t mask = 0;
    for (const PropertyStage& stage : property.stages)
    {
        std::uint16_t stage_mask = 0;
        for (const std::uint32_t operand : stage.operands)
            stage_mask |= rectangle_mask(property.rectangles[operand]);
        if (stage.operation == ExactBooleanStageOperation::union_)
            mask |= stage_mask;
        else
            mask &= static_cast<std::uint16_t>(~stage_mask);
    }
    return mask;
}

std::set<IntegerEdge> expected_boundary(std::uint16_t mask)
{
    std::set<IntegerEdge> edges;
    auto occupied = [&](std::int64_t x, std::int64_t y)
    {
        return 0 <= x && x < kGridSize && 0 <= y && y < kGridSize &&
               (mask &
                static_cast<std::uint16_t>(1U << static_cast<unsigned>(y * kGridSize + x))) != 0;
    };
    for (std::int64_t y = 0; y < kGridSize; ++y)
        for (std::int64_t x = 0; x < kGridSize; ++x)
        {
            if (!occupied(x, y))
                continue;
            if (!occupied(x, y - 1))
                edges.insert({{x, y}, {x + 1, y}});
            if (!occupied(x + 1, y))
                edges.insert({{x + 1, y}, {x + 1, y + 1}});
            if (!occupied(x, y + 1))
                edges.insert({{x + 1, y + 1}, {x, y + 1}});
            if (!occupied(x - 1, y))
                edges.insert({{x, y + 1}, {x, y}});
        }
    return edges;
}

std::uint32_t component_count(std::uint16_t mask)
{
    std::uint16_t visited = 0;
    std::uint32_t components = 0;
    for (std::uint32_t start = 0; start < kGridSize * kGridSize; ++start)
    {
        const auto start_bit = static_cast<std::uint16_t>(1U << start);
        if ((mask & start_bit) == 0 || (visited & start_bit) != 0)
            continue;
        ++components;
        std::array<std::uint32_t, kGridSize * kGridSize> stack{};
        std::uint32_t size = 1;
        stack[0] = start;
        visited |= start_bit;
        while (size != 0)
        {
            const std::uint32_t cell = stack[--size];
            const std::int64_t x = cell % kGridSize;
            const std::int64_t y = cell / kGridSize;
            const std::array<std::pair<std::int64_t, std::int64_t>, 4> neighbors{
                std::pair{x - 1, y}, std::pair{x + 1, y}, std::pair{x, y - 1}, std::pair{x, y + 1}};
            for (const auto [neighbor_x, neighbor_y] : neighbors)
            {
                if (neighbor_x < 0 || neighbor_x >= kGridSize || neighbor_y < 0 ||
                    neighbor_y >= kGridSize)
                    continue;
                const auto bit = static_cast<std::uint16_t>(
                    1U << static_cast<unsigned>(neighbor_y * kGridSize + neighbor_x));
                if ((mask & bit) != 0 && (visited & bit) == 0)
                {
                    visited |= bit;
                    stack[size++] = static_cast<std::uint32_t>(neighbor_y * kGridSize + neighbor_x);
                }
            }
        }
    }
    return components;
}

std::string replay_descriptor(std::uint64_t seed, std::uint32_t case_index,
                              const PropertyCase& property)
{
    std::ostringstream out;
    out << "seed=0x" << std::hex << seed << std::dec << ",case=" << case_index << ",stages=";
    for (const PropertyStage& stage : property.stages)
    {
        out << (stage.operation == ExactBooleanStageOperation::union_ ? 'U' : 'D') << '[';
        for (const std::uint32_t operand : stage.operands)
        {
            const Rectangle& rectangle = property.rectangles[operand];
            out << rectangle.x0 << ',' << rectangle.y0 << ',' << rectangle.x1 << ',' << rectangle.y1
                << ';';
        }
        out << ']';
    }
    return out.str();
}

PropertyCase generate_case(SplitMix64& generator)
{
    PropertyCase property;
    const std::uint32_t stage_count = 1 + generator.bounded(5);
    for (std::uint32_t stage_index = 0; stage_index < stage_count; ++stage_index)
    {
        PropertyStage stage;
        stage.operation = generator.bounded(2) == 0 ? ExactBooleanStageOperation::union_
                                                    : ExactBooleanStageOperation::difference;
        const std::uint32_t operand_count = 1 + generator.bounded(3);
        for (std::uint32_t operand_index = 0; operand_index < operand_count; ++operand_index)
        {
            const std::int64_t x0 = generator.bounded(kGridSize);
            const std::int64_t y0 = generator.bounded(kGridSize);
            const std::int64_t x1 = x0 + 1 + generator.bounded(kGridSize - x0);
            const std::int64_t y1 = y0 + 1 + generator.bounded(kGridSize - y0);
            stage.operands.push_back(static_cast<std::uint32_t>(property.rectangles.size()));
            property.rectangles.push_back({x0, y0, x1, y1});
        }
        property.stages.push_back(std::move(stage));
    }
    return property;
}

struct Scenario
{
    std::set<IntegerEdge> boundary;
    std::string geometry;
    std::string lineage;
    std::size_t regions = 0;
};

enum class PropertyFailure : std::uint8_t
{
    none = 0,
    boundary = 1,
    component_count = 2,
    operand_reversal = 3,
};

Scenario run_scenario(const PropertyCase& property, bool reverse_operands,
                      const std::string& descriptor)
{
    Budget budget({8'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    std::vector<ExactAtomicCurve> curves;
    std::vector<ExactCoverageOccurrence> coverages;
    for (std::uint32_t index = 0; index < property.rectangles.size(); ++index)
        append_rectangle(arena, property.rectangles[index], 100 * (index + 1), index + 1, curves,
                         coverages);
    std::vector<ExactBooleanStage> stages;
    for (std::uint32_t stage_index = 0; stage_index < property.stages.size(); ++stage_index)
    {
        std::vector<ExactBooleanOperand> stage_operands;
        for (const std::uint32_t operand : property.stages[stage_index].operands)
            stage_operands.push_back({operand + 1, 1000 + operand + 1});
        if (reverse_operands)
            std::reverse(stage_operands.begin(), stage_operands.end());
        stages.push_back(
            {stage_index + 1, property.stages[stage_index].operation, std::move(stage_operands)});
    }
    ExactArrangementResult arrangement = build_exact_arrangement(arena, curves, coverages);
    require(arrangement.error == Error::none && arrangement.value,
            descriptor + ": arrangement failed");
    ExactBooleanSelectionResult selection =
        evaluate_exact_boolean_stages(budget, *arrangement.value, stages);
    require(selection.error == Error::none && selection.value, descriptor + ": selection failed");
    ExactBooleanRegionsResult regions =
        build_exact_boolean_regions(budget, *arrangement.value, *selection.value);
    require(regions.error == Error::none && regions.value, descriptor + ": regions failed");
    ExactNormalizedBooleanResultResult normalized =
        normalize_exact_boolean_result(arena, *arrangement.value, *selection.value, *regions.value);
    require(normalized.error == ExactResultNormalizationError::none && normalized.value,
            descriptor + ": normalization failed");

    Scenario result;
    std::ostringstream geometry;
    std::size_t expanded_edge_count = 0;
    for (const ExactNormalizedResultFragment& fragment : normalized.value->fragments())
    {
        require(fragment.kind == ExactAtomicCurveKind::line, descriptor + ": non-line result");
        const ExactNormalizedResultVertex& start =
            normalized.value->vertices()[fragment.start_vertex];
        const ExactNormalizedResultVertex& end = normalized.value->vertices()[fragment.end_vertex];
        const std::int64_t dx = end.x_nm - start.x_nm;
        const std::int64_t dy = end.y_nm - start.y_nm;
        const std::int64_t length = std::max(std::llabs(dx), std::llabs(dy));
        const std::int64_t step_x = (dx > 0) - (dx < 0);
        const std::int64_t step_y = (dy > 0) - (dy < 0);
        require((dx == 0) != (dy == 0) && length == std::llabs(dx) + std::llabs(dy),
                descriptor + ": non-axis-aligned result");
        for (std::int64_t offset = 0; offset < length; ++offset)
        {
            const IntegerPoint begin{start.x_nm + offset * step_x, start.y_nm + offset * step_y};
            result.boundary.insert({begin, {begin.first + step_x, begin.second + step_y}});
            ++expanded_edge_count;
        }
        geometry << start.x_nm << ',' << start.y_nm << ',' << end.x_nm << ',' << end.y_nm << ';';
    }
    require(result.boundary.size() == expanded_edge_count,
            descriptor + ": normalized boundary contains duplicate directed unit edges");
    std::set<IntegerPoint> referenced_vertices;
    for (const IntegerEdge& edge : result.boundary)
    {
        referenced_vertices.insert(edge.first);
        referenced_vertices.insert(edge.second);
    }
    std::set<IntegerPoint> published_vertices;
    for (const ExactNormalizedResultVertex& vertex : normalized.value->vertices())
        published_vertices.insert({vertex.x_nm, vertex.y_nm});
    require(published_vertices == referenced_vertices &&
                published_vertices.size() == normalized.value->vertices().size(),
            descriptor + ": normalized topology contains duplicate or unreferenced vertices");
    result.geometry = geometry.str();
    std::ostringstream lineage;
    for (const ExactSelectedFace& face : selection.value->faces())
    {
        lineage << face.material << 'p';
        for (std::uint32_t index = 0; index < face.positive_source_count; ++index)
            lineage << selection.value->positive_sources()[face.positive_source_begin + index]
                    << ',';
        lineage << 's';
        for (std::uint32_t index = 0; index < face.subtraction_source_count; ++index)
            lineage << selection.value->subtraction_sources()[face.subtraction_source_begin + index]
                    << ',';
        lineage << ';';
    }
    result.lineage = lineage.str();
    result.regions = normalized.value->regions().size();
    return result;
}

std::string boundary_signature(const std::set<IntegerEdge>& boundary)
{
    std::ostringstream out;
    for (const IntegerEdge& edge : boundary)
        out << edge.first.first << edge.first.second << edge.second.first << edge.second.second
            << '.';
    return out.str();
}

struct PropertyEvaluation
{
    Scenario baseline;
    PropertyFailure failure = PropertyFailure::none;
};

PropertyEvaluation evaluate_property(const PropertyCase& property, const std::string& descriptor)
{
    const std::uint16_t mask = expected_mask(property);
    const Scenario baseline = run_scenario(property, false, descriptor);
    if (baseline.boundary != expected_boundary(mask))
        return {baseline, PropertyFailure::boundary};
    if (baseline.regions != component_count(mask))
        return {baseline, PropertyFailure::component_count};
    const Scenario reversed = run_scenario(property, true, descriptor);
    if (reversed.boundary != baseline.boundary || reversed.geometry != baseline.geometry ||
        reversed.lineage != baseline.lineage || reversed.regions != baseline.regions)
        return {baseline, PropertyFailure::operand_reversal};
    return {baseline, PropertyFailure::none};
}

PropertyCase minimize_failure(PropertyCase current,
                              const std::function<bool(const PropertyCase&)>& still_fails)
{
    bool changed = true;
    while (changed)
    {
        changed = false;
        for (std::size_t stage = 0; stage < current.stages.size(); ++stage)
        {
            PropertyCase candidate = current;
            candidate.stages.erase(candidate.stages.begin() + static_cast<std::ptrdiff_t>(stage));
            if (still_fails(candidate))
            {
                current = std::move(candidate);
                changed = true;
                break;
            }
        }
        if (changed)
            continue;
        for (std::size_t stage = 0; stage < current.stages.size() && !changed; ++stage)
            for (std::size_t operand = 0; operand < current.stages[stage].operands.size();
                 ++operand)
            {
                PropertyCase candidate = current;
                candidate.stages[stage].operands.erase(candidate.stages[stage].operands.begin() +
                                                       static_cast<std::ptrdiff_t>(operand));
                if (still_fails(candidate))
                {
                    current = std::move(candidate);
                    changed = true;
                    break;
                }
            }
        if (changed)
            continue;
        for (std::size_t stage_index = 0; stage_index < current.stages.size() && !changed;
             ++stage_index)
            for (std::size_t operand_index = 0;
                 operand_index < current.stages[stage_index].operands.size() && !changed;
                 ++operand_index)
            {
                const std::uint32_t operand = current.stages[stage_index].operands[operand_index];
                const Rectangle original = current.rectangles[operand];
                const std::array<Rectangle, 4> candidates{
                    Rectangle{original.x0, original.y0, original.x0 + 1, original.y1},
                    Rectangle{original.x0, original.y0, original.x1, original.y0 + 1},
                    Rectangle{0, original.y0, original.x1 - original.x0, original.y1},
                    Rectangle{original.x0, 0, original.x1, original.y1 - original.y0},
                };
                for (const Rectangle& rectangle : candidates)
                {
                    if (rectangle.x0 >= rectangle.x1 || rectangle.y0 >= rectangle.y1 ||
                        (rectangle.x0 == original.x0 && rectangle.y0 == original.y0 &&
                         rectangle.x1 == original.x1 && rectangle.y1 == original.y1))
                        continue;
                    PropertyCase candidate = current;
                    candidate.rectangles[operand] = rectangle;
                    if (still_fails(candidate))
                    {
                        current = std::move(candidate);
                        changed = true;
                        break;
                    }
                }
            }
    }
    return current;
}

bool covers_origin(const PropertyCase& property)
{
    for (const PropertyStage& stage : property.stages)
        if (stage.operation == ExactBooleanStageOperation::union_)
            for (const std::uint32_t operand : stage.operands)
            {
                const Rectangle& rectangle = property.rectangles[operand];
                if (rectangle.x0 == 0 && rectangle.y0 == 0)
                    return true;
            }
    return false;
}

std::string require_reducer_sentinel()
{
    PropertyCase fixture;
    fixture.rectangles = {{1, 1, 2, 2}, {0, 0, 4, 4}, {2, 2, 4, 4}, {3, 0, 4, 1}};
    fixture.stages = {
        {ExactBooleanStageOperation::difference, {0}},
        {ExactBooleanStageOperation::union_, {1, 2}},
        {ExactBooleanStageOperation::difference, {3}},
    };
    const PropertyCase minimized = minimize_failure(fixture, covers_origin);
    const std::string descriptor = replay_descriptor(0, 0, minimized);
    require(descriptor == "seed=0x0,case=0,stages=U[0,0,1,1;]",
            "seeded-property reducer sentinel did not reach its unique minimal fixture");
    return descriptor;
}

std::string require_multistage_reducer_sentinel()
{
    PropertyCase fixture;
    fixture.rectangles = {{1, 1, 2, 2}, {0, 0, 4, 4}, {2, 2, 4, 4}};
    fixture.stages = {
        {ExactBooleanStageOperation::difference, {0}},
        {ExactBooleanStageOperation::union_, {1, 2}},
    };
    const PropertyCase minimized =
        minimize_failure(fixture, [](const PropertyCase& candidate)
                         { return candidate.stages.size() >= 2 && covers_origin(candidate); });
    const std::string descriptor = replay_descriptor(0, 1, minimized);
    require(descriptor == "seed=0x0,case=1,stages=D[]U[0,0,1,1;]",
            "multistage reducer sentinel did not exercise a guarded rectangle mutation");
    return descriptor;
}

} // namespace

int main(int argc, char** argv)
{
    require(argc == 1 || (argc == 2 && std::string(argv[1]) == "--nightly"),
            "usage: geometer_exact_boolean_seeded_property_test [--nightly]");
    const bool nightly = argc == 2;
    const std::vector<std::uint64_t> pull_request_seeds{
        0x0000000000000001ULL,
        0x0123456789abcdefULL,
        0x9e3779b97f4a7c15ULL,
        0xffffffffffffffffULL,
    };
    const std::vector<std::uint64_t> nightly_seeds{
        0x0000000000000000ULL, 0x0000000000000001ULL, 0x0123456789abcdefULL, 0x3141592653589793ULL,
        0x9e3779b97f4a7c15ULL, 0xc0ffeec0ffeec0ffULL, 0xdeadbeefcafebabeULL, 0xffffffffffffffffULL,
    };
    const std::vector<std::uint64_t>& seeds = nightly ? nightly_seeds : pull_request_seeds;
    const std::uint32_t cases_per_seed = nightly ? 16 : 8;
    const std::string reducer_sentinel = require_reducer_sentinel();
    const std::string multistage_reducer_sentinel = require_multistage_reducer_sentinel();
    std::ostringstream signature;
    std::uint32_t case_count = 0;
    for (const std::uint64_t seed : seeds)
    {
        SplitMix64 generator(seed);
        for (std::uint32_t case_index = 0; case_index < cases_per_seed; ++case_index)
        {
            const PropertyCase property = generate_case(generator);
            const std::string descriptor = replay_descriptor(seed, case_index, property);
            const std::uint16_t mask = expected_mask(property);
            const PropertyEvaluation evaluation = evaluate_property(property, descriptor);
            if (evaluation.failure != PropertyFailure::none)
            {
                const PropertyFailure failure = evaluation.failure;
                const PropertyCase minimized = minimize_failure(
                    property,
                    [&](const PropertyCase& candidate)
                    {
                        return evaluate_property(candidate, "reducer-candidate").failure == failure;
                    });
                require(false, descriptor + ": property failure=" +
                                   std::to_string(static_cast<unsigned>(failure)) +
                                   ",minimized=" + replay_descriptor(seed, case_index, minimized));
            }
            const Scenario& baseline = evaluation.baseline;
            signature << std::hex << std::setw(4) << std::setfill('0') << mask << std::dec << ':'
                      << boundary_signature(baseline.boundary) << ':' << baseline.regions << ':'
                      << baseline.lineage << '|';
            ++case_count;
        }
    }
    require(case_count == seeds.size() * cases_per_seed, "seeded-property case count changed");
    std::cout << "EXACT_BOOLEAN_SEEDED_PROPERTY=";
    if (nightly)
        std::cout << "profile:nightly,";
    std::cout << "seeds:" << seeds.size() << ",cases:" << case_count << '|' << signature.str()
              << "reducer:" << reducer_sentinel
              << "|reducer_multistage:" << multistage_reducer_sentinel << '\n';
    return 0;
}

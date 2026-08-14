#include "geometer/exact_result_normalization.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace
{

using namespace geometer::exact;

struct Rectangle
{
    std::int64_t x0 = 0;
    std::int64_t y0 = 0;
    std::int64_t x1 = 0;
    std::int64_t y1 = 0;
};

struct StageInput
{
    ExactBooleanStageOperation operation = ExactBooleanStageOperation::union_;
    Rectangle rectangle;
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
    require(result.error == Error::none && result.node, "enumeration rational fixture failed");
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

std::vector<Rectangle> rectangle_catalog()
{
    const std::array<std::pair<std::int64_t, std::int64_t>, 3> spans{
        std::pair{0, 1}, std::pair{0, 2}, std::pair{1, 2}};
    std::vector<Rectangle> rectangles;
    for (const auto [x0, x1] : spans)
        for (const auto [y0, y1] : spans)
            rectangles.push_back({x0, y0, x1, y1});
    return rectangles;
}

std::uint8_t rectangle_mask(const Rectangle& rectangle)
{
    std::uint8_t mask = 0;
    for (std::int64_t y = 0; y < 2; ++y)
        for (std::int64_t x = 0; x < 2; ++x)
            if (rectangle.x0 <= x && x + 1 <= rectangle.x1 && rectangle.y0 <= y &&
                y + 1 <= rectangle.y1)
                mask |= static_cast<std::uint8_t>(1U << static_cast<unsigned>(y * 2 + x));
    return mask;
}

std::uint8_t expected_mask(const std::vector<StageInput>& stages)
{
    std::uint8_t mask = 0;
    for (const StageInput& stage : stages)
    {
        const std::uint8_t operand = rectangle_mask(stage.rectangle);
        if (stage.operation == ExactBooleanStageOperation::union_)
            mask |= operand;
        else
            mask &= static_cast<std::uint8_t>(~operand);
    }
    return static_cast<std::uint8_t>(mask & 0x0FU);
}

std::uint32_t bit_count(std::uint8_t mask)
{
    std::uint32_t count = 0;
    for (; mask != 0; mask >>= 1U)
        count += mask & 1U;
    return count;
}

std::uint32_t component_count(std::uint8_t mask)
{
    std::uint32_t components = 0;
    std::uint8_t visited = 0;
    for (std::uint32_t start = 0; start < 4; ++start)
    {
        const std::uint8_t start_bit = static_cast<std::uint8_t>(1U << start);
        if ((mask & start_bit) == 0 || (visited & start_bit) != 0)
            continue;
        ++components;
        std::array<std::uint32_t, 4> stack{};
        std::uint32_t size = 1;
        stack[0] = start;
        visited |= start_bit;
        while (size != 0)
        {
            const std::uint32_t cell = stack[--size];
            const std::uint32_t x = cell % 2;
            const std::uint32_t y = cell / 2;
            const std::array<std::int32_t, 4> neighbors{
                x == 0 ? -1 : static_cast<std::int32_t>(cell - 1),
                x == 1 ? -1 : static_cast<std::int32_t>(cell + 1),
                y == 0 ? -1 : static_cast<std::int32_t>(cell - 2),
                y == 1 ? -1 : static_cast<std::int32_t>(cell + 2),
            };
            for (const std::int32_t neighbor : neighbors)
            {
                if (neighbor < 0)
                    continue;
                const auto bit = static_cast<std::uint8_t>(1U << neighbor);
                if ((mask & bit) != 0 && (visited & bit) == 0)
                {
                    visited |= bit;
                    stack[size++] = static_cast<std::uint32_t>(neighbor);
                }
            }
        }
    }
    return components;
}

bool face_contains_coverage(const ExactArrangement& arrangement, std::uint32_t face_id,
                            std::uint64_t coverage_id)
{
    const ExactArrangementFace& face = arrangement.faces()[face_id];
    const auto begin = arrangement.face_coverages().begin() + face.coverage_begin;
    return std::binary_search(begin, begin + face.coverage_count, coverage_id);
}

void require_face_oracle(const ExactArrangement& arrangement,
                         const ExactBooleanSelection& selection,
                         const std::vector<StageInput>& stages)
{
    require(selection.faces().size() == arrangement.faces().size(),
            "enumeration selection face inventory changed");
    for (std::uint32_t face_id = 0; face_id < arrangement.faces().size(); ++face_id)
    {
        bool expected = false;
        for (std::uint32_t stage = 0; stage < stages.size(); ++stage)
            if (face_contains_coverage(arrangement, face_id, stage + 1))
                expected = stages[stage].operation == ExactBooleanStageOperation::union_;
        require(selection.faces()[face_id].material == expected,
                "ordered-stage face oracle mismatch");
    }
}

std::int64_t signed_double_area(const ExactNormalizedBooleanResult& result)
{
    std::int64_t area = 0;
    for (const ExactNormalizedResultFragment& fragment : result.fragments())
    {
        require(fragment.kind == ExactAtomicCurveKind::line &&
                    fragment.direction == NormalizedFragmentDirection::not_applicable &&
                    !fragment.major_arc && fragment.radius_nm == 0,
                "rectangle enumeration produced a non-line fragment");
        const ExactNormalizedResultVertex& start = result.vertices()[fragment.start_vertex];
        const ExactNormalizedResultVertex& end = result.vertices()[fragment.end_vertex];
        area += start.x_nm * end.y_nm - start.y_nm * end.x_nm;
    }
    return area;
}

std::string run_case(const std::vector<StageInput>& inputs)
{
    static std::uint32_t next_case = 0;
    const std::string case_label = "rectangle enumeration case " + std::to_string(next_case++);
    Budget budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    std::vector<ExactAtomicCurve> curves;
    std::vector<ExactCoverageOccurrence> coverages;
    std::vector<ExactBooleanStage> stages;
    for (std::uint32_t index = 0; index < inputs.size(); ++index)
    {
        const std::uint64_t identity = index + 1;
        append_rectangle(arena, inputs[index].rectangle, identity * 100, identity, curves,
                         coverages);
        stages.push_back({identity, inputs[index].operation, {{identity, 1000 + identity}}});
    }
    ExactArrangementResult arrangement = build_exact_arrangement(arena, curves, coverages);
    require(arrangement.error == Error::none && arrangement.value,
            case_label + " arrangement failed");
    ExactBooleanSelectionResult selection =
        evaluate_exact_boolean_stages(budget, *arrangement.value, stages);
    require(selection.error == Error::none && selection.value, case_label + " selection failed");
    require_face_oracle(*arrangement.value, *selection.value, inputs);
    ExactBooleanRegionsResult regions =
        build_exact_boolean_regions(budget, *arrangement.value, *selection.value);
    require(regions.error == Error::none && regions.value, case_label + " regions failed");
    ExactNormalizedBooleanResultResult normalized =
        normalize_exact_boolean_result(arena, *arrangement.value, *selection.value, *regions.value);
    require(normalized.error == ExactResultNormalizationError::none && normalized.value,
            case_label + " normalization failed");

    const std::uint8_t mask = expected_mask(inputs);
    const std::uint32_t components = component_count(mask);
    require(signed_double_area(*normalized.value) == 2 * bit_count(mask),
            "enumeration normalized area disagrees with unit-cell oracle");
    require(normalized.value->regions().size() == components &&
                normalized.value->rings().size() == components,
            "enumeration component count disagrees with unit-cell oracle");

    std::ostringstream signature;
    signature << static_cast<unsigned>(mask) << ',' << components << ','
              << normalized.value->vertices().size() << ',' << normalized.value->fragments().size()
              << ';';
    return signature.str();
}

} // namespace

int main()
{
    const std::vector<Rectangle> rectangles = rectangle_catalog();
    const std::array<ExactBooleanStageOperation, 2> operations{
        ExactBooleanStageOperation::union_, ExactBooleanStageOperation::difference};
    std::string signature;
    std::uint32_t case_count = 0;
    for (const Rectangle& first : rectangles)
        for (const auto first_operation : operations)
        {
            signature += run_case({{first_operation, first}});
            ++case_count;
        }
    for (const Rectangle& first : rectangles)
        for (const Rectangle& second : rectangles)
            for (const auto first_operation : operations)
                for (const auto second_operation : operations)
                {
                    signature += run_case({{first_operation, first}, {second_operation, second}});
                    ++case_count;
                }

    const std::array<std::uint32_t, 3> triple_catalog{0, 4, 8};
    for (const std::uint32_t first : triple_catalog)
        for (const std::uint32_t second : triple_catalog)
            for (const std::uint32_t third : triple_catalog)
                for (const auto first_operation : operations)
                    for (const auto second_operation : operations)
                        for (const auto third_operation : operations)
                        {
                            signature += run_case({{first_operation, rectangles[first]},
                                                   {second_operation, rectangles[second]},
                                                   {third_operation, rectangles[third]}});
                            ++case_count;
                        }
    require(case_count == 558, "bounded rectangle enumeration case count changed");
    std::cout << "EXACT_RECTANGLE_ENUMERATION=cases:" << case_count << '|' << signature << '\n';
    return 0;
}

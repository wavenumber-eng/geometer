#include "geometer/exact_boolean_stages.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace
{

using namespace geometer::exact;

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

ConstructionNodeId rational(ConstructionArena& arena, const BigInt& value)
{
    auto result = arena.make_rational(value);
    require(result.error == Error::none && result.node, "stage rational fixture failed");
    return *result.node;
}

ExactPoint point(ConstructionArena& arena, const BigInt& x, const BigInt& y)
{
    return {rational(arena, x), rational(arena, y)};
}

void append_box(ConstructionArena& arena, const BigInt& low, const BigInt& high,
                std::uint64_t occurrence_base, std::uint64_t coverage_id,
                std::vector<ExactAtomicCurve>& curves,
                std::vector<ExactCoverageOccurrence>& coverages)
{
    const ExactPoint a = point(arena, low, low);
    const ExactPoint b = point(arena, high, low);
    const ExactPoint c = point(arena, high, high);
    const ExactPoint d = point(arena, low, high);
    curves.push_back(
        {ExactAtomicCurveKind::line, a, b, {}, true, false, {{occurrence_base, true}}});
    curves.push_back(
        {ExactAtomicCurveKind::line, b, c, {}, true, false, {{occurrence_base + 1, true}}});
    curves.push_back(
        {ExactAtomicCurveKind::line, d, c, {}, true, false, {{occurrence_base + 2, true}}});
    curves.push_back(
        {ExactAtomicCurveKind::line, a, d, {}, true, false, {{occurrence_base + 3, true}}});
    coverages.push_back({occurrence_base, coverage_id, true});
    coverages.push_back({occurrence_base + 1, coverage_id, true});
    coverages.push_back({occurrence_base + 2, coverage_id, false});
    coverages.push_back({occurrence_base + 3, coverage_id, false});
}

std::string selection_signature(const ExactBooleanSelection& selection)
{
    std::ostringstream out;
    for (const ExactSelectedFace& face : selection.faces())
    {
        out << (face.material ? 'm' : 'e') << ':';
        for (std::uint32_t index = 0; index < face.positive_source_count; ++index)
            out << selection.positive_sources()[face.positive_source_begin + index] << ',';
        out << ':';
        for (std::uint32_t index = 0; index < face.subtraction_source_count; ++index)
            out << selection.subtraction_sources()[face.subtraction_source_begin + index] << ',';
        out << ';';
    }
    return out.str();
}

std::vector<ExactBooleanStage> stages(bool alternate)
{
    std::vector<ExactBooleanOperand> union_operands{{100, 1000}, {400, 4000}};
    if (alternate)
        std::reverse(union_operands.begin(), union_operands.end());
    return {
        {10, ExactBooleanStageOperation::union_, union_operands},
        {20, ExactBooleanStageOperation::difference, {{200, 2000}}},
        {30, ExactBooleanStageOperation::union_, {{300, 3000}}},
    };
}

void verify_semantics(const ExactArrangement& arrangement, const ExactBooleanSelection& selection)
{
    require(selection.faces().size() == arrangement.faces().size(),
            "selection must close over every exact face");
    bool found_absorbed = false;
    bool found_removed = false;
    bool found_refilled = false;
    for (std::uint32_t face_id = 0; face_id < arrangement.faces().size(); ++face_id)
    {
        const ExactArrangementFace& arrangement_face = arrangement.faces()[face_id];
        const auto coverage_begin =
            arrangement.face_coverages().begin() + arrangement_face.coverage_begin;
        const auto coverage_end = coverage_begin + arrangement_face.coverage_count;
        const bool in_outer = std::binary_search(coverage_begin, coverage_end, 100);
        const bool in_subtraction = std::binary_search(coverage_begin, coverage_end, 200);
        const bool in_refill = std::binary_search(coverage_begin, coverage_end, 300);
        const ExactSelectedFace& selected = selection.faces()[face_id];
        const auto positive_begin =
            selection.positive_sources().begin() + selected.positive_source_begin;
        const auto positive_end = positive_begin + selected.positive_source_count;
        const auto subtraction_begin =
            selection.subtraction_sources().begin() + selected.subtraction_source_begin;
        const auto subtraction_end = subtraction_begin + selected.subtraction_source_count;
        if (in_outer && !in_subtraction)
        {
            require(selected.material &&
                        std::vector<std::uint64_t>(positive_begin, positive_end) ==
                            std::vector<std::uint64_t>({1000, 4000}) &&
                        subtraction_begin == subtraction_end,
                    "absorbed coincident positives must both remain lineage");
            found_absorbed = true;
        }
        if (in_subtraction && !in_refill)
        {
            require(!selected.material && positive_begin == positive_end &&
                        std::vector<std::uint64_t>(subtraction_begin, subtraction_end) ==
                            std::vector<std::uint64_t>({2000}),
                    "difference must clear positive lineage and retain its effect");
            found_removed = true;
        }
        if (in_refill)
        {
            require(selected.material &&
                        std::vector<std::uint64_t>(positive_begin, positive_end) ==
                            std::vector<std::uint64_t>({3000}) &&
                        std::vector<std::uint64_t>(subtraction_begin, subtraction_end) ==
                            std::vector<std::uint64_t>({2000}),
                    "later refill must start fresh lineage without erasing subtraction history");
            found_refilled = true;
        }
    }
    require(found_absorbed && found_removed && found_refilled,
            "stage fixture omitted a governed lineage cell");
}

} // namespace

int main()
{
    Budget geometry_budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(geometry_budget);
    std::vector<ExactAtomicCurve> curves;
    std::vector<ExactCoverageOccurrence> coverages;
    append_box(arena, 0, 12, 100, 100, curves, coverages);
    append_box(arena, 3, 9, 200, 200, curves, coverages);
    append_box(arena, 5, 7, 300, 300, curves, coverages);
    append_box(arena, 0, 12, 400, 400, curves, coverages);
    ExactArrangementResult arrangement_result = build_exact_arrangement(arena, curves, coverages);
    require(arrangement_result.error == Error::none && arrangement_result.value,
            "stage arrangement failed");

    Budget selection_budget({2'000'000'000, 268'435'456});
    ExactBooleanSelectionResult first =
        evaluate_exact_boolean_stages(selection_budget, *arrangement_result.value, stages(false));
    require(first.error == Error::none && first.value, "ordered stage selection failed");
    verify_semantics(*arrangement_result.value, *first.value);
    const std::string signature = selection_signature(*first.value);
    const BudgetUsage usage = selection_budget.usage();

    Budget alternate_budget({2'000'000'000, 268'435'456});
    ExactBooleanSelectionResult alternate =
        evaluate_exact_boolean_stages(alternate_budget, *arrangement_result.value, stages(true));
    require(alternate.error == Error::none && alternate.value &&
                selection_signature(*alternate.value) == signature,
            "operand permutation changed ordered stage selection");

    Budget short_work({usage.work_units - 1, 268'435'456});
    ExactBooleanSelectionResult work_failure =
        evaluate_exact_boolean_stages(short_work, *arrangement_result.value, stages(false));
    require(work_failure.error == Error::resource_limit_exceeded && !work_failure.value,
            "one-unit-short stage work must fail closed");
    Budget short_storage({2'000'000'000, usage.owned_bytes - 1});
    ExactBooleanSelectionResult storage_failure =
        evaluate_exact_boolean_stages(short_storage, *arrangement_result.value, stages(false));
    require(storage_failure.error == Error::resource_limit_exceeded && !storage_failure.value &&
                short_storage.usage().owned_bytes == 0,
            "one-byte-short stage storage must fail without a logical leak");

    auto invalid = stages(false);
    invalid[0].operands.push_back(invalid[0].operands.front());
    Budget invalid_budget({2'000'000'000, 268'435'456});
    ExactBooleanSelectionResult invalid_result =
        evaluate_exact_boolean_stages(invalid_budget, *arrangement_result.value, invalid);
    require(invalid_result.error == Error::invalid_argument && !invalid_result.value,
            "duplicate stage coverage must fail closed");

    std::cout << "EXACT_BOOLEAN_STAGES_VECTOR=EBS1:" << signature << '\n';
    std::cout << "EXACT_BOOLEAN_STAGES_WORK=" << usage.work_units << '\n';
    std::cout << "EXACT_BOOLEAN_STAGES_STORAGE=" << usage.owned_bytes << '\n';
    return 0;
}

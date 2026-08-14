#include "geometer/exact_boolean_outcomes.h"

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
    require(result.error == Error::none && result.node, "outcome rational fixture failed");
    return *result.node;
}

ExactPoint point(ConstructionArena& arena, const BigInt& x, const BigInt& y)
{
    return {rational(arena, x), rational(arena, y)};
}

void append_box(ConstructionArena& arena, const BigInt& low, const BigInt& high,
                std::uint64_t occurrence_base, std::uint64_t coverage_id, std::uint64_t operand_id,
                std::vector<ExactAtomicCurve>& curves,
                std::vector<ExactCoverageOccurrence>& coverages,
                std::vector<ExactOccurrenceSource>& sources)
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
    for (std::uint64_t offset = 0; offset < 4; ++offset)
    {
        const std::uint64_t occurrence_id = occurrence_base + offset;
        sources.push_back({occurrence_id,
                           coverage_id,
                           {ExactSourceKind::authored_segment_curve, ExactSourceRole::authored_line,
                            operand_id, occurrence_id, occurrence_id + 10'000}});
    }
}

std::string signature(const ExactBooleanOutcomes& outcomes)
{
    std::ostringstream out;
    for (const ExactOperandOutcomeEvent& event : outcomes.events())
    {
        out << event.operand_id << ':' << static_cast<unsigned>(event.kind) << "[r";
        for (std::uint32_t index = 0; index < event.ring_reference_count; ++index)
            out << outcomes.ring_references()[event.ring_reference_begin + index] << ',';
        out << "][g";
        for (std::uint32_t index = 0; index < event.region_reference_count; ++index)
            out << outcomes.region_references()[event.region_reference_begin + index] << ',';
        out << "][s";
        for (std::uint32_t index = 0; index < event.source_count; ++index)
        {
            const ExactSourceReference& source =
                outcomes.source_references()[event.source_begin + index];
            out << static_cast<unsigned>(source.kind) << ',' << static_cast<unsigned>(source.role)
                << ',' << source.operand_id << ',' << source.primary_id << ','
                << source.secondary_id << ';';
        }
        out << ']';
    }
    return out.str();
}

bool has_event(const ExactBooleanOutcomes& outcomes, std::uint64_t operand,
               ExactOperandOutcomeKind kind)
{
    return std::any_of(outcomes.events().begin(), outcomes.events().end(),
                       [operand, kind](const ExactOperandOutcomeEvent& event)
                       { return event.operand_id == operand && event.kind == kind; });
}

std::string run_scenario(const ExactArrangement& arrangement,
                         const std::vector<ExactOccurrenceSource>& sources,
                         const std::vector<ExactBooleanStage>& stages,
                         std::uint64_t required_operand, ExactOperandOutcomeKind required_kind)
{
    Budget selection_budget({2'000'000'000, 268'435'456});
    ExactBooleanSelectionResult selection =
        evaluate_exact_boolean_stages(selection_budget, arrangement, stages);
    require(selection.error == Error::none && selection.value, "outcome scenario selection failed");
    Budget region_budget({2'000'000'000, 268'435'456});
    ExactBooleanRegionsResult regions =
        build_exact_boolean_regions(region_budget, arrangement, *selection.value);
    require(regions.error == Error::none && regions.value, "outcome scenario regions failed");
    Budget provenance_budget({2'000'000'000, 268'435'456});
    ExactBooleanProvenanceResult provenance = build_exact_boolean_provenance(
        provenance_budget, arrangement, *selection.value, *regions.value, stages, sources);
    require(provenance.error == Error::none && provenance.value,
            "outcome scenario provenance failed");
    Budget outcome_budget({2'000'000'000, 268'435'456});
    ExactBooleanOutcomesResult outcomes =
        build_exact_boolean_outcomes(outcome_budget, arrangement, *selection.value, *regions.value,
                                     *provenance.value, stages, sources);
    require(outcomes.error == Error::none && outcomes.value, "outcome scenario failed");
    require(has_event(*outcomes.value, required_operand, required_kind),
            "required outcome event omitted");
    return signature(*outcomes.value);
}

} // namespace

int main()
{
    Budget geometry_budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(geometry_budget);
    std::vector<ExactAtomicCurve> curves;
    std::vector<ExactCoverageOccurrence> coverages;
    std::vector<ExactOccurrenceSource> sources;
    append_box(arena, 0, 12, 100, 10, 1000, curves, coverages, sources);
    append_box(arena, 0, 12, 400, 40, 4000, curves, coverages, sources);
    append_box(arena, 3, 9, 200, 20, 2000, curves, coverages, sources);
    append_box(arena, 5, 7, 300, 30, 3000, curves, coverages, sources);
    ExactArrangementResult arrangement = build_exact_arrangement(arena, curves, coverages);
    require(arrangement.error == Error::none && arrangement.value, "outcome arrangement failed");

    const std::vector<ExactBooleanStage> stages{
        {10, ExactBooleanStageOperation::union_, {{40, 4000}, {10, 1000}}},
        {20, ExactBooleanStageOperation::difference, {{20, 2000}}},
        {30, ExactBooleanStageOperation::union_, {{30, 3000}}},
    };
    Budget selection_budget({2'000'000'000, 268'435'456});
    ExactBooleanSelectionResult selection =
        evaluate_exact_boolean_stages(selection_budget, *arrangement.value, stages);
    require(selection.error == Error::none && selection.value, "outcome selection failed");
    Budget region_budget({2'000'000'000, 268'435'456});
    ExactBooleanRegionsResult regions =
        build_exact_boolean_regions(region_budget, *arrangement.value, *selection.value);
    require(regions.error == Error::none && regions.value, "outcome regions failed");
    Budget provenance_budget({2'000'000'000, 268'435'456});
    ExactBooleanProvenanceResult provenance = build_exact_boolean_provenance(
        provenance_budget, *arrangement.value, *selection.value, *regions.value, stages, sources);
    require(provenance.error == Error::none && provenance.value, "outcome provenance failed");
    Budget outcome_budget({2'000'000'000, 268'435'456});
    ExactBooleanOutcomesResult outcomes =
        build_exact_boolean_outcomes(outcome_budget, *arrangement.value, *selection.value,
                                     *regions.value, *provenance.value, stages, sources);
    require(outcomes.error == Error::none && outcomes.value, "outcome projection failed");

    for (const std::uint64_t operand : {1000ULL, 4000ULL})
    {
        require(has_event(*outcomes.value, operand,
                          ExactOperandOutcomeKind::contributes_final_material),
                "coincident positive contributor omitted");
        require(has_event(*outcomes.value, operand,
                          ExactOperandOutcomeKind::redundant_or_absorbed_coverage),
                "same-stage absorbed coverage omitted");
        require(
            has_event(*outcomes.value, operand, ExactOperandOutcomeKind::partially_removed_later),
            "partially removed positive lineage omitted");
    }
    require(
        has_event(*outcomes.value, 2000, ExactOperandOutcomeKind::subtraction_effect_survives) &&
            has_event(*outcomes.value, 2000,
                      ExactOperandOutcomeKind::subtraction_effect_overwritten_later),
        "surviving and overwritten subtraction effects must coexist");
    require(has_event(*outcomes.value, 3000, ExactOperandOutcomeKind::contributes_final_material),
            "refill contributor omitted");
    const std::string canonical = signature(*outcomes.value);

    auto incoherent_sources = sources;
    incoherent_sources.front().source.role = ExactSourceRole::authored_circular_arc;
    Budget incoherent_source_budget({2'000'000'000, 268'435'456});
    ExactBooleanOutcomesResult incoherent_source =
        build_exact_boolean_outcomes(incoherent_source_budget, *arrangement.value, *selection.value,
                                     *regions.value, *provenance.value, stages, incoherent_sources);
    require(incoherent_source.error == Error::invalid_argument && !incoherent_source.value,
            "outcome source inconsistent with its exact curve must fail closed");
    auto relabeled_stages = stages;
    relabeled_stages.front().operation = ExactBooleanStageOperation::difference;
    Budget relabeled_stage_budget({2'000'000'000, 268'435'456});
    ExactBooleanOutcomesResult relabeled_stage =
        build_exact_boolean_outcomes(relabeled_stage_budget, *arrangement.value, *selection.value,
                                     *regions.value, *provenance.value, relabeled_stages, sources);
    require(relabeled_stage.error == Error::invalid_argument && !relabeled_stage.value,
            "outcome stages inconsistent with selection must fail closed");

    auto permuted = stages;
    std::reverse(permuted.front().operands.begin(), permuted.front().operands.end());
    require(run_scenario(*arrangement.value, sources, permuted, 2000,
                         ExactOperandOutcomeKind::subtraction_effect_survives) == canonical,
            "same-stage operand permutation changed outcomes");

    const std::vector<ExactBooleanStage> complete_stages{
        {10, ExactBooleanStageOperation::union_, {{40, 4000}, {10, 1000}}},
        {15, ExactBooleanStageOperation::union_, {{30, 3000}}},
        {20, ExactBooleanStageOperation::difference, {{20, 2000}}},
    };
    const std::string complete = run_scenario(*arrangement.value, sources, complete_stages, 3000,
                                              ExactOperandOutcomeKind::completely_removed_later);
    const std::vector<ExactBooleanStage> no_effect_stages{
        {10, ExactBooleanStageOperation::union_, {{40, 4000}, {10, 1000}}},
        {20, ExactBooleanStageOperation::difference, {{20, 2000}}},
        {30, ExactBooleanStageOperation::difference, {{30, 3000}}},
    };
    const std::string no_effect = run_scenario(*arrangement.value, sources, no_effect_stages, 3000,
                                               ExactOperandOutcomeKind::no_effect);

    const BudgetUsage usage = outcome_budget.usage();
    Budget short_work({usage.work_units - 1, 268'435'456});
    ExactBooleanOutcomesResult work_failure =
        build_exact_boolean_outcomes(short_work, *arrangement.value, *selection.value,
                                     *regions.value, *provenance.value, stages, sources);
    require(work_failure.error == Error::resource_limit_exceeded && !work_failure.value,
            "one-unit-short outcome work must fail closed");
    Budget short_storage({2'000'000'000, usage.owned_bytes - 1});
    ExactBooleanOutcomesResult storage_failure =
        build_exact_boolean_outcomes(short_storage, *arrangement.value, *selection.value,
                                     *regions.value, *provenance.value, stages, sources);
    require(storage_failure.error == Error::resource_limit_exceeded && !storage_failure.value &&
                short_storage.usage().owned_bytes == 0,
            "one-byte-short outcome storage must fail without a logical leak");

    std::cout << "EXACT_BOOLEAN_OUTCOMES_VECTOR=EBO1:" << canonical << '|' << complete << '|'
              << no_effect << '\n';
    std::cout << "EXACT_BOOLEAN_OUTCOMES_WORK=" << usage.work_units << '\n';
    std::cout << "EXACT_BOOLEAN_OUTCOMES_STORAGE=" << usage.owned_bytes << '\n';
    return 0;
}

#include "geometer/exact_boolean_outcomes.h"
#include "geometer/exact_result_normalization.h"

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
    require(result.error == Error::none && result.node, "identity rational fixture failed");
    return *result.node;
}

ExactPoint point(ConstructionArena& arena, const BigInt& x, const BigInt& y)
{
    return {rational(arena, x), rational(arena, y)};
}

void append_box(ConstructionArena& arena, std::uint64_t occurrence_base, std::uint64_t coverage_id,
                std::uint64_t operand_id, std::vector<ExactAtomicCurve>& curves,
                std::vector<ExactCoverageOccurrence>& coverages,
                std::vector<ExactOccurrenceSource>& sources)
{
    const ExactPoint a = point(arena, 0, 0);
    const ExactPoint b = point(arena, 10, 0);
    const ExactPoint c = point(arena, 10, 10);
    const ExactPoint d = point(arena, 0, 10);
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

std::string geometry_signature(const ExactNormalizedBooleanResult& result)
{
    std::ostringstream out;
    out << 'v';
    for (const auto& vertex : result.vertices())
        out << vertex.x_nm << ',' << vertex.y_nm << ';';
    out << 'f';
    for (const auto& fragment : result.fragments())
        out << fragment.start_vertex << ',' << fragment.end_vertex << ','
            << static_cast<unsigned>(fragment.kind) << ','
            << static_cast<unsigned>(fragment.direction) << ',' << fragment.major_arc << ','
            << fragment.radius_nm << ';';
    out << 'r';
    for (const auto& ring : result.rings())
        out << ring.fragment_begin << ',' << ring.fragment_count << ',' << ring.parent_ring << ','
            << ring.depth << ',' << ring.counterclockwise << ';';
    out << 'g';
    for (const auto& region : result.regions())
        out << region.outer_ring << ';';
    return out.str();
}

std::string selection_signature(const ExactBooleanSelection& selection)
{
    std::ostringstream out;
    for (const auto& face : selection.faces())
    {
        out << (face.material ? 'm' : 'e') << ":p";
        for (std::uint32_t index = 0; index < face.positive_source_count; ++index)
            out << selection.positive_sources()[face.positive_source_begin + index] << ',';
        out << ":s";
        for (std::uint32_t index = 0; index < face.subtraction_source_count; ++index)
            out << selection.subtraction_sources()[face.subtraction_source_begin + index] << ',';
        out << ';';
    }
    return out.str();
}

std::string outcome_signature(const ExactBooleanOutcomes& outcomes)
{
    std::ostringstream out;
    for (const auto& event : outcomes.events())
        out << event.operand_id << ':' << static_cast<unsigned>(event.kind) << ':'
            << event.ring_reference_count << ':' << event.region_reference_count << ';';
    return out.str();
}

struct Scenario
{
    std::string geometry;
    std::string selection;
    std::string outcomes;
    std::vector<std::uint64_t> region_sources;
    std::vector<std::pair<std::uint64_t, ExactOperandOutcomeKind>> events;
};

Scenario run_scenario(bool duplicate_box, const std::vector<ExactBooleanStage>& stages)
{
    Budget budget({4'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    std::vector<ExactAtomicCurve> curves;
    std::vector<ExactCoverageOccurrence> coverages;
    std::vector<ExactOccurrenceSource> sources;
    append_box(arena, 100, 10, 1000, curves, coverages, sources);
    if (duplicate_box)
        append_box(arena, 200, 20, 2000, curves, coverages, sources);

    ExactArrangementResult arrangement = build_exact_arrangement(arena, curves, coverages);
    require(arrangement.error == Error::none && arrangement.value, "identity arrangement failed");
    ExactBooleanSelectionResult selection =
        evaluate_exact_boolean_stages(budget, *arrangement.value, stages);
    require(selection.error == Error::none && selection.value, "identity selection failed");
    ExactBooleanRegionsResult regions =
        build_exact_boolean_regions(budget, *arrangement.value, *selection.value);
    require(regions.error == Error::none && regions.value, "identity regions failed");
    ExactBooleanProvenanceResult provenance = build_exact_boolean_provenance(
        budget, *arrangement.value, *selection.value, *regions.value, stages, sources);
    require(provenance.error == Error::none && provenance.value, "identity provenance failed");
    ExactBooleanOutcomesResult outcomes =
        build_exact_boolean_outcomes(budget, *arrangement.value, *selection.value, *regions.value,
                                     *provenance.value, stages, sources);
    require(outcomes.error == Error::none && outcomes.value, "identity outcomes failed");
    ExactNormalizedBooleanResultResult normalized =
        normalize_exact_boolean_result(arena, *arrangement.value, *selection.value, *regions.value);
    require(normalized.error == ExactResultNormalizationError::none && normalized.value,
            "identity normalization failed");

    Scenario result;
    result.geometry = geometry_signature(*normalized.value);
    result.selection = selection_signature(*selection.value);
    result.outcomes = outcome_signature(*outcomes.value);
    result.region_sources = regions.value->positive_sources();
    for (const auto& event : outcomes.value->events())
        result.events.emplace_back(event.operand_id, event.kind);
    return result;
}

bool has_event(const Scenario& scenario, std::uint64_t operand_id, ExactOperandOutcomeKind kind)
{
    return std::find(scenario.events.begin(), scenario.events.end(), std::pair{operand_id, kind}) !=
           scenario.events.end();
}

} // namespace

int main()
{
    const Scenario baseline =
        run_scenario(false, {{10, ExactBooleanStageOperation::union_, {{10, 1000}}}});
    const Scenario union_empty =
        run_scenario(false, {{10, ExactBooleanStageOperation::union_, {{10, 1000}}},
                             {20, ExactBooleanStageOperation::union_, {}}});
    const Scenario difference_empty =
        run_scenario(false, {{10, ExactBooleanStageOperation::union_, {{10, 1000}}},
                             {20, ExactBooleanStageOperation::difference, {}}});
    require(union_empty.geometry == baseline.geometry &&
                union_empty.selection == baseline.selection &&
                union_empty.outcomes == baseline.outcomes,
            "union(A, empty) changed geometry or lineage");
    require(difference_empty.geometry == baseline.geometry &&
                difference_empty.selection == baseline.selection &&
                difference_empty.outcomes == baseline.outcomes,
            "difference(A, empty) changed geometry or lineage");

    const Scenario union_self =
        run_scenario(true, {{10, ExactBooleanStageOperation::union_, {{20, 2000}, {10, 1000}}}});
    require(union_self.geometry == baseline.geometry &&
                union_self.region_sources == std::vector<std::uint64_t>({1000, 2000}),
            "union(A, A) changed geometry or lost a contributor");
    for (const std::uint64_t operand : {1000ULL, 2000ULL})
    {
        require(
            has_event(union_self, operand, ExactOperandOutcomeKind::contributes_final_material) &&
                has_event(union_self, operand,
                          ExactOperandOutcomeKind::redundant_or_absorbed_coverage),
            "union(A, A) omitted symmetric contributor lineage");
    }

    const Scenario difference_self =
        run_scenario(true, {{10, ExactBooleanStageOperation::union_, {{10, 1000}}},
                            {20, ExactBooleanStageOperation::difference, {{20, 2000}}}});
    require(difference_self.geometry == "vfrg" && difference_self.region_sources.empty(),
            "difference(A, A) must be a successful empty result");
    require(has_event(difference_self, 1000, ExactOperandOutcomeKind::completely_removed_later) &&
                has_event(difference_self, 2000,
                          ExactOperandOutcomeKind::subtraction_effect_survives) &&
                difference_self.outcomes == "1000:4:0:0;2000:5:0:0;",
            "difference(A, A) lineage changed");

    std::cout << "EXACT_BOOLEAN_IDENTITIES=union_self:geometry_equal,region_sources_1000_2000,"
                 "outcomes="
              << union_self.outcomes
              << "|difference_self:empty,outcomes=" << difference_self.outcomes
              << "|union_empty:equal|difference_empty:equal\n";
    return 0;
}

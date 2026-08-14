#include "geometer/exact_boolean_outcomes.h"

#include <algorithm>
#include <array>
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

ConstructionNodeId rational(ConstructionArena& arena, std::int64_t value)
{
    auto result = arena.make_rational(value);
    require(result.error == Error::none && result.node, "lineage rational fixture failed");
    return *result.node;
}

ExactPoint point(ConstructionArena& arena, std::int64_t x, std::int64_t y)
{
    return {rational(arena, x), rational(arena, y)};
}

void append_box(ConstructionArena& arena, std::int64_t x0, std::int64_t x1,
                std::uint64_t occurrence_base, std::uint64_t coverage_id, std::uint64_t operand_id,
                std::vector<ExactAtomicCurve>& curves,
                std::vector<ExactCoverageOccurrence>& coverages,
                std::vector<ExactOccurrenceSource>& sources)
{
    const std::array<ExactPoint, 4> points{point(arena, x0, 0), point(arena, x1, 0),
                                           point(arena, x1, 2), point(arena, x0, 2)};
    const std::array<std::array<std::uint32_t, 2>, 4> endpoints{
        std::array<std::uint32_t, 2>{0, 1}, {1, 2}, {3, 2}, {0, 3}};
    for (std::uint64_t offset = 0; offset < endpoints.size(); ++offset)
    {
        const std::uint64_t occurrence_id = occurrence_base + offset;
        const auto edge = endpoints[offset];
        curves.push_back({ExactAtomicCurveKind::line,
                          points[edge[0]],
                          points[edge[1]],
                          {},
                          true,
                          false,
                          {{occurrence_id, true}}});
        coverages.push_back({occurrence_id, coverage_id, offset < 2});
        sources.push_back({occurrence_id,
                           coverage_id,
                           {ExactSourceKind::authored_segment_curve, ExactSourceRole::authored_line,
                            operand_id, occurrence_id, occurrence_id + 10'000}});
    }
}

std::string region_projection(const ExactBooleanRegions& regions)
{
    std::ostringstream out;
    out << "regions:" << regions.regions().size() << ",associations:";
    for (const ExactResultAssociation& association : regions.associations())
        out << association.source_id << '>' << association.result_region << ',';
    out << ",sources:";
    for (const ExactResultRegion& region : regions.regions())
    {
        out << '[';
        for (std::uint32_t index = 0; index < region.positive_source_count; ++index)
            out << regions.positive_sources()[region.positive_source_begin + index] << ',';
        out << ']';
    }
    return out.str();
}

bool associations_equal(const std::vector<ExactResultAssociation>& actual,
                        const std::vector<ExactResultAssociation>& expected)
{
    return actual.size() == expected.size() &&
           std::equal(actual.begin(), actual.end(), expected.begin(),
                      [](const ExactResultAssociation& left, const ExactResultAssociation& right)
                      {
                          return left.source_id == right.source_id &&
                                 left.result_region == right.result_region;
                      });
}

std::string outcome_projection(const ExactBooleanOutcomes& outcomes)
{
    std::ostringstream out;
    for (const ExactOperandOutcomeEvent& event : outcomes.events())
    {
        out << event.operand_id << ':' << static_cast<unsigned>(event.kind) << "[r";
        for (std::uint32_t index = 0; index < event.region_reference_count; ++index)
            out << outcomes.region_references()[event.region_reference_begin + index] << ',';
        out << "][b";
        for (std::uint32_t index = 0; index < event.ring_reference_count; ++index)
            out << outcomes.ring_references()[event.ring_reference_begin + index] << ',';
        out << ']';
    }
    return out.str();
}

std::string run_pipeline(const ExactArrangement& arrangement,
                         const std::vector<ExactOccurrenceSource>& sources,
                         const std::vector<ExactBooleanStage>& stages)
{
    Budget selection_budget({2'000'000'000, 268'435'456});
    ExactBooleanSelectionResult selection =
        evaluate_exact_boolean_stages(selection_budget, arrangement, stages);
    require(selection.error == Error::none && selection.value, "lineage selection failed");
    Budget region_budget({2'000'000'000, 268'435'456});
    ExactBooleanRegionsResult regions =
        build_exact_boolean_regions(region_budget, arrangement, *selection.value);
    require(regions.error == Error::none && regions.value, "lineage regions failed");
    const std::vector<ExactResultAssociation> expected_associations{
        {1000, 0}, {1000, 1}, {2000, 0}, {2000, 1}};
    require(associations_equal(regions.value->associations(), expected_associations),
            "two contributors must each associate with both disconnected results");
    require(regions.value->regions().size() == 2 && regions.value->rings().size() == 2,
            "lineage matrix must retain two disconnected results");
    for (const ExactResultRegion& region : regions.value->regions())
    {
        const auto begin = regions.value->positive_sources().begin() + region.positive_source_begin;
        require(region.positive_source_count == 2 &&
                    std::vector<std::uint64_t>(begin, begin + region.positive_source_count) ==
                        std::vector<std::uint64_t>({1000, 2000}),
                "each result must carry both positive contributors");
    }

    Budget provenance_budget({2'000'000'000, 268'435'456});
    ExactBooleanProvenanceResult provenance = build_exact_boolean_provenance(
        provenance_budget, arrangement, *selection.value, *regions.value, stages, sources);
    require(provenance.error == Error::none && provenance.value, "lineage provenance failed");
    require(provenance.value->fragments().size() == 8,
            "lineage matrix boundary fragment count changed");
    for (const ExactBoundaryFragmentProvenance& fragment : provenance.value->fragments())
    {
        const auto begin =
            provenance.value->source_references().begin() + fragment.positive_source_begin;
        std::vector<std::uint64_t> operands;
        for (std::uint32_t index = 0; index < fragment.positive_source_count; ++index)
            operands.push_back((begin + index)->operand_id);
        require(operands == std::vector<std::uint64_t>({1000, 2000}) &&
                    fragment.subtraction_source_count == 0,
                "coincident boundary fragment omitted a positive source");
    }

    Budget outcome_budget({2'000'000'000, 268'435'456});
    ExactBooleanOutcomesResult outcomes =
        build_exact_boolean_outcomes(outcome_budget, arrangement, *selection.value, *regions.value,
                                     *provenance.value, stages, sources);
    require(outcomes.error == Error::none && outcomes.value, "lineage outcomes failed");
    const std::string projected_outcomes = outcome_projection(*outcomes.value);
    require(projected_outcomes == "1000:1[r0,1,][b]1000:2[r][b]2000:1[r0,1,][b]2000:2[r][b]",
            "many-to-many contributor or absorbed outcome references changed");
    auto omitted_association = expected_associations;
    omitted_association.pop_back();
    require(!associations_equal(regions.value->associations(), omitted_association),
            "omitted-association mutation escaped the exact lineage oracle");
    return region_projection(*regions.value) + "|outcomes:" + projected_outcomes;
}

} // namespace

int main()
{
    Budget geometry_budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(geometry_budget);
    std::vector<ExactAtomicCurve> curves;
    std::vector<ExactCoverageOccurrence> coverages;
    std::vector<ExactOccurrenceSource> sources;
    append_box(arena, 0, 2, 100, 10, 1000, curves, coverages, sources);
    append_box(arena, 6, 8, 200, 10, 1000, curves, coverages, sources);
    append_box(arena, 0, 2, 300, 20, 2000, curves, coverages, sources);
    append_box(arena, 6, 8, 400, 20, 2000, curves, coverages, sources);
    ExactArrangementResult arrangement = build_exact_arrangement(arena, curves, coverages);
    require(arrangement.error == Error::none && arrangement.value, "lineage arrangement failed");

    const std::vector<ExactBooleanStage> baseline{
        {10, ExactBooleanStageOperation::union_, {{10, 1000}, {20, 2000}}}};
    const std::vector<ExactBooleanStage> permuted{
        {10, ExactBooleanStageOperation::union_, {{20, 2000}, {10, 1000}}}};
    const std::string projection = run_pipeline(*arrangement.value, sources, baseline);
    require(run_pipeline(*arrangement.value, sources, permuted) == projection,
            "operand permutation changed the many-to-many lineage matrix");
    std::cout << "EXACT_BOOLEAN_LINEAGE_MATRIX=EBL1:" << projection
              << "|mutation:omitted_association\n";
    return 0;
}

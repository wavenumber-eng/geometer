#include "geometer/exact_boolean_provenance.h"

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
    require(result.error == Error::none && result.node, "provenance rational fixture failed");
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

std::string source_signature(const ExactSourceReference& source)
{
    std::ostringstream out;
    out << static_cast<unsigned>(source.kind) << ',' << static_cast<unsigned>(source.role) << ','
        << source.operand_id << ',' << source.primary_id << ',' << source.secondary_id;
    return out.str();
}

std::string signature(const ExactBooleanProvenance& provenance)
{
    std::ostringstream out;
    out << "F";
    for (const ExactBoundaryFragmentProvenance& fragment : provenance.fragments())
    {
        out << fragment.half_edge << "[p";
        for (std::uint32_t index = 0; index < fragment.positive_source_count; ++index)
            out << source_signature(
                       provenance.source_references()[fragment.positive_source_begin + index])
                << ';';
        out << "][s";
        for (std::uint32_t index = 0; index < fragment.subtraction_source_count; ++index)
            out << source_signature(
                       provenance.source_references()[fragment.subtraction_source_begin + index])
                << ';';
        out << "]";
    }
    out << "V";
    for (const ExactResultVertexProvenance& vertex : provenance.vertices())
    {
        out << vertex.arrangement_vertex << '[';
        for (std::uint32_t index = 0; index < vertex.source_count; ++index)
            out << source_signature(provenance.source_references()[vertex.source_begin + index])
                << ';';
        out << ']';
    }
    return out.str();
}

const ExactBoundaryFragmentProvenance& find_fragment(const ExactBooleanProvenance& provenance,
                                                     std::uint32_t half_edge)
{
    const auto found =
        std::lower_bound(provenance.fragments().begin(), provenance.fragments().end(), half_edge,
                         [](const ExactBoundaryFragmentProvenance& value, std::uint32_t key)
                         { return value.half_edge < key; });
    require(found != provenance.fragments().end() && found->half_edge == half_edge,
            "ring half-edge omitted from provenance");
    return *found;
}

void verify_ring_sources(const ExactBooleanRegions& regions,
                         const ExactBooleanProvenance& provenance)
{
    for (const ExactResultRing& ring : regions.rings())
    {
        for (std::uint32_t index = 0; index < ring.half_edge_count; ++index)
        {
            const auto& fragment =
                find_fragment(provenance, regions.ring_half_edges()[ring.half_edge_begin + index]);
            if (ring.depth == 0)
                require(fragment.positive_source_count == 2 &&
                            fragment.subtraction_source_count == 0,
                        "coincident outer positives must both own the surviving boundary");
            else if (ring.depth == 1)
                require(fragment.positive_source_count == 0 &&
                            fragment.subtraction_source_count == 1,
                        "difference hole must carry its surviving subtraction effect");
            else
                require(fragment.positive_source_count == 1 &&
                            fragment.subtraction_source_count == 1,
                        "refill boundary must carry fresh positive and surviving subtraction");
        }
    }
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
    ExactArrangementResult arrangement_result = build_exact_arrangement(arena, curves, coverages);
    require(arrangement_result.error == Error::none && arrangement_result.value,
            "provenance arrangement failed");

    const std::vector<ExactBooleanStage> stages{
        {10, ExactBooleanStageOperation::union_, {{40, 4000}, {10, 1000}}},
        {20, ExactBooleanStageOperation::difference, {{20, 2000}}},
        {30, ExactBooleanStageOperation::union_, {{30, 3000}}},
    };
    Budget selection_budget({2'000'000'000, 268'435'456});
    ExactBooleanSelectionResult selection_result =
        evaluate_exact_boolean_stages(selection_budget, *arrangement_result.value, stages);
    require(selection_result.error == Error::none && selection_result.value,
            "provenance selection failed");
    Budget region_budget({2'000'000'000, 268'435'456});
    ExactBooleanRegionsResult region_result = build_exact_boolean_regions(
        region_budget, *arrangement_result.value, *selection_result.value);
    require(region_result.error == Error::none && region_result.value,
            "provenance region merge failed");

    Budget provenance_budget({2'000'000'000, 268'435'456});
    ExactBooleanProvenanceResult provenance_result = build_exact_boolean_provenance(
        provenance_budget, *arrangement_result.value, *selection_result.value, *region_result.value,
        stages, sources);
    require(provenance_result.error == Error::none && provenance_result.value,
            "provenance projection failed");
    verify_ring_sources(*region_result.value, *provenance_result.value);
    require(provenance_result.value->fragments().size() == 12 &&
                provenance_result.value->vertices().size() == 12,
            "nested provenance closure changed");
    std::uint32_t coincident_outer_vertices = 0;
    for (const ExactResultVertexProvenance& vertex : provenance_result.value->vertices())
    {
        require(vertex.source_count == 2 || vertex.source_count == 4,
                "vertex incident-source closure changed");
        if (vertex.source_count == 4)
            ++coincident_outer_vertices;
    }
    require(coincident_outer_vertices == 4,
            "coincident outer vertices must retain both operands' incident curves");
    const std::string canonical = signature(*provenance_result.value);

    auto permuted_sources = sources;
    std::reverse(permuted_sources.begin(), permuted_sources.end());
    Budget permuted_budget({2'000'000'000, 268'435'456});
    ExactBooleanProvenanceResult permuted = build_exact_boolean_provenance(
        permuted_budget, *arrangement_result.value, *selection_result.value, *region_result.value,
        stages, permuted_sources);
    require(permuted.error == Error::none && permuted.value &&
                signature(*permuted.value) == canonical,
            "occurrence metadata permutation changed provenance");

    auto missing = sources;
    missing.pop_back();
    Budget missing_budget({2'000'000'000, 268'435'456});
    ExactBooleanProvenanceResult missing_result = build_exact_boolean_provenance(
        missing_budget, *arrangement_result.value, *selection_result.value, *region_result.value,
        stages, missing);
    require(missing_result.error == Error::invalid_argument && !missing_result.value,
            "missing occurrence provenance must fail closed");
    auto invalid = sources;
    invalid.front().source.secondary_id = 0;
    Budget invalid_budget({2'000'000'000, 268'435'456});
    ExactBooleanProvenanceResult invalid_result = build_exact_boolean_provenance(
        invalid_budget, *arrangement_result.value, *selection_result.value, *region_result.value,
        stages, invalid);
    require(invalid_result.error == Error::invalid_argument && !invalid_result.value,
            "invalid catalog source tuple must fail closed");
    auto incoherent = sources;
    incoherent.front().source.role = ExactSourceRole::authored_circular_arc;
    Budget incoherent_budget({2'000'000'000, 268'435'456});
    ExactBooleanProvenanceResult incoherent_result = build_exact_boolean_provenance(
        incoherent_budget, *arrangement_result.value, *selection_result.value, *region_result.value,
        stages, incoherent);
    require(incoherent_result.error == Error::invalid_argument && !incoherent_result.value,
            "source role inconsistent with its exact curve must fail closed");

    const BudgetUsage usage = provenance_budget.usage();
    Budget short_work({usage.work_units - 1, 268'435'456});
    ExactBooleanProvenanceResult work_failure = build_exact_boolean_provenance(
        short_work, *arrangement_result.value, *selection_result.value, *region_result.value,
        stages, sources);
    require(work_failure.error == Error::resource_limit_exceeded && !work_failure.value,
            "one-unit-short provenance work must fail closed");
    Budget short_storage({2'000'000'000, usage.owned_bytes - 1});
    ExactBooleanProvenanceResult storage_failure = build_exact_boolean_provenance(
        short_storage, *arrangement_result.value, *selection_result.value, *region_result.value,
        stages, sources);
    require(storage_failure.error == Error::resource_limit_exceeded && !storage_failure.value &&
                short_storage.usage().owned_bytes == 0,
            "one-byte-short provenance storage must fail without a logical leak");

    std::cout << "EXACT_BOOLEAN_PROVENANCE_VECTOR=EBP1:" << canonical << '\n';
    std::cout << "EXACT_BOOLEAN_PROVENANCE_WORK=" << usage.work_units << '\n';
    std::cout << "EXACT_BOOLEAN_PROVENANCE_STORAGE=" << usage.owned_bytes << '\n';
    return 0;
}

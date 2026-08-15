#include "geometer/analytic_operand_lowering.h"

#include "geometer/exact_boolean_provenance.h"
#include "geometer/exact_result_normalization.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{

using namespace geometer;
using namespace geometer::exact;

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct SegmentSpec
{
    std::uint64_t id = 0;
    std::uint64_t curve_id = 0;
    std::uint8_t kind = 1;
    std::uint8_t direction = 0;
    bool major_arc = false;
    std::int64_t center_x_nm = 0;
    std::int64_t center_y_nm = 0;
};

std::uint32_t add_ring(AnalyticRequestPacketRecords& records, std::uint64_t ring_id,
                       const std::vector<std::pair<std::int64_t, std::int64_t>>& vertices,
                       const std::vector<SegmentSpec>& segments, std::uint32_t flags = 0)
{
    const auto ring_index = static_cast<std::uint32_t>(records.rings.size());
    const auto vertex_begin = static_cast<std::uint32_t>(records.vertices.size());
    const auto segment_begin = static_cast<std::uint32_t>(records.segments.size());
    std::uint64_t next_vertex_id = ring_id * 100;
    for (const auto& [x, y] : vertices)
        records.vertices.push_back({next_vertex_id++, x, y});
    for (const SegmentSpec& segment : segments)
        records.segments.push_back({segment.id, segment.curve_id, segment.kind, segment.direction,
                                    segment.major_arc, segment.center_x_nm, segment.center_y_nm});
    records.rings.push_back({ring_id, vertex_begin, static_cast<std::uint32_t>(vertices.size()),
                             segment_begin, static_cast<std::uint32_t>(segments.size()), flags});
    return ring_index;
}

AnalyticRequestPacketRecords
single_region_records(const std::vector<std::pair<std::int64_t, std::int64_t>>& vertices,
                      const std::vector<SegmentSpec>& segments)
{
    AnalyticRequestPacketRecords records;
    records.jobs = {{10, 0, 1}};
    records.stages = {{100, 1, 0, 1}};
    records.operands = {{1000, 1, 0}};
    records.planar_regions = {{500, 0, 0, 0}};
    add_ring(records, 600, vertices, segments);
    return records;
}

AnalyticRequestPacketRecords square_region_records(bool clockwise)
{
    std::vector<std::pair<std::int64_t, std::int64_t>> vertices{{0, 0}, {10, 0}, {10, 10}, {0, 10}};
    if (clockwise)
        vertices = {{0, 0}, {0, 10}, {10, 10}, {10, 0}};
    return single_region_records(vertices, {{800, 900, 1, 0, false, 0, 0},
                                            {801, 901, 1, 0, false, 0, 0},
                                            {802, 902, 1, 0, false, 0, 0},
                                            {803, 903, 1, 0, false, 0, 0}});
}

AnalyticRequestPacketRecords capsule_records(std::int64_t start_x, std::int64_t start_y,
                                             std::int64_t end_x, std::int64_t end_y,
                                             std::uint64_t width)
{
    AnalyticRequestPacketRecords records;
    records.jobs = {{10, 0, 1}};
    records.stages = {{100, 1, 0, 1}};
    records.operands = {{1000, 4, 0}};
    records.capsules = {{9300, start_x, start_y, end_x, end_y, width}};
    return records;
}

AnalyticRequestPacketRecords
swept_records(const std::vector<std::pair<std::int64_t, std::int64_t>>& vertices,
              const std::vector<SegmentSpec>& segments, std::uint64_t width)
{
    AnalyticRequestPacketRecords records;
    records.jobs = {{10, 0, 1}};
    records.stages = {{100, 1, 0, 1}};
    records.operands = {{1000, 5, 0}};
    const std::uint32_t ring = add_ring(records, 600, vertices, segments, 1);
    records.swept_paths = {{9400, ring, width}};
    return records;
}

AnalyticLoweredGeometry lower_or_die(ConstructionArena& arena,
                                     const AnalyticRequestPacketRecords& records,
                                     std::uint32_t job_index, const std::string& message)
{
    AnalyticLoweredGeometryResult lowered = lower_analytic_job_geometry(arena, records, job_index);
    require(lowered.error == AnalyticOperandLoweringError::none && lowered.value.has_value(),
            message + " (error " + std::to_string(static_cast<int>(lowered.error)) + ")");
    return std::move(*lowered.value);
}

void require_lowering_error(const AnalyticRequestPacketRecords& records,
                            AnalyticOperandLoweringError expected, const std::string& message)
{
    Budget budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    AnalyticLoweredGeometryResult lowered = lower_analytic_job_geometry(arena, records, 0);
    require(lowered.error == expected && !lowered.value.has_value(), message);
}

void require_source(const ExactOccurrenceSource& source, std::uint64_t occurrence,
                    std::uint64_t coverage, ExactSourceKind kind, ExactSourceRole role,
                    std::uint64_t operand, std::uint64_t primary, std::uint64_t secondary,
                    const std::string& message)
{
    require(source.occurrence_id == occurrence && source.coverage_id == coverage &&
                source.source.kind == kind && source.source.role == role &&
                source.source.operand_id == operand && source.source.primary_id == primary &&
                source.source.secondary_id == secondary,
            message);
}

// Every swept occurrence must carry the operand/feature header and a
// (role, boundary-occurrence-key) pair from the expected set, and every
// expected pair must appear at least once.
void require_swept_sources(const AnalyticLoweredGeometry& lowered,
                           const std::vector<std::pair<ExactSourceRole, std::uint64_t>>& expected,
                           const std::string& label)
{
    for (const ExactOccurrenceSource& source : lowered.occurrence_sources)
    {
        require(source.coverage_id == 1000 &&
                    source.source.kind == ExactSourceKind::compact_feature_role &&
                    source.source.operand_id == 1000 && source.source.primary_id == 9400,
                label + ": swept source header wrong");
        bool found = false;
        for (const auto& [role, secondary] : expected)
            if (source.source.role == role && source.source.secondary_id == secondary)
                found = true;
        require(found, label + ": unexpected swept role/key pair");
    }
    for (const auto& [role, secondary] : expected)
    {
        bool found = false;
        for (const ExactOccurrenceSource& source : lowered.occurrence_sources)
            if (source.source.role == role && source.source.secondary_id == secondary)
                found = true;
        require(found, label + ": missing swept role/key pair");
    }
}

std::string geometry_signature(const ExactNormalizedBooleanResult& result)
{
    std::ostringstream out;
    out << "v";
    for (const auto& vertex : result.vertices())
        out << vertex.x_nm << ',' << vertex.y_nm << ';';
    out << "f";
    for (const auto& fragment : result.fragments())
        out << fragment.start_vertex << ',' << fragment.end_vertex << ','
            << static_cast<unsigned>(fragment.kind) << ','
            << static_cast<unsigned>(fragment.direction) << ',' << fragment.major_arc << ','
            << fragment.radius_nm << ';';
    out << "r";
    for (const auto& ring : result.rings())
        out << ring.fragment_begin << ',' << ring.fragment_count << ',' << ring.parent_ring << ','
            << ring.depth << ',' << ring.counterclockwise << ';';
    out << "g";
    for (const auto& region : result.regions())
        out << region.outer_ring << ';';
    return out.str();
}

struct NormalizedRun
{
    std::string signature;
    std::vector<ExactNormalizedResultRing> rings;
    std::vector<ExactNormalizedResultRegion> regions;
    std::size_t fragment_count = 0;
};

// Lowers one job, runs the exact union pipeline over its single operand
// coverage, checks provenance coverage of every emitted occurrence, and
// returns the certified normalized geometry.
NormalizedRun run_union_pipeline(const AnalyticRequestPacketRecords& records,
                                 std::uint32_t job_index, std::uint64_t coverage_id,
                                 const std::string& label)
{
    // Overlapping swept strokes atomize every curve pair before arranging, so
    // the overlapping-U pipeline needs about 28e9 work units end to end; the
    // limit leaves headroom above that measured peak without masking runaway
    // regressions.
    Budget budget({40'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    const AnalyticLoweredGeometry lowered =
        lower_or_die(arena, records, job_index, label + ": lowering failed");
    ExactArrangementResult arrangement =
        build_exact_arrangement(arena, lowered.curves, lowered.coverages);
    require(arrangement.error == Error::none && arrangement.value.has_value(),
            label + ": arrangement failed");
    // The lowering emits coverage ids and source operand ids as the packet
    // operand id, so the stage operand references it under both roles.
    const std::vector<ExactBooleanStage> stages{
        {1, ExactBooleanStageOperation::union_, {{coverage_id, coverage_id}}}};
    ExactBooleanSelectionResult selection =
        evaluate_exact_boolean_stages(budget, *arrangement.value, stages);
    require(selection.error == Error::none && selection.value.has_value(),
            label + ": selection failed");
    ExactBooleanRegionsResult regions =
        build_exact_boolean_regions(budget, *arrangement.value, *selection.value);
    require(regions.error == Error::none && regions.value.has_value(), label + ": regions failed");
    ExactBooleanProvenanceResult provenance =
        build_exact_boolean_provenance(budget, *arrangement.value, *selection.value, *regions.value,
                                       stages, lowered.occurrence_sources);
    require(provenance.error == Error::none && provenance.value.has_value(),
            label + ": provenance failed");
    ExactNormalizedBooleanResultResult normalized =
        normalize_exact_boolean_result(arena, *arrangement.value, *selection.value, *regions.value);
    require(normalized.error == ExactResultNormalizationError::none && normalized.value.has_value(),
            label + ": normalization failed");
    NormalizedRun run;
    run.signature = geometry_signature(*normalized.value);
    run.rings = normalized.value->rings();
    run.regions = normalized.value->regions();
    run.fragment_count = normalized.value->fragments().size();
    return run;
}

void require_ccw_square_lowering()
{
    Budget budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    const AnalyticLoweredGeometry lowered =
        lower_or_die(arena, square_region_records(false), 0, "ccw square lowering failed");
    require(lowered.curves.size() == 4 && lowered.coverages.size() == 4 &&
                lowered.occurrence_sources.size() == 4,
            "ccw square lowered wrong element counts");
    const bool expected_agrees[4] = {true, true, false, false};
    for (std::size_t index = 0; index < 4; ++index)
    {
        const ExactAtomicCurve& curve = lowered.curves[index];
        require(curve.kind == ExactAtomicCurveKind::line && curve.memberships.size() == 1 &&
                    curve.memberships[0].occurrence_id == index + 1 &&
                    curve.memberships[0].agrees_with_carrier == expected_agrees[index],
                "ccw square curve memberships wrong");
        const ExactCoverageOccurrence& coverage = lowered.coverages[index];
        require(coverage.occurrence_id == index + 1 && coverage.coverage_id == 1000 &&
                    coverage.material_on_left_of_occurrence,
                "ccw square coverage wrong");
        require_source(lowered.occurrence_sources[index], index + 1, 1000,
                       ExactSourceKind::authored_segment_curve, ExactSourceRole::authored_line,
                       1000, 800 + index, 900 + index, "ccw square source reference wrong");
    }
}

void require_cw_square_material_flip()
{
    Budget budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    const AnalyticLoweredGeometry lowered =
        lower_or_die(arena, square_region_records(true), 0, "cw square lowering failed");
    require(lowered.curves.size() == 4, "cw square lowered wrong curve count");
    const bool expected_agrees[4] = {true, true, false, false};
    for (std::size_t index = 0; index < 4; ++index)
    {
        require(lowered.curves[index].memberships[0].agrees_with_carrier == expected_agrees[index],
                "cw square carrier agreement wrong");
        require(!lowered.coverages[index].material_on_left_of_occurrence,
                "cw square must place material on the right of traversal");
    }
}

void require_winding_insensitive_normalization()
{
    const NormalizedRun counterclockwise =
        run_union_pipeline(square_region_records(false), 0, 1000, "ccw square pipeline");
    const NormalizedRun clockwise =
        run_union_pipeline(square_region_records(true), 0, 1000, "cw square pipeline");
    require(!counterclockwise.signature.empty() &&
                counterclockwise.signature == clockwise.signature,
            "authored winding leaked into the normalized result");
    require(counterclockwise.rings.size() == 1 && counterclockwise.rings[0].counterclockwise &&
                counterclockwise.regions.size() == 1,
            "square union normalized to unexpected topology");
}

void require_square_with_circular_hole()
{
    AnalyticRequestPacketRecords records;
    records.jobs = {{10, 0, 1}};
    records.stages = {{100, 1, 0, 1}};
    records.operands = {{1000, 1, 0}};
    add_ring(records, 600, {{0, 0}, {10, 0}, {10, 10}, {0, 10}},
             {{800, 900, 1, 0, false, 0, 0},
              {801, 901, 1, 0, false, 0, 0},
              {802, 902, 1, 0, false, 0, 0},
              {803, 903, 1, 0, false, 0, 0}});
    const std::uint32_t hole_ring =
        add_ring(records, 601, {{3, 5}, {7, 5}},
                 {{810, 910, 2, 1, false, 5, 5}, {811, 910, 2, 1, false, 5, 5}});
    records.planar_regions = {{500, 0, 0, 1}};
    records.ring_references = {hole_ring};

    Budget budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    const AnalyticLoweredGeometry lowered =
        lower_or_die(arena, records, 0, "square-with-hole lowering failed");
    require(lowered.curves.size() == 6, "square-with-hole lowered wrong curve count");
    for (std::size_t index = 0; index < 4; ++index)
        require(lowered.coverages[index].material_on_left_of_occurrence,
                "square-with-hole outer coverage wrong");
    for (std::size_t index = 4; index < 6; ++index)
    {
        require(!lowered.coverages[index].material_on_left_of_occurrence,
                "square-with-hole hole coverage must face outward");
        require(lowered.curves[index].kind == ExactAtomicCurveKind::circular_arc &&
                    lowered.curves[index].counterclockwise &&
                    lowered.curves[index].memberships[0].agrees_with_carrier,
                "square-with-hole hole arcs wrong");
        require(lowered.occurrence_sources[index].source.role ==
                    ExactSourceRole::authored_circular_arc,
                "square-with-hole hole source role wrong");
    }

    const NormalizedRun run = run_union_pipeline(records, 0, 1000, "square-with-hole pipeline");
    require(run.rings.size() == 2 && run.regions.size() == 1,
            "square-with-hole normalized to unexpected topology");
    bool found_hole = false;
    for (const ExactNormalizedResultRing& ring : run.rings)
        if (ring.depth == 1)
            found_hole = !ring.counterclockwise;
    require(found_hole, "square-with-hole lost the clockwise hole ring");
}

void require_arc_leftmost_orientation()
{
    // Left half-disc authored as a clockwise ring: the ring's lexicographic
    // minimum lies strictly inside the bulging arc, so the winding must be
    // read from the arc's travel direction via exact sqrt comparisons.
    const AnalyticRequestPacketRecords records = single_region_records(
        {{2, 0}, {2, 10}}, {{820, 920, 2, 2, false, 2, 5}, {821, 921, 1, 0, false, 0, 0}});
    Budget budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    const AnalyticLoweredGeometry lowered =
        lower_or_die(arena, records, 0, "arc-leftmost lowering failed");
    require(lowered.curves.size() == 2 &&
                lowered.curves[0].kind == ExactAtomicCurveKind::circular_arc &&
                !lowered.curves[0].counterclockwise &&
                !lowered.curves[0].memberships[0].agrees_with_carrier,
            "arc-leftmost curves wrong");
    for (const ExactCoverageOccurrence& coverage : lowered.coverages)
        require(!coverage.material_on_left_of_occurrence,
                "clockwise half-disc must place material on the right");
}

void require_disk_lowering_and_circle_equivalence()
{
    AnalyticRequestPacketRecords disk_records;
    disk_records.jobs = {{10, 0, 1}};
    disk_records.stages = {{100, 1, 0, 1}};
    disk_records.operands = {{1000, 2, 0}};
    disk_records.disks = {{9100, 5, 5, 5}};

    Budget budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    const AnalyticLoweredGeometry lowered =
        lower_or_die(arena, disk_records, 0, "disk lowering failed");
    require(lowered.curves.size() == 2, "disk lowered wrong curve count");
    for (std::size_t index = 0; index < 2; ++index)
    {
        require(lowered.curves[index].kind == ExactAtomicCurveKind::circular_arc &&
                    lowered.curves[index].counterclockwise && !lowered.curves[index].major_arc &&
                    lowered.curves[index].memberships[0].agrees_with_carrier,
                "disk half arcs wrong");
        require(lowered.coverages[index].material_on_left_of_occurrence,
                "disk coverage must face inward");
        require_source(lowered.occurrence_sources[index], index + 1, 1000,
                       ExactSourceKind::compact_feature_role,
                       ExactSourceRole::primitive_outer_circle, 1000, 9100, 0,
                       "disk source reference wrong");
    }

    // The same circle authored as a two-arc ring must normalize identically.
    const AnalyticRequestPacketRecords authored_records = single_region_records(
        {{0, 5}, {10, 5}}, {{830, 930, 2, 1, false, 5, 5}, {831, 930, 2, 1, false, 5, 5}});
    const NormalizedRun disk_run = run_union_pipeline(disk_records, 0, 1000, "disk pipeline");
    const NormalizedRun authored_run =
        run_union_pipeline(authored_records, 0, 1000, "authored circle pipeline");
    require(!disk_run.signature.empty() && disk_run.signature == authored_run.signature,
            "disk and authored circle normalized differently");
}

void require_annulus_lowering()
{
    AnalyticRequestPacketRecords records;
    records.jobs = {{10, 0, 1}};
    records.stages = {{100, 1, 0, 1}};
    records.operands = {{1000, 3, 0}};
    records.annuli = {{9200, 20, 20, 5, 10}};

    Budget budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    const AnalyticLoweredGeometry lowered =
        lower_or_die(arena, records, 0, "annulus lowering failed");
    require(lowered.curves.size() == 4, "annulus lowered wrong curve count");
    for (std::size_t index = 0; index < 4; ++index)
    {
        const bool outer = index < 2;
        require(lowered.curves[index].kind == ExactAtomicCurveKind::circular_arc &&
                    lowered.curves[index].counterclockwise,
                "annulus arcs must stay counterclockwise");
        require(lowered.coverages[index].material_on_left_of_occurrence == outer,
                "annulus material side wrong");
        require_source(lowered.occurrence_sources[index], index + 1, 1000,
                       ExactSourceKind::compact_feature_role,
                       outer ? ExactSourceRole::primitive_outer_circle
                             : ExactSourceRole::primitive_inner_circle,
                       1000, 9200, 0, "annulus source reference wrong");
    }

    const NormalizedRun run = run_union_pipeline(records, 0, 1000, "annulus pipeline");
    require(run.rings.size() == 2 && run.regions.size() == 1 && run.fragment_count == 4,
            "annulus normalized to unexpected topology");
    bool found_hole = false;
    for (const ExactNormalizedResultRing& ring : run.rings)
        if (ring.depth == 1)
            found_hole = !ring.counterclockwise;
    require(found_hole, "annulus lost the clockwise inner ring");
}

void require_multi_operand_occurrence_allocation()
{
    AnalyticRequestPacketRecords records;
    records.jobs = {{10, 0, 1}};
    records.stages = {{100, 1, 0, 2}};
    records.operands = {{1000, 2, 0}, {1001, 3, 0}};
    records.disks = {{9100, 5, 5, 5}};
    records.annuli = {{9200, 40, 40, 5, 10}};

    Budget budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    const AnalyticLoweredGeometry lowered =
        lower_or_die(arena, records, 0, "multi-operand lowering failed");
    require(lowered.curves.size() == 6, "multi-operand lowered wrong curve count");
    const std::uint64_t expected_coverages[6] = {1000, 1000, 1001, 1001, 1001, 1001};
    const bool expected_material[6] = {true, true, true, true, false, false};
    for (std::size_t index = 0; index < 6; ++index)
    {
        require(lowered.coverages[index].occurrence_id == index + 1 &&
                    lowered.coverages[index].coverage_id == expected_coverages[index] &&
                    lowered.coverages[index].material_on_left_of_occurrence ==
                        expected_material[index],
                "multi-operand occurrence allocation wrong");
        require(lowered.occurrence_sources[index].occurrence_id == index + 1,
                "multi-operand source allocation wrong");
    }
}

void require_job_isolation()
{
    AnalyticRequestPacketRecords records;
    records.jobs = {{10, 0, 1}, {20, 1, 1}};
    records.stages = {{100, 1, 0, 1}, {101, 1, 1, 1}};
    records.operands = {{1000, 4, 0}, {1001, 2, 0}};
    records.capsules = {{9300, 0, 0, 0, 0, 50}};
    records.disks = {{9100, 5, 5, 5}};

    Budget budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    AnalyticLoweredGeometryResult capsule_job = lower_analytic_job_geometry(arena, records, 0);
    require(capsule_job.error == AnalyticOperandLoweringError::invalid_topology &&
                !capsule_job.value.has_value(),
            "degenerate capsule job must fail alone with invalid_topology");
    const AnalyticLoweredGeometry disk_job =
        lower_or_die(arena, records, 1, "sibling disk job must survive the capsule failure");
    require(disk_job.curves.size() == 2 && disk_job.coverages[0].coverage_id == 1001 &&
                disk_job.coverages[0].occurrence_id == 1,
            "sibling disk job lowered wrong geometry");
}

void require_invalid_arc_diagnostics()
{
    require_lowering_error(
        single_region_records({{0, 0}, {10, 0}},
                              {{840, 940, 2, 1, false, 4, 0}, {841, 941, 1, 0, false, 0, 0}}),
        AnalyticOperandLoweringError::invalid_arc, "unequal arc radii must report invalid_arc");
    require_lowering_error(single_region_records({{0, 0}, {0, 4}}, {{842, 942, 2, 1, false, 0, 0},
                                                                    {843, 943, 1, 0, false, 0, 0}}),
                           AnalyticOperandLoweringError::invalid_arc,
                           "zero arc radius must report invalid_arc");
    require_lowering_error(
        single_region_records({{-2, 0}, {2, 0}},
                              {{844, 944, 2, 1, true, 0, 0}, {845, 944, 2, 1, false, 0, 0}}),
        AnalyticOperandLoweringError::invalid_arc,
        "major flag contradicting the traversed branch must report invalid_arc");
}

void require_invalid_topology_diagnostics()
{
    require_lowering_error(
        single_region_records({{0, 0}, {0, 0}, {10, 0}, {10, 10}}, {{850, 950, 1, 0, false, 0, 0},
                                                                    {851, 951, 1, 0, false, 0, 0},
                                                                    {852, 952, 1, 0, false, 0, 0},
                                                                    {853, 953, 1, 0, false, 0, 0}}),
        AnalyticOperandLoweringError::invalid_topology,
        "zero-length segment must report invalid_topology");
    require_lowering_error(
        single_region_records({{0, 0}, {10, 0}},
                              {{854, 954, 1, 0, false, 0, 0}, {855, 955, 1, 0, false, 0, 0}}),
        AnalyticOperandLoweringError::invalid_topology,
        "cusp at the minimum vertex must report invalid_topology");
}

void require_span_limit()
{
    constexpr std::int64_t kLimit = 1'000'000'000'000;
    const auto square = [](std::int64_t side)
    {
        return single_region_records({{0, 0}, {side, 0}, {side, side}, {0, side}},
                                     {{860, 960, 1, 0, false, 0, 0},
                                      {861, 961, 1, 0, false, 0, 0},
                                      {862, 962, 1, 0, false, 0, 0},
                                      {863, 963, 1, 0, false, 0, 0}});
    };
    Budget budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    lower_or_die(arena, square(kLimit), 0, "span exactly at the governed limit must lower");
    require_lowering_error(square(kLimit + 1),
                           AnalyticOperandLoweringError::resource_limit_exceeded,
                           "span beyond the governed limit must report resource_limit_exceeded");
}

void require_occurrence_budget()
{
    constexpr std::uint32_t kSegmentCount = 131'073;
    std::vector<std::pair<std::int64_t, std::int64_t>> vertices;
    vertices.reserve(kSegmentCount);
    std::vector<SegmentSpec> segments;
    segments.reserve(kSegmentCount);
    for (std::uint32_t index = 0; index < kSegmentCount; ++index)
    {
        vertices.push_back({static_cast<std::int64_t>(index), index % 2});
        segments.push_back({200'000 + index, 300'000 + index, 1, 0, false, 0, 0});
    }
    require_lowering_error(single_region_records(vertices, segments),
                           AnalyticOperandLoweringError::resource_limit_exceeded,
                           "occurrence budget overflow must report resource_limit_exceeded");
}

void require_capsule_lowering_and_differential()
{
    // Capsule (0,0)->(30,40), width 10: offset vector is exactly (-4, 3).
    const AnalyticRequestPacketRecords records = capsule_records(0, 0, 30, 40, 10);
    Budget budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    const AnalyticLoweredGeometry lowered =
        lower_or_die(arena, records, 0, "capsule lowering failed");
    require(lowered.curves.size() == 4 && lowered.coverages.size() == 4 &&
                lowered.occurrence_sources.size() == 4,
            "capsule lowered wrong element counts");
    const ExactAtomicCurveKind expected_kinds[4] = {
        ExactAtomicCurveKind::line, ExactAtomicCurveKind::circular_arc, ExactAtomicCurveKind::line,
        ExactAtomicCurveKind::circular_arc};
    const bool expected_agrees[4] = {true, true, false, true};
    const ExactSourceRole expected_roles[4] = {
        ExactSourceRole::capsule_right_line, ExactSourceRole::capsule_end_cap,
        ExactSourceRole::capsule_left_line, ExactSourceRole::capsule_start_cap};
    for (std::size_t index = 0; index < 4; ++index)
    {
        const ExactAtomicCurve& curve = lowered.curves[index];
        require(curve.kind == expected_kinds[index] && curve.memberships.size() == 1 &&
                    curve.memberships[0].occurrence_id == index + 1 &&
                    curve.memberships[0].agrees_with_carrier == expected_agrees[index],
                "capsule curve shape wrong");
        if (curve.kind == ExactAtomicCurveKind::circular_arc)
            require(curve.counterclockwise && !curve.major_arc,
                    "capsule caps must be counterclockwise minor semicircles");
        require(lowered.coverages[index].coverage_id == 1000 &&
                    lowered.coverages[index].material_on_left_of_occurrence,
                "capsule coverage wrong");
        require_source(lowered.occurrence_sources[index], index + 1, 1000,
                       ExactSourceKind::compact_feature_role, expected_roles[index], 1000, 9300, 0,
                       "capsule source reference wrong");
    }

    // The same stadium authored as a region ring must normalize identically.
    const AnalyticRequestPacketRecords authored = single_region_records(
        {{4, -3}, {34, 37}, {26, 43}, {-4, 3}}, {{870, 970, 1, 0, false, 0, 0},
                                                 {871, 971, 2, 1, false, 30, 40},
                                                 {872, 972, 1, 0, false, 0, 0},
                                                 {873, 973, 2, 1, false, 0, 0}});
    const NormalizedRun capsule_run = run_union_pipeline(records, 0, 1000, "capsule pipeline");
    const NormalizedRun authored_run =
        run_union_pipeline(authored, 0, 1000, "authored stadium pipeline");
    require(!capsule_run.signature.empty() && capsule_run.signature == authored_run.signature,
            "capsule and authored stadium region normalized differently");
    require(capsule_run.rings.size() == 1 && capsule_run.regions.size() == 1,
            "capsule normalized to unexpected topology");
}

void require_capsule_diagnostics()
{
    require_lowering_error(capsule_records(5, 5, 5, 5, 10),
                           AnalyticOperandLoweringError::invalid_topology,
                           "degenerate capsule endpoints must report invalid_topology");
    // The doubled span accumulator keeps the half-width inflation exact: an
    // even width of 10 lands exactly on the governed limit while width 11
    // overshoots it by one half-unit per side.
    constexpr std::int64_t kFarX = 999'999'999'990;
    Budget budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    lower_or_die(arena, capsule_records(0, 0, kFarX, 0, 10), 0,
                 "capsule span exactly at the governed limit must lower");
    require_lowering_error(capsule_records(0, 0, kFarX, 0, 11),
                           AnalyticOperandLoweringError::resource_limit_exceeded,
                           "capsule half-width span overflow must report resource_limit_exceeded");
}

void require_single_segment_swept_differential()
{
    // One line segment (0,0)->(30,40), width 10: the union boundary is the
    // capsule stadium, but with the disk-seam vertices (-5,0) and (35,40)
    // retained where the cap circles' extreme points survive.
    const AnalyticRequestPacketRecords records =
        swept_records({{0, 0}, {30, 40}}, {{880, 980, 1, 0, false, 0, 0}}, 10);
    Budget budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    const AnalyticLoweredGeometry lowered =
        lower_or_die(arena, records, 0, "single-segment swept lowering failed");
    require(lowered.curves.size() == 6, "single-segment swept wrong curve count");
    require_swept_sources(lowered,
                          {{ExactSourceRole::swept_right_offset_line, std::uint64_t(1) << 32},
                           {ExactSourceRole::swept_left_offset_line, std::uint64_t(1) << 32},
                           {ExactSourceRole::swept_start_cap, std::uint64_t(1) << 32},
                           {ExactSourceRole::swept_end_cap, std::uint64_t(2) << 32}},
                          "single-segment swept");

    const AnalyticRequestPacketRecords authored = single_region_records(
        {{4, -3}, {34, 37}, {35, 40}, {26, 43}, {-4, 3}, {-5, 0}}, {{881, 981, 1, 0, false, 0, 0},
                                                                    {882, 982, 2, 1, false, 30, 40},
                                                                    {883, 982, 2, 1, false, 30, 40},
                                                                    {884, 983, 1, 0, false, 0, 0},
                                                                    {885, 984, 2, 1, false, 0, 0},
                                                                    {886, 984, 2, 1, false, 0, 0}});
    const NormalizedRun swept_run =
        run_union_pipeline(records, 0, 1000, "single-segment swept pipeline");
    const NormalizedRun authored_run =
        run_union_pipeline(authored, 0, 1000, "authored split stadium pipeline");
    require(!swept_run.signature.empty() && swept_run.signature == authored_run.signature,
            "single-segment swept and split stadium normalized differently");
    require(swept_run.rings.size() == 1 && swept_run.regions.size() == 1,
            "single-segment swept normalized to unexpected topology");
}

void require_l_shape_swept_differential()
{
    // Two line segments (0,0)->(20,0)->(20,20), width 10: right angle with a
    // round join at (20,0); the join disk survives only as the outer quarter
    // arc and the inner corner is trimmed at the strip crossing (15,5).
    const AnalyticRequestPacketRecords records =
        swept_records({{0, 0}, {20, 0}, {20, 20}},
                      {{890, 990, 1, 0, false, 0, 0}, {891, 991, 1, 0, false, 0, 0}}, 10);
    Budget budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    const AnalyticLoweredGeometry lowered =
        lower_or_die(arena, records, 0, "l-shape swept lowering failed");
    require(lowered.curves.size() == 8, "l-shape swept wrong curve count");
    require_swept_sources(lowered,
                          {{ExactSourceRole::swept_right_offset_line, std::uint64_t(1) << 32},
                           {ExactSourceRole::swept_left_offset_line, std::uint64_t(1) << 32},
                           {ExactSourceRole::swept_right_offset_line, std::uint64_t(2) << 32},
                           {ExactSourceRole::swept_left_offset_line, std::uint64_t(2) << 32},
                           {ExactSourceRole::swept_round_join, (std::uint64_t(1) << 32) | 2},
                           {ExactSourceRole::swept_start_cap, std::uint64_t(1) << 32},
                           {ExactSourceRole::swept_end_cap, std::uint64_t(3) << 32}},
                          "l-shape swept");

    const AnalyticRequestPacketRecords authored = single_region_records(
        {{0, -5}, {20, -5}, {25, 0}, {25, 20}, {15, 20}, {15, 5}, {0, 5}, {-5, 0}},
        {{920, 1020, 1, 0, false, 0, 0},
         {921, 1021, 2, 1, false, 20, 0},
         {922, 1022, 1, 0, false, 0, 0},
         {923, 1023, 2, 1, false, 20, 20},
         {924, 1024, 1, 0, false, 0, 0},
         {925, 1025, 1, 0, false, 0, 0},
         {926, 1026, 2, 1, false, 0, 0},
         {927, 1026, 2, 1, false, 0, 0}});
    const NormalizedRun swept_run = run_union_pipeline(records, 0, 1000, "l-shape swept pipeline");
    const NormalizedRun authored_run =
        run_union_pipeline(authored, 0, 1000, "authored l-shape pipeline");
    require(!swept_run.signature.empty() && swept_run.signature == authored_run.signature,
            "l-shape swept and authored region normalized differently");
    require(swept_run.rings.size() == 1 && swept_run.regions.size() == 1,
            "l-shape swept normalized to unexpected topology");
}

void require_arc_swept_differential()
{
    // One counterclockwise quarter arc (10,0)->(0,10) around the origin,
    // width 4: the strip is an exact annular sector with radii 8 and 12; the
    // inner offset travels clockwise on the union boundary.
    const AnalyticRequestPacketRecords records =
        swept_records({{10, 0}, {0, 10}}, {{892, 992, 2, 1, false, 0, 0}}, 4);
    Budget budget({2'000'000'000, 268'435'456});
    ConstructionArena arena(budget);
    const AnalyticLoweredGeometry lowered =
        lower_or_die(arena, records, 0, "arc swept lowering failed");
    require(lowered.curves.size() == 5, "arc swept wrong curve count");
    require_swept_sources(lowered,
                          {{ExactSourceRole::swept_left_offset_arc, std::uint64_t(1) << 32},
                           {ExactSourceRole::swept_right_offset_arc, std::uint64_t(1) << 32},
                           {ExactSourceRole::swept_start_cap, std::uint64_t(1) << 32},
                           {ExactSourceRole::swept_end_cap, std::uint64_t(2) << 32}},
                          "arc swept");

    const AnalyticRequestPacketRecords authored = single_region_records(
        {{12, 0}, {0, 12}, {-2, 10}, {0, 8}, {8, 0}}, {{930, 1030, 2, 1, false, 0, 0},
                                                       {931, 1031, 2, 1, false, 0, 10},
                                                       {932, 1031, 2, 1, false, 0, 10},
                                                       {933, 1032, 2, 2, false, 0, 0},
                                                       {934, 1033, 2, 1, false, 10, 0}});
    const NormalizedRun swept_run = run_union_pipeline(records, 0, 1000, "arc swept pipeline");
    const NormalizedRun authored_run =
        run_union_pipeline(authored, 0, 1000, "authored annular sector pipeline");
    require(!swept_run.signature.empty() && swept_run.signature == authored_run.signature,
            "arc swept and authored annular sector normalized differently");
    require(swept_run.rings.size() == 1 && swept_run.regions.size() == 1,
            "arc swept normalized to unexpected topology");
}

void require_swept_overlap_resolution()
{
    // U-shaped centerline with legs sep apart, width 10: sep 8 overlaps the
    // legs in two dimensions (radius-5 cap disks 8 apart cross at rational
    // 3-4-5 points, keeping every pre-arrangement vertex rational), sep 10
    // makes their offset lines exactly coincident with opposite material
    // sides, and sep 20 keeps them apart. All must resolve through the local
    // pre-arrangement into one region.
    const auto records = [](std::int64_t sep)
    {
        return swept_records({{0, 12}, {0, 0}, {sep, 0}, {sep, 12}},
                             {{893, 993, 1, 0, false, 0, 0},
                              {894, 994, 1, 0, false, 0, 0},
                              {895, 995, 1, 0, false, 0, 0}},
                             10);
    };
    const NormalizedRun overlapping =
        run_union_pipeline(records(8), 0, 1000, "overlapping u swept pipeline");
    const NormalizedRun tangent =
        run_union_pipeline(records(10), 0, 1000, "tangent u swept pipeline");
    const NormalizedRun apart =
        run_union_pipeline(records(20), 0, 1000, "separated u swept pipeline");
    require(overlapping.regions.size() == 1 && tangent.regions.size() == 1 &&
                apart.regions.size() == 1,
            "u swept must union into one region");
    require(!overlapping.signature.empty() && overlapping.signature != tangent.signature &&
                tangent.signature != apart.signature && overlapping.signature != apart.signature,
            "u swept separations must normalize distinctly");
}

void require_swept_invalid_inputs()
{
    require_lowering_error(
        swept_records({{0, 0}, {10, 0}, {10, 0}},
                      {{940, 1040, 1, 0, false, 0, 0}, {941, 1041, 1, 0, false, 0, 0}}, 10),
        AnalyticOperandLoweringError::invalid_topology,
        "zero-length swept segment must report invalid_topology");
    require_lowering_error(
        swept_records({{0, 0}, {10, 0}, {5, 0}},
                      {{942, 1042, 1, 0, false, 0, 0}, {943, 1043, 1, 0, false, 0, 0}}, 10),
        AnalyticOperandLoweringError::invalid_topology,
        "swept reversal cusp must report invalid_topology");
    require_lowering_error(swept_records({{0, 0}, {10, 0}, {10, 10}, {5, -5}},
                                         {{944, 1044, 1, 0, false, 0, 0},
                                          {945, 1045, 1, 0, false, 0, 0},
                                          {946, 1046, 1, 0, false, 0, 0}},
                                         2),
                           AnalyticOperandLoweringError::invalid_topology,
                           "self-crossing swept centerline must report invalid_topology");
    require_lowering_error(swept_records({{0, 0}, {20, 0}, {20, 10}, {10, 10}, {10, 0}},
                                         {{947, 1047, 1, 0, false, 0, 0},
                                          {948, 1048, 1, 0, false, 0, 0},
                                          {949, 1049, 1, 0, false, 0, 0},
                                          {950, 1050, 1, 0, false, 0, 0}},
                                         2),
                           AnalyticOperandLoweringError::invalid_topology,
                           "swept centerline endpoint touch must report invalid_topology");
    require_lowering_error(swept_records({{0, 0}, {10, 0}}, {{951, 1051, 2, 1, false, 5, 0}}, 10),
                           AnalyticOperandLoweringError::invalid_arc,
                           "swept arc radius exactly at half the width must report invalid_arc");
}

} // namespace

int main()
{
    require_ccw_square_lowering();
    require_cw_square_material_flip();
    require_winding_insensitive_normalization();
    require_square_with_circular_hole();
    require_arc_leftmost_orientation();
    require_disk_lowering_and_circle_equivalence();
    require_annulus_lowering();
    require_multi_operand_occurrence_allocation();
    require_job_isolation();
    require_invalid_arc_diagnostics();
    require_invalid_topology_diagnostics();
    require_span_limit();
    require_occurrence_budget();
    require_capsule_lowering_and_differential();
    require_capsule_diagnostics();
    require_single_segment_swept_differential();
    require_l_shape_swept_differential();
    require_arc_swept_differential();
    require_swept_overlap_resolution();
    require_swept_invalid_inputs();
    return 0;
}

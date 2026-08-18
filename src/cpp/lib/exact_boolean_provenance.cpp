#include "geometer/exact_boolean_provenance.h"

#include <algorithm>
#include <exception>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace geometer::exact
{
namespace
{

constexpr std::uint64_t kMaximumProvenanceSources = 8'388'608;
constexpr std::uint64_t kMaximumStages = 131'072;
constexpr std::uint64_t kMaximumOperands = 8'388'608;
constexpr std::uint64_t kMaximumFaces = 4'194'304;
constexpr std::uint64_t kMaximumHalfEdges = 16'777'216;
constexpr std::uint64_t kMaximumMemberships = 8'388'608;

std::uint64_t checked_add(std::uint64_t left, std::uint64_t right)
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
        throw std::overflow_error("provenance size addition overflow");
    return left + right;
}

std::uint64_t checked_multiply(std::uint64_t left, std::uint64_t right)
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left)
        throw std::overflow_error("provenance size multiplication overflow");
    return left * right;
}

class StorageReservation
{
  public:
    StorageReservation(Budget& budget, std::uint64_t bytes) : budget_(&budget), bytes_(bytes)
    {
        acquired_ = budget.acquire_storage(bytes);
    }
    ~StorageReservation()
    {
        if (acquired_ && budget_ != nullptr)
            budget_->release_storage(bytes_);
    }
    [[nodiscard]] bool acquired() const
    {
        return acquired_;
    }
    [[nodiscard]] std::uint64_t transfer()
    {
        budget_ = nullptr;
        return bytes_;
    }

  private:
    Budget* budget_ = nullptr;
    std::uint64_t bytes_ = 0;
    bool acquired_ = false;
};

struct OperandMetadata
{
    std::uint64_t coverage_id = 0;
    std::uint64_t operand_id = 0;
    std::uint64_t stage_id = 0;
    ExactBooleanStageOperation operation = ExactBooleanStageOperation::union_;
};

struct ProvenanceInputCounts
{
    std::uint64_t faces = 0;
    std::uint64_t vertices = 0;
    std::uint64_t half_edges = 0;
    std::uint64_t memberships = 0;
    std::uint64_t occurrences = 0;
    std::uint64_t positive_sources = 0;
    std::uint64_t subtraction_sources = 0;
    std::uint64_t operands = 0;
};

ExactBooleanProvenanceResult failure(Error error)
{
    return {error, std::nullopt};
}

auto source_key(const ExactSourceReference& source)
{
    return std::tuple{static_cast<std::uint16_t>(source.kind),
                      static_cast<std::uint16_t>(source.role), source.operand_id, source.primary_id,
                      source.secondary_id};
}

bool source_less(const ExactSourceReference& left, const ExactSourceReference& right)
{
    return source_key(left) < source_key(right);
}

bool source_equal(const ExactSourceReference& left, const ExactSourceReference& right)
{
    return source_key(left) == source_key(right);
}

void insert_unique(std::vector<ExactSourceReference>& values, const ExactSourceReference& value)
{
    const auto position = std::lower_bound(values.begin(), values.end(), value, source_less);
    if (position == values.end() || !source_equal(*position, value))
        values.insert(position, value);
}

void insert_unique_id(std::vector<std::uint64_t>& values, std::uint64_t value)
{
    const auto position = std::lower_bound(values.begin(), values.end(), value);
    if (position == values.end() || *position != value)
        values.insert(position, value);
}

bool role_is_authored(ExactSourceRole role)
{
    return role == ExactSourceRole::authored_line || role == ExactSourceRole::authored_circular_arc;
}

bool role_is_compact_whole_feature(ExactSourceRole role)
{
    return role == ExactSourceRole::primitive_outer_circle ||
           role == ExactSourceRole::primitive_inner_circle ||
           role == ExactSourceRole::capsule_left_line || role == ExactSourceRole::capsule_end_cap ||
           role == ExactSourceRole::capsule_right_line ||
           role == ExactSourceRole::capsule_start_cap;
}

bool role_is_swept(ExactSourceRole role)
{
    return role == ExactSourceRole::swept_left_offset_line ||
           role == ExactSourceRole::swept_left_offset_arc ||
           role == ExactSourceRole::swept_right_offset_line ||
           role == ExactSourceRole::swept_right_offset_arc ||
           role == ExactSourceRole::swept_round_join || role == ExactSourceRole::swept_start_cap ||
           role == ExactSourceRole::swept_end_cap;
}

bool role_matches_curve(ExactSourceRole role, ExactAtomicCurveKind kind)
{
    const bool line_role = role == ExactSourceRole::authored_line ||
                           role == ExactSourceRole::capsule_left_line ||
                           role == ExactSourceRole::capsule_right_line ||
                           role == ExactSourceRole::swept_left_offset_line ||
                           role == ExactSourceRole::swept_right_offset_line;
    const bool arc_role =
        role == ExactSourceRole::authored_circular_arc ||
        role == ExactSourceRole::primitive_outer_circle ||
        role == ExactSourceRole::primitive_inner_circle ||
        role == ExactSourceRole::capsule_end_cap || role == ExactSourceRole::capsule_start_cap ||
        role == ExactSourceRole::swept_left_offset_arc ||
        role == ExactSourceRole::swept_right_offset_arc ||
        role == ExactSourceRole::swept_round_join || role == ExactSourceRole::swept_start_cap ||
        role == ExactSourceRole::swept_end_cap;
    return kind == ExactAtomicCurveKind::line ? line_role : arc_role;
}

bool occurrence_source_valid(const ExactOccurrenceSource& occurrence)
{
    const ExactSourceReference& source = occurrence.source;
    if (occurrence.occurrence_id == 0 || occurrence.coverage_id == 0 || source.operand_id == 0 ||
        source.primary_id == 0)
        return false;
    if (source.kind == ExactSourceKind::authored_segment_curve)
        return role_is_authored(source.role) && source.secondary_id != 0;
    if (source.kind != ExactSourceKind::compact_feature_role)
        return false;
    if (role_is_compact_whole_feature(source.role))
        return source.secondary_id == 0;
    if (!role_is_swept(source.role) || source.secondary_id == 0)
        return false;
    const std::uint64_t incoming = source.secondary_id >> 32U;
    const std::uint64_t outgoing = source.secondary_id & 0xFFFF'FFFFULL;
    if (source.role == ExactSourceRole::swept_round_join)
        return incoming != 0 && outgoing != 0;
    if (source.role == ExactSourceRole::swept_start_cap)
        return incoming == 1 && outgoing == 0;
    return incoming != 0 && outgoing == 0;
}

Error normalize_operands(const std::vector<ExactBooleanStage>& stages,
                         std::vector<OperandMetadata>& by_coverage,
                         std::vector<OperandMetadata>& by_operand)
{
    std::vector<std::uint64_t> stage_ids;
    for (const ExactBooleanStage& stage : stages)
    {
        if (stage.stage_id == 0 || (stage.operation != ExactBooleanStageOperation::union_ &&
                                    stage.operation != ExactBooleanStageOperation::difference))
            return Error::invalid_argument;
        stage_ids.push_back(stage.stage_id);
        for (const ExactBooleanOperand& operand : stage.operands)
        {
            if (operand.coverage_id == 0 || operand.source_id == 0)
                return Error::invalid_argument;
            by_coverage.push_back(
                {operand.coverage_id, operand.source_id, stage.stage_id, stage.operation});
        }
    }
    std::sort(stage_ids.begin(), stage_ids.end());
    if (std::adjacent_find(stage_ids.begin(), stage_ids.end()) != stage_ids.end())
        return Error::invalid_argument;
    std::sort(by_coverage.begin(), by_coverage.end(),
              [](const OperandMetadata& left, const OperandMetadata& right)
              { return left.coverage_id < right.coverage_id; });
    if (std::adjacent_find(by_coverage.begin(), by_coverage.end(),
                           [](const OperandMetadata& left, const OperandMetadata& right)
                           { return left.coverage_id == right.coverage_id; }) != by_coverage.end())
        return Error::invalid_argument;
    by_operand = by_coverage;
    std::sort(by_operand.begin(), by_operand.end(),
              [](const OperandMetadata& left, const OperandMetadata& right)
              { return left.operand_id < right.operand_id; });
    if (std::adjacent_find(by_operand.begin(), by_operand.end(),
                           [](const OperandMetadata& left, const OperandMetadata& right)
                           { return left.operand_id == right.operand_id; }) != by_operand.end())
        return Error::invalid_argument;
    return Error::none;
}

const OperandMetadata* find_coverage(const std::vector<OperandMetadata>& operands,
                                     std::uint64_t coverage_id)
{
    const auto found = std::lower_bound(operands.begin(), operands.end(), coverage_id,
                                        [](const OperandMetadata& operand, std::uint64_t value)
                                        { return operand.coverage_id < value; });
    return found != operands.end() && found->coverage_id == coverage_id ? &*found : nullptr;
}

const OperandMetadata* find_operand(const std::vector<OperandMetadata>& operands,
                                    std::uint64_t operand_id)
{
    const auto found = std::lower_bound(operands.begin(), operands.end(), operand_id,
                                        [](const OperandMetadata& operand, std::uint64_t value)
                                        { return operand.operand_id < value; });
    return found != operands.end() && found->operand_id == operand_id ? &*found : nullptr;
}

const ExactOccurrenceSource* find_occurrence(const std::vector<ExactOccurrenceSource>& occurrences,
                                             std::uint64_t occurrence_id)
{
    const auto found =
        std::lower_bound(occurrences.begin(), occurrences.end(), occurrence_id,
                         [](const ExactOccurrenceSource& occurrence, std::uint64_t value)
                         { return occurrence.occurrence_id < value; });
    return found != occurrences.end() && found->occurrence_id == occurrence_id ? &*found : nullptr;
}

bool face_contains_source(const ExactBooleanSelection& selection, std::uint32_t face,
                          std::uint64_t source_id)
{
    const ExactSelectedFace& selected = selection.faces()[face];
    const auto begin = selection.positive_sources().begin() + selected.positive_source_begin;
    return std::binary_search(begin, begin + selected.positive_source_count, source_id);
}

bool selection_ranges_valid(const ExactBooleanSelection& selection)
{
    std::uint64_t positive_cursor = 0;
    std::uint64_t subtraction_cursor = 0;
    for (const ExactSelectedFace& face : selection.faces())
    {
        const std::uint64_t positive_end =
            static_cast<std::uint64_t>(face.positive_source_begin) + face.positive_source_count;
        const std::uint64_t subtraction_end =
            static_cast<std::uint64_t>(face.subtraction_source_begin) +
            face.subtraction_source_count;
        if (face.positive_source_begin != positive_cursor ||
            face.subtraction_source_begin != subtraction_cursor ||
            positive_end > selection.positive_sources().size() ||
            subtraction_end > selection.subtraction_sources().size())
            return false;
        const auto positive = selection.positive_sources().begin() + face.positive_source_begin;
        const auto subtraction =
            selection.subtraction_sources().begin() + face.subtraction_source_begin;
        if (!std::is_sorted(positive, positive + face.positive_source_count) ||
            !std::is_sorted(subtraction, subtraction + face.subtraction_source_count) ||
            std::adjacent_find(positive, positive + face.positive_source_count) !=
                positive + face.positive_source_count ||
            std::adjacent_find(subtraction, subtraction + face.subtraction_source_count) !=
                subtraction + face.subtraction_source_count)
            return false;
        positive_cursor = positive_end;
        subtraction_cursor = subtraction_end;
    }
    return positive_cursor == selection.positive_sources().size() &&
           subtraction_cursor == selection.subtraction_sources().size();
}

bool face_contains_coverage(const ExactArrangement& arrangement, std::uint32_t face,
                            std::uint64_t coverage_id)
{
    const ExactArrangementFace& value = arrangement.faces()[face];
    const auto begin = arrangement.face_coverages().begin() + value.coverage_begin;
    return std::binary_search(begin, begin + value.coverage_count, coverage_id);
}

bool selection_matches_stages(const ExactArrangement& arrangement,
                              const ExactBooleanSelection& selection,
                              const std::vector<ExactBooleanStage>& stages)
{
    for (std::uint32_t face_id = 0; face_id < selection.faces().size(); ++face_id)
    {
        const ExactArrangementFace& arrangement_face = arrangement.faces()[face_id];
        const std::uint64_t coverage_end =
            static_cast<std::uint64_t>(arrangement_face.coverage_begin) +
            arrangement_face.coverage_count;
        if (coverage_end > arrangement.face_coverages().size())
            return false;
        const auto face_coverages =
            arrangement.face_coverages().begin() + arrangement_face.coverage_begin;
        if (!std::is_sorted(face_coverages, face_coverages + arrangement_face.coverage_count) ||
            std::adjacent_find(face_coverages, face_coverages + arrangement_face.coverage_count) !=
                face_coverages + arrangement_face.coverage_count)
            return false;
        bool material = false;
        std::vector<std::uint64_t> positive;
        std::vector<std::uint64_t> subtraction;
        for (const ExactBooleanStage& stage : stages)
        {
            std::vector<std::uint64_t> covered;
            for (const ExactBooleanOperand& operand : stage.operands)
                if (face_contains_coverage(arrangement, face_id, operand.coverage_id))
                    insert_unique_id(covered, operand.source_id);
            if (covered.empty())
                continue;
            if (stage.operation == ExactBooleanStageOperation::union_)
            {
                material = true;
                for (const std::uint64_t source : covered)
                    insert_unique_id(positive, source);
            }
            else if (material)
            {
                for (const std::uint64_t source : covered)
                    insert_unique_id(subtraction, source);
                material = false;
                positive.clear();
            }
        }
        const ExactSelectedFace& actual = selection.faces()[face_id];
        const auto actual_positive =
            selection.positive_sources().begin() + actual.positive_source_begin;
        const auto actual_subtraction =
            selection.subtraction_sources().begin() + actual.subtraction_source_begin;
        if (actual.material != material || actual.positive_source_count != positive.size() ||
            actual.subtraction_source_count != subtraction.size() ||
            !std::equal(positive.begin(), positive.end(), actual_positive) ||
            !std::equal(subtraction.begin(), subtraction.end(), actual_subtraction))
            return false;
    }
    return true;
}

Error collect_retained_boundary(const ExactArrangement& arrangement,
                                const ExactBooleanSelection& selection,
                                const ExactBooleanRegions& regions,
                                std::vector<bool>& retained_half_edges,
                                std::vector<bool>& retained_vertices)
{
    std::uint64_t ring_cursor = 0;
    for (const ExactResultRing& ring : regions.rings())
    {
        const std::uint64_t end =
            static_cast<std::uint64_t>(ring.half_edge_begin) + ring.half_edge_count;
        if (ring.half_edge_begin != ring_cursor || end > regions.ring_half_edges().size() ||
            ring.half_edge_count == 0)
            return Error::invalid_argument;
        for (std::uint32_t index = 0; index < ring.half_edge_count; ++index)
        {
            const std::uint32_t half_edge = regions.ring_half_edges()[ring.half_edge_begin + index];
            if (half_edge >= retained_half_edges.size() || retained_half_edges[half_edge])
                return Error::invalid_argument;
            retained_half_edges[half_edge] = true;
        }
        ring_cursor = end;
    }
    if (ring_cursor != regions.ring_half_edges().size())
        return Error::invalid_argument;

    for (std::uint32_t id = 0; id < arrangement.half_edges().size(); ++id)
    {
        const ExactArrangementHalfEdge& half_edge = arrangement.half_edges()[id];
        if (half_edge.twin >= arrangement.half_edges().size() ||
            half_edge.edge >= arrangement.edges().size() ||
            half_edge.origin_vertex >= arrangement.vertices().size() ||
            half_edge.face >= arrangement.faces().size())
            return Error::invalid_argument;
        const ExactArrangementHalfEdge& twin = arrangement.half_edges()[half_edge.twin];
        if (twin.origin_vertex >= arrangement.vertices().size() ||
            twin.face >= arrangement.faces().size())
            return Error::invalid_argument;
        const bool expected =
            selection.faces()[half_edge.face].material && !selection.faces()[twin.face].material;
        if (retained_half_edges[id] != expected)
            return Error::invalid_argument;
        if (expected)
        {
            retained_vertices[half_edge.origin_vertex] = true;
            retained_vertices[twin.origin_vertex] = true;
        }
    }
    return Error::none;
}

Error collect_input_counts(const ExactArrangement& arrangement,
                           const ExactBooleanSelection& selection,
                           const std::vector<ExactBooleanStage>& stages,
                           const std::vector<ExactOccurrenceSource>& occurrences,
                           ProvenanceInputCounts& counts)
{
    counts.faces = arrangement.faces().size();
    counts.vertices = arrangement.vertices().size();
    counts.half_edges = arrangement.half_edges().size();
    counts.memberships = arrangement.memberships().size();
    counts.occurrences = occurrences.size();
    counts.positive_sources = selection.positive_sources().size();
    counts.subtraction_sources = selection.subtraction_sources().size();
    for (const ExactBooleanStage& stage : stages)
        counts.operands = checked_add(counts.operands, stage.operands.size());
    if (selection.faces().size() != counts.faces || counts.faces == 0 ||
        !selection_ranges_valid(selection))
        return Error::invalid_argument;
    if (counts.faces > kMaximumFaces || counts.vertices > kMaximumHalfEdges ||
        counts.half_edges > kMaximumHalfEdges || counts.memberships > kMaximumMemberships ||
        stages.size() > kMaximumStages || counts.operands > kMaximumOperands ||
        counts.positive_sources > kMaximumProvenanceSources ||
        counts.subtraction_sources > kMaximumProvenanceSources ||
        counts.occurrences > kMaximumProvenanceSources ||
        counts.half_edges > std::numeric_limits<std::uint32_t>::max() ||
        counts.vertices > std::numeric_limits<std::uint32_t>::max())
        return Error::resource_limit_exceeded;
    return Error::none;
}

Error append_sources(std::vector<ExactSourceReference>& output,
                     const std::vector<ExactSourceReference>& values, std::uint32_t& begin,
                     std::uint32_t& count)
{
    if (output.size() > std::numeric_limits<std::uint32_t>::max() ||
        values.size() > std::numeric_limits<std::uint32_t>::max() ||
        output.size() > kMaximumProvenanceSources ||
        values.size() > kMaximumProvenanceSources - output.size())
        return Error::resource_limit_exceeded;
    begin = static_cast<std::uint32_t>(output.size());
    count = static_cast<std::uint32_t>(values.size());
    output.insert(output.end(), values.begin(), values.end());
    return Error::none;
}

} // namespace

ExactBooleanProvenance::ExactBooleanProvenance(
    Budget& budget, std::uint64_t charged_bytes,
    std::vector<ExactBoundaryFragmentProvenance> fragments,
    std::vector<ExactResultVertexProvenance> vertices,
    std::vector<ExactSourceReference> source_references) noexcept
    : budget_(&budget), charged_bytes_(charged_bytes), fragments_(std::move(fragments)),
      vertices_(std::move(vertices)), source_references_(std::move(source_references))
{
}

ExactBooleanProvenance::~ExactBooleanProvenance()
{
    release();
}

ExactBooleanProvenance::ExactBooleanProvenance(ExactBooleanProvenance&& other) noexcept
    : budget_(std::exchange(other.budget_, nullptr)),
      charged_bytes_(std::exchange(other.charged_bytes_, 0)),
      fragments_(std::move(other.fragments_)), vertices_(std::move(other.vertices_)),
      source_references_(std::move(other.source_references_))
{
}

ExactBooleanProvenance& ExactBooleanProvenance::operator=(ExactBooleanProvenance&& other) noexcept
{
    if (this != &other)
    {
        release();
        budget_ = std::exchange(other.budget_, nullptr);
        charged_bytes_ = std::exchange(other.charged_bytes_, 0);
        fragments_ = std::move(other.fragments_);
        vertices_ = std::move(other.vertices_);
        source_references_ = std::move(other.source_references_);
    }
    return *this;
}

void ExactBooleanProvenance::release()
{
    if (budget_ != nullptr)
        budget_->release_storage(charged_bytes_);
    budget_ = nullptr;
    charged_bytes_ = 0;
}

const std::vector<ExactBoundaryFragmentProvenance>& ExactBooleanProvenance::fragments() const
{
    return fragments_;
}

const std::vector<ExactResultVertexProvenance>& ExactBooleanProvenance::vertices() const
{
    return vertices_;
}

const std::vector<ExactSourceReference>& ExactBooleanProvenance::source_references() const
{
    return source_references_;
}

ExactBooleanProvenanceResult build_exact_boolean_provenance(
    Budget& budget, const ExactArrangement& arrangement, const ExactBooleanSelection& selection,
    const ExactBooleanRegions& regions, const std::vector<ExactBooleanStage>& stages,
    const std::vector<ExactOccurrenceSource>& input_occurrences)
{
    try
    {
        ProvenanceInputCounts counts;
        if (const Error error =
                collect_input_counts(arrangement, selection, stages, input_occurrences, counts);
            error != Error::none)
            return failure(error);
        const std::uint64_t face_count = counts.faces;
        const std::uint64_t vertex_count = counts.vertices;
        const std::uint64_t half_edge_count = counts.half_edges;
        const std::uint64_t membership_count = counts.memberships;
        const std::uint64_t occurrence_count = counts.occurrences;
        const std::uint64_t subtraction_count = counts.subtraction_sources;
        const std::uint64_t maximum_sources =
            std::min(kMaximumProvenanceSources,
                     checked_add(checked_multiply(half_edge_count,
                                                  checked_add(occurrence_count, subtraction_count)),
                                 checked_multiply(vertex_count, occurrence_count)));
        const std::uint64_t charge = checked_add(
            4096,
            checked_add(
                checked_multiply(half_edge_count, 32),
                checked_add(
                    checked_multiply(vertex_count, 24),
                    checked_add(checked_multiply(occurrence_count, 64),
                                checked_add(checked_multiply(counts.operands, 64),
                                            checked_add(checked_multiply(membership_count, 8),
                                                        checked_multiply(maximum_sources, 32)))))));
        const std::uint64_t work = checked_add(
            256, checked_add(
                     checked_multiply(stages.size(), 16),
                     checked_add(
                         checked_multiply(counts.operands, 32),
                         checked_add(
                             checked_multiply(half_edge_count, 16),
                             checked_add(
                                 checked_multiply(vertex_count, 16),
                                 checked_add(
                                     checked_multiply(membership_count, 8),
                                     checked_add(checked_multiply(occurrence_count, 32),
                                                 checked_add(checked_multiply(maximum_sources, 4),
                                                             checked_multiply(
                                                                 checked_multiply(face_count,
                                                                                  counts.operands),
                                                                 4)))))))));
        if (!budget.consume_work(work))
            return failure(Error::resource_limit_exceeded);
        StorageReservation reservation(budget, charge);
        if (!reservation.acquired())
            return failure(Error::resource_limit_exceeded);

        std::vector<OperandMetadata> by_coverage;
        std::vector<OperandMetadata> by_operand;
        if (const Error error = normalize_operands(stages, by_coverage, by_operand);
            error != Error::none)
            return failure(error);
        if (!selection_matches_stages(arrangement, selection, stages))
            return failure(Error::invalid_argument);

        std::vector<ExactOccurrenceSource> occurrences = input_occurrences;
        std::sort(occurrences.begin(), occurrences.end(),
                  [](const ExactOccurrenceSource& left, const ExactOccurrenceSource& right)
                  { return left.occurrence_id < right.occurrence_id; });
        for (std::size_t index = 0; index < occurrences.size(); ++index)
        {
            const ExactOccurrenceSource& occurrence = occurrences[index];
            const OperandMetadata* operand = find_coverage(by_coverage, occurrence.coverage_id);
            if (!occurrence_source_valid(occurrence) || operand == nullptr ||
                operand->operand_id != occurrence.source.operand_id ||
                (index != 0 && occurrences[index - 1].occurrence_id == occurrence.occurrence_id))
                return failure(Error::invalid_argument);
        }
        std::vector<std::uint64_t> used_occurrences;
        used_occurrences.reserve(arrangement.memberships().size());
        for (const ExactArrangementEdge& edge : arrangement.edges())
        {
            const std::uint64_t membership_end =
                static_cast<std::uint64_t>(edge.membership_begin) + edge.membership_count;
            if (membership_end > arrangement.memberships().size() || edge.membership_count == 0)
                return failure(Error::invalid_argument);
            for (std::uint32_t index = 0; index < edge.membership_count; ++index)
            {
                const ExactCurveMembership& membership =
                    arrangement.memberships()[edge.membership_begin + index];
                const ExactOccurrenceSource* occurrence =
                    find_occurrence(occurrences, membership.occurrence_id);
                if (occurrence == nullptr ||
                    !role_matches_curve(occurrence->source.role, edge.kind))
                    return failure(Error::invalid_argument);
                used_occurrences.push_back(membership.occurrence_id);
            }
        }
        std::sort(used_occurrences.begin(), used_occurrences.end());
        used_occurrences.erase(std::unique(used_occurrences.begin(), used_occurrences.end()),
                               used_occurrences.end());
        if (used_occurrences.size() != occurrences.size())
            return failure(Error::invalid_argument);
        for (std::size_t index = 0; index < occurrences.size(); ++index)
            if (used_occurrences[index] != occurrences[index].occurrence_id)
                return failure(Error::invalid_argument);

        std::vector<bool> retained_half_edges(half_edge_count);
        std::vector<bool> retained_vertices(vertex_count);
        if (const Error error = collect_retained_boundary(arrangement, selection, regions,
                                                          retained_half_edges, retained_vertices);
            error != Error::none)
            return failure(error);

        std::vector<ExactBoundaryFragmentProvenance> fragments;
        std::vector<ExactResultVertexProvenance> vertices;
        std::vector<ExactSourceReference> source_references;
        for (std::uint32_t half_edge_id = 0; half_edge_id < half_edge_count; ++half_edge_id)
        {
            if (!retained_half_edges[half_edge_id])
                continue;
            const ExactArrangementHalfEdge& half_edge = arrangement.half_edges()[half_edge_id];
            const ExactArrangementEdge& edge = arrangement.edges()[half_edge.edge];
            const std::uint32_t empty_face = arrangement.half_edges()[half_edge.twin].face;
            std::vector<ExactSourceReference> positive;
            std::vector<ExactSourceReference> subtraction;
            for (std::uint32_t index = 0; index < edge.membership_count; ++index)
            {
                const ExactCurveMembership& membership =
                    arrangement.memberships()[edge.membership_begin + index];
                const ExactOccurrenceSource* occurrence =
                    find_occurrence(occurrences, membership.occurrence_id);
                if (occurrence == nullptr)
                    return failure(Error::invalid_argument);
                const OperandMetadata* operand =
                    find_coverage(by_coverage, occurrence->coverage_id);
                if (operand == nullptr)
                    return failure(Error::invalid_argument);
                if (operand->operation == ExactBooleanStageOperation::union_ &&
                    face_contains_source(selection, half_edge.face, operand->operand_id))
                    insert_unique(positive, occurrence->source);
            }
            const ExactSelectedFace& empty = selection.faces()[empty_face];
            for (std::uint32_t index = 0; index < empty.subtraction_source_count; ++index)
            {
                const std::uint64_t operand_id =
                    selection.subtraction_sources()[empty.subtraction_source_begin + index];
                const OperandMetadata* operand = find_operand(by_operand, operand_id);
                if (operand == nullptr ||
                    operand->operation != ExactBooleanStageOperation::difference)
                    return failure(Error::invalid_argument);
                insert_unique(subtraction,
                              {ExactSourceKind::subtractive_operand_effect, ExactSourceRole::none,
                               operand_id, operand->stage_id, 0});
            }
            ExactBoundaryFragmentProvenance projected;
            projected.half_edge = half_edge_id;
            if (const Error error =
                    append_sources(source_references, positive, projected.positive_source_begin,
                                   projected.positive_source_count);
                error != Error::none)
                return failure(error);
            if (const Error error = append_sources(source_references, subtraction,
                                                   projected.subtraction_source_begin,
                                                   projected.subtraction_source_count);
                error != Error::none)
                return failure(error);
            fragments.push_back(projected);
        }

        for (std::uint32_t vertex_id = 0; vertex_id < vertex_count; ++vertex_id)
        {
            if (!retained_vertices[vertex_id])
                continue;
            const ExactArrangementVertex& vertex = arrangement.vertices()[vertex_id];
            const std::uint64_t outgoing_end =
                static_cast<std::uint64_t>(vertex.outgoing_begin) + vertex.outgoing_count;
            if (outgoing_end > arrangement.outgoing_half_edges().size())
                return failure(Error::invalid_argument);
            std::vector<ExactSourceReference> incident;
            for (std::uint32_t index = 0; index < vertex.outgoing_count; ++index)
            {
                const std::uint32_t half_edge =
                    arrangement.outgoing_half_edges()[vertex.outgoing_begin + index];
                if (half_edge >= half_edge_count)
                    return failure(Error::invalid_argument);
                const ExactArrangementEdge& edge =
                    arrangement.edges()[arrangement.half_edges()[half_edge].edge];
                for (std::uint32_t membership_index = 0; membership_index < edge.membership_count;
                     ++membership_index)
                {
                    const ExactCurveMembership& membership =
                        arrangement.memberships()[edge.membership_begin + membership_index];
                    const ExactOccurrenceSource* occurrence =
                        find_occurrence(occurrences, membership.occurrence_id);
                    if (occurrence == nullptr)
                        return failure(Error::invalid_argument);
                    insert_unique(incident, occurrence->source);
                }
            }
            ExactResultVertexProvenance projected;
            projected.arrangement_vertex = vertex_id;
            if (const Error error = append_sources(source_references, incident,
                                                   projected.source_begin, projected.source_count);
                error != Error::none)
                return failure(error);
            vertices.push_back(projected);
        }

        const std::uint64_t transferred = reservation.transfer();
        return {Error::none,
                ExactBooleanProvenance(budget, transferred, std::move(fragments),
                                       std::move(vertices), std::move(source_references))};
    }
    catch (const std::exception&)
    {
        return failure(Error::resource_limit_exceeded);
    }
}

} // namespace geometer::exact

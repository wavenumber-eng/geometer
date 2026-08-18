#pragma once

#include "geometer/analytic_source_reference.h"
#include "geometer/exact_boolean_regions.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace geometer::exact
{

using ExactSourceKind = AnalyticSourceKind;
using ExactSourceRole = AnalyticSourceRole;
using ExactSourceReference = AnalyticSourceReference;

struct ExactOccurrenceSource
{
    std::uint64_t occurrence_id = 0;
    std::uint64_t coverage_id = 0;
    ExactSourceReference source;
};

struct ExactBoundaryFragmentProvenance
{
    std::uint32_t half_edge = 0;
    std::uint32_t positive_source_begin = 0;
    std::uint32_t positive_source_count = 0;
    std::uint32_t subtraction_source_begin = 0;
    std::uint32_t subtraction_source_count = 0;
};

struct ExactResultVertexProvenance
{
    std::uint32_t arrangement_vertex = 0;
    std::uint32_t source_begin = 0;
    std::uint32_t source_count = 0;
};

struct ExactBooleanProvenanceResult;

class ExactBooleanProvenance
{
  public:
    ~ExactBooleanProvenance();
    ExactBooleanProvenance(ExactBooleanProvenance&& other) noexcept;
    ExactBooleanProvenance& operator=(ExactBooleanProvenance&& other) noexcept;
    ExactBooleanProvenance(const ExactBooleanProvenance&) = delete;
    ExactBooleanProvenance& operator=(const ExactBooleanProvenance&) = delete;

    [[nodiscard]] const std::vector<ExactBoundaryFragmentProvenance>& fragments() const;
    [[nodiscard]] const std::vector<ExactResultVertexProvenance>& vertices() const;
    [[nodiscard]] const std::vector<ExactSourceReference>& source_references() const;

  private:
    friend struct ExactBooleanProvenanceResult;
    friend ExactBooleanProvenanceResult build_exact_boolean_provenance(
        Budget&, const ExactArrangement&, const ExactBooleanSelection&, const ExactBooleanRegions&,
        const std::vector<ExactBooleanStage>&, const std::vector<ExactOccurrenceSource>&);
    ExactBooleanProvenance(Budget& budget, std::uint64_t charged_bytes,
                           std::vector<ExactBoundaryFragmentProvenance> fragments,
                           std::vector<ExactResultVertexProvenance> vertices,
                           std::vector<ExactSourceReference> source_references) noexcept;
    void release();

    Budget* budget_ = nullptr;
    std::uint64_t charged_bytes_ = 0;
    std::vector<ExactBoundaryFragmentProvenance> fragments_;
    std::vector<ExactResultVertexProvenance> vertices_;
    std::vector<ExactSourceReference> source_references_;
};

struct ExactBooleanProvenanceResult
{
    Error error = Error::none;
    std::optional<ExactBooleanProvenance> value;
};

[[nodiscard]] ExactBooleanProvenanceResult build_exact_boolean_provenance(
    Budget& budget, const ExactArrangement& arrangement, const ExactBooleanSelection& selection,
    const ExactBooleanRegions& regions, const std::vector<ExactBooleanStage>& stages,
    const std::vector<ExactOccurrenceSource>& occurrence_sources);

} // namespace geometer::exact

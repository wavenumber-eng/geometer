#pragma once

#include "geometer/exact_boolean_provenance.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace geometer::exact
{

struct ExactSourceSetRecord
{
    std::uint32_t source_reference_index_begin = 0;
    std::uint32_t source_reference_index_count = 0;
};

struct ExactSourceSetTableResult;

class ExactSourceSetTable
{
  public:
    ~ExactSourceSetTable();
    ExactSourceSetTable(ExactSourceSetTable&& other) noexcept;
    ExactSourceSetTable& operator=(ExactSourceSetTable&& other) noexcept;
    ExactSourceSetTable(const ExactSourceSetTable&) = delete;
    ExactSourceSetTable& operator=(const ExactSourceSetTable&) = delete;

    [[nodiscard]] const std::vector<ExactSourceReference>& source_references() const;
    [[nodiscard]] const std::vector<ExactSourceSetRecord>& source_sets() const;
    [[nodiscard]] const std::vector<std::uint32_t>& source_reference_indices() const;
    [[nodiscard]] const std::vector<std::uint32_t>& input_handles() const;

  private:
    friend struct ExactSourceSetTableResult;
    friend ExactSourceSetTableResult
    build_exact_source_sets(Budget&, const std::vector<std::vector<ExactSourceReference>>&);
    ExactSourceSetTable(Budget& budget, std::uint64_t charged_bytes,
                        std::vector<ExactSourceReference> source_references,
                        std::vector<ExactSourceSetRecord> source_sets,
                        std::vector<std::uint32_t> source_reference_indices,
                        std::vector<std::uint32_t> input_handles) noexcept;
    void release();

    Budget* budget_ = nullptr;
    std::uint64_t charged_bytes_ = 0;
    std::vector<ExactSourceReference> source_references_;
    std::vector<ExactSourceSetRecord> source_sets_;
    std::vector<std::uint32_t> source_reference_indices_;
    std::vector<std::uint32_t> input_handles_;
};

struct ExactSourceSetTableResult
{
    Error error = Error::none;
    std::optional<ExactSourceSetTable> value;
};

[[nodiscard]] ExactSourceSetTableResult
build_exact_source_sets(Budget& budget,
                        const std::vector<std::vector<ExactSourceReference>>& input_sets);

} // namespace geometer::exact

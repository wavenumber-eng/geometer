#pragma once

#include "geometer/exact_construction.h"

#include <memory>

namespace geometer::exact
{

struct DecodeConstructionArtifactResult;

[[nodiscard]] EncodeResult
encode_construction_artifact(Budget& budget, const ConstructionArena& arena,
                             const std::vector<ConstructionNodeId>& ordered_roots);

class DecodedConstructionArtifact
{
  public:
    ~DecodedConstructionArtifact();
    DecodedConstructionArtifact(DecodedConstructionArtifact&& other) noexcept;
    DecodedConstructionArtifact& operator=(DecodedConstructionArtifact&& other) noexcept;
    DecodedConstructionArtifact(const DecodedConstructionArtifact&) = delete;
    DecodedConstructionArtifact& operator=(const DecodedConstructionArtifact&) = delete;

    [[nodiscard]] const ConstructionArena& arena() const;
    [[nodiscard]] const std::vector<ConstructionNodeId>& roots() const;

  private:
    friend DecodeConstructionArtifactResult
    decode_construction_artifact(Budget&, const std::vector<std::uint8_t>&);
    DecodedConstructionArtifact(Budget& budget, std::uint64_t charged_bytes,
                                std::unique_ptr<ConstructionArena> arena,
                                std::vector<ConstructionNodeId> roots);

    Budget* budget_ = nullptr;
    std::uint64_t charged_bytes_ = 0;
    std::unique_ptr<ConstructionArena> arena_;
    std::vector<ConstructionNodeId> roots_;
};

struct DecodeConstructionArtifactResult
{
    Error error = Error::none;
    std::optional<DecodedConstructionArtifact> value;
};

[[nodiscard]] DecodeConstructionArtifactResult
decode_construction_artifact(Budget& budget, const std::vector<std::uint8_t>& bytes);

} // namespace geometer::exact

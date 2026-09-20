#pragma once

#include "geometer/generated/contracts/contracts.h"
#include "geometer/status.h"

#include <optional>
#include <string>
#include <vector>

namespace geometer
{

struct PreparedIllustrationFragment
{
    contracts::MeshCollectionA0 collection;
    contracts::FragmentMetadata metadata;
    bool empty = true;
    std::optional<contracts::ModelIllustrationBounds3MmA0> bounds_mm;
};

/// Bake mesh and optional global affine transforms, canonicalize world-space values,
/// and clip every triangle against the ordered B0 half-spaces. Empty output is success.
int prepare_illustration_fragment(std::vector<contracts::MeshIllustrationMesh> meshes,
                                  const std::optional<contracts::IllustrationClipping>& clipping,
                                  const std::optional<std::vector<double>>& global_transform,
                                  const std::string& raw_attachment_sha256,
                                  PreparedIllustrationFragment* result, Status* status = nullptr);

} // namespace geometer

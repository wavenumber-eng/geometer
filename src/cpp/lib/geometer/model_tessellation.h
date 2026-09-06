#pragma once

#include "geometer/generated/contracts/contracts.h"
#include "geometer/status.h"

#include <cstddef>

namespace geometer
{

/// Stateless colored STEP tessellation into the shared millimeter mesh contract.
/// Root placement is explicit; assembly placement and surface colors are retained.
int model_tessellation_from_bytes(const unsigned char* data, std::size_t size,
                                  const contracts::ModelTessellationRequestA0& options,
                                  contracts::MeshCollectionA0* meshes, Status* status = nullptr);

} // namespace geometer

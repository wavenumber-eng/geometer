#pragma once

#include "geometer/fast_hlr.h"

namespace geometer::fast_hlr_internal
{

// OCCT preparation has already welded vertices within topological components.
// Preserve those component boundaries when constructing adjacency.
int prepare_indexed_mesh_preserving_vertices(const FastHlrIndexedMesh& mesh,
                                             const FastHlrOptions& options,
                                             FastHlrPreparedMesh* prepared,
                                             Status* status = nullptr);

} // namespace geometer::fast_hlr_internal

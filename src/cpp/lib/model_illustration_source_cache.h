#pragma once

#include "geometer/model_tessellation.h"

#include <cstddef>

namespace geometer::model_illustration_detail
{

// Private native illustration preparation optimization (64 entries / 256 MiB
// retained by default). WASM delegates statelessly to the original tessellator.
// No OCCT document or mutable mesh is shared: each successful native caller
// receives its own collection copy.
int tessellate_illustration_source(const unsigned char* data, std::size_t size,
                                   const contracts::ModelTessellationRequestA0& options,
                                   contracts::MeshCollectionA0* collection, Status* status,
                                   std::vector<std::string>* warnings,
                                   std::string* source_sha256 = nullptr);

struct SourceCacheLimits
{
    std::size_t max_entries = 64;
    std::size_t max_bytes = 256 * 1024 * 1024;
};

struct SourceCacheStats
{
    std::size_t hits = 0;
    std::size_t misses = 0;
    std::size_t entries = 0;
    std::size_t retained_bytes = 0;
    std::size_t insertions = 0;
    std::size_t evictions = 0;
    std::size_t bypasses = 0;
};

// Private native-test seam, not exported through an SDK or wire contract.
void reset_illustration_source_cache_for_tests(SourceCacheLimits limits = {});
SourceCacheStats illustration_source_cache_stats_for_tests();

// Saturating retained-storage accounting, including capacities and conservative
// allocation overhead. It is not an OCCT/transient-process memory quota.
std::size_t illustration_source_cache_owned_bytes(std::size_t raw_capacity,
                                                  const std::string& options,
                                                  const contracts::MeshCollectionA0& collection,
                                                  const std::vector<std::string>& warnings);

} // namespace geometer::model_illustration_detail

#include "model_illustration_source_cache.h"

#include "geometer/sha256.h"

#include <array>
#include <cstring>
#include <list>
#include <memory>
#include <mutex>
#include <new>
#include <stdexcept>
#include <utility>

namespace geometer::model_illustration_detail
{
namespace
{
#ifndef __EMSCRIPTEN__
constexpr std::size_t maximum_source_bytes = 268435456;
#endif

#ifndef __EMSCRIPTEN__
std::string digest_hex(const std::array<std::uint8_t, 32>& digest)
{
    constexpr char digits[] = "0123456789abcdef";
    std::string text(64, '0');
    for (std::size_t index = 0; index < digest.size(); ++index)
    {
        text[index * 2] = digits[digest[index] >> 4];
        text[index * 2 + 1] = digits[digest[index] & 15];
    }
    return text;
}
#endif

struct Entry
{
    std::array<std::uint8_t, 32> digest{};
    std::vector<unsigned char> source;
    std::string options;
    contracts::MeshCollectionA0 collection;
    std::vector<std::string> warnings;
    std::size_t bytes = 0;
};

struct Cache
{
    std::mutex mutex;
    // This phase qualifies native process-local reuse only. WASM keeps its
    // prior stateless tessellation behavior and retains no source meshes.
#ifdef __EMSCRIPTEN__
    SourceCacheLimits limits{0, 0};
#else
    SourceCacheLimits limits;
#endif
    SourceCacheStats stats;
    std::list<std::shared_ptr<const Entry>> entries;
    std::size_t generation = 0;
};

Cache& cache()
{
    static Cache value;
    return value;
}

#ifndef __EMSCRIPTEN__
bool matches(const Entry& entry, const unsigned char* data, std::size_t size,
             const std::string& options)
{
    // Exact bytes remain the equality proof. A successful hit also authenticates
    // the retained raw-byte SHA without repeating a full cryptographic scan.
    return entry.options == options && entry.source.size() == size &&
           std::memcmp(entry.source.data(), data, size) == 0;
}

std::shared_ptr<const Entry> lookup(const unsigned char* data, std::size_t size,
                                    const std::string& options, std::size_t* generation)
{
    auto& state = cache();
    std::lock_guard<std::mutex> lock(state.mutex);
    *generation = state.generation;
    for (auto iterator = state.entries.begin(); iterator != state.entries.end(); ++iterator)
    {
        if (!matches(**iterator, data, size, options))
            continue;
        auto result = *iterator;
        state.entries.splice(state.entries.begin(), state.entries, iterator);
        ++state.stats.hits;
        return result;
    }
    ++state.stats.misses;
    return {};
}

void bypass(std::size_t generation)
{
    auto& state = cache();
    std::lock_guard<std::mutex> lock(state.mutex);
    if (generation == state.generation)
        ++state.stats.bypasses;
}

bool retainable(std::size_t bytes, std::size_t generation)
{
    auto& state = cache();
    std::lock_guard<std::mutex> lock(state.mutex);
    return generation == state.generation && state.limits.max_entries != 0 &&
           bytes <= state.limits.max_bytes;
}

void insert(std::shared_ptr<const Entry> entry, std::size_t generation)
{
    auto& state = cache();
    std::lock_guard<std::mutex> lock(state.mutex);
    if (generation != state.generation)
        return;
    if (state.limits.max_entries == 0 || entry->bytes > state.limits.max_bytes)
    {
        ++state.stats.bypasses;
        return;
    }
    for (const auto& existing : state.entries)
        if (matches(*existing, entry->source.data(), entry->source.size(), entry->options))
            return; // Concurrent successful miss already retained this exact input.
    // Allocate the new list node before eviction, so failed retention does not
    // destroy previously useful entries. No cache lock spans OCCT work.
    state.entries.push_front(std::move(entry));
    const auto bytes = state.entries.front()->bytes;
    while (state.entries.size() > state.limits.max_entries ||
           bytes > state.limits.max_bytes - state.stats.retained_bytes)
    {
        state.stats.retained_bytes -= state.entries.back()->bytes;
        state.entries.pop_back();
        ++state.stats.evictions;
    }
    state.stats.retained_bytes += bytes;
    ++state.stats.insertions;
}

void retain(const unsigned char* data, std::size_t size, std::string options,
            const std::array<std::uint8_t, 32>& digest,
            const contracts::MeshCollectionA0& collection, const std::vector<std::string>& warnings,
            std::size_t generation)
{
    if (!retainable(illustration_source_cache_owned_bytes(size, options, collection, warnings),
                    generation))
    {
        bypass(generation);
        return;
    }
    try
    {
        auto entry = std::make_shared<Entry>();
        entry->digest = digest;
        entry->source.assign(data, data + size);
        entry->options = std::move(options);
        entry->collection = collection;
        entry->warnings = warnings;
        entry->bytes = illustration_source_cache_owned_bytes(
            entry->source.capacity(), entry->options, entry->collection, entry->warnings);
        insert(std::move(entry), generation);
    }
    catch (const std::bad_alloc&)
    {
        bypass(generation); // Cache retention must not discard successful geometry.
    }
    catch (const std::length_error&)
    {
        bypass(generation);
    }
}
#endif
} // namespace

int tessellate_illustration_source(const unsigned char* data, std::size_t size,
                                   const contracts::ModelTessellationRequestA0& options,
                                   contracts::MeshCollectionA0* collection, Status* status,
                                   std::vector<std::string>* warnings, std::string* source_sha256)
{
    if (source_sha256)
        source_sha256->clear();
    const auto uncached = [&]()
    {
        const int code =
            model_tessellation_from_bytes(data, size, options, collection, status, warnings);
        if (code == 0 && source_sha256)
            *source_sha256 = sha256_hex(data, size);
        return code;
    };
#ifdef __EMSCRIPTEN__
    return uncached();
#else
    // Preserve the original validation order, warning clearing and errors.
    if (!collection || !data || size == 0 || size > maximum_source_bytes)
        return uncached();
    std::string encoded;
    bool valid = false;
    try
    {
        contracts::ContractError error;
        valid = contracts::encode_json(options, &encoded, &error);
    }
    catch (const std::exception&)
    {
        // Validation/allocation failure is handled by the unchanged native
        // tessellator's existing exception boundary, never a cache success.
    }
    if (!valid)
        return uncached();
    std::size_t generation = 0;
    const auto hit = lookup(data, size, encoded, &generation);
    if (hit)
    {
        try
        {
            auto copy = hit->collection;
            if (warnings)
                *warnings = hit->warnings;
            if (source_sha256)
                *source_sha256 = digest_hex(hit->digest);
            *collection = std::move(copy);
            if (status)
                *status = {};
            return 0;
        }
        catch (const std::bad_alloc&)
        {
            // Re-enter the original failure/exception boundary if a copy cannot
            // be allocated; no incomplete cached result is returned.
        }
        catch (const std::length_error&)
        {
        }
    }
    std::vector<std::string> captured_warnings;
    const int code =
        model_tessellation_from_bytes(data, size, options, collection, status, &captured_warnings);
    if (code == 0)
    {
        const auto digest = sha256(data, size);
        if (source_sha256)
            *source_sha256 = digest_hex(digest);
        retain(data, size, std::move(encoded), digest, *collection, captured_warnings, generation);
    }
    if (warnings)
        *warnings = std::move(captured_warnings);
    return code;
#endif
}

void reset_illustration_source_cache_for_tests(SourceCacheLimits limits)
{
    auto& state = cache();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.entries.clear();
    state.limits = limits;
    state.stats = {};
    ++state.generation;
}

SourceCacheStats illustration_source_cache_stats_for_tests()
{
    auto& state = cache();
    std::lock_guard<std::mutex> lock(state.mutex);
    auto result = state.stats;
    result.entries = state.entries.size();
    return result;
}
} // namespace geometer::model_illustration_detail

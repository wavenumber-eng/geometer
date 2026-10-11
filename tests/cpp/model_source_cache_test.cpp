#include "geometer/model_tessellation.h"
#include "geometer/operation_registry.h"
#include "model_illustration_source_cache.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <STEPControl_Writer.hxx>
#include <gp_Pnt.hxx>

#include <array>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace
{
using namespace geometer;
using namespace geometer::contracts;
using namespace geometer::model_illustration_detail;
using Bytes = std::vector<unsigned char>;

void require(bool value, const char* message)
{
    if (!value)
        throw std::runtime_error(message);
}

Bytes fixture(const char* name)
{
    std::ifstream stream(std::string(GEOMETER_TEST_SOURCE_DIR) +
                             "/tests/fixtures/step/embedded_models/" + name,
                         std::ios::binary);
    require(static_cast<bool>(stream), "STEP source fixture missing");
    return {std::istreambuf_iterator<char>(stream), {}};
}

template <typename Value> std::string json(const Value& value)
{
    std::string result;
    ContractError error;
    require(encode_json(value, &result, &error), "actual result failed canonical encoding");
    return result;
}

ModelTessellationRequestA0 options()
{
    ModelTessellationRequestA0 value;
    value.linear_deflection_mm = 0.01;
    value.angular_deflection_rad = 0.5;
    value.root_placement = ModelRootPlacement::preserve;
    return value;
}

struct Tessellation
{
    int code;
    MeshCollectionA0 meshes;
    Status status;
    std::vector<std::string> warnings;
};

Tessellation tessellate(const Bytes& source, const ModelTessellationRequestA0& request, bool cached,
                        bool wants_warnings = true)
{
    Tessellation result{};
    auto* warnings = wants_warnings ? &result.warnings : nullptr;
    result.code = cached ? tessellate_illustration_source(source.data(), source.size(), request,
                                                          &result.meshes, &result.status, warnings)
                         : model_tessellation_from_bytes(source.data(), source.size(), request,
                                                         &result.meshes, &result.status, warnings);
    return result;
}

void same_tessellation(const Tessellation& expected, const Tessellation& actual)
{
    require(actual.code == expected.code, "cache changed operation success/refusal");
    require(actual.status.message == expected.status.message, "cache changed status message");
    require(actual.warnings == expected.warnings, "cache changed retained warnings");
    if (actual.code == 0)
        require(json(actual.meshes) == json(expected.meshes), "cache changed full colored meshes");
    else
        require(actual.meshes.meshes.empty(), "failure leaked cached meshes");
}

void repeated_source_is_exact_and_returned_mutation_is_isolated()
{
    const auto source = fixture("SOT-23.STEP");
    const auto source_before = source;
    const auto expected = tessellate(source, options(), false);
    require(expected.code == 0 && !expected.meshes.meshes.empty(), "baseline STEP failed");
    reset_illustration_source_cache_for_tests({8, 16 * 1024 * 1024});
    auto first = tessellate(source, options(), true);
    same_tessellation(expected, first);
    first.meshes.meshes.front().positions.front() += 123;
    first.meshes.meshes.front().materials.front().color.front() = 0.99;
    first.meshes.meshes.front().materials.front().opacity = 0.25;
    same_tessellation(expected, tessellate(source, options(), true));
    const auto stats = illustration_source_cache_stats_for_tests();
    require(stats.entries == 1 && stats.insertions == 1 && stats.hits == 1 && stats.misses == 1,
            "repeat did not reuse exactly one successful source");
    require(stats.retained_bytes <= 16 * 1024 * 1024, "memo exceeded retained budget");
    require(source == source_before, "source STEP bytes were mutated");
}

void settings_and_exact_source_bytes_are_separate()
{
    const auto source = fixture("SOT-23.STEP");
    std::vector<ModelTessellationRequestA0> requests{options()};
    auto changed = options();
    changed.linear_deflection_mm = 0.02;
    requests.push_back(changed);
    changed = options();
    changed.angular_deflection_rad = 0.35;
    requests.push_back(changed);
    changed = options();
    changed.root_placement = ModelRootPlacement::strip;
    requests.push_back(changed);
    changed = options();
    changed.allow_partial = false;
    requests.push_back(changed);
    changed = options();
    changed.max_triangles = 750001;
    requests.push_back(changed);
    reset_illustration_source_cache_for_tests({16, 32 * 1024 * 1024});
    for (const auto& request : requests)
    {
        const auto expected = tessellate(source, request, false);
        require(expected.code == 0, "valid option fixture failed");
        same_tessellation(expected, tessellate(source, request, true));
    }
    auto different_bytes = source;
    different_bytes.push_back('\n');
    same_tessellation(tessellate(different_bytes, options(), false),
                      tessellate(different_bytes, options(), true));
    const auto stats = illustration_source_cache_stats_for_tests();
    require(stats.entries == requests.size() + 1 && stats.misses == requests.size() + 1,
            "settings or exact source content shared an incorrect key");
}

Bytes translated_step()
{
    // The existing OCCT-authored fixture declares a free-root translation and
    // half-turn, rather than baking a moved solid into its definition geometry.
    std::ifstream stream(std::string(GEOMETER_TEST_SOURCE_DIR) +
                             "/tests/fixtures/step/generated_topology/generated_fused_slab.step",
                         std::ios::binary);
    require(static_cast<bool>(stream), "actual root-placement STEP fixture missing");
    return {std::istreambuf_iterator<char>(stream), {}};
}

void actual_root_placement_is_preserved_per_request()
{
    const auto source = translated_step();
    auto stripped = options();
    stripped.root_placement = ModelRootPlacement::strip;
    const auto preserve = tessellate(source, options(), false);
    const auto strip = tessellate(source, stripped, false);
    require(preserve.code == 0 && strip.code == 0, "translated root fixture failed");
    require(json(preserve.meshes) != json(strip.meshes), "fixture has no actual root placement");
    reset_illustration_source_cache_for_tests({8, 16 * 1024 * 1024});
    same_tessellation(preserve, tessellate(source, options(), true));
    same_tessellation(strip, tessellate(source, stripped, true));
    same_tessellation(preserve, tessellate(source, options(), true));
    require(illustration_source_cache_stats_for_tests().entries == 2,
            "strip and preserve shared a cache entry");
}

void partial_warnings_survive_null_output_and_failures_are_not_cached()
{
    const auto source = fixture("GT-USB-7010C.STEP");
    const auto expected = tessellate(source, options(), false);
    require(expected.code == 0 && expected.warnings.size() >= 2, "partial warning fixture failed");
    reset_illustration_source_cache_for_tests({8, 32 * 1024 * 1024});
    require(tessellate(source, options(), true, false).code == 0, "null warning output failed");
    same_tessellation(expected, tessellate(source, options(), true));
    auto strict = options();
    strict.allow_partial = false;
    auto limited = options();
    limited.max_triangles = 1;
    for (const auto& request : {strict, limited})
    {
        const auto refusal = tessellate(source, request, false);
        require(refusal.code != 0, "partial strict/limit request unexpectedly succeeded");
        same_tessellation(refusal, tessellate(source, request, true));
        same_tessellation(refusal, tessellate(source, request, true));
    }
    const Bytes malformed{'n', 'o', 't', ' ', 'S', 'T', 'E', 'P'};
    const auto refusal = tessellate(malformed, options(), false);
    require(refusal.code != 0, "malformed STEP was accepted");
    same_tessellation(refusal, tessellate(malformed, options(), true));
    same_tessellation(refusal, tessellate(malformed, options(), true));
    require(illustration_source_cache_stats_for_tests().insertions == 1,
            "unsuccessful preparation entered the source memo");
}

void tiny_capacity_and_oversized_input_preserve_operation_results()
{
    const auto source = fixture("SOT-23.STEP");
    const auto expected = tessellate(source, options(), false);
    reset_illustration_source_cache_for_tests({2, 1});
    same_tessellation(expected, tessellate(source, options(), true));
    same_tessellation(expected, tessellate(source, options(), true));
    const auto bypass = illustration_source_cache_stats_for_tests();
    require(bypass.entries == 0 && bypass.insertions == 0 && bypass.bypasses == 2,
            "oversized retained entry was not a success-preserving bypass");
    MeshCollectionA0 output;
    Status status;
    require(tessellate_illustration_source(source.data(), 256 * 1024 * 1024 + 1, options(), &output,
                                           &status, nullptr) != 0 &&
                output.meshes.empty(),
            "oversized attachment was accessed or admitted");
    reset_illustration_source_cache_for_tests({2, 16 * 1024 * 1024});
    auto second = source;
    second.push_back('\n');
    auto third = second;
    third.push_back('\n');
    tessellate(source, options(), true);
    tessellate(second, options(), true);
    tessellate(source, options(), true);
    tessellate(third, options(), true);
    const auto bounded = illustration_source_cache_stats_for_tests();
    require(bounded.entries == 2 && bounded.evictions == 1 && bounded.hits == 1,
            "bounded LRU did not evict exactly one least-recent source");
    same_tessellation(expected, tessellate(source, options(), true));
    require(illustration_source_cache_stats_for_tests().hits == 2,
            "LRU evicted the recently accessed source");
}

void no_usable_surface_refusal_is_not_retained()
{
    STEPControl_Writer writer;
    require(writer.Transfer(BRepBuilderAPI_MakeEdge(gp_Pnt(0, 0, 0), gp_Pnt(1, 0, 0)).Shape(),
                            STEPControl_AsIs) == IFSelect_RetDone,
            "edge source transfer failed");
    std::ostringstream output;
    require(writer.WriteStream(output) == IFSelect_RetDone, "edge STEP serialization failed");
    const auto text = output.str();
    const Bytes source{text.begin(), text.end()};
    const auto expected = tessellate(source, options(), false);
    require(expected.code != 0 && expected.meshes.meshes.empty(),
            "source with no usable surfaces unexpectedly succeeded");
    reset_illustration_source_cache_for_tests({8, 16 * 1024 * 1024});
    same_tessellation(expected, tessellate(source, options(), true));
    same_tessellation(expected, tessellate(source, options(), true));
    require(illustration_source_cache_stats_for_tests().entries == 0,
            "empty preparation refusal entered memo");
}

struct Illustration
{
    bool empty;
    std::string outcome;
    Bytes geometry;
};

Illustration illustrate(const Bytes& source, const ModelIllustrationGeometryRequestB0& request)
{
    const auto encoded = json(request);
    OperationExecution execution;
    execute_operation("geometry.model_illustration_geometry.b0",
                      reinterpret_cast<const unsigned char*>(encoded.data()), encoded.size(),
                      {{"model", "model/step", source.data(), source.size()}}, &execution);
    auto* success = std::get_if<OperationSuccessB0>(&execution.outcome);
    require(success != nullptr, "actual STEP B0 operation failed");
    auto* result = std::get_if<ModelIllustrationGeometryResultB0>(&success->result);
    require(result != nullptr && execution.attachments.size() == 1,
            "B0 response lost governed metadata/geometry");
    const auto empty = result->empty;
    result->timings = {}; // Only the actual elapsed work differs with reuse.
    std::string outcome;
    require(encode_operation_outcome(execution.outcome, &outcome), "B0 outcome encoding failed");
    return {empty, outcome, execution.attachments.front().data};
}

void posed_material_and_empty_b0_results_match_uncached_source()
{
    ModelAttachmentIllustrationSourceA0 source_options;
    source_options.tessellation.emplace();
    source_options.tessellation->linear_deflection_mm = 0.01;
    source_options.tessellation->root_placement = ModelRootPlacement::preserve;
    ModelIllustrationGeometryRequestB0 base;
    base.source = source_options;
    base.view.direction = {0, 0, 1};
    base.view.up = {0, 1, 0};
    base.style.emplace();
    base.style->source_colors = true;
    std::vector<ModelIllustrationGeometryRequestB0> requests{base};
    auto posed = base;
    auto& model = std::get<ModelAttachmentIllustrationSourceA0>(posed.source);
    model.transform = IllustrationMatrix4x4{0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0, 7, -3, 1, 1};
    requests.push_back(posed);
    auto painted = posed;
    std::get<ModelAttachmentIllustrationSourceA0>(painted.source).material_override =
        MeshIllustrationMaterial{{0.8, 0.1, 0.2}, 0.3, std::nullopt};
    requests.push_back(painted);
    auto bottom = base;
    bottom.view.direction = {0, 0, -1};
    bottom.view.mirror_x = true;
    requests.push_back(bottom);
    auto empty = base;
    empty.clipping = IllustrationClipping{{HalfSpacePlane{{0, 0, 1}, 1000, 0.0}}, "none", {}};
    requests.push_back(empty);
    requests.push_back(base);
    const auto source = fixture("SOT-23.STEP");
    std::vector<Illustration> expected;
    reset_illustration_source_cache_for_tests({0, 0});
    for (const auto& request : requests)
        expected.push_back(illustrate(source, request));
    require(!expected.front().empty && expected[4].empty, "empty clipping fixture is ineffective");
    require(expected[0].outcome != expected[1].outcome, "actual pose was not reflected in result");
    require(expected[1].geometry != expected[2].geometry, "material override was not rendered");
    reset_illustration_source_cache_for_tests({8, 16 * 1024 * 1024});
    for (std::size_t index = 0; index < requests.size(); ++index)
    {
        const auto actual = illustrate(source, requests[index]);
        require(actual.empty == expected[index].empty &&
                    actual.outcome == expected[index].outcome &&
                    actual.geometry == expected[index].geometry,
                "posed/material/empty result differs from uncached operation");
    }
    const auto stats = illustration_source_cache_stats_for_tests();
    require(stats.entries == 1 && stats.misses == 1 && stats.hits == requests.size() - 1,
            "view/pose/material requests did not reuse exactly the immutable source");
}

void concurrent_warm_reads_clone_independently_within_capacity()
{
    const auto source = fixture("SOT-23.STEP");
    const auto expected = tessellate(source, options(), false);
    reset_illustration_source_cache_for_tests({2, 16 * 1024 * 1024});
    same_tessellation(expected, tessellate(source, options(), true));
    std::array<std::thread, 4> workers;
    std::array<std::string, 4> failures;
    for (std::size_t index = 0; index < workers.size(); ++index)
        workers[index] = std::thread(
            [&, index]
            {
                try
                {
                    auto value = tessellate(source, options(), true);
                    same_tessellation(expected, value);
                    value.meshes.meshes.front().positions.front() += index + 1;
                }
                catch (const std::exception& error)
                {
                    failures[index] = error.what();
                }
            });
    for (auto& worker : workers)
        worker.join();
    for (const auto& failure : failures)
        require(failure.empty(), "concurrent cached source read changed a result");
    same_tessellation(expected, tessellate(source, options(), true));
    const auto stats = illustration_source_cache_stats_for_tests();
    require(stats.entries == 1 && stats.hits == 5 && stats.retained_bytes <= 16 * 1024 * 1024,
            "concurrent warm reads broke identity/capacity accounting");
}
} // namespace

int main()
{
    try
    {
        repeated_source_is_exact_and_returned_mutation_is_isolated();
        settings_and_exact_source_bytes_are_separate();
        actual_root_placement_is_preserved_per_request();
        partial_warnings_survive_null_output_and_failures_are_not_cached();
        tiny_capacity_and_oversized_input_preserve_operation_results();
        no_usable_surface_refusal_is_not_retained();
        posed_material_and_empty_b0_results_match_uncached_source();
        concurrent_warm_reads_clone_independently_within_capacity();
        std::cout << "model source cache tests passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

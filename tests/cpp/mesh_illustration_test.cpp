#include "geometer/mesh_illustration.h"
#include "geometer/model_tessellation.h"
#include "mesh_illustration_internal.h"
#include "mesh_illustration_svg.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <locale>
#include <rapidjson/document.h>
#include <stdexcept>

namespace
{
class CommaDecimal final : public std::numpunct<char>
{
    char do_decimal_point() const override
    {
        return ',';
    }
};

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void numeric_formats(const char* path)
{
    std::ifstream file(path);
    const std::string bytes{std::istreambuf_iterator<char>(file), {}};
    rapidjson::Document values;
    values.Parse<rapidjson::kParseFullPrecisionFlag>(bytes.data(), bytes.size());
    require(!values.HasParseError() && values.IsArray(), "invalid numeric fixture");
    std::cout << '[';
    bool first = true;
    for (const auto& item : values.GetArray())
    {
        if (!first)
            std::cout << ',';
        first = false;
        const auto value = item.GetDouble();
        std::string number;
        try
        {
            number = geometer::illustration_detail::number_text(value);
        }
        catch (const std::runtime_error&)
        {
            number = "overflow";
        }
        std::cout << '[' << std::quoted(number) << ','
                  << std::quoted(geometer::illustration_detail::integer_text(value)) << ']';
    }
    std::cout << ']';
}

geometer::contracts::MeshIllustrationInputA0 fixture()
{
    geometer::contracts::MeshIllustrationInputA0 input;
    geometer::contracts::MeshIllustrationMesh mesh;
    mesh.id = "square";
    mesh.positions = {0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0};
    mesh.indices = std::vector<std::uint32_t>{0, 1, 2, 0, 2, 3};
    mesh.materials.push_back({{.2, .7, .62}, {}, {}});
    input.meshes.push_back(mesh);
    input.view.direction = {0, 0, 1};
    input.view.up = {0, 1, 0};
    return input;
}

void smoke()
{
    auto input = fixture();
    geometer::contracts::MeshIllustrationResultA0 result, repeated;
    geometer::Status status;
    require(geometer::illustrate_mesh(input, &result, &status) == 0, status.message.c_str());
    require(result.stats.triangles == 2 && result.stats.surface_draws == 1, "square fusion failed");
    require(result.stats.outlines == 0 && result.stats.creases == 0, "open edges became outlines");
    require(result.svg.find("<path") != std::string::npos, "missing fused SVG path");
    require(geometer::illustrate_mesh(input, &repeated, &status) == 0 && result.svg == repeated.svg,
            "SVG not deterministic");
    input.prepare.emplace();
    input.prepare->max_triangles = 1;
    require(geometer::illustrate_mesh(input, &result, &status) == 102 && result.svg.empty(),
            "triangle limit leaked partial SVG");
    input.prepare.reset();
    input.view.up = input.view.direction;
    require(geometer::illustrate_mesh(input, &result, &status) != 0 && result.svg.empty(),
            "parallel view vectors accepted");
    require(geometer::illustration_detail::js_round(-.5) == 0, "negative JS rounding mismatch");
    require(geometer::illustration_detail::number_text(.000001) == "0.000001",
            "JS number formatting");
    require(geometer::illustration_detail::number_text(.5001220703125) == "0.500122070313",
            "ECMAScript precision halfway rounding mismatch");
    require(geometer::illustration_detail::fixed_text(.5001220703125) == "0.500122070313",
            "ECMAScript fixed halfway rounding mismatch");
    const auto previous_locale = std::locale();
    std::locale::global(std::locale(previous_locale, new CommaDecimal));
    require(geometer::illustration_detail::number_text(.5001220703125) == "0.500122070313" &&
                geometer::illustration_detail::fixed_text(.5001220703125) == "0.500122070313",
            "illustration formatting depends on global decimal locale");
    std::locale::global(previous_locale);
    require(geometer::illustration_detail::number_text(1e-7) == "1e-7" &&
                geometer::illustration_detail::number_text(1e21) == "1e+21" &&
                geometer::illustration_detail::number_text(-0.0) == "0" &&
                geometer::illustration_detail::integer_text(42) == "42",
            "portable JS notation thresholds or integer suffix mismatch");
    require(geometer::illustration_detail::number_text(1e23) == "1e+23" &&
                geometer::illustration_detail::integer_text(1e23) == "1e+23",
            "shortest decimal differs from ECMAScript at 1e23");
    std::string bounded;
    geometer::illustration_detail::append_bounded(bounded, "abcd", 4);
    bool rejected = false;
    try
    {
        geometer::illustration_detail::append_bounded(bounded, "\n", 4);
    }
    catch (const geometer::illustration_detail::ResourceLimit&)
    {
        rejected = true;
    }
    require(rejected && bounded == "abcd", "bounded append exceeded exact capacity");
    rejected = false;
    try
    {
        geometer::illustration_detail::append_bounded(bounded, "x", 3);
    }
    catch (const geometer::illustration_detail::ResourceLimit&)
    {
        rejected = true;
    }
    require(rejected, "bounded append subtraction underflow");
    input = fixture();
    input.svg.emplace();
    input.svg->title = "a]]>b";
    require(geometer::illustrate_mesh(input, &result, &status) == 0 &&
                result.svg.find("a]]&gt;b") != std::string::npos,
            "invalid XML text delimiter");
    input.svg->title = "bad\x01title";
    require(geometer::illustrate_mesh(input, &result, &status) != 0 && result.svg.empty(),
            "invalid XML control accepted");
    input = fixture();
    input.prepare.emplace();
    input.prepare->weld_tolerance = 1e308;
    for (std::size_t i = 2; i < input.meshes[0].positions.size(); i += 3)
        input.meshes[0].positions[i] = 1e308;
    require(geometer::illustrate_mesh(input, &result, &status) != 0 && result.svg.empty(),
            "overflowing mean depth accepted");
    input = fixture();
    input.meshes[0].matrix =
        std::vector<double>{1, 0, 0, 1e308, 0, 1, 0, 1e308, 0, 0, 1, 0, 0, 0, 0, 1};
    require(geometer::illustrate_mesh(input, &result, &status) != 0 && result.svg.empty(),
            "overflowing homogeneous divisor accepted");
}

void hlr_smoke()
{
    auto input = fixture();
    input.style.emplace();
    input.style->show_outlines = false;
    input.style->show_creases = false;
    input.style->show_hlr_outline = true;
    input.style->show_hlr_detail = true;
    geometer::contracts::HlrProjectionResultA0 hlr;
    hlr.source.hash = std::string(64, '0');
    geometer::contracts::HlrProjectedView view;
    view.id = "test";
    view.direction = input.view.direction;
    view.up = input.view.up;
    view.modes.outline.segments.push_back({0, 0, 1, 0});
    view.modes.detail.segments.push_back({0, 0, 1, 1});
    hlr.views.push_back(view);
    geometer::contracts::MeshIllustrationResultA0 result, repeated;
    geometer::Status status;
    require(geometer::illustrate_mesh(input, hlr, &result, &status) == 0, status.message.c_str());
    require(result.stats.outlines == 1 && result.stats.details == 1 && result.stats.creases == 0,
            "HLR composition counts");
    // Up can contain a direction component or use a different finite scale.
    input.view.up = {0, 1e-13, 1e-6};
    hlr.views[0].up = {0, 2e-13, 2e-6};
    require(geometer::illustrate_mesh(input, hlr, &repeated, &status) == 0, status.message.c_str());
    require(repeated.svg == result.svg, "equivalent skewed/scaled HLR basis changed SVG");
    hlr.views[0].up = {1, 0, 0};
    require(geometer::illustrate_mesh(input, hlr, &result, &status) != 0 && result.svg.empty(),
            "wrong HLR up accepted or leaked SVG");
    hlr.views[0].up = input.view.up;
    hlr.views[0].modes.detail.segments[0][0] = std::numeric_limits<double>::infinity();
    require(geometer::illustrate_mesh(input, hlr, &result, &status) != 0 && result.svg.empty(),
            "nonfinite HLR accepted or leaked SVG");
    hlr.views[0].modes.detail.segments.clear();
    input.style->show_hlr_outline = false;
    input.style->show_hlr_detail = false;
    hlr.views[0].modes.bbox.segments.resize(1000000);
    require(geometer::illustrate_mesh(input, hlr, &result, &status) == 102 && result.svg.empty(),
            "disabled HLR layer bypassed segment cap");
}
} // namespace

int main(int argc, char** argv)
{
    try
    {
        if (argc == 1)
        {
            smoke();
            hlr_smoke();
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "--numbers")
        {
            numeric_formats(argv[2]);
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "--step")
        {
            std::ifstream file(argv[2], std::ios::binary);
            require(file.good(), "Cannot read STEP fixture");
            const std::string bytes{std::istreambuf_iterator<char>(file), {}};
            geometer::contracts::MeshCollectionA0 meshes;
            geometer::Status status;
            require(geometer::model_tessellation_from_bytes(
                        reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size(), {},
                        &meshes, &status) == 0,
                    status.message.c_str());
            std::string output;
            geometer::contracts::ContractError error;
            require(geometer::contracts::encode_json(meshes, &output, &error),
                    error.message.c_str());
            std::cout << output;
            return 0;
        }
        if (argc != 2)
            throw std::runtime_error("Expected one governed input JSON fixture path.");
        std::ifstream file(argv[1], std::ios::binary);
        require(file.good(), "Cannot read input fixture");
        const std::string bytes{std::istreambuf_iterator<char>(file), {}};
        geometer::contracts::MeshIllustrationInputA0 input;
        geometer::contracts::ContractError error;
        require(
            geometer::contracts::decode_json(reinterpret_cast<const unsigned char*>(bytes.data()),
                                             bytes.size(), &input, &error),
            error.message.c_str());
        geometer::contracts::MeshIllustrationResultA0 result;
        geometer::Status status;
        require(geometer::illustrate_mesh(input, &result, &status) == 0, status.message.c_str());
        std::string output;
        require(geometer::contracts::encode_json(result, &output, &error), error.message.c_str());
        std::cout << output;
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

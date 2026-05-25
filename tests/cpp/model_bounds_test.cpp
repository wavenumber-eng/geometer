#include "geometer/model_bounds.h"
#include "geometer/model_bounds_options_json.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

std::vector<unsigned char> read_bytes(const std::string& path)
{
    std::ifstream input(path, std::ios::binary);
    require(static_cast<bool>(input), "failed opening fixture: " + path);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void model_bounds_reads_step_and_applies_transform()
{
    const std::string path =
        std::string(GEOMETER_TEST_SOURCE_DIR) + "/tests/fixtures/step/embedded_models/SOT-23.STEP";
    const std::vector<unsigned char> bytes = read_bytes(path);

    geometer::ModelBoundsOptions options;
    geometer::ModelBoundsResult base;
    geometer::Status status;
    int code =
        geometer::model_bounds_from_bytes(bytes.data(), bytes.size(), options, &base, &status);
    require(code == 0, "model_bounds_from_bytes failed: " + status.message);
    require(base.schema == "geometry.model_bounds.a0", "unexpected schema");
    require(base.units == "mm", "unexpected units");
    require(base.source_format == "step", "unexpected source format");
    require(!base.source_hash.empty(), "source hash should be populated");
    require(base.max[0] > base.min[0], "x bounds should be non-empty");
    require(base.max[1] > base.min[1], "y bounds should be non-empty");
    require(base.max[2] > base.min[2], "z bounds should be non-empty");

    geometer::ModelBoundsOptions moved_options;
    moved_options.model_transform[3] = 10.0;
    moved_options.model_transform[7] = 20.0;
    moved_options.model_transform[11] = 30.0;

    geometer::ModelBoundsResult moved;
    status = {};
    code = geometer::model_bounds_from_bytes(bytes.data(), bytes.size(), moved_options, &moved,
                                             &status);
    require(code == 0, "translated model_bounds_from_bytes failed: " + status.message);
    require(std::fabs((moved.min[0] - base.min[0]) - 10.0) < 1.0e-6,
            "x translation should move min bounds");
    require(std::fabs((moved.min[1] - base.min[1]) - 20.0) < 1.0e-6,
            "y translation should move min bounds");
    require(std::fabs((moved.min[2] - base.min[2]) - 30.0) < 1.0e-6,
            "z translation should move min bounds");

    std::string json;
    status = {};
    code = geometer::write_model_bounds_json(moved, &json, &status);
    require(code == 0, "write_model_bounds_json failed: " + status.message);
    require(json.find("\"schema\":\"geometry.model_bounds.a0\"") != std::string::npos,
            "bounds JSON should include schema");
    require(json.find("\"format\":\"step\"") != std::string::npos,
            "bounds JSON should include source format");
}

void model_bounds_options_parse_format_and_transform()
{
    geometer::ModelBoundsOptions options;
    geometer::Status status;
    const char* json =
        "{\"format\":\"step\",\"model_transform\":[[1,0,0,4],[0,1,0,5],[0,0,1,6],[0,0,0,1]]}";
    int code = geometer::parse_model_bounds_options_json(json, &options, &status);
    require(code == 0, "parse_model_bounds_options_json failed: " + status.message);
    require(options.format == geometer::ModelFormat::Step, "format should parse as STEP");
    require(options.model_transform[3] == 4.0, "x translation should parse");
    require(options.model_transform[7] == 5.0, "y translation should parse");
    require(options.model_transform[11] == 6.0, "z translation should parse");

    code = geometer::parse_model_bounds_options_json("{\"format\":\"stl\"}", &options, &status);
    require(code == 91, "unsupported format should be rejected");
}

} // namespace

int main()
{
    int result = 0;
    try
    {
        model_bounds_reads_step_and_applies_transform();
        model_bounds_options_parse_format_and_transform();
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        result = 1;
    }
    std::fflush(stdout);
    std::fflush(stderr);
    std::_Exit(result);
}

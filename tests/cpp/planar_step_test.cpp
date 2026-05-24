#include "geometer/planar_step.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace
{

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

} // namespace

int run_planar_step_test()
{
    const char* request = R"json(
{
  "schema": "geometry.planar_step.request.a0",
  "units": "mm",
  "name": "planar_step_test",
  "bodies": [
    {
      "id": "copper",
      "name": "copper",
      "color": "#B87333",
      "thickness_mm": 0.1,
      "regions": [
        {
          "outer": {
            "points": [[0, 0], [10, 0], [10, 5], [0, 5]],
            "segments": [
              {"kind": "line"},
              {"kind": "line"},
              {"kind": "line"},
              {"kind": "line"}
            ]
          },
          "holes": [
            {
              "points": [[6, 2.5], [5, 3.5], [4, 2.5], [5, 1.5]],
              "segments": [
                {"kind": "arc", "center": [5, 2.5], "sweep": "ccw"},
                {"kind": "arc", "center": [5, 2.5], "sweep": "ccw"},
                {"kind": "arc", "center": [5, 2.5], "sweep": "ccw"},
                {"kind": "arc", "center": [5, 2.5], "sweep": "ccw"}
              ]
            }
          ]
        }
      ]
    }
  ]
}
)json";

    std::vector<unsigned char> bytes;
    geometer::PlanarStepResult summary;
    geometer::Status status;
    const int code = geometer::planar_step_from_json_bytes(request, &bytes, &summary, &status);

    require(code == 0, "planar_step_from_json_bytes failed: " + status.message);
    require(bytes.size() > 1000, "STEP output should not be empty");
    const std::string text(reinterpret_cast<const char*>(bytes.data()),
                           std::min<std::size_t>(bytes.size(), 64));
    require(text.rfind("ISO-10303-21;", 0) == 0, "STEP output should start with ISO-10303-21");
    require(summary.body_count == 1, "summary should report one body");
    require(summary.region_count == 1, "summary should report one region");

    const char* fused_request = R"json(
{
  "schema": "geometry.planar_step.request.a0",
  "units": "mm",
  "name": "planar_step_fused_test",
  "bodies": [
    {
      "id": "copper",
      "name": "copper",
      "color": "#B87333",
      "thickness_mm": 0.1,
      "fuse_regions": true,
      "regions": [
        {
          "outer": {
            "points": [[0, 0], [4, 0], [4, 2], [0, 2]],
            "segments": [
              {"kind": "line"},
              {"kind": "line"},
              {"kind": "line"},
              {"kind": "line"}
            ]
          }
        },
        {
          "outer": {
            "points": [[2, 0], [6, 0], [6, 2], [2, 2]],
            "segments": [
              {"kind": "line"},
              {"kind": "line"},
              {"kind": "line"},
              {"kind": "line"}
            ]
          }
        }
      ]
    }
  ]
}
)json";

    bytes.clear();
    summary = {};
    status = {};
    const int fused_code =
        geometer::planar_step_from_json_bytes(fused_request, &bytes, &summary, &status);
    require(fused_code == 0, "fused planar_step_from_json_bytes failed: " + status.message);
    require(bytes.size() > 1000, "fused STEP output should not be empty");
    require(summary.body_count == 1, "fused summary should report one body");
    require(summary.region_count == 1, "overlapping fused regions should become one region");
    return 0;
}

int main()
{
    const int code = run_planar_step_test();
#ifdef _WIN32
    // OCCT STEP writer static state can crash during Windows atexit teardown.
    // Locals above have been destroyed; skip only process-global cleanup.
    std::fflush(nullptr);
    std::_Exit(code);
#else
    return code;
#endif
}

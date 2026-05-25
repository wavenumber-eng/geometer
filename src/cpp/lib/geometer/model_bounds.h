#pragma once

#include "model.h"
#include "status.h"

#include <array>
#include <cstddef>
#include <string>

namespace geometer
{

struct ModelBoundsOptions
{
    ModelFormat format = ModelFormat::Step;
    // Row-major 4x4 affine transform applied to the source model before bounds
    // are computed. Translation lives in the final column. The final row must
    // be [0, 0, 0, 1].
    std::array<double, 16> model_transform = {
        1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0,
    };
};

struct ModelBoundsTimings
{
    double model_read_ms = 0.0;
    double bounds_ms = 0.0;
};

struct ModelBoundsResult
{
    std::string schema = "geometry.model_bounds.a0";
    std::string units = "mm";
    std::string source_format = "step";
    std::string source_hash;
    std::array<double, 3> min = {0.0, 0.0, 0.0};
    std::array<double, 3> max = {0.0, 0.0, 0.0};
    std::array<double, 3> size = {0.0, 0.0, 0.0};
    std::array<double, 3> center = {0.0, 0.0, 0.0};
    ModelBoundsTimings timings;
};

int model_bounds_from_bytes(const unsigned char* model_data, std::size_t model_size,
                            const ModelBoundsOptions& options, ModelBoundsResult* result,
                            Status* status = nullptr);

int write_model_bounds_json(const ModelBoundsResult& result, std::string* json,
                            Status* status = nullptr);

} // namespace geometer

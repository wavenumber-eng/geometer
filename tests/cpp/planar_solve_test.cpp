#include "geometer/c_api.h"
#include "geometer/planar_solve.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

constexpr unsigned char REQUEST_MAGIC[8] = {'G', 'M', 'P', 'B', 'R', 'Q', '0', '1'};
constexpr unsigned char RESPONSE_MAGIC[8] = {'G', 'M', 'P', 'B', 'R', 'S', '0', '1'};
constexpr std::uint32_t FORMAT_VERSION = 1;
constexpr std::uint32_t JOB_SUBTRACT_COMMON_RINGS = 1u << 0u;
constexpr std::uint32_t JOB_FILTER_COMMON_SUBTRACT_BY_BOUNDS = 1u << 1u;

geometer::PlanarSolveRing rect(double min_x, double min_y, double max_x, double max_y)
{
    return {
        {min_x, min_y},
        {max_x, min_y},
        {max_x, max_y},
        {min_x, max_y},
    };
}

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

void require_near(double actual, double expected, double tolerance, const std::string& message)
{
    if (std::fabs(actual - expected) > tolerance)
    {
        throw std::runtime_error(message + ": actual=" + std::to_string(actual) +
                                 " expected=" + std::to_string(expected));
    }
}

void overlapping_strokes_fuse_to_one_region()
{
    geometer::PlanarBatchSolveInput input;
    input.options.decimal_precision = 6;
    input.options.cleanup_radius_mm = 0.002;

    geometer::PlanarSolveStrokeGroup strokes;
    strokes.radius_mm = 1.0;
    strokes.join_type = geometer::PlanarSolveJoinType::Miter;
    strokes.end_type = geometer::PlanarSolveEndType::Round;
    strokes.paths.push_back({{0.0, 0.0}, {10.0, 0.0}});
    strokes.paths.push_back({{8.0, 0.0}, {18.0, 0.0}});

    geometer::PlanarSolveJob job;
    job.stroke_groups.push_back(strokes);
    input.jobs.push_back(job);

    geometer::PlanarBatchSolveResult result;
    geometer::Status status;
    const int code = geometer::solve_planar_batch(input, &result, &status);
    require(code == 0, "solve_planar_batch failed: " + status.message);
    require(result.jobs.size() == 1, "one job should be returned");
    require(result.jobs[0].regions.size() == 1, "overlapping strokes should fuse into one region");
    require(result.jobs[0].regions[0].holes.empty(), "fused stroke should not have holes");
    require(result.jobs[0].area_mm2 > 35.0 && result.jobs[0].area_mm2 < 42.0,
            "fused stroke area should be capsule-like");
}

void subtract_then_clip()
{
    geometer::PlanarBatchSolveInput input;
    input.options.decimal_precision = 6;
    input.final_clip_rings.push_back(rect(0.0, 0.0, 5.0, 10.0));

    geometer::PlanarSolveJob job;
    job.subject_rings.push_back(rect(0.0, 0.0, 10.0, 10.0));
    job.subtract_rings.push_back(rect(4.0, 4.0, 6.0, 6.0));
    input.jobs.push_back(job);

    geometer::PlanarBatchSolveResult result;
    geometer::Status status;
    const int code = geometer::solve_planar_batch(input, &result, &status);
    require(code == 0, "solve_planar_batch failed: " + status.message);
    require(result.jobs.size() == 1, "one job should be returned");
    require(result.jobs[0].regions.size() == 1, "clipped rectangle should stay one region");
    require_near(result.jobs[0].area_mm2, 48.0, 1.0e-6,
                 "subtract then clip area should be stable");
}

class Writer
{
public:
    void bytes(const unsigned char* data, std::size_t size)
    {
        data_.insert(data_.end(), data, data + size);
    }

    void u32(std::uint32_t value)
    {
        data_.push_back(static_cast<unsigned char>(value & 0xffu));
        data_.push_back(static_cast<unsigned char>((value >> 8u) & 0xffu));
        data_.push_back(static_cast<unsigned char>((value >> 16u) & 0xffu));
        data_.push_back(static_cast<unsigned char>((value >> 24u) & 0xffu));
    }

    void f64(double value)
    {
        std::uint64_t bits = 0;
        std::memcpy(&bits, &value, sizeof(value));
        for (std::size_t i = 0; i < 8; ++i)
        {
            data_.push_back(static_cast<unsigned char>((bits >> (8u * i)) & 0xffu));
        }
    }

    const std::vector<unsigned char>& data() const
    {
        return data_;
    }

private:
    std::vector<unsigned char> data_;
};

class Reader
{
public:
    Reader(const unsigned char* data, std::size_t size) : data_(data), size_(size) {}

    void magic(const unsigned char expected[8])
    {
        require(offset_ + 8 <= size_, "response ended before magic");
        require(std::memcmp(data_ + offset_, expected, 8) == 0, "response magic mismatch");
        offset_ += 8;
    }

    std::uint32_t u32()
    {
        require(offset_ + 4 <= size_, "response ended before u32");
        std::uint32_t value = 0;
        value |= static_cast<std::uint32_t>(data_[offset_]);
        value |= static_cast<std::uint32_t>(data_[offset_ + 1]) << 8u;
        value |= static_cast<std::uint32_t>(data_[offset_ + 2]) << 16u;
        value |= static_cast<std::uint32_t>(data_[offset_ + 3]) << 24u;
        offset_ += 4;
        return value;
    }

    double f64()
    {
        require(offset_ + 8 <= size_, "response ended before f64");
        std::uint64_t bits = 0;
        for (std::size_t i = 0; i < 8; ++i)
        {
            bits |= static_cast<std::uint64_t>(data_[offset_ + i]) << (8u * i);
        }
        offset_ += 8;
        double value = 0.0;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }

private:
    const unsigned char* data_ = nullptr;
    std::size_t size_ = 0;
    std::size_t offset_ = 0;
};

void write_ring(Writer* writer, const geometer::PlanarSolveRing& ring)
{
    writer->u32(static_cast<std::uint32_t>(ring.size()));
    for (const geometer::PlanarSolvePoint& point : ring)
    {
        writer->f64(point.x);
        writer->f64(point.y);
    }
}

void c_api_binary_batch_solve()
{
    Writer writer;
    writer.bytes(REQUEST_MAGIC, 8);
    writer.u32(FORMAT_VERSION);
    writer.u32(0);
    writer.u32(6);
    writer.u32(1);
    writer.f64(0.0);
    writer.f64(2.0);
    writer.f64(0.0);
    writer.u32(1);
    writer.u32(0);
    writer.u32(0);
    writer.u32(0);
    write_ring(&writer, rect(1.0, 1.0, 2.0, 2.0));

    writer.u32(JOB_SUBTRACT_COMMON_RINGS | JOB_FILTER_COMMON_SUBTRACT_BY_BOUNDS);
    writer.f64(0.0);
    writer.u32(1);
    writer.u32(0);
    writer.u32(0);
    writer.u32(0);
    write_ring(&writer, rect(0.0, 0.0, 3.0, 3.0));

    unsigned char* value = nullptr;
    std::size_t value_size = 0;
    char* error = nullptr;
    const int code = geometer_planar_batch_solve_bytes(
        writer.data().data(), writer.data().size(), &value, &value_size, &error);
    if (code != 0)
    {
        const std::string message = error == nullptr ? "" : error;
        geometer_free_string(error);
        throw std::runtime_error("geometer_planar_batch_solve_bytes failed: " + message);
    }

    Reader reader(value, value_size);
    reader.magic(RESPONSE_MAGIC);
    require(reader.u32() == FORMAT_VERSION, "response version mismatch");
    require(reader.u32() == 1, "response job count mismatch");
    require(reader.u32() == 1, "response region count mismatch");
    (void)reader.u32();
    (void)reader.u32();
    (void)reader.u32();
    require(reader.u32() == 1, "job region count mismatch");
    (void)reader.u32();
    (void)reader.u32();
    (void)reader.u32();
    require_near(reader.f64(), 8.0, 1.0e-6, "C API binary area should be stable");
    geometer_free_bytes(value);
}

} // namespace

int main()
{
    try
    {
        overlapping_strokes_fuse_to_one_region();
        subtract_then_clip();
        c_api_binary_batch_solve();
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}

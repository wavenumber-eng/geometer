#include "geometer/planar_triangulate.h"

#include <clipper2/clipper.h>
#include <clipper2/clipper.triangulation.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace geometer
{
namespace
{

constexpr unsigned char REQUEST_MAGIC[8] = {'G', 'M', 'T', 'R', 'R', 'Q', '0', '1'};
constexpr unsigned char RESPONSE_MAGIC[8] = {'G', 'M', 'T', 'R', 'R', 'S', '0', '1'};
constexpr std::uint32_t FORMAT_VERSION = 1;

using Clipper2Lib::PathD;
using Clipper2Lib::PathsD;
using Clipper2Lib::PointD;
using Clipper2Lib::Triangulate;
using Clipper2Lib::TriangulateResult;

void set_status(Status* status, int code, const std::string& message)
{
    if (status == nullptr)
    {
        return;
    }
    status->code = code;
    status->message = message;
}

class BinaryReader
{
  public:
    BinaryReader(const unsigned char* data, std::size_t size) : data_(data), size_(size) {}

    void require(std::size_t count)
    {
        if (offset_ > size_ || count > size_ - offset_)
        {
            throw std::runtime_error("Planar triangulate request ended unexpectedly.");
        }
    }

    std::uint32_t u32()
    {
        require(4);
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
        require(8);
        std::uint64_t bits = 0;
        for (std::size_t i = 0; i < 8; ++i)
        {
            bits |= static_cast<std::uint64_t>(data_[offset_ + i]) << (8u * i);
        }
        offset_ += 8;
        double value = 0.0;
        static_assert(sizeof(value) == sizeof(bits), "double size must be 8 bytes");
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }

    void magic(const unsigned char expected[8])
    {
        require(8);
        if (std::memcmp(data_ + offset_, expected, 8) != 0)
        {
            throw std::runtime_error("Planar triangulate request has an invalid magic value.");
        }
        offset_ += 8;
    }

    void done() const
    {
        if (offset_ != size_)
        {
            throw std::runtime_error("Planar triangulate request has trailing bytes.");
        }
    }

  private:
    const unsigned char* data_ = nullptr;
    std::size_t size_ = 0;
    std::size_t offset_ = 0;
};

class BinaryWriter
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

    std::vector<unsigned char> take()
    {
        return std::move(data_);
    }

  private:
    std::vector<unsigned char> data_;
};

std::uint32_t checked_count(std::size_t value, const char* label)
{
    if (value > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
    {
        throw std::runtime_error(std::string(label) + " exceeds uint32 range.");
    }
    return static_cast<std::uint32_t>(value);
}

PlanarTriangulateRing read_ring(BinaryReader* reader)
{
    const std::uint32_t point_count = reader->u32();
    PlanarTriangulateRing ring;
    ring.reserve(point_count);
    for (std::uint32_t i = 0; i < point_count; ++i)
    {
        const double x = reader->f64();
        const double y = reader->f64();
        ring.push_back({x, y});
    }
    return ring;
}

PlanarTriangulateInput decode_request(const unsigned char* request_data, std::size_t request_size)
{
    if (request_data == nullptr || request_size == 0)
    {
        throw std::runtime_error("Planar triangulate request is empty.");
    }
    BinaryReader reader(request_data, request_size);
    reader.magic(REQUEST_MAGIC);
    const std::uint32_t version = reader.u32();
    if (version != FORMAT_VERSION)
    {
        throw std::runtime_error("Unsupported planar triangulate request version.");
    }
    PlanarTriangulateInput input;
    const std::uint32_t region_count = reader.u32();
    input.options.decimal_precision = static_cast<int>(reader.u32());
    (void)reader.u32(); // reserved
    input.regions.reserve(region_count);
    for (std::uint32_t i = 0; i < region_count; ++i)
    {
        PlanarTriangulateRegion region;
        const std::uint32_t outline_count = reader.u32();
        const std::uint32_t hole_count = reader.u32();
        region.outline.reserve(outline_count);
        for (std::uint32_t j = 0; j < outline_count; ++j)
        {
            const double x = reader.f64();
            const double y = reader.f64();
            region.outline.push_back({x, y});
        }
        region.holes.reserve(hole_count);
        for (std::uint32_t h = 0; h < hole_count; ++h)
        {
            region.holes.push_back(read_ring(&reader));
        }
        input.regions.push_back(std::move(region));
    }
    reader.done();
    return input;
}

std::int64_t quantize(double value, int precision)
{
    const double scale = std::pow(10.0, static_cast<double>(precision));
    if (!std::isfinite(value))
    {
        return 0;
    }
    return static_cast<std::int64_t>(std::llround(value * scale));
}

struct PointKey
{
    std::int64_t qx = 0;
    std::int64_t qy = 0;

    bool operator==(const PointKey& other) const
    {
        return qx == other.qx && qy == other.qy;
    }
};

struct PointKeyHash
{
    std::size_t operator()(const PointKey& key) const noexcept
    {
        // Mix the two int64s with a simple hash.
        const std::uint64_t a = static_cast<std::uint64_t>(key.qx);
        const std::uint64_t b = static_cast<std::uint64_t>(key.qy);
        std::uint64_t h = a * 1469598103934665603ull;
        h ^= b + 0x9e3779b97f4a7c15ull + (h << 6u) + (h >> 2u);
        return static_cast<std::size_t>(h);
    }
};

PathD ring_to_clipper(const PlanarTriangulateRing& ring)
{
    PathD path;
    path.reserve(ring.size());
    for (const PlanarTriangulatePoint& point : ring)
    {
        path.emplace_back(point.x, point.y);
    }
    return path;
}

PlanarTriangulateRegionResult triangulate_region(const PlanarTriangulateRegion& region,
                                                 int decimal_precision)
{
    PlanarTriangulateRegionResult result;
    if (region.outline.size() < 3)
    {
        result.status = PlanarTriangulateStatus::InputTooSmall;
        return result;
    }

    // Build the merged points -> index map. Indices reference [outline, hole_0, hole_1, ...]
    // exactly as the JS consumer expects.
    std::unordered_map<PointKey, std::uint32_t, PointKeyHash> index_for_point;
    index_for_point.reserve(region.outline.size() + 8 * region.holes.size());
    std::uint32_t merged_index = 0;
    for (const PlanarTriangulatePoint& point : region.outline)
    {
        const PointKey key{quantize(point.x, decimal_precision),
                           quantize(point.y, decimal_precision)};
        // Keep the first index seen if duplicates exist.
        index_for_point.emplace(key, merged_index);
        ++merged_index;
    }
    for (const PlanarTriangulateRing& hole : region.holes)
    {
        for (const PlanarTriangulatePoint& point : hole)
        {
            const PointKey key{quantize(point.x, decimal_precision),
                               quantize(point.y, decimal_precision)};
            index_for_point.emplace(key, merged_index);
            ++merged_index;
        }
    }

    PathsD input_paths;
    input_paths.reserve(1 + region.holes.size());
    input_paths.push_back(ring_to_clipper(region.outline));
    for (const PlanarTriangulateRing& hole : region.holes)
    {
        if (hole.size() < 3)
        {
            continue;
        }
        input_paths.push_back(ring_to_clipper(hole));
    }

    PathsD triangles;
    const TriangulateResult tri_status =
        Triangulate(input_paths, decimal_precision, triangles, true);
    switch (tri_status)
    {
    case TriangulateResult::success:
        result.status = PlanarTriangulateStatus::Ok;
        break;
    case TriangulateResult::no_polygons:
        result.status = PlanarTriangulateStatus::NoPolygons;
        return result;
    case TriangulateResult::paths_intersect:
        result.status = PlanarTriangulateStatus::PathsIntersect;
        return result;
    case TriangulateResult::fail:
    default:
        result.status = PlanarTriangulateStatus::Fail;
        return result;
    }

    result.indices.reserve(triangles.size() * 3);
    for (const PathD& triangle : triangles)
    {
        if (triangle.size() != 3)
        {
            continue;
        }
        std::uint32_t resolved[3] = {0, 0, 0};
        bool ok = true;
        for (std::size_t i = 0; i < 3; ++i)
        {
            const PointKey key{quantize(triangle[i].x, decimal_precision),
                               quantize(triangle[i].y, decimal_precision)};
            const auto it = index_for_point.find(key);
            if (it == index_for_point.end())
            {
                ok = false;
                break;
            }
            resolved[i] = it->second;
        }
        if (!ok)
        {
            // Should not happen; Clipper2 Delaunay triangulates among the input vertices.
            // Drop this triangle silently rather than corrupt the index buffer.
            continue;
        }
        result.indices.push_back(resolved[0]);
        result.indices.push_back(resolved[1]);
        result.indices.push_back(resolved[2]);
    }
    return result;
}

std::vector<unsigned char> encode_response(const PlanarTriangulateResult& result)
{
    BinaryWriter writer;
    writer.bytes(RESPONSE_MAGIC, 8);
    writer.u32(FORMAT_VERSION);
    writer.u32(checked_count(result.regions.size(), "region count"));
    writer.u32(0); // reserved

    for (const PlanarTriangulateRegionResult& region : result.regions)
    {
        const std::uint32_t triangle_count =
            checked_count(region.indices.size() / 3, "triangle count");
        writer.u32(static_cast<std::uint32_t>(region.status));
        writer.u32(triangle_count);
        for (std::size_t i = 0; i < triangle_count * 3u; ++i)
        {
            writer.u32(region.indices[i]);
        }
    }
    return writer.take();
}

bool valid_options(const PlanarTriangulateOptions& options, Status* status)
{
    if (options.decimal_precision < 0 || options.decimal_precision > 8)
    {
        set_status(status, 3, "Planar triangulate decimal_precision must be between 0 and 8.");
        return false;
    }
    return true;
}

} // namespace

int triangulate_planar(const PlanarTriangulateInput& input, PlanarTriangulateResult* result,
                       Status* status)
{
    if (result == nullptr)
    {
        set_status(status, 93, "Planar triangulate result pointer is null.");
        return 93;
    }
    if (!valid_options(input.options, status))
    {
        return 3;
    }
    result->regions.clear();
    result->regions.reserve(input.regions.size());
    for (const PlanarTriangulateRegion& region : input.regions)
    {
        result->regions.push_back(triangulate_region(region, input.options.decimal_precision));
    }
    return 0;
}

int triangulate_planar_from_bytes(const unsigned char* request_data, std::size_t request_size,
                                  std::vector<unsigned char>* response_bytes, Status* status)
{
    if (response_bytes == nullptr)
    {
        set_status(status, 93, "Planar triangulate response pointer is null.");
        return 93;
    }
    response_bytes->clear();
    PlanarTriangulateInput input;
    try
    {
        input = decode_request(request_data, request_size);
    }
    catch (const std::exception& error)
    {
        set_status(status, 3, error.what());
        return 3;
    }
    PlanarTriangulateResult result;
    const int code = triangulate_planar(input, &result, status);
    if (code != 0)
    {
        return code;
    }
    try
    {
        *response_bytes = encode_response(result);
    }
    catch (const std::exception& error)
    {
        set_status(status, 3, error.what());
        return 3;
    }
    return 0;
}

} // namespace geometer

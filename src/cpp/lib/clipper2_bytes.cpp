#include "geometer/clipper2_bytes.h"

#include <clipper2/clipper.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace geometer
{
namespace
{

constexpr unsigned char BOOLEAN_REQUEST_MAGIC[8] = {'G', 'M', 'C', '2', 'B', 'Q', '0', '1'};
constexpr unsigned char BOOLEAN_RESPONSE_MAGIC[8] = {'G', 'M', 'C', '2', 'B', 'S', '0', '1'};
constexpr unsigned char INFLATE_REQUEST_MAGIC[8] = {'G', 'M', 'C', '2', 'I', 'Q', '0', '1'};
constexpr unsigned char INFLATE_RESPONSE_MAGIC[8] = {'G', 'M', 'C', '2', 'I', 'S', '0', '1'};
constexpr std::uint32_t FORMAT_VERSION = 1;

using Clipper2Lib::ClipType;
using Clipper2Lib::ClipperD;
using Clipper2Lib::EndType;
using Clipper2Lib::FillRule;
using Clipper2Lib::JoinType;
using Clipper2Lib::PathD;
using Clipper2Lib::PathsD;
using Clipper2Lib::PointD;
using Clipper2Lib::PolyPathD;
using Clipper2Lib::PolyTreeD;

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
            throw std::runtime_error("Clipper2 bytes request ended unexpectedly.");
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
            throw std::runtime_error("Clipper2 bytes request has an invalid magic value.");
        }
        offset_ += 8;
    }

    void done() const
    {
        if (offset_ != size_)
        {
            throw std::runtime_error("Clipper2 bytes request has trailing bytes.");
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

    void f64(double value)
    {
        std::uint64_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        for (std::size_t i = 0; i < 8; ++i)
        {
            data_.push_back(static_cast<unsigned char>((bits >> (8u * i)) & 0xffull));
        }
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

PathD read_path(BinaryReader* reader)
{
    const std::uint32_t point_count = reader->u32();
    PathD path;
    path.reserve(point_count);
    for (std::uint32_t i = 0; i < point_count; ++i)
    {
        const double x = reader->f64();
        const double y = reader->f64();
        path.emplace_back(x, y);
    }
    return path;
}

PathsD read_paths(BinaryReader* reader)
{
    const std::uint32_t path_count = reader->u32();
    PathsD paths;
    paths.reserve(path_count);
    for (std::uint32_t i = 0; i < path_count; ++i)
    {
        paths.push_back(read_path(reader));
    }
    return paths;
}

ClipType to_clip_type(std::uint32_t value)
{
    switch (value)
    {
    case 1:
        return ClipType::Intersection;
    case 2:
        return ClipType::Union;
    case 3:
        return ClipType::Difference;
    case 4:
        return ClipType::Xor;
    default:
        throw std::runtime_error("Clipper2 bytes request has invalid clip_type.");
    }
}

FillRule to_fill_rule(std::uint32_t value)
{
    switch (value)
    {
    case 0:
        return FillRule::EvenOdd;
    case 1:
        return FillRule::NonZero;
    case 2:
        return FillRule::Positive;
    case 3:
        return FillRule::Negative;
    default:
        throw std::runtime_error("Clipper2 bytes request has invalid fill_rule.");
    }
}

JoinType to_join_type(std::uint32_t value)
{
    switch (value)
    {
    case 0:
        return JoinType::Square;
    case 1:
        return JoinType::Bevel;
    case 2:
        return JoinType::Round;
    case 3:
        return JoinType::Miter;
    default:
        throw std::runtime_error("Clipper2 bytes request has invalid join_type.");
    }
}

EndType to_end_type(std::uint32_t value)
{
    switch (value)
    {
    case 0:
        return EndType::Polygon;
    case 1:
        return EndType::Joined;
    case 2:
        return EndType::Butt;
    case 3:
        return EndType::Square;
    case 4:
        return EndType::Round;
    default:
        throw std::runtime_error("Clipper2 bytes request has invalid end_type.");
    }
}

void append_paths(PathsD* target, const PathsD& source)
{
    if (target == nullptr || source.empty())
    {
        return;
    }
    target->reserve(target->size() + source.size());
    for (const PathD& path : source)
    {
        target->push_back(path);
    }
}

void write_path(BinaryWriter* writer, const PathD& path)
{
    writer->u32(checked_count(path.size(), "point count"));
    for (const PointD& point : path)
    {
        writer->f64(point.x);
        writer->f64(point.y);
    }
}

void write_region(BinaryWriter* writer, const PolyPathD& node)
{
    write_path(writer, node.Polygon());
    const std::size_t hole_count = node.Count();
    writer->u32(checked_count(hole_count, "hole count"));
    for (std::size_t index = 0; index < hole_count; ++index)
    {
        const PolyPathD* hole = node.Child(index);
        if (hole != nullptr)
        {
            write_path(writer, hole->Polygon());
        }
        else
        {
            writer->u32(0);
        }
    }
}

void count_regions(const PolyPathD& node, std::uint32_t* count)
{
    *count += 1;
    const std::size_t hole_count = node.Count();
    for (std::size_t index = 0; index < hole_count; ++index)
    {
        const PolyPathD* hole = node.Child(index);
        if (hole == nullptr)
        {
            continue;
        }
        const std::size_t island_count = hole->Count();
        for (std::size_t j = 0; j < island_count; ++j)
        {
            const PolyPathD* island = hole->Child(j);
            if (island != nullptr)
            {
                count_regions(*island, count);
            }
        }
    }
}

void write_regions_under(BinaryWriter* writer, const PolyPathD& node)
{
    write_region(writer, node);
    const std::size_t hole_count = node.Count();
    for (std::size_t index = 0; index < hole_count; ++index)
    {
        const PolyPathD* hole = node.Child(index);
        if (hole == nullptr)
        {
            continue;
        }
        const std::size_t island_count = hole->Count();
        for (std::size_t j = 0; j < island_count; ++j)
        {
            const PolyPathD* island = hole->Child(j);
            if (island != nullptr)
            {
                write_regions_under(writer, *island);
            }
        }
    }
}

std::uint32_t total_regions(const PolyTreeD& tree)
{
    std::uint32_t count = 0;
    const std::size_t top = tree.Count();
    for (std::size_t i = 0; i < top; ++i)
    {
        const PolyPathD* child = tree.Child(i);
        if (child != nullptr)
        {
            count_regions(*child, &count);
        }
    }
    return count;
}

void write_tree(BinaryWriter* writer, const PolyTreeD& tree)
{
    writer->u32(total_regions(tree));
    const std::size_t top = tree.Count();
    for (std::size_t i = 0; i < top; ++i)
    {
        const PolyPathD* child = tree.Child(i);
        if (child != nullptr)
        {
            write_regions_under(writer, *child);
        }
    }
}

void execute_boolean(ClipType clip_type, FillRule fill_rule, const PathsD& subjects,
                     const PathsD& clips, int decimal_precision, PolyTreeD* tree)
{
    if (tree == nullptr)
    {
        return;
    }
    tree->Clear();
    if (subjects.empty())
    {
        return;
    }
    ClipperD clipper(decimal_precision);
    clipper.AddSubject(subjects);
    if (!clips.empty())
    {
        clipper.AddClip(clips);
    }
    clipper.Execute(clip_type, fill_rule, *tree);
}

PathsD tree_to_paths(const PolyTreeD& tree)
{
    return Clipper2Lib::PolyTreeToPathsD(tree);
}

void apply_cleanup(PathsD* paths, double radius, double miter_limit, double arc_tolerance,
                   FillRule fill_rule, int decimal_precision)
{
    if (!(radius > 0.0) || paths == nullptr || paths->empty())
    {
        return;
    }

    const double safe_arc = std::max(0.0, arc_tolerance);

    const PathsD inflated = Clipper2Lib::InflatePaths(*paths, radius, JoinType::Miter,
                                                      EndType::Polygon, miter_limit,
                                                      decimal_precision, safe_arc);
    const PathsD closed = Clipper2Lib::InflatePaths(inflated, -radius, JoinType::Miter,
                                                    EndType::Polygon, miter_limit,
                                                    decimal_precision, safe_arc);

    PathsD subject = *paths;
    append_paths(&subject, closed);

    PolyTreeD tree;
    ClipperD clipper(decimal_precision);
    clipper.AddSubject(subject);
    clipper.Execute(ClipType::Union, fill_rule, tree);
    *paths = tree_to_paths(tree);
}

std::vector<unsigned char> encode_response(const unsigned char magic[8], const PolyTreeD& tree)
{
    BinaryWriter writer;
    writer.bytes(magic, 8);
    writer.u32(FORMAT_VERSION);
    writer.u32(0); // status (0 = ok)
    writer.u32(0); // reserved
    write_tree(&writer, tree);
    return writer.take();
}

std::vector<unsigned char> encode_response_from_paths(const unsigned char magic[8],
                                                      const PathsD& paths, FillRule fill_rule,
                                                      int decimal_precision)
{
    PolyTreeD tree;
    if (!paths.empty())
    {
        ClipperD clipper(decimal_precision);
        clipper.AddSubject(paths);
        clipper.Execute(ClipType::Union, fill_rule, tree);
    }
    return encode_response(magic, tree);
}

bool valid_precision(int decimal_precision, Status* status)
{
    if (decimal_precision < 0 || decimal_precision > 8)
    {
        set_status(status, 3, "Clipper2 bytes decimal_precision must be between 0 and 8.");
        return false;
    }
    return true;
}

} // namespace

int clipper2_boolean_from_bytes(const unsigned char* request_data, std::size_t request_size,
                                std::vector<unsigned char>* response_bytes, Status* status)
{
    if (response_bytes == nullptr)
    {
        set_status(status, 93, "Clipper2 boolean response pointer is null.");
        return 93;
    }
    response_bytes->clear();

    ClipType clip_type = ClipType::Union;
    FillRule fill_rule = FillRule::NonZero;
    int decimal_precision = 0;
    double cleanup_radius = 0.0;
    double cleanup_miter_limit = 2.0;
    double cleanup_arc_tolerance = 0.0;
    PathsD subjects;
    PathsD clips;

    try
    {
        if (request_data == nullptr || request_size == 0)
        {
            throw std::runtime_error("Clipper2 boolean request is empty.");
        }
        BinaryReader reader(request_data, request_size);
        reader.magic(BOOLEAN_REQUEST_MAGIC);
        const std::uint32_t version = reader.u32();
        if (version != FORMAT_VERSION)
        {
            throw std::runtime_error("Unsupported Clipper2 boolean request version.");
        }
        clip_type = to_clip_type(reader.u32());
        fill_rule = to_fill_rule(reader.u32());
        decimal_precision = static_cast<int>(reader.u32());
        cleanup_radius = reader.f64();
        cleanup_miter_limit = reader.f64();
        cleanup_arc_tolerance = reader.f64();
        (void)reader.u32(); // reserved
        subjects = read_paths(&reader);
        clips = read_paths(&reader);
        reader.done();
    }
    catch (const std::exception& error)
    {
        set_status(status, 3, error.what());
        return 3;
    }

    if (!valid_precision(decimal_precision, status))
    {
        return 3;
    }

    try
    {
        PolyTreeD tree;
        execute_boolean(clip_type, fill_rule, subjects, clips, decimal_precision, &tree);
        if (cleanup_radius > 0.0)
        {
            PathsD paths = tree_to_paths(tree);
            apply_cleanup(&paths, cleanup_radius, cleanup_miter_limit, cleanup_arc_tolerance,
                          fill_rule, decimal_precision);
            *response_bytes = encode_response_from_paths(BOOLEAN_RESPONSE_MAGIC, paths, fill_rule,
                                                        decimal_precision);
        }
        else
        {
            *response_bytes = encode_response(BOOLEAN_RESPONSE_MAGIC, tree);
        }
    }
    catch (const std::exception& error)
    {
        set_status(status, 3, error.what());
        return 3;
    }
    return 0;
}

int clipper2_inflate_open_from_bytes(const unsigned char* request_data, std::size_t request_size,
                                     std::vector<unsigned char>* response_bytes, Status* status)
{
    if (response_bytes == nullptr)
    {
        set_status(status, 93, "Clipper2 inflate-open response pointer is null.");
        return 93;
    }
    response_bytes->clear();

    JoinType join_type = JoinType::Round;
    EndType end_type = EndType::Round;
    FillRule fill_rule = FillRule::NonZero;
    int decimal_precision = 0;
    double delta = 0.0;
    double miter_limit = 2.0;
    double arc_tolerance = 0.0;
    PathsD paths;

    try
    {
        if (request_data == nullptr || request_size == 0)
        {
            throw std::runtime_error("Clipper2 inflate-open request is empty.");
        }
        BinaryReader reader(request_data, request_size);
        reader.magic(INFLATE_REQUEST_MAGIC);
        const std::uint32_t version = reader.u32();
        if (version != FORMAT_VERSION)
        {
            throw std::runtime_error("Unsupported Clipper2 inflate-open request version.");
        }
        join_type = to_join_type(reader.u32());
        end_type = to_end_type(reader.u32());
        fill_rule = to_fill_rule(reader.u32());
        decimal_precision = static_cast<int>(reader.u32());
        delta = reader.f64();
        miter_limit = reader.f64();
        arc_tolerance = reader.f64();
        (void)reader.u32(); // reserved
        paths = read_paths(&reader);
        reader.done();
    }
    catch (const std::exception& error)
    {
        set_status(status, 3, error.what());
        return 3;
    }

    if (!valid_precision(decimal_precision, status))
    {
        return 3;
    }

    try
    {
        const double safe_arc = std::max(0.0, arc_tolerance);
        const PathsD inflated = Clipper2Lib::InflatePaths(paths, delta, join_type, end_type,
                                                          miter_limit, decimal_precision,
                                                          safe_arc);
        *response_bytes = encode_response_from_paths(INFLATE_RESPONSE_MAGIC, inflated, fill_rule,
                                                    decimal_precision);
    }
    catch (const std::exception& error)
    {
        set_status(status, 3, error.what());
        return 3;
    }
    return 0;
}

} // namespace geometer

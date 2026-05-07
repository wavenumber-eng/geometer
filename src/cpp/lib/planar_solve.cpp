#include "geometer/planar_solve.h"

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

constexpr unsigned char REQUEST_MAGIC[8] = {'G', 'M', 'P', 'B', 'R', 'Q', '0', '1'};
constexpr unsigned char RESPONSE_MAGIC[8] = {'G', 'M', 'P', 'B', 'R', 'S', '0', '1'};
constexpr std::uint32_t FORMAT_VERSION = 2;
constexpr std::uint32_t JOB_SUBTRACT_COMMON_RINGS = 1u << 0u;
constexpr std::uint32_t JOB_FILTER_COMMON_SUBTRACT_BY_BOUNDS = 1u << 1u;
constexpr std::uint32_t JOB_CLIP_TO_FINAL_RINGS = 1u << 2u;
constexpr double MIN_RING_AREA_MM2 = 1.0e-12;
constexpr double POINT_EPSILON_MM = 1.0e-12;

using Clipper2Lib::ClipType;
using Clipper2Lib::FillRule;
using Clipper2Lib::PathD;
using Clipper2Lib::PathsD;
using Clipper2Lib::PolyPathD;
using Clipper2Lib::PolyTreeD;

struct Bounds2d
{
    bool valid = false;
    double min_x = 0.0;
    double min_y = 0.0;
    double max_x = 0.0;
    double max_y = 0.0;
};

void set_status(Status* status, int code, const std::string& message)
{
    if (status == nullptr)
    {
        return;
    }
    status->code = code;
    status->message = message;
}

bool finite(double value)
{
    return std::isfinite(value);
}

double numeric_or(double value, double fallback)
{
    return finite(value) ? value : fallback;
}

bool points_close(const PlanarSolvePoint& first, const PlanarSolvePoint& second)
{
    return std::fabs(first.x - second.x) <= POINT_EPSILON_MM &&
           std::fabs(first.y - second.y) <= POINT_EPSILON_MM;
}

double signed_area(const PlanarSolveRing& ring)
{
    if (ring.size() < 3)
    {
        return 0.0;
    }

    double area = 0.0;
    for (std::size_t i = 0; i < ring.size(); ++i)
    {
        const PlanarSolvePoint& current = ring[i];
        const PlanarSolvePoint& next = ring[(i + 1) % ring.size()];
        area += (current.x * next.y) - (next.x * current.y);
    }
    return area * 0.5;
}

PlanarSolveRing clean_path(const PlanarSolvePath& source, bool closed)
{
    PlanarSolveRing cleaned;
    cleaned.reserve(source.size());
    for (const PlanarSolvePoint& point : source)
    {
        if (!finite(point.x) || !finite(point.y))
        {
            continue;
        }
        if (cleaned.empty() || !points_close(cleaned.back(), point))
        {
            cleaned.push_back(point);
        }
    }

    if (closed && cleaned.size() > 1 && points_close(cleaned.front(), cleaned.back()))
    {
        cleaned.pop_back();
    }
    return cleaned;
}

PlanarSolveRing clean_ring(const PlanarSolveRing& source)
{
    PlanarSolveRing ring = clean_path(source, true);
    if (ring.size() < 3 || std::fabs(signed_area(ring)) <= MIN_RING_AREA_MM2)
    {
        ring.clear();
    }
    return ring;
}

PlanarSolvePath clean_open_path(const PlanarSolvePath& source)
{
    PlanarSolvePath path = clean_path(source, false);
    if (path.size() < 2)
    {
        path.clear();
    }
    return path;
}

PlanarSolveRing orient_ring(const PlanarSolveRing& source, bool want_positive)
{
    PlanarSolveRing ring = clean_ring(source);
    if (ring.empty())
    {
        return ring;
    }

    const bool positive = signed_area(ring) > 0.0;
    if (positive != want_positive)
    {
        std::reverse(ring.begin(), ring.end());
    }
    return ring;
}

PathD to_clipper_path(const PlanarSolvePath& source, bool closed)
{
    const PlanarSolvePath path = closed ? clean_ring(source) : clean_open_path(source);
    PathD result;
    result.reserve(path.size());
    for (const PlanarSolvePoint& point : path)
    {
        result.emplace_back(point.x, point.y);
    }
    return result;
}

PathsD to_clipper_paths(const std::vector<PlanarSolveRing>& rings)
{
    PathsD paths;
    paths.reserve(rings.size());
    for (const PlanarSolveRing& ring : rings)
    {
        PathD path = to_clipper_path(ring, true);
        if (path.size() >= 3)
        {
            paths.push_back(std::move(path));
        }
    }
    return paths;
}

PathsD to_clipper_open_paths(const std::vector<PlanarSolvePath>& source_paths)
{
    PathsD paths;
    paths.reserve(source_paths.size());
    for (const PlanarSolvePath& source : source_paths)
    {
        PathD path = to_clipper_path(source, false);
        if (path.size() >= 2)
        {
            paths.push_back(std::move(path));
        }
    }
    return paths;
}

PlanarSolveRing from_clipper_path(const PathD& path)
{
    PlanarSolveRing ring;
    ring.reserve(path.size());
    for (const auto& point : path)
    {
        ring.push_back({point.x, point.y});
    }
    return clean_ring(ring);
}

Clipper2Lib::JoinType to_clipper_join_type(PlanarSolveJoinType join_type)
{
    switch (join_type)
    {
    case PlanarSolveJoinType::Round:
        return Clipper2Lib::JoinType::Round;
    case PlanarSolveJoinType::Bevel:
        return Clipper2Lib::JoinType::Bevel;
    case PlanarSolveJoinType::Square:
        return Clipper2Lib::JoinType::Square;
    case PlanarSolveJoinType::Miter:
    default:
        return Clipper2Lib::JoinType::Miter;
    }
}

Clipper2Lib::EndType to_clipper_end_type(PlanarSolveEndType end_type)
{
    switch (end_type)
    {
    case PlanarSolveEndType::Square:
        return Clipper2Lib::EndType::Square;
    case PlanarSolveEndType::Butt:
        return Clipper2Lib::EndType::Butt;
    case PlanarSolveEndType::Joined:
        return Clipper2Lib::EndType::Joined;
    case PlanarSolveEndType::Round:
    default:
        return Clipper2Lib::EndType::Round;
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

void append_poly_path_region(const PolyPathD& node, std::vector<PlanarSolveRegion>* regions)
{
    PlanarSolveRing outline = orient_ring(from_clipper_path(node.Polygon()), true);
    if (outline.size() < 3)
    {
        return;
    }

    PlanarSolveRegion region;
    region.outline = std::move(outline);

    const std::size_t child_count = node.Count();
    region.holes.reserve(child_count);
    for (std::size_t index = 0; index < child_count; ++index)
    {
        const PolyPathD* child = node.Child(index);
        if (child == nullptr)
        {
            continue;
        }
        PlanarSolveRing hole = orient_ring(from_clipper_path(child->Polygon()), false);
        if (hole.size() >= 3)
        {
            region.holes.push_back(std::move(hole));
        }
    }

    regions->push_back(std::move(region));

    for (std::size_t index = 0; index < child_count; ++index)
    {
        const PolyPathD* hole = node.Child(index);
        if (hole == nullptr)
        {
            continue;
        }
        for (std::size_t island = 0; island < hole->Count(); ++island)
        {
            const PolyPathD* island_node = hole->Child(island);
            if (island_node != nullptr)
            {
                append_poly_path_region(*island_node, regions);
            }
        }
    }
}

std::vector<PlanarSolveRegion> regions_from_tree(const PolyTreeD& tree)
{
    std::vector<PlanarSolveRegion> regions;
    regions.reserve(tree.Count());
    for (std::size_t index = 0; index < tree.Count(); ++index)
    {
        const PolyPathD* child = tree.Child(index);
        if (child != nullptr)
        {
            append_poly_path_region(*child, &regions);
        }
    }
    return regions;
}

std::vector<PlanarSolveRing> rings_from_regions(const std::vector<PlanarSolveRegion>& regions)
{
    std::vector<PlanarSolveRing> rings;
    for (const PlanarSolveRegion& region : regions)
    {
        PlanarSolveRing outline = orient_ring(region.outline, true);
        if (outline.size() >= 3)
        {
            rings.push_back(std::move(outline));
        }
        for (const PlanarSolveRing& hole_source : region.holes)
        {
            PlanarSolveRing hole = orient_ring(hole_source, false);
            if (hole.size() >= 3)
            {
                rings.push_back(std::move(hole));
            }
        }
    }
    return rings;
}

double regions_area(const std::vector<PlanarSolveRegion>& regions)
{
    double area = 0.0;
    for (const PlanarSolveRegion& region : regions)
    {
        double region_area = std::fabs(signed_area(region.outline));
        for (const PlanarSolveRing& hole : region.holes)
        {
            region_area -= std::fabs(signed_area(hole));
        }
        area += std::max(0.0, region_area);
    }
    return area;
}

Bounds2d bounds_for_ring(const PlanarSolveRing& source)
{
    Bounds2d bounds;
    const PlanarSolveRing ring = clean_ring(source);
    for (const PlanarSolvePoint& point : ring)
    {
        if (!bounds.valid)
        {
            bounds.valid = true;
            bounds.min_x = point.x;
            bounds.max_x = point.x;
            bounds.min_y = point.y;
            bounds.max_y = point.y;
            continue;
        }
        bounds.min_x = std::min(bounds.min_x, point.x);
        bounds.max_x = std::max(bounds.max_x, point.x);
        bounds.min_y = std::min(bounds.min_y, point.y);
        bounds.max_y = std::max(bounds.max_y, point.y);
    }
    return bounds;
}

Bounds2d merge_bounds(Bounds2d target, const Bounds2d& source)
{
    if (!source.valid)
    {
        return target;
    }
    if (!target.valid)
    {
        return source;
    }
    target.min_x = std::min(target.min_x, source.min_x);
    target.max_x = std::max(target.max_x, source.max_x);
    target.min_y = std::min(target.min_y, source.min_y);
    target.max_y = std::max(target.max_y, source.max_y);
    return target;
}

Bounds2d bounds_for_rings(const std::vector<PlanarSolveRing>& rings)
{
    Bounds2d bounds;
    for (const PlanarSolveRing& ring : rings)
    {
        bounds = merge_bounds(bounds, bounds_for_ring(ring));
    }
    return bounds;
}

std::uint32_t diagnostic_count(std::size_t value)
{
    return value > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())
               ? std::numeric_limits<std::uint32_t>::max()
               : static_cast<std::uint32_t>(value);
}

Bounds2d expand_bounds(Bounds2d bounds, double margin)
{
    if (!bounds.valid)
    {
        return bounds;
    }
    const double safe_margin = std::max(0.0, numeric_or(margin, 0.0));
    bounds.min_x -= safe_margin;
    bounds.max_x += safe_margin;
    bounds.min_y -= safe_margin;
    bounds.max_y += safe_margin;
    return bounds;
}

bool bounds_overlap(const Bounds2d& first, const Bounds2d& second)
{
    if (!first.valid || !second.valid)
    {
        return false;
    }
    return !(first.max_x < second.min_x || second.max_x < first.min_x ||
             first.max_y < second.min_y || second.max_y < first.min_y);
}

PathsD offset_stroke_group(const PlanarSolveStrokeGroup& group,
                           const PlanarBatchSolveOptions& options)
{
    const double radius = numeric_or(group.radius_mm, 0.0);
    if (!(radius > 0.0))
    {
        return {};
    }

    const PathsD open_paths = to_clipper_open_paths(group.paths);
    if (open_paths.empty())
    {
        return {};
    }

    return Clipper2Lib::InflatePaths(
        open_paths, radius, to_clipper_join_type(group.join_type),
        to_clipper_end_type(group.end_type), numeric_or(group.miter_limit, 2.0),
        options.decimal_precision, std::max(0.0, numeric_or(group.arc_tolerance_mm, 0.0)));
}

std::vector<PlanarSolveRegion> execute_boolean(ClipType clip_type, const PathsD& subject_paths,
                                               const PathsD& clip_paths,
                                               const PlanarBatchSolveOptions& options)
{
    if (subject_paths.empty())
    {
        return {};
    }

    Clipper2Lib::ClipperD clipper(options.decimal_precision);
    clipper.AddSubject(subject_paths);
    if (!clip_paths.empty())
    {
        clipper.AddClip(clip_paths);
    }

    PolyTreeD tree;
    clipper.Execute(clip_type, FillRule::NonZero, tree);
    return regions_from_tree(tree);
}

std::vector<PlanarSolveRegion> cleanup_regions(const std::vector<PlanarSolveRegion>& regions,
                                               const PlanarBatchSolveOptions& options)
{
    const double radius = std::max(0.0, numeric_or(options.cleanup_radius_mm, 0.0));
    if (!(radius > 0.0) || regions.empty())
    {
        return regions;
    }

    const PathsD exact = to_clipper_paths(rings_from_regions(regions));
    if (exact.empty())
    {
        return regions;
    }

    const PathsD inflated = Clipper2Lib::InflatePaths(
        exact, radius, Clipper2Lib::JoinType::Miter, Clipper2Lib::EndType::Polygon,
        numeric_or(options.cleanup_miter_limit, 2.0), options.decimal_precision,
        std::max(0.0, numeric_or(options.cleanup_arc_tolerance_mm, 0.0)));
    const PathsD closed = Clipper2Lib::InflatePaths(
        inflated, -radius, Clipper2Lib::JoinType::Miter, Clipper2Lib::EndType::Polygon,
        numeric_or(options.cleanup_miter_limit, 2.0), options.decimal_precision,
        std::max(0.0, numeric_or(options.cleanup_arc_tolerance_mm, 0.0)));

    PathsD cleanup_subject = exact;
    append_paths(&cleanup_subject, closed);
    return execute_boolean(ClipType::Union, cleanup_subject, {}, options);
}

std::vector<PlanarSolveRing> common_subtract_rings_for_job(const PlanarSolveJob& job,
                                                           const std::vector<PlanarSolveRing>& common,
                                                           const std::vector<PlanarSolveRing>& subject,
                                                           const PlanarBatchSolveOptions& options)
{
    if (!job.subtract_common_rings || common.empty())
    {
        return {};
    }

    if (!job.filter_common_subtract_by_bounds)
    {
        return common;
    }

    const double margin = std::max(0.001, std::max(numeric_or(job.common_subtract_filter_margin_mm, 0.0),
                                                   numeric_or(options.cleanup_radius_mm, 0.0)));
    const Bounds2d subject_bounds = expand_bounds(bounds_for_rings(subject), margin);
    if (!subject_bounds.valid)
    {
        return common;
    }

    std::vector<PlanarSolveRing> filtered;
    for (const PlanarSolveRing& ring : common)
    {
        if (bounds_overlap(subject_bounds, bounds_for_ring(ring)))
        {
            filtered.push_back(ring);
        }
    }
    return filtered;
}

PlanarSolveJobResult solve_job(const PlanarSolveJob& job, const PlanarBatchSolveInput& input)
{
    PlanarSolveJobResult result;
    result.raw_subject_ring_count = diagnostic_count(job.subject_rings.size());
    result.local_subtract_ring_count = diagnostic_count(job.subtract_rings.size());

    std::vector<PlanarSolveRing> subject_rings = job.subject_rings;
    for (const PlanarSolveStrokeGroup& stroke_group : job.stroke_groups)
    {
        result.stroke_path_count += diagnostic_count(stroke_group.paths.size());
        const PathsD stroke_paths = offset_stroke_group(stroke_group, input.options);
        const std::vector<PlanarSolveRegion> stroke_regions =
            execute_boolean(ClipType::Union, stroke_paths, {}, input.options);
        result.stroke_region_count += diagnostic_count(stroke_regions.size());
        const std::vector<PlanarSolveRing> stroke_rings = rings_from_regions(stroke_regions);
        subject_rings.reserve(subject_rings.size() + stroke_rings.size());
        for (const PlanarSolveRing& ring : stroke_rings)
        {
            subject_rings.push_back(ring);
        }
    }
    result.source_subject_ring_count = diagnostic_count(subject_rings.size());

    PathsD subject_paths = to_clipper_paths(subject_rings);
    if (subject_paths.empty())
    {
        return result;
    }

    std::vector<PlanarSolveRegion> regions =
        execute_boolean(ClipType::Union, subject_paths, {}, input.options);
    regions = cleanup_regions(regions, input.options);

    std::vector<PlanarSolveRing> subtract_rings = job.subtract_rings;
    const std::vector<PlanarSolveRing> prepared_subject = rings_from_regions(regions);
    std::vector<PlanarSolveRing> common_rings =
        common_subtract_rings_for_job(job, input.common_subtract_rings, prepared_subject,
                                      input.options);
    result.common_subtract_ring_count = diagnostic_count(common_rings.size());
    subtract_rings.reserve(subtract_rings.size() + common_rings.size());
    for (const PlanarSolveRing& ring : common_rings)
    {
        subtract_rings.push_back(ring);
    }

    if (!subtract_rings.empty())
    {
        const PathsD difference_subject = to_clipper_paths(rings_from_regions(regions));
        regions =
            execute_boolean(ClipType::Difference, difference_subject, to_clipper_paths(subtract_rings),
                            input.options);
        regions = cleanup_regions(regions, input.options);
    }

    if (job.clip_to_final_rings && !input.final_clip_rings.empty())
    {
        PlanarBatchSolveOptions clip_options = input.options;
        clip_options.cleanup_radius_mm = 0.0;
        const PathsD clip_subject = to_clipper_paths(rings_from_regions(regions));
        regions = execute_boolean(ClipType::Intersection, clip_subject,
                                  to_clipper_paths(input.final_clip_rings), clip_options);
    }

    result.area_mm2 = regions_area(regions);
    result.regions = std::move(regions);
    return result;
}

class BinaryReader
{
public:
    BinaryReader(const unsigned char* data, std::size_t size) : data_(data), size_(size) {}

    void require(std::size_t count)
    {
        if (offset_ > size_ || count > size_ - offset_)
        {
            throw std::runtime_error("Planar solve request ended unexpectedly.");
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
            throw std::runtime_error("Planar solve request has an invalid magic value.");
        }
        offset_ += 8;
    }

    void done() const
    {
        if (offset_ != size_)
        {
            throw std::runtime_error("Planar solve request has trailing bytes.");
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
        static_assert(sizeof(value) == sizeof(bits), "double size must be 8 bytes");
        std::memcpy(&bits, &value, sizeof(value));
        for (std::size_t i = 0; i < 8; ++i)
        {
            data_.push_back(static_cast<unsigned char>((bits >> (8u * i)) & 0xffu));
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

PlanarSolveRing read_path(BinaryReader* reader, bool closed)
{
    const std::uint32_t point_count = reader->u32();
    PlanarSolvePath path;
    path.reserve(point_count);
    for (std::uint32_t i = 0; i < point_count; ++i)
    {
        path.push_back({reader->f64(), reader->f64()});
    }
    return closed ? clean_ring(path) : clean_open_path(path);
}

std::vector<PlanarSolveRing> read_rings(BinaryReader* reader, std::uint32_t count)
{
    std::vector<PlanarSolveRing> rings;
    rings.reserve(count);
    for (std::uint32_t i = 0; i < count; ++i)
    {
        PlanarSolveRing ring = read_path(reader, true);
        if (ring.size() >= 3)
        {
            rings.push_back(std::move(ring));
        }
    }
    return rings;
}

PlanarSolveStrokeGroup read_stroke_group(BinaryReader* reader)
{
    PlanarSolveStrokeGroup group;
    group.radius_mm = reader->f64();
    group.miter_limit = reader->f64();
    group.arc_tolerance_mm = reader->f64();
    group.join_type = static_cast<PlanarSolveJoinType>(reader->u32());
    group.end_type = static_cast<PlanarSolveEndType>(reader->u32());
    const std::uint32_t path_count = reader->u32();
    (void)reader->u32();
    group.paths.reserve(path_count);
    for (std::uint32_t i = 0; i < path_count; ++i)
    {
        PlanarSolvePath path = read_path(reader, false);
        if (path.size() >= 2)
        {
            group.paths.push_back(std::move(path));
        }
    }
    return group;
}

PlanarBatchSolveInput decode_request(const unsigned char* request_data, std::size_t request_size)
{
    if (request_data == nullptr || request_size == 0)
    {
        throw std::runtime_error("Planar solve request is empty.");
    }

    BinaryReader reader(request_data, request_size);
    reader.magic(REQUEST_MAGIC);
    const std::uint32_t version = reader.u32();
    if (version != FORMAT_VERSION)
    {
        throw std::runtime_error("Unsupported planar solve request version.");
    }

    PlanarBatchSolveInput input;
    (void)reader.u32();
    input.options.decimal_precision = static_cast<int>(reader.u32());
    const std::uint32_t job_count = reader.u32();
    input.options.cleanup_radius_mm = reader.f64();
    input.options.cleanup_miter_limit = reader.f64();
    input.options.cleanup_arc_tolerance_mm = reader.f64();
    const std::uint32_t common_subtract_count = reader.u32();
    const std::uint32_t final_clip_count = reader.u32();
    (void)reader.u32();
    (void)reader.u32();

    input.common_subtract_rings = read_rings(&reader, common_subtract_count);
    input.final_clip_rings = read_rings(&reader, final_clip_count);
    input.jobs.reserve(job_count);

    for (std::uint32_t job_index = 0; job_index < job_count; ++job_index)
    {
        PlanarSolveJob job;
        const std::uint32_t flags = reader.u32();
        job.subtract_common_rings = (flags & JOB_SUBTRACT_COMMON_RINGS) != 0u;
        job.filter_common_subtract_by_bounds =
            (flags & JOB_FILTER_COMMON_SUBTRACT_BY_BOUNDS) != 0u;
        job.clip_to_final_rings = (flags & JOB_CLIP_TO_FINAL_RINGS) != 0u;
        job.common_subtract_filter_margin_mm = reader.f64();
        const std::uint32_t subject_count = reader.u32();
        const std::uint32_t local_subtract_count = reader.u32();
        const std::uint32_t stroke_group_count = reader.u32();
        (void)reader.u32();

        job.subject_rings = read_rings(&reader, subject_count);
        job.subtract_rings = read_rings(&reader, local_subtract_count);
        job.stroke_groups.reserve(stroke_group_count);
        for (std::uint32_t stroke_group_index = 0; stroke_group_index < stroke_group_count;
             ++stroke_group_index)
        {
            PlanarSolveStrokeGroup group = read_stroke_group(&reader);
            if (!group.paths.empty() && group.radius_mm > 0.0)
            {
                job.stroke_groups.push_back(std::move(group));
            }
        }
        input.jobs.push_back(std::move(job));
    }
    reader.done();
    return input;
}

void write_ring(BinaryWriter* writer, const PlanarSolveRing& source)
{
    const PlanarSolveRing ring = clean_ring(source);
    writer->u32(checked_count(ring.size(), "ring point count"));
    for (const PlanarSolvePoint& point : ring)
    {
        writer->f64(point.x);
        writer->f64(point.y);
    }
}

std::vector<unsigned char> encode_response(const PlanarBatchSolveResult& result)
{
    std::size_t region_count = 0;
    std::size_t ring_count = 0;
    std::size_t point_count = 0;
    for (const PlanarSolveJobResult& job : result.jobs)
    {
        region_count += job.regions.size();
        for (const PlanarSolveRegion& region : job.regions)
        {
            ++ring_count;
            point_count += region.outline.size();
            ring_count += region.holes.size();
            for (const PlanarSolveRing& hole : region.holes)
            {
                point_count += hole.size();
            }
        }
    }

    BinaryWriter writer;
    writer.bytes(RESPONSE_MAGIC, 8);
    writer.u32(FORMAT_VERSION);
    writer.u32(checked_count(result.jobs.size(), "job count"));
    writer.u32(checked_count(region_count, "region count"));
    writer.u32(checked_count(ring_count, "ring count"));
    writer.u32(checked_count(point_count, "point count"));
    writer.u32(0);

    for (const PlanarSolveJobResult& job : result.jobs)
    {
        std::size_t job_ring_count = 0;
        std::size_t job_point_count = 0;
        for (const PlanarSolveRegion& region : job.regions)
        {
            ++job_ring_count;
            job_point_count += region.outline.size();
            job_ring_count += region.holes.size();
            for (const PlanarSolveRing& hole : region.holes)
            {
                job_point_count += hole.size();
            }
        }

        writer.u32(checked_count(job.regions.size(), "job region count"));
        writer.u32(checked_count(job_ring_count, "job ring count"));
        writer.u32(checked_count(job_point_count, "job point count"));
        writer.u32(job.source_subject_ring_count);
        writer.f64(job.area_mm2);
        writer.u32(job.raw_subject_ring_count);
        writer.u32(job.stroke_path_count);
        writer.u32(job.stroke_region_count);
        writer.u32(job.local_subtract_ring_count);
        writer.u32(job.common_subtract_ring_count);
        writer.u32(0);

        for (const PlanarSolveRegion& region : job.regions)
        {
            writer.u32(checked_count(region.holes.size(), "hole count"));
            writer.u32(0);
            write_ring(&writer, region.outline);
            for (const PlanarSolveRing& hole : region.holes)
            {
                write_ring(&writer, hole);
            }
        }
    }
    return writer.take();
}

bool valid_options(const PlanarBatchSolveOptions& options, Status* status)
{
    if (options.decimal_precision < 0 || options.decimal_precision > 8)
    {
        set_status(status, 3, "Planar batch decimal_precision must be between 0 and 8.");
        return false;
    }
    if (!finite(options.cleanup_radius_mm) || options.cleanup_radius_mm < 0.0)
    {
        set_status(status, 3, "Planar batch cleanup_radius_mm must be finite and non-negative.");
        return false;
    }
    return true;
}

} // namespace

int solve_planar_batch(const PlanarBatchSolveInput& input, PlanarBatchSolveResult* result,
                       Status* status)
{
    if (result == nullptr)
    {
        set_status(status, 2, "Planar batch result pointer is null.");
        return 2;
    }
    result->jobs.clear();

    if (!valid_options(input.options, status))
    {
        return status == nullptr ? 3 : status->code;
    }

    try
    {
        result->jobs.reserve(input.jobs.size());
        for (const PlanarSolveJob& job : input.jobs)
        {
            result->jobs.push_back(solve_job(job, input));
        }
    }
    catch (const std::exception& error)
    {
        set_status(status, 4, error.what());
        return 4;
    }

    set_status(status, 0, "");
    return 0;
}

int solve_planar_batch_from_bytes(const unsigned char* request_data, std::size_t request_size,
                                  std::vector<unsigned char>* response_bytes, Status* status)
{
    if (response_bytes == nullptr)
    {
        set_status(status, 2, "Planar batch response pointer is null.");
        return 2;
    }
    response_bytes->clear();

    try
    {
        PlanarBatchSolveInput input = decode_request(request_data, request_size);
        PlanarBatchSolveResult result;
        const int code = solve_planar_batch(input, &result, status);
        if (code != 0)
        {
            return code;
        }
        *response_bytes = encode_response(result);
    }
    catch (const std::exception& error)
    {
        set_status(status, 4, error.what());
        return 4;
    }

    set_status(status, 0, "");
    return 0;
}

} // namespace geometer

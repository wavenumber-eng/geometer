#include "geometer/planar_step.h"

#include "geometer/planar_solve.h"

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRep_Builder.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <Message_ProgressRange.hxx>
#include <Quantity_Color.hxx>
#include <STEPCAFControl_Writer.hxx>
#include <STEPControl_StepModelType.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TDF_Label.hxx>
#include <TDataStd_Name.hxx>
#include <TDocStd_Document.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Wire.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace geometer
{
namespace
{

constexpr double kPi = 3.141592653589793238462643383279502884;
constexpr double kTwoPi = 2.0 * kPi;
constexpr double kEpsilon = 1e-7;

struct Point2
{
    double x = 0.0;
    double y = 0.0;
};

struct Color
{
    double r = 0.72;
    double g = 0.45;
    double b = 0.2;
};

struct Segment2
{
    std::string kind = "line";
    bool ccw = true;
    bool has_center = false;
    Point2 center;
    bool has_radius = false;
    double radius = 0.0;
};

struct Ring
{
    std::vector<Point2> points;
    std::vector<Segment2> segments;
};

struct Region
{
    Ring outer;
    std::vector<Ring> holes;
};

struct BodySpec
{
    std::string id;
    std::string name;
    Color color;
    double z_mm = 0.0;
    double thickness_mm = 0.035;
    bool fuse_regions = false;
    std::vector<Region> regions;
    std::vector<Region> cutouts;
};

struct Request
{
    std::string name = "planar_step";
    double unit_scale_to_mm = 1.0;
    std::vector<BodySpec> bodies;
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

std::string temp_planar_step_path(const char* extension)
{
    static std::atomic<unsigned long> counter{0};
    const unsigned long id = ++counter;
    return std::string("geometer_planar_step_") + std::to_string(id) + extension;
}

bool read_binary_file(const std::string& path, std::vector<unsigned char>* bytes)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
    {
        return false;
    }
    bytes->assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    return input.good() || input.eof();
}

bool is_number(const rapidjson::Value& value)
{
    return value.IsNumber() && std::isfinite(value.GetDouble());
}

const rapidjson::Value* member(const rapidjson::Value& object, const char* name)
{
    if (!object.IsObject())
    {
        return nullptr;
    }
    const auto it = object.FindMember(name);
    if (it == object.MemberEnd())
    {
        return nullptr;
    }
    return &it->value;
}

const rapidjson::Value* first_member(const rapidjson::Value& object,
                                     const std::vector<const char*>& names)
{
    for (const char* name : names)
    {
        const rapidjson::Value* value = member(object, name);
        if (value != nullptr)
        {
            return value;
        }
    }
    return nullptr;
}

std::string string_member(const rapidjson::Value& object, const char* name,
                          const std::string& default_value)
{
    const rapidjson::Value* value = member(object, name);
    if (value == nullptr || !value->IsString())
    {
        return default_value;
    }
    return value->GetString();
}

bool bool_member(const rapidjson::Value& object, const std::vector<const char*>& names,
                 bool default_value)
{
    const rapidjson::Value* value = first_member(object, names);
    if (value == nullptr || !value->IsBool())
    {
        return default_value;
    }
    return value->GetBool();
}

double unit_scale_to_mm(const std::string& units)
{
    std::string normalized = units;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    if (normalized == "nm" || normalized == "nanometer" || normalized == "nanometers")
    {
        return 0.000001;
    }
    if (normalized == "mil" || normalized == "mils")
    {
        return 0.0254;
    }
    if (normalized == "inch" || normalized == "inches" || normalized == "in")
    {
        return 25.4;
    }
    return 1.0;
}

bool parse_length_value(const rapidjson::Value& value, double default_scale, double* output,
                        std::string* error, const std::string& label)
{
    if (is_number(value))
    {
        *output = value.GetDouble() * default_scale;
        return true;
    }
    if (!value.IsObject())
    {
        *error = label + " must be a number or length object";
        return false;
    }
    const rapidjson::Value* mm = member(value, "mm");
    if (mm != nullptr && is_number(*mm))
    {
        *output = mm->GetDouble();
        return true;
    }
    const rapidjson::Value* nm = member(value, "nm");
    if (nm != nullptr && is_number(*nm))
    {
        *output = nm->GetDouble() * 0.000001;
        return true;
    }
    const rapidjson::Value* mils = member(value, "mils");
    if (mils == nullptr)
    {
        mils = member(value, "mil");
    }
    if (mils != nullptr && is_number(*mils))
    {
        *output = mils->GetDouble() * 0.0254;
        return true;
    }
    *error = label + " length object must contain mm, nm, or mils";
    return false;
}

bool parse_length_member(const rapidjson::Value& object, const std::vector<const char*>& names,
                         double default_scale, double default_value, double* output,
                         std::string* error, const std::string& label)
{
    const rapidjson::Value* value = first_member(object, names);
    if (value == nullptr || value->IsNull())
    {
        *output = default_value;
        return true;
    }
    if (!parse_length_value(*value, default_scale, output, error, label))
    {
        return false;
    }

    if (names.size() == 1)
    {
        return true;
    }
    return true;
}

bool parse_suffixed_length_member(const rapidjson::Value& object, const char* base_name,
                                  double default_scale, double default_value, double* output,
                                  std::string* error)
{
    const std::string base(base_name);
    const std::vector<const char*> direct_names = {base_name};
    const rapidjson::Value* direct = first_member(object, direct_names);
    if (direct != nullptr && !direct->IsNull())
    {
        return parse_length_value(*direct, default_scale, output, error, base);
    }

    const std::string mm_name = base + "_mm";
    const rapidjson::Value* mm = member(object, mm_name.c_str());
    if (mm != nullptr && is_number(*mm))
    {
        *output = mm->GetDouble();
        return true;
    }
    const std::string nm_name = base + "_nm";
    const rapidjson::Value* nm = member(object, nm_name.c_str());
    if (nm != nullptr && is_number(*nm))
    {
        *output = nm->GetDouble() * 0.000001;
        return true;
    }
    const std::string mils_name = base + "_mils";
    const rapidjson::Value* mils = member(object, mils_name.c_str());
    if (mils != nullptr && is_number(*mils))
    {
        *output = mils->GetDouble() * 0.0254;
        return true;
    }
    *output = default_value;
    return true;
}

bool parse_point(const rapidjson::Value& value, double default_scale, Point2* point,
                 std::string* error, const std::string& label)
{
    if (value.IsArray() && value.Size() >= 2 && is_number(value[0]) && is_number(value[1]))
    {
        point->x = value[0].GetDouble() * default_scale;
        point->y = value[1].GetDouble() * default_scale;
        return true;
    }
    if (!value.IsObject())
    {
        *error = label + " must be a point array or object";
        return false;
    }

    const rapidjson::Value* x_mm = member(value, "x_mm");
    const rapidjson::Value* y_mm = member(value, "y_mm");
    if (x_mm != nullptr || y_mm != nullptr)
    {
        if (x_mm == nullptr || y_mm == nullptr || !is_number(*x_mm) || !is_number(*y_mm))
        {
            *error = label + " x_mm/y_mm must both be numeric";
            return false;
        }
        point->x = x_mm->GetDouble();
        point->y = y_mm->GetDouble();
        return true;
    }

    const rapidjson::Value* x_nm = member(value, "x_nm");
    const rapidjson::Value* y_nm = member(value, "y_nm");
    if (x_nm != nullptr || y_nm != nullptr)
    {
        if (x_nm == nullptr || y_nm == nullptr || !is_number(*x_nm) || !is_number(*y_nm))
        {
            *error = label + " x_nm/y_nm must both be numeric";
            return false;
        }
        point->x = x_nm->GetDouble() * 0.000001;
        point->y = y_nm->GetDouble() * 0.000001;
        return true;
    }

    const rapidjson::Value* x_value = member(value, "x");
    const rapidjson::Value* y_value = member(value, "y");
    if (x_value == nullptr || y_value == nullptr || !is_number(*x_value) || !is_number(*y_value))
    {
        *error = label + " must contain numeric x/y coordinates";
        return false;
    }
    point->x = x_value->GetDouble() * default_scale;
    point->y = y_value->GetDouble() * default_scale;
    return true;
}

bool parse_color(const rapidjson::Value& value, Color* color, std::string* error)
{
    if (!value.IsString())
    {
        *error = "color must be #RRGGBB or #AARRGGBB";
        return false;
    }
    std::string text = value.GetString();
    if (!text.empty() && text[0] == '#')
    {
        text.erase(text.begin());
    }
    if (text.rfind("0x", 0) == 0 || text.rfind("0X", 0) == 0)
    {
        text.erase(0, 2);
    }
    if (text.size() == 8)
    {
        text.erase(0, 2);
    }
    if (text.size() != 6)
    {
        *error = "color must be #RRGGBB or #AARRGGBB";
        return false;
    }
    unsigned int value_rgb = 0;
    std::istringstream input(text);
    input >> std::hex >> value_rgb;
    if (!input)
    {
        *error = "color contains invalid hex digits";
        return false;
    }
    color->r = static_cast<double>((value_rgb >> 16) & 0xFF) / 255.0;
    color->g = static_cast<double>((value_rgb >> 8) & 0xFF) / 255.0;
    color->b = static_cast<double>(value_rgb & 0xFF) / 255.0;
    return true;
}

bool parse_segment(const rapidjson::Value& value, double default_scale, Segment2* segment,
                   std::string* error, const std::string& label)
{
    if (!value.IsObject())
    {
        *error = label + " segment must be an object";
        return false;
    }
    segment->kind = string_member(value, "kind", string_member(value, "type", "line"));
    std::transform(segment->kind.begin(), segment->kind.end(), segment->kind.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    if (segment->kind.empty())
    {
        segment->kind = "line";
    }

    const rapidjson::Value* center = member(value, "center");
    if (center != nullptr && !center->IsNull())
    {
        if (!parse_point(*center, default_scale, &segment->center, error, label + ".center"))
        {
            return false;
        }
        segment->has_center = true;
    }

    double radius = 0.0;
    if (!parse_suffixed_length_member(value, "radius", default_scale, -1.0, &radius, error))
    {
        return false;
    }
    if (radius > 0.0)
    {
        segment->radius = radius;
        segment->has_radius = true;
    }

    const rapidjson::Value* sweep = member(value, "sweep");
    if (sweep != nullptr && sweep->IsString())
    {
        std::string text = sweep->GetString();
        std::transform(text.begin(), text.end(), text.begin(),
                       [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        segment->ccw = text != "cw" && text != "clockwise";
    }
    else
    {
        const rapidjson::Value* clockwise = member(value, "clockwise");
        if (clockwise != nullptr && clockwise->IsBool())
        {
            segment->ccw = !clockwise->GetBool();
        }
    }
    return true;
}

bool points_close(const Point2& a, const Point2& b)
{
    return std::abs(a.x - b.x) <= kEpsilon && std::abs(a.y - b.y) <= kEpsilon;
}

bool parse_ring_from_points(const rapidjson::Value& value, double default_scale, Ring* ring,
                            std::string* error, const std::string& label)
{
    const rapidjson::Value* points = member(value, "points");
    if (points == nullptr || !points->IsArray())
    {
        *error = label + " requires points array";
        return false;
    }
    for (rapidjson::SizeType i = 0; i < points->Size(); ++i)
    {
        Point2 point;
        if (!parse_point((*points)[i], default_scale, &point, error,
                         label + ".points[" + std::to_string(i) + "]"))
        {
            return false;
        }
        if (ring->points.empty() || !points_close(ring->points.back(), point))
        {
            ring->points.push_back(point);
        }
    }
    if (ring->points.size() > 1 && points_close(ring->points.front(), ring->points.back()))
    {
        ring->points.pop_back();
    }
    if (ring->points.size() < 3)
    {
        *error = label + " must contain at least three unique points";
        return false;
    }

    const rapidjson::Value* segments = member(value, "segments");
    if (segments == nullptr || segments->IsNull())
    {
        ring->segments.assign(ring->points.size(), Segment2{});
        return true;
    }
    if (!segments->IsArray() || segments->Size() != ring->points.size())
    {
        *error = label + " segments length must match points length";
        return false;
    }
    for (rapidjson::SizeType i = 0; i < segments->Size(); ++i)
    {
        Segment2 segment;
        if (!parse_segment((*segments)[i], default_scale, &segment, error,
                           label + ".segments[" + std::to_string(i) + "]"))
        {
            return false;
        }
        ring->segments.push_back(segment);
    }
    return true;
}

bool parse_ring_from_geom_contour(const rapidjson::Value& value, double default_scale, Ring* ring,
                                  std::string* error, const std::string& label)
{
    const rapidjson::Value* start_value = member(value, "start");
    const rapidjson::Value* segments_value = member(value, "segments");
    if (start_value == nullptr || segments_value == nullptr || !segments_value->IsArray())
    {
        *error = label + " requires either points[] or GeomContour start/segments";
        return false;
    }

    Point2 start;
    if (!parse_point(*start_value, default_scale, &start, error, label + ".start"))
    {
        return false;
    }
    ring->points.push_back(start);
    for (rapidjson::SizeType i = 0; i < segments_value->Size(); ++i)
    {
        const rapidjson::Value& raw_segment = (*segments_value)[i];
        Segment2 segment;
        if (!parse_segment(raw_segment, default_scale, &segment, error,
                           label + ".segments[" + std::to_string(i) + "]"))
        {
            return false;
        }
        const rapidjson::Value* end_value = member(raw_segment, "end");
        if (end_value == nullptr || end_value->IsNull())
        {
            *error = label + ".segments[" + std::to_string(i) + "] requires end";
            return false;
        }
        Point2 end;
        if (!parse_point(*end_value, default_scale, &end, error,
                         label + ".segments[" + std::to_string(i) + "].end"))
        {
            return false;
        }
        ring->segments.push_back(segment);
        ring->points.push_back(end);
    }

    if (ring->points.size() > 1 && points_close(ring->points.front(), ring->points.back()))
    {
        ring->points.pop_back();
    }
    const rapidjson::Value* closed = member(value, "closed");
    const bool is_closed = closed == nullptr || !closed->IsBool() || closed->GetBool();
    if (is_closed && ring->segments.size() == ring->points.size() - 1)
    {
        ring->segments.push_back(Segment2{});
    }
    if (ring->points.size() < 3 || ring->segments.size() != ring->points.size())
    {
        *error =
            label + " closed contour must have at least three points and one segment per point";
        return false;
    }
    return true;
}

bool parse_ring(const rapidjson::Value& value, double default_scale, Ring* ring, std::string* error,
                const std::string& label)
{
    if (!value.IsObject())
    {
        *error = label + " must be an object";
        return false;
    }
    if (member(value, "points") != nullptr)
    {
        return parse_ring_from_points(value, default_scale, ring, error, label);
    }
    return parse_ring_from_geom_contour(value, default_scale, ring, error, label);
}

bool parse_region(const rapidjson::Value& value, double default_scale, Region* region,
                  std::string* error, const std::string& label)
{
    if (!value.IsObject())
    {
        *error = label + " must be an object";
        return false;
    }
    const rapidjson::Value* outer = member(value, "outer");
    if (outer == nullptr)
    {
        *error = label + " requires outer contour";
        return false;
    }
    if (!parse_ring(*outer, default_scale, &region->outer, error, label + ".outer"))
    {
        return false;
    }

    const rapidjson::Value* holes = member(value, "holes");
    if (holes != nullptr && !holes->IsNull())
    {
        if (!holes->IsArray())
        {
            *error = label + ".holes must be an array";
            return false;
        }
        for (rapidjson::SizeType i = 0; i < holes->Size(); ++i)
        {
            Ring hole;
            if (!parse_ring((*holes)[i], default_scale, &hole, error,
                            label + ".holes[" + std::to_string(i) + "]"))
            {
                return false;
            }
            region->holes.push_back(hole);
        }
    }
    return true;
}

bool parse_regions_array(const rapidjson::Value& object, const char* field_name,
                         double default_scale, std::vector<Region>* regions, std::string* error,
                         const std::string& label)
{
    const rapidjson::Value* value = member(object, field_name);
    if (value == nullptr || value->IsNull())
    {
        return true;
    }
    if (!value->IsArray())
    {
        *error = label + "." + field_name + " must be an array";
        return false;
    }
    for (rapidjson::SizeType i = 0; i < value->Size(); ++i)
    {
        Region region;
        if (!parse_region((*value)[i], default_scale, &region, error,
                          label + "." + field_name + "[" + std::to_string(i) + "]"))
        {
            return false;
        }
        regions->push_back(region);
    }
    return true;
}

bool parse_request(const char* request_json, Request* request, std::string* error)
{
    if (request_json == nullptr || request_json[0] == '\0')
    {
        *error = "planar STEP request JSON is empty";
        return false;
    }
    rapidjson::Document document;
    document.Parse(request_json);
    if (document.HasParseError() || !document.IsObject())
    {
        *error = "invalid planar STEP request JSON";
        if (document.HasParseError())
        {
            *error += ": ";
            *error += rapidjson::GetParseError_En(document.GetParseError());
        }
        return false;
    }

    request->name =
        string_member(document, "name", string_member(document, "product_name", "planar_step"));
    request->unit_scale_to_mm = unit_scale_to_mm(string_member(document, "units", "mm"));

    const rapidjson::Value* bodies = member(document, "bodies");
    if (bodies == nullptr || !bodies->IsArray() || bodies->Empty())
    {
        *error = "planar STEP request requires non-empty bodies array";
        return false;
    }
    for (rapidjson::SizeType i = 0; i < bodies->Size(); ++i)
    {
        const rapidjson::Value& raw_body = (*bodies)[i];
        if (!raw_body.IsObject())
        {
            *error = "bodies[" + std::to_string(i) + "] must be an object";
            return false;
        }
        BodySpec body;
        body.id = string_member(raw_body, "id", "body_" + std::to_string(i + 1));
        body.name = string_member(raw_body, "name", body.id);
        body.fuse_regions = bool_member(raw_body, {"fuse_regions", "fuseRegions", "fuse"}, false);
        if (!parse_suffixed_length_member(raw_body, "thickness", request->unit_scale_to_mm, 0.035,
                                          &body.thickness_mm, error))
        {
            return false;
        }
        if (!parse_suffixed_length_member(raw_body, "z", request->unit_scale_to_mm, 0.0, &body.z_mm,
                                          error))
        {
            return false;
        }
        if (body.thickness_mm <= 0.0)
        {
            *error = "bodies[" + std::to_string(i) + "].thickness must be positive";
            return false;
        }
        const rapidjson::Value* color = member(raw_body, "color");
        if (color != nullptr && !parse_color(*color, &body.color, error))
        {
            return false;
        }
        if (!parse_regions_array(raw_body, "regions", request->unit_scale_to_mm, &body.regions,
                                 error, "bodies[" + std::to_string(i) + "]"))
        {
            return false;
        }
        if (!parse_regions_array(raw_body, "cutouts", request->unit_scale_to_mm, &body.cutouts,
                                 error, "bodies[" + std::to_string(i) + "]"))
        {
            return false;
        }
        if (body.regions.empty())
        {
            *error = "bodies[" + std::to_string(i) + "] requires at least one region";
            return false;
        }
        request->bodies.push_back(std::move(body));
    }
    return true;
}

double normalized_ccw_delta(double start, double end)
{
    double delta = std::fmod(end - start, kTwoPi);
    if (delta < 0.0)
    {
        delta += kTwoPi;
    }
    return delta;
}

bool infer_arc_center(const Point2& start, const Point2& end, double radius, bool ccw,
                      Point2* center)
{
    const double dx = end.x - start.x;
    const double dy = end.y - start.y;
    const double chord = std::hypot(dx, dy);
    if (chord <= kEpsilon || radius <= 0.0 || chord > 2.0 * radius + kEpsilon)
    {
        return false;
    }
    const Point2 mid{(start.x + end.x) * 0.5, (start.y + end.y) * 0.5};
    const double half_chord = chord * 0.5;
    const double height = std::sqrt(std::max(0.0, radius * radius - half_chord * half_chord));
    const double nx = -dy / chord;
    const double ny = dx / chord;
    Point2 candidates[2] = {{mid.x + nx * height, mid.y + ny * height},
                            {mid.x - nx * height, mid.y - ny * height}};
    double best_score = std::numeric_limits<double>::infinity();
    Point2 best = candidates[0];
    for (const Point2& candidate : candidates)
    {
        const double start_angle = std::atan2(start.y - candidate.y, start.x - candidate.x);
        const double end_angle = std::atan2(end.y - candidate.y, end.x - candidate.x);
        const double sweep = ccw ? normalized_ccw_delta(start_angle, end_angle)
                                 : normalized_ccw_delta(end_angle, start_angle);
        const double score = sweep <= kPi + 1e-6 ? sweep : sweep + kTwoPi;
        if (score < best_score)
        {
            best_score = score;
            best = candidate;
        }
    }
    *center = best;
    return true;
}

bool arc_midpoint(const Point2& start, const Point2& end, const Segment2& segment, Point2* midpoint,
                  std::string* error)
{
    Point2 center = segment.center;
    if (!segment.has_center)
    {
        if (!segment.has_radius ||
            !infer_arc_center(start, end, segment.radius, segment.ccw, &center))
        {
            *error = "arc segment requires a valid center or radius";
            return false;
        }
    }
    const double radius = std::hypot(start.x - center.x, start.y - center.y);
    if (radius <= kEpsilon)
    {
        *error = "arc segment radius is zero";
        return false;
    }
    const double start_angle = std::atan2(start.y - center.y, start.x - center.x);
    const double end_angle = std::atan2(end.y - center.y, end.x - center.x);
    const double sweep = segment.ccw ? normalized_ccw_delta(start_angle, end_angle)
                                     : -normalized_ccw_delta(end_angle, start_angle);
    if (std::abs(sweep) <= kEpsilon || std::abs(sweep) >= kTwoPi - 1e-6)
    {
        *error = "arc segment cannot be zero-length or a full circle";
        return false;
    }
    const double mid_angle = start_angle + sweep * 0.5;
    midpoint->x = center.x + radius * std::cos(mid_angle);
    midpoint->y = center.y + radius * std::sin(mid_angle);
    return true;
}

void append_area_point(std::vector<Point2>* points, const Point2& point)
{
    if (points->empty() || !points_close(points->back(), point))
    {
        points->push_back(point);
    }
}

double ring_signed_area(const Ring& ring)
{
    std::vector<Point2> samples;
    for (std::size_t i = 0; i < ring.points.size(); ++i)
    {
        const Point2& start = ring.points[i];
        const Point2& end = ring.points[(i + 1) % ring.points.size()];
        const Segment2& segment = ring.segments[i];
        append_area_point(&samples, start);
        if (segment.kind == "arc")
        {
            Point2 center = segment.center;
            if (!segment.has_center)
            {
                if (!infer_arc_center(start, end, segment.radius, segment.ccw, &center))
                {
                    append_area_point(&samples, end);
                    continue;
                }
            }
            const double start_angle = std::atan2(start.y - center.y, start.x - center.x);
            const double end_angle = std::atan2(end.y - center.y, end.x - center.x);
            const double sweep = segment.ccw ? normalized_ccw_delta(start_angle, end_angle)
                                             : -normalized_ccw_delta(end_angle, start_angle);
            const int steps =
                std::max(2, static_cast<int>(std::ceil(std::abs(sweep) / (kPi / 16.0))));
            const double radius = std::hypot(start.x - center.x, start.y - center.y);
            for (int step = 1; step < steps; ++step)
            {
                const double angle =
                    start_angle + sweep * static_cast<double>(step) / static_cast<double>(steps);
                append_area_point(&samples, Point2{center.x + radius * std::cos(angle),
                                                   center.y + radius * std::sin(angle)});
            }
        }
        append_area_point(&samples, end);
    }
    if (samples.size() < 3)
    {
        return 0.0;
    }
    double twice_area = 0.0;
    for (std::size_t i = 0; i < samples.size(); ++i)
    {
        const Point2& a = samples[i];
        const Point2& b = samples[(i + 1) % samples.size()];
        twice_area += a.x * b.y - b.x * a.y;
    }
    return 0.5 * twice_area;
}

void append_solve_point(PlanarSolveRing* ring, const Point2& point)
{
    if (ring->empty() || !points_close(Point2{ring->back().x, ring->back().y}, point))
    {
        ring->push_back({point.x, point.y});
    }
}

double solve_ring_signed_area(const PlanarSolveRing& ring)
{
    if (ring.size() < 3)
    {
        return 0.0;
    }
    double twice_area = 0.0;
    for (std::size_t i = 0; i < ring.size(); ++i)
    {
        const PlanarSolvePoint& a = ring[i];
        const PlanarSolvePoint& b = ring[(i + 1) % ring.size()];
        twice_area += a.x * b.y - b.x * a.y;
    }
    return 0.5 * twice_area;
}

void orient_solve_ring(PlanarSolveRing* ring, bool want_positive)
{
    if (ring == nullptr || ring->size() < 3)
    {
        return;
    }
    const bool positive = solve_ring_signed_area(*ring) > 0.0;
    if (positive != want_positive)
    {
        std::reverse(ring->begin(), ring->end());
    }
}

bool sample_ring_for_solve(const Ring& ring, PlanarSolveRing* path, std::string* error)
{
    path->clear();
    for (std::size_t i = 0; i < ring.points.size(); ++i)
    {
        const Point2& start = ring.points[i];
        const Point2& end = ring.points[(i + 1) % ring.points.size()];
        const Segment2& segment = ring.segments[i];
        append_solve_point(path, start);
        if (segment.kind != "arc")
        {
            continue;
        }

        Point2 center = segment.center;
        if (!segment.has_center)
        {
            if (!segment.has_radius ||
                !infer_arc_center(start, end, segment.radius, segment.ccw, &center))
            {
                *error = "arc segment requires a valid center or radius for fusion";
                return false;
            }
        }
        const double radius = std::hypot(start.x - center.x, start.y - center.y);
        if (radius <= kEpsilon)
        {
            *error = "arc segment radius is zero";
            return false;
        }
        const double start_angle = std::atan2(start.y - center.y, start.x - center.x);
        const double end_angle = std::atan2(end.y - center.y, end.x - center.x);
        const double sweep = segment.ccw ? normalized_ccw_delta(start_angle, end_angle)
                                         : -normalized_ccw_delta(end_angle, start_angle);
        if (std::abs(sweep) <= kEpsilon || std::abs(sweep) >= kTwoPi - 1e-6)
        {
            *error = "arc segment cannot be zero-length or a full circle";
            return false;
        }
        const int steps = std::max(2, static_cast<int>(std::ceil(std::abs(sweep) / (kPi / 16.0))));
        for (int step = 1; step < steps; ++step)
        {
            const double angle =
                start_angle + sweep * static_cast<double>(step) / static_cast<double>(steps);
            append_solve_point(path, Point2{center.x + radius * std::cos(angle),
                                            center.y + radius * std::sin(angle)});
        }
    }
    if (path->size() > 1)
    {
        const Point2 first{path->front().x, path->front().y};
        const Point2 last{path->back().x, path->back().y};
        if (points_close(first, last))
        {
            path->pop_back();
        }
    }
    if (path->size() < 3)
    {
        *error = "fused planar ring must contain at least three points";
        return false;
    }
    return true;
}

Ring ring_from_solve_ring(const PlanarSolveRing& source)
{
    Ring ring;
    ring.points.reserve(source.size());
    for (const PlanarSolvePoint& point : source)
    {
        ring.points.push_back(Point2{point.x, point.y});
    }
    ring.segments.assign(ring.points.size(), Segment2{});
    return ring;
}

bool fuse_region_set(const std::vector<Region>& regions, std::vector<Region>* fused,
                     std::string* error)
{
    PlanarBatchSolveInput input;
    input.options.decimal_precision = 6;

    PlanarSolveJob job;
    for (const Region& region : regions)
    {
        PlanarSolveRing outer;
        if (!sample_ring_for_solve(region.outer, &outer, error))
        {
            return false;
        }
        orient_solve_ring(&outer, true);
        job.subject_rings.push_back(std::move(outer));

        for (const Ring& hole : region.holes)
        {
            PlanarSolveRing hole_ring;
            if (!sample_ring_for_solve(hole, &hole_ring, error))
            {
                return false;
            }
            orient_solve_ring(&hole_ring, false);
            job.subject_rings.push_back(std::move(hole_ring));
        }
    }

    input.jobs.push_back(std::move(job));
    PlanarBatchSolveResult result;
    Status status;
    const int code = solve_planar_batch(input, &result, &status);
    if (code != 0)
    {
        *error = "failed fusing planar regions: " + status.message;
        return false;
    }
    if (result.jobs.empty())
    {
        *error = "fusing planar regions returned no result";
        return false;
    }

    fused->clear();
    for (const PlanarSolveRegion& source_region : result.jobs[0].regions)
    {
        if (source_region.outline.size() < 3)
        {
            continue;
        }
        Region region;
        region.outer = ring_from_solve_ring(source_region.outline);
        for (const PlanarSolveRing& source_hole : source_region.holes)
        {
            if (source_hole.size() >= 3)
            {
                region.holes.push_back(ring_from_solve_ring(source_hole));
            }
        }
        fused->push_back(std::move(region));
    }
    if (fused->empty())
    {
        *error = "fusing planar regions produced no regions";
        return false;
    }
    return true;
}

bool build_wire(const Ring& ring, double z_mm, bool hole, TopoDS_Wire* wire, std::string* error)
{
    BRepBuilderAPI_MakeWire wire_maker;
    for (std::size_t i = 0; i < ring.points.size(); ++i)
    {
        const Point2& start = ring.points[i];
        const Point2& end = ring.points[(i + 1) % ring.points.size()];
        const Segment2& segment = ring.segments[i];
        TopoDS_Edge edge_shape;
        if (segment.kind == "arc")
        {
            Point2 mid;
            if (!arc_midpoint(start, end, segment, &mid, error))
            {
                return false;
            }
            GC_MakeArcOfCircle arc(gp_Pnt(start.x, start.y, z_mm), gp_Pnt(mid.x, mid.y, z_mm),
                                   gp_Pnt(end.x, end.y, z_mm));
            if (!arc.IsDone())
            {
                *error = "failed creating OCCT arc edge";
                return false;
            }
            BRepBuilderAPI_MakeEdge edge_maker(arc.Value());
            if (!edge_maker.IsDone())
            {
                *error = "failed creating OCCT arc edge";
                return false;
            }
            edge_shape = edge_maker.Edge();
        }
        else
        {
            BRepBuilderAPI_MakeEdge edge_maker(gp_Pnt(start.x, start.y, z_mm),
                                               gp_Pnt(end.x, end.y, z_mm));
            if (!edge_maker.IsDone())
            {
                *error = "failed creating OCCT line edge";
                return false;
            }
            edge_shape = edge_maker.Edge();
        }
        wire_maker.Add(edge_shape);
        if (!wire_maker.IsDone())
        {
            *error = "failed adding edge to OCCT wire";
            return false;
        }
    }
    *wire = wire_maker.Wire();
    const double area = ring_signed_area(ring);
    if ((!hole && area < 0.0) || (hole && area > 0.0))
    {
        wire->Reverse();
    }
    return true;
}

bool build_region_prism(const Region& region, double z_mm, double thickness_mm, TopoDS_Shape* shape,
                        std::string* error)
{
    TopoDS_Wire outer;
    if (!build_wire(region.outer, z_mm, false, &outer, error))
    {
        return false;
    }
    BRepBuilderAPI_MakeFace face_maker(outer);
    if (!face_maker.IsDone())
    {
        *error = "failed creating face from outer wire";
        return false;
    }
    for (const Ring& hole : region.holes)
    {
        TopoDS_Wire hole_wire;
        if (!build_wire(hole, z_mm, true, &hole_wire, error))
        {
            return false;
        }
        face_maker.Add(hole_wire);
        if (!face_maker.IsDone())
        {
            *error = "failed adding hole wire to face";
            return false;
        }
    }
    TopoDS_Face face = face_maker.Face();
    BRepPrimAPI_MakePrism prism_maker(face, gp_Vec(0.0, 0.0, thickness_mm));
    prism_maker.Build();
    if (!prism_maker.IsDone())
    {
        *error = "failed extruding planar face";
        return false;
    }
    *shape = prism_maker.Shape();
    return true;
}

bool build_cutouts_compound(const std::vector<Region>& cutouts, double z_mm, double thickness_mm,
                            TopoDS_Shape* cuts, std::string* error)
{
    TopoDS_Compound compound;
    BRep_Builder builder;
    builder.MakeCompound(compound);
    bool has_cut = false;
    for (const Region& cutout : cutouts)
    {
        TopoDS_Shape cut_shape;
        if (!build_region_prism(cutout, z_mm - 0.001, thickness_mm + 0.002, &cut_shape, error))
        {
            return false;
        }
        builder.Add(compound, cut_shape);
        has_cut = true;
    }
    if (!has_cut)
    {
        return false;
    }
    *cuts = compound;
    return true;
}

bool build_body_shape(const BodySpec& body, TopoDS_Shape* shape, std::string* error,
                      int* region_count)
{
    std::vector<Region> fused_regions;
    const std::vector<Region>* body_regions = &body.regions;
    if (body.fuse_regions)
    {
        if (!fuse_region_set(body.regions, &fused_regions, error))
        {
            return false;
        }
        body_regions = &fused_regions;
    }

    TopoDS_Shape cuts;
    const bool has_cuts =
        !body.cutouts.empty() &&
        build_cutouts_compound(body.cutouts, body.z_mm, body.thickness_mm, &cuts, error);
    if (!body.cutouts.empty() && !has_cuts)
    {
        return false;
    }

    TopoDS_Compound compound;
    BRep_Builder builder;
    builder.MakeCompound(compound);
    int added = 0;
    for (const Region& region : *body_regions)
    {
        TopoDS_Shape region_shape;
        if (!build_region_prism(region, body.z_mm, body.thickness_mm, &region_shape, error))
        {
            return false;
        }
        if (has_cuts)
        {
            BRepAlgoAPI_Cut cut(region_shape, cuts);
            cut.Build();
            if (cut.IsDone())
            {
                region_shape = cut.Shape();
            }
        }
        builder.Add(compound, region_shape);
        added += 1;
    }
    if (added == 0)
    {
        *error = "body contains no generated region solids";
        return false;
    }
    if (region_count != nullptr)
    {
        *region_count += added;
    }
    *shape = compound;
    return true;
}

void assign_shape_name(const TDF_Label& label, const std::string& name)
{
    TDataStd_Name::Set(label, TCollection_ExtendedString(name.c_str()));
}

bool write_step_file(const Request& request, const std::string& step_path, PlanarStepResult* result,
                     std::string* error)
{
    Handle(TDocStd_Document) doc =
        new TDocStd_Document(TCollection_ExtendedString("GeometerPlanarStep"));
    Handle(XCAFDoc_ShapeTool) shape_tool = XCAFDoc_DocumentTool::ShapeTool(doc->Main());
    Handle(XCAFDoc_ColorTool) color_tool = XCAFDoc_DocumentTool::ColorTool(doc->Main());

    PlanarStepResult local_result;
    for (const BodySpec& body : request.bodies)
    {
        TopoDS_Shape body_shape;
        if (!build_body_shape(body, &body_shape, error, &local_result.region_count))
        {
            return false;
        }
        TDF_Label label = shape_tool->AddShape(body_shape, Standard_False);
        assign_shape_name(label, body.name.empty() ? body.id : body.name);
        Quantity_Color color(body.color.r, body.color.g, body.color.b, Quantity_TOC_RGB);
        color_tool->SetColor(label, color, XCAFDoc_ColorSurf);
        color_tool->SetColor(label, color, XCAFDoc_ColorGen);
        local_result.body_count += 1;
        local_result.cutout_count += static_cast<int>(body.cutouts.size());
    }

    STEPCAFControl_Writer writer;
    writer.SetColorMode(Standard_True);
    writer.SetNameMode(Standard_True);
    if (!writer.Transfer(doc, STEPControl_AsIs, nullptr, Message_ProgressRange()))
    {
        *error = "failed transferring planar STEP document";
        return false;
    }
    const IFSelect_ReturnStatus status = writer.Write(step_path.c_str());
    if (status != IFSelect_RetDone)
    {
        *error = "failed writing planar STEP file";
        return false;
    }
    if (result != nullptr)
    {
        *result = local_result;
    }
    return true;
}

} // namespace

int planar_step_from_json(const char* request_json, const std::string& step_path,
                          PlanarStepResult* result, Status* status)
{
    Request request;
    std::string error;
    if (!parse_request(request_json, &request, &error))
    {
        set_status(status, 2, error);
        return 2;
    }
    if (step_path.empty())
    {
        set_status(status, 2, "STEP output path is empty.");
        return 2;
    }
    if (!write_step_file(request, step_path, result, &error))
    {
        set_status(status, 3, error);
        return 3;
    }
    set_status(status, 0, "");
    return 0;
}

int planar_step_from_json_bytes(const char* request_json, std::vector<unsigned char>* step_bytes,
                                PlanarStepResult* result, Status* status)
{
    if (step_bytes == nullptr)
    {
        set_status(status, 92, "STEP byte output pointer is null.");
        return 92;
    }
    step_bytes->clear();
    const std::string step_path = temp_planar_step_path(".step");
    const int code = planar_step_from_json(request_json, step_path, result, status);
    if (code != 0)
    {
        std::remove(step_path.c_str());
        return code;
    }
    if (!read_binary_file(step_path, step_bytes))
    {
        std::remove(step_path.c_str());
        set_status(status, 3, "Failed reading temporary planar STEP output.");
        return 3;
    }
    std::remove(step_path.c_str());
    set_status(status, 0, "");
    return 0;
}

} // namespace geometer

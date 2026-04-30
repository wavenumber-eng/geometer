#include "geometer/projection.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

namespace geometer
{
namespace
{

void set_status(Status* status, int code, const std::string& message)
{
    if (status == nullptr)
    {
        return;
    }
    status->code = code;
    status->message = message;
}

void append_json_string(std::ostringstream& out, const std::string& value)
{
    out << '"';
    for (char ch : value)
    {
        switch (ch)
        {
        case '\\':
            out << "\\\\";
            break;
        case '"':
            out << "\\\"";
            break;
        case '\n':
            out << "\\n";
            break;
        case '\r':
            out << "\\r";
            break;
        case '\t':
            out << "\\t";
            break;
        default:
            out << ch;
            break;
        }
    }
    out << '"';
}

void append_vec3(std::ostringstream& out, const std::array<double, 3>& value)
{
    out << '[' << value[0] << ',' << value[1] << ',' << value[2] << ']';
}

void append_vec2(std::ostringstream& out, const std::array<double, 2>& value)
{
    out << '[' << value[0] << ',' << value[1] << ']';
}

void append_segments(std::ostringstream& out, const std::vector<ProjectedSegment>& segments)
{
    out << '[';
    for (std::size_t i = 0; i < segments.size(); ++i)
    {
        const ProjectedSegment& segment = segments[i];
        if (i > 0)
        {
            out << ',';
        }
        out << '[' << segment.x1 << ',' << segment.y1 << ',' << segment.x2 << ',' << segment.y2
            << ']';
    }
    out << ']';
}

void append_arcs(std::ostringstream& out, const std::vector<ProjectedArc>& arcs)
{
    out << '[';
    for (std::size_t i = 0; i < arcs.size(); ++i)
    {
        const ProjectedArc& arc = arcs[i];
        if (i > 0)
        {
            out << ',';
        }
        out << "{\"start\":";
        append_vec2(out, arc.start);
        out << ",\"end\":";
        append_vec2(out, arc.end);
        out << ",\"center\":";
        append_vec2(out, arc.center);
        out << ",\"radius\":" << arc.radius;
        out << ",\"extent_rad\":" << arc.extent_rad;
        out << ",\"ccw\":" << (arc.ccw ? "true" : "false");
        out << ",\"full_circle\":" << (arc.full_circle ? "true" : "false") << '}';
    }
    out << ']';
}

void append_mode(std::ostringstream& out, const ProjectedModeGeometry& mode)
{
    out << "{\"segments\":";
    append_segments(out, mode.segments);
    out << ",\"arcs\":";
    append_arcs(out, mode.arcs);
    out << '}';
}

const ProjectedModeGeometry* select_mode(const ProjectedViewGeometry& view, const std::string& mode)
{
    return mode == "simple" ? &view.simple : &view.detail;
}

struct Bounds
{
    double min_x = std::numeric_limits<double>::infinity();
    double min_y = std::numeric_limits<double>::infinity();
    double max_x = -std::numeric_limits<double>::infinity();
    double max_y = -std::numeric_limits<double>::infinity();

    bool valid() const
    {
        return std::isfinite(min_x) && std::isfinite(min_y) && std::isfinite(max_x) &&
               std::isfinite(max_y);
    }
};

void include_point(Bounds& bounds, double x, double y)
{
    bounds.min_x = std::min(bounds.min_x, x);
    bounds.min_y = std::min(bounds.min_y, y);
    bounds.max_x = std::max(bounds.max_x, x);
    bounds.max_y = std::max(bounds.max_y, y);
}

Bounds geometry_bounds(const ProjectedModeGeometry& geometry)
{
    Bounds bounds;
    for (const ProjectedSegment& segment : geometry.segments)
    {
        include_point(bounds, segment.x1, segment.y1);
        include_point(bounds, segment.x2, segment.y2);
    }
    for (const ProjectedArc& arc : geometry.arcs)
    {
        if (arc.full_circle)
        {
            include_point(bounds, arc.center[0] - arc.radius, arc.center[1] - arc.radius);
            include_point(bounds, arc.center[0] + arc.radius, arc.center[1] + arc.radius);
        }
        else
        {
            include_point(bounds, arc.start[0], arc.start[1]);
            include_point(bounds, arc.end[0], arc.end[1]);
        }
    }
    return bounds;
}

std::string svg_number(double value)
{
    std::ostringstream out;
    out << std::setprecision(12) << value;
    return out.str();
}

} // namespace

int step_hlr_projection_from_bytes(const unsigned char* step_data, std::size_t step_size,
                                   const HlrProjectionOptions& options, HlrProjectionResult* result,
                                   Status* status)
{
    (void)step_data;
    (void)step_size;
    (void)options;
    (void)result;
    set_status(status, 90, "STEP HLR projection is not implemented yet.");
    return 90;
}

int write_hlr_projection_json(const HlrProjectionResult& result, std::string* json, Status* status)
{
    if (json == nullptr)
    {
        set_status(status, 2, "Output JSON pointer is null.");
        return 2;
    }

    std::ostringstream out;
    out << std::setprecision(12);
    out << "{\"schema\":";
    append_json_string(out, result.schema);
    out << ",\"units\":";
    append_json_string(out, result.units);
    out << ",\"source\":{\"kind\":\"step\",\"hash\":";
    append_json_string(out, result.source_hash);
    out << "},\"views\":[";
    for (std::size_t i = 0; i < result.views.size(); ++i)
    {
        const ProjectedViewGeometry& view = result.views[i];
        if (i > 0)
        {
            out << ',';
        }
        out << "{\"id\":";
        append_json_string(out, view.view.id);
        out << ",\"direction\":";
        append_vec3(out, view.view.direction);
        out << ",\"up\":";
        append_vec3(out, view.view.up);
        out << ",\"modes\":{\"simple\":";
        append_mode(out, view.simple);
        out << ",\"detail\":";
        append_mode(out, view.detail);
        out << "}}";
    }
    out << "]}";

    *json = out.str();
    set_status(status, 0, "");
    return 0;
}

int write_hlr_projection_svg(const HlrProjectionResult& result, const std::string& view_id,
                             const std::string& mode, std::string* svg, Status* status)
{
    if (svg == nullptr)
    {
        set_status(status, 2, "Output SVG pointer is null.");
        return 2;
    }

    const ProjectedViewGeometry* selected_view = nullptr;
    for (const ProjectedViewGeometry& view : result.views)
    {
        if (view.view.id == view_id)
        {
            selected_view = &view;
            break;
        }
    }
    if (selected_view == nullptr)
    {
        set_status(status, 3, "Requested projection view was not found.");
        return 3;
    }

    const std::string normalized_mode = mode == "simple" ? "simple" : "detail";
    const ProjectedModeGeometry* geometry = select_mode(*selected_view, normalized_mode);
    Bounds bounds = geometry_bounds(*geometry);
    if (!bounds.valid())
    {
        bounds.min_x = 0.0;
        bounds.min_y = 0.0;
        bounds.max_x = 1.0;
        bounds.max_y = 1.0;
    }

    const double pad = std::max((bounds.max_x - bounds.min_x) * 0.05, 1.0);
    const double view_min_x = bounds.min_x - pad;
    const double view_min_y = -bounds.max_y - pad;
    const double view_width = std::max(bounds.max_x - bounds.min_x + (2.0 * pad), 1.0);
    const double view_height = std::max(bounds.max_y - bounds.min_y + (2.0 * pad), 1.0);

    std::ostringstream out;
    out << std::setprecision(12);
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"" << svg_number(view_min_x) << " "
        << svg_number(view_min_y) << " " << svg_number(view_width) << " " << svg_number(view_height)
        << "\">\n";
    out << "  <g fill=\"none\" stroke=\"#111827\" stroke-width=\"0.08\" "
           "stroke-linecap=\"round\" stroke-linejoin=\"round\">\n";

    for (const ProjectedSegment& segment : geometry->segments)
    {
        out << "    <line x1=\"" << svg_number(segment.x1) << "\" y1=\"" << svg_number(-segment.y1)
            << "\" x2=\"" << svg_number(segment.x2) << "\" y2=\"" << svg_number(-segment.y2)
            << "\"/>\n";
    }

    for (const ProjectedArc& arc : geometry->arcs)
    {
        if (arc.full_circle)
        {
            out << "    <circle cx=\"" << svg_number(arc.center[0]) << "\" cy=\""
                << svg_number(-arc.center[1]) << "\" r=\"" << svg_number(arc.radius) << "\"/>\n";
            continue;
        }
        const int large_arc = arc.extent_rad > 3.14159265358979323846 ? 1 : 0;
        const int sweep = arc.ccw ? 0 : 1;
        out << "    <path d=\"M " << svg_number(arc.start[0]) << " " << svg_number(-arc.start[1])
            << " A " << svg_number(arc.radius) << " " << svg_number(arc.radius) << " 0 "
            << large_arc << " " << sweep << " " << svg_number(arc.end[0]) << " "
            << svg_number(-arc.end[1]) << "\"/>\n";
    }

    out << "  </g>\n";
    out << "</svg>\n";

    *svg = out.str();
    set_status(status, 0, "");
    return 0;
}

} // namespace geometer

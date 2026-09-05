#include "mesh_illustration_svg.h"

#include <algorithm>
#include <regex>
#include <unordered_map>

namespace geometer::illustration_detail
{
void append_bounded(std::string& target, const std::string& text, std::size_t maximum)
{
    if (target.size() > maximum || text.size() > maximum - target.size())
        throw ResourceLimit("Mesh illustration SVG exceeds the output byte limit.");
    target += text;
}

namespace
{
constexpr std::size_t kMaxSvgBytes = 256u * 1024u * 1024u;

void validate_xml_text(const std::string& text)
{
    for (std::size_t i = 0; i < text.size();)
    {
        const auto first = static_cast<unsigned char>(text[i++]);
        unsigned code = first, following = 0, minimum = 0;
        if (first >= 0x80)
        {
            if (first >= 0xc2 && first <= 0xdf)
            {
                following = 1;
                code &= 0x1f;
                minimum = 0x80;
            }
            else if (first >= 0xe0 && first <= 0xef)
            {
                following = 2;
                code &= 0xf;
                minimum = 0x800;
            }
            else if (first >= 0xf0 && first <= 0xf4)
            {
                following = 3;
                code &= 7;
                minimum = 0x10000;
            }
            else
                throw std::runtime_error("Illustration SVG text must be valid UTF-8.");
            for (unsigned j = 0; j < following; ++j)
            {
                if (i == text.size() || (static_cast<unsigned char>(text[i]) & 0xc0) != 0x80)
                    throw std::runtime_error("Illustration SVG text must be valid UTF-8.");
                code = (code << 6) | (static_cast<unsigned char>(text[i++]) & 0x3f);
            }
        }
        if (code < minimum || (code < 0x20 && code != 9 && code != 10 && code != 13) ||
            (code >= 0xd800 && code <= 0xdfff) || code == 0xfffe || code == 0xffff ||
            code > 0x10ffff)
            throw std::runtime_error("Illustration SVG text contains an invalid XML character.");
    }
}

std::string escape_xml(const std::string& text)
{
    validate_xml_text(text);
    std::string result;
    for (char c : text)
        switch (c)
        {
        case '&':
            result += "&amp;";
            break;
        case '<':
            result += "&lt;";
            break;
        case '>':
            result += "&gt;";
            break;
        case '"':
            result += "&quot;";
            break;
        default:
            result += c;
            break;
        }
    return result;
}

std::string safe_color(const std::string& text)
{
    // Match ECMAScript trim and /iu regular-expression semantics independently
    // of the process locale, while retaining the original accepted CSS spelling.
    struct Character
    {
        std::size_t begin, end;
        unsigned code;
    };
    std::vector<Character> characters;
    for (std::size_t i = 0; i < text.size();)
    {
        const auto begin = i;
        const auto first = static_cast<unsigned char>(text[i++]);
        unsigned code = first, remaining = 0;
        if (first >= 0xc2 && first <= 0xdf)
        {
            code &= 0x1f;
            remaining = 1;
        }
        else if (first >= 0xe0 && first <= 0xef)
        {
            code &= 0xf;
            remaining = 2;
        }
        else if (first >= 0xf0 && first <= 0xf4)
        {
            code &= 7;
            remaining = 3;
        }
        else if (first >= 0x80)
            return "#000000";
        for (unsigned j = 0; j < remaining; ++j)
        {
            if (i == text.size() || (static_cast<unsigned char>(text[i]) & 0xc0) != 0x80)
                return "#000000";
            code = (code << 6) | (static_cast<unsigned char>(text[i++]) & 0x3f);
        }
        characters.push_back({begin, i, code});
    }
    const auto whitespace = [](unsigned code)
    {
        return (code >= 9 && code <= 13) || code == 0x20 || code == 0xa0 || code == 0x1680 ||
               (code >= 0x2000 && code <= 0x200a) || code == 0x2028 || code == 0x2029 ||
               code == 0x202f || code == 0x205f || code == 0x3000 || code == 0xfeff;
    };
    std::size_t first = 0, last = characters.size();
    while (first < last && whitespace(characters[first].code))
        ++first;
    while (last > first && whitespace(characters[last - 1].code))
        --last;
    if (first == last)
        return "#000000";
    const auto color =
        text.substr(characters[first].begin, characters[last - 1].end - characters[first].begin);
    std::string normalized;
    for (auto i = first; i < last; ++i)
    {
        unsigned code = characters[i].code;
        if (whitespace(code))
            code = ' ';
        // These are the only non-ASCII simple folds into ECMAScript [a-z].
        if (code == 0x212a)
            code = 'k';
        if (code == 0x17f)
            code = 's';
        if (code >= 'A' && code <= 'Z')
            code += 'a' - 'A';
        if (code >= 0x80)
            return "#000000";
        normalized += static_cast<char>(code);
    }
    static const std::regex allowed(
        "^(?:#[0-9a-f]{3,8}|[a-z]+|(?:rgb|rgba|hsl|hsla)\\([0-9.,%+\\- ]+\\))$",
        std::regex::ECMAScript);
    return std::regex_match(normalized, allowed) ? color : "#000000";
}

struct Svg
{
    double scale, source_x, source_y, seam;
    std::string body, rules;
    std::unordered_map<std::string, std::string> surface_classes, line_classes;

    void append(const std::string& text)
    {
        append_bounded(body, text, kMaxSvgBytes);
        append_bounded(body, "\n", kMaxSvgBytes);
    }

    Vec2 map(Vec2 point) const
    {
        return {js_round((point[0] - source_x) * scale), js_round((-point[1] - source_y) * scale)};
    }

    std::string surface_class(const Layer& layer)
    {
        const auto color = safe_color(layer.fill), opacity = number_text(layer.opacity);
        const auto key = color + "|" + opacity;
        const auto found = surface_classes.find(key);
        if (found != surface_classes.end())
            return found->second;
        const auto name = "gms" + std::to_string(surface_classes.size());
        surface_classes.emplace(key, name);
        append_bounded(rules,
                       "." + name + "{fill:" + color + ";fill-rule:evenodd;stroke:" + color +
                           ";stroke-width:" + number_text(seam) + ";stroke-linejoin:round" +
                           (layer.opacity < .999 ? ";opacity:" + opacity : "") + "}",
                       kMaxSvgBytes);
        return name;
    }

    std::string line_class(const Line& line)
    {
        const auto color = safe_color(line.color);
        const auto width = integer_text(std::max(1.0, js_round(line.width * scale)));
        const auto key = color + "|" + width;
        const auto found = line_classes.find(key);
        if (found != line_classes.end())
            return found->second;
        const auto name = "gml" + std::to_string(line_classes.size());
        line_classes.emplace(key, name);
        append_bounded(rules,
                       "." + name + "{fill:none;stroke:" + color + ";stroke-width:" + width +
                           ";stroke-linecap:round;stroke-linejoin:round}",
                       kMaxSvgBytes);
        return name;
    }

    void surface(const Surface& surface)
    {
        for (const auto& layer : surface.layers)
        {
            std::string geometry;
            for (const auto& ring : layer.rings)
            {
                for (std::size_t i = 0; i < ring.size(); ++i)
                {
                    const auto point = map(ring[i]);
                    if (surface.polygon)
                        geometry +=
                            (i ? " " : "") + number_text(point[0]) + "," + number_text(point[1]);
                    else
                        geometry +=
                            (i ? "L" : "M") + number_text(point[0]) + " " + number_text(point[1]);
                }
                if (!surface.polygon)
                    geometry += "Z";
            }
            const auto name = surface_class(layer);
            append(surface.polygon ? "<polygon class=\"" + name + "\" points=\"" + geometry + "\"/>"
                                   : "<path class=\"" + name + "\" d=\"" + geometry + "\"/>");
        }
    }
};
} // namespace

std::string render_svg(const Scene& scene, const Style& style, const Commands& commands,
                       const contracts::MeshIllustrationSvgOptions& options)
{
    const double width = std::max(scene.bounds.max_x - scene.bounds.min_x, 1e-9);
    const double height = std::max(scene.bounds.max_y - scene.bounds.min_y, 1e-9);
    const double pad = std::max(width, height) * .06;
    Svg svg;
    svg.source_x = scene.bounds.min_x - pad;
    svg.source_y = -scene.bounds.max_y - pad;
    svg.scale = options.coordinate_span.value_or(1000000) / std::max(width, height);
    svg.seam = std::max(1.0, js_round(std::max(width, height) * .003 * svg.scale));
    const auto svg_width = number_text(std::max(1.0, js_round((width + pad * 2) * svg.scale)));
    const auto svg_height = number_text(std::max(1.0, js_round((height + pad * 2) * svg.scale)));
    if (!style.transparent_background)
        svg.append("<rect width=\"" + svg_width + "\" height=\"" + svg_height + "\" fill=\"" +
                   escape_xml(safe_color(style.background)) + "\"/>");
    for (const auto& surface : commands.surfaces)
        svg.surface(surface);
    for (std::size_t index = 0; index < commands.lines.size();)
    {
        const auto& line = commands.lines[index];
        auto end = index + 1;
        while (end < commands.lines.size() && commands.lines[end].color == line.color &&
               std::abs(commands.lines[end].width - line.width) <= 1e-15)
            ++end;
        const auto path = chained_line_path(commands.lines, index, end,
                                            [&](Vec2 point) { return svg.map(point); });
        if (!path.empty())
            svg.append("<path class=\"" + svg.line_class(line) + "\" d=\"" + path + "\"/>");
        index = end;
    }
    std::string result =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 " +
        svg_width + " " + svg_height + "\" role=\"img\">\n<title>" +
        escape_xml(options.title.value_or("Geometer mesh illustration")) +
        "</title>\n<metadata>geometry.mesh_illustration.result.a0</metadata>\n<style>" +
        escape_xml(svg.rules) + "</style>\n";
    append_bounded(result, svg.body, kMaxSvgBytes);
    append_bounded(result, "</svg>\n", kMaxSvgBytes);
    return result;
}
} // namespace geometer::illustration_detail

#include "mesh_illustration_internal.h"

#include <algorithm>
#include <iomanip>
#include <locale>
#include <rapidjson/document.h>
#include <sstream>

namespace geometer::illustration_detail
{
double clamp(double value, double minimum, double maximum)
{
    return std::min(maximum, std::max(minimum, std::isfinite(value) ? value : 0));
}

double js_round(double value)
{
    // Unlike std::round, ECMAScript rounds negative halfway values toward +infinity.
    const double integral = std::floor(value);
    return value - integral < .5 ? integral : integral + 1;
}

namespace
{
bool parse_decimal(const std::string& text, double& value)
{
    rapidjson::Document parsed;
    parsed.Parse<rapidjson::kParseFullPrecisionFlag>(text.data(), text.size());
    if (parsed.HasParseError() || !parsed.IsNumber())
        return false;
    value = parsed.GetDouble();
    return std::isfinite(value);
}

std::string adjacent_decimal(const std::string& text, int delta)
{
    const auto exponent_at = text.find('e');
    const bool negative = text.front() == '-';
    auto digits = text.substr(negative ? 1 : 0, exponent_at - (negative ? 1 : 0));
    const auto point = digits.find('.');
    if (point != std::string::npos)
        digits.erase(point, 1);
    const auto significand = std::stoull(digits);
    auto adjacent = std::to_string(delta < 0 ? significand - 1 : significand + 1);
    const int exponent = std::stoi(text.substr(exponent_at + 1)) +
                         static_cast<int>(adjacent.size()) - static_cast<int>(digits.size());
    if (adjacent.size() > 1)
        adjacent.insert(1, ".");
    return (negative ? "-" : "") + adjacent + "e" + std::to_string(exponent);
}

std::string shortest_decimal(double value)
{
    // Select the nearest decimal with the fewest significant digits that
    // round-trips to this binary64. Grisu2 can choose a longer decimal (1e23).
    for (int digits = 1; digits <= 17; ++digits)
    {
        std::ostringstream stream;
        stream.imbue(std::locale::classic());
        stream << std::scientific << std::setprecision(digits - 1) << value;
        if (!stream)
            throw std::runtime_error("Illustration decimal formatting failed.");
        const auto text = stream.str();
        double decoded = 0;
        if (parse_decimal(text, decoded) && decoded == value)
            return text;
        // Powers of two have asymmetric binary rounding intervals. A decimal
        // neighbor can round-trip even when the nearest decimal does not.
        for (const int delta : {-1, 1})
        {
            const auto neighbor = adjacent_decimal(text, delta);
            if (parse_decimal(neighbor, decoded) && decoded == value)
                return neighbor;
        }
    }
    throw std::runtime_error("Illustration shortest decimal formatting failed.");
}

double finite_result(double value)
{
    if (!std::isfinite(value))
        throw std::runtime_error("Mesh illustration numeric overflow.");
    return value;
}

// Produce the exact decimal expansion of the binary64 value, then round away
// from zero at a decimal halfway boundary, as ECMAScript toPrecision/toFixed do.
// Standard C++ formatting uses ties-to-even instead. 1074 fractional digits
// suffice for every binary64, including the smallest subnormal.
std::string rounded_decimal(double value, unsigned digits, bool scientific)
{
    // Floating charconv is unavailable in the pinned Emscripten library and
    // requires macOS 13.3. Classic-locale streams preserve the macOS 11 floor.
    std::ostringstream expansion;
    expansion.imbue(std::locale::classic());
    expansion << (scientific ? std::scientific : std::fixed) << std::setprecision(1074)
              << std::abs(value);
    if (!expansion)
        throw std::runtime_error("Illustration decimal formatting failed.");
    std::string text = expansion.str();
    const auto exponent_at = text.find('e');
    int exponent = scientific ? std::stoi(text.substr(exponent_at + 1)) : 0;
    if (scientific)
        text.erase(exponent_at);
    const auto point = text.find('.');
    text.erase(point, 1);
    const std::size_t keep = scientific ? digits : point + digits;
    const bool up = text[keep] >= '5';
    text.resize(keep);
    if (up)
    {
        auto i = text.size();
        while (i > 0 && text[i - 1] == '9')
            text[--i] = '0';
        if (i > 0)
            ++text[i - 1];
        else
        {
            text.insert(0, "1");
            if (scientific)
            {
                text.pop_back();
                ++exponent;
            }
        }
    }
    if (scientific)
    {
        text.insert(1, ".");
        text += "e" + std::to_string(exponent);
    }
    else
        text.insert(text.size() - digits, ".");
    return (value < 0 ? "-" : "") + text;
}

std::string js_number(double value, bool precision12)
{
    if (!std::isfinite(value))
        throw std::runtime_error("Mesh illustration numeric overflow.");
    if (value == 0)
        return "0";
    if (precision12 && !(std::trunc(value) == value && std::abs(value) < 1e12))
    {
        const auto rounded = rounded_decimal(value, 12, true);
        if (!parse_decimal(rounded, value))
            throw std::runtime_error("Illustration decimal rounding overflow.");
    }
    std::string text = shortest_decimal(value);
    const auto exponent_at = text.find('e');
    if (exponent_at == std::string::npos)
        return text;
    const int exponent = std::stoi(text.substr(exponent_at + 1));
    if (exponent < -6 || exponent >= 21)
        return text.substr(0, exponent_at) + "e" + (exponent >= 0 ? "+" : "") +
               std::to_string(exponent);
    std::string digits = text.substr(0, exponent_at);
    const bool negative = digits.front() == '-';
    if (negative)
        digits.erase(0, 1);
    const auto point = digits.find('.');
    if (point != std::string::npos)
        digits.erase(point, 1);
    const int decimal = exponent + 1;
    if (decimal <= 0)
        digits = "0." + std::string(-decimal, '0') + digits;
    else if (decimal >= static_cast<int>(digits.size()))
        digits.append(decimal - digits.size(), '0');
    else
        digits.insert(decimal, ".");
    return (negative ? "-" : "") + digits;
}
} // namespace

std::string number_text(double value)
{
    return js_number(value, true);
}

std::string integer_text(double value)
{
    return js_number(js_round(value), false);
}

std::string fixed_text(double value)
{
    finite_result(value);
    return rounded_decimal(value, 12, false);
}

Vec3 add(Vec3 a, Vec3 b)
{
    return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}
Vec3 subtract(Vec3 a, Vec3 b)
{
    return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}
double dot(Vec3 a, Vec3 b)
{
    return finite_result(a[0] * b[0] + a[1] * b[1] + a[2] * b[2]);
}
Vec3 cross(Vec3 a, Vec3 b)
{
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}
double length(Vec3 a)
{
    return std::hypot(a[0], a[1], a[2]);
}
Vec3 normalize(Vec3 a, const char* label)
{
    const auto magnitude = length(a);
    if (!std::isfinite(magnitude) || magnitude < 1e-12)
        throw std::runtime_error(std::string(label) + " must be a finite non-zero vector.");
    return {a[0] / magnitude, a[1] / magnitude, a[2] / magnitude};
}
Vec3 vector3(const std::vector<double>& values)
{
    if (values.size() != 3)
        throw std::runtime_error("Illustration vector must contain three values.");
    return {values[0], values[1], values[2]};
}
Vec3 position(const std::vector<double>& values, std::size_t index)
{
    Vec3 result{};
    for (std::size_t axis = 0; axis < 3; ++axis)
        if (index * 3 + axis < values.size())
            result[axis] = values[index * 3 + axis];
    return result;
}
Vec3 transform_point(const Matrix& m, Vec3 p)
{
    double w = m[3] * p[0] + m[7] * p[1] + m[11] * p[2] + m[15];
    finite_result(w);
    if (w == 0)
        w = 1;
    Vec3 result{};
    for (std::size_t axis = 0; axis < 3; ++axis)
    {
        result[axis] =
            (m[axis] * p[0] + m[axis + 4] * p[1] + m[axis + 8] * p[2] + m[axis + 12]) / w;
        if (!std::isfinite(result[axis]))
            throw std::runtime_error("Illustration transformed position is not finite.");
    }
    return result;
}
double determinant_sign(const Matrix& m)
{
    const double d = m[0] * (m[5] * m[10] - m[9] * m[6]) - m[4] * (m[1] * m[10] - m[9] * m[2]) +
                     m[8] * (m[1] * m[6] - m[5] * m[2]);
    return finite_result(d) < 0 ? -1 : 1;
}
Vec3 transform_normal(const Matrix& m, Vec3 n)
{
    const double a = m[0], b = m[4], c = m[8], d = m[1], e = m[5], f = m[9], g = m[2], h = m[6],
                 i = m[10];
    Vec3 result{(e * i - f * h) * n[0] + (f * g - d * i) * n[1] + (d * h - e * g) * n[2],
                (c * h - b * i) * n[0] + (a * i - c * g) * n[1] + (b * g - a * h) * n[2],
                (b * f - c * e) * n[0] + (c * d - a * f) * n[1] + (a * e - b * d) * n[2]};
    for (auto& value : result)
        value *= determinant_sign(m);
    return normalize(result, "Transformed normal");
}
void Bounds::include(Vec2 point)
{
    min_x = std::min(min_x, point[0]);
    min_y = std::min(min_y, point[1]);
    max_x = std::max(max_x, point[0]);
    max_y = std::max(max_y, point[1]);
}
double cross2(Vec2 a, Vec2 b, Vec2 p)
{
    return finite_result((b[0] - a[0]) * (p[1] - a[1]) - (b[1] - a[1]) * (p[0] - a[0]));
}
double signed_area(const Ring& points)
{
    double area = 0;
    for (std::size_t i = 0; i < points.size(); ++i)
    {
        const auto& a = points[i];
        const auto& b = points[(i + 1) % points.size()];
        area += a[0] * b[1] - b[0] * a[1];
    }
    return finite_result(area * .5);
}
double signed_area(const std::array<Vec2, 3>& points)
{
    return signed_area(Ring(points.begin(), points.end()));
}
Bounds projected_bounds(const Triangle& triangle)
{
    Bounds result;
    for (const auto point : triangle.points)
        result.include(point);
    return result;
}
bool bounds_overlap(const Bounds& a, const Bounds& b, double epsilon)
{
    return !(a.max_x <= b.min_x + epsilon || b.max_x <= a.min_x + epsilon ||
             a.max_y <= b.min_y + epsilon || b.max_y <= a.min_y + epsilon);
}
Ring clip_polygon(const Triangle& a, const Triangle& b, double epsilon)
{
    Ring output(a.points.begin(), a.points.end());
    const double orientation = signed_area(b.points) >= 0 ? 1 : -1;
    for (std::size_t edge = 0; edge < 3 && !output.empty(); ++edge)
    {
        Ring input = std::move(output);
        output.clear();
        const auto start = b.points[edge], end = b.points[(edge + 1) % 3];
        auto previous = input.back();
        double previous_distance = orientation * cross2(start, end, previous);
        for (const auto current : input)
        {
            const double distance = orientation * cross2(start, end, current);
            if ((previous_distance >= -epsilon) != (distance >= -epsilon))
            {
                const double denominator = previous_distance - distance;
                if (std::abs(denominator) > epsilon)
                {
                    const double ratio = previous_distance / denominator;
                    output.push_back({previous[0] + (current[0] - previous[0]) * ratio,
                                      previous[1] + (current[1] - previous[1]) * ratio});
                }
            }
            if (distance >= -epsilon)
                output.push_back(current);
            previous = current;
            previous_distance = distance;
        }
    }
    return output;
}
bool significant_overlap(const Ring& overlap, const Triangle& a, const Triangle& b, double epsilon)
{
    if (overlap.size() < 3)
        return false;
    const auto ba = projected_bounds(a), bb = projected_bounds(b);
    const double scale = std::max({ba.max_x - ba.min_x, ba.max_y - ba.min_y, bb.max_x - bb.min_x,
                                   bb.max_y - bb.min_y, epsilon});
    return std::abs(signed_area(overlap)) > epsilon * scale * 4;
}
double depth_at(const Triangle& t, Vec2 p)
{
    const auto a = t.points[0], b = t.points[1], c = t.points[2];
    const double denominator = (b[1] - c[1]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[1] - c[1]);
    finite_result(denominator);
    if (std::abs(denominator) < 1e-20)
        return t.depth;
    const double wa = ((b[1] - c[1]) * (p[0] - c[0]) + (c[0] - b[0]) * (p[1] - c[1])) / denominator;
    const double wb = ((c[1] - a[1]) * (p[0] - c[0]) + (a[0] - c[0]) * (p[1] - c[1])) / denominator;
    return finite_result(wa * t.depths[0] + wb * t.depths[1] + (1 - wa - wb) * t.depths[2]);
}
} // namespace geometer::illustration_detail

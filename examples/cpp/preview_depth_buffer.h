#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace geometer_preview
{
struct ScreenVertex
{
    double x, y, depth;
};

// Orthographic, opaque preview only. Larger camera-space depth is nearer.
// Per-pixel depth is essential: average-depth triangle sorting cannot resolve
// overlapping sloped faces, even when their 3D surfaces do not intersect.
class DepthBuffer
{
  public:
    DepthBuffer(int width, int height)
        : width_(width), height_(height), pixels_(static_cast<std::size_t>(width) * height, 0),
          depths_(pixels_.size(), -std::numeric_limits<double>::infinity())
    {
    }

    const std::vector<std::uint32_t>& pixels() const
    {
        return pixels_;
    }

    void triangle(const std::array<ScreenVertex, 3>& p, std::uint32_t color)
    {
        for (const auto& v : p)
            if (!std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.depth))
                return;
        const auto edge = [](const ScreenVertex& a, const ScreenVertex& b, double x, double y)
        { return (b.x - a.x) * (y - a.y) - (b.y - a.y) * (x - a.x); };
        const double area = edge(p[0], p[1], p[2].x, p[2].y);
        if (!std::isfinite(area) || std::abs(area) < 1.0e-12)
            return;
        const int x0 = static_cast<int>(std::clamp(std::floor(std::min({p[0].x, p[1].x, p[2].x})),
                                                   0.0, static_cast<double>(width_)));
        const int x1 = static_cast<int>(std::clamp(std::ceil(std::max({p[0].x, p[1].x, p[2].x})),
                                                   0.0, static_cast<double>(width_)));
        const int y0 = static_cast<int>(std::clamp(std::floor(std::min({p[0].y, p[1].y, p[2].y})),
                                                   0.0, static_cast<double>(height_)));
        const int y1 = static_cast<int>(std::clamp(std::ceil(std::max({p[0].y, p[1].y, p[2].y})),
                                                   0.0, static_cast<double>(height_)));
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
            {
                const double a = edge(p[1], p[2], x + 0.5, y + 0.5) / area;
                const double b = edge(p[2], p[0], x + 0.5, y + 0.5) / area;
                const double c = 1.0 - a - b;
                if (a < -1.0e-10 || b < -1.0e-10 || c < -1.0e-10)
                    continue;
                const double depth = a * p[0].depth + b * p[1].depth + c * p[2].depth;
                const auto index = static_cast<std::size_t>(y) * width_ + x;
                if (depth > depths_[index])
                {
                    depths_[index] = depth;
                    pixels_[index] = color;
                }
            }
    }

  private:
    int width_, height_;
    std::vector<std::uint32_t> pixels_;
    std::vector<double> depths_;
};
} // namespace geometer_preview

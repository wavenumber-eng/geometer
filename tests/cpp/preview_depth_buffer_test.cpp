#include "../../examples/cpp/preview_depth_buffer.h"
#include <iostream>

int main()
{
    using geometer_preview::DepthBuffer;
    using geometer_preview::ScreenVertex;
    // Avoid exact depth ties: coplanar pixels have no unique front-most face.
    const std::array<ScreenVertex, 3> sloped = {{{0, 0, 10}, {8, 0, -9}, {0, 8, -9}}};
    const std::array<ScreenVertex, 3> flat = {{{0, 0, 0}, {8, 0, 0}, {0, 8, 0}}};
    DepthBuffer forward(8, 8), reverse(8, 8), winding(8, 8);
    forward.triangle(sloped, 1);
    forward.triangle(flat, 2);
    reverse.triangle(flat, 2);
    reverse.triangle(sloped, 1);
    winding.triangle({sloped[2], sloped[1], sloped[0]}, 1);
    winding.triangle({flat[2], flat[1], flat[0]}, 2);
    if (forward.pixels() != reverse.pixels() || forward.pixels() != winding.pixels() ||
        forward.pixels()[0] != 1 || forward.pixels()[5] != 2 || forward.pixels()[63] != 0)
    {
        std::cerr << "Per-pixel depth, draw order, or winding regression\n";
        return 1;
    }
    DepthBuffer clipped(8, 8);
    clipped.triangle({ScreenVertex{-100, -100, 1}, {100, -100, 1}, {0, 100, 1}}, 3);
    clipped.triangle({flat[0], flat[0], flat[0]}, 4);
    clipped.triangle({ScreenVertex{100, 100, 2}, {108, 100, 2}, {100, 108, 2}}, 4);
    for (const auto pixel : clipped.pixels())
        if (pixel != 3)
            return 1;
    std::cout
        << "Preview depth buffer: occlusion, order, winding, clipping and degeneracy passed\n";
    return 0;
}

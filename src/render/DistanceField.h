#pragma once

#include <QImage>

#include <vector>

namespace avloop::render {

// The exact Euclidean distance transform (Felzenszwalb–Huttenlocher, two passes of the 1-D lower envelope, O(W·H)) of a
// shape's raster (D-024): for each pixel centre its signed distance in px to the shape's edge — negative inside, where
// coverage ≥ ½ — the edge taken half way between an inside and an outside pixel. Memory: two float grids of the
// raster's size while it runs (the output size, bounded by 1920 × 1920).
std::vector<float> signedDistance(const QImage& coverage);

// The squared distance of every cell to the nearest cell where `seed` is true (+∞ when none): the transform itself,
// exposed for the tests' brute-force check.
std::vector<float> squaredDistance(const std::vector<bool>& seed, int width, int height);

} // namespace avloop::render

#include "render/DistanceField.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace avloop::render {

namespace {

constexpr float kInf = std::numeric_limits<float>::infinity();

// One line's squared distances: d(q) = min over p of (q − p)² + f(p), by the lower envelope of the parabolas.
void transform1d(const float* f, float* d, int n, std::vector<int>& v, std::vector<float>& z) {
    int k = 0;
    int first = 0;
    while (first < n && f[first] == kInf)
        ++first;
    if (first == n) {
        for (int q = 0; q < n; ++q)
            d[q] = kInf;
        return;
    }
    v[0] = first;
    z[0] = -kInf;
    z[1] = kInf;
    for (int q = first + 1; q < n; ++q) {
        if (f[q] == kInf) continue;
        float s = 0;
        for (;;) {
            const int p = v[k];
            s = ((f[q] + float(q) * q) - (f[p] + float(p) * p)) / (2.f * float(q - p));
            if (s > z[k] || k == 0) break;
            --k;
        }
        if (s <= z[k]) {
            v[0] = q; // k == 0 and the new parabola lies below the whole envelope
            z[1] = kInf;
            continue;
        }
        ++k;
        v[k] = q;
        z[k] = s;
        z[k + 1] = kInf;
    }
    k = 0;
    for (int q = 0; q < n; ++q) {
        while (z[k + 1] < float(q))
            ++k;
        const float dq = float(q - v[k]);
        d[q] = dq * dq + f[v[k]];
    }
}

} // namespace

std::vector<float> squaredDistance(const std::vector<bool>& seed, int width, int height) {
    std::vector<float> grid(size_t(width) * height);
    for (size_t i = 0; i < grid.size(); ++i)
        grid[i] = seed[i] ? 0.f : kInf;
    const size_t longest = size_t(std::max(width, height));
    std::vector<float> f(longest), d(longest), z(longest + 1);
    std::vector<int> v(longest);
    // Columns, then rows.
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y)
            f[y] = grid[size_t(y) * width + x];
        transform1d(f.data(), d.data(), height, v, z);
        for (int y = 0; y < height; ++y)
            grid[size_t(y) * width + x] = d[y];
    }
    for (int y = 0; y < height; ++y) {
        float* row = grid.data() + size_t(y) * width;
        transform1d(row, d.data(), width, v, z);
        std::copy(d.begin(), d.begin() + width, row);
    }
    return grid;
}

std::vector<float> signedDistance(const QImage& coverage) {
    const int w = coverage.width();
    const int h = coverage.height();
    std::vector<bool> inside(size_t(w) * h);
    for (int y = 0; y < h; ++y) {
        const uchar* row = coverage.constScanLine(y);
        for (int x = 0; x < w; ++x)
            inside[size_t(y) * w + x] = row[x] >= 128;
    }
    std::vector<bool> outside(inside.size());
    for (size_t i = 0; i < inside.size(); ++i)
        outside[i] = !inside[i];
    const std::vector<float> toInside = squaredDistance(inside, w, h);
    std::vector<float> out = squaredDistance(outside, w, h);
    // Outside: the distance to the nearest inside centre less half a pixel; inside, the negative of the same.
    for (size_t i = 0; i < out.size(); ++i)
        out[i] = inside[i] ? -(std::sqrt(out[i]) - 0.5f) : std::sqrt(toInside[i]) - 0.5f;
    return out;
}

} // namespace avloop::render

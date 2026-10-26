#include "render/CpuRenderer.h"

#include <QtConcurrent/QtConcurrentMap>

#include <algorithm>
#include <cmath>
#include <numeric>

namespace avloop::render {

namespace {

// GL_LINEAR with GL_CLAMP_TO_BORDER (0, 0, 0, 0): the four texels around (u, v), those outside transparent.
inline void sample(const media::Frame& f, double u, double v, float out[4]) {
    const double tx = u * f.width - 0.5;
    const double ty = v * f.height - 0.5;
    const double fx0 = std::floor(tx);
    const double fy0 = std::floor(ty);
    const auto fx = float(tx - fx0);
    const auto fy = float(ty - fy0);
    const int x0 = int(fx0);
    const int y0 = int(fy0);
    out[0] = out[1] = out[2] = out[3] = 0;
    const float w[4] = {(1 - fx) * (1 - fy), fx * (1 - fy), (1 - fx) * fy, fx * fy};
    const int xs[4] = {x0, x0 + 1, x0, x0 + 1};
    const int ys[4] = {y0, y0, y0 + 1, y0 + 1};
    for (int i = 0; i < 4; ++i) {
        if (xs[i] < 0 || ys[i] < 0 || xs[i] >= f.width || ys[i] >= f.height || w[i] == 0) continue;
        const std::uint8_t* p = f.data.get() + std::ptrdiff_t(ys[i]) * f.stride + std::ptrdiff_t(xs[i]) * 4;
        for (int c = 0; c < 4; ++c)
            out[c] += w[i] * (p[c] / 255.f);
    }
}

inline std::uint8_t unorm(float v) {
    return std::uint8_t(std::lround(std::clamp(v, 0.f, 1.f) * 255.f));
}

} // namespace

void CpuRenderer::render(const Composite& c, std::uint8_t* out, int stride) {
    const int width = c.size.width();
    if (rows_.size() != c.size.height()) {
        rows_.resize(c.size.height());
        std::iota(rows_.begin(), rows_.end(), 0);
    }
    const float bg[3] = {float(c.background.redF()), float(c.background.greenF()), float(c.background.blueF())};
    QtConcurrent::blockingMap(rows_, [&](int y) {
        std::uint8_t* row = out + std::ptrdiff_t(y) * stride;
        const std::uint8_t* maskRow = c.mask ? c.mask->constScanLine(y) : nullptr;
        for (int x = 0; x < width; ++x) {
            float sum[4] = {0, 0, 0, 0};
            for (const Layer& l : c.layers) {
                const double u = l.place.ax * (x + 0.5) + l.place.bx;
                const double v = l.place.ay * (y + 0.5) + l.place.by;
                float s[4];
                sample(l.frame, u, v, s);
                for (int k = 0; k < 4; ++k)
                    sum[k] += float(l.weight) * s[k];
            }
            if (maskRow) {
                const float m = maskRow[x] / 255.f;
                for (float& k : sum)
                    k *= m;
            }
            if (!c.alpha) {
                for (int k = 0; k < 3; ++k)
                    sum[k] += bg[k] * (1 - sum[3]);
                sum[3] = 1;
            }
            std::uint8_t* p = row + std::ptrdiff_t(x) * 4;
            for (int k = 0; k < 4; ++k)
                p[k] = unorm(sum[k]);
        }
    });
}

} // namespace avloop::render

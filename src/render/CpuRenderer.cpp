#include "render/CpuRenderer.h"

#include <QtConcurrent/QtConcurrentMap>

#include <algorithm>
#include <cmath>
#include <numeric>

namespace avloop::render {

namespace {

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
                sampleLinear(l.frame, u, v, s);
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

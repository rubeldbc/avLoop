#include "render/EffectsPass.h"

#include <QtConcurrent/QtConcurrentMap>

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <numeric>

namespace avloop::render {

namespace {

constexpr int kStrip = 16; // the columns one task of a vertical pass walks down together

// The radii of three boxes whose passes make a Gaussian of σ (Kutskir's "boxes for gauss"); 0: no pass.
std::array<int, 3> boxRadii(double sigma) {
    std::array<int, 3> r{0, 0, 0};
    if (sigma <= 0) return r;
    int wl = int(std::floor(std::sqrt(4 * sigma * sigma + 1)));
    if (wl % 2 == 0) --wl;
    const int wu = wl + 2;
    const double ideal = (12 * sigma * sigma - 3.0 * wl * wl - 12.0 * wl - 9) / (-4.0 * wl - 4);
    const long m = std::lround(ideal);
    for (int i = 0; i < 3; ++i)
        r[std::size_t(i)] = ((i < m ? wl : wu) - 1) / 2;
    return r;
}

// One row through a box of 2r + 1 px, the edge pixel held beyond the edge; integer sums, rounded once.
void boxRow(const std::uint8_t* s, std::uint8_t* d, int w, int r) {
    const int n = 2 * r + 1;
    for (int c = 0; c < 4; ++c) {
        int acc = (r + 1) * s[c];
        for (int i = 1; i <= r; ++i)
            acc += s[std::min(i, w - 1) * 4 + c];
        for (int x = 0; x < w; ++x) {
            d[x * 4 + c] = std::uint8_t((acc + n / 2) / n);
            acc += s[std::min(x + r + 1, w - 1) * 4 + c] - s[std::max(x - r, 0) * 4 + c];
        }
    }
}

// Columns x0 … x1 − 1 down through the same box.
void boxColumns(const std::uint8_t* s, int ss, std::uint8_t* d, int ds, int h, int x0, int x1, int r) {
    const int n = 2 * r + 1;
    const int width = (x1 - x0) * 4;
    std::array<int, kStrip * 4> acc{};
    const std::uint8_t* top = s + x0 * 4;
    for (int k = 0; k < width; ++k) {
        int a = (r + 1) * top[k];
        for (int i = 1; i <= r; ++i)
            a += top[std::ptrdiff_t(std::min(i, h - 1)) * ss + k];
        acc[std::size_t(k)] = a;
    }
    for (int y = 0; y < h; ++y) {
        const std::uint8_t* add = top + std::ptrdiff_t(std::min(y + r + 1, h - 1)) * ss;
        const std::uint8_t* sub = top + std::ptrdiff_t(std::max(y - r, 0)) * ss;
        std::uint8_t* out = d + std::ptrdiff_t(y) * ds + x0 * 4;
        for (int k = 0; k < width; ++k) {
            out[k] = std::uint8_t((acc[std::size_t(k)] + n / 2) / n);
            acc[std::size_t(k)] += add[k] - sub[k];
        }
    }
}

inline std::uint8_t unorm(float v) {
    return std::uint8_t(std::lround(std::clamp(v, 0.f, 1.f) * 255.f));
}

} // namespace

EffectsPass::Plane EffectsPass::gauss(Plane src, int w, int h, double sigma, std::vector<std::uint8_t>& one,
                                      std::vector<std::uint8_t>& two) {
    const std::array<int, 3> radii = boxRadii(sigma);
    const std::size_t bytes = std::size_t(w) * std::size_t(h) * 4;
    if (one.size() != bytes) one.assign(bytes, 0);
    if (two.size() != bytes) two.assign(bytes, 0);
    Plane cur = src;
    bool toOne = true;
    for (int pass = 0; pass < 6; ++pass) {
        const int r = radii[std::size_t(pass % 3)];
        if (r == 0) continue;
        std::uint8_t* dst = toOne ? one.data() : two.data();
        if (pass < 3) {
            QtConcurrent::blockingMap(rows_, [&](int y) {
                boxRow(cur.data + std::ptrdiff_t(y) * cur.stride, dst + std::ptrdiff_t(y) * w * 4, w, r);
            });
        } else {
            QtConcurrent::blockingMap(strips_, [&](int strip) {
                const int x0 = strip * kStrip;
                boxColumns(cur.data, cur.stride, dst, w * 4, h, x0, std::min(w, x0 + kStrip), r);
            });
        }
        cur = {dst, w * 4};
        toOne = !toOne;
    }
    return cur;
}

media::Frame EffectsPass::apply(const ClipEffects& e, const media::Frame& in, double pxScale) {
    if (e.isNeutral() || in.isNull() || in.width <= 0 || in.height <= 0) return in;
    const int w = in.width;
    const int h = in.height;
    if (rows_.size() != h) {
        rows_.resize(h);
        std::iota(rows_.begin(), rows_.end(), 0);
    }
    const int strips = (w + kStrip - 1) / kStrip;
    if (strips_.size() != strips) {
        strips_.resize(strips);
        std::iota(strips_.begin(), strips_.end(), 0);
    }
    Plane base{in.data.get(), in.stride};
    if (e.blur > 0) base = gauss(base, w, h, e.blurSigma() * pxScale, a_, b_);
    Plane soft;
    if (e.sharpen > 0)
        soft = gauss(base, w, h, std::max(0.5, ClipEffects::kSharpenSigma * pxScale), c_,
                     base.data == a_.data() ? b_ : a_);

    auto buffer = std::make_shared<std::vector<std::uint8_t>>(std::size_t(w) * std::size_t(h) * 4);
    std::uint8_t* out = buffer->data();
    const effects::Colour colour(e);
    const bool changes = e.changesColour();
    const bool straight = changes || e.posterize >= 2 || e.vignette != 0 || e.opacity < 100;
    const auto amount = float(e.sharpen / 100 * 2);
    const auto opacity = float(e.opacity / 100);
    QtConcurrent::blockingMap(rows_, [&](int y) {
        const std::uint8_t* row = base.data + std::ptrdiff_t(y) * base.stride;
        const std::uint8_t* softRow = soft.data ? soft.data + std::ptrdiff_t(y) * soft.stride : nullptr;
        std::uint8_t* o = out + std::ptrdiff_t(y) * w * 4;
        for (int x = 0; x < w; ++x) {
            const std::uint8_t* p = row + x * 4;
            float c[4] = {p[0] / 255.f, p[1] / 255.f, p[2] / 255.f, p[3] / 255.f};
            if (softRow)
                for (int i = 0; i < 3; ++i)
                    c[i] = std::clamp(c[i] + amount * (c[i] - softRow[x * 4 + i] / 255.f), 0.f, c[3]);
            if (straight) {
                if (c[3] <= 0) {
                    std::fill(o + x * 4, o + x * 4 + 4, std::uint8_t(0));
                    continue;
                }
                float rgb[3] = {std::min(1.f, c[0] / c[3]), std::min(1.f, c[1] / c[3]), std::min(1.f, c[2] / c[3])};
                if (changes) colour(rgb);
                if (e.posterize >= 2)
                    for (float& v : rgb)
                        v = e.posterized(v);
                if (e.vignette != 0) e.vignetteOn(rgb, e.vignetteWeight((x + 0.5f) / w, (y + 0.5f) / h));
                c[3] *= opacity;
                for (int i = 0; i < 3; ++i)
                    c[i] = rgb[i] * c[3];
            }
            for (int i = 0; i < 4; ++i)
                o[x * 4 + i] = unorm(c[i]);
        }
    });
    media::Frame f = in;
    f.stride = w * 4;
    f.data = std::shared_ptr<const std::uint8_t>(buffer, buffer->data());
    return f;
}

} // namespace avloop::render

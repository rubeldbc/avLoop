#pragma once

// The renderer's unit cases (AVADR-0001): small synthetic frames whose right answers are known — identity, half and
// double size, flips, a half-texel offset, a mask, a 0.3 blend, opaque over a colour. test-renderer-cpu checks the
// CPU back end's answers; test-renderer checks the GPU back end against it.

#include "render/CpuRenderer.h"

#include <QImage>

#include <array>

#include <memory>
#include <vector>

namespace avloop::test {

// A w × h premultiplied RGBA frame, pixel (x, y) from `fill`.
template <typename Fill> media::Frame makeFrame(int w, int h, std::int64_t pts, Fill fill) {
    auto bytes = std::make_shared<std::vector<std::uint8_t>>(std::size_t(w) * h * 4);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            const auto c = fill(x, y);
            for (int k = 0; k < 4; ++k)
                (*bytes)[(std::size_t(y) * w + x) * 4 + k] = std::uint8_t(c[k]);
        }
    media::Frame f;
    f.width = w;
    f.height = h;
    f.stride = w * 4;
    f.pts = pts;
    f.data = std::shared_ptr<const std::uint8_t>(bytes, bytes->data());
    return f;
}

// A gradient with every pixel different (and opaque).
inline media::Frame gradient(int w, int h, std::int64_t pts = 1) {
    return makeFrame(w, h, pts, [](int x, int y) {
        return std::array<int, 4>{(x * 37 + y * 11) % 256, (x * 5 + y * 53) % 256, (x * 101 + y * 7) % 256, 255};
    });
}

// Output pixel x samples texel x of a same-sized frame: u = (x + ½) / w.
inline render::Placement fit(int outW, int outH, double scaleX = 1, double scaleY = 1, double dx = 0, double dy = 0) {
    render::Placement p;
    p.ax = scaleX / outW;
    p.bx = dx;
    p.ay = scaleY / outH;
    p.by = dy;
    return p;
}

struct Case {
    const char* name;
    render::Composite composite;
    std::shared_ptr<QImage> mask;
};

inline std::vector<Case> cases() {
    std::vector<Case> list;
    list.reserve(16); // the cases are filled in through pointers to them
    auto add = [&](const char* name, QSize size, QVector<render::Layer> layers) {
        Case c{name, {}, {}};
        c.composite.size = size;
        c.composite.layers = std::move(layers);
        list.push_back(c);
        return &list.back();
    };
    const media::Frame g = gradient(16, 12);
    add("identity", {16, 12}, {{g, 1, fit(16, 12)}});
    add("half size", {8, 6}, {{g, 1, fit(8, 6)}});
    add("double size", {32, 24}, {{g, 1, fit(32, 24)}});
    render::Placement flip = fit(16, 12);
    flip.ax = -flip.ax;
    flip.bx = 1;
    add("flip H", {16, 12}, {{g, 1, flip}});
    add("half-texel offset", {16, 12}, {{g, 1, fit(16, 12, 1, 1, 0.5 / 16, 0)}});
    add("outside is transparent", {16, 12}, {{g, 1, fit(16, 12, 2, 2, -0.5, -0.5)}});
    const media::Frame red = makeFrame(8, 8, 2, [](int, int) { return std::array<int, 4>{200, 20, 10, 255}; });
    const media::Frame blue = makeFrame(8, 8, 3, [](int, int) { return std::array<int, 4>{0, 40, 180, 255}; });
    add("blend 0.3", {8, 8}, {{red, 0.7, fit(8, 8)}, {blue, 0.3, fit(8, 8)}});
    Case* masked = add("mask", {16, 12}, {{g, 1, fit(16, 12)}});
    masked->mask = std::make_shared<QImage>(16, 12, QImage::Format_Grayscale8);
    for (int y = 0; y < 12; ++y)
        for (int x = 0; x < 16; ++x)
            masked->mask->scanLine(y)[x] = std::uint8_t((x * 16 + y * 5) % 256);
    masked->composite.mask = masked->mask.get();
    const media::Frame half =
        makeFrame(8, 8, 4, [](int x, int) { return std::array<int, 4>{x * 16, x * 16, 0, x * 32}; });
    Case* opaque = add("opaque over a colour", {8, 8}, {{half, 1, fit(8, 8)}});
    opaque->composite.alpha = false;
    opaque->composite.background = QColor(30, 60, 90);
    return list;
}

} // namespace avloop::test

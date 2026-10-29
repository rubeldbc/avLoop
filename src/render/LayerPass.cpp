#include "render/LayerPass.h"

#include "media/Decoder.h"
#include "media/MediaProbe.h"

#include <algorithm>
#include <cmath>

namespace avloop::render {

namespace {

// A layer's frame at texture coordinates (u, v), bilinear, texel centres at +½, clear outside (as the back ends).
void sample(const media::Frame& f, double u, double v, float out[4]) {
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

} // namespace

LayerPass::LayerPass() = default;
LayerPass::~LayerPass() = default;

void LayerPass::setLayers(const QList<VideoLayer>& layers, const QRectF& frame, const QSize& outputSize) {
    layers_ = layers.mid(0, VideoLayer::kMaxLayers);
    order_ = videolayers::drawOrder(layers_);
    frame_ = frame;
    size_ = outputSize;
    decoders_.clear();
    decoders_.resize(size_t(layers_.size()));
    infos_.assign(size_t(layers_.size()), media::MediaInfo{});
    failed_.assign(size_t(layers_.size()), false);
}

int LayerPass::open() const {
    return int(std::count_if(decoders_.begin(), decoders_.end(), [](const auto& d) { return bool(d); }));
}

media::Frame LayerPass::frameOf(int index, double own) {
    const size_t i = size_t(index);
    if (!decoders_[i] && !failed_[i]) {
        const media::MediaProbe::Result probed = media::MediaProbe::probe(layers_[index].path);
        if (probed.info) {
            infos_[i] = *probed.info;
            media::Decoder::Options options;
            options.hardware = false; // a layer's few frames: the software decoder, no second GPU device
            decoders_[i] = std::make_unique<media::Decoder>(infos_[i], options);
            if (!decoders_[i]->open()) decoders_[i].reset();
        }
        failed_[i] = !decoders_[i];
    }
    return decoders_[i] ? decoders_[i]->frameAt(own + 1e-4) : media::Frame{};
}

void LayerPass::apply(double seconds, QImage& out, const QImage* mask, bool alpha, const QColor& background) {
    const int w = size_.width(), h = size_.height();
    for (int index : order_) {
        const VideoLayer& l = layers_[index];
        double own = 0;
        if (!l.visible || l.opacity <= 0 || !l.sourceTime(seconds, &own)) continue;
        const media::Frame f = frameOf(index, own);
        if (f.isNull()) continue;
        // Output px → scene → the layer's own 0 … 1 (its flips folded in).
        const QRectF r = l.rect();
        if (r.isEmpty()) continue;
        const double ax = frame_.width() / (w * r.width()), bx = (frame_.left() - r.left()) / r.width();
        const double ay = frame_.height() / (h * r.height()), by = (frame_.top() - r.top()) / r.height();
        const float opacity = float(l.opacity / 100.0);
        for (int y = 0; y < h; ++y) {
            std::uint8_t* row = out.scanLine(y);
            const uchar* m = mask ? mask->constScanLine(y) : nullptr;
            double v = ay * (y + 0.5) + by;
            if (l.transform.flipV) v = 1 - v;
            if (v < -0.01 || v > 1.01) continue;
            for (int x = 0; x < w; ++x) {
                double u = ax * (x + 0.5) + bx;
                if (l.transform.flipH) u = 1 - u;
                if (u < -0.01 || u > 1.01) continue;
                float s[4];
                sample(f, u, v, s);
                const float a = s[3] * opacity;
                if (a <= 0) continue;
                const float k = m ? m[x] / 255.f : 1.f;
                std::uint8_t* p = row + std::ptrdiff_t(x) * 4;
                for (int c = 0; c < 4; ++c) {
                    const float layer = (c == 3 ? a : s[c] * opacity) * k;
                    p[c] = std::uint8_t(std::lround(std::clamp(layer + p[c] / 255.f * (1 - a), 0.f, 1.f) * 255.f));
                }
            }
        }
    }
    if (alpha) return;
    const float bg[3] = {float(background.redF()), float(background.greenF()), float(background.blueF())};
    for (int y = 0; y < h; ++y) {
        std::uint8_t* row = out.scanLine(y);
        for (int x = 0; x < w; ++x) {
            std::uint8_t* p = row + std::ptrdiff_t(x) * 4;
            const float clear = 1 - p[3] / 255.f;
            for (int c = 0; c < 3; ++c)
                p[c] = std::uint8_t(std::lround(std::clamp(p[c] / 255.f + bg[c] * clear, 0.f, 1.f) * 255.f));
            p[3] = 255;
        }
    }
}

} // namespace avloop::render

#pragma once

#include "core/VideoLayer.h"
#include "media/MediaInfo.h"
#include "render/EffectsPass.h"

#include <QColor>
#include <QImage>
#include <QList>
#include <QRectF>

#include <memory>
#include <vector>

namespace avloop::media {
class Decoder;
} // namespace avloop::media

namespace avloop::render {

// The video layers over the main video (D-027, AVADR-0001): after a back end has drawn the main video's frame —
// masked, premultiplied, with alpha — each shown layer present at the frame's time is sampled bilinearly into the
// output by its own place and scale, at its opacity, and laid over in track order inside the crop's mask:
// acc = mask·L + acc·(1 − αL); with Alpha Off the background goes under last. The same CPU pass for both back ends,
// so the GPU and the CPU agree. One decoder a layer, opened on first use (at most 8, VideoLayer::kMaxLayers).
class LayerPass {
public:
    LayerPass();
    ~LayerPass();

    void setLayers(const QList<VideoLayer>& layers, const QRectF& frame, const QSize& outputSize);
    bool empty() const { return layers_.isEmpty(); }
    // Lays the layers present at sequence time `seconds` over `out` (premultiplied RGBA8 of the output size); `mask`
    // the crop's (or null); `alpha` false: the background under everything, alpha 255.
    void apply(double seconds, QImage& out, const QImage* mask, bool alpha, const QColor& background);
    int open() const; // decoders open (the memory walk)

private:
    media::Frame frameOf(int index, double own);

    QList<VideoLayer> layers_;
    QList<int> order_;
    QRectF frame_;
    QSize size_;
    std::vector<std::unique_ptr<media::Decoder>> decoders_;
    std::vector<media::MediaInfo> infos_;
    std::vector<bool> failed_;
    EffectsPass effects_; // a layer's own clip effects (D-029)
};

} // namespace avloop::render

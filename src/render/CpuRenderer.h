#pragma once

#include "media/MediaInfo.h"
#include "render/RenderJob.h"

#include <QColor>
#include <QImage>
#include <QSize>
#include <QVector>

#include <cstdint>

namespace avloop::render {

// One source frame of an output frame (spec §3.4 steps 3–5): where it lands and its share of the blend.
struct Layer {
    media::Frame frame;
    double weight = 1;
    Placement place;
};

// What one output frame is made of: up to four weighted layers (the loop's blend × the FPS method's), the crop's
// mask, and — without alpha — the colour behind.
struct Composite {
    QSize size;
    QVector<Layer> layers;
    const QImage* mask = nullptr; // Grayscale8 of `size`, or none
    bool alpha = true;
    QColor background = Qt::black;
};

// The CPU back end (AVADR-0001): for each output pixel the layers sampled bilinearly (texel centres at +½,
// transparent outside), summed by weight in float, times the mask, over the background when opaque, rounded once.
// The GPU back end does the same maths. Rows run in parallel; nothing is allocated a frame.
class CpuRenderer {
public:
    // Premultiplied RGBA8 into `out` (`stride` bytes a row).
    void render(const Composite& c, std::uint8_t* out, int stride);

private:
    QVector<int> rows_;
};

} // namespace avloop::render

#pragma once

#include "media/FfmpegPtr.h"

#include <QByteArray>
#include <QSize>
#include <QString>

#include <cstdint>

namespace avloop::media {

// GIF's colours (spec §10.4): `palettegen` (stats_mode=diff, one entry kept for transparency) over every frame makes
// the palette — 256 colours, 16×16 RGB32, 1 KB —, then `paletteuse` (bayer dithering, alpha ≥ 128 opaque) maps each
// frame onto it as PAL8 for the `gif` encoder. Frames in: straight RGBA of the output size. One thread drives each.
class GifPaletteMaker {
public:
    bool open(QSize size, QString* error = nullptr);
    bool add(const std::uint8_t* rgba, int stride, QString* error = nullptr);
    // The palette, after every frame: 1024 bytes, or empty on failure.
    QByteArray finish(QString* error = nullptr);

private:
    FilterGraphPtr graph_;
    AVFilterContext* src_ = nullptr;
    AVFilterContext* sink_ = nullptr;
    QSize size_;
    std::int64_t pts_ = 0;
};

class GifQuantizer {
public:
    bool open(QSize size, const QByteArray& palette, QString* error = nullptr);
    // Frame `rgba` as PAL8 into `out` (its palette in data[1]).
    bool quantize(const std::uint8_t* rgba, int stride, AVFrame* out, QString* error = nullptr);

    // The palette `GifPaletteMaker` makes of one frame alone: a single pass's GIF (the Encoded preview, the estimate).
    static QByteArray paletteOf(QSize size, const std::uint8_t* rgba, int stride);

private:
    FilterGraphPtr graph_;
    AVFilterContext* src_ = nullptr;
    AVFilterContext* sink_ = nullptr;
    QSize size_;
    std::int64_t pts_ = 0;
};

} // namespace avloop::media

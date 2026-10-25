#pragma once

#include "core/Rational.h"

#include <QMetaType>
#include <QString>

#include <cstdint>
#include <memory>

namespace avloop::media {

// What MediaProbe reads of a file (spec §3.9, §12.1). Sizes are the display size: after the rotation and with square
// pixels (SAR applied), as the canvas shows the video.
struct MediaInfo {
    QString path;
    QString container;     // short name ("mov,mp4,m4a,3gp,3g2,mj2")
    QString containerLong; // "QuickTime / MOV"
    QString codec;         // "h264"
    QString codecLong;     // "H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10"
    int streamIndex = -1;
    int codedWidth = 0;
    int codedHeight = 0;
    int width = 0;  // display width (rotation and SAR applied)
    int height = 0; // display height
    Rational sar{1, 1};
    int rotation = 0; // clockwise degrees the picture is turned for display: 0, 90, 180, 270
    bool flipH = false;
    bool flipV = false;
    Rational avgFps;
    Rational realFps;
    bool vfr = false;
    double duration = 0;         // seconds
    std::int64_t frameCount = 0; // estimate
    Rational timeBase{1, 1};
    std::int64_t startPts = 0;
    QString pixelFormat;
    int bitDepth = 8;
    bool hasAlpha = false;
    bool hdr = false;
    QString transfer; // "smpte2084", "arib-std-b67", "bt709" …
    bool interlaced = false;
    bool hardwareCandidate = false; // a codec D3D11VA can decode (spec §3.6)

    bool isValid() const { return streamIndex >= 0 && width > 0 && height > 0; }
    double fps() const { return avgFps.isValid() ? avgFps.value() : realFps.value(); }
    // "1920×1080 · 29.97 fps · H.264 · VFR → CFR" (spec §5.1).
    QString statusLine() const;
};

// One decoded, normalized frame: RGBA8, premultiplied alpha, at the working size. Its pixels are shared and return to
// FFmpeg's frame pool when the last copy goes (spec §3.7.4).
struct Frame {
    int width = 0;
    int height = 0;
    int stride = 0; // bytes per row
    std::int64_t pts = 0;
    double seconds = 0; // source time, the stream's start subtracted
    std::shared_ptr<const std::uint8_t> data;

    bool isNull() const { return !data; }
    std::int64_t bytes() const { return std::int64_t(stride) * height; }
};

} // namespace avloop::media

Q_DECLARE_METATYPE(avloop::media::Frame)

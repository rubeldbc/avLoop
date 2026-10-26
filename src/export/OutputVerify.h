#pragma once

#include "core/QualitySettings.h"
#include "core/Rational.h"

#include <QSize>
#include <QString>

#include <cstdint>

namespace avloop::exporting {

// What a written loop must be (spec §10.1 step 5).
struct Expected {
    OutputFormat format = OutputFormat::WebmVp9;
    QSize size;
    Rational fps;
    int frames = 0;
    bool alpha = false;
};

// What the file is, read back: the stream, its packets counted, its first frame decoded by libvpx (alpha kept).
struct Verified {
    bool opened = false;
    QString codec; // "vp9", "vp8"
    QSize size;
    Rational fps;
    int frames = 0;
    bool firstKey = false;
    bool alphaMarked = false; // the stream's alpha_mode
    bool alphaPlane = false;  // the first frame decoded with an alpha plane
    int audioStreams = 0;
    std::int64_t bytes = 0;
    QString problem; // empty when it is what was expected
};

// Reopens `path` and checks frame count, rate, size, codec, alpha and the first keyframe (spec §10.1 step 5).
Verified verifyOutput(const QString& path, const Expected& expected);

} // namespace avloop::exporting

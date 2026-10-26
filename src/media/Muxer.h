#pragma once

#include "media/FfmpegPtr.h"

#include <QString>

namespace avloop::media {

// The WebM container around an encoded loop (spec §10.1): one video stream copied from the encoder's parameters,
// `alpha_mode` set when the frames carry alpha, packets rescaled from the encoder's time base, no audio.
class Muxer {
public:
    Muxer() = default;
    ~Muxer();
    Muxer(const Muxer&) = delete;
    Muxer& operator=(const Muxer&) = delete;

    bool open(const QString& path, const AVCodecContext* codec, bool alpha, QString* error = nullptr);
    bool write(AVPacket* packet, QString* error = nullptr);
    bool finish(QString* error = nullptr);
    void close();                                   // without the trailer
    bool outOfSpace() const { return outOfSpace_; } // a write failed for want of disk space (spec §12.3)

private:
    AVFormatContext* fmt_ = nullptr;
    AVStream* stream_ = nullptr;
    AVRational codecTimeBase_{1, 1};
    bool headerWritten_ = false;
    bool outOfSpace_ = false;
};

} // namespace avloop::media

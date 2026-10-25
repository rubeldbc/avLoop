#pragma once

// The decoder's private state, shared by Decoder.cpp (open, read, decode), DecoderSeek.cpp (frame-accurate access)
// and DecoderHw.cpp (D3D11VA). Internal to media/.

#include "media/Decoder.h"
#include "media/FfmpegPtr.h"
#include "media/SourceNormalizer.h"

#include <QString>

#include <memory>

namespace avloop::media {

struct Decoder::Impl {
    MediaInfo info;
    Options options;
    FormatContextPtr fmt;
    CodecContextPtr ctx;
    AVStream* stream = nullptr;
    BufferRefPtr hwDevice;
    bool hwActive = false;
    QString hwNote;
    PacketPtr packet;
    // Three neighbours in display order: prev < last (the frame on screen) < pending (decoded ahead).
    FramePtr prevRaw;
    FramePtr lastRaw;
    FramePtr pendingRaw;
    Frame lastFrame; // lastRaw normalized, made when first asked
    std::unique_ptr<SourceNormalizer> normalizer;
    bool flushed = false;       // the end of the stream was sent to the codec
    bool atEnd = false;         // the codec has nothing more
    bool needReseek = false;    // hardware failed mid-way: the position must be found again in software
    bool waitKey = false;       // after a seek: packets before the next keyframe are dropped
    bool keysUnflagged = false; // a container that never flags keyframes: no waiting
    bool rewound = false;       // the last seek went to the start of the file
    int droppedForKey = 0;
    std::int64_t lastPtsSeen = AV_NOPTS_VALUE;
    std::int64_t frameDuration = 1; // pts units, for frames without a timestamp
    int corrupt = 0;
    int seekCount = 0;

    // Decoder.cpp
    bool openCodec(bool hardware, QString* error);
    FramePtr decodeRaw();
    Frame current();
    double seconds(std::int64_t pts) const;
    std::int64_t ptsOf(double seconds) const;
    QSize computeWorkingSize() const;
    // DecoderSeek.cpp
    void seekTo(std::int64_t pts);
    Frame locate(std::int64_t target);
    Frame frameAtPts(std::int64_t target);
    // DecoderHw.cpp
    bool setupHardware(AVCodecContext* c, const AVCodec* codec);
    bool toSoftware(const AVFrame* in, FramePtr& out);
    void fallbackToSoftware(const QString& why);
};

} // namespace avloop::media

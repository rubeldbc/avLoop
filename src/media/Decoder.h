#pragma once

#include "media/MediaInfo.h"

#include <QSize>
#include <QString>

#include <memory>

namespace avloop::media {

// Decodes one video stream frame-accurately (spec §7.3, §3.6): a seek goes BACKWARD to the keyframe and decodes up to
// the target; moves forward within two seconds decode on without seeking. Frames come out normalized (rotation, SAR,
// deinterlace, HDR → SDR, alpha kept, premultiplied RGBA) at the working size. D3D11VA is tried first for the codecs
// it knows, software decoding takes over on any failure. Not thread-safe: one thread owns a decoder (spec §3.5).
class Decoder {
public:
    struct Options {
        bool hardware = true;   // "Hardware decoding: Auto" (false = Off)
        QSize workingSize;      // display orientation; empty = the display size, long side capped at maxLongSide
        int maxLongSide = 1920; // a 4K source is scaled down right after decoding (spec §3.7.3)
        int threads = 0;        // software decoding's threads; 0 = one a logical core (each holds frames: memory)
    };

    explicit Decoder(const MediaInfo& info);
    Decoder(const MediaInfo& info, const Options& options);
    ~Decoder();
    Decoder(const Decoder&) = delete;
    Decoder& operator=(const Decoder&) = delete;

    bool open(QString* error = nullptr);
    const MediaInfo& info() const;

    // The frame on screen at `seconds` of source time (its PTS ≤ t < the next frame's PTS); the first frame before
    // it, the last after it. Null only when nothing decodes.
    Frame frameAt(double seconds);
    // The scrub's fast frame (spec §7.2): the keyframe at or before `seconds` — or, already near it, the frame on
    // screen — without decoding up to it.
    Frame keyframeAt(double seconds);
    // Frame N in display order for a constant-rate source (t = (N + ½) / fps).
    Frame frameAtIndex(std::int64_t index);
    // The next frame in display order after the last one returned (sequential playback); null at the end.
    Frame next();

    void setWorkingSize(const QSize& size);
    QSize workingSize() const;
    bool hardwareActive() const;
    QString hardwareNote() const; // why hardware decoding is off, said once a file
    int corruptFrames() const;    // skipped frames (spec §12.1)
    int seeks() const;            // seeks performed (frame-accuracy tests)

    struct Impl;

private:
    std::unique_ptr<Impl> d;
};

} // namespace avloop::media

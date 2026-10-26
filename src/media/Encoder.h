#pragma once

#include "core/QualitySettings.h"
#include "core/Rational.h"

#include <QByteArray>
#include <QSize>
#include <QString>

#include <cstdint>
#include <memory>

struct AVFrame;
struct AVPacket;

namespace avloop::media {

// What one encode is (spec §10.3): the format, the output size and rate, the loop's length, alpha, the quality.
struct EncodeSettings {
    OutputFormat format = OutputFormat::WebmVp9;
    QSize size;
    Rational fps{30, 1};
    int frames = 0; // L: the whole loop is one GOP
    bool alpha = true;
    QualitySettings quality;
    int threads = 0; // 0: one a logical core
    // Two passes (§10.3's Two-pass and Target size): pass 1 writes no file and leaves its statistics in
    // Encoder::stats(); pass 2 reads them from `passStats`.
    int pass = 0; // 0: one pass; 1 or 2
    QByteArray passStats;
    int bitrateKbps = 0;  // Target size: an average bitrate instead of the quality's CRF
    int lagInFrames = 25; // VP9's look-ahead (§10.3); a short encode needs no more than its frames
};

// The encoder AvLoop writes loops with (spec §10.1–§10.4): premultiplied RGBA frames in, made straight, then for
// WebM yuva420p (yuv420p without alpha; limited range, BT.709 for VP9, BT.601 for VP8, tagged) through libvpx-vp9 or
// libvpx with §10.3's parameters; GIF through its palette (two passes: the palette of every frame, then each frame
// mapped onto it; one pass takes the first frame's); APNG as RGBA; MP4 H.264 by Media Foundation (OpenH264 when it
// cannot); MOV ProRes 4444 in yuva444p10le. pts = k in 1/fps, the first frame a keyframe, no audio; WebM marks the
// alpha. Not thread-safe; one thread drives it.
class Encoder {
public:
    explicit Encoder(const EncodeSettings& settings);
    ~Encoder();
    Encoder(const Encoder&) = delete;
    Encoder& operator=(const Encoder&) = delete;

    // An empty path writes no file: the packets are only counted (pass 1, the size estimate).
    bool open(const QString& path, QString* error = nullptr);
    // Output frame k = the number of frames written so far (premultiplied RGBA8 of the output size).
    bool write(const std::uint8_t* rgba, int stride, QString* error = nullptr);
    // The encoder's delayed packets and the file's end; the file is complete only after this.
    bool finish(QString* error = nullptr);
    // Closes without finishing and deletes the file (a cancel or an error).
    void abort();
    int written() const { return written_; }
    std::int64_t bytes() const { return bytes_; }        // the packets so far, alpha included
    std::int64_t keyBytes() const { return keyBytes_; }  // of which keyframes
    QByteArray stats() const { return stats_; }          // pass 1's statistics, after finish()
    bool outOfSpace() const { return outOfSpace_; }      // a write failed for want of disk space
    QString encoderUsed() const { return encoderUsed_; } // "h264_mf", or its fallback "libopenh264" …
    const EncodeSettings& settings() const { return settings_; }

    struct State;

private:
    EncodeSettings settings_;
    QString path_;
    int written_ = 0;
    std::int64_t bytes_ = 0;
    std::int64_t keyBytes_ = 0;
    QByteArray stats_;
    bool outOfSpace_ = false;
    std::unique_ptr<State> s_;
    QString encoderUsed_;
    bool openCodec(QString* error);
    bool send(AVFrame* frame, QString* error); // null: the flush
    bool put(AVPacket* packet, QString* error);
    void close();
};

} // namespace avloop::media

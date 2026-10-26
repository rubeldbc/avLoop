#pragma once

#include "media/Encoder.h"
#include "media/FfmpegPtr.h"

namespace avloop::media {

// §10.3's and §10.4's encoder parameters, in one place for the Encoded preview and the export.
const char* encoderName(const EncodeSettings& s); // "libvpx-vp9", "libvpx", "gif", "apng", "h264_mf", "prores_ks"
const char* fallbackEncoderName(const EncodeSettings& s); // MP4: "libopenh264" when Media Foundation cannot; else null
const char* containerName(OutputFormat f);                // "webm", "gif", "apng", "mp4", "mov"
AVPixelFormat pixelFormat(const EncodeSettings& s);
// Size, rate, time base 1/fps, the one GOP, the pixel format, the colour tags, threads, bitrates.
void configure(AVCodecContext* c, const EncodeSettings& s);
// The encoder's private options (the caller frees them): VP8/VP9's crf, deadline, cpu-used …; ProRes's profile 4444.
AVDictionary* encoderOptions(const EncodeSettings& s);
// The container's: GIF loop=0, APNG plays=0, MP4 +faststart (the caller frees them).
AVDictionary* muxerOptions(OutputFormat f);
// log2(W / 256), at least 0 (VP9 tile columns).
int tileColumns(int width);

} // namespace avloop::media

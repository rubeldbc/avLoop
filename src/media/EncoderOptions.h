#pragma once

#include "media/Encoder.h"
#include "media/FfmpegPtr.h"

namespace avloop::media {

// §10.3's encoder parameters, in one place for the Encoded preview and the export.
const char* encoderName(const EncodeSettings& s); // "libvpx-vp9" or "libvpx"
// Size, rate, time base 1/fps, the one GOP, the pixel format, BT.709 limited range, threads, bitrates.
void configure(AVCodecContext* c, const EncodeSettings& s);
// crf, deadline good, cpu-used, row-mt, tile-columns, auto-alt-ref, lag-in-frames (the caller frees it).
AVDictionary* encoderOptions(const EncodeSettings& s);
// log2(W / 256), at least 0 (VP9 tile columns).
int tileColumns(int width);

} // namespace avloop::media

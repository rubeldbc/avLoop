#include "media/EncoderOptions.h"

#include <QThread>

#include <algorithm>
#include <cmath>

namespace avloop::media {

namespace {

bool isVpx(OutputFormat f) {
    return f == OutputFormat::WebmVp9 || f == OutputFormat::WebmVp8;
}

} // namespace

const char* encoderName(const EncodeSettings& s) {
    switch (s.format) {
    case OutputFormat::WebmVp8:
        return "libvpx";
    case OutputFormat::Gif:
        return "gif";
    case OutputFormat::Apng:
        return "apng";
    case OutputFormat::Mp4H264:
        return "h264_mf"; // Media Foundation: hardware when there is one, never GPL (spec §3.1, §10.4)
    case OutputFormat::MovProRes:
        return "prores_ks";
    case OutputFormat::WebmVp9:
        break;
    }
    return "libvpx-vp9";
}

const char* fallbackEncoderName(const EncodeSettings& s) {
    return s.format == OutputFormat::Mp4H264 ? "libopenh264" : nullptr; // BSD; when Media Foundation cannot open
}

const char* containerName(OutputFormat f) {
    switch (f) {
    case OutputFormat::Gif:
        return "gif";
    case OutputFormat::Apng:
        return "apng";
    case OutputFormat::Mp4H264:
        return "mp4";
    case OutputFormat::MovProRes:
        return "mov";
    case OutputFormat::WebmVp9:
    case OutputFormat::WebmVp8:
        break;
    }
    return "webm";
}

AVPixelFormat pixelFormat(const EncodeSettings& s) {
    switch (s.format) {
    case OutputFormat::Gif:
        return AV_PIX_FMT_PAL8;
    case OutputFormat::Apng:
        return s.alpha ? AV_PIX_FMT_RGBA : AV_PIX_FMT_RGB24;
    case OutputFormat::Mp4H264:
        return AV_PIX_FMT_YUV420P;
    case OutputFormat::MovProRes:
        return s.alpha ? AV_PIX_FMT_YUVA444P10LE : AV_PIX_FMT_YUV444P10LE;
    case OutputFormat::WebmVp9:
    case OutputFormat::WebmVp8:
        break;
    }
    return s.alpha ? AV_PIX_FMT_YUVA420P : AV_PIX_FMT_YUV420P;
}

int tileColumns(int width) {
    return std::max(0, int(std::floor(std::log2(std::max(1, width) / 256.0))));
}

void configure(AVCodecContext* c, const EncodeSettings& s) {
    const bool vp8 = s.format == OutputFormat::WebmVp8;
    const QualitySettings q = s.quality.resolved(vp8);
    c->width = s.size.width();
    c->height = s.size.height();
    c->time_base = AVRational{s.fps.den, s.fps.num}; // pts = k
    c->framerate = AVRational{s.fps.num, s.fps.den};
    c->pix_fmt = pixelFormat(s);
    if (s.format == OutputFormat::Gif || s.format == OutputFormat::Apng) return; // RGB: no matrix, no rate
    c->gop_size = std::clamp(s.frames, 1, 9999); // the whole loop one GOP: OBS decodes from its start each time
    // VP9, H.264 and ProRes carry BT.709; VP8 cannot, and decoders read it as BT.601 — so VP8 is made in BT.601.
    c->color_primaries = vp8 ? AVCOL_PRI_SMPTE170M : AVCOL_PRI_BT709;
    c->color_trc = vp8 ? AVCOL_TRC_SMPTE170M : AVCOL_TRC_BT709;
    c->colorspace = vp8 ? AVCOL_SPC_SMPTE170M : AVCOL_SPC_BT709;
    c->color_range = AVCOL_RANGE_MPEG;
    if (s.format == OutputFormat::MovProRes) return; // intra only, its profile sets the rate
    if (s.format == OutputFormat::Mp4H264) {
        // A bitrate from the preset, as VP8's (Low 2, Medium 6, High 12 Mbit/s; Custom: Max bitrate).
        c->bit_rate = std::int64_t(s.bitrateKbps > 0 ? s.bitrateKbps : s.quality.vp8Bitrate()) * 1000;
        c->max_b_frames = 0;
        return;
    }
    c->thread_count = s.threads > 0 ? s.threads : QThread::idealThreadCount();
    if (s.pass == 1) c->flags |= AV_CODEC_FLAG_PASS1;
    if (s.pass == 2) c->flags |= AV_CODEC_FLAG_PASS2;
    if (s.bitrateKbps > 0) {
        c->bit_rate = std::int64_t(s.bitrateKbps) * 1000; // Target size: an average bitrate (VBR)
    } else if (vp8) {
        c->bit_rate = std::int64_t(q.vp8Bitrate()) * 1000; // VP8: crf within a bitrate
    } else {
        c->bit_rate = 0; // VP9: constant quality
        if (q.maxBitrate > 0) c->rc_max_rate = std::int64_t(q.maxBitrate) * 1000;
    }
}

AVDictionary* encoderOptions(const EncodeSettings& s) {
    AVDictionary* o = nullptr;
    if (s.format == OutputFormat::MovProRes) {
        av_dict_set(&o, "profile", "4444", 0);
        return o;
    }
    if (!isVpx(s.format)) return o;
    const bool vp8 = s.format == OutputFormat::WebmVp8;
    const QualitySettings q = s.quality.resolved(vp8);
    if (s.bitrateKbps <= 0) av_dict_set_int(&o, "crf", q.crf, 0);
    av_dict_set(&o, "deadline", "good", 0);
    av_dict_set_int(&o, "cpu-used", q.speed, 0);
    if (vp8) {
        av_dict_set(&o, "auto-alt-ref", "0", 0); // required for VP8's alpha
    } else {
        av_dict_set(&o, "row-mt", "1", 0);
        av_dict_set_int(&o, "tile-columns", tileColumns(s.size.width()), 0);
        av_dict_set(&o, "auto-alt-ref", "1", 0);
        av_dict_set_int(&o, "lag-in-frames", std::clamp(s.lagInFrames, 0, 25), 0);
    }
    return o;
}

AVDictionary* muxerOptions(OutputFormat f) {
    AVDictionary* o = nullptr;
    if (f == OutputFormat::Gif) av_dict_set(&o, "loop", "0", 0);   // for ever
    if (f == OutputFormat::Apng) av_dict_set(&o, "plays", "0", 0); // for ever
    if (f == OutputFormat::Mp4H264) av_dict_set(&o, "movflags", "+faststart", 0);
    return o;
}

} // namespace avloop::media

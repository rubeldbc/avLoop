#include "media/EncoderOptions.h"

#include <QThread>

#include <algorithm>
#include <cmath>

namespace avloop::media {

const char* encoderName(const EncodeSettings& s) {
    return s.format == OutputFormat::WebmVp8 ? "libvpx" : "libvpx-vp9";
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
    c->gop_size = std::clamp(s.frames, 1, 9999); // the whole loop one GOP: OBS decodes from its start each time
    c->pix_fmt = s.alpha ? AV_PIX_FMT_YUVA420P : AV_PIX_FMT_YUV420P;
    // VP9 carries BT.709 in its bitstream; VP8 cannot, and decoders read it as BT.601 — so VP8 is made in BT.601.
    c->color_primaries = vp8 ? AVCOL_PRI_SMPTE170M : AVCOL_PRI_BT709;
    c->color_trc = vp8 ? AVCOL_TRC_SMPTE170M : AVCOL_TRC_BT709;
    c->colorspace = vp8 ? AVCOL_SPC_SMPTE170M : AVCOL_SPC_BT709;
    c->color_range = AVCOL_RANGE_MPEG;
    c->thread_count = s.threads > 0 ? s.threads : QThread::idealThreadCount();
    if (vp8) {
        c->bit_rate = std::int64_t(q.vp8Bitrate()) * 1000; // VP8: crf within a bitrate
    } else {
        c->bit_rate = 0; // VP9: constant quality
        if (q.maxBitrate > 0) c->rc_max_rate = std::int64_t(q.maxBitrate) * 1000;
    }
}

AVDictionary* encoderOptions(const EncodeSettings& s) {
    const bool vp8 = s.format == OutputFormat::WebmVp8;
    const QualitySettings q = s.quality.resolved(vp8);
    AVDictionary* o = nullptr;
    av_dict_set_int(&o, "crf", q.crf, 0);
    av_dict_set(&o, "deadline", "good", 0);
    av_dict_set_int(&o, "cpu-used", q.speed, 0);
    if (vp8) {
        av_dict_set(&o, "auto-alt-ref", "0", 0); // required for VP8's alpha
    } else {
        av_dict_set(&o, "row-mt", "1", 0);
        av_dict_set_int(&o, "tile-columns", tileColumns(s.size.width()), 0);
        av_dict_set(&o, "auto-alt-ref", "1", 0);
        av_dict_set(&o, "lag-in-frames", "25", 0);
    }
    return o;
}

} // namespace avloop::media

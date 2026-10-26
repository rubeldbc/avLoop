#include "media/SourceNormalizer.h"

extern "C" {
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/pixdesc.h>
}

#include <QStringList>

#include <utility>

namespace avloop::media {

namespace {

// Premultiplies an RGBA8 picture in place (spec §3.4: frames are drawn premultiplied).
void premultiply(AVFrame* f) {
    for (int y = 0; y < f->height; ++y) {
        std::uint8_t* p = f->data[0] + std::ptrdiff_t(y) * f->linesize[0];
        for (int x = 0; x < f->width; ++x, p += 4) {
            const unsigned a = p[3];
            if (a == 255) continue;
            p[0] = std::uint8_t((p[0] * a + 127) / 255);
            p[1] = std::uint8_t((p[1] * a + 127) / 255);
            p[2] = std::uint8_t((p[2] * a + 127) / 255);
        }
    }
}

} // namespace

SourceNormalizer::SourceNormalizer(const MediaInfo& info) : info_(info), size_(info.width, info.height) {}

void SourceNormalizer::setOutputSize(const QSize& size) {
    const QSize even(qMax(2, size.width() + (size.width() & 1)), qMax(2, size.height() + (size.height() & 1)));
    if (even == size_) return;
    size_ = even;
    graph_.reset();
}

QString SourceNormalizer::chain(int inFormat) const {
    QStringList f;
    // A hardware decoder's frames come down as NV12 / P010: made planar first, they take the very path a software
    // decoder's frames take (without accurate_rnd swscale converts NV12 by a coarser chroma path — 180 apart at
    // colour edges).
    if (inFormat == AV_PIX_FMT_NV12) f << QStringLiteral("format=yuv420p");
    if (inFormat == AV_PIX_FMT_P010LE) f << QStringLiteral("format=yuv420p10le");
    if (info_.interlaced) f << QStringLiteral("bwdif=mode=send_frame:parity=auto:deint=interlaced");
    if (info_.hdr) {
        f << QStringLiteral("zscale=t=linear:npl=100") << QStringLiteral("format=gbrpf32le")
          << QStringLiteral("zscale=p=bt709") << QStringLiteral("tonemap=tonemap=hable:desat=0")
          << QStringLiteral("zscale=t=bt709:m=bt709:r=tv") << QStringLiteral("format=yuv420p");
    }
    int w = size_.width();
    int h = size_.height();
    if (info_.rotation == 90 || info_.rotation == 270) std::swap(w, h);
    // No accurate_rnd: it turns off swscale's SIMD paths — 10 ms a 1080p frame against 0.7 ms, too slow for 60 fps
    // playback — for rounding within 1/255; full_chroma_int stays (sharp chroma, and cheap).
    f << QStringLiteral("scale=%1:%2:flags=bicubic+full_chroma_int:in_color_matrix=auto:"
                        "in_range=auto:out_range=full")
             .arg(w)
             .arg(h);
    if (info_.rotation == 90) f << QStringLiteral("transpose=clock");
    if (info_.rotation == 270) f << QStringLiteral("transpose=cclock");
    if (info_.rotation == 180) f << QStringLiteral("hflip") << QStringLiteral("vflip");
    if (info_.flipH) f << QStringLiteral("hflip");
    if (info_.flipV) f << QStringLiteral("vflip");
    f << QStringLiteral("format=rgba");
    return f.join(QLatin1Char(','));
}

bool SourceNormalizer::build(const AVFrame* sample, QString* error) {
    graph_.reset(avfilter_graph_alloc());
    src_ = nullptr;
    sink_ = nullptr;
    if (!graph_) return false;
    // One thread: the conversions take a millisecond, and every thread a graph starts costs memory for good in a
    // process the GPU driver watches (its arena grows with each; spec §3.7.5).
    graph_->nb_threads = 1;
    const QByteArray args = QStringLiteral("video_size=%1x%2:pix_fmt=%3:time_base=%4/%5:pixel_aspect=1/1")
                                .arg(sample->width)
                                .arg(sample->height)
                                .arg(sample->format)
                                .arg(info_.timeBase.num)
                                .arg(info_.timeBase.den)
                                .toLatin1();
    int r = avfilter_graph_create_filter(&src_, avfilter_get_by_name("buffer"), "in", args.constData(), nullptr,
                                         graph_.get());
    if (r >= 0) {
        // The frames' colour space and range belong to the link from the start (FFmpeg 7.1 negotiates them): zscale
        // finds no path from an unknown space, and a change on the fly upsets the filters.
        AVBufferSrcParameters* par = av_buffersrc_parameters_alloc();
        if (par) {
            par->format = sample->format;
            par->width = sample->width;
            par->height = sample->height;
            par->time_base = AVRational{info_.timeBase.num, info_.timeBase.den};
            par->sample_aspect_ratio = AVRational{1, 1};
            par->color_space = sample->colorspace;
            par->color_range = sample->color_range;
            r = av_buffersrc_parameters_set(src_, par);
            av_free(par);
        }
    }
    if (r >= 0)
        r = avfilter_graph_create_filter(&sink_, avfilter_get_by_name("buffersink"), "out", nullptr, nullptr,
                                         graph_.get());
    AVFilterInOut* outputs = avfilter_inout_alloc();
    AVFilterInOut* inputs = avfilter_inout_alloc();
    if (r >= 0 && outputs && inputs) {
        outputs->name = av_strdup("in");
        outputs->filter_ctx = src_;
        inputs->name = av_strdup("out");
        inputs->filter_ctx = sink_;
        const QByteArray spec = chain(sample->format).toLatin1();
        r = avfilter_graph_parse_ptr(graph_.get(), spec.constData(), &inputs, &outputs, nullptr);
        if (r >= 0) r = avfilter_graph_config(graph_.get(), nullptr);
    } else if (r >= 0) {
        r = AVERROR(ENOMEM);
    }
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    if (r < 0) {
        if (error) *error = QStringLiteral("filter graph: %1").arg(avError(r));
        graph_.reset();
        return false;
    }
    inWidth_ = sample->width;
    inHeight_ = sample->height;
    inFormat_ = sample->format;
    return true;
}

bool SourceNormalizer::push(const AVFrame* frame, QString* error) {
    const int r = av_buffersrc_add_frame_flags(src_, const_cast<AVFrame*>(frame), AV_BUFFERSRC_FLAG_KEEP_REF);
    if (r < 0 && error) *error = QStringLiteral("filter input: %1").arg(avError(r));
    return r >= 0;
}

void SourceNormalizer::wrap(AVFrame* filtered, std::int64_t pts, Frame& out) const {
    AVFrame* keep = av_frame_alloc();
    av_frame_move_ref(keep, filtered);
    if (info_.hasAlpha && av_frame_make_writable(keep) >= 0) premultiply(keep);
    out.width = keep->width;
    out.height = keep->height;
    out.stride = keep->linesize[0];
    out.pts = pts;
    out.data = std::shared_ptr<const std::uint8_t>(keep->data[0], [keep](const std::uint8_t*) mutable {
        av_frame_free(&keep); // the pixels go back to libavfilter's pool
    });
}

bool SourceNormalizer::convert(const AVFrame* prev, const AVFrame* cur, const AVFrame* next, Frame& out,
                               QString* error) {
    if (!cur) return false;
    const bool fresh = !graph_ || cur->width != inWidth_ || cur->height != inHeight_ || cur->format != inFormat_ ||
                       info_.interlaced; // bwdif keeps state: an interlaced source gets a graph a frame
    if (fresh && !build(cur, error)) return false;
    FramePtr filtered = makeFrame();
    if (!info_.interlaced) {
        if (!push(cur, error)) return false;
        const int r = av_buffersink_get_frame(sink_, filtered.get());
        if (r < 0) {
            if (error) *error = QStringLiteral("filter output: %1").arg(avError(r));
            return false;
        }
        wrap(filtered.get(), cur->pts, out);
        return true;
    }
    // Interlaced: the field neighbours in, the end of stream, then the frame whose time is cur's.
    if (prev && prev->pts < cur->pts && !push(prev, error)) return false;
    if (!push(cur, error)) return false;
    if (next && next->pts > cur->pts && !push(next, error)) return false;
    av_buffersrc_add_frame_flags(src_, nullptr, 0);
    bool found = false;
    // bwdif halves the time base (for its field mode): its pts are compared in the source's time base.
    const AVRational sinkTb = av_buffersink_get_time_base(sink_);
    const AVRational srcTb{info_.timeBase.num, info_.timeBase.den};
    while (av_buffersink_get_frame(sink_, filtered.get()) >= 0) {
        if (!found && av_rescale_q(filtered->pts, sinkTb, srcTb) == cur->pts) {
            wrap(filtered.get(), cur->pts, out);
            found = true;
        }
        av_frame_unref(filtered.get());
    }
    graph_.reset();
    if (!found && error) *error = QStringLiteral("deinterlace: no field pair for the frame");
    return found;
}

} // namespace avloop::media

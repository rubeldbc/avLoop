#include "media/GifPalette.h"

extern "C" {
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/imgutils.h>
}

#include <cstring>

namespace avloop::media {

namespace {

constexpr int kPaletteSide = 16;
constexpr int kPaletteBytes = kPaletteSide * kPaletteSide * 4;

bool fail(QString* error, const QString& what, int code = 0) {
    if (error) *error = code ? QStringLiteral("%1: %2").arg(what, avError(code)) : what;
    return false;
}

AVFilterContext* source(AVFilterGraph* graph, const char* name, QSize size, AVPixelFormat format) {
    AVFilterContext* f = nullptr;
    const QByteArray args = QStringLiteral("video_size=%1x%2:pix_fmt=%3:time_base=1/1000:pixel_aspect=1/1")
                                .arg(size.width())
                                .arg(size.height())
                                .arg(int(format))
                                .toLatin1();
    return avfilter_graph_create_filter(&f, avfilter_get_by_name("buffer"), name, args.constData(), nullptr, graph) >= 0
               ? f
               : nullptr;
}

AVFilterContext* filter(AVFilterGraph* graph, const char* kind, const char* args) {
    AVFilterContext* f = nullptr;
    return avfilter_graph_create_filter(&f, avfilter_get_by_name(kind), kind, args, nullptr, graph) >= 0 ? f : nullptr;
}

// A frame wrapping straight RGBA (copied: the graph keeps its own reference).
FramePtr rgbaFrame(QSize size, const std::uint8_t* rgba, int stride, std::int64_t pts) {
    FramePtr f = makeFrame();
    f->format = AV_PIX_FMT_RGBA;
    f->width = size.width();
    f->height = size.height();
    if (av_frame_get_buffer(f.get(), 0) < 0) return {};
    for (int y = 0; y < size.height(); ++y)
        std::memcpy(f->data[0] + std::ptrdiff_t(y) * f->linesize[0], rgba + std::ptrdiff_t(y) * stride,
                    std::size_t(size.width()) * 4);
    f->pts = pts;
    return f;
}

} // namespace

bool GifPaletteMaker::open(QSize size, QString* error) {
    size_ = size;
    pts_ = 0;
    graph_.reset(avfilter_graph_alloc());
    if (!graph_) return fail(error, QStringLiteral("no memory for the palette"));
    graph_->nb_threads = 1;
    src_ = source(graph_.get(), "in", size, AV_PIX_FMT_RGBA);
    AVFilterContext* make = filter(graph_.get(), "palettegen", "max_colors=256:reserve_transparent=1:stats_mode=diff");
    sink_ = filter(graph_.get(), "buffersink", nullptr);
    if (!src_ || !make || !sink_) return fail(error, QStringLiteral("the palette's filters"));
    if (avfilter_link(src_, 0, make, 0) < 0 || avfilter_link(make, 0, sink_, 0) < 0)
        return fail(error, QStringLiteral("the palette's links"));
    const int r = avfilter_graph_config(graph_.get(), nullptr);
    return r >= 0 || fail(error, QStringLiteral("the palette's graph"), r);
}

bool GifPaletteMaker::add(const std::uint8_t* rgba, int stride, QString* error) {
    FramePtr f = rgbaFrame(size_, rgba, stride, pts_);
    pts_ += 40;
    if (!f) return fail(error, QStringLiteral("no memory for a frame"));
    const int r = av_buffersrc_add_frame_flags(src_, f.get(), AV_BUFFERSRC_FLAG_KEEP_REF);
    return r >= 0 || fail(error, QStringLiteral("palette"), r);
}

QByteArray GifPaletteMaker::finish(QString* error) {
    av_buffersrc_add_frame(src_, nullptr); // the end: palettegen gives its palette now
    FramePtr palette = makeFrame();
    const int r = av_buffersink_get_frame(sink_, palette.get());
    graph_.reset();
    if (r < 0 || palette->width != kPaletteSide || palette->height != kPaletteSide) {
        fail(error, QStringLiteral("no palette"), r < 0 ? r : 0);
        return {};
    }
    QByteArray bytes(kPaletteBytes, '\0');
    for (int y = 0; y < kPaletteSide; ++y)
        std::memcpy(bytes.data() + y * kPaletteSide * 4, palette->data[0] + std::ptrdiff_t(y) * palette->linesize[0],
                    kPaletteSide * 4);
    return bytes;
}

bool GifQuantizer::open(QSize size, const QByteArray& palette, QString* error) {
    if (palette.size() != kPaletteBytes) return fail(error, QStringLiteral("no palette for the GIF"));
    size_ = size;
    pts_ = 0;
    graph_.reset(avfilter_graph_alloc());
    if (!graph_) return fail(error, QStringLiteral("no memory for the GIF's colours"));
    graph_->nb_threads = 1;
    src_ = source(graph_.get(), "in", size, AV_PIX_FMT_RGBA);
    AVFilterContext* pal = source(graph_.get(), "palette", QSize(kPaletteSide, kPaletteSide), AV_PIX_FMT_RGB32);
    AVFilterContext* use = filter(graph_.get(), "paletteuse", "dither=bayer:bayer_scale=3:alpha_threshold=128");
    sink_ = filter(graph_.get(), "buffersink", nullptr);
    if (!src_ || !pal || !use || !sink_) return fail(error, QStringLiteral("the GIF's filters"));
    if (avfilter_link(src_, 0, use, 0) < 0 || avfilter_link(pal, 0, use, 1) < 0 || avfilter_link(use, 0, sink_, 0) < 0)
        return fail(error, QStringLiteral("the GIF's links"));
    int r = avfilter_graph_config(graph_.get(), nullptr);
    if (r < 0) return fail(error, QStringLiteral("the GIF's graph"), r);
    // The palette once, then its end: paletteuse keeps it for every frame.
    FramePtr p = makeFrame();
    p->format = AV_PIX_FMT_RGB32;
    p->width = p->height = kPaletteSide;
    if (av_frame_get_buffer(p.get(), 0) < 0) return fail(error, QStringLiteral("no memory for the palette"));
    for (int y = 0; y < kPaletteSide; ++y)
        std::memcpy(p->data[0] + std::ptrdiff_t(y) * p->linesize[0], palette.constData() + y * kPaletteSide * 4,
                    kPaletteSide * 4);
    p->pts = 0;
    r = av_buffersrc_add_frame(pal, p.get());
    if (r >= 0) r = av_buffersrc_add_frame(pal, nullptr);
    return r >= 0 || fail(error, QStringLiteral("the palette"), r);
}

bool GifQuantizer::quantize(const std::uint8_t* rgba, int stride, AVFrame* out, QString* error) {
    FramePtr f = rgbaFrame(size_, rgba, stride, pts_);
    pts_ += 40;
    if (!f) return fail(error, QStringLiteral("no memory for a frame"));
    int r = av_buffersrc_add_frame(src_, f.get());
    if (r < 0) return fail(error, QStringLiteral("GIF colours"), r);
    av_frame_unref(out);
    r = av_buffersink_get_frame(sink_, out);
    return r >= 0 || fail(error, QStringLiteral("GIF colours"), r);
}

QByteArray GifQuantizer::paletteOf(QSize size, const std::uint8_t* rgba, int stride) {
    GifPaletteMaker maker;
    if (!maker.open(size) || !maker.add(rgba, stride)) return {};
    return maker.finish();
}

} // namespace avloop::media

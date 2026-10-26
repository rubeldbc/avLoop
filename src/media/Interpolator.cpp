#include "media/Interpolator.h"

#include "media/Decoder.h"
#include "media/DiskCache.h"
#include "media/FfmpegPtr.h"

extern "C" {
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/imgutils.h>
}

#include <QCryptographicHash>

#include <cmath>
#include <cstring>

namespace avloop::media {

namespace {

constexpr AVRational kMicro{1, 1000000}; // the frames go in with source time in µs

// The graph: colour through minterpolate in YUV 4:4:4, alpha (when there is any) on a branch of its own.
QByteArray chain(const Interpolator::Job& job) {
    const QString mi =
        QStringLiteral("minterpolate=fps=%1/%2:mi_mode=mci:mc_mode=aobmc:vsbmc=1").arg(job.fps.num).arg(job.fps.den);
    const QString stretch = QStringLiteral("setpts=PTS/%1").arg(job.speed, 0, 'g', 6);
    if (!job.info.hasAlpha) return QStringLiteral("%1,format=yuv444p,%2,format=rgba").arg(stretch, mi).toLatin1();
    return QStringLiteral("%1,split[c][a];[c]format=yuv444p,%2,format=rgba[cc];[a]alphaextract,%2[aa];"
                          "[cc][aa]alphamerge")
        .arg(stretch, mi)
        .toLatin1();
}

// A decoded frame as an AVFrame the graph can take (a copy: the decoder's frame goes back to its pool).
FramePtr toAv(const Frame& f) {
    FramePtr av = makeFrame();
    av->format = AV_PIX_FMT_RGBA;
    av->width = f.width;
    av->height = f.height;
    if (av_frame_get_buffer(av.get(), 0) < 0) return {};
    for (int y = 0; y < f.height; ++y)
        std::memcpy(av->data[0] + std::ptrdiff_t(y) * av->linesize[0], f.data.get() + std::ptrdiff_t(y) * f.stride,
                    std::size_t(f.width) * 4);
    av->pts = std::llround(f.seconds * 1e6);
    return av;
}

Frame fromAv(AVFrame* av) {
    AVFrame* keep = av_frame_alloc();
    av_frame_move_ref(keep, av);
    Frame f;
    f.width = keep->width;
    f.height = keep->height;
    f.stride = keep->linesize[0];
    f.data = std::shared_ptr<const std::uint8_t>(keep->data[0],
                                                 [keep](const std::uint8_t*) mutable { av_frame_free(&keep); });
    return f;
}

} // namespace

Interpolator::Interpolator(DiskCache* cache) : cache_(cache) {}

QString Interpolator::key(const Job& job, int i) {
    const QByteArray what = QStringLiteral("%1|%2|%3|%4|%5/%6|%7x%8")
                                .arg(job.info.path)
                                .arg(job.tIn, 0, 'f', 6)
                                .arg(job.segment)
                                .arg(job.speed, 0, 'f', 4)
                                .arg(job.fps.num)
                                .arg(job.fps.den)
                                .arg(job.size.width())
                                .arg(job.size.height())
                                .toUtf8();
    const QString hash = QString::fromLatin1(QCryptographicHash::hash(what, QCryptographicHash::Sha1).toHex().left(16));
    return QStringLiteral("mi-%1-%2").arg(hash).arg(i);
}

Frame Interpolator::frame(const Job& job, int i) const {
    return cache_ ? cache_->get(key(job, i)) : Frame{};
}

bool Interpolator::run(const Job& job, const Progress& progress, QString* error) {
    auto fail = [error](const QString& why) {
        if (error) *error = why;
        return false;
    };
    if (!cache_ || job.segment <= 0 || !job.fps.isValid()) return fail(QStringLiteral("nothing to interpolate"));
    Decoder::Options options;
    options.workingSize = job.size;
    Decoder decoder(job.info, options);
    QString why;
    if (!decoder.open(&why)) return fail(why);

    // The source from a frame before τ(0) to a few frames after τ(D − 1): minterpolate makes an output frame only
    // once it holds the input after it.
    const double step = 1.0 / job.fps.value() * job.speed;
    const double sourceFrame = 1.0 / std::max(1.0, job.info.fps());
    const double from = std::max(0.0, job.tIn - 2 * step);
    const double until = job.tIn + job.segment * step + 4 * sourceFrame;
    Frame f = decoder.frameAt(from);
    if (f.isNull()) return fail(QStringLiteral("no frame decodes at the segment's start"));

    FilterGraphPtr graph(avfilter_graph_alloc());
    AVFilterContext* src = nullptr;
    AVFilterContext* sink = nullptr;
    const QByteArray args = QStringLiteral("video_size=%1x%2:pix_fmt=%3:time_base=1/1000000:pixel_aspect=1/1")
                                .arg(f.width)
                                .arg(f.height)
                                .arg(int(AV_PIX_FMT_RGBA))
                                .toLatin1();
    int r = avfilter_graph_create_filter(&src, avfilter_get_by_name("buffer"), "in", args.constData(), nullptr,
                                         graph.get());
    if (r >= 0)
        r = avfilter_graph_create_filter(&sink, avfilter_get_by_name("buffersink"), "out", nullptr, nullptr,
                                         graph.get());
    AVFilterInOut* outputs = avfilter_inout_alloc();
    AVFilterInOut* inputs = avfilter_inout_alloc();
    if (r >= 0) {
        outputs->name = av_strdup("in");
        outputs->filter_ctx = src;
        inputs->name = av_strdup("out");
        inputs->filter_ctx = sink;
        r = avfilter_graph_parse_ptr(graph.get(), chain(job).constData(), &inputs, &outputs, nullptr);
        if (r >= 0) r = avfilter_graph_config(graph.get(), nullptr);
    }
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    if (r < 0) return fail(QStringLiteral("minterpolate: %1").arg(avError(r)));

    // Output pts n is n / fps of stretched time; segment frame i is the one at (tIn / s) × fps + i.
    const AVRational sinkTb = av_buffersink_get_time_base(sink);
    const double first = job.tIn / job.speed * job.fps.value();
    int done = 0;
    FramePtr out = makeFrame();
    auto drain = [&]() -> bool {
        while (av_buffersink_get_frame(sink, out.get()) >= 0) {
            const double n = double(out->pts) * sinkTb.num / sinkTb.den * job.fps.value();
            const int i = int(std::lround(n - first));
            if (i >= 0 && i < job.segment) {
                Frame result = fromAv(out.get());
                result.seconds = job.tIn + i * step;
                cache_->put(key(job, i), result);
                ++done;
                if (progress && !progress(done, job.segment)) return false;
            }
            av_frame_unref(out.get());
        }
        return true;
    };
    while (!f.isNull() && f.seconds <= until) {
        FramePtr in = toAv(f);
        if (!in || av_buffersrc_add_frame(src, in.get()) < 0) return fail(QStringLiteral("minterpolate: input"));
        if (!drain()) return fail(QStringLiteral("cancelled"));
        f = decoder.next();
    }
    av_buffersrc_add_frame(src, nullptr); // the end: what minterpolate holds comes out
    if (!drain()) return fail(QStringLiteral("cancelled"));
    if (done < job.segment) return fail(QStringLiteral("%1 of %2 frames made").arg(done).arg(job.segment));
    return true;
}

} // namespace avloop::media

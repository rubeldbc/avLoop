#include "render/FrameRenderer.h"

#include "core/AppSettings.h"
#include "media/Decoder.h"
#include "media/FrameIndex.h"
#include "render/GlRenderer.h"

#include <cmath>

namespace avloop::render {

namespace {
constexpr double kSameFrame = 1e-6; // seconds: two times this close name the same frame
}

FrameRenderer::FrameRenderer(const RenderJob& job, media::DiskCache* diskCache, bool preferGpu, GlRenderer* sharedGl)
    : job_(job), disk_(diskCache), preferGpu_(preferGpu) {
    if (!preferGpu_) return;
    if (sharedGl) {
        gl_ = sharedGl;
    } else {
        ownGl_ = std::make_unique<GlRenderer>();
        gl_ = ownGl_.get();
    }
}

FrameRenderer::~FrameRenderer() = default;

bool FrameRenderer::gpuSetting() {
    return appsettings::gpuRendering();
}

bool FrameRenderer::open(QString* error) {
    media::Decoder::Options options;
    options.workingSize = job_.workingSize;
    decoders_[0] = std::make_unique<media::Decoder>(job_.source, options);
    QString why;
    if (!decoders_[0]->open(&why)) {
        if (error) *error = why;
        return false;
    }
    // The source's frame times: a constant rate by its rate, a variable one by its packets (spec §8.3).
    const Rational rate = job_.source.avgFps.isValid() ? job_.source.avgFps : job_.source.realFps;
    if (job_.source.vfr) {
        const double reach = job_.range.length() + 1.0;
        times_ = FrameTimes::variable(media::frameTimes(job_.source, job_.range.in, job_.range.in + reach * 4));
    }
    if (!job_.source.vfr || times_.count() == 0) {
        const auto frames = job_.source.frameCount > 0 ? job_.source.frameCount
                                                       : std::int64_t(std::ceil(job_.source.duration * rate.value()));
        times_ = FrameTimes::constant(rate, std::max<std::int64_t>(1, frames));
    }
    if (!job_.crop.isEmpty()) mask_ = masks_.mask(job_.crop, job_.outputSize);
    backend_ = Backend::Cpu;
    if (gl_) {
        if (gl_->ready() || gl_->init(&gpuNote_))
            backend_ = Backend::Gpu;
        else
            gl_ = nullptr;
    }
    return true;
}

QString FrameRenderer::backendName() const {
    return backend_ == Backend::Gpu && gl_ ? QStringLiteral("GPU: %1").arg(gl_->name()) : QStringLiteral("CPU");
}

bool FrameRenderer::prepare(const std::function<bool(int, int)>& progress, QString* error) {
    if (job_.loop.method != LoopSettings::Method::Motion || motionReady_) return true;
    if (!disk_) {
        if (error) *error = QStringLiteral("Motion Interpolation needs the disk cache");
        return false;
    }
    motionJob_ = {job_.source, job_.range.in, job_.plan.segment, job_.loop.speed, job_.fps, job_.workingSize};
    motion_ = std::make_unique<media::Interpolator>(disk_);
    motionReady_ = motion_->run(motionJob_, progress, error);
    return motionReady_;
}

media::Frame FrameRenderer::sourceFrame(double seconds, int lane) {
    for (const Cached& c : recent_)
        if (!c.frame.isNull() && std::abs(c.seconds - seconds) < kSameFrame) return c.frame;
    if (!decoders_[lane]) {
        media::Decoder::Options options;
        options.workingSize = job_.workingSize;
        decoders_[lane] = std::make_unique<media::Decoder>(job_.source, options);
        if (!decoders_[lane]->open()) decoders_[lane].reset();
    }
    media::Decoder* d = decoders_[lane] ? decoders_[lane].get() : decoders_[0].get();
    // The frame starting at `seconds` (a hair later, so rounding to the stream's time base cannot fall short).
    const media::Frame f = d->frameAt(seconds + kSameFrame);
    recent_[recentNext_] = {seconds, f};
    recentNext_ = (recentNext_ + 1) % 8;
    return f;
}

bool FrameRenderer::compose(int k, Composite& c, QString* error) {
    const LoopPlan& plan = job_.plan;
    if (k < 0 || k >= plan.length) {
        if (error) *error = QStringLiteral("frame %1 is outside the loop's %2").arg(k).arg(plan.length);
        return false;
    }
    const Pick pick = loopmap::pick(job_.loop.mode, plan.segment, plan.fade, plan.turn, job_.loop.curve, k);
    const Placement place = Placement::of(job_);
    c.size = job_.outputSize;
    c.layers.clear();
    c.mask = mask_.isNull() ? nullptr : &mask_;
    c.alpha = job_.alpha;
    c.background = job_.background;
    // One segment frame's source frames, at its share of the blend; the loop's partner frame on the second lane.
    auto add = [&](int i, double weight, int lane) -> bool {
        if (weight <= 0) return true;
        if (job_.loop.method == LoopSettings::Method::Motion) {
            media::Frame f = motion_ ? motion_->frame(motionJob_, i) : media::Frame{};
            if (f.isNull()) return false;
            f.pts = -1 - i; // its own key in the GPU's texture pool
            c.layers.append({f, weight, place});
            return true;
        }
        const SourceMix mix = timemap::sources(job_.loop.method, job_.tau(i), times_);
        for (int m = 0; m < mix.count; ++m) {
            const media::Frame f = sourceFrame(mix.seconds[m], lane);
            if (f.isNull()) return false;
            c.layers.append({f, weight * mix.weight[m], place});
        }
        return true;
    };
    if (!add(pick.a, 1 - pick.w, 0) || !add(pick.b, pick.w, 1)) {
        if (error)
            *error = job_.loop.method == LoopSettings::Method::Motion
                         ? QStringLiteral("frame %1: the Motion Interpolation pre-pass has not run").arg(k)
                         : QStringLiteral("frame %1: a source frame did not decode").arg(k);
        return false;
    }
    return true;
}

bool FrameRenderer::render(int k, QImage& out, QString* error) {
    Composite c;
    if (!compose(k, c, error)) return false;
    if (out.size() != c.size || out.format() != QImage::Format_RGBA8888_Premultiplied)
        out = QImage(c.size, QImage::Format_RGBA8888_Premultiplied);
    if (backend_ == Backend::Gpu) {
        if (gl_->render(c, out.bits(), int(out.bytesPerLine()))) return true;
        // A lost or failing GPU: made again once, else the CPU from this frame on (spec §3.6).
        if (gl_->recover() && gl_->render(c, out.bits(), int(out.bytesPerLine()))) return true;
        gpuNote_ = QStringLiteral("the GPU failed at frame %1; the CPU renders from here").arg(k);
        backend_ = Backend::Cpu;
    }
    cpu_.render(c, out.bits(), int(out.bytesPerLine()));
    return true;
}

} // namespace avloop::render

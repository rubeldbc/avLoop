#pragma once

#include "core/TimeMap.h"
#include "media/Interpolator.h"
#include "render/CpuRenderer.h"
#include "render/EffectsPass.h"
#include "render/MaskBuilder.h"
#include "render/RenderJob.h"

#include <QImage>
#include <QString>

#include <functional>
#include <memory>

namespace avloop::media {
class Decoder;
class DiskCache;
} // namespace avloop::media

namespace avloop::render {

class GlRenderer;

// Output frame k of a loop (spec §3.4, AVADR-0001) — the one place output frames are made: the Preview, the export
// and avloop-cli all call it. LoopMap → TimeMap → normalized source frames (two decoders, one a lane, so a crossfade's
// two streams each walk forward; Motion's from the Interpolator) → the composite → the GPU back end, or the CPU one
// when GPU rendering is Off, OpenGL 3.3 is missing or the GPU fails (the frame is then made again on the CPU).
class FrameRenderer {
public:
    enum class Backend { Gpu, Cpu };

    // Construct on the GUI thread (the GPU back end's surface is made there); render on one thread. With `sharedGl`
    // (the RenderService's) that context is used — made on first use — instead of one of its own.
    FrameRenderer(const RenderJob& job, media::DiskCache* diskCache = nullptr, bool preferGpu = true,
                  GlRenderer* sharedGl = nullptr);
    ~FrameRenderer();

    // "GPU rendering: Auto / Off" (spec §3.6; Settings → Performance, key performance/gpuRendering).
    static bool gpuSetting();

    bool open(QString* error = nullptr);
    Backend backend() const { return backend_; }
    QString backendName() const; // "GPU: <GL_RENDERER>" or "CPU"
    // Why the GPU is not used (empty when it is, or was not asked for).
    QString gpuNote() const { return gpuNote_; }
    const RenderJob& job() const { return job_; }
    int length() const { return job_.plan.length; }

    // Motion Interpolation's pre-pass (R-30): runs it when the job needs it; `progress(done, total)` may cancel.
    bool prepare(const std::function<bool(int, int)>& progress = {}, QString* error = nullptr);
    // Output frame k (0 … L − 1), premultiplied RGBA8 of the output size.
    bool render(int k, QImage& out, QString* error = nullptr);
    // The composite of frame k without drawing it (the tests' unit cases, the parity walk).
    bool compose(int k, Composite& c, QString* error = nullptr);

private:
    media::Frame sourceFrame(double seconds, int lane);

    RenderJob job_;
    media::DiskCache* disk_;
    bool preferGpu_;
    Backend backend_ = Backend::Cpu;
    QString gpuNote_;
    std::unique_ptr<GlRenderer> ownGl_;
    GlRenderer* gl_ = nullptr; // ownGl_, or a shared one
    CpuRenderer cpu_;
    MaskBuilder masks_;
    EffectsPass effects_;
    double effectsScale_ = 1; // working px a source px
    QImage mask_;
    std::unique_ptr<media::Decoder> decoders_[2];
    FrameTimes times_;
    media::Interpolator::Job motionJob_;
    std::unique_ptr<media::Interpolator> motion_;
    bool motionReady_ = false;
    struct Cached {
        double seconds = -1;
        media::Frame frame;
    };
    Cached recent_[8]; // the last frames decoded (the clip effects applied): a blend's frames come up more than once
    int recentNext_ = 0;
};

} // namespace avloop::render

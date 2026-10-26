#include "export/ExportJob.h"

#include "export/FormatProfiles.h"
#include "export/OutputVerify.h"
#include "media/Encoder.h"
#include "render/FrameRenderer.h"
#include "render/RenderService.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QStorageInfo>
#include <QThread>

#include <windows.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <deque>
#include <mutex>

namespace avloop::exporting {

namespace {

constexpr int kAhead = 6;   // frames made ahead of the encoder (spec §3.7)
constexpr int kPerTurn = 2; // frames a turn on the render thread before the Preview's work gets its go
constexpr int kRenameTries = 6;

QString tr(const char* text) {
    return QCoreApplication::translate("ExportJob", text);
}

bool moveFile(const QString& from, const QString& to, bool replace) {
    const DWORD flags = MOVEFILE_WRITE_THROUGH | (replace ? MOVEFILE_REPLACE_EXISTING : 0);
    return MoveFileExW(reinterpret_cast<LPCWSTR>(QDir::toNativeSeparators(from).utf16()),
                       reinterpret_cast<LPCWSTR>(QDir::toNativeSeparators(to).utf16()), flags) != 0;
}

QString megabytes(double bytes) {
    return QString::number(bytes / 1048576.0, 'f', 1);
}

} // namespace

struct ExportShared {
    render::RenderService* service = nullptr;
    std::shared_ptr<render::FrameRenderer> renderer;
    std::mutex m;
    std::condition_variable cv;
    std::deque<QImage> queue;
    int next = 0;
    int total = 0;
    bool renderPending = false;
    bool opened = false;
    bool ended = false;
    QString renderError;
    QString gpuName; // "GPU: …", read on the render thread (its context is current only there)
    std::atomic<bool> stop{false};
};

namespace {

// On the render thread: up to kPerTurn frames into the queue, then the next turn queued behind other work.
void renderSome(const std::shared_ptr<ExportShared>& sh) {
    for (int n = 0; n < kPerTurn; ++n) {
        int k = 0;
        {
            std::lock_guard lock(sh->m);
            if (sh->stop || sh->next >= sh->total || int(sh->queue.size()) >= kAhead) {
                sh->renderPending = false;
                return;
            }
            k = sh->next++;
        }
        QImage image;
        QString error;
        bool ok = false;
        try {
            ok = sh->renderer->render(k, image, &error);
        } catch (const std::bad_alloc&) {
            error = tr("out of memory");
        }
        {
            std::lock_guard lock(sh->m);
            if (ok) {
                sh->queue.push_back(std::move(image));
            } else {
                sh->renderError = error.isEmpty() ? tr("frame %1 could not be made").arg(k) : error;
                sh->renderPending = false;
            }
        }
        sh->cv.notify_all();
        if (!ok) return;
    }
    sh->service->post([sh] { renderSome(sh); });
}

// On the export thread: rendering resumes once the queue has room.
void wake(const std::shared_ptr<ExportShared>& sh) {
    std::lock_guard lock(sh->m);
    if (sh->renderPending || sh->stop || sh->next >= sh->total || int(sh->queue.size()) >= kAhead) return;
    sh->renderPending = true;
    sh->service->post([sh] { renderSome(sh); });
}

void startPass(const std::shared_ptr<ExportShared>& sh) {
    {
        std::lock_guard lock(sh->m);
        sh->queue.clear();
        sh->next = 0;
    }
    wake(sh);
}

// The next frame in order; null when stopped or when the renderer failed.
QImage take(const std::shared_ptr<ExportShared>& sh) {
    std::unique_lock lock(sh->m);
    sh->cv.wait(lock, [&] { return sh->stop || !sh->queue.empty() || !sh->renderError.isEmpty(); });
    if (sh->stop || sh->queue.empty()) return {};
    QImage image = std::move(sh->queue.front());
    sh->queue.pop_front();
    return image;
}

} // namespace

ExportJob::ExportJob(const render::RenderJob& job, const QString& path, render::RenderService* service,
                     QObject* encodeContext, media::DiskCache* disk, QObject* parent)
    : QObject(parent), job_(job), path_(path), service_(service), encodeContext_(encodeContext), disk_(disk),
      sh_(std::make_shared<ExportShared>()), preferGpu_(render::FrameRenderer::gpuSetting()) {
    passes_ = twoPasses(job) ? 2 : 1;
    total_ = passes_ * job.plan.length;
}

ExportJob::~ExportJob() {
    cancel();
    std::unique_lock lock(sh_->m);
    sh_->cv.wait(lock, [this] { return sh_->ended || state_ == State::Idle; });
}

bool ExportJob::twoPasses(const render::RenderJob& job) {
    // GIF: its palette of every frame first (spec §10.4); WebM: Two-pass or Target size (§10.3).
    if (job.output.format == OutputFormat::Gif) return true;
    const bool vpx = job.output.format == OutputFormat::WebmVp9 || job.output.format == OutputFormat::WebmVp8;
    const QualitySettings& q = job.quality;
    return vpx && q.preset == QualitySettings::Preset::Custom && (q.twoPass || q.targetMB > 0);
}

int ExportJob::targetKbps(double targetMB, double seconds) {
    return std::max(1, int(std::lround(targetMB * 1048576.0 * 8.0 / std::max(0.001, seconds) * 0.9 / 1000.0)));
}

double ExportJob::etaSeconds() const {
    return fps_ > 0 ? (total_ - done_) / fps_ : -1;
}

void ExportJob::start() {
    if (state_ != State::Idle) return;
    state_ = State::Running;
    sh_->service = service_;
    sh_->total = job_.plan.length;
    sh_->renderer =
        std::make_shared<render::FrameRenderer>(job_, disk_, preferGpu_, preferGpu_ ? service_->gl() : nullptr);
    auto sh = sh_;
    service_->post([sh] {
        QString error;
        bool ok = false;
        try {
            ok = sh->renderer->open(&error) && sh->renderer->prepare([sh](int, int) { return !sh->stop; }, &error);
        } catch (const std::bad_alloc&) {
            error = tr("out of memory");
        }
        std::lock_guard lock(sh->m);
        sh->opened = true;
        if (ok) sh->gpuName = sh->renderer->backendName();
        if (!ok) sh->renderError = error.isEmpty() ? tr("the source could not be opened") : error;
        sh->cv.notify_all();
    });
    QMetaObject::invokeMethod(encodeContext_, [this] { run(); }, Qt::QueuedConnection);
}

void ExportJob::cancel() {
    sh_->stop = true;
    sh_->cv.notify_all();
}

void ExportJob::run() {
    const std::shared_ptr<ExportShared> sh = sh_;
    QElapsedTimer clock;
    clock.start();
    Result result;
    State state = State::Failed;
    const QString part = path_ + QStringLiteral(".part");
    const int L = job_.plan.length;
    int done = 0;
    int passes = passes_;
    try {
        {
            std::unique_lock lock(sh->m);
            sh->cv.wait(lock, [&] { return sh->opened || sh->stop; });
        }
        const QualitySettings& q = job_.quality;
        const bool vpx = job_.output.format == OutputFormat::WebmVp9 || job_.output.format == OutputFormat::WebmVp8;
        const bool target = vpx && q.preset == QualitySettings::Preset::Custom && q.targetMB > 0;
        media::EncodeSettings base;
        base.format = job_.output.format;
        base.size = job_.outputSize;
        base.fps = job_.fps;
        base.frames = L;
        base.alpha = job_.alpha;
        base.quality = q;
        const double seconds = L / job_.fps.value();
        int kbps = target ? targetKbps(q.targetMB, seconds) : 0;
        QByteArray stats;
        QElapsedTimer report;
        report.start();
        int pass = 0;
        std::int64_t lostBytes = 0; // what a pass had written when the disk filled
        int lostFrames = 0;

        auto encodePass = [&](int which) {
            ++pass;
            media::EncodeSettings s = base;
            s.pass = which;
            s.passStats = stats;
            s.bitrateKbps = kbps;
            media::Encoder encoder(s);
            if (!encoder.open(which == 1 ? QString() : part, &result.error)) {
                result.outOfSpace = encoder.outOfSpace();
                return false;
            }
            startPass(sh);
            for (int k = 0; k < L; ++k) {
                const QImage image = take(sh);
                if (image.isNull()) {
                    encoder.abort();
                    std::lock_guard lock(sh->m);
                    if (!sh->stop) result.error = sh->renderError;
                    return false;
                }
                wake(sh);
                if (!encoder.write(image.constBits(), int(image.bytesPerLine()), &result.error)) {
                    result.outOfSpace = encoder.outOfSpace();
                    lostBytes = encoder.bytes();
                    lostFrames = k + 1;
                    encoder.abort();
                    return false;
                }
                ++done;
                if (report.elapsed() >= 100 || k == L - 1) {
                    report.restart();
                    const double fps = done / std::max(0.001, clock.elapsed() / 1000.0);
                    QMetaObject::invokeMethod(
                        this,
                        [this, done, pass, passes, fps] {
                            done_ = done;
                            pass_ = pass;
                            passes_ = passes;
                            total_ = passes * job_.plan.length;
                            fps_ = fps;
                            emit progressed();
                        },
                        Qt::QueuedConnection);
                }
            }
            if (!encoder.finish(&result.error)) {
                result.outOfSpace = encoder.outOfSpace();
                lostBytes = encoder.bytes();
                lostFrames = L;
                encoder.abort();
                return false;
            }
            if (which == 1) stats = encoder.stats();
            result.encoder = encoder.encoderUsed();
            return true;
        };

        bool ok = false;
        {
            std::lock_guard lock(sh->m);
            ok = !sh->stop && sh->renderError.isEmpty();
            if (!ok) result.error = sh->renderError;
        }
        if (ok) ok = passes == 2 ? encodePass(1) && encodePass(2) : encodePass(0);
        if (ok && target) {
            // More than 5 % off the target: once more at the bitrate that size asks for (the first pass reused).
            const double want = q.targetMB * 1048576.0;
            const double got = double(QFileInfo(part).size());
            if (got > 0 && std::abs(got - want) > 0.05 * want) {
                // Over: a little under the asked size; under: as asked (an easy loop may not use it all).
                kbps = std::max(1, int(std::lround(kbps * want / got * (got > want ? 0.97 : 1.0))));
                ++passes;
                ok = encodePass(2);
            }
        }
        // A GPU that failed on the way left the CPU making the rest (FrameRenderer's note says from where).
        result.renderer =
            sh->renderer->backend() == render::FrameRenderer::Backend::Gpu ? sh->gpuName : QStringLiteral("CPU");
        if (!sh->renderer->gpuNote().isEmpty()) result.renderer += QStringLiteral(" (%1)").arg(sh->renderer->gpuNote());

        if (sh->stop) {
            state = State::Cancelled;
        } else if (!ok) {
            if (result.outOfSpace) {
                // What the whole file needs, from what the pass had written.
                const double need = double(lostBytes) * L / std::max(1, lostFrames);
                const qint64 free = QStorageInfo(QFileInfo(path_).absolutePath()).bytesAvailable();
                result.error = tr("Not enough disk space: the export needs about %1 MB, %2 MB are free")
                                   .arg(megabytes(need), megabytes(double(free)));
            }
        } else {
            // The rename over the chosen name; a file OBS holds gets five more tries, then the name beside it.
            bool moved = false;
            for (int t = 0; t < kRenameTries && !moved && !sh->stop; ++t) {
                moved = moveFile(part, path_, true);
                for (int w = 0; !moved && t + 1 < kRenameTries && w < 10 && !sh->stop; ++w)
                    QThread::msleep(100);
            }
            result.path = path_;
            if (!moved && !sh->stop) {
                result.path = alternativePath(path_);
                result.movedAside = moved = moveFile(part, result.path, false);
            }
            if (sh->stop) {
                state = State::Cancelled;
            } else if (!moved) {
                result.error = tr("%1 could not be written (error %2)")
                                   .arg(QDir::toNativeSeparators(path_))
                                   .arg(qulonglong(GetLastError()));
            } else {
                const Verified v =
                    verifyOutput(result.path, Expected{job_.output.format, job_.outputSize, job_.fps, L, job_.alpha});
                result.bytes = v.bytes;
                result.verifyProblem = v.problem;
                state = State::Done;
            }
        }
    } catch (const std::bad_alloc&) {
        result.error = tr("Out of memory — the export stopped");
        state = State::Failed;
    }
    if (state != State::Done) QFile::remove(part);
    if (state == State::Failed && result.error.isEmpty()) result.error = tr("the export failed");
    result.seconds = clock.elapsed() / 1000.0;
    QMetaObject::invokeMethod(
        this,
        [this, state, result] {
            state_ = state;
            result_ = result;
            emit finished();
        },
        Qt::QueuedConnection);
    std::lock_guard lock(sh->m);
    sh->stop = true;
    sh->ended = true;
    sh->queue.clear();
    sh->service->post([sh] { sh->renderer.reset(); }); // its decoders let go on the thread that used them
    sh->service->trim();                               // and its textures on the GPU
    sh->cv.notify_all();
}

} // namespace avloop::exporting

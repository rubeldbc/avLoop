#pragma once

#include "render/RenderJob.h"

#include <QObject>
#include <QString>

#include <cstdint>
#include <memory>

namespace avloop::media {
class DiskCache;
}
namespace avloop::render {
class RenderService;
}

namespace avloop::exporting {

struct ExportShared;

// One export of a project snapshot (spec §10.1, §12.3). Its frames are made on the RenderService's thread with the
// shared GPU context, two a turn so the Preview's work goes between them, at most six ahead of the encoder; the encoder
// runs on the export thread and writes `<name>.part`, which is then renamed over `<name>` (MoveFileEx,
// REPLACE_EXISTING; five more tries a second apart, then `name (1).webm`) and checked. Cancel, an error or a full disk
// delete the .part. Two-pass and Target size take two passes — Target a third when the size came out more than 5 % off.
// A GPU that fails leaves the CPU to make the rest. Construct and drive it on the GUI thread.
class ExportJob : public QObject {
    Q_OBJECT
public:
    enum class State { Idle, Running, Done, Failed, Cancelled };
    struct Result {
        QString path;            // the file written: the chosen one, or its alternative when that stayed locked
        bool movedAside = false; // written as `name (1).webm`
        std::int64_t bytes = 0;
        QString error; // Failed: why
        bool outOfSpace = false;
        QString verifyProblem; // Done, but the check found this (spec §10.1 step 5)
        QString renderer;      // "GPU: …" or "CPU", with the GPU's note
        QString encoder;       // "libvpx-vp9", "h264_mf" or its fallback "libopenh264" …
        double seconds = 0;
    };

    // `encodeContext` lives on the export thread; `service` outlives the job.
    ExportJob(const render::RenderJob& job, const QString& path, render::RenderService* service, QObject* encodeContext,
              media::DiskCache* disk = nullptr, QObject* parent = nullptr);
    ~ExportJob() override; // cancels, and waits for the export thread to let go

    void start();
    void cancel();
    void setPreferGpu(bool gpu) { preferGpu_ = gpu; } // before start(); Settings → Performance by default

    State state() const { return state_; }
    const render::RenderJob& job() const { return job_; }
    const QString& path() const { return path_; }
    const Result& result() const { return result_; }
    int done() const { return done_; } // frames encoded over every pass
    int total() const { return total_; }
    int pass() const { return pass_; } // 1-based
    int passes() const { return passes_; }
    double fps() const { return fps_; }
    double etaSeconds() const;

    // Target size's first bitrate: target_bytes × 8 / duration × 0.9, kbit/s (spec §10.3).
    static int targetKbps(double targetMB, double seconds);
    // Whether the job takes two passes: GIF (its palette), WebM's Two-pass and Target size.
    static bool twoPasses(const render::RenderJob& job);

signals:
    void progressed();
    void finished();

private:
    void run(); // on the export thread

    render::RenderJob job_;
    QString path_;
    render::RenderService* service_;
    QObject* encodeContext_;
    media::DiskCache* disk_;
    std::shared_ptr<ExportShared> sh_;
    bool preferGpu_;
    State state_ = State::Idle;
    Result result_;
    int done_ = 0;
    int total_ = 0;
    int pass_ = 1;
    int passes_ = 1;
    double fps_ = 0;
};

} // namespace avloop::exporting

#pragma once

#include <QObject>

#include <functional>
#include <memory>

namespace avloop::media {
class WorkerThread;
}

namespace avloop::render {

class GlRenderer;

// Where the app makes output frames (AVADR-0001): one thread and one GPU context for the window's life, shared by the
// Preview's Live frames, its Encoded pass and — at AV-009 — the export. Opening a Preview starts no thread and makes no
// context (each costs the GPU driver memory for good; AV-007). Construct it on the GUI thread.
class RenderService : public QObject {
    Q_OBJECT
public:
    explicit RenderService(QObject* parent = nullptr);
    ~RenderService() override;

    // The GPU back end, its context made on the render thread at first use; null when GPU rendering is Off.
    GlRenderer* gl() const { return gl_.get(); }
    QObject* context() const { return context_; } // lives on the render thread: queue work on it
    // Runs `work` on the render thread, after what was queued before.
    void post(std::function<void()> work);
    // Runs `work` there and waits for it.
    void run(std::function<void()> work);
    // Queued on the render thread: the GPU's buffers of the jobs before it freed (an export, an estimate, a Preview
    // ended — spec §3.7.5); the next job makes what it needs again.
    void trim();

private:
    std::unique_ptr<media::WorkerThread> thread_;
    std::unique_ptr<GlRenderer> gl_;
    QObject* context_ = nullptr;
};

} // namespace avloop::render

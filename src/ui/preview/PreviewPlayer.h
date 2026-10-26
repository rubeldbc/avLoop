#pragma once

#include "render/RenderJob.h"

#include <QElapsedTimer>
#include <QImage>
#include <QObject>
#include <QTimer>
#include <QVector>

#include <atomic>
#include <functional>
#include <memory>

namespace avloop::media {
class DiskCache;
}
namespace avloop::render {
class FrameRenderer;
class RenderService;
} // namespace avloop::render

namespace avloop {

// The Preview's player (spec §9.2, §9.3): output frames 0 … L − 1, always looping. Live: frames made by the
// FrameRenderer on the render service's thread, kept in a cache of the preview's own (bounded in bytes); playback
// waits for a frame not yet made ("Caching…") rather than dropping it, then runs in real time. Encoded: the loop
// encoded with the current Output and Quality into a temporary WebM (PreviewEncode.cpp), then that file's frames, as
// libvpx decodes them, played the same way. Everything goes with the player.
class PreviewPlayer : public QObject {
    Q_OBJECT
public:
    enum class Mode { Live, Encoded };
    enum class Encode { None, Running, Done, OutOfDate, Failed };

    PreviewPlayer(render::RenderService* service, media::DiskCache* disk, std::int64_t cacheBytes,
                  QObject* parent = nullptr);
    ~PreviewPlayer() override;

    // A new job (the project changed): Live frames are made again; an Encoded result goes out of date.
    void setJob(const render::RenderJob& job);
    const render::RenderJob& job() const { return job_; }
    int length() const { return job_.plan.length; }
    int position() const { return shown_; }
    const QImage& frame() const { return frame_; }
    bool isCached(int k) const;
    int cachedCount() const;
    bool caching() const { return caching_; }

    void setMode(Mode mode);
    Mode mode() const { return mode_; }
    void play();
    void pause();
    void toggle();
    bool playing() const { return playing_; }
    void seek(int k);
    void step(int n);
    void setAroundSeam(bool on);
    bool aroundSeam() const { return aroundSeam_; }

    // Encoded (PreviewEncode.cpp): into `path`, in the background.
    void encode(const QString& path);
    Encode encodeState() const { return encodeState_; }
    QString encodedPath() const { return encodedPath_; }
    qint64 encodedBytes() const { return encodedBytes_; }
    int encodeDone() const { return encodeDone_; }
    QString error() const { return error_; }

signals:
    void frameChanged(int k);
    void stateChanged(); // playing, caching, the cache, the encode

private:
    // One mode's frames: a slot a frame, what is cached and its bytes, and how frame k is made (on the render thread).
    struct Track {
        QVector<QImage> frames;
        int cached = 0;
        std::int64_t bytes = 0;
        std::function<QImage(int)> make;
    };
    Track& track();
    void ask(); // keeps one frame request in flight
    void delivered(int generation, int k, QImage image);
    void tick();
    int order(int step) const; // the frame played `step` frames after the start of a pass
    int passLength() const;
    void show(int k);
    void store(Track& t, int k, QImage image);
    void useEncoded(); // PreviewEncode.cpp: the file's frames become a track

    render::RenderService* service_;
    media::DiskCache* disk_;
    std::int64_t cacheBytes_;
    render::RenderJob job_;
    std::shared_ptr<render::FrameRenderer> renderer_;
    std::shared_ptr<int> life_ = std::make_shared<int>(0); // the render thread's results find the player by it
    std::unique_ptr<Track> live_;
    std::unique_ptr<Track> encoded_;
    Mode mode_ = Mode::Live;
    int generation_ = 0;
    bool inFlight_ = false;
    QImage frame_;
    int shown_ = 0;
    bool playing_ = false;
    bool caching_ = false;
    bool aroundSeam_ = false;
    double step_ = 0; // the playback position within a pass, in frames
    QElapsedTimer clock_;
    QTimer timer_;
    Encode encodeState_ = Encode::None;
    QString encodedPath_;
    qint64 encodedBytes_ = 0;
    int encodeDone_ = 0;
    QString error_;
    std::shared_ptr<std::atomic<bool>> cancelEncode_;
};

} // namespace avloop

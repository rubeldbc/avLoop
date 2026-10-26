#pragma once

#include "media/Decoder.h"
#include "media/MediaInfo.h"

#include <QMutex>
#include <QObject>
#include <QThread>
#include <QWaitCondition>

#include <atomic>
#include <deque>
#include <memory>

namespace avloop::media {

class FrameCache;

// A decoder on its own thread (spec §3.5: decoding never happens on the UI thread). Requests are coalesced — only the
// newest waiting request is decoded — and answered by a signal; decoded frames go into the frame cache. Playback
// decodes ahead in display order into a ring of six frames the UI's clock takes from (spec §7.3).
class DecodeWorker : public QObject {
    Q_OBJECT
public:
    // Exact: the frame on screen at the time. Fast: the scrub's keyframe preview (spec §7.2).
    enum class Mode { Exact, Fast };
    static constexpr int kRing = 6;

    // On `thread` when given (a WorkerThread that outlives it), else on a thread of its own.
    DecodeWorker(const MediaInfo& info, const Decoder::Options& options, FrameCache* cache, QThread* thread = nullptr,
                 QObject* parent = nullptr);
    ~DecodeWorker() override;

    const MediaInfo& info() const { return info_; }
    quint64 sourceId() const { return sourceId_; }
    bool hardware() const { return hardware_; }

    // Opens the decoder on its thread; opened() follows.
    void start();
    // The frame at `seconds`; frameReady() follows (a newer request replaces one still waiting). Stops playback.
    void requestFrame(double seconds, Mode mode = Mode::Exact);

    // Playback: the frames from `from` in display order, ahead of the clock; past `until` they go on from `loopFrom`
    // (a new lap) when `loop`, else an end mark follows the last.
    void play(double from, double until, double loopFrom, bool loop);
    void stopPlayback();
    struct Due {
        Frame frame;        // the frame to show now, null when none is due
        int skipped = 0;    // frames passed over unseen (dropped)
        bool ended = false; // the source ran out: nothing follows
    };
    // What to show at the clock's source time `t` in lap `lap` (a lap lasts `lapLength` s): the oldest frame due,
    // so a late tick still shows every frame in turn; a frame more than `slack` s late is dropped when a newer one is
    // due behind it.
    Due takeDue(double t, int lap, double lapLength, double slack);

signals:
    void opened(bool ok, const QString& error, bool hardware, const QString& note);
    void frameReady(double requested, const avloop::media::Frame& frame);

private:
    struct Slot {
        Frame frame;
        int lap = 0;
        bool end = false; // the end mark: due when the clock reaches `at`
        double at = 0;
    };
    void serve();
    void produce(double from, double until, double loopFrom, bool loop);
    bool push(Slot slot); // waits for room; false when playback stopped

    MediaInfo info_;
    Decoder::Options options_;
    FrameCache* cache_;
    quint64 sourceId_;
    std::unique_ptr<QThread> ownThread_;
    QObject* inThread_ = nullptr; // lives on the worker's thread: the context the decoder's work is queued on
    std::unique_ptr<Decoder> decoder_;
    QMutex mutex_;
    double pending_ = 0;
    Mode pendingMode_ = Mode::Exact;
    bool hasPending_ = false;
    bool serving_ = false;
    std::atomic<bool> hardware_{false};

    QMutex ringMutex_;
    QWaitCondition ringRoom_;
    std::deque<Slot> ring_;
    std::atomic<bool> stopPlay_{false};
};

} // namespace avloop::media

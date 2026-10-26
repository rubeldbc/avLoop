#pragma once

#include "media/MediaInfo.h"

#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QTimer>

namespace avloop {

namespace media {
class DecodeWorker;
}

// The editor's playback clock (spec §7.3): a QElapsedTimer read every few milliseconds; the source time runs from
// where playback started (at Speed — 1 until AV-006); with loop on it cycles In → Out, otherwise it stops at the
// file's end. Frames are taken from the decoder's ring in order and never waited for: a tick that comes late shows the
// oldest frame due and the next tick the following one; a frame more than two frames late with a newer one due is
// dropped and counted, and the clock goes on.
class PlaybackClock : public QObject {
    Q_OBJECT
public:
    explicit PlaybackClock(QObject* parent = nullptr);

    void setWorker(media::DecodeWorker* worker);
    // From `from` to `fileEnd`, or round [loopIn, loopOut) when `loop` (a start outside the range starts at In).
    void start(double from, double loopIn, double loopOut, double fileEnd, double frameSeconds, bool loop,
               double speed = 1.0);
    void stop();
    bool running() const { return timer_.isActive(); }
    int dropped() const { return dropped_; }
    int shown() const { return shown_; }
    void resetCounts();
    double lastShown() const { return lastShown_; }

signals:
    void frameShown(const avloop::media::Frame& frame);
    void stopped(double lastShown);

private:
    void tick();

    QPointer<media::DecodeWorker> worker_;
    QTimer timer_;
    QElapsedTimer elapsed_;
    double base_ = 0; // the source time at elapsed 0 of this lap
    double loopIn_ = 0;
    double loopOut_ = 0;
    double end_ = 0;
    double slack_ = 0.1; // two output frames
    bool loop_ = false;
    double speed_ = 1.0;
    int lap_ = 0;
    int dropped_ = 0;
    int shown_ = 0;
    double lastShown_ = 0;
};

} // namespace avloop

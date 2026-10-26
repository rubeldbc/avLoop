#include "ui/timeline/PlaybackClock.h"

#include "media/DecodeWorker.h"

#include <cmath>

namespace avloop {

namespace {
constexpr int kTickMs = 4; // well under a 60 fps frame, so no frame falls due unseen while decoding keeps up
}

PlaybackClock::PlaybackClock(QObject* parent) : QObject(parent) {
    timer_.setTimerType(Qt::PreciseTimer);
    timer_.setInterval(kTickMs);
    connect(&timer_, &QTimer::timeout, this, &PlaybackClock::tick);
}

void PlaybackClock::setWorker(media::DecodeWorker* worker) {
    stop();
    worker_ = worker;
}

void PlaybackClock::start(double from, double loopIn, double loopOut, double fileEnd, double frameSeconds, bool loop,
                          double speed) {
    if (!worker_) return;
    loop_ = loop && loopOut > loopIn;
    loopIn_ = loopIn;
    loopOut_ = loopOut;
    end_ = fileEnd;
    slack_ = 2 * frameSeconds;
    speed_ = speed > 0 ? speed : 1.0;
    if (loop_ && (from < loopIn_ || from >= loopOut_ - 1e-9)) from = loopIn_;
    if (!loop_ && from >= end_ - 1e-9) from = 0; // at the end: from the start
    base_ = from;
    lap_ = 0;
    lastShown_ = from;
    worker_->play(from, loop_ ? loopOut_ : end_, loopIn_, loop_);
    elapsed_.start();
    timer_.start();
}

void PlaybackClock::stop() {
    if (!timer_.isActive()) return;
    timer_.stop();
    if (worker_) worker_->stopPlayback();
    emit stopped(lastShown_);
}

void PlaybackClock::resetCounts() {
    dropped_ = 0;
    shown_ = 0;
}

void PlaybackClock::tick() {
    if (!worker_) {
        stop();
        return;
    }
    double t = base_ + elapsed_.nsecsElapsed() / 1e9 * speed_;
    if (loop_ && t >= loopOut_) {
        // A new lap: the time past Out carries over from In.
        const double over = std::fmod(t - loopOut_, loopOut_ - loopIn_);
        ++lap_;
        base_ = loopIn_ + over;
        elapsed_.start();
        t = base_;
    }
    const media::DecodeWorker::Due due = worker_->takeDue(t, lap_, loop_ ? loopOut_ - loopIn_ : 0, slack_);
    dropped_ += due.skipped;
    if (!due.frame.isNull()) {
        ++shown_;
        lastShown_ = due.frame.seconds;
        emit frameShown(due.frame);
    }
    if (due.ended || (!loop_ && t >= end_)) stop();
}

} // namespace avloop

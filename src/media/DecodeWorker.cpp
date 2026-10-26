#include "media/DecodeWorker.h"

#include "media/FrameCache.h"

#include <QMetaObject>

namespace avloop::media {

DecodeWorker::DecodeWorker(const MediaInfo& info, const Decoder::Options& options, FrameCache* cache, QObject* parent)
    : QObject(parent), info_(info), options_(options), cache_(cache), sourceId_(FrameCache::sourceId(info.path)) {
    qRegisterMetaType<avloop::media::Frame>("avloop::media::Frame");
    thread_.setObjectName(QStringLiteral("avloop-decode"));
    inThread_.moveToThread(&thread_);
    thread_.start();
}

DecodeWorker::~DecodeWorker() {
    stopPlayback();
    // The decoder is destroyed on the thread that used it, then the thread stops.
    QMetaObject::invokeMethod(&inThread_, [this] { decoder_.reset(); }, Qt::BlockingQueuedConnection);
    thread_.quit();
    thread_.wait();
}

void DecodeWorker::start() {
    QMetaObject::invokeMethod(&inThread_, [this] {
        decoder_ = std::make_unique<Decoder>(info_, options_);
        QString error;
        const bool ok = decoder_->open(&error);
        hardware_ = ok && decoder_->hardwareActive();
        emit opened(ok, error, hardware_, ok ? decoder_->hardwareNote() : QString());
        if (!ok) decoder_.reset();
    });
}

void DecodeWorker::requestFrame(double seconds, Mode mode) {
    stopPlayback();
    QMutexLocker lock(&mutex_);
    pending_ = seconds;
    pendingMode_ = mode;
    hasPending_ = true;
    if (serving_) return;
    serving_ = true;
    QMetaObject::invokeMethod(&inThread_, [this] { serve(); });
}

void DecodeWorker::serve() {
    while (true) {
        double t = 0;
        Mode mode = Mode::Exact;
        {
            QMutexLocker lock(&mutex_);
            if (!hasPending_) {
                serving_ = false;
                return;
            }
            t = pending_;
            mode = pendingMode_;
            hasPending_ = false;
        }
        Frame frame;
        if (decoder_) frame = mode == Mode::Fast ? decoder_->keyframeAt(t) : decoder_->frameAt(t);
        if (decoder_) hardware_ = decoder_->hardwareActive();
        if (!frame.isNull() && cache_)
            cache_->insert(FrameCache::Key{sourceId_, frame.width, frame.height, frame.pts}, frame);
        emit frameReady(t, frame);
    }
}

void DecodeWorker::play(double from, double until, double loopFrom, bool loop) {
    stopPlayback();
    {
        QMutexLocker lock(&ringMutex_);
        ring_.clear();
        stopPlay_ = false;
    }
    QMetaObject::invokeMethod(&inThread_, [=, this] { produce(from, until, loopFrom, loop); });
}

void DecodeWorker::stopPlayback() {
    QMutexLocker lock(&ringMutex_);
    stopPlay_ = true;
    ring_.clear();
    ringRoom_.wakeAll();
}

bool DecodeWorker::push(Slot slot) {
    QMutexLocker lock(&ringMutex_);
    while (!stopPlay_ && ring_.size() >= std::size_t(kRing))
        ringRoom_.wait(&ringMutex_);
    if (stopPlay_) return false;
    ring_.push_back(std::move(slot));
    return true;
}

void DecodeWorker::produce(double from, double until, double loopFrom, bool loop) {
    if (!decoder_ || stopPlay_) return;
    int lap = 0;
    Frame f = decoder_->frameAt(from);
    while (!stopPlay_) {
        if (f.isNull() || f.seconds >= until - 1e-9) {
            if (!loop || lap > 1'000'000) {
                push(Slot{{}, lap, true, until});
                return;
            }
            ++lap; // the clock wraps to loopFrom at `until`
            f = decoder_->frameAt(loopFrom);
            if (f.isNull()) {
                push(Slot{{}, lap, true, until});
                return;
            }
        }
        if (!push(Slot{f, lap, false, f.seconds})) return;
        f = decoder_->next();
    }
}

DecodeWorker::Due DecodeWorker::takeDue(double t, int lap, double lapLength, double slack) {
    Due due;
    QMutexLocker lock(&ringMutex_);
    // How late a slot is on the clock: a slot of an earlier lap is that many laps later.
    auto lateness = [&](const Slot& s) { return t - (s.at - (lap - s.lap) * lapLength); };
    while (!ring_.empty()) {
        Slot& s = ring_.front();
        if (lateness(s) < -1e-9) break; // not due yet
        if (s.end) {
            due.ended = true;
            ring_.pop_front();
            break;
        }
        const bool newerDue = ring_.size() > 1 && !ring_[1].end && lateness(ring_[1]) >= -1e-9;
        if (lateness(s) > slack && newerDue) {
            ++due.skipped;
            ring_.pop_front();
            continue;
        }
        due.frame = std::move(s.frame);
        ring_.pop_front();
        break;
    }
    ringRoom_.wakeAll();
    return due;
}

} // namespace avloop::media

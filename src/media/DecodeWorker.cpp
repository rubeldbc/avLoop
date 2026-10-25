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

void DecodeWorker::requestFrame(double seconds) {
    QMutexLocker lock(&mutex_);
    pending_ = seconds;
    hasPending_ = true;
    if (serving_) return;
    serving_ = true;
    QMetaObject::invokeMethod(&inThread_, [this] { serve(); });
}

void DecodeWorker::serve() {
    while (true) {
        double t = 0;
        {
            QMutexLocker lock(&mutex_);
            if (!hasPending_) {
                serving_ = false;
                return;
            }
            t = pending_;
            hasPending_ = false;
        }
        Frame frame = decoder_ ? decoder_->frameAt(t) : Frame{};
        if (decoder_) hardware_ = decoder_->hardwareActive();
        if (!frame.isNull() && cache_)
            cache_->insert(FrameCache::Key{sourceId_, frame.width, frame.height, frame.pts}, frame);
        emit frameReady(t, frame);
    }
}

} // namespace avloop::media

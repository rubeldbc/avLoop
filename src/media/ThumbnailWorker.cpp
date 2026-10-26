#include "media/ThumbnailWorker.h"

#include "media/Decoder.h"

#include <QMetaObject>

#include <algorithm>
#include <cmath>

namespace avloop::media {

ThumbnailWorker::ThumbnailWorker(const MediaInfo& info, QThread* thread, QObject* parent)
    : QObject(parent), info_(info) {
    const double frames = info.frameCount > 0 ? double(info.frameCount) : info.duration * std::max(1.0, info.fps());
    count_ = info.duration > 0 ? std::clamp(int(std::lround(frames)), 1, kMaxSamples) : 0;
    images_.resize(count_);
    if (!thread) {
        ownThread_ = std::make_unique<QThread>();
        ownThread_->setObjectName(QStringLiteral("avloop-thumbs"));
        ownThread_->start(QThread::LowPriority);
        thread = ownThread_.get();
    }
    inThread_ = new QObject;
    inThread_->moveToThread(thread);
}

ThumbnailWorker::~ThumbnailWorker() {
    stop_ = true;
    QMetaObject::invokeMethod(
        inThread_,
        [this] {
            delete decoder_;
            decoder_ = nullptr;
        },
        Qt::BlockingQueuedConnection);
    inThread_->deleteLater();
    if (ownThread_) {
        ownThread_->quit();
        ownThread_->wait();
    }
}

double ThumbnailWorker::timeOf(int index) const {
    return count_ > 0 ? (index + 0.5) * info_.duration / count_ : 0;
}

int ThumbnailWorker::indexAt(double seconds) const {
    if (count_ <= 0 || info_.duration <= 0) return -1;
    return std::clamp(int(seconds / info_.duration * count_), 0, count_ - 1);
}

void ThumbnailWorker::request(const QList<int>& order) {
    QMutexLocker lock(&mutex_);
    queue_.clear();
    for (int i : order)
        if (i >= 0 && i < count_ && images_[i].isNull()) queue_.append(i);
    if (working_ || queue_.isEmpty()) return;
    working_ = true;
    QMetaObject::invokeMethod(inThread_, [this] { work(); });
}

QImage ThumbnailWorker::image(int index) const {
    QMutexLocker lock(&mutex_);
    return index >= 0 && index < count_ ? images_[index] : QImage();
}

void ThumbnailWorker::work() {
    if (stop_) return; // queued before the worker began to go
    if (!decoder_) {
        Decoder::Options options;
        options.hardware = false; // the playback decoder keeps the GPU's decoder
        options.threads = 1;      // keyframes need no threads; a 4K decoder on every core holds ~300 MB
        options.workingSize = QSize(info_.width, info_.height).scaled(kWidth, kHeight, Qt::KeepAspectRatio);
        decoder_ = new Decoder(info_, options);
        if (!decoder_->open()) {
            QMutexLocker lock(&mutex_);
            queue_.clear();
            working_ = false;
            return;
        }
    }
    while (!stop_) {
        int index = -1;
        {
            QMutexLocker lock(&mutex_);
            if (queue_.isEmpty()) {
                working_ = false;
                return;
            }
            index = queue_.takeFirst();
            if (!images_[index].isNull()) continue;
        }
        const Frame f = decoder_->keyframeAt(timeOf(index));
        // A copy of its own: the decoded frame goes back to FFmpeg's pool at once (spec §3.7.4).
        QImage image =
            f.isNull()
                ? QImage(1, 1, QImage::Format_RGBA8888_Premultiplied)
                : QImage(f.data.get(), f.width, f.height, f.stride, QImage::Format_RGBA8888_Premultiplied).copy();
        if (f.isNull()) image.fill(Qt::transparent);
        {
            QMutexLocker lock(&mutex_);
            images_[index] = std::move(image);
        }
        ++made_;
        emit ready(index);
    }
}

} // namespace avloop::media

#pragma once

#include "media/MediaInfo.h"

#include <QImage>
#include <QList>
#include <QMutex>
#include <QObject>
#include <QThread>
#include <QVector>

#include <atomic>
#include <memory>

namespace avloop::media {

class Decoder;

// The timeline's thumbnails (spec §7.1, §3.7): a software decoder of its own on its own thread, keyframe seeks only,
// each frame fitted in 160 × 90. The file is sampled at most 400 times; the strip names the samples to make, the
// visible ones first, and a new order replaces what still waits. Everything goes with the worker.
class ThumbnailWorker : public QObject {
    Q_OBJECT
public:
    static constexpr int kMaxSamples = 400;
    static constexpr int kWidth = 160;
    static constexpr int kHeight = 90;

    // On `thread` when given (a WorkerThread that outlives it), else on a thread of its own.
    explicit ThumbnailWorker(const MediaInfo& info, QThread* thread = nullptr, QObject* parent = nullptr);
    ~ThumbnailWorker() override;

    const MediaInfo& info() const { return info_; }
    int count() const { return count_; }
    double timeOf(int index) const; // the middle of the sample's share of the file
    int indexAt(double seconds) const;
    void request(const QList<int>& order);
    QImage image(int index) const; // null until made
    int made() const { return made_; }

signals:
    void ready(int index);

private:
    void work();

    MediaInfo info_;
    int count_ = 0;
    std::unique_ptr<QThread> ownThread_;
    QObject* inThread_ = nullptr;
    Decoder* decoder_ = nullptr; // made, used and destroyed on the worker's thread
    mutable QMutex mutex_;
    QList<int> queue_;
    QVector<QImage> images_;
    bool working_ = false;
    std::atomic<bool> stop_{false};
    std::atomic<int> made_{0};
};

} // namespace avloop::media

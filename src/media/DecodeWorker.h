#pragma once

#include "media/Decoder.h"
#include "media/MediaInfo.h"

#include <QMutex>
#include <QObject>
#include <QThread>

#include <atomic>
#include <memory>

namespace avloop::media {

class FrameCache;

// A decoder on its own thread (spec §3.5: decoding never happens on the UI thread). Requests are coalesced — only the
// newest waiting request is decoded — and answered by a signal; decoded frames go into the frame cache.
class DecodeWorker : public QObject {
    Q_OBJECT
public:
    DecodeWorker(const MediaInfo& info, const Decoder::Options& options, FrameCache* cache, QObject* parent = nullptr);
    ~DecodeWorker() override;

    const MediaInfo& info() const { return info_; }
    quint64 sourceId() const { return sourceId_; }
    bool hardware() const { return hardware_; }

    // Opens the decoder on its thread; opened() follows.
    void start();
    // The frame on screen at `seconds`; frameReady() follows (a newer request replaces one still waiting).
    void requestFrame(double seconds);

signals:
    void opened(bool ok, const QString& error, bool hardware, const QString& note);
    void frameReady(double requested, const avloop::media::Frame& frame);

private:
    void serve();

    MediaInfo info_;
    Decoder::Options options_;
    FrameCache* cache_;
    quint64 sourceId_;
    QThread thread_;
    QObject inThread_; // lives on thread_: the context the decoder's work is queued on
    std::unique_ptr<Decoder> decoder_;
    QMutex mutex_;
    double pending_ = 0;
    bool hasPending_ = false;
    bool serving_ = false;
    std::atomic<bool> hardware_{false};
};

} // namespace avloop::media

#pragma once

#include <QThread>

namespace avloop::media {

// A thread that lives as long as its owner (spec §3.5): the decode and thumbnail workers of one video after another
// run on it, so opening a video starts no thread — each new thread costs memory for good in a process the GPU driver
// watches (AV-007's memory walk).
class WorkerThread : public QThread {
public:
    explicit WorkerThread(const QString& name, Priority priority = InheritPriority) {
        setObjectName(name);
        start(priority);
    }
    ~WorkerThread() override {
        quit();
        wait();
    }
};

} // namespace avloop::media

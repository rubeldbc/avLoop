#pragma once

#include "core/ProjectJson.h"

#include <QJsonObject>
#include <QList>
#include <QLockFile>
#include <QObject>
#include <QSize>
#include <QString>

#include <cstdint>
#include <functional>
#include <memory>

namespace avloop::render {
struct RenderJob;
}

namespace avloop::exporting {

class ExportJob;

// The batch queue (spec §10.5): each entry a project snapshot and the file it writes; Start runs them one after
// another, one at a time (a single export holds the render thread and the encoder); a failure keeps its reason and the
// next entry runs. Pause stops the entry under way — an encode cannot wait half-written —, which goes back to Pending.
// The queue is saved in `file` (locked while this queue lives; another AvLoop holding it leaves this one unsaved) at
// every change; on reopening an entry that was running is Pending again. Drive it on the GUI thread.
class ExportQueue : public QObject {
    Q_OBJECT
public:
    enum class State { Pending, Running, Done, Failed };
    struct Entry {
        QString id;
        QJsonObject project; // the snapshot (ProjectJson; its video's path absolute)
        QString output;      // the file it writes
        State state = State::Pending;
        QString reason;         // Failed: why
        QString format;         // what the list shows, from the snapshot: "webm-vp9" …
        QSize size;             // the output's W×H
        double progress = 0;    // 0…1 while it runs; 1 when done (not saved)
        std::int64_t bytes = 0; // Done: the file's size
    };
    // Makes the export of one entry (the main window's render thread, export thread and disk cache).
    using JobMaker = std::function<std::unique_ptr<ExportJob>(const render::RenderJob&, const QString& path)>;

    ExportQueue(const QString& file, JobMaker makeJob, QObject* parent = nullptr);
    ~ExportQueue() override; // cancels the entry under way (it is Pending in the file)

    bool isSaved() const { return lock_ && lock_->isLocked(); }
    const QList<Entry>& entries() const { return entries_; }
    int indexOf(const QString& id) const;
    // Appends a Pending entry; its output size and format from `snapshot`.
    void add(const ProjectData& snapshot, QSize outputSize, const QString& output);
    void remove(const QString& id);       // the entry under way is cancelled first
    void reorder(const QStringList& ids); // the list's new order (ids missing keep their place at the end)
    void clearDone();
    void start();
    void pause();
    bool isStarted() const { return started_; }     // Start pressed and not paused nor finished
    bool isBusy() const { return job_ != nullptr; } // an entry's export runs
    ExportJob* job() const { return job_.get(); }
    // Whether an entry may start now (false while the main window's own export runs: the queue pauses).
    std::function<bool()> mayStart;

    static QString stateId(State s); // "pending", "running", "done", "failed"

signals:
    void changed();             // an entry added, removed, moved or its state
    void progressed(int index); // the entry under way, its progress
    void startedChanged();

private:
    void next();
    void finishEntry();
    void load();
    void save() const;

    QString file_;
    JobMaker makeJob_;
    std::unique_ptr<QLockFile> lock_;
    QList<Entry> entries_;
    std::unique_ptr<ExportJob> job_;
    QString running_; // the id under way
    bool started_ = false;
};

} // namespace avloop::exporting

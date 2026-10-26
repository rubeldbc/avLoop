// The main window's batch queue (spec §10.5): Menu → Batch Queue opens its window; Ctrl+Shift+E (Menu → Add to Batch
// Queue, the window's Add current) adds the open project as it is now, written where Export would suggest — the last
// export's name in the format's extension, else the name template in the export folder —; a name already on disk or
// in the queue takes the first free "name (n)". The queue's entries use the window's
// render thread and export thread, one at a time; Export waits while the queue runs, and the queue does not start
// while Export runs. What walks read of it, and its walk steps.
#include "core/AppSettings.h"
#include "core/Project.h"
#include "export/ExportJob.h"
#include "export/ExportQueue.h"
#include "export/FormatProfiles.h"
#include "render/RenderService.h"
#include "ui/MainWindow.h"
#include "ui/WalkRunner.h"
#include "ui/export/BatchWindow.h"
#include "ui/widgets/Toast.h"

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFileInfo>

namespace avloop {

void MainWindow::buildBatch() {
    queue_ = std::make_unique<exporting::ExportQueue>(
        appsettings::queueFile(), [this](const render::RenderJob& job, const QString& path) {
            return std::make_unique<exporting::ExportJob>(job, path, renderService(), exportContext(), disk_.get());
        });
    queue_->mayStart = [this] { return !exportRunning(); };
    connect(queue_.get(), &exporting::ExportQueue::changed, this, &MainWindow::refreshExportState);
    connect(findChild<QAction*>(QStringLiteral("action.batch")), &QAction::triggered, this, &MainWindow::openBatch);
    connect(findChild<QAction*>(QStringLiteral("action.batchAdd")), &QAction::triggered, this,
            [this] { addToBatch(); });

    exporting::ExportQueue* queue = queue_.get();
    WalkProbes::add(QStringLiteral("batch.open"), [this] { return batch_ != nullptr; });
    WalkProbes::add(QStringLiteral("batch.count"), [queue] { return int(queue->entries().size()); });
    WalkProbes::add(QStringLiteral("batch.states"), [queue] {
        QStringList s;
        for (const exporting::ExportQueue::Entry& e : queue->entries())
            s << exporting::ExportQueue::stateId(e.state);
        return s.join(QLatin1Char(','));
    });
    WalkProbes::add(QStringLiteral("batch.files"), [queue] {
        QStringList s;
        for (const exporting::ExportQueue::Entry& e : queue->entries())
            s << QFileInfo(e.output).fileName();
        return s.join(QLatin1Char(','));
    });
    // The reasons as a walk's log may keep them: the temp folder and the home folder (they name the owner) left out.
    WalkProbes::add(QStringLiteral("batch.reasons"), [queue] {
        QStringList s;
        for (const exporting::ExportQueue::Entry& e : queue->entries())
            if (!e.reason.isEmpty()) s << e.reason;
        return s.join(QStringLiteral(" | "))
            .replace(QDir::fromNativeSeparators(QDir::tempPath()), QStringLiteral("%TEMP%"), Qt::CaseInsensitive)
            .replace(QDir::fromNativeSeparators(QDir::homePath()), QStringLiteral("~"), Qt::CaseInsensitive);
    });
    WalkProbes::add(QStringLiteral("batch.started"), [queue] { return queue->isStarted(); });
    WalkProbes::add(QStringLiteral("batch.saved"), [queue] { return queue->isSaved(); });
    // {"do": "batchAdd", "file": "x.gif", "temp": true}: Ctrl+Shift+E with the entry written as `file` — in the
    // walk's folder, or with "temp" in the lab run's data folder (gone with it).
    WalkRunner::addStep(QStringLiteral("batchAdd"), [this](WalkRunner& walk, const QJsonObject& step) {
        const QString base = step.value(QStringLiteral("temp")).toBool() ? appsettings::dataDir() : walk.outDir();
        const QString file = QDir(base).absoluteFilePath(step.value(QStringLiteral("file")).toString());
        walk.log(QStringLiteral("     batchAdd %1").arg(QFileInfo(file).fileName()));
        addToBatch(file);
        return 200;
    });
    // {"do": "batchStart"}, {"do": "batchPause"}, {"do": "batchClose"}: Start, Pause, the window closed.
    WalkRunner::addStep(QStringLiteral("batchStart"), [queue](WalkRunner& walk, const QJsonObject&) {
        walk.log(QStringLiteral("     batchStart"));
        queue->start();
        return 200;
    });
    WalkRunner::addStep(QStringLiteral("batchPause"), [queue](WalkRunner& walk, const QJsonObject&) {
        walk.log(QStringLiteral("     batchPause"));
        queue->pause();
        return 200;
    });
    WalkRunner::addStep(QStringLiteral("batchClose"), [this](WalkRunner& walk, const QJsonObject&) {
        walk.log(QStringLiteral("     batchClose"));
        if (batch_) batch_->close();
        return 300;
    });
    // {"do": "batchMove", "from": 2, "to": 0}: an entry dragged to another place.
    WalkRunner::addStep(QStringLiteral("batchMove"), [queue](WalkRunner& walk, const QJsonObject& step) {
        const int from = step.value(QStringLiteral("from")).toInt();
        const int to = step.value(QStringLiteral("to")).toInt();
        walk.log(QStringLiteral("     batchMove %1 → %2").arg(from).arg(to));
        QStringList ids;
        for (const exporting::ExportQueue::Entry& e : queue->entries())
            ids << e.id;
        if (from >= 0 && from < ids.size() && to >= 0 && to < ids.size()) ids.move(from, to);
        queue->reorder(ids);
        return 200;
    });
}

bool MainWindow::batchBusy() const {
    return queue_ && queue_->isBusy();
}

void MainWindow::openBatch() {
    if (batch_) {
        if (!qApp->property("avloop.lab").toBool()) {
            batch_->raise();
            batch_->activateWindow();
        }
        return;
    }
    batch_ = new BatchWindow(queue_.get(), this);
    connect(batch_, &BatchWindow::addRequested, this, [this] { addToBatch(); });
    refreshExportState();
    showQuietly(batch_);
}

void MainWindow::addToBatch(const QString& path) {
    if (!video_) return;
    if (const QString reason = exportBlocker(); !reason.isEmpty()) {
        Toast::post(centralWidget(), reason, Toast::Kind::Warning);
        return;
    }
    QString out = path.isEmpty() ? suggestedExportPath() : path;
    // Never over a file already there, nor two entries writing one file.
    auto taken = [this](const QString& p) {
        if (QFileInfo::exists(p)) return true;
        for (const exporting::ExportQueue::Entry& e : queue_->entries())
            if (QFileInfo(e.output).absoluteFilePath().compare(QFileInfo(p).absoluteFilePath(), Qt::CaseInsensitive) ==
                0)
                return true;
        return false;
    };
    const QFileInfo info(out);
    const QString suffix = info.suffix().isEmpty() ? QString() : QStringLiteral(".") + info.suffix();
    for (int n = 1; taken(out); ++n)
        out = info.dir().filePath(QStringLiteral("%1 (%2)%3").arg(info.completeBaseName()).arg(n).arg(suffix));
    queue_->add(project_->snapshot(), project_->outputSize(), out);
    Toast::post(centralWidget(), tr("Added to the batch queue: %1").arg(QFileInfo(out).fileName()),
                Toast::Kind::Success, 4000, tr("Open queue"), [this] { openBatch(); });
}

} // namespace avloop

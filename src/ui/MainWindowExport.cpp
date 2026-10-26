// The main window's export side (spec §10.1, §12.2, §12.3): Export and Ctrl+E open the Save dialog (the project's last
// export, else `<source>_loop_<W>x<H>.webm` in the last folder), the job runs in the background from a snapshot with
// its bar in the status bar, the toast says how it ended (Show in folder; the OBS tip once a session, Don't show
// again), quitting asks first. Export and Preview stand disabled with the reason when they cannot run. The size
// estimate follows the project's changes while the Quality section is open.
#include "core/AppInfo.h"
#include "core/AppSettings.h"
#include "core/Geometry.h"
#include "core/Project.h"
#include "export/ExportJob.h"
#include "export/ExportQueue.h"
#include "export/FormatProfiles.h"
#include "export/SizeEstimator.h"
#include "media/WorkerThread.h"
#include "render/RenderService.h"
#include "ui/MainWindow.h"
#include "ui/WalkRunner.h"
#include "ui/export/BatchWindow.h"
#include "ui/export/ExportBar.h"
#include "ui/panels/Section.h"
#include "ui/panels/SidePanel.h"
#include "ui/widgets/Toast.h"

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>

#include <memory>

namespace avloop {

namespace {

QString stateName(exporting::ExportJob::State s) {
    using S = exporting::ExportJob::State;
    switch (s) {
    case S::Running:
        return QStringLiteral("running");
    case S::Done:
        return QStringLiteral("done");
    case S::Failed:
        return QStringLiteral("failed");
    case S::Cancelled:
        return QStringLiteral("cancelled");
    case S::Idle:
        break;
    }
    return QStringLiteral("idle");
}

} // namespace

// The export thread and the estimator start with the first estimate or export: a window that never exports starts no
// thread for them (each costs the GPU driver memory for good, AV-007).
QObject* MainWindow::exportContext() {
    if (!exportThread_) {
        exportThread_ = std::make_unique<media::WorkerThread>(QStringLiteral("AvLoop export"));
        exportContext_ = std::make_unique<QObject>();
        exportContext_->moveToThread(exportThread_.get());
    }
    return exportContext_.get();
}

exporting::SizeEstimator* MainWindow::estimator() {
    if (!estimator_) {
        estimator_ = std::make_unique<exporting::SizeEstimator>(renderService(), exportContext(), disk_.get());
        connect(estimator_.get(), &exporting::SizeEstimator::changed, this,
                [this] { side_->setEstimate(estimator_->text()); });
    }
    return estimator_.get();
}

void MainWindow::buildExport() {
    side_->bindQuality(project_);
    // The estimate follows a change of what shapes the file (spec §10.3), or the Quality section opened; opening a
    // video alone measures nothing — it stays cheap (§3.7.5).
    connect(side_->quality(), &Section::toggled, this, [this](bool) { requestEstimate(); });
    auto opening = std::make_shared<bool>(false); // a new document's range, transform and crop follow its Source
    connect(project_, &Project::changed, this, [this, opening](Project::Aspect a) {
        if (a == Project::Aspect::View || a == Project::Aspect::Time) return;
        refreshExportState();
        if (a == Project::Aspect::Source) {
            if (estimator_) estimator_->clear();
            *opening = true;
            QTimer::singleShot(0, this, [opening] { *opening = false; });
        } else if (!*opening) {
            requestEstimate();
        }
    });
    connect(export_, &QPushButton::clicked, this, &MainWindow::chooseExport);
    auto* ctrlE = new QAction(tr("Export"), this);
    ctrlE->setObjectName(QStringLiteral("action.export"));
    ctrlE->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));
    connect(ctrlE, &QAction::triggered, this, &MainWindow::chooseExport);
    addAction(ctrlE);
    connect(exportBar_, &ExportBar::cancelRequested, this, [this] {
        if (exportJob_) exportJob_->cancel();
    });

    auto job = [this] { return exportJob_.get(); };
    WalkProbes::add(QStringLiteral("export.enabled"), [this] { return export_->isEnabled(); });
    WalkProbes::add(QStringLiteral("export.reason"), [this] { return exportBlocker(); });
    WalkProbes::add(QStringLiteral("export.state"),
                    [job] { return stateName(job() ? job()->state() : exporting::ExportJob::State::Idle); });
    WalkProbes::add(QStringLiteral("export.done"), [job] { return job() ? job()->done() : 0; });
    WalkProbes::add(QStringLiteral("export.total"), [job] { return job() ? job()->total() : 0; });
    WalkProbes::add(QStringLiteral("export.passes"), [job] { return job() ? job()->passes() : 0; });
    WalkProbes::add(QStringLiteral("export.path"), [job] { return job() ? job()->result().path : QString(); });
    WalkProbes::add(QStringLiteral("export.bytes"), [job] { return job() ? double(job()->result().bytes) : 0.0; });
    WalkProbes::add(QStringLiteral("export.error"), [job] { return job() ? job()->result().error : QString(); });
    WalkProbes::add(QStringLiteral("export.verify"),
                    [job] { return job() ? job()->result().verifyProblem : QString(); });
    WalkProbes::add(QStringLiteral("export.renderer"), [job] { return job() ? job()->result().renderer : QString(); });
    WalkProbes::add(QStringLiteral("export.text"), [this] { return exportBar_->text(); });
    WalkProbes::add(QStringLiteral("export.defaultName"), [this] {
        return video_ ? exporting::defaultFileName(project_->source().path, project_->outputSize(),
                                                   project_->output().format, appsettings::nameTemplate())
                      : QString();
    });
    WalkProbes::add(QStringLiteral("question.last"), [this] { return lastQuestion_; });
    // What the last export left beside its name: "file", "part" (never once it ended), "none".
    WalkProbes::add(QStringLiteral("export.leftovers"), [job] {
        if (!job()) return QStringLiteral("none");
        if (QFileInfo::exists(job()->path() + QStringLiteral(".part"))) return QStringLiteral("part");
        return QFileInfo::exists(job()->path()) ? QStringLiteral("file") : QStringLiteral("none");
    });
    // {"do": "exportTo", "file": "x.webm"}: Export with the Save dialog answered by `file` (in the walk's folder; a
    // walk opens no dialog).
    WalkRunner::addStep(QStringLiteral("exportTo"), [this](WalkRunner& walk, const QJsonObject& step) {
        const QString file = QDir(walk.outDir()).absoluteFilePath(step.value(QStringLiteral("file")).toString());
        walk.log(QStringLiteral("     exportTo %1").arg(file));
        exportTo(file);
        return 200;
    });
    // {"do": "answer", "yes": true, "choice": "Don't save", "text": "…"}: how the walk answers the questions that
    // follow — yes/no, one of several choices, a name or a file — as no dialog opens in a lab run.
    WalkRunner::addStep(QStringLiteral("answer"), [this](WalkRunner& walk, const QJsonObject& step) {
        labAnswer_ = step.value(QStringLiteral("yes")).toBool(true);
        labChoice_ = step.value(QStringLiteral("choice")).toString();
        labText_ = step.value(QStringLiteral("text")).toString();
        lastQuestion_.clear();
        walk.log(QStringLiteral("     answer %1").arg(labAnswer_ ? QStringLiteral("yes") : QStringLiteral("no")));
        return 50;
    });
    // {"do": "closeWindow"}: the window's close button, as the user presses it — the export's question, then the save
    // question — without closing a lab run's window.
    WalkRunner::addStep(QStringLiteral("closeWindow"), [this](WalkRunner& walk, const QJsonObject&) {
        const bool closed = mayCloseDuringExport() && maySaveChanges();
        walk.log(QStringLiteral("     closeWindow: %1")
                     .arg(closed ? QStringLiteral("would close") : QStringLiteral("stays open")));
        if (closed && exportJob_) exportJob_->cancel();
        return 200;
    });
    refreshExportState();
}

bool MainWindow::exportRunning() const {
    return exportJob_ && exportJob_->state() == exporting::ExportJob::State::Running;
}

QString MainWindow::exportBlocker() const {
    if (!video_) return tr("Open a video to export it");
    if (render::RenderJob::of(project_->snapshot(), *video_).plan.length < 2)
        return tr("The loop is shorter than 2 frames: move In or Out");
    return {};
}

void MainWindow::refreshExportState() {
    const QString reason = exportBlocker();
    const QString busy = exportRunning() ? tr("An export is running")
                         : batchBusy()   ? tr("The batch queue is running")
                                         : QString();
    export_->setEnabled(reason.isEmpty() && busy.isEmpty());
    export_->setToolTip(!reason.isEmpty() ? reason : !busy.isEmpty() ? busy : tr("Export (Ctrl+E)"));
    preview_->setEnabled(reason.isEmpty());
    preview_->setToolTip(reason.isEmpty() ? tr("Preview (F5)") : reason);
    if (QAction* add = findChild<QAction*>(QStringLiteral("action.batchAdd"))) add->setEnabled(reason.isEmpty());
    if (batch_) {
        batch_->setCanAdd(reason.isEmpty(), reason);
        batch_->setCanStart(!exportRunning(), tr("An export is running"));
    }
}

void MainWindow::requestEstimate() {
    if (!video_ || !side_->quality()->isOpen() || !exportBlocker().isEmpty()) {
        if (estimator_) estimator_->clear();
        return;
    }
    estimator()->request(render::RenderJob::of(project_->snapshot(), *video_));
}

bool MainWindow::confirm(const QString& text) {
    lastQuestion_ = text;
    if (qApp->property("avloop.lab").toBool()) return labAnswer_;
    return QMessageBox::question(this, AppInfo::displayName(), text) == QMessageBox::Yes;
}

bool MainWindow::mayCloseDuringExport() {
    if (batchBusy()) {
        // Its entry under way is Pending again when AvLoop next opens.
        if (!confirm(tr("The batch queue is running — stop it and quit?"))) return false;
        queue_->pause();
        return true;
    }
    if (!exportRunning()) return true;
    if (!confirm(tr("Export in progress — cancel and quit?"))) return false;
    exportJob_->cancel();
    return true;
}

QString MainWindow::exportFolder() const {
    const QString folder = appsettings::exportFolder();
    // A lab run writes nothing into the owner's Videos: its own data folder instead.
    const QString videos = qApp->property("avloop.lab").toBool()
                               ? appsettings::dataDir()
                               : QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    return !folder.isEmpty() ? folder : appsettings::value(appsettings::kLastExportFolder, videos).toString();
}

// The project's last export with the format's extension; else the name template in Settings' default folder, the
// last folder or Videos.
QString MainWindow::suggestedExportPath() const {
    const QString extension = exporting::profileOf(project_->output().format).extension;
    if (const QString last = project_->exportPath(); !last.isEmpty()) {
        const QFileInfo info(last);
        return info.dir().filePath(info.completeBaseName() + QLatin1Char('.') + extension);
    }
    return QDir(exportFolder())
        .filePath(exporting::defaultFileName(project_->source().path, project_->outputSize(), project_->output().format,
                                             appsettings::nameTemplate()));
}

void MainWindow::chooseExport() {
    if (!video_ || exportRunning() || batchBusy()) return;
    if (const QString reason = exportBlocker(); !reason.isEmpty()) {
        Toast::post(centralWidget(), reason, Toast::Kind::Warning);
        return;
    }
    const QString suggested = suggestedExportPath();
    const exporting::FormatProfile& profile = exporting::profileOf(project_->output().format);
    const QString path = QFileDialog::getSaveFileName(this, tr("Export loop"), suggested,
                                                      tr("%1 (*.%2)").arg(profile.name, profile.extension));
    if (path.isEmpty()) return;
    appsettings::setValue(appsettings::kLastExportFolder, QFileInfo(path).absolutePath());
    exportTo(path);
}

void MainWindow::exportTo(const QString& path) {
    if (!video_ || exportRunning() || batchBusy()) return;
    if (const QString reason = exportBlocker(); !reason.isEmpty()) {
        Toast::post(centralWidget(), reason, Toast::Kind::Warning);
        return;
    }
    // Spec §12.2: a crop entirely outside the video is allowed, after a question.
    if (geometry::coverage(project_->crop(), project_->transform(), project_->source().size()) ==
            geometry::Coverage::None &&
        !confirm(tr("The output is fully transparent. Export it anyway?")))
        return;
    lastQuestion_.clear();
    exportJob_.reset(); // the last one, finished: what it held goes now
    exportJob_ = std::make_unique<exporting::ExportJob>(render::RenderJob::of(project_->snapshot(), *video_), path,
                                                        renderService(), exportContext(), disk_.get());
    connect(exportJob_.get(), &exporting::ExportJob::finished, this, &MainWindow::exportFinished);
    exportBar_->track(exportJob_.get());
    exportJob_->start();
    refreshExportState();
}

void MainWindow::exportFinished() {
    using S = exporting::ExportJob::State;
    const exporting::ExportJob& job = *exportJob_;
    const auto& r = job.result();
    refreshExportState();
    if (job.state() == S::Cancelled) {
        Toast::post(centralWidget(), tr("Export cancelled"), Toast::Kind::Info);
        return;
    }
    if (job.state() == S::Failed) {
        qWarning("export failed: %s", qPrintable(r.error));
        Toast::post(centralWidget(), tr("Export failed: %1").arg(r.error), Toast::Kind::Error, 10000);
        return;
    }
    project_->setExportPath(job.path());
    const render::RenderJob& j = job.job();
    if (!r.verifyProblem.isEmpty()) {
        qWarning("export check: %s: %s", qPrintable(r.path), qPrintable(r.verifyProblem));
        Toast::post(centralWidget(), tr("Exported, but the check found: %1").arg(r.verifyProblem), Toast::Kind::Error,
                    10000);
        return;
    }
    QString text = tr("Exported %1×%2 · %3 s · %4")
                       .arg(j.outputSize.width())
                       .arg(j.outputSize.height())
                       .arg(j.plan.length / j.fps.value(), 0, 'f', 2)
                       .arg(exporting::SizeEstimator::sizeText(r.bytes));
    if (r.movedAside)
        text = tr("%1 was in use, so the loop was saved as %2.")
                   .arg(QFileInfo(job.path()).fileName(), QFileInfo(r.path).fileName()) +
               QLatin1Char('\n') + text;
    const bool tip = !obsTipShown_ && appsettings::showObsTips();
    if (tip)
        text += QLatin1Char('\n') + tr("OBS: Media Source → Loop on · Use hardware decoding off (keeps transparency)");
    obsTipShown_ = obsTipShown_ || tip;
    const QString file = r.path;
    Toast* toast =
        Toast::post(centralWidget(), text, r.movedAside ? Toast::Kind::Warning : Toast::Kind::Success,
                    tip ? 12000 : 6000, tr("Show in folder"), [file] {
                        if (qApp->property("avloop.lab").toBool())
                            return; // a lab run never opens Explorer over the owner's work
                        QProcess::startDetached(QStringLiteral("explorer.exe"),
                                                {QStringLiteral("/select,"), QDir::toNativeSeparators(file)});
                    });
    if (tip) toast->addButton(tr("Don't show again"), [] { appsettings::setValue(appsettings::kShowObsTips, false); });
}

} // namespace avloop

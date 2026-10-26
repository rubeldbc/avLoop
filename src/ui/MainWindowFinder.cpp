// The main window's Loop Finder (spec §11.5): Menu → Loop Finder and Ctrl+L open its window for the open video (again:
// to the front); a new video closes it — its fingerprints go with it. What walks read of it, and its walk steps.
#include "core/Project.h"
#include "ui/MainWindow.h"
#include "ui/WalkRunner.h"
#include "ui/export/LoopFinderWindow.h"

#include <QAction>
#include <QApplication>
#include <QListWidget>

namespace avloop {

void MainWindow::buildFinder() {
    QAction* action = findChild<QAction*>(QStringLiteral("action.finder"));
    connect(action, &QAction::triggered, this, &MainWindow::openFinder);
    action->setEnabled(false);
    connect(project_, &Project::changed, this, [this, action](Project::Aspect a) {
        if (a == Project::Aspect::Source) action->setEnabled(project_->source().isValid());
    });

    auto finder = [this] { return finder_.data(); };
    WalkProbes::add(QStringLiteral("loopfinder.open"), [finder] { return finder() != nullptr; });
    WalkProbes::add(QStringLiteral("loopfinder.state"), [finder] { return finder() ? finder()->state() : QString(); });
    WalkProbes::add(QStringLiteral("loopfinder.count"),
                    [finder] { return finder() ? int(finder()->results().size()) : 0; });
    // "in-out score" of the best, in seconds.
    WalkProbes::add(QStringLiteral("loopfinder.best"), [finder] {
        if (!finder() || finder()->results().isEmpty()) return QString();
        const auto& c = finder()->results().front();
        return QStringLiteral("%1-%2 %3").arg(c.in, 0, 'f', 3).arg(c.out, 0, 'f', 3).arg(c.score);
    });
    WalkProbes::add(QStringLiteral("loopfinder.bestLength"), [finder] {
        return finder() && !finder()->results().isEmpty()
                   ? finder()->results().front().out - finder()->results().front().in
                   : 0.0;
    });
    // {"do": "finderClose"}: the window closed as its close button does (its fingerprints go).
    WalkRunner::addStep(QStringLiteral("finderClose"), [finder](WalkRunner& walk, const QJsonObject&) {
        walk.log(QStringLiteral("     finderClose"));
        if (finder()) finder()->close();
        return 300;
    });
    // {"do": "finderChoose", "row": 0, "preview": false}: a click (a double-click) on that row of the results.
    WalkRunner::addStep(QStringLiteral("finderChoose"), [finder](WalkRunner& walk, const QJsonObject& step) {
        const int row = step.value(QStringLiteral("row")).toInt();
        const bool preview = step.value(QStringLiteral("preview")).toBool();
        walk.log(QStringLiteral("     finderChoose %1%2")
                     .arg(row)
                     .arg(preview ? QStringLiteral(" (and Preview)") : QString()));
        if (finder()) finder()->choose(row, preview);
        return 400;
    });
}

void MainWindow::openFinder() {
    if (!video_) return;
    if (finder_) {
        if (!qApp->property("avloop.lab").toBool()) {
            finder_->raise();
            finder_->activateWindow();
        }
        return;
    }
    finder_ = new LoopFinderWindow(project_, *video_, this);
    connect(finder_, &LoopFinderWindow::previewRequested, this, &MainWindow::openPreview);
    showQuietly(finder_);
}

} // namespace avloop

// The main window's Preview side (spec §9): the Preview button and F5 open the one Preview window (again: to the
// front), any change to the project reaches it 250 ms later, and what walks read of it. Frames are made on the render
// service, which lives with the window (one thread, one GPU context — AV-007).
#include "core/Project.h"
#include "core/ProjectJson.h"
#include "media/DiskCache.h"
#include "media/FrameCache.h"
#include "render/RenderService.h"
#include "ui/MainWindow.h"
#include "ui/WalkRunner.h"
#include "ui/preview/PreviewPlayer.h"
#include "ui/preview/PreviewWindow.h"

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QPushButton>
#include <QTimer>

namespace avloop {

namespace {
QString encodeName(PreviewPlayer::Encode e) {
    switch (e) {
    case PreviewPlayer::Encode::Running:
        return QStringLiteral("running");
    case PreviewPlayer::Encode::Done:
        return QStringLiteral("done");
    case PreviewPlayer::Encode::OutOfDate:
        return QStringLiteral("outofdate");
    case PreviewPlayer::Encode::Failed:
        return QStringLiteral("failed");
    case PreviewPlayer::Encode::None:
        break;
    }
    return QStringLiteral("none");
}
} // namespace

void MainWindow::buildPreview() {
    previewDue_ = new QTimer(this);
    previewDue_->setSingleShot(true);
    previewDue_->setInterval(250); // spec §9.1: the Live preview follows 250 ms after a change
    connect(previewDue_, &QTimer::timeout, this, [this] {
        if (previewWindow_ && video_) previewWindow_->setJob(render::RenderJob::of(project_->snapshot(), *video_));
    });
    connect(project_, &Project::changed, this, [this](Project::Aspect a) {
        if (a != Project::Aspect::View && previewWindow_) previewDue_->start();
    });
    connect(preview_, &QPushButton::clicked, this, &MainWindow::openPreview);
    auto* f5 = new QAction(tr("Preview"), this);
    f5->setObjectName(QStringLiteral("action.preview"));
    f5->setShortcut(QKeySequence(Qt::Key_F5));
    connect(f5, &QAction::triggered, this, &MainWindow::openPreview);
    addAction(f5);

    auto p = [this] { return previewWindow_ ? previewWindow_->player() : nullptr; };
    WalkProbes::add(QStringLiteral("preview.open"), [this] { return !previewWindow_.isNull(); });
    WalkProbes::add(QStringLiteral("preview.position"), [p] { return p() ? p()->position() : -1; });
    WalkProbes::add(QStringLiteral("preview.length"), [p] { return p() ? p()->length() : 0; });
    WalkProbes::add(QStringLiteral("preview.cached"), [p] { return p() ? p()->cachedCount() : 0; });
    WalkProbes::add(QStringLiteral("preview.playing"), [p] { return p() && p()->playing(); });
    WalkProbes::add(QStringLiteral("preview.aroundSeam"), [p] { return p() && p()->aroundSeam(); });
    WalkProbes::add(QStringLiteral("preview.mode"), [p] {
        return p() && p()->mode() == PreviewPlayer::Mode::Encoded ? QStringLiteral("encoded") : QStringLiteral("live");
    });
    WalkProbes::add(QStringLiteral("preview.encode"), [p] { return p() ? encodeName(p()->encodeState()) : QString(); });
    WalkProbes::add(QStringLiteral("preview.encodedPath"), [p] { return p() ? p()->encodedPath() : QString(); });
    WalkProbes::add(QStringLiteral("preview.encodedBytes"), [p] { return p() ? p()->encodedBytes() : 0; });
    WalkProbes::add(QStringLiteral("preview.info"),
                    [this] { return previewWindow_ ? previewWindow_->info() : QString(); });
    WalkProbes::add(QStringLiteral("preview.status"),
                    [this] { return previewWindow_ ? previewWindow_->status() : QString(); });
    WalkProbes::add(QStringLiteral("preview.frame"), [p] {
        return p() && !p()->frame().isNull()
                   ? QStringLiteral("%1x%2").arg(p()->frame().width()).arg(p()->frame().height())
                   : QString();
    });
    // {"do": "previewWrite", "file": "x.rgba", "encoded": "y.webm"}: the frame on show as premultiplied RGBA bytes (and
    // x.rgba.png); with "encoded", a copy of the Encoded temporary file as well.
    WalkRunner::addStep(QStringLiteral("previewWrite"), [p](WalkRunner& walk, const QJsonObject& step) {
        const QString file = QDir::current().absoluteFilePath(step.value(QStringLiteral("file")).toString());
        const QImage f = p() ? p()->frame() : QImage();
        QFile out(file);
        const bool ok = !f.isNull() && out.open(QIODevice::WriteOnly);
        for (int y = 0; ok && y < f.height(); ++y)
            out.write(reinterpret_cast<const char*>(f.constScanLine(y)), qint64(f.width()) * 4);
        if (ok) f.convertToFormat(QImage::Format_RGBA8888).save(file + QStringLiteral(".png"));
        if (const QString copy = step.value(QStringLiteral("encoded")).toString(); !copy.isEmpty() && p()) {
            const QString to = QDir::current().absoluteFilePath(copy);
            QFile::remove(to);
            QFile::copy(p()->encodedPath(), to);
        }
        walk.log(QStringLiteral("%1 previewWrite %2: %3")
                     .arg(ok ? QStringLiteral("PASS") : QStringLiteral("FAIL"), file,
                          ok ? QStringLiteral("%1x%2").arg(f.width()).arg(f.height()) : QStringLiteral("no frame")));
        return 200;
    });
    // {"do": "previewSeek", "at": k}: paused on output frame k, as the slider puts it there.
    WalkRunner::addStep(QStringLiteral("previewSeek"), [p](WalkRunner& walk, const QJsonObject& step) {
        const int k = step.value(QStringLiteral("at")).toInt();
        walk.log(QStringLiteral("     previewSeek %1").arg(k));
        if (p()) {
            p()->pause();
            p()->seek(k);
        }
        return 300;
    });
    // {"do": "previewClose"}: the Preview window closed as its close button does.
    WalkRunner::addStep(QStringLiteral("previewClose"), [this](WalkRunner& walk, const QJsonObject&) {
        walk.log(QStringLiteral("     previewClose"));
        if (previewWindow_) previewWindow_->close();
        return 300;
    });
    // {"do": "projectSave", "file": "x.avloop"}: the project as it stands, for avloop-cli.
    WalkRunner::addStep(QStringLiteral("projectSave"), [this](WalkRunner& walk, const QJsonObject& step) {
        const QString file = QDir::current().absoluteFilePath(step.value(QStringLiteral("file")).toString());
        QString error;
        const bool ok = projectjson::save(project_->snapshot(), file, &error);
        walk.log(QStringLiteral("%1 projectSave %2 %3")
                     .arg(ok ? QStringLiteral("PASS") : QStringLiteral("FAIL"), file, error));
        return 200;
    });
}

render::RenderService* MainWindow::renderService() {
    if (!renderService_) renderService_ = std::make_unique<render::RenderService>();
    return renderService_.get();
}

void MainWindow::openPreview() {
    if (!video_) return;
    if (previewWindow_) {
        // Again: to the front (a lab run never takes the focus).
        if (!qApp->property("avloop.lab").toBool()) {
            previewWindow_->raise();
            previewWindow_->activateWindow();
        }
        return;
    }
    // The preview's cache: a third of the frame cache's budget, at most 512 MB (spec §3.7).
    const std::int64_t budget = std::min<std::int64_t>(512LL << 20, cache_->budget() / 3);
    previewWindow_ = new PreviewWindow(renderService(), disk_.get(), budget, this);
    previewWindow_->setJob(render::RenderJob::of(project_->snapshot(), *video_));
    if (qApp->property("avloop.lab").toBool()) {
        previewWindow_->setAttribute(Qt::WA_ShowWithoutActivating);
        previewWindow_->show();
        WalkRunner::placeQuietly(previewWindow_, previewWindow_->size());
    } else {
        previewWindow_->show();
    }
}

} // namespace avloop

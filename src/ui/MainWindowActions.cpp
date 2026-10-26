// The main window's video (spec §3.9, §5.1, §12.1): Open, Ctrl+O, drag and drop and `avloop.exe <file>` open a file
// by its content on a worker; a file that cannot be opened says why in a toast; the status bar shows the source, the
// GPU and the memory. Opening a new video releases everything of the previous one (spec §3.7.5).
#include "ui/MainWindow.h"

#include "media/DecodeWorker.h"
#include "media/DiskCache.h"
#include "media/FrameCache.h"
#include "media/MediaProbe.h"
#include "media/SystemMemory.h"
#include "media/WorkerThread.h"
#include "ui/WalkRunner.h"
#include "ui/canvas/Canvas.h"
#include "ui/canvas/DropZone.h"
#include "ui/timeline/PlaybackClock.h"
#include "ui/timeline/Timeline.h"
#include "ui/widgets/Toast.h"

#include <QDir>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QImage>
#include <QLabel>
#include <QMimeData>
#include <QPushButton>
#include <QSettings>
#include <QStackedWidget>
#include <QtConcurrent>

namespace avloop {

namespace {

QString megabytes(std::int64_t bytes) {
    return QString::number(bytes / (1024 * 1024));
}

QString gigabytes(std::int64_t bytes) {
    return QString::number(double(bytes) / (1LL << 30), 'f', 1);
}

SourceInfo sourceInfoOf(const media::MediaInfo& m) {
    SourceInfo s;
    s.path = m.path;
    s.width = m.width;
    s.height = m.height;
    s.fps = m.avgFps.isValid() ? m.avgFps : m.realFps;
    s.vfr = m.vfr;
    s.duration = m.duration;
    s.frameCount = m.frameCount;
    s.hasAlpha = m.hasAlpha;
    const QFileInfo fi(m.path);
    s.fileSize = fi.size();
    s.modified = fi.lastModified();
    s.stream = m.streamIndex;
    return s;
}

} // namespace

void MainWindow::chooseFile() {
    QSettings settings;
    const QString dir = settings.value(QStringLiteral("ui/lastOpenDir"), QDir::homePath()).toString();
    const QString filter = tr("All media (*.*);;Video files (*.mp4 *.mov *.mkv *.webm *.avi *.wmv *.asf *.flv *.ts "
                              "*.m2ts *.mts *.mpg *.mpeg *.vob *.mxf *.3gp *.ogv *.y4m *.gif *.apng *.webp *.m4v)");
    const QString path = QFileDialog::getOpenFileName(this, tr("Open a video"), dir, filter);
    if (path.isEmpty()) return;
    settings.setValue(QStringLiteral("ui/lastOpenDir"), QFileInfo(path).absolutePath());
    openFiles({path});
}

void MainWindow::openFiles(const QStringList& paths) {
    if (paths.isEmpty()) return;
    if (paths.first().endsWith(QLatin1String(".avloop"), Qt::CaseInsensitive))
        openProject(paths.first());
    else
        openVideo(paths.first());
}

void MainWindow::openProject(const QString& path) {
    ProjectData loaded;
    QString error;
    if (!projectjson::load(path, loaded, &error)) {
        Toast::post(centralWidget(), tr("Cannot open %1: %2").arg(QFileInfo(path).fileName(), error),
                    Toast::Kind::Error, 8000);
        return;
    }
    if (!QFileInfo::exists(loaded.source.path)) {
        Toast::post(centralWidget(), tr("The project's video was not found: %1").arg(loaded.source.path),
                    Toast::Kind::Error, 8000);
        return;
    }
    pendingProject_ = loaded;
    openVideo(loaded.source.path);
}

void MainWindow::openVideo(const QString& path) {
    videoState_ = QStringLiteral("opening");
    sourceLabel_->setText(tr("Opening %1…").arg(QFileInfo(path).fileName()));
    auto* watcher = new QFutureWatcher<media::MediaProbe::Result>(this);
    connect(watcher, &QFutureWatcher<media::MediaProbe::Result>::finished, this, [this, watcher, path] {
        const media::MediaProbe::Result r = watcher->result();
        watcher->deleteLater();
        probed(path, r.info, r.error);
    });
    watcher->setFuture(QtConcurrent::run([path] { return media::MediaProbe::probe(path); }));
}

void MainWindow::probed(const QString& path, const std::optional<media::MediaInfo>& info, const QString& error) {
    if (!info) {
        videoState_ = video_ ? QStringLiteral("ready") : QStringLiteral("error");
        lastError_ = error;
        Toast::post(centralWidget(), error, Toast::Kind::Error, 8000);
        qWarning().noquote() << "[media] cannot open" << path << ":" << error;
        updateStatus();
        return;
    }
    closeVideo();
    video_ = info;
    const SourceInfo source = sourceInfoOf(*info);
    if (pendingProject_ && QFileInfo(pendingProject_->source.path) == QFileInfo(path)) {
        ProjectData restored = *pendingProject_;
        restored.source = source;
        project_->restore(restored);
    } else {
        project_->setSource(source);
    }
    pendingProject_.reset();
    media::Decoder::Options options;
    options.hardware =
        QSettings().value(QStringLiteral("performance/hardwareDecoding"), QStringLiteral("auto")).toString() !=
        QLatin1String("off");
    if (!decodeThread_) decodeThread_ = std::make_unique<media::WorkerThread>(QStringLiteral("avloop-decode"));
    worker_ = std::make_unique<media::DecodeWorker>(*info, options, cache_.get(), decodeThread_.get());
    connect(worker_.get(), &media::DecodeWorker::opened, this,
            [this, w = worker_.get()](bool ok, const QString& err, bool, const QString& note) {
                if (worker_.get() != w) return;
                if (!ok) {
                    videoState_ = QStringLiteral("error");
                    lastError_ = err;
                    Toast::post(centralWidget(), err, Toast::Kind::Error, 8000);
                    return;
                }
                if (!note.isEmpty()) qInfo().noquote() << "[media]" << video_->path << note;
                worker_->requestFrame(0);
            });
    connect(worker_.get(), &media::DecodeWorker::frameReady, this,
            [this, w = worker_.get()](double, const media::Frame& frame) {
                if (worker_.get() == w) showFrame(frame);
            });
    timeline_->setSource(&*video_);
    clock_->setWorker(worker_.get());
    worker_->start();
    refreshTransport();
    lastError_.clear();
    updateStatus();
}

void MainWindow::closeVideo() {
    clock_->setWorker(nullptr);
    timeline_->setSource(nullptr);
    if (worker_ && cache_) cache_->removeSource(worker_->sourceId());
    worker_.reset();
    video_.reset();
    shownFrame_ = {};
    canvas_->setFrame({});
    canvasStack_->setCurrentWidget(dropZone_);
    videoState_ = QStringLiteral("none");
}

void MainWindow::showFrame(const media::Frame& frame) {
    if (frame.isNull()) {
        videoState_ = QStringLiteral("error");
        lastError_ = tr("No frame could be decoded from %1.").arg(QFileInfo(video_->path).fileName());
        Toast::post(centralWidget(), lastError_, Toast::Kind::Error, 8000);
        return;
    }
    canvasStack_->setCurrentWidget(canvas_);
    canvas_->setFrame(frame);
    shownSeconds_ = frame.seconds;
    shownFrame_ = QSize(frame.width, frame.height);
    videoState_ = QStringLiteral("ready");
    updateStatus();
}

void MainWindow::updateStatus() {
    if (video_) {
        sourceLabel_->setText(QStringLiteral("%1 · %2").arg(QFileInfo(video_->path).fileName(), video_->statusLine()));
        const bool hw = worker_ && worker_->hardware();
        gpuLabel_->setText(
            tr("GPU: %1 · HW decode %2").arg(media::SystemMemory::adapterName(), hw ? tr("on") : tr("off")));
        droppedLabel_->setText(tr("Dropped: %1").arg(clock_->dropped()));
    } else if (videoState_ != QLatin1String("opening")) {
        sourceLabel_->setText(tr("No video open"));
    }
    droppedLabel_->setVisible(bool(video_));
    const media::MemorySnapshot m = media::SystemMemory::read();
    memoryLabel_->setText(tr("RAM %1 MB · VRAM %2 MB · Cache %3/%4 GB")
                              .arg(megabytes(m.privateBytes), megabytes(m.vramUsed), gigabytes(cache_->used()),
                                   gigabytes(cache_->budget())));
    // Under pressure the caches are halved (spec §3.7.6); they come back when RAM is free again.
    constexpr std::int64_t kLow = 1LL << 30;
    if (m.freePhysical > 0 && m.freePhysical < kLow && !cache_->halved()) {
        cache_->halve();
        if (!pressureToastShown_)
            Toast::post(centralWidget(), tr("Memory is low: the frame cache was halved."), Toast::Kind::Warning);
        pressureToastShown_ = true;
    } else if (m.freePhysical > kLow + kLow / 2 && cache_->halved()) {
        cache_->restore();
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event) {
    QStringList paths;
    for (const QUrl& url : event->mimeData()->urls())
        if (url.isLocalFile()) paths << url.toLocalFile();
    if (paths.isEmpty()) return;
    event->acceptProposedAction();
    openFiles(paths);
}

void MainWindow::registerMediaProbes() {
    WalkProbes::add(QStringLiteral("video.state"), [this] { return videoState_; });
    WalkProbes::add(QStringLiteral("video.size"), [this] {
        return video_ ? QStringLiteral("%1x%2").arg(video_->width).arg(video_->height) : QString();
    });
    WalkProbes::add(QStringLiteral("video.codec"), [this] { return video_ ? video_->codec : QString(); });
    WalkProbes::add(QStringLiteral("video.error"), [this] { return lastError_; });
    WalkProbes::add(QStringLiteral("video.hardware"), [this] { return worker_ && worker_->hardware(); });
    WalkProbes::add(QStringLiteral("frame.size"),
                    [this] { return QStringLiteral("%1x%2").arg(shownFrame_.width()).arg(shownFrame_.height()); });
    WalkProbes::add(QStringLiteral("toast.text"), [this] {
        Toast* t = Toast::current(centralWidget());
        return t ? t->text() : QString();
    });
    WalkProbes::add(QStringLiteral("status.gpu"), [this] { return gpuLabel_->text(); });
    WalkProbes::add(QStringLiteral("status.memory"), [this] { return memoryLabel_->text(); });
    WalkProbes::add(QStringLiteral("cache.used"), [this] { return double(cache_->used()); });
    // {"do": "open", "file": "<path>"}: the walk's way to open a file (a file dialog is never opened by a walk).
    WalkRunner::addStep(QStringLiteral("open"), [this](WalkRunner& walk, const QJsonObject& step) {
        const QString file = step.value(QStringLiteral("file")).toString();
        walk.log(QStringLiteral("     open %1").arg(file));
        openVideo(QDir::isAbsolutePath(file) ? file : QDir::current().absoluteFilePath(file));
        return 400;
    });
}

} // namespace avloop

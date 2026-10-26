// The main window's timeline side (spec §6.4, §7): the timeline and the transport bound to the project, the playhead
// and what the canvas shows, the playback clock, the keys (Space, ←/→, Shift+←/→, Home/End, Ctrl+Home/End, L, I, O,
// Alt+I/Alt+O, \ and Shift+\), the Dropped counter, and what walks read of them. The range covers the frames from In
// up to Out: I takes the frame under the playhead as the first, O as the last (reading R-24).
#include "core/Project.h"
#include "core/Timecode.h"
#include "media/DecodeWorker.h"
#include "ui/MainWindow.h"
#include "ui/WalkRunner.h"
#include "ui/canvas/Canvas.h"
#include "ui/timeline/PlaybackClock.h"
#include "ui/timeline/Timeline.h"
#include "ui/timeline/Transport.h"
#include "ui/tools/ToolManager.h"
#include "ui/widgets/TimeEdit.h"

#include <QAction>
#include <QLabel>
#include <QStatusBar>
#include <QToolButton>

#include <algorithm>

namespace avloop {

namespace {

double frameLength(Project* p) {
    return timecode::secondsOf(1, p->outputFps());
}

} // namespace

void MainWindow::buildTimeline() {
    clock_ = new PlaybackClock(this);
    droppedLabel_ = new QLabel(statusBar());
    droppedLabel_->setObjectName(QStringLiteral("status.dropped"));
    statusBar()->addWidget(droppedLabel_);

    connect(timeline_, &Timeline::scrubbed, this, [this](double t, bool fast) { seek(t, fast); });
    connect(tools_, &ToolManager::activeChanged, timeline_, &Timeline::setClickTool);
    connect(clock_, &PlaybackClock::frameShown, this, [this](const media::Frame& f) {
        canvas_->setFrame(f);
        shownSeconds_ = f.seconds;
        // The timeline and the time field follow at most 30 times a second: next to the GL canvas a raster repaint
        // costs a whole-window composition (about 6 ms), too much to pay at every frame of a 60 fps video.
        if (rasterClock_.isValid() && rasterClock_.elapsed() < 33) return;
        rasterClock_.start();
        const double t = timecode::snap(f.seconds, project_->outputFps());
        timeline_->setPlayhead(t);
        transport_->timeEdit()->setSeconds(t);
    });
    connect(clock_, &PlaybackClock::stopped, this, [this](double last) {
        transport_->setPlaying(false);
        timeline_->setPlayhead(timecode::snap(last, project_->outputFps()));
        transport_->timeEdit()->setSeconds(timeline_->playhead());
        updateStatus();
    });
    connect(project_, &Project::changed, this, [this](Project::Aspect a) {
        if (a != Project::Aspect::Range && a != Project::Aspect::Source && a != Project::Aspect::Loop &&
            a != Project::Aspect::Output)
            return;
        refreshTransport();
        // Playing: a new range, Speed or rate takes effect at once.
        if (a != Project::Aspect::Source && clock_->running()) startPlayback(clock_->lastShown());
    });

    connect(transport_->play(), &QToolButton::clicked, this, &MainWindow::togglePlay);
    connect(transport_->prev(), &QToolButton::clicked, this, [this] { stepFrames(-1); });
    connect(transport_->next(), &QToolButton::clicked, this, [this] { stepFrames(1); });
    connect(transport_->goIn(), &QToolButton::clicked, this, [this] { seek(project_->range().in); });
    connect(transport_->goOut(), &QToolButton::clicked, this,
            [this] { seek(project_->range().out - frameLength(project_)); });
    connect(transport_->loop(), &QToolButton::clicked, this, &MainWindow::toggleLoop);
    connect(transport_->timeEdit(), &TimeEdit::timeEntered, this, [this](double t) { seek(t); });

    auto key = [this](const QList<QKeySequence>& keys, auto act) {
        auto* a = new QAction(this);
        a->setShortcuts(keys);
        connect(a, &QAction::triggered, this, act);
        addAction(a);
    };
    key({QKeySequence(Qt::Key_Space)}, [this] { togglePlay(); });
    key({QKeySequence(Qt::Key_Left)}, [this] { stepFrames(-1); });
    key({QKeySequence(Qt::Key_Right)}, [this] { stepFrames(1); });
    key({QKeySequence(Qt::SHIFT | Qt::Key_Left)}, [this] { stepFrames(-10); });
    key({QKeySequence(Qt::SHIFT | Qt::Key_Right)}, [this] { stepFrames(10); });
    key({QKeySequence(Qt::Key_Home)}, [this] { seek(project_->range().in); });
    key({QKeySequence(Qt::Key_End)}, [this] { seek(project_->range().out - frameLength(project_)); });
    key({QKeySequence(Qt::CTRL | Qt::Key_Home)}, [this] { seek(0); });
    key({QKeySequence(Qt::CTRL | Qt::Key_End)}, [this] { seek(fileEnd() - frameLength(project_)); });
    key({QKeySequence(Qt::Key_L)}, [this] { toggleLoop(); });
    key({QKeySequence(Qt::Key_Backslash)}, [this] { timeline_->fitFile(); });
    key({QKeySequence(Qt::SHIFT | Qt::Key_Backslash), QKeySequence(Qt::Key_Bar)}, [this] { timeline_->fitRange(); });
    auto setRange = [this](auto rule) {
        if (!project_->source().isValid()) return;
        project_->changeRange(rule(project_->range(), project_->source().duration, project_->outputFps()));
    };
    key({QKeySequence(Qt::Key_I)}, [this, setRange] {
        const double t = timeline_->playhead();
        setRange([t](TimeRange r, double dur, Rational fps) { return timerange::withIn(r, t, dur, fps); });
    });
    key({QKeySequence(Qt::Key_O)}, [this, setRange] {
        const double t = timeline_->playhead() + frameLength(project_); // the frame under the playhead is the last
        setRange([t](TimeRange r, double dur, Rational fps) { return timerange::withOut(r, t, dur, fps); });
    });
    key({QKeySequence(Qt::ALT | Qt::Key_I)}, [setRange] { setRange(timerange::clearIn); });
    key({QKeySequence(Qt::ALT | Qt::Key_O)}, [setRange] { setRange(timerange::clearOut); });

    WalkProbes::add(QStringLiteral("playhead"), [this] { return timeline_->playhead(); });
    WalkProbes::add(QStringLiteral("playback.playing"), [this] { return clock_->running(); });
    WalkProbes::add(QStringLiteral("playback.dropped"), [this] { return clock_->dropped(); });
    WalkProbes::add(QStringLiteral("playback.shown"), [this] { return clock_->shown(); });
    WalkProbes::add(QStringLiteral("playback.loop"), [this] { return loopOn_; });
    WalkProbes::add(QStringLiteral("frame.seconds"), [this] { return shownSeconds_; });
    WalkProbes::add(QStringLiteral("status.dropped"), [this] { return droppedLabel_->text(); });
    WalkProbes::add(QStringLiteral("range.in"), [this] { return project_->range().in; });
    WalkProbes::add(QStringLiteral("range.out"), [this] { return project_->range().out; });
    WalkProbes::add(QStringLiteral("range.text"), [this] { return rangeText_; });
    WalkProbes::add(QStringLiteral("transport.time"), [this] { return transport_->timeEdit()->text(); });
    WalkProbes::add(QStringLiteral("timeline.pxPerSecond"), [this] { return timeline_->pxPerSecond(); });
    WalkProbes::add(QStringLiteral("timeline.labels"), [this] {
        return timeline_->frameLabels() ? QStringLiteral("frames") : QStringLiteral("timecode");
    });
    WalkProbes::add(QStringLiteral("timeline.overlaps"), [this] { return timeline_->labelOverlaps(); });
    WalkProbes::add(QStringLiteral("timeline.thumbs"), [this] { return timeline_->thumbnailsMade(); });
    WalkProbes::add(QStringLiteral("timeline.fadeEdge"), [this] { return timeline_->fadeEdge(); });
    // Walks give timeline points as [seconds, y px]: {"space": "timeline", "at": [2.5, 40]}.
    WalkProbes::addMapper(QStringLiteral("timeline"),
                          [this](const QPointF& p) { return QPointF(timeline_->xOf(p.x()), p.y()); });
    refreshTransport();
}

double MainWindow::fileEnd() const {
    return timerange::fileEnd(project_->source().duration, project_->outputFps());
}

void MainWindow::seek(double t, bool fast) {
    if (!worker_ || !project_->source().isValid()) return;
    clock_->stop();
    const Rational fps = project_->outputFps();
    t = std::clamp(t, 0.0, std::max(0.0, fileEnd() - frameLength(project_)));
    t = timecode::secondsOf(timecode::nearestFrame(t, fps), fps);
    timeline_->setPlayhead(t);
    transport_->timeEdit()->setSeconds(t);
    worker_->requestFrame(t, fast ? media::DecodeWorker::Mode::Fast : media::DecodeWorker::Mode::Exact);
}

void MainWindow::stepFrames(int n) {
    const Rational fps = project_->outputFps();
    const double from = clock_->running() ? clock_->lastShown() : timeline_->playhead();
    seek(timecode::secondsOf(timecode::nearestFrame(from, fps) + n, fps));
}

void MainWindow::togglePlay() {
    if (!worker_ || !project_->source().isValid()) return;
    if (clock_->running()) {
        clock_->stop();
        return;
    }
    clock_->resetCounts();
    startPlayback(timeline_->playhead());
}

void MainWindow::startPlayback(double from) {
    const TimeRange r = project_->range();
    clock_->start(from, r.in, r.out, fileEnd(), frameLength(project_), loopOn_, project_->loop().speed);
    transport_->setPlaying(clock_->running());
}

void MainWindow::toggleLoop() {
    loopOn_ = !loopOn_;
    transport_->setLoopOn(loopOn_);
    if (clock_->running()) startPlayback(clock_->lastShown());
}

void MainWindow::refreshTransport() {
    const bool open = project_->source().isValid() && worker_;
    transport_->setEnabled(open);
    const Rational fps = project_->outputFps();
    const TimeRange r = project_->range();
    transport_->timeEdit()->setFps(fps);
    transport_->setTotal(timecode::format(fileEnd(), fps));
    rangeText_ =
        open ? tr("In %1 · Out %2 · %3 (%4 f)")
                   .arg(timecode::format(r.in, fps), timecode::format(r.out, fps), timecode::format(r.length(), fps))
                   .arg(timecode::nearestFrame(r.length(), fps))
             : QString();
    transport_->setRangeText(rangeText_);
    transport_->setLoopOn(loopOn_);
}

} // namespace avloop

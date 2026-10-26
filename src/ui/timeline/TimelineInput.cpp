// The timeline's input (spec §7.2, §6.4): a click or drag on the ruler or the strip scrubs (fast while moving, exact
// when it rests 150 ms or ends — the two-speed rule); with the In or Out tool active a click sets In or Out there;
// a handle drags In or Out, the bar moves the range keeping its length (one Undo step each); the playhead snaps to In
// and Out (and keyframes) within 8 px; Ctrl+wheel zooms about the pointer, the wheel and Shift+wheel scroll; a
// double-click on the ruler switches timecode and frame numbers. On the keyframe row a diamond clicked takes the
// playhead to it (it is then the one the options and Delete act on), dragged moves it in time on the grid, and
// Alt+dragged copies it there (the original stays; the drag is one Undo step).
#include "core/LoopMap.h"
#include "core/Project.h"
#include "core/Timecode.h"
#include "ui/timeline/Timeline.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QWheelEvent>

#include <cmath>

namespace avloop {

int Timeline::keyAt(const QPointF& at) const {
    if (!rows().keys.adjusted(0, -2, 0, 2).contains(at.toPoint())) return -1;
    const Keyframes& keys = project_->keyframes();
    int best = -1;
    double nearest = kHandlePx + 0.5;
    for (int i = 0; i < keys.size(); ++i) {
        const double d = std::abs(at.x() - xOf(keys[i].t));
        if (d < nearest) {
            nearest = d;
            best = i;
        }
    }
    return best;
}

Timeline::Drag Timeline::hit(const QPointF& at) const {
    const Rows r = rows();
    const TimeRange range = project_->range();
    if (!project_->source().isValid()) return Drag::None;
    if (keyAt(at) >= 0) return Drag::Key;
    // The crossfade band's left edge sets F.
    if (r.band.contains(at.toPoint()) && project_->loop().mode == LoopSettings::Mode::Crossfade &&
        std::abs(at.x() - xOf(fadeEdge())) <= kHandlePx)
        return Drag::Fade;
    if (at.y() > r.ruler.bottom() && at.y() <= r.overlayBottom()) {
        const double din = std::abs(at.x() - xOf(range.in));
        const double dout = std::abs(at.x() - xOf(range.out));
        // Both near (a short range): the nearer, and Out when the pointer is past the middle.
        if (std::min(din, dout) <= kHandlePx)
            return dout < din || (dout == din && at.x() > xOf(range.in)) ? Drag::Out : Drag::In;
    }
    if (r.bar.contains(at.toPoint()) && at.x() >= xOf(range.in) && at.x() <= xOf(range.out)) return Drag::Bar;
    return Drag::Scrub;
}

double Timeline::scrubTime(double x) const {
    const TimeRange range = project_->range();
    for (double edge : {range.in, range.out})
        if (std::abs(x - xOf(edge)) <= kSnapPx) return edge;
    for (const Keyframe& k : project_->keyframes())
        if (std::abs(x - xOf(k.t)) <= kSnapPx) return k.t;
    const double t = std::clamp(timeAt(x), 0.0, duration());
    return std::min(timecode::secondsOf(timecode::frameOf(t, fps()), fps()), duration());
}

void Timeline::scrubTo(double t, bool fast) {
    playhead_ = t;
    update();
    emit scrubbed(t, fast);
}

void Timeline::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton || !project_->source().isValid()) return;
    drag_ = hit(event->position());
    dragFrom_ = timeAt(event->position().x());
    dragRange_ = project_->range();
    dragKey_ = QStringLiteral("drag:range:%1").arg(++drags_);
    const double t = scrubTime(event->position().x());
    if (drag_ == Drag::Scrub && (clickTool_ == QLatin1String("in") || clickTool_ == QLatin1String("out"))) {
        // The In or Out tool: the click sets it there by the rules of the keys — the frame clicked is the first or
        // the last of the range — and the playhead goes with it.
        const double d = project_->source().duration;
        project_->changeRange(clickTool_ == QLatin1String("in")
                                  ? timerange::withIn(dragRange_, t, d, fps())
                                  : timerange::withOut(dragRange_, t + timecode::secondsOf(1, fps()), d, fps()));
        drag_ = Drag::None;
        scrubTo(t, false);
        return;
    }
    if (drag_ == Drag::Scrub) {
        scrubTo(t, true);
        rest_->start();
    }
    if (drag_ == Drag::Bar) setCursor(Qt::ClosedHandCursor);
    if (drag_ == Drag::Key) {
        dragKeyTime_ = project_->keyframes()[keyAt(event->position())].t;
        dragKey_ = QStringLiteral("drag:key:%1").arg(drags_);
        dragCopy_ = event->modifiers().testFlag(Qt::AltModifier);
        scrubTo(dragKeyTime_, false);
    }
}

void Timeline::mouseMoveEvent(QMouseEvent* event) {
    const double x = event->position().x();
    const double d = project_->source().duration;
    switch (drag_) {
    case Drag::None: {
        const Drag h = hit(event->position());
        if (h == Drag::Key && (event->modifiers() & Qt::AltModifier))
            setCursor(Qt::DragCopyCursor);
        else
            setCursor(h == Drag::In || h == Drag::Out || h == Drag::Fade || h == Drag::Key ? Qt::SizeHorCursor
                      : h == Drag::Bar                                                     ? Qt::OpenHandCursor
                                                                                           : Qt::ArrowCursor);
        return;
    }
    case Drag::Scrub:
        scrubTo(scrubTime(x), true);
        rest_->start();
        return;
    case Drag::In: {
        const TimeRange next = timerange::dragIn(project_->range(), timeAt(x), d, fps());
        project_->changeRange(next, dragKey_);
        scrubTo(next.in, true); // the canvas shows that frame
        return;
    }
    case Drag::Out: {
        const TimeRange next = timerange::dragOut(project_->range(), timeAt(x), d, fps());
        project_->changeRange(next, dragKey_);
        scrubTo(next.out - timecode::secondsOf(1, fps()), true); // the last frame of the range
        return;
    }
    case Drag::Bar:
        project_->changeRange(timerange::moved(dragRange_, timeAt(x) - dragFrom_, d, fps()), dragKey_);
        return;
    case Drag::Key: {
        // On the output grid; refused where another keyframe already is. With Alt the first step off its frame
        // copies it there and the copy moves on.
        const double t =
            std::min(timecode::secondsOf(timecode::nearestFrame(std::max(0.0, timeAt(x)), fps()), fps()), duration());
        const int i = keyframes::indexAt(project_->keyframes(), dragKeyTime_, project_->keyTolerance());
        const Keyframes before = project_->keyframes();
        if (dragCopy_)
            project_->copyKeyframe(i, t, dragKey_);
        else
            project_->moveKeyframe(i, t, dragKey_);
        if (project_->keyframes() != before) { // the one dragged (or its copy) is at t; refused, it stays
            dragCopy_ = false;
            dragKeyTime_ = t;
            scrubTo(t, true);
        }
        return;
    }
    case Drag::Fade: {
        // F = the frames from the pointer to Out, within 1 … ⌊D/2⌋.
        const TimeRange r = project_->range();
        LoopSettings l = project_->loop();
        const LoopPlan plan = LoopPlan::of(l, r.in, r.out, fps());
        const int f = loopmap::clampFade(int(std::lround((r.out - timeAt(x)) / segmentFrameSeconds())), plan.segment);
        l.fade = loopmap::secondsOf(f, fps());
        project_->changeLoop(l, dragKey_);
        return;
    }
    }
}

void Timeline::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) return;
    const Drag was = drag_;
    drag_ = Drag::None;
    rest_->stop();
    if (was == Drag::Scrub || was == Drag::In || was == Drag::Out || was == Drag::Key) emit scrubbed(playhead_, false);
    setCursor(Qt::ArrowCursor);
}

void Timeline::keyPressEvent(QKeyEvent* event) {
    // Delete (or Backspace) removes the keyframe at the playhead — the one shown solid.
    if ((event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) && project_->keyframeAtTime() >= 0) {
        project_->deleteKeyframe(project_->keyframeAtTime());
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void Timeline::mouseDoubleClickEvent(QMouseEvent* event) {
    if (rows().ruler.contains(event->position().toPoint())) setFrameLabels(!frameLabels_);
}

void Timeline::wheelEvent(QWheelEvent* event) {
    const QPoint delta = event->angleDelta();
    const int notch = delta.y() != 0 ? delta.y() : delta.x();
    if (notch == 0 || !project_->source().isValid()) return;
    if (event->modifiers() & Qt::ControlModifier) {
        zoomAbout(std::pow(1.25, notch / 120.0), event->position().x());
    } else {
        // The wheel and Shift+wheel scroll across, a tenth of the view a notch.
        const double before = scroll_;
        scroll_ -= notch / 120.0 * visibleSeconds() * 0.1;
        viewChanged();
        if (scroll_ != before) fit_ = false;
    }
    event->accept();
}

} // namespace avloop

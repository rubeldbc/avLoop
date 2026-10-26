// The timeline's input (spec §7.2, §6.4): a click or drag on the ruler or the strip scrubs (fast while moving, exact
// when it rests 150 ms or ends — the two-speed rule); with the In or Out tool active a click sets In or Out there;
// a handle drags In or Out, the bar moves the range keeping its length (one Undo step each); the playhead snaps to In
// and Out within 8 px; Ctrl+wheel zooms about the pointer, the wheel and Shift+wheel scroll; a double-click on the
// ruler switches timecode and frame numbers.
#include "core/Project.h"
#include "core/Timecode.h"
#include "ui/timeline/Timeline.h"

#include <QMouseEvent>
#include <QTimer>
#include <QWheelEvent>

#include <cmath>

namespace avloop {

Timeline::Drag Timeline::hit(const QPointF& at) const {
    const Rows r = rows();
    const TimeRange range = project_->range();
    if (!project_->source().isValid()) return Drag::None;
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
}

void Timeline::mouseMoveEvent(QMouseEvent* event) {
    const double x = event->position().x();
    const double d = project_->source().duration;
    switch (drag_) {
    case Drag::None: {
        const Drag h = hit(event->position());
        setCursor(h == Drag::In || h == Drag::Out ? Qt::SizeHorCursor
                  : h == Drag::Bar                ? Qt::OpenHandCursor
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
    }
}

void Timeline::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) return;
    const Drag was = drag_;
    drag_ = Drag::None;
    rest_->stop();
    if (was == Drag::Scrub || was == Drag::In || was == Drag::Out) emit scrubbed(playhead_, false);
    setCursor(Qt::ArrowCursor);
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

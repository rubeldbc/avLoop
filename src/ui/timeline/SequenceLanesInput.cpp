// The lanes' mouse (D-034): select, move, trim, copy, cut — each drag made from the sequence as it began, one Undo
// step.
#include "core/Project.h"
#include "core/SequenceEdit.h"
#include "ui/timeline/SequenceLanes.h"
#include "ui/timeline/Timeline.h"

#include <QMouseEvent>

#include <cmath>

namespace avloop {

namespace se = sequenceedit;

void SequenceLanes::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) return;
    Drag edge = Drag::None;
    const int id = clipAt(event->position(), &edge);
    drag_ = Drag::None;
    if (razor_) {
        // The Razor: the clip cut where it was clicked.
        Sequence s = project_->sequence();
        if (id && se::split(s, id, snapped(timeline_->timeAt(event->position().x()), {}, true))) commit(s, tr("Cut"));
        return;
    }
    if (!id) {
        if (!(event->modifiers() & Qt::ShiftModifier)) select({});
        return;
    }
    if (event->modifiers() & Qt::ShiftModifier) {
        QList<int> ids = selected_;
        ids.contains(id) ? void(ids.removeAll(id)) : ids.append(id);
        return select(ids);
    }
    if (!selected_.contains(id)) select({id});
    const Clip* c = se::clip(project_->sequence(), id);
    const se::Where w = se::find(project_->sequence(), id);
    if (!c || c->locked || project_->sequence().tracks[w.track].locked) return;
    drag_ = edge;
    pressed_ = id;
    pressTrack_ = w.track + 1;
    copy_ = edge == Drag::Move && (event->modifiers() & Qt::AltModifier);
    pressT_ = timeline_->timeAt(event->position().x());
    start_ = project_->sequence();
    frozenHead_ = headTrack();
    mergeKey_ = QStringLiteral("drag:lanes:%1").arg(++drags_);
}

void SequenceLanes::mouseMoveEvent(QMouseEvent* event) {
    if (drag_ == Drag::None) {
        if (razor_) return;
        Drag edge = Drag::None;
        const int over = clipAt(event->position(), &edge);
        setCursor(!over ? Qt::ArrowCursor : edge == Drag::Move ? Qt::SizeAllCursor : Qt::SizeHorCursor);
        return;
    }
    const bool noSnap = event->modifiers() & Qt::ControlModifier;
    const double dt = timeline_->timeAt(event->position().x()) - pressT_;
    if (drag_ == Drag::Move) return moveTo(event->position(), noSnap);
    Sequence s = start_;
    const Clip* was = se::clip(start_, pressed_);
    if (!was) return;
    const bool ok = drag_ == Drag::TrimIn ? se::trimIn(s, pressed_, snapped(was->start + dt, {pressed_}, noSnap))
                                          : se::trimOut(s, pressed_, snapped(was->end() + dt, {pressed_}, noSnap));
    if (ok) commit(s, drag_ == Drag::TrimIn ? tr("Trim the start") : tr("Trim the end"), mergeKey_);
}

void SequenceLanes::moveTo(const QPointF& at, bool noSnap) {
    const double dt = timeline_->timeAt(at.x()) - pressT_;
    const int dTrack = trackAt(at.y()) - pressTrack_;
    if (copy_ && std::abs(dt) * timeline_->pxPerSecond() < 2 && dTrack == 0) return;
    // The clips that move: the selected (the pressed one among them), or a copy of the pressed one (Alt).
    QList<int> ids = copy_ ? QList<int>{pressed_} : selected_;
    const Clip* lead = se::clip(start_, pressed_);
    if (!lead) return;
    // The lead's start snaps, or its end; the others move by as much.
    double start = snapped(lead->start + dt, ids, noSnap);
    const double end = snapped(lead->end() + dt, ids, noSnap);
    if (end != lead->end() + dt && start == lead->start + dt) start = end - lead->length();
    const double shift = start - lead->start;
    Sequence s = start_;
    struct Moving {
        Clip clip;
        int track;
    };
    QList<Moving> moving;
    for (int id : ids) {
        const se::Where w = se::find(s, id);
        if (!w.valid()) continue;
        Clip c = s.tracks[w.track].clips[w.index];
        if (copy_) {
            c.id = s.nextId++;
            c.name = tr("%1 copy").arg(c.name);
        } else {
            s.tracks[w.track].clips.removeAt(w.index);
        }
        moving.append({c, w.track});
    }
    for (Moving& m : moving) {
        // Up or down only where every clip fits; else in time on their own tracks.
        const int to = std::clamp(m.track + dTrack, 0, Sequence::kMaxTracks - 1);
        while (s.tracks.size() <= to)
            s.tracks.append({QStringLiteral("V%1").arg(s.tracks.size() + 1), true, false, {}});
        m.clip.start = std::max(0.0, m.clip.start + shift);
        const bool there = !s.tracks[to].locked && se::fits(s, to, m.clip.start, m.clip.length(), m.clip.id);
        const int track = there ? to : m.track;
        if (!se::fits(s, track, m.clip.start, m.clip.length(), m.clip.id) || s.clipCount() >= Sequence::kMaxClips)
            return; // nowhere to go: as it was
        s.tracks[track].clips.append(m.clip);
        std::sort(s.tracks[track].clips.begin(), s.tracks[track].clips.end(),
                  [](const Clip& a, const Clip& b) { return a.start < b.start; });
    }
    if (commit(s, copy_ ? tr("Copy a clip") : tr("Move clips"), mergeKey_) && copy_ && !moving.isEmpty())
        selected_ = {moving.first().clip.id};
}

void SequenceLanes::mouseReleaseEvent(QMouseEvent*) {
    if (drag_ != Drag::None && copy_ && !selected_.isEmpty()) select(selected_);
    drag_ = Drag::None;
    copy_ = false;
    frozenHead_ = 0;
    relayout();
    update();
}

void SequenceLanes::mouseDoubleClickEvent(QMouseEvent* event) {
    const int id = clipAt(event->position(), nullptr);
    const Clip* c = id ? se::clip(project_->sequence(), id) : nullptr;
    if (!c || !seek_) return;
    // On a key's diamond: to that key; else to the clip's start.
    for (const LayerKey& k : c->keys) {
        const double at = c->start + (k.t - c->range.in);
        if (std::abs(timeline_->xOf(at) - event->position().x()) <= 5) return seek_(at);
    }
    seek_(c->start);
}

} // namespace avloop

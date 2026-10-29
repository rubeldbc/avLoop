#include "ui/timeline/SequenceLanes.h"

#include "core/Project.h"
#include "core/SequenceEdit.h"
#include "core/SequenceView.h"
#include "ui/WalkRunner.h"
#include "ui/theme/Theme.h"
#include "ui/timeline/Timeline.h"
#include "ui/widgets/Toast.h"

#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QPainter>
#include <QPolygonF>

#include <cmath>

namespace avloop {

namespace se = sequenceedit;

SequenceLanes::SequenceLanes(Project* project, Timeline* timeline, std::function<void(double)> seek, QWidget* parent)
    : QWidget(parent), project_(project), timeline_(timeline), seek_(std::move(seek)) {
    setObjectName(QStringLiteral("lanes"));
    setFocusPolicy(Qt::ClickFocus);
    setMouseTracking(true);
    timeline_->installEventFilter(this); // its zoom and scroll repaint the lanes too
    connect(project, &Project::changed, this, [this](Project::Aspect a) {
        if (a == Project::Aspect::Sequence || a == Project::Aspect::Source) {
            QList<int> still;
            for (int id : selected_)
                if (se::clip(project_->sequence(), id)) still << id;
            selected_ = still;
            relayout();
        }
        if (a == Project::Aspect::LayerSelection) followLayerSelection();
        update();
    });
    // Walks give lane points as (seconds, track), V1 = 1.
    WalkProbes::addMapper(QStringLiteral("lanes"),
                          [this](const QPointF& p) { return QPointF(timeline_->xOf(p.x()), rowY(int(p.y()))); });
    WalkProbes::add(QStringLiteral("lanes.tracks"), [this] { return topTrack() >= 2 ? topTrack() - 1 : 0; });
    WalkProbes::add(QStringLiteral("lanes.selected"), [this] {
        QStringList names;
        for (int id : selected_)
            if (const Clip* c = se::clip(project_->sequence(), id))
                names << QStringLiteral("%1@%2").arg(c->name).arg(c->start, 0, 'f', 2);
        return names.join(QLatin1Char(' '));
    });
    WalkProbes::add(QStringLiteral("lanes.razor"), [this] { return razor_; });
    relayout();
}

int SequenceLanes::topTrack() const {
    int top = 1;
    const QList<Track>& tracks = project_->sequence().tracks;
    for (qsizetype t = 0; t < tracks.size(); ++t)
        if (!tracks[t].clips.isEmpty()) top = int(t) + 1;
    return top;
}

int SequenceLanes::headTrack() const {
    return frozenHead_ ? frozenHead_ : std::min(topTrack() + 1, Sequence::kMaxTracks);
}

double SequenceLanes::rowY(int track) const {
    return (headTrack() - track) * kLane + kLane / 2.0;
}

int SequenceLanes::trackAt(double y) const {
    return std::clamp(headTrack() - int(std::floor(y / kLane)), 1, Sequence::kMaxTracks);
}

void SequenceLanes::relayout() {
    if (!frozenHead_) setFixedHeight(project_->source().isValid() ? headTrack() * kLane : 0);
}

void SequenceLanes::setRazor(bool on) {
    razor_ = on;
    setCursor(on ? Qt::SplitHCursor : Qt::ArrowCursor);
    update();
}

void SequenceLanes::select(const QList<int>& ids) {
    selected_ = ids;
    // A layer's selection is the Layers section's too (its index among AvLoop's layers).
    int layer = -1;
    const QList<VideoLayer> layers = project_->layers();
    if (ids.size() == 1)
        for (qsizetype i = 0; i < layers.size(); ++i)
            if (layers[i].id == ids.first()) layer = int(i);
    project_->setSelectedLayer(layer);
    update();
}

void SequenceLanes::followLayerSelection() {
    const int i = project_->selectedLayer();
    if (i >= 0) {
        const int id = project_->layers()[i].id;
        if (selected_ != QList<int>{id}) selected_ = {id};
    } else if (selected_.size() == 1 && sequenceedit::find(project_->sequence(), selected_.first()).track > 0) {
        selected_.clear(); // the Layers section let it go
    }
}

bool SequenceLanes::eventFilter(QObject* watched, QEvent* event) {
    if (watched == timeline_ && event->type() == QEvent::Paint) update();
    return QWidget::eventFilter(watched, event);
}

int SequenceLanes::clipAt(const QPointF& at, Drag* edge) const {
    const int track = trackAt(at.y()) - 1;
    const QList<Track>& tracks = project_->sequence().tracks;
    if (track >= tracks.size()) return 0;
    constexpr double kEdge = 6; // px: a press this near a bar's end trims it
    for (const Clip& c : tracks[track].clips) {
        const double x0 = timeline_->xOf(c.start), x1 = timeline_->xOf(c.end());
        if (at.x() < x0 - kEdge / 2 || at.x() > x1 + kEdge / 2) continue;
        if (edge) *edge = at.x() <= x0 + kEdge ? Drag::TrimIn : at.x() >= x1 - kEdge ? Drag::TrimOut : Drag::Move;
        return c.id;
    }
    return 0;
}

double SequenceLanes::snapped(double t, const QList<int>& except, bool off) const {
    const double fps = project_->outputFps().value();
    if (off) return fps > 0 ? std::round(t * fps) / fps : t; // Ctrl: no snapping, still on a frame
    QList<double> targets = {project_->range().in, project_->range().out, project_->time()};
    for (const Track& track : project_->sequence().tracks)
        for (const Clip& c : track.clips)
            if (!except.contains(c.id)) targets << c.start << c.end();
    const double reach = 8.0 / std::max(1.0, timeline_->pxPerSecond());
    double best = t, bestD = reach;
    for (double target : targets)
        if (std::abs(target - t) <= bestD) {
            bestD = std::abs(target - t);
            best = target;
        }
    // Else on the output's frame grid, as Premiere's edits land on frames.
    return best == t && fps > 0 ? std::round(t * fps) / fps : best;
}

bool SequenceLanes::commit(Sequence after, const QString& text, const QString& mergeKey) {
    if (after.tracks.isEmpty() || after.tracks[0].clips.isEmpty()) {
        Toast::post(window(), tr("V1 keeps one clip: the main video."), Toast::Kind::Warning);
        return false;
    }
    project_->changeSequence(after, text, mergeKey);
    return true;
}

void SequenceLanes::cutAtPlayhead() {
    Sequence s = project_->sequence();
    const double t = project_->time();
    int cuts = 0;
    if (selected_.isEmpty())
        cuts = se::splitAll(s, t);
    else
        for (int id : selected_)
            cuts += se::split(s, id, t) ? 1 : 0;
    if (cuts > 0) commit(s, tr("Cut at the playhead"));
}

void SequenceLanes::removeSelected(bool ripple) {
    Sequence s = project_->sequence();
    for (int id : selected_)
        ripple ? se::rippleRemove(s, id) : se::remove(s, id);
    if (s == project_->sequence()) return;
    if (commit(s, ripple ? tr("Ripple delete") : tr("Delete"))) select({});
}

void SequenceLanes::keyPressEvent(QKeyEvent* event) {
    const bool shift = event->modifiers() & Qt::ShiftModifier;
    if (event->key() == Qt::Key_K && (event->modifiers() & Qt::ControlModifier)) return cutAtPlayhead();
    if ((event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) && !selected_.isEmpty())
        return removeSelected(shift);
    if (event->key() == Qt::Key_Escape && razor_) return setRazor(false);
    QWidget::keyPressEvent(event);
}

void SequenceLanes::contextMenuEvent(QContextMenuEvent* event) {
    const int index = trackAt(event->pos().y()) - 1;
    if (event->pos().x() > kHeader || index >= project_->sequence().tracks.size()) return;
    const Track& track = project_->sequence().tracks[index];
    QMenu menu(this);
    QAction* hide = menu.addAction(track.visible ? tr("Hide V%1").arg(index + 1) : tr("Show V%1").arg(index + 1));
    QAction* lock = menu.addAction(track.locked ? tr("Unlock V%1").arg(index + 1) : tr("Lock V%1").arg(index + 1));
    QAction* chosen = menu.exec(event->globalPos());
    if (!chosen) return;
    Sequence s = project_->sequence();
    if (chosen == hide) s.tracks[index].visible = !track.visible;
    if (chosen == lock) s.tracks[index].locked = !track.locked;
    commit(s, chosen->text());
}

void SequenceLanes::paintEvent(QPaintEvent*) {
    QPainter p(this);
    const ThemeTokens& t = Theme::instance().tokens();
    p.fillRect(rect(), t.bg);
    if (!project_->source().isValid()) return;
    const Sequence& s = project_->sequence();
    p.setFont(QFont(QStringLiteral("Segoe UI"), 8));
    for (int track = headTrack(); track >= 1; --track) {
        const QRectF lane(0, rowY(track) - kLane / 2.0, width(), kLane);
        const bool known = track - 1 < s.tracks.size();
        if (known && s.tracks[track - 1].locked) p.fillRect(lane, t.panel);
        p.setPen(t.border);
        p.drawLine(lane.bottomLeft(), lane.bottomRight());
    }
    for (qsizetype index = 0; index < s.tracks.size(); ++index)
        for (const Clip& c : s.tracks[index].clips) {
            const double y = rowY(int(index) + 1);
            const QRectF bar(QPointF(timeline_->xOf(c.start), y - kLane / 2.0 + 3),
                             QPointF(timeline_->xOf(c.end()), y + kLane / 2.0 - 3));
            QColor fill = c.id == s.main ? t.accent : t.accent.darker(130);
            fill.setAlpha(c.enabled && s.tracks[index].visible ? 110 : 40);
            p.fillRect(bar, fill);
            const bool selected = selected_.contains(c.id);
            p.setPen(QPen(selected ? t.text : t.accent, selected ? 1.5 : 1));
            p.drawRect(bar);
            p.setBrush(t.text); // its keys: diamonds at their time on the sequence
            for (const LayerKey& k : c.keys) {
                const QPointF d(timeline_->xOf(c.start + (k.t - c.range.in)), bar.bottom() - 4);
                p.drawPolygon(
                    QPolygonF({d + QPointF(0, -4), d + QPointF(4, 0), d + QPointF(0, 4), d + QPointF(-4, 0)}));
            }
            p.setBrush(Qt::NoBrush);
            p.setPen(c.enabled ? t.text : t.text2);
            p.drawText(bar.adjusted(6, 0, -4, 0), Qt::AlignVCenter | Qt::AlignLeft,
                       c.locked ? tr("%1 (locked)").arg(c.name) : c.name);
        }
    // The tracks' names over the bars, each on a chip of the background.
    for (int track = headTrack(); track >= 1; --track) {
        QString name = QStringLiteral("V%1").arg(track);
        if (track - 1 < s.tracks.size() && !s.tracks[track - 1].visible) name += QStringLiteral(" ∅");
        const QRectF chip(2, rowY(track) - 8, p.fontMetrics().horizontalAdvance(name) + 6, 16);
        p.fillRect(chip, t.bg);
        p.setPen(t.text2);
        p.drawText(chip, Qt::AlignCenter, name);
    }
    const double x = std::round(timeline_->xOf(project_->time())) + 0.5;
    p.setPen(QPen(t.accent, 1));
    p.drawLine(QPointF(x, 0), QPointF(x, height()));
}

} // namespace avloop

#include "ui/timeline/TrackLanes.h"

#include "core/Project.h"
#include "ui/WalkRunner.h"
#include "ui/theme/Theme.h"
#include "ui/timeline/Timeline.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPolygonF>

#include <cmath>

namespace avloop {

namespace {

constexpr double kEdge = 6; // px: a press this near a bar's end trims it
constexpr double kSnap = 8; // px

// The layers' state when a drag began: every move is made from it, so the drag merges into one Undo step.
QList<VideoLayer> g_start;

} // namespace

TrackLanes::TrackLanes(Project* project, Timeline* timeline, std::function<void(double)> seek, QWidget* parent)
    : QWidget(parent), project_(project), timeline_(timeline), seek_(std::move(seek)) {
    setObjectName(QStringLiteral("lanes"));
    setFocusPolicy(Qt::ClickFocus);
    setMouseTracking(true);
    timeline_->installEventFilter(this); // its zoom and scroll repaint the lanes too
    connect(project, &Project::changed, this, [this](Project::Aspect a) {
        if (a == Project::Aspect::Layers || a == Project::Aspect::Source) relayout();
        update();
    });
    // Walks give lane points as (seconds, track).
    WalkProbes::addMapper(QStringLiteral("lanes"),
                          [this](const QPointF& p) { return QPointF(timeline_->xOf(p.x()), rowY(int(p.y()))); });
    WalkProbes::add(QStringLiteral("lanes.tracks"), [this] { return topTrack() >= 2 ? topTrack() - 1 : 0; });
    relayout();
}

int TrackLanes::topTrack() const {
    int top = 0;
    for (const VideoLayer& l : project_->layers())
        top = std::max(top, l.track);
    return top;
}

int TrackLanes::headTrack() const {
    if (frozenHead_) return frozenHead_;
    const int top = topTrack();
    return top < 2 ? 0 : std::min(top + 1, 1 + VideoLayer::kMaxLayers);
}

double TrackLanes::rowY(int track) const {
    return (headTrack() - track) * kLane + kLane / 2.0;
}

int TrackLanes::trackAt(double y) const {
    return std::clamp(headTrack() - int(std::floor(y / kLane)), 2, 1 + VideoLayer::kMaxLayers);
}

void TrackLanes::relayout() {
    if (frozenHead_) return;
    // The lanes from the head (one empty lane above the highest, to drag a bar up into) down to V2.
    const int head = headTrack();
    setFixedHeight(head >= 2 ? (head - 1) * kLane : 0);
}

bool TrackLanes::eventFilter(QObject* watched, QEvent* event) {
    if (watched == timeline_ && event->type() == QEvent::Paint) update();
    return QWidget::eventFilter(watched, event);
}

int TrackLanes::barAt(const QPointF& at, Drag* edge) const {
    const QList<VideoLayer>& layers = project_->layers();
    const int track = trackAt(at.y());
    for (qsizetype i = layers.size() - 1; i >= 0; --i) {
        const VideoLayer& l = layers[i];
        if (l.track != track) continue;
        const double x0 = timeline_->xOf(l.start), x1 = timeline_->xOf(l.end());
        if (at.x() < x0 - kEdge / 2 || at.x() > x1 + kEdge / 2) continue;
        if (edge) *edge = at.x() <= x0 + kEdge ? Drag::TrimIn : at.x() >= x1 - kEdge ? Drag::TrimOut : Drag::Move;
        return int(i);
    }
    return -1;
}

double TrackLanes::snapped(double t, int except, bool off) const {
    if (off) return t;
    QList<double> targets = {project_->range().in, project_->range().out, project_->time()};
    const QList<VideoLayer>& layers = project_->layers();
    for (qsizetype i = 0; i < layers.size(); ++i)
        if (int(i) != except) targets << layers[i].start << layers[i].end();
    const double reach = kSnap / std::max(1.0, timeline_->pxPerSecond());
    double best = t, bestD = reach;
    for (double target : targets)
        if (std::abs(target - t) <= bestD) {
            bestD = std::abs(target - t);
            best = target;
        }
    return best;
}

void TrackLanes::paintEvent(QPaintEvent*) {
    QPainter p(this);
    const ThemeTokens& t = Theme::instance().tokens();
    p.fillRect(rect(), t.bg);
    const int head = headTrack();
    if (head < 2) return;
    p.setFont(QFont(QStringLiteral("Segoe UI"), 8));
    for (int track = head; track >= 2; --track) {
        const QRectF lane(0, rowY(track) - kLane / 2.0, width(), kLane);
        if (lane.top() >= height()) continue;
        p.setPen(t.border);
        p.drawLine(lane.bottomLeft(), lane.bottomRight());
        p.setPen(t.text2);
        p.drawText(lane.adjusted(4, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, QStringLiteral("V%1").arg(track));
    }
    const QList<VideoLayer>& layers = project_->layers();
    for (qsizetype i = 0; i < layers.size(); ++i) {
        const VideoLayer& l = layers[i];
        const double y = rowY(l.track);
        const QRectF bar(QPointF(timeline_->xOf(l.start), y - kLane / 2.0 + 3),
                         QPointF(timeline_->xOf(l.end()), y + kLane / 2.0 - 3));
        QColor fill = t.accent;
        fill.setAlpha(l.visible ? 110 : 40);
        p.fillRect(bar, fill);
        const bool selected = int(i) == project_->selectedLayer();
        p.setPen(QPen(selected ? t.text : t.accent, selected ? 1.5 : 1));
        p.drawRect(bar);
        // Its keyframes (D-029): diamonds at their time on the sequence.
        p.setBrush(t.text);
        for (const LayerKey& k : l.keys) {
            const QPointF c(timeline_->xOf(l.start + (k.t - l.range.in)), bar.bottom() - 4);
            p.drawPolygon(QPolygonF({c + QPointF(0, -4), c + QPointF(4, 0), c + QPointF(0, 4), c + QPointF(-4, 0)}));
        }
        p.setBrush(Qt::NoBrush);
        p.setPen(l.visible ? t.text : t.text2);
        p.drawText(bar.adjusted(6, 0, -4, 0), Qt::AlignVCenter | Qt::AlignLeft,
                   l.locked ? tr("%1 (locked)").arg(l.name) : l.name);
    }
    const double x = std::round(timeline_->xOf(project_->time())) + 0.5;
    p.setPen(QPen(t.accent, 1));
    p.drawLine(QPointF(x, 0), QPointF(x, height()));
}

void TrackLanes::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) return;
    Drag edge = Drag::None;
    index_ = barAt(event->position(), &edge);
    project_->setSelectedLayer(index_);
    if (index_ < 0 || project_->layers()[index_].locked) {
        drag_ = Drag::None;
        return;
    }
    drag_ = edge;
    frozenHead_ = headTrack();
    copy_ = edge == Drag::Move && (event->modifiers() & Qt::AltModifier);
    copied_ = false;
    pressT_ = timeline_->timeAt(event->position().x());
    g_start = project_->layers();
    mergeKey_ = QStringLiteral("drag:lane:%1").arg(++drags_);
}

void TrackLanes::mouseMoveEvent(QMouseEvent* event) {
    if (drag_ == Drag::None) {
        Drag edge = Drag::None;
        const int over = barAt(event->position(), &edge);
        setCursor(over < 0 ? Qt::ArrowCursor : edge == Drag::Move ? Qt::SizeAllCursor : Qt::SizeHorCursor);
        return;
    }
    const bool noSnap = event->modifiers() & Qt::ControlModifier;
    const double dt = timeline_->timeAt(event->position().x()) - pressT_;
    QList<VideoLayer> layers = g_start;
    int i = index_;
    if (copy_) { // Alt+drag: the copy is what moves, the original stays
        if (std::abs(dt) * timeline_->pxPerSecond() < 2 && trackAt(event->position().y()) == layers[i].track) return;
        layers.append(layers[i]);
        layers.last().name = tr("%1 copy").arg(layers[i].name);
        i = int(layers.size() - 1);
        if (layers.size() > VideoLayer::kMaxLayers) return;
        copied_ = true;
    }
    VideoLayer& l = layers[i];
    const VideoLayer& was = g_start[index_];
    if (drag_ == Drag::Move) {
        double start = snapped(was.start + dt, i, noSnap);
        const double end = snapped(was.end() + dt, i, noSnap);
        if (end != was.end() + dt && start == was.start + dt) start = end - was.length(); // the end snapped
        const int track = trackAt(event->position().y());
        l.start = start;
        if (videolayers::fits(layers, track, start, was.length(), i))
            l.track = track;
        else if (!videolayers::fits(layers, l.track, start, was.length(), i))
            return; // nowhere to go: as it was
    } else if (drag_ == Drag::TrimIn) {
        const double in = std::clamp(was.range.in + dt, 0.0, was.range.out - 0.1);
        l.start = was.start + (in - was.range.in);
        l.range.in = in;
        if (!videolayers::fits(layers, l.track, l.start, l.length(), i)) return;
    } else {
        l.range.out = std::clamp(was.range.out + dt, was.range.in + 0.1, std::max(was.range.in + 0.1, was.duration));
        if (!videolayers::fits(layers, l.track, l.start, l.length(), i)) return;
    }
    project_->changeLayers(layers, mergeKey_);
    if (copied_) project_->setSelectedLayer(i);
}

void TrackLanes::mouseReleaseEvent(QMouseEvent*) {
    drag_ = Drag::None;
    copy_ = false;
    frozenHead_ = 0;
    relayout();
    update();
}

void TrackLanes::mouseDoubleClickEvent(QMouseEvent* event) {
    const int i = barAt(event->position(), nullptr);
    if (i < 0 || !seek_) return;
    // On a keyframe's diamond: to that key; else to the bar's start.
    const VideoLayer& l = project_->layers()[i];
    for (const LayerKey& k : l.keys) {
        const double at = l.start + (k.t - l.range.in);
        if (std::abs(timeline_->xOf(at) - event->position().x()) <= 5) return seek_(at);
    }
    seek_(l.start);
}

void TrackLanes::keyPressEvent(QKeyEvent* event) {
    const int i = project_->selectedLayer();
    if ((event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) && i >= 0) {
        QList<VideoLayer> layers = project_->layers();
        layers.removeAt(i);
        project_->setSelectedLayer(-1);
        project_->changeLayers(layers);
        return;
    }
    QWidget::keyPressEvent(event);
}

} // namespace avloop

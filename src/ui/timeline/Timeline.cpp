#include "ui/timeline/Timeline.h"

#include "core/LoopMap.h"
#include "core/Project.h"
#include "media/ThumbnailWorker.h"
#include "ui/timeline/TimelineScale.h"

#include <QTimer>

#include <algorithm>
#include <cmath>

namespace avloop {

Timeline::Timeline(Project* project, QWidget* parent) : QWidget(parent), project_(project) {
    setObjectName(QStringLiteral("timeline"));
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent); // it fills itself: the panels behind it are not repainted
    setMinimumHeight(kRuler + kBar + kStripMin + kBand + kKeys);
    thumbsDue_ = new QTimer(this);
    thumbsDue_->setSingleShot(true);
    thumbsDue_->setInterval(40);
    connect(thumbsDue_, &QTimer::timeout, this, qOverload<>(&QWidget::update));
    rest_ = new QTimer(this);
    rest_->setSingleShot(true);
    rest_->setInterval(150);
    connect(rest_, &QTimer::timeout, this, [this] {
        if (drag_ == Drag::Scrub) emit scrubbed(playhead_, false);
    });
    connect(project_, &Project::changed, this, [this](Project::Aspect a) {
        if (a == Project::Aspect::Source) {
            playhead_ = 0;
            fitFile();
        }
        if (a == Project::Aspect::Range || a == Project::Aspect::Source || a == Project::Aspect::Loop ||
            a == Project::Aspect::Output)
            update();
    });
}

Timeline::~Timeline() = default;

void Timeline::setSource(const media::MediaInfo* info) {
    thumbs_.reset(); // the last video's thumbnails and their decoder go first
    if (info && info->isValid()) {
        thumbs_ = std::make_unique<media::ThumbnailWorker>(*info);
        connect(thumbs_.get(), &media::ThumbnailWorker::ready, this, [this](int) {
            if (!thumbsDue_->isActive()) thumbsDue_->start();
        });
    }
    fitFile();
}

int Timeline::thumbnailsMade() const {
    return thumbs_ ? thumbs_->made() : 0;
}

double Timeline::duration() const {
    return timerange::fileEnd(project_->source().duration, fps());
}

Rational Timeline::fps() const {
    return project_->outputFps();
}

double Timeline::segmentFrameSeconds() const {
    return loopmap::secondsOf(1, fps()) * project_->loop().speed;
}

double Timeline::fadeEdge() const {
    const TimeRange r = project_->range();
    if (project_->loop().mode != LoopSettings::Mode::Crossfade) return r.out;
    const LoopPlan plan = LoopPlan::of(project_->loop(), r.in, r.out, fps());
    return r.out - plan.fade * segmentFrameSeconds();
}

Timeline::Rows Timeline::rows() const {
    Rows r;
    const int w = width();
    const int strip = std::clamp(height() - (kRuler + kBar + kBand + kKeys), kStripMin, kStripMax);
    r.ruler = QRect(0, 0, w, kRuler);
    r.bar = QRect(0, r.ruler.bottom() + 1, w, kBar);
    r.strip = QRect(0, r.bar.bottom() + 1, w, strip);
    r.band = QRect(0, r.strip.bottom() + 1, w, kBand);
    r.keys = QRect(0, r.band.bottom() + 1, w, kKeys);
    return r;
}

double Timeline::visibleSeconds() const {
    return std::max(1, width() - 2 * kMargin) / pxPerSecond_;
}

double Timeline::xOf(double t) const {
    return kMargin + (t - scroll_) * pxPerSecond_;
}

double Timeline::timeAt(double x) const {
    return scroll_ + (x - kMargin) / pxPerSecond_;
}

void Timeline::viewChanged() {
    const double lo = timelinescale::minPxPerSecond(duration(), std::max(1, width() - 2 * kMargin));
    const double hi = std::max(lo, timelinescale::maxPxPerSecond(fps()));
    pxPerSecond_ = std::clamp(pxPerSecond_, lo, hi);
    scroll_ = std::clamp(scroll_, 0.0, std::max(0.0, duration() - visibleSeconds()));
    requestThumbnails();
    update();
}

void Timeline::zoomAbout(double factor, double x) {
    const double t = timeAt(x);
    fit_ = false;
    pxPerSecond_ *= factor;
    viewChanged();
    scroll_ = t - (x - kMargin) / pxPerSecond_;
    viewChanged();
}

void Timeline::fitFile() {
    fit_ = true;
    pxPerSecond_ = 0; // the clamp makes it the whole file
    scroll_ = 0;
    viewChanged();
}

void Timeline::fitRange() {
    const TimeRange r = project_->range();
    if (r.isEmpty()) return;
    fit_ = false;
    // The range with a little room either side.
    const double room = r.length() * 0.05;
    pxPerSecond_ = std::max(1, width() - 2 * kMargin) / (r.length() + 2 * room);
    viewChanged();
    scroll_ = r.in - room;
    viewChanged();
}

void Timeline::setFrameLabels(bool on) {
    frameLabels_ = on;
    update();
}

void Timeline::setPlayhead(double t) {
    playhead_ = t;
    // The view follows the playhead page by page (playback, the keys).
    if (!fit_ && (xOf(t) > width() - kMargin || xOf(t) < kMargin)) {
        scroll_ = t - visibleSeconds() * 0.1;
        viewChanged();
    }
    update();
}

void Timeline::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (fit_) pxPerSecond_ = 0;
    viewChanged();
}

void Timeline::requestThumbnails() {
    if (!thumbs_ || thumbs_->count() == 0) return;
    // The visible samples from the middle outwards, then the whole file in order.
    const double t0 = std::max(0.0, timeAt(kMargin));
    const double t1 = std::min(duration(), timeAt(width() - kMargin));
    const int a = thumbs_->indexAt(t0);
    const int b = thumbs_->indexAt(t1);
    const int mid = (a + b) / 2;
    QList<int> order;
    for (int d = 0; mid - d >= a || mid + d <= b; ++d) {
        if (mid + d <= b) order.append(mid + d);
        if (d > 0 && mid - d >= a) order.append(mid - d);
    }
    for (int i = 0; i < thumbs_->count(); ++i)
        if (i < a || i > b) order.append(i);
    thumbs_->request(order);
}

} // namespace avloop

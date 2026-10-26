// The timeline's painting (spec §7.1): the ruler with labels that never overlap, the thumbnails (a `panel2` block
// until each is made), the In/Out overlay — `accentSoft` inside, 50% black outside, the 3 px orange bar and the two
// bracket handles —, the crossfade band, the keyframe row (diamonds: the one at the playhead solid orange, the others
// hollow; Ease ones with rounded corners) and the playhead over all.
#include "core/LoopMap.h"
#include "core/Project.h"
#include "media/ThumbnailWorker.h"
#include "ui/theme/Theme.h"
#include "ui/timeline/Timeline.h"
#include "ui/timeline/TimelineScale.h"

#include <QPainter>
#include <QPainterPath>

#include <cmath>

namespace avloop {

void Timeline::paintEvent(QPaintEvent*) {
    QPainter p(this);
    const ThemeTokens& t = Theme::instance().tokens();
    const Rows r = rows();
    p.fillRect(rect(), t.panel);
    p.fillRect(r.ruler, t.bg);
    p.fillRect(r.keys, t.bg);
    if (!project_->source().isValid()) {
        overlaps_ = 0;
        return;
    }
    paintRuler(p, r);
    paintStrip(p, r);
    paintRange(p, r);
    paintBand(p, r);
    paintKeys(p, r);
    paintPlayhead(p, r);
}

void Timeline::paintRuler(QPainter& p, const Rows& r) {
    const ThemeTokens& t = Theme::instance().tokens();
    QFont mono(QStringLiteral("Consolas"));
    mono.setPixelSize(10);
    p.setFont(mono);
    const QFontMetricsF fm(mono);
    const double labelWidth = fm.horizontalAdvance(timelinescale::widestLabel(duration(), fps(), frameLabels_));
    const auto steps = timelinescale::steps(pxPerSecond_, fps(), labelWidth, frameLabels_);
    const double from = timeAt(0);
    const double to = std::min(duration(), timeAt(width()));
    overlaps_ = 0;
    double lastRight = -1e9;
    for (const timelinescale::Tick& tick : timelinescale::ticks(from, to, steps)) {
        const double x = std::round(xOf(tick.seconds)) + 0.5;
        p.setPen(tick.major ? t.text2 : t.border);
        p.drawLine(QPointF(x, r.ruler.bottom() - (tick.major ? 8 : 4)), QPointF(x, r.ruler.bottom()));
        if (!tick.major) continue;
        const QString text = timelinescale::label(tick.seconds, fps(), frameLabels_);
        const double w = fm.horizontalAdvance(text);
        if (x + 3 < lastRight) ++overlaps_;
        p.setPen(t.text2);
        p.drawText(QPointF(x + 3, r.ruler.top() + fm.ascent() + 1), text);
        lastRight = x + 3 + w;
    }
    p.setPen(t.border);
    p.drawLine(r.ruler.bottomLeft(), r.ruler.bottomRight());
}

void Timeline::paintStrip(QPainter& p, const Rows& r) {
    const ThemeTokens& t = Theme::instance().tokens();
    const QRectF file(xOf(0), r.strip.top(), duration() * pxPerSecond_, r.strip.height());
    p.save();
    p.setClipRect(file.intersected(QRectF(r.strip)));
    p.fillRect(file, t.panel2);
    if (thumbs_ && thumbs_->count() > 0) {
        // Cells as wide as a thumbnail at the strip's height, laid on the file's time so they hold still on a scroll.
        const QSizeF video = project_->source().size();
        const double aspect = std::clamp(video.width() / std::max(1.0, video.height()), 0.3, 4.0);
        const double cellPx = r.strip.height() * aspect;
        const double cellSeconds = cellPx / pxPerSecond_;
        const auto first = std::int64_t(std::floor(std::max(0.0, timeAt(0)) / cellSeconds));
        const auto last = std::int64_t(std::ceil(std::min(duration(), timeAt(width())) / cellSeconds));
        for (std::int64_t c = first; c <= last; ++c) {
            const QRectF cell(xOf(double(c) * cellSeconds), r.strip.top(), cellPx, r.strip.height());
            const QImage image = thumbs_->image(thumbs_->indexAt((double(c) + 0.5) * cellSeconds));
            if (image.isNull()) continue;
            const QSizeF fitted = QSizeF(image.size()).scaled(cell.size(), Qt::KeepAspectRatio);
            p.drawImage(QRectF(cell.center() - QPointF(fitted.width() / 2, fitted.height() / 2), fitted), image);
        }
    }
    p.restore();
}

void Timeline::paintRange(QPainter& p, const Rows& r) {
    const ThemeTokens& t = Theme::instance().tokens();
    const TimeRange range = project_->range();
    const double xin = xOf(range.in);
    const double xout = xOf(range.out);
    const double top = r.overlayTop();
    const double bottom = r.overlayBottom() + 1;
    // Outside: 50% black; inside: accentSoft.
    p.fillRect(QRectF(QPointF(xOf(0), top), QPointF(xin, bottom)), QColor(0, 0, 0, 128));
    p.fillRect(QRectF(QPointF(xout, top), QPointF(xOf(duration()), bottom)), QColor(0, 0, 0, 128));
    QColor soft = t.accentSoft;
    if (soft.alpha() == 255) soft.setAlpha(60);
    p.fillRect(QRectF(QPointF(xin, top), QPointF(xout, bottom)), soft);
    // The bar above the range: drag it to move the range.
    p.fillRect(QRectF(QPointF(xin, r.bar.bottom() - 2), QPointF(xout, r.bar.bottom() + 1)), t.accent);
    // The brackets: [ at In, ] at Out.
    auto bracket = [&](double x, int dir) {
        QPainterPath path;
        path.moveTo(x + dir * 10, r.bar.top() + 1);
        path.lineTo(x, r.bar.top() + 1);
        path.lineTo(x, bottom - 1);
        path.lineTo(x + dir * 10, bottom - 1);
        p.setPen(QPen(t.accent, 3, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);
    };
    bracket(xin, 1);
    bracket(xout, -1);
}

void Timeline::paintKeys(QPainter& p, const Rows& r) {
    const ThemeTokens& t = Theme::instance().tokens();
    const Keyframes& keys = project_->keyframes();
    const int current = project_->keyframeAtTime();
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    const double cy = r.keys.center().y() + 0.5;
    for (int i = 0; i < keys.size(); ++i) {
        const double x = xOf(keys[i].t);
        if (x < -10 || x > width() + 10) continue;
        QPainterPath diamond;
        diamond.addRoundedRect(QRectF(-4, -4, 8, 8), keys[i].ease ? 2.5 : 0, keys[i].ease ? 2.5 : 0);
        p.setTransform(QTransform().translate(x, cy).rotate(45));
        p.setPen(QPen(t.accent, 1.5));
        p.setBrush(i == current ? QBrush(t.accent) : QBrush(t.bg));
        p.drawPath(diamond);
    }
    p.restore();
}

void Timeline::paintBand(QPainter& p, const Rows& r) {
    // Crossfade: a gradient over the F frames before Out (they blend into the start) and a dashed outline over the F
    // after In (the part blended in). Ping-pong + Smooth turn: both ends' T frames filled quiet orange.
    const ThemeTokens& t = Theme::instance().tokens();
    const LoopSettings& l = project_->loop();
    const TimeRange range = project_->range();
    const LoopPlan plan = LoopPlan::of(l, range.in, range.out, fps());
    const double unit = segmentFrameSeconds();
    const QRectF band(0, r.band.top() + 1, 0, r.band.height() - 2);
    auto span = [&](double from, double to) {
        return QRectF(xOf(from), band.top(), (to - from) * pxPerSecond_, band.height());
    };
    if (l.mode == LoopSettings::Mode::Crossfade) {
        const QRectF end = span(range.out - plan.fade * unit, range.out);
        QLinearGradient g(end.topLeft(), end.topRight());
        QColor clear = t.accent;
        clear.setAlpha(0);
        g.setColorAt(0, clear);
        g.setColorAt(1, t.accent);
        p.fillRect(end, g);
        p.fillRect(QRectF(end.left() - 1, end.top() - 1, 2, end.height() + 2), t.accent); // the edge to drag
        p.setPen(QPen(t.accent, 1, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawRect(span(range.in, range.in + plan.fade * unit).adjusted(0.5, 0.5, -0.5, -0.5));
    } else if (l.mode == LoopSettings::Mode::PingPongSmooth) {
        QColor quiet = t.accent;
        quiet.setAlpha(90);
        p.fillRect(span(range.in, range.in + plan.turn * unit), quiet);
        p.fillRect(span(range.out - plan.turn * unit, range.out), quiet);
    }
}

void Timeline::paintPlayhead(QPainter& p, const Rows& r) {
    const ThemeTokens& t = Theme::instance().tokens();
    const double x = std::round(xOf(playhead_)) + 0.5;
    p.setPen(QPen(t.accent, 1));
    p.drawLine(QPointF(x, r.ruler.top() + 6), QPointF(x, r.keys.bottom()));
    QPainterPath head;
    head.moveTo(x - 5, r.ruler.top());
    head.lineTo(x + 5, r.ruler.top());
    head.lineTo(x, r.ruler.top() + 7);
    head.closeSubpath();
    p.setRenderHint(QPainter::Antialiasing);
    p.fillPath(head, t.accent);
}

} // namespace avloop

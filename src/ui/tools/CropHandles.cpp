#include "ui/tools/CropHandles.h"

#include <algorithm>
#include <cmath>

namespace avloop::crophandles {

namespace {

bool movesLeft(Handle h) {
    return h == Handle::TopLeft || h == Handle::Left || h == Handle::BottomLeft;
}
bool movesRight(Handle h) {
    return h == Handle::TopRight || h == Handle::Right || h == Handle::BottomRight;
}
bool movesTop(Handle h) {
    return h == Handle::TopLeft || h == Handle::Top || h == Handle::TopRight;
}
bool movesBottom(Handle h) {
    return h == Handle::BottomLeft || h == Handle::Bottom || h == Handle::BottomRight;
}

} // namespace

QList<Handle> all() {
    return {Handle::TopLeft,     Handle::Top,    Handle::TopRight,   Handle::Right,
            Handle::BottomRight, Handle::Bottom, Handle::BottomLeft, Handle::Left};
}

QPointF position(const QRectF& r, Handle h) {
    const double cx = r.center().x();
    const double cy = r.center().y();
    switch (h) {
    case Handle::TopLeft:
        return r.topLeft();
    case Handle::Top:
        return {cx, r.top()};
    case Handle::TopRight:
        return r.topRight();
    case Handle::Right:
        return {r.right(), cy};
    case Handle::BottomRight:
        return r.bottomRight();
    case Handle::Bottom:
        return {cx, r.bottom()};
    case Handle::BottomLeft:
        return r.bottomLeft();
    case Handle::Left:
        return {r.left(), cy};
    case Handle::None:
        break;
    }
    return r.center();
}

Handle hit(const QRectF& viewRect, const QPointF& view, double reach) {
    for (Handle h : all()) {
        const QPointF d = view - position(viewRect, h);
        if (std::abs(d.x()) <= reach && std::abs(d.y()) <= reach) return h;
    }
    return Handle::None;
}

Qt::CursorShape cursor(Handle h) {
    switch (h) {
    case Handle::TopLeft:
    case Handle::BottomRight:
        return Qt::SizeFDiagCursor;
    case Handle::TopRight:
    case Handle::BottomLeft:
        return Qt::SizeBDiagCursor;
    case Handle::Top:
    case Handle::Bottom:
        return Qt::SizeVerCursor;
    case Handle::Left:
    case Handle::Right:
        return Qt::SizeHorCursor;
    case Handle::None:
        break;
    }
    return Qt::ArrowCursor;
}

QRectF drawn(const QPointF& anchor, const QPointF& pointer, double ratio, bool fromCentre) {
    double dx = pointer.x() - anchor.x();
    double dy = pointer.y() - anchor.y();
    if (ratio > 0) {
        double w = std::abs(dx);
        double h = std::abs(dy);
        if (w < h * ratio) w = h * ratio;
        h = w / ratio;
        dx = std::copysign(w, dx == 0 ? 1 : dx);
        dy = std::copysign(h, dy == 0 ? 1 : dy);
    }
    if (fromCentre)
        return QRectF(anchor.x() - std::abs(dx), anchor.y() - std::abs(dy), 2 * std::abs(dx), 2 * std::abs(dy));
    return QRectF(anchor, QPointF(anchor.x() + dx, anchor.y() + dy)).normalized();
}

QRectF resized(const QRectF& start, Handle h, const QPointF& delta, double ratio, bool fromCentre) {
    double l = start.left(), r = start.right(), t = start.top(), b = start.bottom();
    if (movesLeft(h)) l += delta.x();
    if (movesRight(h)) r += delta.x();
    if (movesTop(h)) t += delta.y();
    if (movesBottom(h)) b += delta.y();
    if (fromCentre) {
        if (movesLeft(h)) r -= delta.x();
        if (movesRight(h)) l -= delta.x();
        if (movesTop(h)) b -= delta.y();
        if (movesBottom(h)) t -= delta.y();
    }
    QRectF out = QRectF(QPointF(l, t), QPointF(r, b)).normalized();
    if (ratio <= 0) return out;
    // The ratio kept: the larger side wins on a corner; an edge sets its side and the other follows, centred.
    double w = out.width();
    double hgt = out.height();
    const bool horizontal = h == Handle::Left || h == Handle::Right;
    const bool vertical = h == Handle::Top || h == Handle::Bottom;
    if (horizontal)
        hgt = w / ratio;
    else if (vertical)
        w = hgt * ratio;
    else if (w < hgt * ratio)
        w = hgt * ratio;
    else
        hgt = w / ratio;
    QPointF anchor;
    if (fromCentre || horizontal || vertical) anchor = out.center();
    if (fromCentre) return QRectF(anchor.x() - w / 2, anchor.y() - hgt / 2, w, hgt);
    if (horizontal) {
        const double x = movesLeft(h) ? out.right() - w : out.left();
        return QRectF(x, anchor.y() - hgt / 2, w, hgt);
    }
    if (vertical) {
        const double y = movesTop(h) ? out.bottom() - hgt : out.top();
        return QRectF(anchor.x() - w / 2, y, w, hgt);
    }
    // A corner: the opposite corner stays.
    const double x = movesLeft(h) ? start.right() - w : start.left();
    const double y = movesTop(h) ? start.bottom() - hgt : start.top();
    return QRectF(x, y, w, hgt);
}

double snap(double value, const QList<double>& targets, double tolerance, double* line) {
    double best = value;
    double bestDistance = tolerance;
    for (double t : targets) {
        const double d = std::abs(value - t);
        if (d <= bestDistance) {
            bestDistance = d;
            best = t;
            if (line) *line = t;
        }
    }
    return best;
}

QList<double> targetsX(const QRectF& v) {
    return {v.left(), v.center().x(), v.right()};
}

QList<double> targetsY(const QRectF& v) {
    return {v.top(), v.center().y(), v.bottom()};
}

} // namespace avloop::crophandles

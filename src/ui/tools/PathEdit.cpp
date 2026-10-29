#include "ui/tools/PathEdit.h"

#include <QLineF>
#include <QStringList>

#include <cmath>

namespace avloop::pathedit {

namespace {

QPointF lerp(const QPointF& a, const QPointF& b, double t) {
    return a + (b - a) * t;
}

double length(const QPointF& v) {
    return std::hypot(v.x(), v.y());
}

} // namespace

QPointF at(const MaskShape& shape, int segment, double t) {
    const qsizetype n = shape.points.size();
    const PathPoint& a = shape.points[segment];
    const PathPoint& b = shape.points[(segment + 1) % n];
    const double u = 1 - t;
    return a.p * (u * u * u) + a.out * (3 * u * u * t) + b.in * (3 * u * t * t) + b.p * (t * t * t);
}

SegmentHit hitSegment(const MaskShape& shape, const QTransform& toView, const QPointF& view, double reach) {
    SegmentHit best;
    double bestD = reach;
    const qsizetype n = shape.points.size();
    if (n < 2) return best;
    constexpr int kSteps = 48;
    for (int s = 0; s < int(n); ++s)
        for (int i = 0; i <= kSteps; ++i) {
            const double t = double(i) / kSteps;
            const double d = QLineF(toView.map(at(shape, s, t)), view).length();
            if (d <= bestD) {
                bestD = d;
                best = {s, t};
            }
        }
    // Away from the ends: a click on an anchor is the anchor's.
    if (best.segment >= 0 && (best.t < 0.02 || best.t > 0.98)) best.segment = -1;
    return best;
}

int hitHandle(const MaskShape& shape, const QTransform& toView, const QPointF& view, bool* out, double reach) {
    for (qsizetype i = shape.points.size() - 1; i >= 0; --i) {
        const PathPoint& pt = shape.points[i];
        for (bool o : {true, false}) {
            const QPointF h = o ? pt.out : pt.in;
            if (h == pt.p) continue;
            if (QLineF(toView.map(h), view).length() <= reach) {
                if (out) *out = o;
                return int(i);
            }
        }
    }
    return -1;
}

MaskShape inserted(const MaskShape& shape, int segment, double t, int* index) {
    MaskShape s = asPath(shape);
    const qsizetype n = s.points.size();
    PathPoint& a = s.points[segment];
    PathPoint& b = s.points[(segment + 1) % n];
    const bool straight = a.out == a.p && b.in == b.p;
    PathPoint mid;
    if (straight) {
        mid = PathPoint::corner(lerp(a.p, b.p, t));
    } else {
        const QPointF q0 = lerp(a.p, a.out, t), q1 = lerp(a.out, b.in, t), q2 = lerp(b.in, b.p, t);
        const QPointF r0 = lerp(q0, q1, t), r1 = lerp(q1, q2, t);
        mid = {lerp(r0, r1, t), r0, r1, true};
        a.out = q0;
        b.in = q2;
    }
    s.points.insert(segment + 1, mid);
    if (index) *index = segment + 1;
    return s;
}

MaskShape removed(const MaskShape& shape, int anchor) {
    if (shape.points.size() <= 2 || anchor < 0 || anchor >= shape.points.size()) return shape;
    MaskShape s = asPath(shape);
    s.points.removeAt(anchor);
    return s;
}

MaskShape converted(const MaskShape& shape, int anchor) {
    MaskShape s = asPath(shape);
    const qsizetype n = s.points.size();
    if (anchor < 0 || anchor >= n) return s;
    PathPoint& pt = s.points[anchor];
    if (pt.smooth || pt.hasHandles()) {
        pt = PathPoint::corner(pt.p);
        return s;
    }
    const QPointF prev = s.points[(anchor + n - 1) % n].p;
    const QPointF next = s.points[(anchor + 1) % n].p;
    const QPointF line = next - prev;
    const double len = length(line);
    if (len <= 0) return s;
    const QPointF dir = line / len;
    pt.in = pt.p - dir * (length(pt.p - prev) / 3);
    pt.out = pt.p + dir * (length(next - pt.p) / 3);
    pt.smooth = true;
    return s;
}

void moveHandle(PathPoint& pt, bool out, const QPointF& to, bool breakPair) {
    QPointF& mine = out ? pt.out : pt.in;
    QPointF& other = out ? pt.in : pt.out;
    mine = to;
    if (breakPair) {
        pt.smooth = false;
        return;
    }
    if (!pt.smooth) return;
    const QPointF away = pt.p - to;
    const double len = length(away);
    if (len <= 0) return;
    const double keep = other == pt.p ? len : length(other - pt.p);
    other = pt.p + away / len * keep;
}

QString kinds(const MaskShape& shape) {
    QStringList k;
    for (const PathPoint& pt : shape.points)
        k << (pt.smooth ? QStringLiteral("s") : pt.hasHandles() ? QStringLiteral("b") : QStringLiteral("c"));
    return k.join(QLatin1Char(','));
}

MaskShape asPath(const MaskShape& shape) {
    MaskShape s = shape;
    if (s.kind == MaskShape::Kind::Polygon) s.kind = MaskShape::Kind::Path;
    return s;
}

} // namespace avloop::pathedit

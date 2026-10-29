#include "core/ShapeCombine.h"

#include <algorithm>

namespace avloop::shapecombine {

QPainterPath combinedPath(const QList<QPainterPath>& paths, Op op) {
    if (paths.isEmpty()) return {};
    QPainterPath result = paths.first();
    for (qsizetype i = 1; i < paths.size(); ++i) {
        const QPainterPath& p = paths[i];
        switch (op) {
        case Op::Unite:
            result = result.united(p);
            break;
        case Op::MinusFront:
            result = result.subtracted(p);
            break;
        case Op::Intersect:
            result = result.intersected(p);
            break;
        case Op::Exclude:
            result = result.united(p).subtracted(result.intersected(p));
            break;
        }
    }
    return result.simplified();
}

namespace {

// Qt's path booleans give a curve back as many short lines: each outline of corners only is simplified to half a
// scene px (Douglas–Peucker): fewer anchors, the outline within half a pixel of the exact one (R-89).
QList<PathPoint> fewerAnchors(const QList<PathPoint>& points) {
    MaskShape s;
    s.points = points;
    QList<PathPoint> out;
    for (const auto& [begin, end] : s.outlines()) {
        const QList<PathPoint> part = points.mid(begin, end - begin);
        if (std::any_of(part.begin(), part.end(), [](const PathPoint& p) { return p.hasHandles(); })) {
            out += part;
            continue;
        }
        QList<QPointF> at;
        for (const PathPoint& p : part)
            at.append(p.p);
        at.append(at.first()); // closed: the first again, so the last stretch is weighed too
        QList<QPointF> kept = maskshapes::simplified(at, 0.5);
        kept.removeLast();
        for (qsizetype i = 0; i < kept.size(); ++i) {
            PathPoint p = PathPoint::corner(kept[i]);
            p.start = i == 0 && !out.isEmpty();
            out.append(p);
        }
    }
    return out;
}

} // namespace

QList<MaskShape> combined(const QList<MaskShape>& shapes, const QList<int>& indices, Op op, int* at) {
    if (at) *at = -1;
    QList<int> order;
    for (int i : indices)
        if (i >= 0 && i < shapes.size() && !order.contains(i)) order.append(i);
    std::sort(order.begin(), order.end());
    if (order.size() < 2) return shapes;
    QList<QPainterPath> paths;
    for (int i : order)
        paths.append(shapes[i].path());
    const QPainterPath result = combinedPath(paths, op);
    MaskShape s = shapes[order.first()];
    s.kind = MaskShape::Kind::Path;
    s.points = fewerAnchors(MaskShape::anchorsOf(result));
    if (s.points.size() < 3) return shapes;
    s.rect = QRectF(); // a custom shape is its anchors (bounds() from its outline)
    s.radius = 0;
    QList<MaskShape> out;
    for (qsizetype i = 0; i < shapes.size(); ++i) {
        if (i == order.first()) {
            s.name = maskshapes::nextName(out + shapes.mid(i + 1), MaskShape::Kind::Path);
            if (at) *at = int(out.size());
            out.append(s);
        } else if (!order.contains(int(i))) {
            out.append(shapes[i]);
        }
    }
    return out;
}

} // namespace avloop::shapecombine

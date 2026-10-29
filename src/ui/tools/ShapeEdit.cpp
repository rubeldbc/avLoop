#include "ui/tools/ShapeEdit.h"

#include <QStringList>

#include <algorithm>
#include <cmath>

namespace avloop::shapeedit {

int hitShape(const QList<MaskShape>& shapes, const QPointF& scene) {
    for (qsizetype i = shapes.size() - 1; i >= 0; --i) {
        const MaskShape& s = shapes[i];
        if (s.visible && !s.locked && s.path().contains(scene)) return int(i);
    }
    return -1;
}

int hitAnchor(const MaskShape& shape, const QTransform& toView, const QPointF& view, double reach) {
    if (shape.isStandard()) return -1;
    for (qsizetype i = shape.points.size() - 1; i >= 0; --i) {
        const QPointF at = toView.map(shape.points[i].p);
        if (std::abs(at.x() - view.x()) <= reach && std::abs(at.y() - view.y()) <= reach) return int(i);
    }
    return -1;
}

MaskShape standard(ShapeDraft::Kind kind, MaskShape::Role role, const QRectF& rect) {
    MaskShape s;
    s.kind = kind == ShapeDraft::Kind::Ellipse   ? MaskShape::Kind::Ellipse
             : kind == ShapeDraft::Kind::Rounded ? MaskShape::Kind::Rounded
                                                 : MaskShape::Kind::Rect;
    s.role = role;
    s.rect = rect.normalized();
    if (s.kind == MaskShape::Kind::Rounded) s.radius = std::min(s.rect.width(), s.rect.height()) / 6;
    return s;
}

MaskShape polygon(const QList<QPointF>& points, MaskShape::Role role) {
    MaskShape s;
    s.kind = MaskShape::Kind::Polygon;
    s.role = role;
    for (const QPointF& p : points)
        s.points.append(PathPoint::corner(p));
    return s;
}

QString summary(const QList<MaskShape>& shapes) {
    QStringList parts;
    for (const MaskShape& s : shapes) {
        const QString head = MaskShape::kindId(s.kind) + QLatin1Char(':') + MaskShape::roleId(s.role);
        if (s.isStandard())
            parts << QStringLiteral("%1:%2,%3,%4,%5")
                         .arg(head)
                         .arg(std::lround(s.rect.x()))
                         .arg(std::lround(s.rect.y()))
                         .arg(std::lround(s.rect.width()))
                         .arg(std::lround(s.rect.height()));
        else
            parts << QStringLiteral("%1:n%2").arg(head).arg(s.points.size());
    }
    return parts.join(QLatin1Char(';'));
}

QList<MaskShape> removed(const QList<MaskShape>& shapes, const QList<int>& indices) {
    QList<MaskShape> out;
    for (qsizetype i = 0; i < shapes.size(); ++i)
        if (!indices.contains(int(i))) out.append(shapes[i]);
    return out;
}

QList<MaskShape> moved(const QList<MaskShape>& shapes, const QList<int>& indices, const QPointF& d) {
    QList<MaskShape> out = shapes;
    for (int i : indices)
        if (i >= 0 && i < out.size() && !out[i].locked) out[i].translate(d);
    return out;
}

} // namespace avloop::shapeedit

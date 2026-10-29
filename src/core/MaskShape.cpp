#include "core/MaskShape.h"

#include <QCoreApplication>
#include <QJsonArray>

#include <algorithm>
#include <cmath>

namespace avloop {

QPainterPath MaskShape::path() const {
    QPainterPath path;
    path.setFillRule(Qt::WindingFill);
    switch (kind) {
    case Kind::Rect:
        path.addRect(rect);
        break;
    case Kind::Rounded: {
        const double r = std::clamp(radius, 0.0, std::min(rect.width(), rect.height()) / 2);
        path.addRoundedRect(rect, r, r);
        break;
    }
    case Kind::Ellipse:
        path.addEllipse(rect);
        break;
    case Kind::Polygon:
    case Kind::Path: {
        if (points.size() < 2) break;
        path.moveTo(points.first().p);
        // Each segment a line, or a cubic where either end has a handle; the last joins the first (closed).
        for (qsizetype i = 1; i <= points.size(); ++i) {
            const PathPoint& a = points[i - 1];
            const PathPoint& b = points[i % points.size()];
            if (a.out == a.p && b.in == b.p)
                path.lineTo(b.p);
            else
                path.cubicTo(a.out, b.in, b.p);
        }
        path.closeSubpath();
        break;
    }
    }
    return path;
}

QRectF MaskShape::bounds() const {
    return isStandard() ? rect.normalized() : path().boundingRect();
}

void MaskShape::translate(const QPointF& d) {
    rect.translate(d);
    for (PathPoint& pt : points) {
        pt.p += d;
        pt.in += d;
        pt.out += d;
    }
}

MaskShape MaskShape::clamped() const {
    MaskShape s = *this;
    s.rect = rect.normalized();
    s.radius = std::clamp(radius, 0.0, std::min(s.rect.width(), s.rect.height()) / 2);
    s.feather = std::clamp(feather, 0.0, kMaxFeather);
    s.strength = std::clamp(strength, 0.0, 100.0);
    return s;
}

QJsonObject MaskShape::toJson() const {
    QJsonObject json{{QStringLiteral("kind"), kindId(kind)}, {QStringLiteral("role"), roleId(role)},
                     {QStringLiteral("name"), name},         {QStringLiteral("visible"), visible},
                     {QStringLiteral("locked"), locked},     {QStringLiteral("feather"), feather},
                     {QStringLiteral("strength"), strength}};
    if (isStandard()) {
        json.insert(QStringLiteral("x"), rect.x());
        json.insert(QStringLiteral("y"), rect.y());
        json.insert(QStringLiteral("w"), rect.width());
        json.insert(QStringLiteral("h"), rect.height());
        json.insert(QStringLiteral("radius"), radius);
        return json;
    }
    // An anchor: [x, y], or [x, y, in x, in y, out x, out y, smooth] when it has handles.
    QJsonArray list;
    for (const PathPoint& pt : points) {
        QJsonArray a{pt.p.x(), pt.p.y()};
        if (pt.hasHandles() || pt.smooth)
            for (QJsonValue v : {QJsonValue(pt.in.x()), QJsonValue(pt.in.y()), QJsonValue(pt.out.x()),
                                 QJsonValue(pt.out.y()), QJsonValue(pt.smooth ? 1 : 0)})
                a.append(v);
        list.append(a);
    }
    json.insert(QStringLiteral("points"), list);
    return json;
}

MaskShape MaskShape::fromJson(const QJsonObject& json) {
    MaskShape s;
    s.kind = kindFromId(json.value(QStringLiteral("kind")).toString());
    s.role = roleFromId(json.value(QStringLiteral("role")).toString());
    s.name = json.value(QStringLiteral("name")).toString();
    s.visible = json.value(QStringLiteral("visible")).toBool(true);
    s.locked = json.value(QStringLiteral("locked")).toBool(false);
    s.feather = json.value(QStringLiteral("feather")).toDouble(0);
    s.strength = json.value(QStringLiteral("strength")).toDouble(100);
    s.rect = QRectF(json.value(QStringLiteral("x")).toDouble(), json.value(QStringLiteral("y")).toDouble(),
                    json.value(QStringLiteral("w")).toDouble(), json.value(QStringLiteral("h")).toDouble());
    s.radius = json.value(QStringLiteral("radius")).toDouble();
    for (const QJsonValue& v : json.value(QStringLiteral("points")).toArray()) {
        const QJsonArray a = v.toArray();
        PathPoint pt = PathPoint::corner(QPointF(a.at(0).toDouble(), a.at(1).toDouble()));
        if (a.size() >= 7) {
            pt.in = QPointF(a.at(2).toDouble(), a.at(3).toDouble());
            pt.out = QPointF(a.at(4).toDouble(), a.at(5).toDouble());
            pt.smooth = a.at(6).toInt() != 0;
        }
        s.points.append(pt);
    }
    return s.clamped();
}

QString MaskShape::kindId(Kind k) {
    switch (k) {
    case Kind::Rounded:
        return QStringLiteral("rounded");
    case Kind::Ellipse:
        return QStringLiteral("ellipse");
    case Kind::Polygon:
        return QStringLiteral("polygon");
    case Kind::Path:
        return QStringLiteral("path");
    case Kind::Rect:
        break;
    }
    return QStringLiteral("rect");
}

MaskShape::Kind MaskShape::kindFromId(const QString& id) {
    for (Kind k : {Kind::Rect, Kind::Rounded, Kind::Ellipse, Kind::Polygon, Kind::Path})
        if (kindId(k) == id) return k;
    return Kind::Rect;
}

QString MaskShape::roleId(Role r) {
    switch (r) {
    case Role::Cut:
        return QStringLiteral("cut");
    case Role::KeepOnly:
        return QStringLiteral("keep");
    case Role::Exclude:
        return QStringLiteral("exclude");
    case Role::Add:
        break;
    }
    return QStringLiteral("add");
}

MaskShape::Role MaskShape::roleFromId(const QString& id) {
    for (Role r : {Role::Add, Role::Cut, Role::KeepOnly, Role::Exclude})
        if (roleId(r) == id) return r;
    return Role::Add;
}

namespace maskshapes {

double start(const QList<MaskShape>& shapes) {
    return std::any_of(shapes.begin(), shapes.end(),
                       [](const MaskShape& s) { return s.visible && s.role == MaskShape::Role::Add; })
               ? 0.0
               : 1.0;
}

bool anyVisible(const QList<MaskShape>& shapes) {
    return std::any_of(shapes.begin(), shapes.end(), [](const MaskShape& s) { return s.visible; });
}

namespace {

double distanceToSegment(const QPointF& p, const QPointF& a, const QPointF& b) {
    const QPointF ab = b - a;
    const double len2 = QPointF::dotProduct(ab, ab);
    const double t = len2 > 0 ? std::clamp(QPointF::dotProduct(p - a, ab) / len2, 0.0, 1.0) : 0.0;
    const QPointF q = a + ab * t;
    return std::hypot(p.x() - q.x(), p.y() - q.y());
}

void reduce(const QList<QPointF>& in, qsizetype from, qsizetype to, double tolerance, QList<bool>& keep) {
    if (to <= from + 1) return;
    double worst = -1;
    qsizetype at = from;
    for (qsizetype i = from + 1; i < to; ++i)
        if (const double d = distanceToSegment(in[i], in[from], in[to]); d > worst) {
            worst = d;
            at = i;
        }
    if (worst <= tolerance) return;
    keep[at] = true;
    reduce(in, from, at, tolerance, keep);
    reduce(in, at, to, tolerance, keep);
}

} // namespace

QList<QPointF> simplified(const QList<QPointF>& points, double tolerance) {
    if (points.size() < 3) return points;
    QList<bool> keep(points.size(), false);
    keep.first() = keep.last() = true;
    reduce(points, 0, points.size() - 1, tolerance, keep);
    QList<QPointF> out;
    for (qsizetype i = 0; i < points.size(); ++i)
        if (keep[i]) out.append(points[i]);
    return out;
}

QString nextName(const QList<MaskShape>& shapes, MaskShape::Kind kind) {
    const char* words[] = {QT_TRANSLATE_NOOP("MaskShape", "Rectangle"), QT_TRANSLATE_NOOP("MaskShape", "Rounded"),
                           QT_TRANSLATE_NOOP("MaskShape", "Ellipse"), QT_TRANSLATE_NOOP("MaskShape", "Polygon"),
                           QT_TRANSLATE_NOOP("MaskShape", "Path")};
    const QString word = QCoreApplication::translate("MaskShape", words[int(kind)]);
    for (int n = 1;; ++n) {
        const QString name = QStringLiteral("%1 %2").arg(word).arg(n);
        if (std::none_of(shapes.begin(), shapes.end(), [&](const MaskShape& s) { return s.name == name; })) return name;
    }
}

} // namespace maskshapes

} // namespace avloop

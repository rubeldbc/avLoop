#include "core/ShapeAlign.h"

#include <QCoreApplication>

#include <algorithm>

namespace avloop::shapealign {

namespace {

QList<int> valid(const QList<MaskShape>& shapes, const QList<int>& indices) {
    QList<int> out;
    for (int i : indices)
        if (i >= 0 && i < shapes.size() && !out.contains(i)) out.append(i);
    return out;
}

} // namespace

QRectF selectionBounds(const QList<MaskShape>& shapes, const QList<int>& indices) {
    QRectF box;
    for (int i : valid(shapes, indices))
        box = box.isNull() ? shapes[i].bounds() : box.united(shapes[i].bounds());
    return box;
}

QList<MaskShape> aligned(const QList<MaskShape>& shapes, const QList<int>& indices, Align how, const QRectF& to) {
    const QList<int> chosen = valid(shapes, indices);
    const QRectF target = to.isEmpty() ? selectionBounds(shapes, chosen) : to;
    QList<MaskShape> out = shapes;
    if (target.isNull()) return out;
    for (int i : chosen) {
        if (out[i].locked) continue;
        const QRectF b = out[i].bounds();
        QPointF d;
        switch (how) {
        case Align::Left:
            d.setX(target.left() - b.left());
            break;
        case Align::Centre:
            d.setX(target.center().x() - b.center().x());
            break;
        case Align::Right:
            d.setX(target.right() - b.right());
            break;
        case Align::Top:
            d.setY(target.top() - b.top());
            break;
        case Align::Middle:
            d.setY(target.center().y() - b.center().y());
            break;
        case Align::Bottom:
            d.setY(target.bottom() - b.bottom());
            break;
        }
        out[i].translate(d);
    }
    return out;
}

QList<MaskShape> distributed(const QList<MaskShape>& shapes, const QList<int>& indices, Spread how, const QRectF& to) {
    QList<int> chosen = valid(shapes, indices);
    QList<MaskShape> out = shapes;
    if (chosen.size() < 3) return out;
    const bool across = how == Spread::Horizontal;
    auto lead = [&](int i) { return across ? out[i].bounds().left() : out[i].bounds().top(); };
    auto size = [&](int i) { return across ? out[i].bounds().width() : out[i].bounds().height(); };
    std::sort(chosen.begin(), chosen.end(), [&](int a, int b) { return lead(a) < lead(b); });
    auto place = [&](int i, double at) {
        if (out[i].locked) return;
        const double d = at - lead(i);
        out[i].translate(across ? QPointF(d, 0) : QPointF(0, d));
    };
    if (!to.isEmpty()) {
        place(chosen.first(), across ? to.left() : to.top());
        place(chosen.last(), (across ? to.right() : to.bottom()) - size(chosen.last()));
    }
    const double start = lead(chosen.first());
    const double end = lead(chosen.last()) + size(chosen.last());
    double total = 0;
    for (int i : chosen)
        total += size(i);
    const double gap = (end - start - total) / double(chosen.size() - 1);
    double at = start + size(chosen.first()) + gap;
    for (qsizetype k = 1; k + 1 < chosen.size(); ++k) {
        place(chosen[k], at);
        at += size(chosen[k]) + gap;
    }
    return out;
}

QList<MaskShape> moved(const QList<MaskShape>& shapes, int from, int to) {
    QList<MaskShape> out = shapes;
    if (from < 0 || from >= out.size() || to < 0 || to >= out.size() || from == to) return out;
    out.move(from, to);
    return out;
}

QList<MaskShape> duplicated(const QList<MaskShape>& shapes, int index, const QPointF& offset) {
    QList<MaskShape> out = shapes;
    if (index < 0 || index >= out.size() || out.size() >= MaskShape::kMaxShapes) return out;
    MaskShape copy = out[index];
    copy.translate(offset);
    copy.locked = false;
    copy.name = QCoreApplication::translate("MaskShape", "%1 copy").arg(out[index].name);
    out.insert(index + 1, copy);
    return out;
}

} // namespace avloop::shapealign

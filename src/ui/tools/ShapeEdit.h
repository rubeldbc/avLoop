#pragma once

#include "core/MaskShape.h"

#include <QList>
#include <QPointF>
#include <QString>
#include <QTransform>

namespace avloop {

// What the Shape tool draws next (its options bar): the kind — a standard one, a polygon of clicks or a freehand
// outline — and the role a new shape takes (D-022).
struct ShapeDraft {
    enum class Kind { Rect, Rounded, Ellipse, Polygon, Freehand };
    Kind kind = Kind::Rect;
    MaskShape::Role role = MaskShape::Role::Add;
};

// The Shape and Pen tools' geometry, without widgets (scene px unless said).
namespace shapeedit {

constexpr double kMinSide = 16; // a smaller drawing is cancelled (as the crop's, spec §12.2)

// The topmost visible, unlocked shape whose inside holds `scene`, else −1.
int hitShape(const QList<MaskShape>& shapes, const QPointF& scene);
// The anchor of a custom shape within `reach` view px of `view`, else −1.
int hitAnchor(const MaskShape& shape, const QTransform& toView, const QPointF& view, double reach = 6);
// A standard shape over `rect`; a rounded one's radius a sixth of its short side.
MaskShape standard(ShapeDraft::Kind kind, MaskShape::Role role, const QRectF& rect);
// A custom shape of corner anchors (a polygon, a simplified freehand outline).
MaskShape polygon(const QList<QPointF>& points, MaskShape::Role role);
// What walks read: "kind:role:x,y,w,h" a shape (custom: "kind:role:nN" with its anchor count), ";" between.
QString summary(const QList<MaskShape>& shapes);
// The shapes with `indices` removed (Delete), and with them moved by `d` (a drag, the arrows).
QList<MaskShape> removed(const QList<MaskShape>& shapes, const QList<int>& indices);
QList<MaskShape> moved(const QList<MaskShape>& shapes, const QList<int>& indices, const QPointF& d);

} // namespace shapeedit

} // namespace avloop

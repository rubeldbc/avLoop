#pragma once

#include <QList>
#include <QPointF>
#include <QRectF>

namespace avloop::crophandles {

// The crop's eight handles (spec §6.1) and the rectangle maths of drawing, resizing and snapping, in scene px.
enum class Handle { None, TopLeft, Top, TopRight, Right, BottomRight, Bottom, BottomLeft, Left };

QList<Handle> all();
QPointF position(const QRectF& rect, Handle h);
// The handle under a view point (within `reach` px of its centre), given the crop's rectangle in view px.
Handle hit(const QRectF& viewRect, const QPointF& view, double reach = 6);
Qt::CursorShape cursor(Handle h);

// A rectangle drawn from `anchor` to `pointer`: `ratio` > 0 keeps w/h; `fromCentre` makes the anchor its centre.
QRectF drawn(const QPointF& anchor, const QPointF& pointer, double ratio, bool fromCentre);
// `start` with one handle moved by `delta`: `ratio` > 0 keeps w/h (about the opposite side or corner, or the centre);
// `fromCentre` moves the opposite side by the same amount.
QRectF resized(const QRectF& start, Handle h, const QPointF& delta, double ratio, bool fromCentre);

// Snapping (spec §6.1): `value` moved to the nearest target within `tolerance`; `line` says which target held.
double snap(double value, const QList<double>& targets, double tolerance, double* line = nullptr);
// The video's edges and centre on one axis: the targets a crop snaps to.
QList<double> targetsX(const QRectF& video);
QList<double> targetsY(const QRectF& video);

} // namespace avloop::crophandles

#pragma once

#include "core/MaskShape.h"

#include <QPointF>
#include <QTransform>

namespace avloop {

// The Pen's geometry (D-022, AV-025), without widgets: a path's cubic segments — segment i runs from anchor i to anchor
// i + 1 (the last back to the first) by anchor i's out handle and anchor i + 1's in handle.
namespace pathedit {

// The point at `t` (0 … 1) of segment i.
QPointF at(const MaskShape& shape, int segment, double t);
// The segment within `reach` view px of `view` and where on it, else segment −1.
struct SegmentHit {
    int segment = -1;
    double t = 0;
};
SegmentHit hitSegment(const MaskShape& shape, const QTransform& toView, const QPointF& view, double reach = 5);
// A handle within `reach` view px (only handles apart from their anchor): its anchor, else −1; `out` which one.
int hitHandle(const MaskShape& shape, const QTransform& toView, const QPointF& view, bool* out, double reach = 5);
// An anchor added on segment i at `t` (de Casteljau): the outline unchanged, the new anchor smooth on a curve and a
// corner on a straight segment. Returns the new anchor's index through `index`.
MaskShape inserted(const MaskShape& shape, int segment, double t, int* index = nullptr);
// The anchor removed; the path keeps at least two anchors (fewer: unchanged).
MaskShape removed(const MaskShape& shape, int anchor);
// Smooth ↔ corner (Alt+click): a corner gets handles along the line from its previous to its next anchor, each a third
// of the way to that neighbour; a smooth anchor or one with handles loses them.
MaskShape converted(const MaskShape& shape, int anchor);
// A handle moved to `to`: a smooth anchor turns its other handle opposite, keeping that one's length (its own length
// when it had none); `breakPair` (Alt) makes the anchor a corner with two free handles.
void moveHandle(PathPoint& pt, bool out, const QPointF& to, bool breakPair);
// "c" a corner without handles, "s" a smooth anchor, "b" a corner with handles, "," between (what walks read).
QString kinds(const MaskShape& shape);
// A polygon or freehand outline the Pen edits becomes a Path (R-86).
MaskShape asPath(const MaskShape& shape);

} // namespace pathedit

} // namespace avloop

#pragma once

#include "core/MaskShape.h"

#include <QList>
#include <QRectF>

namespace avloop {

// Illustrator's Align for the shapes (D-026), pure: the selected shapes moved — never resized — by their bounding
// boxes.
namespace shapealign {

enum class Align { Left, Centre, Right, Top, Middle, Bottom };
enum class Spread { Horizontal, Vertical };

// The selected shapes' boxes joined (what "To: Selection" aligns to).
QRectF selectionBounds(const QList<MaskShape>& shapes, const QList<int>& indices);
// Each selected, unlocked shape's edge or centre put on the target's: `to` the frame's rectangle, or empty for the
// selection's own box.
QList<MaskShape> aligned(const QList<MaskShape>& shapes, const QList<int>& indices, Align how, const QRectF& to = {});
// Three or more: the first and last (by their left, or top) stay, those between placed so the gaps between boxes are
// equal; `to` a frame: the first and last put on its edges first.
QList<MaskShape> distributed(const QList<MaskShape>& shapes, const QList<int>& indices, Spread how,
                             const QRectF& to = {});

// The layers (D-026): shape `from` moved to place `to` (the list's order is the mask's, bottom to top).
QList<MaskShape> moved(const QList<MaskShape>& shapes, int from, int to);
// A copy of shape `index` above it, offset by `offset` scene px, named "<name> copy".
QList<MaskShape> duplicated(const QList<MaskShape>& shapes, int index, const QPointF& offset = {10, 10});

} // namespace shapealign

} // namespace avloop

#pragma once

#include "core/MaskShape.h"

#include <QList>
#include <QPainterPath>

namespace avloop {

// Illustrator's Pathfinder for the shapes (D-023): the selected shapes baked into one Path shape.
namespace shapecombine {

enum class Op { Unite, MinusFront, Intersect, Exclude };

// The outlines joined in their order: Unite all; Minus front — the lowest less everything above it; Intersect where
// all are; Exclude where an odd number are (pairwise: united less intersected).
QPainterPath combinedPath(const QList<QPainterPath>& paths, Op op);
// The shapes at `indices` (two or more) replaced by one Path shape at the lowest one's place, with its role, name
// "Path N" and its anchors from the combined outline (several outlines when the result has them); `at` its index.
// Fewer than two, or an empty result: the shapes unchanged and `at` −1.
QList<MaskShape> combined(const QList<MaskShape>& shapes, const QList<int>& indices, Op op, int* at = nullptr);

} // namespace shapecombine

} // namespace avloop

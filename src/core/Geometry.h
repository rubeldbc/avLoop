#pragma once

#include "core/VideoTransform.h"

#include <QRectF>
#include <QSize>
#include <QSizeF>

namespace avloop {

// Scene-space geometry shared by the canvas, the tools and the renderer (spec §3.3).
namespace geometry {

constexpr int kMaxLongSide = 1920; // no output above 1080p (the owner's decision 6)
constexpr int kMinSide = 16;

// The output size of a frame (spec §3.3): its size, scaled down so the long side is at most 1920 keeping the ratio,
// both sides to the nearest even number, at least 16.
QSize outputSize(const QSizeF& frame);

// What is exported with no crop: the whole video rectangle (reading R-8).
QRectF exportFrame(const QRectF& crop, const VideoTransform& t, const QSizeF& video);

// The effective scale of the source in the output: video scale × output / frame (spec §6.2's upscale warning).
double effectiveScale(const VideoTransform& t, const QRectF& frame);

// The four corners of a rectangle, clockwise from the top-left.
enum class Corner { TopLeft, TopRight, BottomRight, BottomLeft };
QPointF corner(const QRectF& r, Corner c);
Corner opposite(Corner c);

} // namespace geometry
} // namespace avloop

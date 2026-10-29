#pragma once

#include "core/CropShape.h"
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
// The crop's frame: its rectangle, or the video's with no crop or an inverted one (a hole in the whole video, R-79).
QRectF exportFrame(const CropShape& crop, const VideoTransform& t, const QSizeF& video);

// Whether the output needs an alpha channel (spec §6.1, Alpha Auto): a source with alpha, a rounded or elliptic crop,
// a feather, or a rectangle reaching past the video.
bool needsAlpha(const CropShape& crop, const VideoTransform& t, const QSizeF& video, bool sourceAlpha);

// How much of the crop the video covers: Full, Partly (transparent areas), None (fully transparent).
enum class Coverage { Full, Partly, None };
Coverage coverage(const CropShape& crop, const VideoTransform& t, const QSizeF& video);

// The effective scale of the source in the output: video scale × output / frame (spec §6.2's upscale warning).
double effectiveScale(const VideoTransform& t, const QRectF& frame);

// The four corners of a rectangle, clockwise from the top-left.
enum class Corner { TopLeft, TopRight, BottomRight, BottomLeft };
QPointF corner(const QRectF& r, Corner c);
Corner opposite(Corner c);

} // namespace geometry
} // namespace avloop

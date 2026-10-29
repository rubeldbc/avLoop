#include "core/Geometry.h"

#include <algorithm>
#include <cmath>

namespace avloop::geometry {

namespace {

int evenRound(double v) {
    const int n = int(std::lround(v / 2.0)) * 2;
    return std::max(kMinSide, n);
}

} // namespace

QSize outputSize(const QSizeF& frame) {
    if (frame.width() <= 0 || frame.height() <= 0) return {kMinSide, kMinSide};
    double w = frame.width();
    double h = frame.height();
    const double longSide = std::max(w, h);
    if (longSide > kMaxLongSide) {
        const double k = kMaxLongSide / longSide;
        w *= k;
        h *= k;
    }
    return {std::min(evenRound(w), kMaxLongSide), std::min(evenRound(h), kMaxLongSide)};
}

QRectF exportFrame(const QRectF& crop, const VideoTransform& t, const QSizeF& video) {
    return crop.isEmpty() ? t.rect(video) : crop;
}

QRectF exportFrame(const CropShape& crop, const VideoTransform& t, const QSizeF& video) {
    return exportFrame(crop.invert ? QRectF() : crop.rect, t, video);
}

double effectiveScale(const VideoTransform& t, const QRectF& frame) {
    if (frame.isEmpty()) return t.scale;
    const QSize out = outputSize(frame.size());
    return t.scale * double(out.width()) / frame.width();
}

bool needsAlpha(const CropShape& crop, const VideoTransform& t, const QSizeF& video, bool sourceAlpha) {
    if (sourceAlpha) return true;
    if (maskshapes::anyVisible(crop.shapes)) return true; // shapes select or cut inside the frame (D-022)
    if (crop.isEmpty()) return false;                     // the whole video rectangle
    if (crop.invert) return true;                         // a hole is clear
    if (crop.kind != CropShape::Kind::Rect || crop.feather > 0) return true;
    return coverage(crop, t, video) != Coverage::Full;
}

Coverage coverage(const CropShape& crop, const VideoTransform& t, const QSizeF& video) {
    if (crop.isEmpty()) return Coverage::Full;
    const QRectF v = t.rect(video);
    if (crop.invert) return v.intersects(crop.rect) ? Coverage::Partly : Coverage::Full; // the hole in the video
    if (v.contains(crop.rect)) return Coverage::Full;
    return v.intersects(crop.rect) ? Coverage::Partly : Coverage::None;
}

QPointF corner(const QRectF& r, Corner c) {
    switch (c) {
    case Corner::TopLeft:
        return r.topLeft();
    case Corner::TopRight:
        return r.topRight();
    case Corner::BottomRight:
        return r.bottomRight();
    case Corner::BottomLeft:
        return r.bottomLeft();
    }
    return r.topLeft();
}

Corner opposite(Corner c) {
    switch (c) {
    case Corner::TopLeft:
        return Corner::BottomRight;
    case Corner::TopRight:
        return Corner::BottomLeft;
    case Corner::BottomRight:
        return Corner::TopLeft;
    case Corner::BottomLeft:
        return Corner::TopRight;
    }
    return Corner::BottomRight;
}

} // namespace avloop::geometry

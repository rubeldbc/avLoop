#include "core/VideoTransform.h"

#include <algorithm>

namespace avloop {

QRectF VideoTransform::rect(const QSizeF& video) const {
    const double w = video.width() * scale;
    const double h = video.height() * scale;
    return {x - w / 2, y - h / 2, w, h};
}

VideoTransform VideoTransform::clamped() const {
    VideoTransform t = *this;
    t.scale = std::clamp(scale, kMinScale, kMaxScale);
    return t;
}

VideoTransform VideoTransform::initial(const QSizeF& video) {
    VideoTransform t;
    t.x = video.width() / 2;
    t.y = video.height() / 2;
    return t;
}

VideoTransform VideoTransform::fittedTo(const QRectF& frame, const QSizeF& video) const {
    if (video.isEmpty() || frame.isEmpty()) return *this;
    VideoTransform t = *this;
    t.scale = std::min(frame.width() / video.width(), frame.height() / video.height());
    t.x = frame.center().x();
    t.y = frame.center().y();
    return t.clamped();
}

VideoTransform VideoTransform::filling(const QRectF& frame, const QSizeF& video) const {
    if (video.isEmpty() || frame.isEmpty()) return *this;
    VideoTransform t = *this;
    t.scale = std::max(frame.width() / video.width(), frame.height() / video.height());
    t.x = frame.center().x();
    t.y = frame.center().y();
    return t.clamped();
}

VideoTransform VideoTransform::centredOn(const QRectF& frame) const {
    VideoTransform t = *this;
    t.x = frame.center().x();
    t.y = frame.center().y();
    return t;
}

} // namespace avloop

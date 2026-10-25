#pragma once

#include <QPointF>
#include <QRectF>
#include <QSizeF>

namespace avloop {

// Where the source video stands in scene space (spec §3.3): its centre, one scale (the ratio never changes, R-2) and
// two flips. The video rectangle = centre ± (w·scale/2, h·scale/2).
struct VideoTransform {
    static constexpr double kMinScale = 0.05;
    static constexpr double kMaxScale = 10.0;

    double x = 0;
    double y = 0;
    double scale = 1.0;
    bool flipH = false;
    bool flipV = false;

    QPointF centre() const { return {x, y}; }
    QRectF rect(const QSizeF& video) const;
    VideoTransform clamped() const;

    // On load and on Reset: 100%, the video's top-left at (0,0), no flips.
    static VideoTransform initial(const QSizeF& video);
    // The whole video inside `frame`, centred (Fit); `frame` completely covered, centred (Fill); centred on `frame`
    // at the same scale (Center). Flips are kept.
    VideoTransform fittedTo(const QRectF& frame, const QSizeF& video) const;
    VideoTransform filling(const QRectF& frame, const QSizeF& video) const;
    VideoTransform centredOn(const QRectF& frame) const;

    friend bool operator==(const VideoTransform& a, const VideoTransform& b) = default;
};

} // namespace avloop

#pragma once

#include "core/TimeRange.h"
#include "core/VideoTransform.h"

#include <QJsonObject>
#include <QList>
#include <QSizeF>
#include <QString>

namespace avloop {

// A video on a track above the main one (D-027): its file, where it starts on the main video's time (the sequence), the
// part of it used (In/Out in its own seconds), its place and scale in scene space, its opacity, shown or hidden,
// locked. Track 2 is just above the main video (track 1); a higher track is drawn later, over the lower. At most 8
// layers (AVADR-0002).
struct VideoLayer {
    static constexpr int kMaxLayers = 8;

    QString name;
    QString path;
    QSizeF size;         // its display size, px
    double duration = 0; // its length, s
    int track = 2;       // 2 … 9
    double start = 0;    // sequence seconds where its In shows
    TimeRange range;     // In/Out in its own seconds
    VideoTransform transform;
    double opacity = 100; // %
    bool visible = true;
    bool locked = false;

    double length() const { return range.out - range.in; }
    double end() const { return start + length(); }
    // Its own time at sequence time `t`; false when it does not show then.
    bool sourceTime(double t, double* own) const;
    // Sequence seconds → its rectangle in scene px (the transform does not move with time).
    QRectF rect() const { return transform.rect(size); }

    QJsonObject toJson() const;
    static VideoLayer fromJson(const QJsonObject& json);
    friend bool operator==(const VideoLayer&, const VideoLayer&) = default;
};

namespace videolayers {

// The layers drawn in order: by track, then as listed (a stable sort).
QList<int> drawOrder(const QList<VideoLayer>& layers);
// The topmost shown, unlocked layer whose rectangle holds `scene` at time `t`, else −1.
int hitLayer(const QList<VideoLayer>& layers, const QPointF& scene, double t);
// Whether a layer could sit on `track` from `start` for `length` without overlapping another on that track (`except`
// the layer being moved).
bool fits(const QList<VideoLayer>& layers, int track, double start, double length, int except = -1);

} // namespace videolayers

} // namespace avloop

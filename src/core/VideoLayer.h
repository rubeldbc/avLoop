#pragma once

#include "core/ClipEffects.h"
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
// A layer's keyframe (D-029): its place, scale and opacity at a time of its own; Ease eases into the next.
struct LayerKey {
    double t = 0; // the layer's own seconds
    double x = 0;
    double y = 0;
    double scale = 1;
    double opacity = 100;
    bool ease = false;
    friend bool operator==(const LayerKey&, const LayerKey&) = default;
};

struct VideoLayer {
    static constexpr int kMaxLayers = 8;
    static constexpr int kMaxKeys = 256;

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
    QList<LayerKey> keys; // sorted by t; none: the transform and opacity above hold (D-029)
    ClipEffects effects;  // its own clip effects, applied to each of its frames (D-029)

    // Its place, scale and opacity at its own time `own`: keyed (between two keys linear, or eased when the first
    // eases), else the transform and opacity.
    struct Look {
        VideoTransform transform;
        double opacity = 100;
    };
    Look at(double own) const;
    // The key within `tolerance` s of `own`, else −1.
    int keyAt(double own, double tolerance) const;
    // `look` written at `own`: into the key there (made when none) when keyed, else into the transform and opacity.
    void setLook(double own, const Look& look, double tolerance);

    double length() const { return range.out - range.in; }
    double end() const { return start + length(); }
    // Its own time at sequence time `t`; false when it does not show then.
    bool sourceTime(double t, double* own) const;
    // Its rectangle in scene px: unkeyed, and at its own time `own`.
    QRectF rect() const { return transform.rect(size); }
    QRectF rectAt(double own) const { return at(own).transform.rect(size); }

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

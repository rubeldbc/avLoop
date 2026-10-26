#pragma once

#include "core/VideoTransform.h"

#include <QList>

namespace avloop {

// One keyframe of the video's position and scale (spec §6.4, §11.1 "transform.keyframes"): a source time, a centre,
// a scale, and how it goes on to the next one — Linear, or Ease (smoothstep). Flips are not keyframed: they hold
// (reading R-47).
struct Keyframe {
    double t = 0; // seconds of source time
    double x = 0;
    double y = 0;
    double scale = 1.0;
    bool ease = false;

    friend bool operator==(const Keyframe&, const Keyframe&) = default;
};

using Keyframes = QList<Keyframe>; // sorted by time, at most one a frame

namespace keyframes {

// The transform at source time `t` (spec §6.4, §12.2): no keyframes — `base` itself; before the first — the first;
// after the last — the last; between k and k + 1 — their mix at u = (t − tk) / (tk+1 − tk), eased by u²(3 − 2u)
// when keyframe k is Ease. Keyframes outside In–Out count like any other. The flips are always `base`'s.
VideoTransform valueAt(const Keyframes& keys, const VideoTransform& base, double t);
double ease(double u); // smoothstep

// The keyframe within `tolerance` of `t` (half a frame), else −1.
int indexAt(const Keyframes& keys, double t, double tolerance);
// `key` put in its place: one within `tolerance` of its time is replaced, else it is inserted in order.
Keyframes withKey(Keyframes keys, const Keyframe& key, double tolerance);
// Keyframe `index` moved to time `t`; refused (unchanged) when another one is already within `tolerance` there.
Keyframes moved(Keyframes keys, int index, double t, double tolerance);
// The time of the keyframe before / after `t` (beyond `tolerance`); `t` itself when there is none.
double previous(const Keyframes& keys, double t, double tolerance);
double next(const Keyframes& keys, double t, double tolerance);
// The transform at each keyframe (or `base` alone): what the output must cover at every time between them, as the
// interpolation never leaves the hull of its ends (alpha, the working size).
QList<VideoTransform> extremes(const Keyframes& keys, const VideoTransform& base);

} // namespace keyframes
} // namespace avloop

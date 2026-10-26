#include "core/Keyframes.h"

#include <algorithm>
#include <cmath>

namespace avloop::keyframes {

namespace {

VideoTransform at(const Keyframe& k, const VideoTransform& base) {
    VideoTransform t = base;
    t.x = k.x;
    t.y = k.y;
    t.scale = k.scale;
    return t;
}

} // namespace

double ease(double u) {
    u = std::clamp(u, 0.0, 1.0);
    return u * u * (3 - 2 * u);
}

VideoTransform valueAt(const Keyframes& keys, const VideoTransform& base, double t) {
    if (keys.isEmpty()) return base;
    if (t <= keys.front().t) return at(keys.front(), base);
    if (t >= keys.back().t) return at(keys.back(), base);
    const auto after =
        std::upper_bound(keys.begin(), keys.end(), t, [](double v, const Keyframe& k) { return v < k.t; });
    const Keyframe& b = *after;
    const Keyframe& a = *(after - 1);
    double u = b.t > a.t ? (t - a.t) / (b.t - a.t) : 0.0;
    if (a.ease) u = ease(u);
    VideoTransform v = base;
    v.x = a.x + (b.x - a.x) * u;
    v.y = a.y + (b.y - a.y) * u;
    v.scale = a.scale + (b.scale - a.scale) * u;
    return v;
}

int indexAt(const Keyframes& keys, double t, double tolerance) {
    for (int i = 0; i < keys.size(); ++i)
        if (std::abs(keys[i].t - t) <= tolerance) return i;
    return -1;
}

Keyframes withKey(Keyframes keys, const Keyframe& key, double tolerance) {
    const int i = indexAt(keys, key.t, tolerance);
    if (i >= 0) {
        keys[i] = key;
        return keys;
    }
    const auto place =
        std::upper_bound(keys.begin(), keys.end(), key.t, [](double v, const Keyframe& k) { return v < k.t; });
    keys.insert(place, key);
    return keys;
}

Keyframes moved(Keyframes keys, int index, double t, double tolerance) {
    if (index < 0 || index >= keys.size()) return keys;
    for (int i = 0; i < keys.size(); ++i)
        if (i != index && std::abs(keys[i].t - t) <= tolerance) return keys;
    Keyframe k = keys.takeAt(index);
    k.t = t;
    return withKey(keys, k, 0);
}

Keyframes copied(Keyframes keys, int index, double t, double tolerance) {
    if (index < 0 || index >= keys.size() || indexAt(keys, t, tolerance) >= 0) return keys;
    Keyframe k = keys[index];
    k.t = t;
    return withKey(keys, k, 0);
}

double previous(const Keyframes& keys, double t, double tolerance) {
    for (auto it = keys.rbegin(); it != keys.rend(); ++it)
        if (it->t < t - tolerance) return it->t;
    return t;
}

double next(const Keyframes& keys, double t, double tolerance) {
    for (const Keyframe& k : keys)
        if (k.t > t + tolerance) return k.t;
    return t;
}

QList<VideoTransform> extremes(const Keyframes& keys, const VideoTransform& base) {
    if (keys.isEmpty()) return {base};
    QList<VideoTransform> list;
    for (const Keyframe& k : keys)
        list << at(k, base);
    return list;
}

} // namespace avloop::keyframes

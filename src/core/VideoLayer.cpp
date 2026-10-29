#include "core/VideoLayer.h"

#include <QJsonArray>

#include <algorithm>
#include <cmath>
#include <numeric>

namespace avloop {

bool VideoLayer::sourceTime(double t, double* own) const {
    if (t < start - 1e-9 || t >= end() - 1e-9) return false;
    if (own) *own = range.in + (t - start);
    return true;
}

VideoLayer::Look VideoLayer::at(double own) const {
    return videolayers::keyedLook(keys, transform, opacity, own);
}

VideoLayer::Look videolayers::keyedLook(const QList<LayerKey>& keys, const VideoTransform& transform, double opacity,
                                        double own) {
    VideoLayer::Look look{transform, opacity};
    if (keys.isEmpty()) return look;
    auto apply = [&](const LayerKey& k) {
        look.transform.x = k.x;
        look.transform.y = k.y;
        look.transform.scale = k.scale;
        look.opacity = k.opacity;
    };
    if (own <= keys.first().t) {
        apply(keys.first());
        return look;
    }
    for (qsizetype i = 1; i < keys.size(); ++i) {
        const LayerKey& a = keys[i - 1];
        const LayerKey& b = keys[i];
        if (own > b.t) continue;
        double u = b.t > a.t ? (own - a.t) / (b.t - a.t) : 1;
        if (a.ease) u = u * u * (3 - 2 * u);
        apply({own, a.x + (b.x - a.x) * u, a.y + (b.y - a.y) * u, a.scale + (b.scale - a.scale) * u,
               a.opacity + (b.opacity - a.opacity) * u, false});
        return look;
    }
    apply(keys.last());
    return look;
}

int VideoLayer::keyAt(double own, double tolerance) const {
    for (qsizetype i = 0; i < keys.size(); ++i)
        if (std::abs(keys[i].t - own) <= tolerance) return int(i);
    return -1;
}

void VideoLayer::setLook(double own, const Look& look, double tolerance) {
    if (keys.isEmpty()) {
        transform = look.transform;
        opacity = look.opacity;
        return;
    }
    const LayerKey k{own, look.transform.x, look.transform.y, look.transform.scale, look.opacity, false};
    if (const int i = keyAt(own, tolerance); i >= 0) {
        keys[i] = {keys[i].t, k.x, k.y, k.scale, k.opacity, keys[i].ease};
        return;
    }
    if (keys.size() >= kMaxKeys) return;
    keys.append(k);
    std::sort(keys.begin(), keys.end(), [](const LayerKey& a, const LayerKey& b) { return a.t < b.t; });
}

QJsonObject VideoLayer::toJson() const {
    QJsonArray keyList;
    for (const LayerKey& k : keys)
        keyList.append(QJsonArray{k.t, k.x, k.y, k.scale, k.opacity, k.ease ? 1 : 0});
    return {{QStringLiteral("keys"), keyList},
            {QStringLiteral("effects"), effects.toJson()},
            {QStringLiteral("name"), name},
            {QStringLiteral("path"), path},
            {QStringLiteral("w"), size.width()},
            {QStringLiteral("h"), size.height()},
            {QStringLiteral("duration"), duration},
            {QStringLiteral("track"), track},
            {QStringLiteral("start"), start},
            {QStringLiteral("in"), range.in},
            {QStringLiteral("out"), range.out},
            {QStringLiteral("x"), transform.x},
            {QStringLiteral("y"), transform.y},
            {QStringLiteral("scale"), transform.scale},
            {QStringLiteral("flipH"), transform.flipH},
            {QStringLiteral("flipV"), transform.flipV},
            {QStringLiteral("opacity"), opacity},
            {QStringLiteral("visible"), visible},
            {QStringLiteral("locked"), locked}};
}

VideoLayer VideoLayer::fromJson(const QJsonObject& json) {
    VideoLayer l;
    l.name = json.value(QStringLiteral("name")).toString();
    l.path = json.value(QStringLiteral("path")).toString();
    l.size = QSizeF(json.value(QStringLiteral("w")).toDouble(), json.value(QStringLiteral("h")).toDouble());
    l.duration = json.value(QStringLiteral("duration")).toDouble();
    l.track = std::clamp(json.value(QStringLiteral("track")).toInt(2), 2, 1 + kMaxLayers);
    l.start = json.value(QStringLiteral("start")).toDouble();
    l.range.in = json.value(QStringLiteral("in")).toDouble();
    l.range.out = json.value(QStringLiteral("out")).toDouble(l.duration);
    l.transform.x = json.value(QStringLiteral("x")).toDouble();
    l.transform.y = json.value(QStringLiteral("y")).toDouble();
    l.transform.scale = json.value(QStringLiteral("scale")).toDouble(1);
    l.transform.flipH = json.value(QStringLiteral("flipH")).toBool();
    l.transform.flipV = json.value(QStringLiteral("flipV")).toBool();
    l.opacity = std::clamp(json.value(QStringLiteral("opacity")).toDouble(100), 0.0, 100.0);
    l.visible = json.value(QStringLiteral("visible")).toBool(true);
    l.locked = json.value(QStringLiteral("locked")).toBool(false);
    for (const QJsonValue& v : json.value(QStringLiteral("keys")).toArray()) {
        const QJsonArray a = v.toArray();
        if (a.size() >= 6 && l.keys.size() < kMaxKeys)
            l.keys.append({a.at(0).toDouble(), a.at(1).toDouble(), a.at(2).toDouble(), a.at(3).toDouble(1),
                           a.at(4).toDouble(100), a.at(5).toInt() != 0});
    }
    std::sort(l.keys.begin(), l.keys.end(), [](const LayerKey& a, const LayerKey& b) { return a.t < b.t; });
    l.effects = ClipEffects::fromJson(json.value(QStringLiteral("effects")).toObject());
    return l;
}

namespace videolayers {

QList<int> drawOrder(const QList<VideoLayer>& layers) {
    QList<int> order(layers.size());
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return layers[a].track < layers[b].track; });
    return order;
}

int hitLayer(const QList<VideoLayer>& layers, const QPointF& scene, double t) {
    const QList<int> order = drawOrder(layers);
    for (qsizetype k = order.size() - 1; k >= 0; --k) {
        const VideoLayer& l = layers[order[k]];
        double own = 0;
        if (l.visible && !l.locked && l.sourceTime(t, &own) && l.rectAt(own).contains(scene)) return order[k];
    }
    return -1;
}

bool fits(const QList<VideoLayer>& layers, int track, double start, double length, int except) {
    for (qsizetype i = 0; i < layers.size(); ++i) {
        if (int(i) == except || layers[i].track != track) continue;
        if (start < layers[i].end() - 1e-9 && start + length > layers[i].start + 1e-9) return false;
    }
    return true;
}

} // namespace videolayers

} // namespace avloop

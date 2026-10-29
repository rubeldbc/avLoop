#include "core/VideoLayer.h"

#include <QJsonArray>

#include <algorithm>
#include <numeric>

namespace avloop {

bool VideoLayer::sourceTime(double t, double* own) const {
    if (t < start - 1e-9 || t >= end() - 1e-9) return false;
    if (own) *own = range.in + (t - start);
    return true;
}

QJsonObject VideoLayer::toJson() const {
    return {{QStringLiteral("name"), name},
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
        if (l.visible && !l.locked && l.sourceTime(t, nullptr) && l.rect().contains(scene)) return order[k];
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

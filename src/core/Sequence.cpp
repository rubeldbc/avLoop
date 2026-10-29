#include "core/Sequence.h"

#include "core/Geometry.h"
#include "core/ProjectJson.h"

#include <QFileInfo>
#include <QJsonArray>

#include <algorithm>
#include <cmath>

namespace avloop {

namespace {

QJsonArray keysJson(const QList<LayerKey>& keys) {
    QJsonArray list;
    for (const LayerKey& k : keys)
        list.append(QJsonArray{k.t, k.x, k.y, k.scale, k.opacity, k.ease ? 1 : 0});
    return list;
}

QList<LayerKey> keysFrom(const QJsonArray& list) {
    QList<LayerKey> keys;
    for (const QJsonValue& v : list) {
        const QJsonArray a = v.toArray();
        if (a.size() >= 6 && keys.size() < VideoLayer::kMaxKeys)
            keys.append({a.at(0).toDouble(), a.at(1).toDouble(), a.at(2).toDouble(), a.at(3).toDouble(1),
                         a.at(4).toDouble(100), a.at(5).toInt() != 0});
    }
    std::sort(keys.begin(), keys.end(), [](const LayerKey& a, const LayerKey& b) { return a.t < b.t; });
    return keys;
}

// A shape of the old scene in a clip's own pixels: (scene − its rectangle's top-left) / its scale.
MaskShape ownPixels(MaskShape s, const QRectF& video, double scale) {
    const auto map = [&](const QPointF& p) { return (p - video.topLeft()) / scale; };
    s.rect = QRectF(map(s.rect.topLeft()), s.rect.size() / scale);
    s.radius /= scale;
    for (PathPoint& pt : s.points) {
        pt.p = map(pt.p);
        pt.in = map(pt.in);
        pt.out = map(pt.out);
    }
    return s;
}

} // namespace

bool Clip::ownTime(double t, double* own) const {
    if (!enabled || t < start - 1e-9 || t >= end() - 1e-9) return false;
    if (own) *own = range.in + (t - start);
    return true;
}

VideoLayer::Look Clip::look(double own) const {
    return videolayers::keyedLook(keys, transform, opacity, own);
}

QJsonObject Clip::toJson() const {
    QJsonArray maskList;
    for (const MaskShape& m : masks)
        maskList.append(m.toJson());
    return {{QStringLiteral("id"), id},
            {QStringLiteral("name"), name},
            {QStringLiteral("path"), path},
            {QStringLiteral("w"), size.width()},
            {QStringLiteral("h"), size.height()},
            {QStringLiteral("duration"), duration},
            {QStringLiteral("start"), start},
            {QStringLiteral("in"), range.in},
            {QStringLiteral("out"), range.out},
            {QStringLiteral("x"), transform.x},
            {QStringLiteral("y"), transform.y},
            {QStringLiteral("scale"), transform.scale},
            {QStringLiteral("flipH"), transform.flipH},
            {QStringLiteral("flipV"), transform.flipV},
            {QStringLiteral("keys"), keysJson(keys)},
            {QStringLiteral("opacity"), opacity},
            {QStringLiteral("effects"), effects.toJson()},
            {QStringLiteral("masks"), maskList},
            {QStringLiteral("enabled"), enabled}};
}

Clip Clip::fromJson(const QJsonObject& json) {
    Clip c;
    c.id = json.value(QStringLiteral("id")).toInt();
    c.name = json.value(QStringLiteral("name")).toString();
    c.path = json.value(QStringLiteral("path")).toString();
    c.size = QSizeF(json.value(QStringLiteral("w")).toDouble(), json.value(QStringLiteral("h")).toDouble());
    c.duration = json.value(QStringLiteral("duration")).toDouble();
    c.start = json.value(QStringLiteral("start")).toDouble();
    c.range = {json.value(QStringLiteral("in")).toDouble(), json.value(QStringLiteral("out")).toDouble(c.duration)};
    c.transform.x = json.value(QStringLiteral("x")).toDouble();
    c.transform.y = json.value(QStringLiteral("y")).toDouble();
    c.transform.scale = json.value(QStringLiteral("scale")).toDouble(1);
    c.transform.flipH = json.value(QStringLiteral("flipH")).toBool();
    c.transform.flipV = json.value(QStringLiteral("flipV")).toBool();
    c.keys = keysFrom(json.value(QStringLiteral("keys")).toArray());
    c.opacity = std::clamp(json.value(QStringLiteral("opacity")).toDouble(100), 0.0, 100.0);
    c.effects = ClipEffects::fromJson(json.value(QStringLiteral("effects")).toObject());
    for (const QJsonValue& v : json.value(QStringLiteral("masks")).toArray())
        if (c.masks.size() < MaskShape::kMaxShapes) c.masks.append(MaskShape::fromJson(v.toObject()));
    c.enabled = json.value(QStringLiteral("enabled")).toBool(true);
    return c;
}

int Sequence::clipCount() const {
    int n = 0;
    for (const Track& t : tracks)
        n += int(t.clips.size());
    return n;
}

double Sequence::duration() const {
    double end = 0;
    for (const Track& t : tracks)
        for (const Clip& c : t.clips)
            end = std::max(end, c.end());
    return end;
}

QJsonObject Sequence::toJson() const {
    QJsonArray trackList;
    for (const Track& t : tracks) {
        QJsonArray clips;
        for (const Clip& c : t.clips)
            clips.append(c.toJson());
        trackList.append(QJsonObject{{QStringLiteral("name"), t.name},
                                     {QStringLiteral("visible"), t.visible},
                                     {QStringLiteral("locked"), t.locked},
                                     {QStringLiteral("clips"), clips}});
    }
    return {{QStringLiteral("w"), frame.width()},
            {QStringLiteral("h"), frame.height()},
            {QStringLiteral("fps"), QStringLiteral("%1/%2").arg(fps.num).arg(fps.den)},
            {QStringLiteral("in"), inOut.in},
            {QStringLiteral("out"), inOut.out},
            {QStringLiteral("nextId"), nextId},
            {QStringLiteral("tracks"), trackList}};
}

Sequence Sequence::fromJson(const QJsonObject& json) {
    Sequence s;
    s.frame = QSize(json.value(QStringLiteral("w")).toInt(1920), json.value(QStringLiteral("h")).toInt(1080));
    const QStringList fps = json.value(QStringLiteral("fps")).toString(QStringLiteral("30/1")).split(QLatin1Char('/'));
    s.fps = {fps.value(0).toInt(), std::max(1, fps.value(1).toInt())};
    if (!s.fps.isValid()) s.fps = {30, 1};
    s.inOut = {json.value(QStringLiteral("in")).toDouble(), json.value(QStringLiteral("out")).toDouble()};
    s.nextId = std::max(1, json.value(QStringLiteral("nextId")).toInt(1));
    for (const QJsonValue& v : json.value(QStringLiteral("tracks")).toArray()) {
        if (s.tracks.size() >= kMaxTracks) break;
        const QJsonObject o = v.toObject();
        Track t{o.value(QStringLiteral("name")).toString(),
                o.value(QStringLiteral("visible")).toBool(true),
                o.value(QStringLiteral("locked")).toBool(false),
                {}};
        for (const QJsonValue& c : o.value(QStringLiteral("clips")).toArray())
            t.clips.append(Clip::fromJson(c.toObject()));
        std::sort(t.clips.begin(), t.clips.end(), [](const Clip& a, const Clip& b) { return a.start < b.start; });
        s.tracks.append(t);
        for (const Clip& c : s.tracks.last().clips)
            s.nextId = std::max(s.nextId, c.id + 1);
    }
    return s;
}

Sequence Sequence::fromAvLoop(const ProjectData& d) {
    Sequence s;
    const QSizeF video(d.source.width, d.source.height);
    const QRectF frame = geometry::exportFrame(d.crop, d.transform, video);
    const QPointF origin = frame.topLeft();
    s.frame = QSize(std::max(16, int(std::lround(frame.width()))), std::max(16, int(std::lround(frame.height()))));
    s.fps = d.output.sourceFps && d.source.fps.isValid() ? d.source.fps : d.output.fps;
    if (!s.fps.isValid()) s.fps = {30, 1};
    s.inOut = d.range;
    s.tracks.append({QStringLiteral("V1"), true, false, {}});
    if (!d.source.path.isEmpty()) {
        Clip main;
        main.id = s.nextId++;
        main.name = QFileInfo(d.source.path).completeBaseName();
        main.path = d.source.path;
        main.size = video;
        main.duration = d.source.duration;
        main.range = {0, d.source.duration};
        main.transform = d.transform;
        main.transform.x -= origin.x();
        main.transform.y -= origin.y();
        for (const Keyframe& k : d.keyframes)
            main.keys.append({k.t, k.x - origin.x(), k.y - origin.y(), k.scale, 100, k.ease});
        main.effects = d.effects;
        // The crop's shapes, then its own shape (a hole when inverted), in the clip's own pixels at its place at In.
        const QRectF placed = d.transform.rect(video);
        const double scale = d.transform.scale > 0 ? d.transform.scale : 1.0;
        for (const MaskShape& m : d.crop.shapes)
            main.masks.append(ownPixels(m, placed, scale));
        const bool plainRect = d.crop.kind == CropShape::Kind::Rect && d.crop.feather <= 0 && !d.crop.invert;
        if (!d.crop.isEmpty() && !plainRect) {
            MaskShape own;
            own.kind = d.crop.kind == CropShape::Kind::Ellipse   ? MaskShape::Kind::Ellipse
                       : d.crop.kind == CropShape::Kind::Rounded ? MaskShape::Kind::Rounded
                                                                 : MaskShape::Kind::Rect;
            own.role = d.crop.invert ? MaskShape::Role::Cut : MaskShape::Role::KeepOnly;
            own.rect = d.crop.rect;
            own.radius = d.crop.radius;
            own.feather = d.crop.feather;
            own.name = QStringLiteral("Crop");
            main.masks.append(ownPixels(own, placed, scale));
        }
        s.tracks[0].clips.append(main);
    }
    for (const VideoLayer& l : d.layers) {
        const int index = std::clamp(l.track, 2, kMaxTracks) - 1;
        while (s.tracks.size() <= index)
            s.tracks.append({QStringLiteral("V%1").arg(s.tracks.size() + 1), true, false, {}});
        Clip c;
        c.id = s.nextId++;
        c.name = l.name;
        c.path = l.path;
        c.size = l.size;
        c.duration = l.duration;
        c.start = l.start;
        c.range = l.range;
        c.transform = l.transform;
        c.transform.x -= origin.x();
        c.transform.y -= origin.y();
        for (LayerKey k : l.keys) {
            k.x -= origin.x();
            k.y -= origin.y();
            c.keys.append(k);
        }
        c.opacity = l.opacity;
        c.effects = l.effects;
        c.enabled = l.visible;
        s.tracks[index].clips.append(c);
        std::sort(s.tracks[index].clips.begin(), s.tracks[index].clips.end(),
                  [](const Clip& a, const Clip& b) { return a.start < b.start; });
    }
    return s;
}

} // namespace avloop

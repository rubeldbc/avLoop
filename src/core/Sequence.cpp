#include "core/Sequence.h"

#include "core/Geometry.h"
#include "core/Keyframes.h"
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

QJsonObject matteJson(const CropShape& m) {
    QJsonArray shapes;
    for (const MaskShape& s : m.shapes)
        shapes.append(s.toJson());
    return {{QStringLiteral("kind"), CropShape::kindId(m.kind)},
            {QStringLiteral("x"), m.rect.x()},
            {QStringLiteral("y"), m.rect.y()},
            {QStringLiteral("w"), m.rect.width()},
            {QStringLiteral("h"), m.rect.height()},
            {QStringLiteral("radius"), m.radius},
            {QStringLiteral("feather"), m.feather},
            {QStringLiteral("invert"), m.invert},
            {QStringLiteral("shapes"), shapes}};
}

CropShape matteFrom(const QJsonObject& json) {
    CropShape m;
    m.kind = CropShape::kindFromId(json.value(QStringLiteral("kind")).toString());
    m.rect = QRectF(json.value(QStringLiteral("x")).toDouble(), json.value(QStringLiteral("y")).toDouble(),
                    json.value(QStringLiteral("w")).toDouble(), json.value(QStringLiteral("h")).toDouble());
    m.radius = json.value(QStringLiteral("radius")).toDouble();
    m.feather = json.value(QStringLiteral("feather")).toDouble();
    m.invert = json.value(QStringLiteral("invert")).toBool();
    for (const QJsonValue& v : json.value(QStringLiteral("shapes")).toArray())
        if (m.shapes.size() < MaskShape::kMaxShapes) m.shapes.append(MaskShape::fromJson(v.toObject()));
    return m;
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

int Clip::keyAt(double own, double tolerance) const {
    for (qsizetype i = 0; i < keys.size(); ++i)
        if (std::abs(keys[i].t - own) <= tolerance) return int(i);
    return -1;
}

void Clip::setLook(double own, const VideoLayer::Look& l, double tolerance) {
    if (keys.isEmpty()) {
        const bool flipH = transform.flipH, flipV = transform.flipV;
        transform = l.transform;
        transform.flipH = flipH;
        transform.flipV = flipV;
        opacity = l.opacity;
        return;
    }
    const LayerKey k{own, l.transform.x, l.transform.y, l.transform.scale, l.opacity, false};
    if (const int i = keyAt(own, tolerance); i >= 0) {
        keys[i] = {keys[i].t, k.x, k.y, k.scale, k.opacity, keys[i].ease};
        return;
    }
    if (keys.size() >= VideoLayer::kMaxKeys) return;
    keys.append(k);
    std::sort(keys.begin(), keys.end(), [](const LayerKey& a, const LayerKey& b) { return a.t < b.t; });
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
            {QStringLiteral("enabled"), enabled},
            {QStringLiteral("locked"), locked}};
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
    c.locked = json.value(QStringLiteral("locked")).toBool(false);
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
    return {{QStringLiteral("x"), frame.x()},
            {QStringLiteral("y"), frame.y()},
            {QStringLiteral("w"), frame.width()},
            {QStringLiteral("h"), frame.height()},
            {QStringLiteral("fps"), QStringLiteral("%1/%2").arg(fps.num).arg(fps.den)},
            {QStringLiteral("in"), inOut.in},
            {QStringLiteral("out"), inOut.out},
            {QStringLiteral("main"), main},
            {QStringLiteral("nextId"), nextId},
            {QStringLiteral("tracks"), trackList},
            {QStringLiteral("matte"), matteJson(matte)}};
}

Sequence Sequence::fromJson(const QJsonObject& json) {
    Sequence s;
    s.frame = QRectF(json.value(QStringLiteral("x")).toDouble(), json.value(QStringLiteral("y")).toDouble(),
                     json.value(QStringLiteral("w")).toDouble(1920), json.value(QStringLiteral("h")).toDouble(1080));
    const QStringList fps = json.value(QStringLiteral("fps")).toString(QStringLiteral("30/1")).split(QLatin1Char('/'));
    s.fps = {fps.value(0).toInt(), std::max(1, fps.value(1).toInt())};
    if (!s.fps.isValid()) s.fps = {30, 1};
    s.inOut = {json.value(QStringLiteral("in")).toDouble(), json.value(QStringLiteral("out")).toDouble()};
    s.nextId = std::max(1, json.value(QStringLiteral("nextId")).toInt(1));
    s.main = json.value(QStringLiteral("main")).toInt();
    s.matte = matteFrom(json.value(QStringLiteral("matte")).toObject());
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
    // The export's frame: the crop, else the video where it stands at In (RenderJob's).
    s.frame = geometry::exportFrame(d.crop, keyframes::valueAt(d.keyframes, d.transform, d.range.in), video);
    s.fps = d.output.sourceFps && d.source.fps.isValid() ? d.source.fps : d.output.fps;
    if (!s.fps.isValid()) s.fps = {30, 1};
    s.inOut = d.range;
    s.tracks.append({QStringLiteral("V1"), true, false, {}});
    {
        // The main video — a placeholder without a path when the project has none (D-033: V1 always holds it).
        Clip main;
        main.id = s.nextId++;
        main.name = QFileInfo(d.source.path).completeBaseName();
        main.path = d.source.path;
        main.size = video;
        main.duration = d.source.duration;
        main.range = {0, d.source.duration};
        main.transform = d.transform;
        for (const Keyframe& k : d.keyframes)
            main.keys.append({k.t, k.x, k.y, k.scale, 100, k.ease});
        main.effects = d.effects;
        s.main = main.id;
        s.tracks[0].clips.append(main);
    }
    // The crop over everything, as AvLoop drew it over the main video and the layers (RS-10).
    s.matte = d.crop;
    s.matte.ratio = CropShape().ratio;
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
        c.keys = l.keys;
        c.opacity = l.opacity;
        c.effects = l.effects;
        c.enabled = l.visible;
        c.locked = l.locked;
        s.tracks[index].clips.append(c);
        std::sort(s.tracks[index].clips.begin(), s.tracks[index].clips.end(),
                  [](const Clip& a, const Clip& b) { return a.start < b.start; });
    }
    return s;
}

} // namespace avloop

#include "core/ProjectJson.h"

#include "core/ProjectMigrate.h"
#include "core/Sequence.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>

#include <algorithm>

namespace avloop::projectjson {

namespace {

QString tr(const char* text) {
    return QCoreApplication::translate("ProjectJson", text);
}

// A known object written over the one read, so its unknown keys stay.
QJsonArray shapesJson(const QList<MaskShape>& shapes) {
    QJsonArray list;
    for (const MaskShape& s : shapes)
        list.append(s.toJson());
    return list;
}

QJsonObject merged(const QJsonObject& original, const QString& key, const QJsonObject& known) {
    QJsonObject o = original.value(key).toObject();
    for (auto it = known.begin(); it != known.end(); ++it)
        o.insert(it.key(), it.value());
    return o;
}

} // namespace

QJsonObject toJson(const ProjectData& d, const QString& projectPath) {
    QJsonObject root = d.original;
    root.insert(QStringLiteral("format"), QStringLiteral("avloop"));
    root.insert(QStringLiteral("version"), kVersion);

    QString relative = d.relativePath;
    if (!projectPath.isEmpty() && !d.source.path.isEmpty())
        relative = QFileInfo(projectPath).absoluteDir().relativeFilePath(d.source.path);
    QJsonObject source{
        {QStringLiteral("path"), QDir::fromNativeSeparators(d.source.path)},
        {QStringLiteral("relativePath"), QDir::fromNativeSeparators(relative)},
        {QStringLiteral("size"), double(d.source.fileSize)},
        {QStringLiteral("mtime"), d.source.modified.toUTC().toString(Qt::ISODate)},
        {QStringLiteral("stream"), d.source.stream},
    };
    root.insert(QStringLiteral("source"), merged(d.original, QStringLiteral("source"), source));

    const QJsonObject range{{QStringLiteral("in"), d.range.in}, {QStringLiteral("out"), d.range.out}};
    root.insert(QStringLiteral("range"), merged(d.original, QStringLiteral("range"), range));
    root.insert(QStringLiteral("loop"), merged(d.original, QStringLiteral("loop"), loopToJson(d.loop)));
    root.insert(QStringLiteral("output"), merged(d.original, QStringLiteral("output"), outputToJson(d.output)));
    if (!d.exportPath.isEmpty())
        root.insert(QStringLiteral("export"), merged(d.original, QStringLiteral("export"),
                                                     QJsonObject{{QStringLiteral("lastPath"), d.exportPath}}));
    root.insert(QStringLiteral("quality"), merged(d.original, QStringLiteral("quality"), qualityToJson(d.quality)));

    QJsonObject transform{
        {QStringLiteral("x"), d.transform.x},         {QStringLiteral("y"), d.transform.y},
        {QStringLiteral("scale"), d.transform.scale}, {QStringLiteral("flipH"), d.transform.flipH},
        {QStringLiteral("flipV"), d.transform.flipV},
    };
    QJsonArray keys;
    for (const Keyframe& k : d.keyframes)
        keys.append(
            QJsonObject{{QStringLiteral("t"), k.t},
                        {QStringLiteral("x"), k.x},
                        {QStringLiteral("y"), k.y},
                        {QStringLiteral("scale"), k.scale},
                        {QStringLiteral("interp"), k.ease ? QStringLiteral("ease") : QStringLiteral("linear")}});
    transform.insert(QStringLiteral("keyframes"), keys);
    root.insert(QStringLiteral("transform"), merged(d.original, QStringLiteral("transform"), transform));

    QJsonObject crop{
        {QStringLiteral("kind"), CropShape::kindId(d.crop.kind)},
        {QStringLiteral("x"), d.crop.rect.x()},
        {QStringLiteral("y"), d.crop.rect.y()},
        {QStringLiteral("w"), d.crop.rect.width()},
        {QStringLiteral("h"), d.crop.rect.height()},
        {QStringLiteral("ratio"), d.crop.ratio},
        {QStringLiteral("radius"), d.crop.radius},
        {QStringLiteral("feather"), d.crop.feather},
        {QStringLiteral("invert"), d.crop.invert},
        {QStringLiteral("shapes"), shapesJson(d.crop.shapes)},
    };
    root.insert(QStringLiteral("crop"), merged(d.original, QStringLiteral("crop"), crop));
    root.insert(QStringLiteral("effects"), merged(d.original, QStringLiteral("effects"), d.effects.toJson()));
    QJsonArray layers;
    for (const VideoLayer& l : d.layers)
        layers.append(l.toJson());
    root.insert(QStringLiteral("layers"), layers);
    // The sequence (version 4, D-031): AvLoop's model as tracks and clips — derived from it until the editor edits the
    // sequence itself (RS-7). Only a probed source gives it (its size and length are not in the file); else the one
    // read stays (the root starts as the file read).
    if (d.source.width > 0 && d.source.height > 0)
        root.insert(QStringLiteral("sequence"), Sequence::fromAvLoop(d).toJson());

    QJsonObject view{
        {QStringLiteral("zoom"), d.view.zoom},
        {QStringLiteral("panX"), d.view.panX},
        {QStringLiteral("panY"), d.view.panY},
        {QStringLiteral("tool"), d.view.tool},
    };
    root.insert(QStringLiteral("view"), merged(d.original, QStringLiteral("view"), view));
    return root;
}

bool fromJson(const QJsonObject& read, ProjectData& d, QString* error, const QString& projectPath) {
    if (read.value(QStringLiteral("format")).toString() != QLatin1String("avloop")) {
        if (error) *error = tr("This is not an AvLoop project.");
        return false;
    }
    QJsonObject json = read;
    if (!projectmigrate::upgrade(json, kVersion, error)) return false;
    d = ProjectData{};
    d.original = json;
    const QJsonObject source = json.value(QStringLiteral("source")).toObject();
    d.source.path = QDir::toNativeSeparators(source.value(QStringLiteral("path")).toString());
    d.relativePath = source.value(QStringLiteral("relativePath")).toString();
    d.source.fileSize = qint64(source.value(QStringLiteral("size")).toDouble());
    d.source.modified = QDateTime::fromString(source.value(QStringLiteral("mtime")).toString(), Qt::ISODate);
    d.source.stream = source.value(QStringLiteral("stream")).toInt(0);
    // The source is found by its path, then by its path relative to the project file (spec §11.1).
    if (!QFileInfo::exists(d.source.path) && !projectPath.isEmpty() && !d.relativePath.isEmpty()) {
        const QString beside = QFileInfo(projectPath).absoluteDir().absoluteFilePath(d.relativePath);
        if (QFileInfo::exists(beside)) d.source.path = QDir::toNativeSeparators(QDir::cleanPath(beside));
    }

    // Out 0 (or none) is the whole file; the project fits the range to the source's grid when it is restored.
    const QJsonObject r = json.value(QStringLiteral("range")).toObject();
    d.range.in = r.value(QStringLiteral("in")).toDouble();
    d.range.out = r.value(QStringLiteral("out")).toDouble();

    d.loop = loopFromJson(json.value(QStringLiteral("loop")).toObject());
    d.output = outputFromJson(json.value(QStringLiteral("output")).toObject());
    d.quality = qualityFromJson(json.value(QStringLiteral("quality")).toObject());
    d.exportPath = json.value(QStringLiteral("export")).toObject().value(QStringLiteral("lastPath")).toString();

    const QJsonObject t = json.value(QStringLiteral("transform")).toObject();
    d.transform.x = t.value(QStringLiteral("x")).toDouble();
    d.transform.y = t.value(QStringLiteral("y")).toDouble();
    d.transform.scale = t.value(QStringLiteral("scale")).toDouble(1.0);
    d.transform.flipH = t.value(QStringLiteral("flipH")).toBool();
    d.transform.flipV = t.value(QStringLiteral("flipV")).toBool();
    d.transform = d.transform.clamped();
    for (const QJsonValue& v : t.value(QStringLiteral("keyframes")).toArray()) {
        const QJsonObject o = v.toObject();
        Keyframe k{o.value(QStringLiteral("t")).toDouble(), o.value(QStringLiteral("x")).toDouble(),
                   o.value(QStringLiteral("y")).toDouble(), o.value(QStringLiteral("scale")).toDouble(1.0),
                   o.value(QStringLiteral("interp")).toString() == QLatin1String("ease")};
        k.scale = std::clamp(k.scale, VideoTransform::kMinScale, VideoTransform::kMaxScale);
        d.keyframes = keyframes::withKey(d.keyframes, k, 0); // kept in order whatever the file's order
    }

    const QJsonObject c = json.value(QStringLiteral("crop")).toObject();
    d.crop.kind = CropShape::kindFromId(c.value(QStringLiteral("kind")).toString());
    d.crop.rect = QRectF(c.value(QStringLiteral("x")).toDouble(), c.value(QStringLiteral("y")).toDouble(),
                         c.value(QStringLiteral("w")).toDouble(), c.value(QStringLiteral("h")).toDouble());
    d.crop.ratio = c.value(QStringLiteral("ratio")).toString(QStringLiteral("free"));
    d.crop.radius = c.value(QStringLiteral("radius")).toDouble();
    d.crop.feather = c.value(QStringLiteral("feather")).toDouble();
    d.crop.invert = c.value(QStringLiteral("invert")).toBool(); // absent before version 3: not inverted
    d.crop.shapes.clear();
    for (const QJsonValue& v : c.value(QStringLiteral("shapes")).toArray())
        d.crop.shapes.append(MaskShape::fromJson(v.toObject()));
    d.crop = d.crop.clamped();
    d.effects = ClipEffects::fromJson(json.value(QStringLiteral("effects")).toObject());
    d.layers.clear();
    for (const QJsonValue& v : json.value(QStringLiteral("layers")).toArray())
        if (d.layers.size() < VideoLayer::kMaxLayers) d.layers.append(VideoLayer::fromJson(v.toObject()));

    const QJsonObject v = json.value(QStringLiteral("view")).toObject();
    d.view.zoom = v.value(QStringLiteral("zoom")).toDouble(1.0);
    d.view.panX = v.value(QStringLiteral("panX")).toDouble();
    d.view.panY = v.value(QStringLiteral("panY")).toDouble();
    d.view.tool = v.value(QStringLiteral("tool")).toString(QStringLiteral("select"));
    return true;
}

bool save(const ProjectData& data, const QString& path, QString* error) {
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) *error = file.errorString();
        return false;
    }
    file.write(QJsonDocument(toJson(data, path)).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        if (error) *error = file.errorString();
        return false;
    }
    return true;
}

bool load(const QString& path, ProjectData& data, QString* error) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = file.errorString();
        return false;
    }
    QJsonParseError parse{};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parse);
    if (parse.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error) *error = tr("This project file is damaged: %1.").arg(parse.errorString());
        return false;
    }
    return fromJson(doc.object(), data, error, path);
}

} // namespace avloop::projectjson

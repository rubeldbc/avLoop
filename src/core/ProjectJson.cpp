#include "core/ProjectJson.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>

namespace avloop::projectjson {

namespace {

QString tr(const char* text) {
    return QCoreApplication::translate("ProjectJson", text);
}

// A known object written over the one read, so its unknown keys stay.
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
    const QJsonObject loop{
        {QStringLiteral("mode"), LoopSettings::modeId(d.loop.mode)},
        {QStringLiteral("fade"), d.loop.fade},
        {QStringLiteral("curve"), LoopSettings::curveId(d.loop.curve)},
        {QStringLiteral("turn"), d.loop.turn},
        {QStringLiteral("speed"), d.loop.speed},
        {QStringLiteral("fpsMethod"), LoopSettings::methodId(d.loop.method)},
    };
    root.insert(QStringLiteral("loop"), merged(d.original, QStringLiteral("loop"), loop));
    const QJsonObject output{{QStringLiteral("fps"), d.output.fpsId()}};
    root.insert(QStringLiteral("output"), merged(d.original, QStringLiteral("output"), output));

    QJsonObject transform{
        {QStringLiteral("x"), d.transform.x},         {QStringLiteral("y"), d.transform.y},
        {QStringLiteral("scale"), d.transform.scale}, {QStringLiteral("flipH"), d.transform.flipH},
        {QStringLiteral("flipV"), d.transform.flipV},
    };
    if (!d.original.value(QStringLiteral("transform")).toObject().contains(QStringLiteral("keyframes")))
        transform.insert(QStringLiteral("keyframes"), QJsonArray());
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
    };
    root.insert(QStringLiteral("crop"), merged(d.original, QStringLiteral("crop"), crop));

    QJsonObject view{
        {QStringLiteral("zoom"), d.view.zoom},
        {QStringLiteral("panX"), d.view.panX},
        {QStringLiteral("panY"), d.view.panY},
        {QStringLiteral("tool"), d.view.tool},
    };
    root.insert(QStringLiteral("view"), merged(d.original, QStringLiteral("view"), view));
    return root;
}

bool fromJson(const QJsonObject& json, ProjectData& d, QString* error, const QString& projectPath) {
    if (json.value(QStringLiteral("format")).toString() != QLatin1String("avloop")) {
        if (error) *error = tr("This is not an AvLoop project.");
        return false;
    }
    const int version = json.value(QStringLiteral("version")).toInt(0);
    if (version > kVersion) {
        if (error)
            *error =
                tr("This project was saved by a newer AvLoop (version %1); update AvLoop to open it.").arg(version);
        return false;
    }
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

    const QJsonObject l = json.value(QStringLiteral("loop")).toObject();
    const LoopSettings defaults;
    d.loop.mode = l.contains(QStringLiteral("mode"))
                      ? LoopSettings::modeFromId(l.value(QStringLiteral("mode")).toString())
                      : defaults.mode;
    d.loop.fade = l.value(QStringLiteral("fade")).toDouble(defaults.fade);
    d.loop.curve = l.contains(QStringLiteral("curve"))
                       ? LoopSettings::curveFromId(l.value(QStringLiteral("curve")).toString())
                       : defaults.curve;
    d.loop.turn = l.value(QStringLiteral("turn")).toDouble(defaults.turn);
    d.loop.speed = l.value(QStringLiteral("speed")).toDouble(defaults.speed);
    d.loop.method = LoopSettings::methodFromId(l.value(QStringLiteral("fpsMethod")).toString());
    d.loop = d.loop.clamped();
    d.output = OutputSettings::fromFpsId(
        json.value(QStringLiteral("output")).toObject().value(QStringLiteral("fps")).toString());

    const QJsonObject t = json.value(QStringLiteral("transform")).toObject();
    d.transform.x = t.value(QStringLiteral("x")).toDouble();
    d.transform.y = t.value(QStringLiteral("y")).toDouble();
    d.transform.scale = t.value(QStringLiteral("scale")).toDouble(1.0);
    d.transform.flipH = t.value(QStringLiteral("flipH")).toBool();
    d.transform.flipV = t.value(QStringLiteral("flipV")).toBool();
    d.transform = d.transform.clamped();

    const QJsonObject c = json.value(QStringLiteral("crop")).toObject();
    d.crop.kind = CropShape::kindFromId(c.value(QStringLiteral("kind")).toString());
    d.crop.rect = QRectF(c.value(QStringLiteral("x")).toDouble(), c.value(QStringLiteral("y")).toDouble(),
                         c.value(QStringLiteral("w")).toDouble(), c.value(QStringLiteral("h")).toDouble());
    d.crop.ratio = c.value(QStringLiteral("ratio")).toString(QStringLiteral("free"));
    d.crop.radius = c.value(QStringLiteral("radius")).toDouble();
    d.crop.feather = c.value(QStringLiteral("feather")).toDouble();
    d.crop = d.crop.clamped();

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

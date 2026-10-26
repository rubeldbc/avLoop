#include "core/Presets.h"

#include "core/ProjectJson.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QSaveFile>

#include <algorithm>

namespace avloop::presets {

namespace {

Preset obs(const char* name, int longSide) {
    Preset p;
    p.name = QCoreApplication::translate("Presets", name);
    p.output.setSizeId(QStringLiteral("long-%1").arg(longSide));
    p.builtIn = true;
    return p;
}

} // namespace

QList<Preset> builtIn() {
    return {obs("OBS Overlay 1:1 512", 512), obs("OBS Lower Third 16:9 1280", 1280),
            obs("OBS Vertical 9:16 1080", 1080)};
}

bool isBuiltIn(const QString& name) {
    const QList<Preset> shipped = builtIn();
    return std::any_of(shipped.begin(), shipped.end(), [&](const Preset& p) { return p.name == name; });
}

QJsonObject toJson(const Preset& p) {
    return {
        {QStringLiteral("format"), QStringLiteral("avloop-preset")},
        {QStringLiteral("version"), 1},
        {QStringLiteral("name"), p.name},
        {QStringLiteral("loop"), projectjson::loopToJson(p.loop)},
        {QStringLiteral("output"), projectjson::outputToJson(p.output)},
        {QStringLiteral("quality"), projectjson::qualityToJson(p.quality)},
    };
}

std::optional<Preset> fromJson(const QJsonObject& json) {
    if (json.value(QStringLiteral("format")).toString() != QLatin1String("avloop-preset")) return std::nullopt;
    Preset p;
    p.name = json.value(QStringLiteral("name")).toString().trimmed();
    if (p.name.isEmpty()) return std::nullopt;
    p.loop = projectjson::loopFromJson(json.value(QStringLiteral("loop")).toObject());
    p.output = projectjson::outputFromJson(json.value(QStringLiteral("output")).toObject());
    p.quality = projectjson::qualityFromJson(json.value(QStringLiteral("quality")).toObject());
    return p;
}

QString fileName(const QString& name) {
    QString safe = name.trimmed();
    for (QChar& c : safe)
        if (QStringLiteral("<>:\"/\\|?*").contains(c) || c.unicode() < 32) c = QLatin1Char('_');
    return safe + QStringLiteral(".json");
}

QList<Preset> all(const QString& dir) {
    QList<Preset> list = builtIn();
    QList<Preset> mine;
    for (const QFileInfo& f : QDir(dir).entryInfoList({QStringLiteral("*.json")}, QDir::Files, QDir::Name)) {
        QFile file(f.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly)) continue;
        const std::optional<Preset> p = fromJson(QJsonDocument::fromJson(file.readAll()).object());
        if (p && !isBuiltIn(p->name)) mine << *p;
    }
    std::sort(mine.begin(), mine.end(),
              [](const Preset& a, const Preset& b) { return a.name.localeAwareCompare(b.name) < 0; });
    return list + mine;
}

bool save(const QString& dir, const Preset& p, QString* error) {
    if (p.name.trimmed().isEmpty() || isBuiltIn(p.name)) {
        if (error) *error = QCoreApplication::translate("Presets", "Choose another name.");
        return false;
    }
    QDir().mkpath(dir);
    QSaveFile file(QDir(dir).filePath(fileName(p.name)));
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) *error = file.errorString();
        return false;
    }
    file.write(QJsonDocument(toJson(p)).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        if (error) *error = file.errorString();
        return false;
    }
    return true;
}

bool remove(const QString& dir, const QString& name) {
    return !isBuiltIn(name) && QFile::remove(QDir(dir).filePath(fileName(name)));
}

} // namespace avloop::presets

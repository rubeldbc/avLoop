#include "core/AppSettings.h"

#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

#include <algorithm>

namespace avloop::appsettings {

namespace {

constexpr int kRecentMax = 10;

const char* recentKey(Recent kind) {
    return kind == Recent::Projects ? kRecentProjects : kRecentVideos;
}

QString made(const QString& dir) {
    QDir().mkpath(dir);
    return dir;
}

} // namespace

QVariant value(const char* key, const QVariant& fallback) {
    return QSettings().value(QLatin1String(key), fallback);
}

void setValue(const char* key, const QVariant& v) {
    QSettings().setValue(QLatin1String(key), v);
}

bool openLastProject() {
    return value(kOpenLastProject, false).toBool();
}

int autosaveMinutes() {
    return std::clamp(value(kAutosaveMinutes, 2).toInt(), 1, 60);
}

bool hardwareDecoding() {
    return value(kHardwareDecoding, QStringLiteral("auto")).toString() != QLatin1String("off");
}

bool gpuRendering() {
    // AVLOOP_GPU_RENDERING=off: the lab's whole set again on the CPU (CP-A1's S13), whatever the settings say.
    const QByteArray forced = qgetenv("AVLOOP_GPU_RENDERING");
    if (!forced.isEmpty()) return forced != "off";
    return value(kGpuRendering, QStringLiteral("auto")).toString() != QLatin1String("off");
}

int ramCacheMB() {
    return std::clamp(value(kRamCacheMB, 1536).toInt(), 256, 8192);
}

QString diskCacheFolder() {
    return value(kDiskCacheFolder).toString();
}

int diskCacheGB() {
    return std::clamp(value(kDiskCacheGB, 20).toInt(), 1, 500);
}

QString exportFolder() {
    return value(kExportFolder).toString();
}

QString nameTemplate() {
    const QString t = value(kNameTemplate).toString().trimmed();
    return t.isEmpty() ? QStringLiteral("{source}_loop_{w}x{h}") : t;
}

bool showObsTips() {
    return value(kShowObsTips, true).toBool();
}

QStringList recent(Recent kind) {
    return value(recentKey(kind)).toStringList();
}

void addRecent(Recent kind, const QString& path) {
    QStringList list = recent(kind);
    const QString clean = QDir::toNativeSeparators(QFileInfo(path).absoluteFilePath());
    list.removeIf([&](const QString& p) { return p.compare(clean, Qt::CaseInsensitive) == 0; });
    list.prepend(clean);
    while (list.size() > kRecentMax)
        list.removeLast();
    setValue(recentKey(kind), list);
}

void clearRecent() {
    setValue(kRecentProjects, QStringList());
    setValue(kRecentVideos, QStringList());
}

QString dataDir() {
    const QString forced = qEnvironmentVariable("AVLOOP_DATA_DIR");
    if (!forced.isEmpty()) return made(forced);
    return made(
        QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)).filePath(QStringLiteral("AvLoop")));
}

QString autosaveDir() {
    return made(QDir(dataDir()).filePath(QStringLiteral("autosave")));
}

QString logsDir() {
    return made(QDir(dataDir()).filePath(QStringLiteral("logs")));
}

QString presetsDir() {
    return made(QFileInfo(QSettings().fileName()).absoluteDir().filePath(QStringLiteral("presets")));
}

} // namespace avloop::appsettings

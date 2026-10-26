#pragma once

#include <QString>
#include <QStringList>
#include <QVariant>

namespace avloop::appsettings {

// AvLoop's settings (spec §11.3) in %APPDATA%\AvLoop\AvLoop.ini — a lab run's in its own temporary folder. The keys
// are frozen from AV-010 on (spec §2.7): a key is never renamed, only added.
inline constexpr char kTheme[] = "ui/theme";                                // "navy" | "black"
inline constexpr char kOpenLastProject[] = "general/openLastProject";       // bool, false
inline constexpr char kAutosaveMinutes[] = "general/autosaveMinutes";       // 1–60, 2
inline constexpr char kHardwareDecoding[] = "performance/hardwareDecoding"; // "auto" | "off"
inline constexpr char kGpuRendering[] = "performance/gpuRendering";         // "auto" | "off"
inline constexpr char kRamCacheMB[] = "performance/ramCacheMB";             // 256–8192, 1536
inline constexpr char kDiskCacheFolder[] = "performance/diskCacheFolder";   // "" = %LOCALAPPDATA%\AvLoop\cache
inline constexpr char kDiskCacheGB[] = "performance/diskCacheGB";           // 1–500, 20
inline constexpr char kExportFolder[] = "export/defaultFolder";             // "" = the last folder, else Videos
inline constexpr char kNameTemplate[] = "export/nameTemplate";              // "{source}_loop_{w}x{h}"
inline constexpr char kShowObsTips[] = "export/showObsTips";                // bool, true
inline constexpr char kLastExportFolder[] = "export/lastFolder";
inline constexpr char kRecentProjects[] = "recent/projects"; // the last 10, newest first
inline constexpr char kRecentVideos[] = "recent/videos";

QVariant value(const char* key, const QVariant& fallback = {});
void setValue(const char* key, const QVariant& value);

bool openLastProject();
int autosaveMinutes();
bool hardwareDecoding();
bool gpuRendering();
int ramCacheMB();
QString diskCacheFolder();
int diskCacheGB();
QString exportFolder();
QString nameTemplate();
bool showObsTips();

enum class Recent { Projects, Videos };
QStringList recent(Recent kind);
void addRecent(Recent kind, const QString& path); // first, without a duplicate, at most 10
void clearRecent();

// Where AvLoop keeps its files: %LOCALAPPDATA%\AvLoop (AVLOOP_DATA_DIR when set — a lab run's folder) for the autosave
// and the logs; the presets beside the settings file (%APPDATA%\AvLoop\presets).
QString dataDir();
QString autosaveDir();
QString logsDir();
QString presetsDir();
// The batch queue (spec §10.5): queue.json beside the settings (%APPDATA%\AvLoop), in AVLOOP_DATA_DIR when set.
QString queueFile();

} // namespace avloop::appsettings

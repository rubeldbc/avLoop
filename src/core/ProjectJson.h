#pragma once

#include "core/Project.h"

#include <QJsonObject>
#include <QString>

namespace avloop {

// Everything a project holds, as a value: what a file is read into, what is saved, what an export snapshots.
struct ProjectData {
    SourceInfo source;
    QString relativePath; // the source relative to the project file, as read
    TimeRange range;
    LoopSettings loop;
    OutputSettings output;
    QualitySettings quality;
    VideoTransform transform;
    Keyframes keyframes; // "transform": {"keyframes": […]} (spec §11.1)
    CropShape crop;
    ClipEffects effects; // "effects" (version 2)
    ViewState view;
    QString exportPath;   // "export": {"lastPath"} — the last file this project was exported to
    QJsonObject original; // the JSON as read: unknown keys are written back unchanged (spec §11.1)
};

// The `.avloop` file (spec §11.1): UTF-8 JSON, "format": "avloop", "version": 2 (2 added the clip's effects); times in
// seconds, geometry in scene px. Keys are never renamed (spec §2.7); a format change bumps the version with a
// migration (AV-010).
namespace projectjson {

constexpr int kVersion = 2;

QJsonObject toJson(const ProjectData& data, const QString& projectPath = {});
// Fails on another format or a newer version, with a message that says so; an older one is brought up to date
// (ProjectMigrate).
bool fromJson(const QJsonObject& json, ProjectData& data, QString* error, const QString& projectPath = {});

bool save(const ProjectData& data, const QString& path, QString* error); // atomic (QSaveFile)

// The Loop, Output and Quality sections alone (ProjectSections.cpp): the file's and a preset's (spec §11.3).
QJsonObject loopToJson(const LoopSettings& loop);
LoopSettings loopFromJson(const QJsonObject& json);
QJsonObject outputToJson(const OutputSettings& output);
OutputSettings outputFromJson(const QJsonObject& json);
QJsonObject qualityToJson(const QualitySettings& quality);
QualitySettings qualityFromJson(const QJsonObject& json);
bool load(const QString& path, ProjectData& data, QString* error);

} // namespace projectjson
} // namespace avloop

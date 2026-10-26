#pragma once

#include "core/LoopSettings.h"
#include "core/QualitySettings.h"

#include <QJsonObject>
#include <QList>
#include <QString>

#include <optional>

namespace avloop {

// A named Loop + Output + Quality (spec §11.3): a file `<name>.json` in the presets folder; three are built in for OBS
// (reading R-40: their sizes are long sides, so no crop is stretched).
struct Preset {
    QString name;
    LoopSettings loop;
    OutputSettings output;
    QualitySettings quality;
    bool builtIn = false;

    bool matches(const LoopSettings& l, const OutputSettings& o, const QualitySettings& q) const {
        return loop == l && output == o && quality == q;
    }
};

namespace presets {

QList<Preset> builtIn();
// The built-in ones first, then the folder's by name.
QList<Preset> all(const QString& dir);
bool save(const QString& dir, const Preset& preset, QString* error = nullptr); // atomic; replaces one of that name
bool remove(const QString& dir, const QString& name);                          // never a built-in one
bool isBuiltIn(const QString& name);

QJsonObject toJson(const Preset& preset);
std::optional<Preset> fromJson(const QJsonObject& json);
QString fileName(const QString& name); // the name made a safe file name, ".json"

} // namespace presets
} // namespace avloop

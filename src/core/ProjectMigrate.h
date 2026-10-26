#pragma once

#include <QJsonObject>
#include <QString>

namespace avloop::projectmigrate {

// Brings a project read from disk up to `current` (spec §11.1): each older version's step in turn, so an old file
// opens as if it were new; a newer version is refused with a message that says so. Version 1 is the first format; a
// file without a version (none was ever written) reads as version 1 (reading R-39).
bool upgrade(QJsonObject& json, int current, QString* error);

} // namespace avloop::projectmigrate

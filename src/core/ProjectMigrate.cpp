#include "core/ProjectMigrate.h"

#include <QCoreApplication>

namespace avloop::projectmigrate {

namespace {

// Version 0 → 1: the first format, unchanged; the version written.
void toVersion1(QJsonObject& json) {
    json.insert(QStringLiteral("version"), 1);
}

} // namespace

bool upgrade(QJsonObject& json, int current, QString* error) {
    int version = json.value(QStringLiteral("version")).toInt(0);
    if (version > current) {
        if (error)
            *error = QCoreApplication::translate("ProjectJson",
                                                 "This project was saved by a newer AvLoop (version %1); update AvLoop "
                                                 "to open it.")
                         .arg(version);
        return false;
    }
    if (version < 1) {
        toVersion1(json);
        version = 1;
    }
    // Later versions add their steps here: if (version < 2) { toVersion2(json); version = 2; } …
    return version == current;
}

} // namespace avloop::projectmigrate

#include "core/ProjectMigrate.h"

#include <QCoreApplication>

namespace avloop::projectmigrate {

namespace {

// Version 0 → 1: the first format, unchanged; the version written.
void toVersion1(QJsonObject& json) {
    json.insert(QStringLiteral("version"), 1);
}

// Version 1 → 2: the clip's effects; a file without them has none.
void toVersion2(QJsonObject& json) {
    json.insert(QStringLiteral("version"), 2);
}

// Version 2 → 3: the crop's "invert" (D-021) and what items 20 … 27 add; a file without them has none.
void toVersion3(QJsonObject& json) {
    json.insert(QStringLiteral("version"), 3);
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
    if (version < 2) {
        toVersion2(json);
        version = 2;
    }
    if (version < 3) {
        toVersion3(json);
        version = 3;
    }
    if (version < 4) {
        // Version 3 → 4: "sequence" (D-031) — made from the rest when the file has none (Sequence::fromAvLoop).
        json.insert(QStringLiteral("version"), 4);
        version = 4;
    }
    // Later versions add their steps here: if (version < 5) { toVersion5(json); version = 5; } …
    return version == current;
}

} // namespace avloop::projectmigrate

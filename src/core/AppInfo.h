#pragma once

#include <QString>

namespace avloop {

// The application's identity: the names QSettings and the folders under %APPDATA% / %LOCALAPPDATA% use.
struct AppInfo {
    static QString name();         // "AvLoop"
    static QString organization(); // "AvLoop"
    static QString version();      // from the root CMakeLists.txt
};

} // namespace avloop

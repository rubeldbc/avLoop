#pragma once

#include <QString>

namespace avloop {

// The application's identity: the names QSettings and the folders under %APPDATA% / %LOCALAPPDATA% use. Those never
// change (settings keys are never renamed); what the user reads is displayName().
struct AppInfo {
    static QString name();         // "AvLoop"
    static QString organization(); // "AvLoop"
    static QString displayName();  // "AvLoop by Rubel": the window title, About, the installer
    static QString version();      // from the root CMakeLists.txt
};

} // namespace avloop

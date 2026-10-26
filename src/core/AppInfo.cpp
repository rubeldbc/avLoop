#include "core/AppInfo.h"

#include <AvLoopVersion.h>

namespace avloop {

QString AppInfo::name() {
    return QStringLiteral("AvLoop");
}

QString AppInfo::organization() {
    return QStringLiteral("AvLoop");
}

QString AppInfo::displayName() {
    return QStringLiteral("AvLoop by Rubel");
}

QString AppInfo::version() {
    return QStringLiteral(AVLOOP_VERSION);
}

} // namespace avloop

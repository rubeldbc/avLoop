#include "export/FormatProfiles.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

namespace avloop::exporting {

const QList<FormatProfile>& formatProfiles() {
    static const QList<FormatProfile> profiles = {
        {OutputFormat::WebmVp9, QCoreApplication::translate("FormatProfiles", "WebM VP9"), QStringLiteral("webm"), true,
         60},
        {OutputFormat::WebmVp8, QCoreApplication::translate("FormatProfiles", "WebM VP8"), QStringLiteral("webm"), true,
         60},
    };
    return profiles;
}

const FormatProfile& profileOf(OutputFormat format) {
    for (const FormatProfile& p : formatProfiles())
        if (p.format == format) return p;
    return formatProfiles().front();
}

QString defaultFileName(const QString& sourcePath, QSize size, OutputFormat format, const QString& pattern) {
    const QString base = sourcePath.isEmpty() ? QStringLiteral("loop") : QFileInfo(sourcePath).completeBaseName();
    QString name = pattern;
    name.replace(QStringLiteral("{source}"), base)
        .replace(QStringLiteral("{w}"), QString::number(size.width()))
        .replace(QStringLiteral("{h}"), QString::number(size.height()));
    for (QChar& c : name)
        if (QStringLiteral("<>:\"/\\|?*").contains(c)) c = QLatin1Char('_'); // nothing a file name cannot hold
    return name + QLatin1Char('.') + profileOf(format).extension;
}

QString alternativePath(const QString& path) {
    const QFileInfo info(path);
    const QString stem = info.completeBaseName();
    const QString suffix = info.suffix().isEmpty() ? QString() : QStringLiteral(".") + info.suffix();
    for (int n = 1;; ++n) {
        const QString candidate = info.dir().filePath(QStringLiteral("%1 (%2)%3").arg(stem).arg(n).arg(suffix));
        if (!QFileInfo::exists(candidate)) return candidate;
    }
}

} // namespace avloop::exporting

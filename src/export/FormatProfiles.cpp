#include "export/FormatProfiles.h"

#include "media/EncoderOptions.h"

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
        {OutputFormat::Gif, QCoreApplication::translate("FormatProfiles", "GIF"), QStringLiteral("gif"), true, 50},
        {OutputFormat::Apng, QCoreApplication::translate("FormatProfiles", "APNG"), QStringLiteral("apng"), true, 60},
        {OutputFormat::Mp4H264, QCoreApplication::translate("FormatProfiles", "MP4 H.264"), QStringLiteral("mp4"),
         false, 60},
        {OutputFormat::MovProRes, QCoreApplication::translate("FormatProfiles", "MOV ProRes 4444"),
         QStringLiteral("mov"), true, 60},
    };
    return profiles;
}

const FormatProfile& profileOf(OutputFormat format) {
    for (const FormatProfile& p : formatProfiles())
        if (p.format == format) return p;
    return formatProfiles().front();
}

bool available(OutputFormat format, QString* reason) {
    media::EncodeSettings s;
    s.format = format;
    for (const char* name : {media::encoderName(s), media::fallbackEncoderName(s)})
        if (name && avcodec_find_encoder_by_name(name)) return true;
    if (reason)
        *reason = QCoreApplication::translate("FormatProfiles", "This FFmpeg build has no %1 encoder.")
                      .arg(QString::fromLatin1(media::encoderName(s)));
    return false;
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

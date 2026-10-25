// avloop-cli.exe (spec §2.6, §3.2): the same core without a GUI, for the lab and for scripts.
//   avloop-cli version     AvLoop's version and FFmpeg's libraries, configuration and licence; refuses a GPL build
// Later phases add: probe, frame, export.
#include "core/AppInfo.h"

#include <QCoreApplication>
#include <QStringList>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

#include <cstdio>

namespace {

void out(const QString& line) {
    std::fputs((line + QLatin1Char('\n')).toUtf8().constData(), stdout);
    std::fflush(stdout);
}

QString versionOf(unsigned v) {
    return QStringLiteral("%1.%2.%3").arg(AV_VERSION_MAJOR(v)).arg(AV_VERSION_MINOR(v)).arg(AV_VERSION_MICRO(v));
}

int version() {
    const QString configuration = QString::fromUtf8(avcodec_configuration());
    out(QStringLiteral("AvLoop %1").arg(avloop::AppInfo::version()));
    out(QStringLiteral("FFmpeg %1").arg(QString::fromUtf8(av_version_info())));
    out(QStringLiteral("libavcodec %1 · libavformat %2 · libavutil %3")
            .arg(versionOf(avcodec_version()), versionOf(avformat_version()), versionOf(avutil_version())));
    out(QStringLiteral("licence: %1").arg(QString::fromUtf8(avcodec_license())));
    out(QStringLiteral("configuration: %1").arg(configuration));
    if (configuration.contains(QLatin1String("--enable-gpl")) ||
        configuration.contains(QLatin1String("--enable-nonfree"))) {
        out(QStringLiteral("FAIL: this FFmpeg build is GPL or non-free; AvLoop links LGPL builds only"));
        return 2;
    }
    out(QStringLiteral("PASS: LGPL build (no --enable-gpl)"));
    return 0;
}

int usage() {
    out(QStringLiteral("usage: avloop-cli version"));
    return 1;
}

} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(avloop::AppInfo::name());
    const QStringList args = QCoreApplication::arguments().mid(1);
    if (args.isEmpty()) return usage();
    if (args.first() == QLatin1String("version")) return version();
    return usage();
}

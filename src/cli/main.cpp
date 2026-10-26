// avloop-cli.exe (spec §2.6, §3.2): the same core without a GUI, for the lab and for scripts.
//   avloop-cli version     AvLoop's version and FFmpeg's libraries, configuration and licence; refuses a GPL build
//   avloop-cli probe …     a video's reading (CliMedia.cpp)
//   avloop-cli frame …     a normalized frame as a PNG (CliMedia.cpp); of a project: output frame N (CliRender.cpp)
//   avloop-cli frames …    output frames in a row, round the loop (CliRender.cpp)
//   avloop-cli export …    a project's loop written as the app exports it (CliExport.cpp)
#include "core/AppInfo.h"

#include <QGuiApplication>
#include <QStringList>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

#include <cstdio>
#include <cstring>
#include <memory>

namespace avloop::cli {
int probe(const QStringList& args);
int frame(const QStringList& args);
int renderFrame(const QStringList& args);
int renderFrames(const QStringList& args);
int exportLoop(const QStringList& args);
} // namespace avloop::cli

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
    out(QStringLiteral("usage: avloop-cli version | probe <video> | frame <video|project> --at N -o x.png | "
                       "frames <project> [--count N] | export <project> -o out.webm"));
    return 1;
}

} // namespace

int main(int argc, char** argv) {
    // Rendering a project needs OpenGL, so a GUI application (no window is shown); everything else does not.
    const bool renders = argc > 2 && (std::strcmp(argv[1], "frames") == 0 || std::strcmp(argv[1], "export") == 0 ||
                                      (std::strcmp(argv[1], "frame") == 0 && std::strstr(argv[2], ".avloop")));
    std::unique_ptr<QCoreApplication> app;
    if (renders)
        app = std::make_unique<QGuiApplication>(argc, argv);
    else
        app = std::make_unique<QCoreApplication>(argc, argv);
    QCoreApplication::setApplicationName(avloop::AppInfo::name());
    const QStringList args = QCoreApplication::arguments().mid(1);
    if (args.isEmpty()) return usage();
    if (args.first() == QLatin1String("version")) return version();
    if (args.first() == QLatin1String("probe")) return avloop::cli::probe(args.mid(1));
    if (args.first() == QLatin1String("frames")) return avloop::cli::renderFrames(args.mid(1));
    if (args.first() == QLatin1String("export")) return avloop::cli::exportLoop(args.mid(1));
    if (args.first() == QLatin1String("frame"))
        return renders ? avloop::cli::renderFrame(args.mid(1)) : avloop::cli::frame(args.mid(1));
    return usage();
}

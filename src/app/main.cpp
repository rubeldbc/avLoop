// avloop.exe (spec §3.2): the main window. The lab's entry points (spec §2.6, control/04 §1):
//   --shot <file.png> [--theme navy|black] [--tool <id>]   open maximized, grab the window, exit
//   --walk <script.json> --walk-out <folder>               replay a walk on the real widgets, exit with its verdict
//   --scale <factor>                                       the display scale (S1 at 100/150/200%)
//   --mem-log <file>                                       Private Bytes, VRAM and the cache every second (spec §3.7.8)
// In lab mode the settings live in a temporary folder, never the owner's.
#include "core/AppInfo.h"
#include "media/DiskCache.h"
#include "media/FrameCache.h"
#include "media/SystemMemory.h"
#include "ui/MainWindow.h"
#include "ui/WalkRunner.h"
#include "ui/theme/IconFactory.h"
#include "ui/theme/Theme.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimer>

#include <cstdio>
#include <cstring>
#include <memory>

namespace {

void say(const QString& line) {
    std::fputs((line + QLatin1Char('\n')).toUtf8().constData(), stdout);
    std::fflush(stdout);
}

// --scale must be known before QApplication exists.
void applyScaleArgument(int argc, char** argv) {
    for (int i = 1; i + 1 < argc; ++i)
        if (std::strcmp(argv[i], "--scale") == 0) qputenv("QT_SCALE_FACTOR", argv[i + 1]);
}

} // namespace

int main(int argc, char** argv) {
    QElapsedTimer clock;
    clock.start();
    applyScaleArgument(argc, argv);
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication app(argc, argv);
    QApplication::setOrganizationName(avloop::AppInfo::organization());
    QApplication::setApplicationName(avloop::AppInfo::name());
    QApplication::setApplicationVersion(avloop::AppInfo::version());
    QSettings::setDefaultFormat(QSettings::IniFormat);

    QCommandLineParser cli;
    cli.setApplicationDescription(QApplication::translate("main", "AvLoop — seamless loops for OBS"));
    cli.addHelpOption();
    cli.addVersionOption();
    const QCommandLineOption shot(QStringLiteral("shot"), QStringLiteral("Grab the main window and exit."),
                                  QStringLiteral("file.png"));
    const QCommandLineOption walk(QStringLiteral("walk"), QStringLiteral("Replay a walk script and exit."),
                                  QStringLiteral("script.json"));
    const QCommandLineOption walkOut(QStringLiteral("walk-out"), QStringLiteral("Folder for the walk's pictures."),
                                     QStringLiteral("folder"));
    const QCommandLineOption theme(QStringLiteral("theme"), QStringLiteral("navy or black."), QStringLiteral("name"));
    const QCommandLineOption tool(QStringLiteral("tool"), QStringLiteral("The active tool."), QStringLiteral("id"));
    const QCommandLineOption scale(QStringLiteral("scale"), QStringLiteral("Display scale factor."),
                                   QStringLiteral("factor"));
    const QCommandLineOption memLog(QStringLiteral("mem-log"), QStringLiteral("Log memory every second."),
                                    QStringLiteral("file"));
    cli.addOptions({shot, walk, walkOut, theme, tool, scale, memLog});
    cli.addPositionalArgument(QStringLiteral("file"), QStringLiteral("A video or project to open."));
    cli.process(app);

    const bool lab = cli.isSet(shot) || cli.isSet(walk);
    std::unique_ptr<QTemporaryDir> labSettings;
    if (lab) {
        labSettings = std::make_unique<QTemporaryDir>(QDir::temp().filePath(QStringLiteral("avloop-lab-XXXXXX")));
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, labSettings->path());
    }

    const QString themeName =
        cli.isSet(theme) ? cli.value(theme) : QSettings().value(QStringLiteral("ui/theme")).toString();
    avloop::Theme::instance().apply(avloop::Theme::idFromName(themeName));
    QApplication::setWindowIcon(avloop::IconFactory::appIcon());

    avloop::media::DiskCache::removeStaleSessions();
    avloop::MainWindow window;
    if (cli.isSet(tool)) window.selectTool(cli.value(tool));
    QObject::connect(&window, &avloop::MainWindow::firstShown, &window, [&] {
        if (lab) say(QStringLiteral("startup: %1 ms").arg(clock.elapsed()));
    });
    window.showMaximized();
    if (!cli.positionalArguments().isEmpty()) window.openFiles(cli.positionalArguments());

    // --mem-log: one line a second — ms since start, Private Bytes, VRAM, cache (MB).
    QFile memFile;
    QTimer memTimer;
    if (cli.isSet(memLog)) {
        memFile.setFileName(cli.value(memLog));
        if (memFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            memFile.write("ms,private_mb,vram_mb,cache_mb\n");
            QObject::connect(&memTimer, &QTimer::timeout, &window, [&] {
                const avloop::media::MemorySnapshot m = avloop::media::SystemMemory::read();
                memFile.write(QStringLiteral("%1,%2,%3,%4\n")
                                  .arg(clock.elapsed())
                                  .arg(double(m.privateBytes) / (1 << 20), 0, 'f', 1)
                                  .arg(double(m.vramUsed) / (1 << 20), 0, 'f', 1)
                                  .arg(double(window.frameCache()->used()) / (1 << 20), 0, 'f', 1)
                                  .toLatin1());
                memFile.flush();
            });
            memTimer.start(1000);
        }
    }

    std::unique_ptr<avloop::WalkRunner> runner;
    if (cli.isSet(shot)) {
        const QString file = cli.value(shot);
        QTimer::singleShot(800, &window, [&window, file] {
            const bool ok = window.grab().save(file, "PNG");
            say(QStringLiteral("%1 shot %2").arg(ok ? QStringLiteral("PASS") : QStringLiteral("FAIL"), file));
            window.close();
            QApplication::exit(ok ? 0 : 1);
        });
    } else if (cli.isSet(walk)) {
        runner = std::make_unique<avloop::WalkRunner>(&window, cli.value(walk), cli.value(walkOut));
        QObject::connect(runner.get(), &avloop::WalkRunner::finished, &window, [&window](int failures) {
            window.close();
            QApplication::exit(failures ? 1 : 0);
        });
        QTimer::singleShot(0, runner.get(), &avloop::WalkRunner::start);
    }
    return QApplication::exec();
}

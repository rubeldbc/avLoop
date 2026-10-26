// avloop-cli's rendering (spec §2.6, AVADR-0001): the FrameRenderer the Preview and the export use, from a project.
//   avloop-cli frame <project.avloop> --at N -o x.png [--raw x.rgba] [--cpu]
//       output frame N of the loop; --raw writes the premultiplied RGBA8 bytes as they are
//   avloop-cli frames <project.avloop> [--count N] [--cpu] [--mem-log x.csv]
//       N output frames in a row, round the loop (the lab's speed and memory measure)
#include "core/ProjectJson.h"
#include "media/DiskCache.h"
#include "media/MediaProbe.h"
#include "media/SystemMemory.h"
#include "render/FrameRenderer.h"

#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QStringList>

#include <cstdio>
#include <memory>

namespace avloop::cli {

namespace {

void say(const QString& line) {
    std::fputs((line + QLatin1Char('\n')).toUtf8().constData(), stdout);
    std::fflush(stdout);
}

QString arg(const QStringList& args, const QString& name, const QString& fallback = {}) {
    const qsizetype i = args.indexOf(name);
    return i >= 0 && i + 1 < args.size() ? args[i + 1] : fallback;
}

// The project and its source, made into a renderer; null after saying why.
std::unique_ptr<render::FrameRenderer> rendererFor(const QStringList& args, media::DiskCache* disk) {
    ProjectData project;
    QString error;
    if (!projectjson::load(args.first(), project, &error)) {
        say(QStringLiteral("ERROR %1").arg(error));
        return {};
    }
    const media::MediaProbe::Result probed = media::MediaProbe::probe(project.source.path);
    if (!probed.info) {
        say(QStringLiteral("ERROR %1").arg(probed.error));
        return {};
    }
    project.range = timerange::valid(project.range, probed.info->duration,
                                     project.output.effectiveFps(probed.info->avgFps, probed.info->vfr));
    const render::RenderJob job = render::RenderJob::of(project, *probed.info);
    const bool gpu = !args.contains(QStringLiteral("--cpu")) && render::FrameRenderer::gpuSetting();
    auto r = std::make_unique<render::FrameRenderer>(job, disk, gpu);
    if (!r->open(&error)) {
        say(QStringLiteral("ERROR %1").arg(error));
        return {};
    }
    if (!r->gpuNote().isEmpty()) say(QStringLiteral("note: CPU rendering — %1").arg(r->gpuNote()));
    int shown = -1;
    const bool ok = r->prepare(
        [&shown](int done, int total) {
            const int pct = done * 100 / std::max(1, total);
            if (pct / 10 != shown / 10) say(QStringLiteral("motion interpolation %1%").arg(pct));
            shown = pct;
            return true;
        },
        &error);
    if (!ok) {
        say(QStringLiteral("ERROR %1").arg(error));
        return {};
    }
    return r;
}

} // namespace

int renderFrame(const QStringList& args) {
    if (args.isEmpty() || !args.contains(QStringLiteral("-o"))) {
        say(QStringLiteral("usage: avloop-cli frame <project.avloop> --at N -o x.png [--raw x.rgba] [--cpu]"));
        return 1;
    }
    media::DiskCache disk;
    auto r = rendererFor(args, &disk);
    if (!r) return 2;
    const int k = arg(args, QStringLiteral("--at"), QStringLiteral("0")).toInt();
    QElapsedTimer clock;
    clock.start();
    QImage image;
    QString error;
    if (!r->render(k, image, &error)) {
        say(QStringLiteral("ERROR %1").arg(error));
        return 3;
    }
    const qint64 ms = clock.elapsed();
    const QString png = arg(args, QStringLiteral("-o"));
    if (!image.convertToFormat(QImage::Format_RGBA8888).save(png, "PNG")) {
        say(QStringLiteral("ERROR cannot write %1").arg(png));
        return 4;
    }
    if (const QString raw = arg(args, QStringLiteral("--raw")); !raw.isEmpty()) {
        QFile f(raw);
        if (!f.open(QIODevice::WriteOnly)) return 4;
        for (int y = 0; y < image.height(); ++y)
            f.write(reinterpret_cast<const char*>(image.constScanLine(y)), qint64(image.width()) * 4);
    }
    const render::RenderJob& job = r->job();
    say(QStringLiteral("frame %1 of %2 · %3x%4 · %5 · %6 · %7 ms → %8")
            .arg(k)
            .arg(r->length())
            .arg(image.width())
            .arg(image.height())
            .arg(LoopSettings::modeId(job.loop.mode), r->backendName())
            .arg(ms)
            .arg(QFileInfo(png).fileName()));
    return 0;
}

int renderFrames(const QStringList& args) {
    if (args.isEmpty()) {
        say(QStringLiteral("usage: avloop-cli frames <project.avloop> [--count N] [--cpu] [--mem-log x.csv]"));
        return 1;
    }
    media::DiskCache disk;
    auto r = rendererFor(args, &disk);
    if (!r) return 2;
    const int count = arg(args, QStringLiteral("--count"), QString::number(r->length())).toInt();
    std::unique_ptr<QFile> log;
    if (const QString path = arg(args, QStringLiteral("--mem-log")); !path.isEmpty()) {
        log = std::make_unique<QFile>(path);
        if (!log->open(QIODevice::WriteOnly | QIODevice::Text)) return 4;
        log->write("ms,private_mb,vram_mb,cache_mb\n");
    }
    QElapsedTimer clock;
    clock.start();
    qint64 nextLog = 0;
    QImage image;
    QString error;
    for (int i = 0; i < count; ++i) {
        if (!r->render(i % r->length(), image, &error)) {
            say(QStringLiteral("ERROR %1").arg(error));
            return 3;
        }
        if (log && clock.elapsed() >= nextLog) {
            const media::MemorySnapshot m = media::SystemMemory::read();
            log->write(QStringLiteral("%1,%2,%3,%4\n")
                           .arg(clock.elapsed())
                           .arg(m.privateBytes / 1048576.0, 0, 'f', 1)
                           .arg(m.vramUsed / 1048576.0, 0, 'f', 1)
                           .arg(disk.used() / 1048576.0, 0, 'f', 1)
                           .toLatin1());
            log->flush();
            nextLog += 1000;
        }
    }
    const double seconds = clock.elapsed() / 1000.0;
    say(QStringLiteral("%1 frames of a %2-frame loop · %3x%4 · %5 · %6 s · %7 fps")
            .arg(count)
            .arg(r->length())
            .arg(r->job().outputSize.width())
            .arg(r->job().outputSize.height())
            .arg(r->backendName())
            .arg(seconds, 0, 'f', 1)
            .arg(count / std::max(0.001, seconds), 0, 'f', 1));
    return 0;
}

} // namespace avloop::cli

// avloop-cli's export (spec §2.6, §10.1): the app's ExportJob — the render thread, the export thread, the .part and the
// rename, the check — from a project file.
//   avloop-cli export <project.avloop> -o out.webm [--format webm-vp9|webm-vp8|gif|apng|mp4-h264|mov-prores] [--cpu]
//                     [--repeat N]
//       the loop as Export writes it (--format: in that format instead of the project's); --repeat exports it N times
//       in a row and reports the memory after each
#include "core/ProjectJson.h"
#include "export/ExportJob.h"
#include "media/DiskCache.h"
#include "media/MediaProbe.h"
#include "media/SystemMemory.h"
#include "media/WorkerThread.h"
#include "render/FrameRenderer.h"
#include "render/RenderService.h"

#include <QDir>
#include <QEventLoop>
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

} // namespace

int exportLoop(const QStringList& args) {
    if (args.isEmpty() || !args.contains(QStringLiteral("-o"))) {
        say(QStringLiteral("usage: avloop-cli export <project.avloop> -o out.webm [--format id] [--cpu] [--repeat N]"));
        return 1;
    }
    ProjectData project;
    QString error;
    if (!projectjson::load(args.first(), project, &error)) {
        say(QStringLiteral("ERROR %1").arg(error));
        return 1;
    }
    if (args.contains(QStringLiteral("--format"))) {
        const QString id = arg(args, QStringLiteral("--format"));
        if (formatId(formatFromId(id)) != id) {
            say(QStringLiteral("ERROR unknown format %1").arg(id));
            return 1;
        }
        project.output.format = formatFromId(id);
    }
    const media::MediaProbe::Result probed = media::MediaProbe::probe(project.source.path);
    if (!probed.info) {
        say(QStringLiteral("ERROR %1").arg(probed.error));
        return 1;
    }
    project.range = timerange::valid(project.range, probed.info->duration,
                                     project.output.effectiveFps(probed.info->avgFps, probed.info->vfr));
    const render::RenderJob job = render::RenderJob::of(project, *probed.info);
    const QString out = QDir::current().absoluteFilePath(arg(args, QStringLiteral("-o")));
    const bool gpu = !args.contains(QStringLiteral("--cpu")) && render::FrameRenderer::gpuSetting();
    const int repeat = std::max(1, arg(args, QStringLiteral("--repeat"), QStringLiteral("1")).toInt());

    auto context = std::make_unique<QObject>(); // lives on the export thread; deleted after it stopped
    media::WorkerThread thread(QStringLiteral("AvLoop export"));
    context->moveToThread(&thread);
    media::DiskCache disk;         // Motion Interpolation's frames; outlives the render thread's last task
    render::RenderService service; // gone first: its thread finishes what was posted to it
    int failures = 0;
    for (int round = 1; round <= repeat; ++round) {
        exporting::ExportJob exporter(job, out, &service, context.get(), &disk);
        exporter.setPreferGpu(gpu);
        QEventLoop loop;
        int shown = -1;
        QObject::connect(&exporter, &exporting::ExportJob::progressed, &loop, [&] {
            const int pct = exporter.done() * 100 / std::max(1, exporter.total());
            if (pct / 25 != shown / 25)
                say(QStringLiteral("export %1% (pass %2/%3)").arg(pct).arg(exporter.pass()).arg(exporter.passes()));
            shown = pct;
        });
        QObject::connect(&exporter, &exporting::ExportJob::finished, &loop, &QEventLoop::quit);
        exporter.start();
        loop.exec();
        const exporting::ExportJob::Result& r = exporter.result();
        const bool ok = exporter.state() == exporting::ExportJob::State::Done && r.verifyProblem.isEmpty();
        failures += ok ? 0 : 1;
        if (ok) {
            say(QStringLiteral("PASS export %1: %2x%3, %4 frames at %5/%6, %7 bytes, %8 pass(es), %9 s, %10, %11")
                    .arg(QDir::toNativeSeparators(r.path))
                    .arg(job.outputSize.width())
                    .arg(job.outputSize.height())
                    .arg(job.plan.length)
                    .arg(job.fps.num)
                    .arg(job.fps.den)
                    .arg(r.bytes)
                    .arg(exporter.passes())
                    .arg(r.seconds, 0, 'f', 2)
                    .arg(r.encoder, r.renderer));
            if (r.movedAside) say(QStringLiteral("moved aside: %1 was in use").arg(QDir::toNativeSeparators(out)));
        } else {
            say(QStringLiteral("FAIL export: %1").arg(!r.error.isEmpty() ? r.error : r.verifyProblem));
        }
        if (repeat > 1) {
            const media::MemorySnapshot m = media::SystemMemory::read();
            say(QStringLiteral("memory after export %1: private %2 MB, VRAM %3 MB")
                    .arg(round)
                    .arg(m.privateBytes / 1048576.0, 0, 'f', 1)
                    .arg(m.vramUsed / 1048576.0, 0, 'f', 1));
        }
    }
    return failures == 0 ? 0 : 2;
}

} // namespace avloop::cli

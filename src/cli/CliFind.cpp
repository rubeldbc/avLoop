// avloop-cli's Loop Finder (spec §2.6, §11.5): the app's LoopFinder on a video (the whole picture) or a project (its
// crop, transform and keyframes, its In–Out ± 5 s by default).
//   avloop-cli find <video|project.avloop> [--from s] [--to s] [--min s] [--max s] [--fps n]
//       the best five loops: In, Out, length, score
#include "core/ProjectJson.h"
#include "core/Timecode.h"
#include "export/LoopFinder.h"
#include "media/MediaProbe.h"

#include <QElapsedTimer>
#include <QStringList>

#include <algorithm>
#include <atomic>
#include <cstdio>

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

int findLoops(const QStringList& args) {
    if (args.isEmpty()) {
        say(QStringLiteral("usage: avloop-cli find <video|project.avloop> [--from s] [--to s] [--min s] [--max s]"));
        return 1;
    }
    ProjectData project;
    QString error;
    const bool isProject = args.first().endsWith(QLatin1String(".avloop"), Qt::CaseInsensitive);
    if (isProject && !projectjson::load(args.first(), project, &error)) {
        say(QStringLiteral("ERROR %1").arg(error));
        return 1;
    }
    const media::MediaProbe::Result probed = media::MediaProbe::probe(isProject ? project.source.path : args.first());
    if (!probed.info) {
        say(QStringLiteral("ERROR %1").arg(probed.error));
        return 1;
    }
    const media::MediaInfo& info = *probed.info;
    exporting::LoopFinder::Input in;
    in.source = info;
    const Rational sourceFps = info.avgFps.isValid() ? info.avgFps : info.realFps;
    in.fps = isProject ? project.output.effectiveFps(sourceFps, info.vfr)
                       : OutputSettings{}.effectiveFps(sourceFps, info.vfr);
    const double end = timerange::fileEnd(info.duration, in.fps);
    if (isProject) {
        in.transform = project.transform;
        in.keyframes = project.keyframes;
        const TimeRange r = timerange::valid(project.range, info.duration, in.fps);
        const VideoTransform atIn = keyframes::valueAt(project.keyframes, project.transform, r.in);
        in.frame = project.crop.rect.isEmpty() ? atIn.rect(QSizeF(info.width, info.height)) : project.crop.rect;
        in.from = std::max(0.0, r.in - 5);
        in.to = std::min(end, r.out + 5);
    } else {
        in.transform = VideoTransform::initial(QSizeF(info.width, info.height));
        in.frame = in.transform.rect(QSizeF(info.width, info.height));
        in.to = end;
    }
    in.from = arg(args, QStringLiteral("--from"), QString::number(in.from)).toDouble();
    in.to = std::min(end, arg(args, QStringLiteral("--to"), QString::number(in.to)).toDouble());
    in.minLength = arg(args, QStringLiteral("--min"), QStringLiteral("2")).toDouble();
    in.maxLength = arg(args, QStringLiteral("--max"), QStringLiteral("10")).toDouble();

    QElapsedTimer clock;
    clock.start();
    std::atomic<bool> cancel{false};
    const QList<exporting::LoopFinder::Candidate> found = exporting::LoopFinder::find(in, cancel, {}, &error);
    if (found.isEmpty()) {
        say(QStringLiteral("ERROR %1").arg(error));
        return 2;
    }
    say(QStringLiteral("searched %1 – %2 s, loops of %3 – %4 s at %5 fps in %6 s")
            .arg(in.from, 0, 'f', 2)
            .arg(in.to, 0, 'f', 2)
            .arg(in.minLength)
            .arg(in.maxLength)
            .arg(timecode::rateText(in.fps))
            .arg(clock.elapsed() / 1000.0, 0, 'f', 2));
    for (int i = 0; i < found.size(); ++i)
        say(QStringLiteral("%1. in %2 out %3 length %4 score %5 raw %6")
                .arg(i + 1)
                .arg(found[i].in, 0, 'f', 3)
                .arg(found[i].out, 0, 'f', 3)
                .arg(found[i].out - found[i].in, 0, 'f', 3)
                .arg(found[i].score)
                .arg(found[i].raw, 0, 'f', 3));
    return 0;
}

} // namespace avloop::cli

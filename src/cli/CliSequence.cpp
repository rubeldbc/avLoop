// avloop-cli's sequence (D-031): a project read as AvLoop Studio's sequence — its tracks and clips, made from AvLoop's
// model while the editor moves over (RS-7).
//   avloop-cli sequence <project.avloop> [--save x.avloop]
//       the sequence as one line of JSON (the source probed for its size and length, as the app opens it); --save
//       writes the project in the file's own version to x.avloop
#include "core/ProjectJson.h"
#include "core/Sequence.h"
#include "media/MediaProbe.h"

#include <QJsonDocument>
#include <QStringList>

#include <cstdio>

namespace avloop::cli {

namespace {

void tell(const QString& line) {
    std::fputs((line + QLatin1Char('\n')).toUtf8().constData(), stdout);
    std::fflush(stdout);
}

} // namespace

int sequence(const QStringList& args) {
    if (args.isEmpty()) {
        tell(QStringLiteral("usage: avloop-cli sequence <project.avloop> [--save x.avloop]"));
        return 1;
    }
    ProjectData project;
    QString error;
    if (!projectjson::load(args.first(), project, &error)) {
        tell(QStringLiteral("ERROR %1").arg(error));
        return 2;
    }
    const media::MediaProbe::Result probed = media::MediaProbe::probe(project.source.path);
    if (!probed.info) {
        tell(QStringLiteral("ERROR %1").arg(probed.error));
        return 2;
    }
    const media::MediaInfo& m = *probed.info;
    project.source.width = m.width;
    project.source.height = m.height;
    project.source.fps = m.avgFps.isValid() ? m.avgFps : m.realFps;
    project.source.vfr = m.vfr;
    project.source.duration = m.duration;
    project.source.frameCount = m.frameCount;
    project.source.hasAlpha = m.hasAlpha;
    tell(QString::fromUtf8(QJsonDocument(Sequence::fromAvLoop(project).toJson()).toJson(QJsonDocument::Compact)));
    const qsizetype save = args.indexOf(QStringLiteral("--save"));
    if (save > 0 && save + 1 < args.size()) {
        if (!projectjson::save(project, args[save + 1], &error)) {
            tell(QStringLiteral("ERROR %1").arg(error));
            return 3;
        }
        tell(QStringLiteral("saved %1").arg(args[save + 1]));
    }
    return 0;
}

} // namespace avloop::cli

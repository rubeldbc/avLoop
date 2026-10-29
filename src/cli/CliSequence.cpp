// avloop-cli's sequence (D-031): a project read as AvLoop Studio's sequence — its tracks and clips, made from AvLoop's
// model while the editor moves over (RS-7).
//   avloop-cli sequence <project.avloop> [--save x.avloop] [--at T -o x.png [--raw x.rgba]]
//       the sequence as one line of JSON (the source probed for its size and length, as the app opens it); --save
//       writes the project in the file's own version to x.avloop; --at draws the file's own sequence (else the one made
//       from AvLoop's model) at sequence time T by the composite (D-032), its frame's size, with alpha
#include "core/ProjectJson.h"
#include "core/Sequence.h"
#include "media/MediaProbe.h"
#include "render/SequenceRenderer.h"

#include <QFile>
#include <QImage>
#include <QJsonDocument>
#include <QStringList>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace avloop::cli {

namespace {

void tell(const QString& line) {
    std::fputs((line + QLatin1Char('\n')).toUtf8().constData(), stdout);
    std::fflush(stdout);
}

QString option(const QStringList& args, const QString& name) {
    const qsizetype i = args.indexOf(name);
    return i > 0 && i + 1 < args.size() ? args[i + 1] : QString();
}

// The sequence at `t` into a PNG (and its premultiplied RGBA8 bytes as they are).
int draw(const Sequence& s, double t, const QString& png, const QString& raw) {
    const QSize size(std::max(16, int(std::lround(s.frame.width()))), std::max(16, int(std::lround(s.frame.height()))));
    render::SequenceRenderer r(s, s.frame, size, LoopSettings::Method::Nearest, true, Qt::black);
    QImage img;
    QString error;
    if (!r.render({{t, 1, 0, 0}}, img, &error)) {
        tell(QStringLiteral("ERROR %1").arg(error));
        return 4;
    }
    if (!png.isEmpty() && !img.save(png)) return 4;
    if (!raw.isEmpty()) {
        QFile f(raw);
        if (!f.open(QIODevice::WriteOnly)) return 4;
        for (int y = 0; y < img.height(); ++y)
            f.write(reinterpret_cast<const char*>(img.constScanLine(y)), qint64(img.width()) * 4);
    }
    tell(QStringLiteral("frame %1x%2 at %3 s, %4 decoders")
             .arg(img.width())
             .arg(img.height())
             .arg(t)
             .arg(r.decodersOpen()));
    return 0;
}

} // namespace

int sequence(const QStringList& args) {
    if (args.isEmpty()) {
        tell(QStringLiteral("usage: avloop-cli sequence <project.avloop> [--save x.avloop] [--at T -o x.png]"));
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
    const Sequence derived = Sequence::fromAvLoop(project);
    tell(QString::fromUtf8(QJsonDocument(derived.toJson()).toJson(QJsonDocument::Compact)));
    if (const QString save = option(args, QStringLiteral("--save")); !save.isEmpty()) {
        if (!projectjson::save(project, save, &error)) {
            tell(QStringLiteral("ERROR %1").arg(error));
            return 3;
        }
        tell(QStringLiteral("saved %1").arg(save));
    }
    if (const QString at = option(args, QStringLiteral("--at")); !at.isEmpty()) {
        const QJsonValue own = project.original.value(QStringLiteral("sequence"));
        return draw(own.isObject() ? Sequence::fromJson(own.toObject()) : derived, at.toDouble(),
                    option(args, QStringLiteral("-o")), option(args, QStringLiteral("--raw")));
    }
    return 0;
}

} // namespace avloop::cli

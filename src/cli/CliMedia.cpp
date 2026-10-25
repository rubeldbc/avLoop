// avloop-cli's media commands (spec §2.6):
//   probe <video>                                   MediaProbe's reading as JSON
//   frame <video> --at N -o x.png [--hw on|off] [--size WxH] [--max N]
//                                                   frame N in display order, normalized, as a PNG (unpremultiplied)
#include "media/Decoder.h"
#include "media/MediaProbe.h"

#include <QElapsedTimer>
#include <QFileInfo>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

#include <cstdio>

namespace avloop::cli {

namespace {

void out(const QString& line) {
    std::fputs((line + QLatin1Char('\n')).toUtf8().constData(), stdout);
    std::fflush(stdout);
}

QString option(const QStringList& args, const QString& name, const QString& fallback = {}) {
    const qsizetype at = args.indexOf(name);
    return at >= 0 && at + 1 < args.size() ? args.at(at + 1) : fallback;
}

QString rational(const Rational& r) {
    return QStringLiteral("%1/%2").arg(r.num).arg(r.den);
}

QJsonObject toJson(const media::MediaInfo& i) {
    return QJsonObject{
        {QStringLiteral("path"), i.path},
        {QStringLiteral("container"), i.container},
        {QStringLiteral("containerLong"), i.containerLong},
        {QStringLiteral("codec"), i.codec},
        {QStringLiteral("codecLong"), i.codecLong},
        {QStringLiteral("stream"), i.streamIndex},
        {QStringLiteral("codedWidth"), i.codedWidth},
        {QStringLiteral("codedHeight"), i.codedHeight},
        {QStringLiteral("width"), i.width},
        {QStringLiteral("height"), i.height},
        {QStringLiteral("sar"), rational(i.sar)},
        {QStringLiteral("rotation"), i.rotation},
        {QStringLiteral("flipH"), i.flipH},
        {QStringLiteral("flipV"), i.flipV},
        {QStringLiteral("avgFps"), rational(i.avgFps)},
        {QStringLiteral("realFps"), rational(i.realFps)},
        {QStringLiteral("vfr"), i.vfr},
        {QStringLiteral("duration"), i.duration},
        {QStringLiteral("frameCount"), double(i.frameCount)},
        {QStringLiteral("pixelFormat"), i.pixelFormat},
        {QStringLiteral("bitDepth"), i.bitDepth},
        {QStringLiteral("hasAlpha"), i.hasAlpha},
        {QStringLiteral("hdr"), i.hdr},
        {QStringLiteral("transfer"), i.transfer},
        {QStringLiteral("interlaced"), i.interlaced},
        {QStringLiteral("hardwareCandidate"), i.hardwareCandidate},
        {QStringLiteral("status"), i.statusLine()},
    };
}

} // namespace

int probe(const QStringList& args) {
    if (args.isEmpty()) {
        out(QStringLiteral("usage: avloop-cli probe <video>"));
        return 1;
    }
    const media::MediaProbe::Result r = media::MediaProbe::probe(args.first());
    if (!r.info) {
        out(QStringLiteral("ERROR %1").arg(r.error));
        return 2;
    }
    out(QString::fromUtf8(QJsonDocument(toJson(*r.info)).toJson(QJsonDocument::Indented)));
    return 0;
}

int frame(const QStringList& args) {
    if (args.isEmpty() || !args.contains(QStringLiteral("-o"))) {
        out(QStringLiteral("usage: avloop-cli frame <video> --at N -o x.png [--hw on|off] [--size WxH] [--max N]"));
        return 1;
    }
    const media::MediaProbe::Result r = media::MediaProbe::probe(args.first());
    if (!r.info) {
        out(QStringLiteral("ERROR %1").arg(r.error));
        return 2;
    }
    media::Decoder::Options options;
    options.hardware = option(args, QStringLiteral("--hw"), QStringLiteral("on")) != QLatin1String("off");
    options.maxLongSide = option(args, QStringLiteral("--max"), QStringLiteral("1920")).toInt();
    const QStringList size = option(args, QStringLiteral("--size")).split(QLatin1Char('x'));
    if (size.size() == 2) options.workingSize = QSize(size[0].toInt(), size[1].toInt());
    media::Decoder decoder(*r.info, options);
    QString error;
    if (!decoder.open(&error)) {
        out(QStringLiteral("ERROR %1").arg(error));
        return 2;
    }
    QElapsedTimer clock;
    clock.start();
    const qint64 index = option(args, QStringLiteral("--at"), QStringLiteral("0")).toLongLong();
    const media::Frame f = decoder.frameAtIndex(index);
    if (f.isNull()) {
        out(QStringLiteral("ERROR no frame %1 decodes").arg(index));
        return 3;
    }
    const QImage image(f.data.get(), f.width, f.height, f.stride, QImage::Format_RGBA8888_Premultiplied);
    const QString file = option(args, QStringLiteral("-o"));
    if (!image.convertToFormat(QImage::Format_RGBA8888).save(file, "PNG")) {
        out(QStringLiteral("ERROR cannot write %1").arg(file));
        return 4;
    }
    out(QStringLiteral("frame %1 at %2 s (pts %3): %4x%5 → %6 · %7 ms · %8 · corrupt %9")
            .arg(index)
            .arg(f.seconds, 0, 'f', 4)
            .arg(f.pts)
            .arg(f.width)
            .arg(f.height)
            .arg(QFileInfo(file).fileName())
            .arg(clock.elapsed())
            .arg(decoder.hardwareActive() ? QStringLiteral("hw") : QStringLiteral("sw"))
            .arg(decoder.corruptFrames()));
    if (!decoder.hardwareNote().isEmpty()) out(decoder.hardwareNote());
    return 0;
}

} // namespace avloop::cli

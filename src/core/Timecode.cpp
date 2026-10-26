#include "core/Timecode.h"

#include <QStringList>

#include <algorithm>
#include <cmath>

namespace avloop::timecode {

namespace {

constexpr double kHair = 1e-6; // frames

double rate(Rational fps) {
    return fps.isValid() ? fps.value() : 30.0;
}

// The first grid frame at or after `second`.
std::int64_t firstFrameOf(std::int64_t second, Rational fps) {
    return std::int64_t(std::ceil(double(second) * rate(fps) - kHair));
}

} // namespace

Rational defaultFps(Rational source) {
    if (!source.isValid()) return {30, 1};
    if (source.value() > 60.0 + 1e-9) return {60, 1};
    return source.reduced();
}

double secondsOf(std::int64_t frame, Rational fps) {
    return fps.isValid() ? double(frame) * fps.den / fps.num : double(frame) / 30.0;
}

std::int64_t frameOf(double seconds, Rational fps) {
    return std::int64_t(std::floor(seconds * rate(fps) + kHair));
}

std::int64_t nearestFrame(double seconds, Rational fps) {
    return std::int64_t(std::llround(seconds * rate(fps)));
}

double snap(double seconds, Rational fps) {
    return secondsOf(nearestFrame(seconds, fps), fps);
}

QString rateText(Rational fps) {
    QString s = QString::number(rate(fps), 'f', 3);
    while (s.endsWith(QLatin1Char('0')))
        s.chop(1);
    if (s.endsWith(QLatin1Char('.'))) s.chop(1);
    return s;
}

QString format(double seconds, Rational fps) {
    const std::int64_t n = std::max<std::int64_t>(0, frameOf(seconds, fps));
    const auto whole = std::int64_t(std::floor(secondsOf(n, fps) + 1e-9));
    const std::int64_t ff = n - firstFrameOf(whole, fps);
    return QStringLiteral("%1:%2:%3.%4")
        .arg(whole / 3600, 2, 10, QLatin1Char('0'))
        .arg(whole / 60 % 60, 2, 10, QLatin1Char('0'))
        .arg(whole % 60, 2, 10, QLatin1Char('0'))
        .arg(ff, 2, 10, QLatin1Char('0'));
}

bool parse(const QString& text, Rational fps, double* seconds) {
    const QString s = text.trimmed();
    if (s.isEmpty()) return false;
    bool ok = false;
    if (s.endsWith(QLatin1Char('f'), Qt::CaseInsensitive)) {
        const qlonglong frame = s.chopped(1).trimmed().toLongLong(&ok);
        if (!ok || frame < 0) return false;
        *seconds = secondsOf(frame, fps);
        return true;
    }
    if (!s.contains(QLatin1Char(':'))) {
        const double v = s.toDouble(&ok);
        if (!ok || v < 0) return false;
        *seconds = v;
        return true;
    }
    // [HH:]MM:SS[.FF]
    const QStringList parts = s.split(QLatin1Char(':'));
    if (parts.size() > 3) return false;
    std::int64_t whole = 0;
    for (qsizetype i = 0; i + 1 < parts.size(); ++i) {
        const int v = parts[i].toInt(&ok);
        if (!ok || v < 0) return false;
        whole = whole * 60 + v;
    }
    const QStringList last = parts.last().split(QLatin1Char('.'));
    if (last.size() > 2) return false;
    const int sec = last[0].toInt(&ok);
    if (!ok || sec < 0) return false;
    whole = whole * 60 + sec;
    int ff = 0;
    if (last.size() == 2) {
        ff = last[1].toInt(&ok);
        if (!ok || ff < 0) return false;
    }
    *seconds = secondsOf(firstFrameOf(whole, fps) + ff, fps);
    return true;
}

} // namespace avloop::timecode
